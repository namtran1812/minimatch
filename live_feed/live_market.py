#!/usr/bin/env python3
from __future__ import annotations

import argparse
import asyncio
import contextlib
import json
import os
import signal
import sqlite3
import ssl
import time
import urllib.request
import uuid
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Deque, Dict, Optional

import websockets

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "live"
DATA_DIR.mkdir(parents=True, exist_ok=True)
MAX_WS_SIZE = 16 * 1024 * 1024


def now_ns() -> int:
    return time.time_ns()


def atomic_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(payload, separators=(",", ":"), allow_nan=False))
    tmp.replace(path)


async def fetch_json(url: str) -> Any:
    loop = asyncio.get_running_loop()

    def work() -> Any:
        req = urllib.request.Request(url, headers={"User-Agent": "MiniMatch/10"})
        with urllib.request.urlopen(req, timeout=15, context=ssl.create_default_context()) as response:
            return json.loads(response.read())

    return await loop.run_in_executor(None, work)


class Recorder:
    def __init__(self, venue: str, symbol: str) -> None:
        self.venue = venue.lower()
        self.symbol = symbol.lower()
        self.session_id = f"{self.venue}-{self.symbol}-{int(time.time())}-{uuid.uuid4().hex[:8]}"
        self.started_ns = now_ns()
        self.count = 0
        self.last_commit_ns = self.started_ns
        self.jsonl = (DATA_DIR / f"{self.venue}_{self.symbol}.jsonl").open("a", buffering=1)
        self.db = sqlite3.connect(DATA_DIR / "market_events.db", timeout=30, isolation_level=None)
        self.db.execute("PRAGMA journal_mode=WAL")
        self.db.execute("PRAGMA synchronous=NORMAL")
        self.db.execute("PRAGMA busy_timeout=30000")
        self.db.execute("""
            CREATE TABLE IF NOT EXISTS sessions(
                session_id TEXT PRIMARY KEY, venue TEXT NOT NULL, symbol TEXT NOT NULL,
                started_ns INTEGER NOT NULL, ended_ns INTEGER, event_count INTEGER NOT NULL DEFAULT 0
            )
        """)
        self.db.execute("""
            CREATE TABLE IF NOT EXISTS market_events(
                id INTEGER PRIMARY KEY AUTOINCREMENT, session_id TEXT NOT NULL,
                venue TEXT NOT NULL, symbol TEXT NOT NULL, receive_ts_ns INTEGER NOT NULL,
                exchange_ts_ns INTEGER, sequence TEXT, event_type TEXT NOT NULL,
                side TEXT, price REAL, quantity REAL, raw_json TEXT
            )
        """)
        self.db.execute("CREATE INDEX IF NOT EXISTS idx_events_session ON market_events(session_id,id)")
        self.db.execute(
            "INSERT OR REPLACE INTO sessions(session_id,venue,symbol,started_ns,ended_ns,event_count) VALUES(?,?,?,?,NULL,0)",
            (self.session_id, self.venue, self.symbol.upper(), self.started_ns),
        )
        self.db.execute("BEGIN")

    def record(self, event: dict[str, Any]) -> None:
        payload = {"sessionId": self.session_id, "venue": self.venue, "symbol": self.symbol.upper(), **event}
        self.jsonl.write(json.dumps(payload, separators=(",", ":"), default=str) + "\n")
        raw = event.get("raw")
        self.db.execute(
            """INSERT INTO market_events(session_id,venue,symbol,receive_ts_ns,exchange_ts_ns,sequence,event_type,side,price,quantity,raw_json)
               VALUES(?,?,?,?,?,?,?,?,?,?,?)""",
            (
                self.session_id, self.venue, self.symbol.upper(), int(event.get("receiveTsNs") or now_ns()),
                event.get("exchangeTsNs"), None if event.get("sequence") is None else str(event.get("sequence")),
                str(event.get("type", "unknown")), event.get("side"), event.get("price"), event.get("quantity"),
                json.dumps(raw, separators=(",", ":"), default=str) if raw is not None else None,
            ),
        )
        self.count += 1
        current = now_ns()
        if self.count % 200 == 0 or current - self.last_commit_ns >= 1_000_000_000:
            self.db.execute("COMMIT")
            self.db.execute("BEGIN")
            self.last_commit_ns = current

    def close(self) -> None:
        with contextlib.suppress(Exception):
            self.db.execute("COMMIT")
        with contextlib.suppress(Exception):
            self.db.execute(
                "UPDATE sessions SET ended_ns=?,event_count=? WHERE session_id=?",
                (now_ns(), self.count, self.session_id),
            )
        with contextlib.suppress(Exception):
            self.db.close()
        with contextlib.suppress(Exception):
            self.jsonl.close()


