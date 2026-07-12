#!/usr/bin/env python3
"""MiniMatch live market-data adapters.

Streams public Coinbase, Kraken, and Binance.US market data, or authenticated
Alpaca quotes/trades, normalizes them, writes SQLite + JSONL, and publishes
compact state JSON files
consumed by the MiniMatch C++ dashboard.
"""
from __future__ import annotations

import argparse
import asyncio
import json
import os
import signal
import sqlite3
import ssl
import time
import urllib.request
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import websockets

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "live"
DB_PATH = DATA_DIR / "market_events.db"


def now_ns() -> int:
    return time.time_ns()


def atomic_json(path: Path, payload: dict[str, Any]) -> None:
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(payload, separators=(",", ":")))
    tmp.replace(path)


class Recorder:
    def __init__(self, venue: str, symbol: str) -> None:
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        self.venue = venue
        self.symbol = symbol.lower()
        self.session_id = f"{venue}-{self.symbol}-{int(time.time())}"
        self.jsonl_path = DATA_DIR / f"{self.session_id}.jsonl"
        self.db = sqlite3.connect(DB_PATH, timeout=30.0)
        self.db.execute("PRAGMA busy_timeout=30000")
        self.db.execute("PRAGMA journal_mode=WAL")
        self.db.execute("PRAGMA synchronous=NORMAL")
        self.db.executescript(
            """
            CREATE TABLE IF NOT EXISTS sessions (
              session_id TEXT PRIMARY KEY,
              venue TEXT NOT NULL,
              symbol TEXT NOT NULL,
              started_ns INTEGER NOT NULL,
              ended_ns INTEGER,
              event_count INTEGER NOT NULL DEFAULT 0
            );
            CREATE TABLE IF NOT EXISTS market_events (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              session_id TEXT NOT NULL,
              receive_ts_ns INTEGER NOT NULL,
              exchange_ts_ns INTEGER,
              sequence_number INTEGER,
              venue TEXT NOT NULL,
              symbol TEXT NOT NULL,
              event_type TEXT NOT NULL,
              side TEXT,
              price REAL,
              quantity REAL,
              payload_json TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_events_session_seq
              ON market_events(session_id, sequence_number, receive_ts_ns);
            CREATE INDEX IF NOT EXISTS idx_events_symbol_time
              ON market_events(venue, symbol, receive_ts_ns);
            """
        )
        self.db.execute(
            "INSERT OR REPLACE INTO sessions(session_id,venue,symbol,started_ns,event_count) VALUES(?,?,?,?,0)",
            (self.session_id, venue, self.symbol, now_ns()),
        )
        self.db.commit()
        self.count = 0

    def record(self, event: dict[str, Any]) -> None:
        payload = json.dumps(event, separators=(",", ":"))
        with self.jsonl_path.open("a") as out:
            out.write(payload + "\n")
        self.db.execute(
            """INSERT INTO market_events(
                 session_id,receive_ts_ns,exchange_ts_ns,sequence_number,venue,symbol,
                 event_type,side,price,quantity,payload_json
               ) VALUES(?,?,?,?,?,?,?,?,?,?,?)""",
            (
                self.session_id,
                int(event.get("receiveTsNs", now_ns())),
                event.get("exchangeTsNs"),
                event.get("sequence"),
                self.venue,
                self.symbol,
                event.get("type", "unknown"),
                event.get("side"),
                event.get("price"),
                event.get("quantity"),
                payload,
            ),
        )
        self.count += 1
        if self.count % 100 == 0:
            self.db.execute("UPDATE sessions SET event_count=? WHERE session_id=?", (self.count, self.session_id))
            self.db.commit()

    def close(self) -> None:
        self.db.execute(
            "UPDATE sessions SET ended_ns=?,event_count=? WHERE session_id=?",
            (now_ns(), self.count, self.session_id),
        )
        self.db.commit()
        self.db.close()