@dataclass
class LiveState:
    venue: str
    symbol: str
    depth_limit: int = 20
    session_id: str = ""
    bids: Dict[float, float] = field(default_factory=dict)
    asks: Dict[float, float] = field(default_factory=dict)
    trades: Deque[dict[str, Any]] = field(default_factory=lambda: deque(maxlen=200))
    connected: bool = False
    transport_connected: bool = False
    status: str = "starting"
    started: float = field(default_factory=time.monotonic)
    last_event_ns: Optional[int] = None
    last_sequence: Optional[int] = None
    event_count: int = 0
    trade_count: int = 0
    sequence_gaps: int = 0
    reconnects: int = 0
    dropped_events: int = 0
    duplicate_events: int = 0

    def clear_book(self) -> None:
        self.bids.clear()
        self.asks.clear()

    def validate(self, venue: str) -> None:
        if self.bids and self.asks:
            bid, ask = max(self.bids), min(self.asks)
            if bid >= ask:
                raise RuntimeError(f"{venue} crossed book: best_bid={bid} best_ask={ask}")

    def publish(self) -> None:
        ts = now_ns()
        bids = sorted(self.bids.items(), reverse=True)[: self.depth_limit]
        asks = sorted(self.asks.items())[: self.depth_limit]
        bid = bids[0][0] if bids else None
        ask = asks[0][0] if asks else None
        bq = bids[0][1] if bids else 0.0
        aq = asks[0][1] if asks else 0.0
        spread = ask - bid if bid is not None and ask is not None else None
        mid = (ask + bid) / 2 if bid is not None and ask is not None else None
        micro = ((ask * bq + bid * aq) / (bq + aq)) if mid is not None and bq + aq > 0 else mid
        uptime = max(time.monotonic() - self.started, 1e-9)
        atomic_json(
            DATA_DIR / f"{self.venue}_{self.symbol}.json",
            {
                "mode": "LIVE", "venue": self.venue, "symbol": self.symbol.upper(),
                "connected": self.connected, "transportConnected": self.transport_connected,
                "status": self.status, "sessionId": self.session_id, "timestampNs": ts,
                "lastEventTsNs": self.last_event_ns,
                "ageMs": (ts - self.last_event_ns) / 1_000_000 if self.last_event_ns else None,
                "lastSequence": self.last_sequence, "sequenceGaps": self.sequence_gaps,
                "bids": [{"price": p, "quantity": q, "orders": 1} for p, q in bids],
                "asks": [{"price": p, "quantity": q, "orders": 1} for p, q in asks],
                "trades": list(self.trades),
                "metrics": {
                    "events": self.event_count, "trades": self.trade_count,
                    "updatesPerSecond": self.event_count / uptime, "spread": spread,
                    "mid": mid, "microprice": micro, "uptimeSeconds": uptime,
                    "reconnects": self.reconnects, "droppedEvents": self.dropped_events,
                    "duplicateEvents": self.duplicate_events,
                },
            },
        )


def record_depth(recorder: Recorder, state: LiveState, sequence: Any, side: str, price: float,
                 quantity: float, exchange_ns: Optional[int] = None) -> None:
    event_ns = now_ns()
    recorder.record({"receiveTsNs": event_ns, "exchangeTsNs": exchange_ns, "sequence": sequence,
                     "type": "depth", "side": side, "price": price, "quantity": quantity})
    state.event_count += 1
    state.last_event_ns = event_ns


# ------------------------------ Binance.US ------------------------------

def apply_binance_depth(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    sequence = int(data["u"])
    exchange_ns = int(data.get("E", 0)) * 1_000_000
    for side, key, book in (("BUY", "b", state.bids), ("SELL", "a", state.asks)):
        for price_text, qty_text in data.get(key, []):
            price, qty = float(price_text), float(qty_text)
            if qty == 0.0:
                book.pop(price, None)
            else:
                book[price] = qty
            record_depth(recorder, state, sequence, side, price, qty, exchange_ns)
    state.last_sequence = sequence


def process_binance_trade(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    side = "SELL" if data.get("m") else "BUY"
    event_ns = now_ns()
    trade = {
        "sequence": int(data.get("t", 0)), "side": side, "price": float(data["p"]),
        "quantity": float(data["q"]),
        "timestampNs": int(data.get("T", data.get("E", 0))) * 1_000_000,
    }
    state.trades.appendleft(trade)
    state.trade_count += 1
    state.event_count += 1
    state.last_event_ns = event_ns
    recorder.record({"receiveTsNs": event_ns, "exchangeTsNs": trade["timestampNs"],
                     "sequence": trade["sequence"], "type": "trade", "side": side,
                     "price": trade["price"], "quantity": trade["quantity"]})


def process_binance_ticker(data: dict[str, Any], recorder: Recorder, state: LiveState) -> None:
    # Telemetry only. Never mutate the synchronized depth maps here.
    bid, bq = float(data["b"]), float(data["B"])
    ask, aq = float(data["a"]), float(data["A"])
    event_ns = now_ns()
    state.event_count += 1
    state.last_event_ns = event_ns
    recorder.record({"receiveTsNs": event_ns, "exchangeTsNs": None,
                     "sequence": int(data.get("u", state.last_sequence or 0)),
                     "type": "bookTicker", "side": None, "price": (bid + ask) / 2,
                     "quantity": bq + aq, "bestBid": bid, "bestBidQuantity": bq,
                     "bestAsk": ask, "bestAskQuantity": aq})


async def run_binance(symbol: str, recorder: Recorder, state: LiveState) -> None:
    symbol = symbol.lower()
    rest = os.getenv("BINANCE_REST_BASE", "https://api.binance.us")
    ws_base = os.getenv("BINANCE_WS_BASE", "wss://stream.binance.us:9443")
    streams = f"{symbol}@depth@100ms/{symbol}@trade/{symbol}@bookTicker"
    snapshot_url = f"{rest}/api/v3/depth?symbol={symbol.upper()}&limit=1000"

    while True:
        try:
            state.clear_book(); state.connected = False; state.transport_connected = False
            state.status = "connecting"; state.last_sequence = None; state.publish()
            async with websockets.connect(
                f"{ws_base}/stream?streams={streams}", ping_interval=20, ping_timeout=20,
                max_size=MAX_WS_SIZE, max_queue=256, close_timeout=5,
            ) as ws:
                state.transport_connected = True; state.publish()
                snapshot_task = asyncio.create_task(fetch_json(snapshot_url))
                buffered: Deque[dict[str, Any]] = deque()

                while not snapshot_task.done():
                    data = json.loads(await asyncio.wait_for(ws.recv(), 15))
                    data = data.get("data", data)
                    if data.get("e") == "depthUpdate": buffered.append(data)
                    elif data.get("e") == "trade": process_binance_trade(data, recorder, state)
                    elif {"b", "a", "s"}.issubset(data): process_binance_ticker(data, recorder, state)

                snapshot = await snapshot_task
                state.bids = {float(p): float(q) for p, q in snapshot["bids"] if float(q) > 0}
                state.asks = {float(p): float(q) for p, q in snapshot["asks"] if float(q) > 0}
                snapshot_id = int(snapshot["lastUpdateId"])
                previous: Optional[int] = None

                while previous is None:
                    while buffered and previous is None:
                        update = buffered.popleft()
                        first, final = int(update["U"]), int(update["u"])
                        if final <= snapshot_id: continue
                        if first <= snapshot_id + 1 <= final:
                            apply_binance_depth(update, recorder, state)
                            previous = final
                    if previous is not None: break
                    data = json.loads(await asyncio.wait_for(ws.recv(), 15))
                    data = data.get("data", data)
                    if data.get("e") == "depthUpdate": buffered.append(data)
                    elif data.get("e") == "trade": process_binance_trade(data, recorder, state)
                    elif {"b", "a", "s"}.issubset(data): process_binance_ticker(data, recorder, state)

                while buffered:
                    update = buffered.popleft()
                    first, final = int(update["U"]), int(update["u"])
                    expected = int(previous) + 1
                    if final < expected: continue
                    if first > expected:
                        state.sequence_gaps += 1
                        raise RuntimeError(f"buffered gap expected={expected} got={first}-{final}")
                    apply_binance_depth(update, recorder, state)
                    previous = final

                state.last_sequence = previous
                state.validate("Binance")
                state.connected = True; state.status = "live"; state.publish()

                async for raw in ws:
                    data = json.loads(raw); data = data.get("data", data)
                    if data.get("e") == "depthUpdate":
                        first, final = int(data["U"]), int(data["u"])
                        expected = int(state.last_sequence) + 1
                        if final < expected:
                            state.duplicate_events += 1; continue
                        if first > expected:
                            state.sequence_gaps += 1
                            raise RuntimeError(f"live gap expected={expected} got={first}-{final}")
                        apply_binance_depth(data, recorder, state)
                        state.validate("Binance")
                    elif data.get("e") == "trade": process_binance_trade(data, recorder, state)
                    elif {"b", "a", "s"}.issubset(data): process_binance_ticker(data, recorder, state)
                    if state.event_count % 5 == 0: state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False; state.transport_connected = False; state.clear_book()
            state.reconnects += 1; state.status = f"reconnecting: {exc}"; state.publish()
            await asyncio.sleep(2)


# ------------------------------ Coinbase ------------------------------

def coinbase_product(symbol: str) -> str:
    compact = symbol.upper().replace("-", "").replace("/", "").replace("_", "")
    return {"BTCUSD": "BTC-USD", "ETHUSD": "ETH-USD", "SOLUSD": "SOL-USD",
            "ADAUSD": "ADA-USD"}.get(compact, symbol.upper())


async def run_coinbase(symbol: str, recorder: Recorder, state: LiveState) -> None:
    product = coinbase_product(symbol)
    url = os.getenv("COINBASE_WS_URL", "wss://advanced-trade-ws.coinbase.com")
    while True:
        try:
            state.clear_book(); state.connected = False; state.transport_connected = False
            state.status = "connecting"; state.last_sequence = None; state.publish()
            async with websockets.connect(
                url, open_timeout=15, ping_interval=20, ping_timeout=20,
                max_size=MAX_WS_SIZE, max_queue=128, close_timeout=5,
            ) as ws:
                state.transport_connected = True; state.publish()
                for payload in (
                    {"type": "subscribe", "channel": "level2", "product_ids": [product]},
                    {"type": "subscribe", "channel": "market_trades", "product_ids": [product]},
                    {"type": "subscribe", "channel": "heartbeats"},
                ):
                    await ws.send(json.dumps(payload))
                snapshot = False
                async for raw in ws:
                    msg = json.loads(raw); channel = msg.get("channel"); seq = msg.get("sequence_num")
                    if isinstance(seq, int):
                        if state.last_sequence is not None and seq > state.last_sequence + 1:
                            state.sequence_gaps += 1
                        state.last_sequence = seq
                    if channel == "l2_data":
                        for event in msg.get("events", []):
                            if event.get("type") == "snapshot": state.clear_book()
                            for update in event.get("updates", []):
                                side_text = str(update.get("side", "")).lower()
                                side = "BUY" if side_text in {"bid", "buy"} else "SELL"
                                price, qty = float(update["price_level"]), float(update["new_quantity"])
                                book = state.bids if side == "BUY" else state.asks
                                if qty == 0: book.pop(price, None)
                                else: book[price] = qty
                                record_depth(recorder, state, seq, side, price, qty)
                            if event.get("type") == "snapshot": snapshot = True
                        if snapshot:
                            state.validate("Coinbase"); state.connected = True; state.status = "live"
                    elif channel == "market_trades":
                        for event in msg.get("events", []):
                            for item in event.get("trades", []):
                                event_ns = now_ns(); side = str(item.get("side", "UNKNOWN")).upper()
                                trade = {"sequence": item.get("trade_id", seq), "side": side,
                                         "price": float(item["price"]), "quantity": float(item["size"]),
                                         "timestamp": item.get("time")}
                                state.trades.appendleft(trade); state.trade_count += 1
                                state.event_count += 1; state.last_event_ns = event_ns
                                recorder.record({"receiveTsNs": event_ns, "exchangeTsNs": None,
                                                 "sequence": trade["sequence"], "type": "trade", "side": side,
                                                 "price": trade["price"], "quantity": trade["quantity"]})
                    if state.event_count % 5 == 0 or channel == "market_trades": state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False; state.transport_connected = False; state.clear_book()
            state.reconnects += 1; state.status = f"reconnecting: {exc}"; state.publish()
            await asyncio.sleep(3)


# ------------------------------ Kraken ------------------------------

def kraken_pair(symbol: str) -> str:
    value = symbol.upper().replace("-", "/").replace("_", "/")
    if "/" in value: return value
    return {"BTCUSD": "BTC/USD", "ETHUSD": "ETH/USD", "SOLUSD": "SOL/USD",
            "ADAUSD": "ADA/USD"}.get(value, value)


async def run_kraken(symbol: str, recorder: Recorder, state: LiveState) -> None:
    pair = kraken_pair(symbol)
    url = os.getenv("KRAKEN_WS_URL", "wss://ws.kraken.com/v2")
    depth = 25 if state.depth_limit <= 25 else 100
    local_seq = 0

    def truncate() -> None:
        state.bids = dict(sorted(state.bids.items(), reverse=True)[:depth])
        state.asks = dict(sorted(state.asks.items())[:depth])

    while True:
        try:
            state.clear_book(); state.connected = False; state.transport_connected = False
            state.status = "connecting"; state.publish()
            async with websockets.connect(
                url, open_timeout=15, ping_interval=20, ping_timeout=20,
                max_size=MAX_WS_SIZE, max_queue=128, close_timeout=5,
            ) as ws:
                state.transport_connected = True; state.publish()
                await ws.send(json.dumps({"method": "subscribe", "params": {
                    "channel": "book", "symbol": [pair], "depth": depth, "snapshot": True}}))
                await ws.send(json.dumps({"method": "subscribe", "params": {
                    "channel": "trade", "symbol": [pair], "snapshot": False}}))
                snapshot = False
                async for raw in ws:
                    msg = json.loads(raw); channel = msg.get("channel"); msg_type = msg.get("type")
                    if channel == "book":
                        for payload in msg.get("data", []):
                            if msg_type == "snapshot": state.clear_book()
                            for side, key, book in (("BUY", "bids", state.bids), ("SELL", "asks", state.asks)):
                                for level in payload.get(key, []):
                                    price, qty = float(level["price"]), float(level["qty"])
                                    if qty == 0: book.pop(price, None)
                                    else: book[price] = qty
                                    local_seq += 1
                                    record_depth(recorder, state, local_seq, side, price, qty)
                            truncate(); state.validate("Kraken"); state.last_sequence = local_seq
                            if msg_type == "snapshot":
                                snapshot = True; state.connected = True; state.status = "live"
                    elif channel == "trade":
                        for item in msg.get("data", []):
                            local_seq += 1; event_ns = now_ns(); side = str(item.get("side", "UNKNOWN")).upper()
                            trade = {"sequence": item.get("trade_id", local_seq), "side": side,
                                     "price": float(item["price"]), "quantity": float(item["qty"]),
                                     "timestamp": item.get("timestamp")}
                            state.trades.appendleft(trade); state.trade_count += 1
                            state.event_count += 1; state.last_event_ns = event_ns; state.last_sequence = local_seq
                            recorder.record({"receiveTsNs": event_ns, "exchangeTsNs": None,
                                             "sequence": trade["sequence"], "type": "trade", "side": side,
                                             "price": trade["price"], "quantity": trade["quantity"]})
                    if snapshot and (state.event_count % 5 == 0 or channel == "trade"): state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False; state.transport_connected = False; state.clear_book()
            state.reconnects += 1; state.status = f"reconnecting: {exc}"; state.publish()
            await asyncio.sleep(3)


# ------------------------------ Alpaca ------------------------------

async def run_alpaca(symbol: str, recorder: Recorder, state: LiveState) -> None:
    key, secret = os.getenv("APCA_API_KEY_ID"), os.getenv("APCA_API_SECRET_KEY")
    if not key or not secret:
        state.status = "coming soon: configure Alpaca API credentials"; state.publish()
        while True: await asyncio.sleep(3600)
    feed = os.getenv("ALPACA_FEED", "iex")
    url = os.getenv("ALPACA_WS_URL", f"wss://stream.data.alpaca.markets/v2/{feed}")
    symbol = symbol.upper()
    while True:
        try:
            state.clear_book(); state.status = "connecting"; state.publish()
            async with websockets.connect(url, max_size=MAX_WS_SIZE, ping_interval=20, ping_timeout=20) as ws:
                state.transport_connected = True
                await ws.send(json.dumps({"action": "auth", "key": key, "secret": secret}))
                auth = json.loads(await ws.recv())
                if not any(x.get("T") == "success" for x in auth): raise RuntimeError(f"auth failed: {auth}")
                await ws.send(json.dumps({"action": "subscribe", "trades": [symbol], "quotes": [symbol]}))
                state.connected = True; state.status = f"live:{feed}"; state.publish()
                async for raw in ws:
                    for item in json.loads(raw):
                        event_ns = now_ns()
                        if item.get("T") == "q":
                            bid, ask = float(item["bp"]), float(item["ap"])
                            state.bids = {bid: float(item["bs"])}; state.asks = {ask: float(item["as"])}
                            state.event_count += 1; state.last_event_ns = event_ns
                        elif item.get("T") == "t":
                            trade = {"sequence": item.get("i", state.trade_count + 1), "side": "UNKNOWN",
                                     "price": float(item["p"]), "quantity": float(item["s"]),
                                     "timestamp": item.get("t")}
                            state.trades.appendleft(trade); state.trade_count += 1
                            state.event_count += 1; state.last_event_ns = event_ns
                        state.publish()
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            state.connected = False; state.transport_connected = False; state.clear_book()
            state.reconnects += 1; state.status = f"reconnecting: {exc}"; state.publish()
            await asyncio.sleep(3)


RUNNERS = {"coinbase": run_coinbase, "kraken": run_kraken, "binance": run_binance, "alpaca": run_alpaca}


async def main_async(args: argparse.Namespace) -> None:
    venue, symbol = args.venue.lower(), args.symbol.lower()
    recorder = Recorder(venue, symbol)
    state = LiveState(venue=venue, symbol=symbol, depth_limit=args.depth, session_id=recorder.session_id)
    state.publish()
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        with contextlib.suppress(NotImplementedError): loop.add_signal_handler(sig, stop.set)
    task = asyncio.create_task(RUNNERS[venue](symbol, recorder, state))
    waiter = asyncio.create_task(stop.wait())
    try:
        done, _ = await asyncio.wait([task, waiter], return_when=asyncio.FIRST_COMPLETED)
        if task in done and task.exception(): raise task.exception()
    finally:
        task.cancel(); waiter.cancel()
        with contextlib.suppress(asyncio.CancelledError): await task
        state.connected = False; state.transport_connected = False; state.status = "stopped"; state.publish()
        recorder.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="MiniMatch live market adapter")
    parser.add_argument("--venue", choices=sorted(RUNNERS), required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--depth", type=int, default=20)
    args = parser.parse_args()
    if args.depth <= 0: parser.error("--depth must be positive")
    asyncio.run(main_async(args))


if __name__ == "__main__":
    main()