@dataclass
class LiveState:
    venue: str
    symbol: str
    depth_limit: int = 20
    bids: dict[float, float] = field(default_factory=dict)
    asks: dict[float, float] = field(default_factory=dict)
    trades: deque[dict[str, Any]] = field(default_factory=lambda: deque(maxlen=100))
    connected: bool = False
    status: str = "starting"
    last_sequence: int | None = None
    sequence_gaps: int = 0
    event_count: int = 0
    trade_count: int = 0
    update_rate: float = 0.0
    started: float = field(default_factory=time.monotonic)
    last_publish: float = field(default_factory=time.monotonic)
    publish_events: int = 0
    session_id: str = ""
    last_event_ns: int = 0
    reconnects: int = 0
    dropped_events: int = 0
    duplicate_events: int = 0

    def publish(self) -> None:
        now = time.monotonic()
        elapsed = max(0.001, now - self.last_publish)
        if elapsed >= 1.0:
            self.update_rate = (self.event_count - self.publish_events) / elapsed
            self.publish_events = self.event_count
            self.last_publish = now
        bids = sorted(((p, q) for p, q in self.bids.items() if q > 0), reverse=True)[: self.depth_limit]
        asks = sorted((p, q) for p, q in self.asks.items() if q > 0)[: self.depth_limit]
        best_bid = bids[0][0] if bids else None
        best_ask = asks[0][0] if asks else None
        spread = best_ask - best_bid if best_bid is not None and best_ask is not None else None
        mid = (best_bid + best_ask) / 2 if spread is not None else None
        bid_qty = bids[0][1] if bids else 0.0
        ask_qty = asks[0][1] if asks else 0.0
        micro = ((best_ask * bid_qty + best_bid * ask_qty) / (bid_qty + ask_qty)) if bid_qty + ask_qty and spread is not None else mid
        age_ms = ((now_ns() - self.last_event_ns) / 1_000_000.0) if self.last_event_ns else None
        healthy = bool(self.connected and age_ms is not None and age_ms < 5000)
        effective_status = "live" if healthy else ("stale" if self.connected else self.status)
        payload = {
            "mode": "LIVE",
            "venue": self.venue,
            "symbol": self.symbol.upper(),
            "connected": healthy,
            "transportConnected": self.connected,
            "status": effective_status,
            "sessionId": self.session_id,
            "timestampNs": now_ns(),
            "lastEventTsNs": self.last_event_ns or None,
            "ageMs": age_ms,
            "lastSequence": self.last_sequence,
            "sequenceGaps": self.sequence_gaps,
            "bids": [{"price": p, "quantity": q, "orders": 1} for p, q in bids],
            "asks": [{"price": p, "quantity": q, "orders": 1} for p, q in asks],
            "trades": list(self.trades),
            "metrics": {
                "events": self.event_count,
                "trades": self.trade_count,
                "updatesPerSecond": self.update_rate,
                "spread": spread,
                "mid": mid,
                "microprice": micro,
                "uptimeSeconds": time.monotonic() - self.started,
                "reconnects": self.reconnects,
                "droppedEvents": self.dropped_events,
                "duplicateEvents": self.duplicate_events,
            },
        }
        atomic_json(DATA_DIR / f"{self.venue}_{self.symbol}.json", payload)


async def fetch_json(url: str) -> Any:
    loop = asyncio.get_running_loop()
    def work() -> Any:
        req = urllib.request.Request(url, headers={"User-Agent": "MiniMatch/9"})
        with urllib.request.urlopen(req, timeout=15, context=ssl.create_default_context()) as response:
            return json.loads(response.read())
    return await loop.run_in_executor(None, work)


async def run_binance(symbol: str, recorder: Recorder, state: LiveState) -> None:
    symbol = symbol.lower()
    rest_base = os.getenv("BINANCE_REST_BASE", "https://api.binance.us")
    ws_base = os.getenv("BINANCE_WS_BASE", "wss://stream.binance.us:9443")
    stream = f"{symbol}@depth@100ms/{symbol}@trade/{symbol}@bookTicker"
    snapshot_url = f"{rest_base}/api/v3/depth?symbol={symbol.upper()}&limit=1000"
    while True:
        state.status = "connecting"
        state.publish()
        try:
            async with websockets.connect(f"{ws_base}/stream?streams={stream}", ping_interval=None, close_timeout=5) as ws:
                buffered: list[dict[str, Any]] = []
                snapshot_task = asyncio.create_task(fetch_json(snapshot_url))
                while not snapshot_task.done():
                    msg = json.loads(await asyncio.wait_for(ws.recv(), timeout=10))
                    data = msg.get("data", msg)
                    if data.get("e") == "depthUpdate":
                        buffered.append(data)
                    elif data.get("e") == "trade":
                        process_binance_trade(data, recorder, state)
                    elif "b" in data and "a" in data and "s" in data:
                        process_binance_book_ticker(data, recorder, state)
                snapshot = await snapshot_task
                state.bids = {float(p): float(q) for p, q in snapshot["bids"] if float(q) > 0}
                state.asks = {float(p): float(q) for p, q in snapshot["asks"] if float(q) > 0}
                last_id = int(snapshot["lastUpdateId"])
                for update in buffered:
                    if int(update["u"]) <= last_id:
                        continue
                    if int(update["U"]) <= last_id + 1 <= int(update["u"]):
                        apply_binance_depth(update, recorder, state)
                        last_id = int(update["u"])
                state.last_sequence = last_id
                state.connected = True
                state.status = "live"
                state.publish()
                async for raw in ws:
                    msg = json.loads(raw)
                    data = msg.get("data", msg)
                    if data.get("e") == "depthUpdate":
                        first, final = int(data["U"]), int(data["u"])
                        expected = (state.last_sequence or 0) + 1
                        if final < expected:
                            continue
                        if not (first <= expected <= final):
                            state.sequence_gaps += 1
                            raise RuntimeError(f"sequence gap: expected {expected}, received {first}-{final}")
                        apply_binance_depth(data, recorder, state)
                    elif data.get("e") == "trade":
                        process_binance_trade(data, recorder, state)
                    elif "b" in data and "a" in data and "s" in data:
                        process_binance_book_ticker(data, recorder, state)
                    if state.event_count % 5 == 0:
                        state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False
            state.reconnects += 1
            state.status = f"reconnecting: {exc}"
            state.publish()
            await asyncio.sleep(2)


def apply_binance_depth(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    sequence = int(data["u"])
    for side_name, key, book in (("BUY", "b", state.bids), ("SELL", "a", state.asks)):
        for price_text, qty_text in data[key]:
            price, qty = float(price_text), float(qty_text)
            if qty == 0:
                book.pop(price, None)
            else:
                book[price] = qty
            recorder.record({
                "receiveTsNs": now_ns(), "exchangeTsNs": int(data.get("E", 0)) * 1_000_000,
                "sequence": sequence, "type": "depth", "side": side_name,
                "price": price, "quantity": qty,
            })
            state.event_count += 1
            state.last_event_ns = now_ns()
    state.last_sequence = sequence


def process_binance_trade(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    side = "SELL" if data.get("m") else "BUY"
    trade = {
        "sequence": int(data.get("t", 0)), "side": side,
        "price": float(data["p"]), "quantity": float(data["q"]),
        "timestampNs": int(data.get("T", data.get("E", 0))) * 1_000_000,
    }
    state.trades.appendleft(trade)
    state.trade_count += 1
    state.event_count += 1
    state.last_event_ns = now_ns()
    recorder.record({
        "receiveTsNs": now_ns(), "exchangeTsNs": trade["timestampNs"],
        "sequence": trade["sequence"], "type": "trade", "side": side,
        "price": trade["price"], "quantity": trade["quantity"],
    })


def process_binance_book_ticker(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    bid, bid_qty = float(data["b"]), float(data["B"])
    ask, ask_qty = float(data["a"]), float(data["A"])
    if bid_qty > 0:
        state.bids[bid] = bid_qty
    if ask_qty > 0:
        state.asks[ask] = ask_qty
    sequence = int(data.get("u", state.last_sequence or 0))
    state.last_sequence = max(state.last_sequence or 0, sequence)
    state.event_count += 1
    state.last_event_ns = now_ns()
    recorder.record({
        "receiveTsNs": state.last_event_ns, "exchangeTsNs": None,
        "sequence": sequence, "type": "bookTicker", "side": None,
        "price": (bid + ask) / 2.0, "quantity": bid_qty + ask_qty,
        "bestBid": bid, "bestBidQuantity": bid_qty,
        "bestAsk": ask, "bestAskQuantity": ask_qty,
    })


async def run_coinbase(symbol: str, recorder: Recorder, state: LiveState) -> None:
    """Public Coinbase Advanced Trade L2 + trade feed (no API key required)."""
    product = symbol.upper().replace('/', '-').replace('_', '-')
    if '-' not in product:
        product = {"BTCUSD": "BTC-USD", "ETHUSD": "ETH-USD", "SOLUSD": "SOL-USD"}.get(product, product)
    url = os.getenv("COINBASE_WS_URL", "wss://advanced-trade-ws.coinbase.com")
    while True:
        try:
            state.status = "connecting"
            state.publish()
            async with websockets.connect(url, ping_interval=20, ping_timeout=20, close_timeout=5) as ws:
                for channel in ("level2", "market_trades", "heartbeats"):
                    await ws.send(json.dumps({"type": "subscribe", "product_ids": [product], "channel": channel}))
                state.connected = True
                state.status = "live"
                state.publish()
                async for raw in ws:
                    msg = json.loads(raw)
                    channel = msg.get("channel")
                    seq = msg.get("sequence_num")
                    if isinstance(seq, int):
                        if state.last_sequence is not None and seq > state.last_sequence + 1:
                            state.sequence_gaps += 1
                        state.last_sequence = seq
                    if channel == "l2_data":
                        for event in msg.get("events", []):
                            for update in event.get("updates", []):
                                side_text = str(update.get("side", "")).lower()
                                side = "BUY" if side_text in {"bid", "buy"} else "SELL"
                                price = float(update["price_level"])
                                qty = float(update["new_quantity"])
                                book = state.bids if side == "BUY" else state.asks
                                if qty == 0:
                                    book.pop(price, None)
                                else:
                                    book[price] = qty
                                event_ns = now_ns()
                                recorder.record({
                                    "receiveTsNs": event_ns, "exchangeTsNs": None,
                                    "sequence": seq, "type": "depth", "side": side,
                                    "price": price, "quantity": qty,
                                })
                                state.event_count += 1
                                state.last_event_ns = event_ns
                    elif channel == "market_trades":
                        for event in msg.get("events", []):
                            for item in event.get("trades", []):
                                side = str(item.get("side", "UNKNOWN")).upper()
                                trade = {
                                    "sequence": item.get("trade_id", seq),
                                    "side": side,
                                    "price": float(item["price"]),
                                    "quantity": float(item["size"]),
                                    "timestamp": item.get("time"),
                                }
                                state.trades.appendleft(trade)
                                state.trade_count += 1
                                state.event_count += 1
                                state.last_event_ns = now_ns()
                                recorder.record({
                                    "receiveTsNs": state.last_event_ns, "exchangeTsNs": None,
                                    "sequence": trade["sequence"], "type": "trade", "side": side,
                                    "price": trade["price"], "quantity": trade["quantity"],
                                })
                    if state.event_count % 5 == 0 or channel == "market_trades":
                        state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False
            state.reconnects += 1
            state.status = f"reconnecting: {exc}"
            state.publish()
            await asyncio.sleep(3)


async def run_kraken(symbol: str, recorder: Recorder, state: LiveState) -> None:
    """Public Kraken Spot WebSocket v2 L2 book + trades (no API key required)."""
    pair = symbol.upper().replace('-', '/').replace('_', '/')
    if '/' not in pair:
        pair = {"BTCUSD": "BTC/USD", "ETHUSD": "ETH/USD", "SOLUSD": "SOL/USD"}.get(pair, pair)
    url = os.getenv("KRAKEN_WS_URL", "wss://ws.kraken.com/v2")
    local_seq = 0
    while True:
        try:
            state.status = "connecting"
            state.publish()
            async with websockets.connect(url, ping_interval=20, ping_timeout=20, close_timeout=5) as ws:
                await ws.send(json.dumps({"method": "subscribe", "params": {"channel": "book", "symbol": [pair], "depth": 25, "snapshot": True}}))
                await ws.send(json.dumps({"method": "subscribe", "params": {"channel": "trade", "symbol": [pair], "snapshot": False}}))
                state.connected = True
                state.status = "live"
                state.publish()
                async for raw in ws:
                    msg = json.loads(raw)
                    channel = msg.get("channel")
                    if channel == "book":
                        for payload in msg.get("data", []):
                            for side_name, key, book in (("BUY", "bids", state.bids), ("SELL", "asks", state.asks)):
                                for level in payload.get(key, []):
                                    price = float(level["price"])
                                    qty = float(level["qty"])
                                    if qty == 0:
                                        book.pop(price, None)
                                    else:
                                        book[price] = qty
                                    local_seq += 1
                                    event_ns = now_ns()
                                    recorder.record({
                                        "receiveTsNs": event_ns, "exchangeTsNs": None,
                                        "sequence": local_seq, "type": "depth", "side": side_name,
                                        "price": price, "quantity": qty,
                                    })
                                    state.event_count += 1
                                    state.last_event_ns = event_ns
                            state.last_sequence = local_seq
                    elif channel == "trade":
                        for item in msg.get("data", []):
                            local_seq += 1
                            side = str(item.get("side", "UNKNOWN")).upper()
                            trade = {
                                "sequence": item.get("trade_id", local_seq),
                                "side": side,
                                "price": float(item["price"]),
                                "quantity": float(item["qty"]),
                                "timestamp": item.get("timestamp"),
                            }
                            state.trades.appendleft(trade)
                            state.trade_count += 1
                            state.event_count += 1
                            state.last_event_ns = now_ns()
                            state.last_sequence = local_seq
                            recorder.record({
                                "receiveTsNs": state.last_event_ns, "exchangeTsNs": None,
                                "sequence": trade["sequence"], "type": "trade", "side": side,
                                "price": trade["price"], "quantity": trade["quantity"],
                            })
                    if state.event_count % 5 == 0 or channel == "trade":
                        state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False
            state.reconnects += 1
            state.status = f"reconnecting: {exc}"
            state.publish()
            await asyncio.sleep(3)


async def run_alpaca(symbol: str, recorder: Recorder, state: LiveState) -> None:
    key = os.environ.get("APCA_API_KEY_ID")
    secret = os.environ.get("APCA_API_SECRET_KEY")
    if not key or not secret:
        raise RuntimeError("Set APCA_API_KEY_ID and APCA_API_SECRET_KEY")
    feed = os.environ.get("ALPACA_FEED", "iex")
    url = os.environ.get("ALPACA_WS_URL", f"wss://stream.data.alpaca.markets/v2/{feed}")
    symbol = symbol.upper()
    while True:
        try:
            state.status = "connecting"
            state.publish()
            async with websockets.connect(url, ping_interval=20, ping_timeout=20) as ws:
                await ws.send(json.dumps({"action": "auth", "key": key, "secret": secret}))
                auth = json.loads(await ws.recv())
                if not any(item.get("T") == "success" for item in auth):
                    raise RuntimeError(f"authentication failed: {auth}")
                await ws.send(json.dumps({"action": "subscribe", "trades": [symbol], "quotes": [symbol]}))
                state.connected = True
                state.status = f"live:{feed}"
                state.publish()
                async for raw in ws:
                    for item in json.loads(raw):
                        event_type = item.get("T")
                        if event_type == "q":
                            bid, ask = float(item["bp"]), float(item["ap"])
                            state.bids = {bid: float(item["bs"])}
                            state.asks = {ask: float(item["as"])}
                            state.event_count += 1
                            state.last_event_ns = now_ns()
                            recorder.record({
                                "receiveTsNs": now_ns(), "exchangeTsNs": None,
                                "sequence": None, "type": "quote", "side": None,
                                "price": (bid + ask) / 2, "quantity": float(item["bs"]) + float(item["as"]),
                                "raw": item,
                            })
                        elif event_type == "t":
                            trade = {
                                "sequence": int(item.get("i", state.trade_count + 1)),
                                "side": "UNKNOWN", "price": float(item["p"]),
                                "quantity": float(item["s"]), "timestamp": item.get("t"),
                            }
                            state.trades.appendleft(trade)
                            state.trade_count += 1
                            state.event_count += 1
                            state.last_event_ns = now_ns()
                            recorder.record({
                                "receiveTsNs": now_ns(), "exchangeTsNs": None,
                                "sequence": trade["sequence"], "type": "trade", "side": "UNKNOWN",
                                "price": trade["price"], "quantity": trade["quantity"], "raw": item,
                            })
                        state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False
            state.reconnects += 1
            state.status = f"reconnecting: {exc}"
            state.publish()
            await asyncio.sleep(3)


async def main_async(args: argparse.Namespace) -> None:
    venue = args.venue.lower()
    symbol = args.symbol.lower()
    recorder = Recorder(venue, symbol)
    state = LiveState(venue=venue, symbol=symbol, depth_limit=args.depth)
    state.session_id = recorder.session_id
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, stop.set)
        except NotImplementedError:
            pass
    runners = {
        "coinbase": run_coinbase,
        "kraken": run_kraken,
        "binance": run_binance,
        "alpaca": run_alpaca,
    }
    task = asyncio.create_task(runners[venue](symbol, recorder, state))
    try:
        await stop.wait()
    finally:
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass
        state.connected = False
        state.status = "stopped"
        state.publish()
        recorder.close()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--venue", choices=["coinbase", "kraken", "binance", "alpaca"], required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--depth", type=int, default=20)
    args = parser.parse_args()
    asyncio.run(main_async(args))


if __name__ == "__main__":
    main()
