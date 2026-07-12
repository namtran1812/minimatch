#!/usr/bin/env python3
"""Import Massive stock minute aggregates into MiniMatch's historical SQLite catalog."""
from __future__ import annotations
import argparse, csv, gzip, json, sqlite3
from pathlib import Path

def open_text(path: Path):
    return gzip.open(path, "rt", newline="") if path.suffix == ".gz" else path.open("r", newline="")

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("file", type=Path)
    p.add_argument("--ticker", default="AAPL")
    p.add_argument("--db", type=Path, default=Path("data/historical/market_history.db"))
    p.add_argument("--json", type=Path, default=None, help="Optional frontend-ready JSON output")
    args = p.parse_args()
    ticker = args.ticker.upper()
    args.db.parent.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(args.db)
    db.execute("PRAGMA journal_mode=WAL")
    db.execute("PRAGMA synchronous=NORMAL")
    db.executescript('''
    CREATE TABLE IF NOT EXISTS historical_bars(
      provider TEXT NOT NULL,
      dataset TEXT NOT NULL,
      ticker TEXT NOT NULL,
      window_start_ns INTEGER NOT NULL,
      open REAL NOT NULL,
      high REAL NOT NULL,
      low REAL NOT NULL,
      close REAL NOT NULL,
      volume REAL NOT NULL,
      transactions INTEGER NOT NULL,
      source_file TEXT NOT NULL,
      PRIMARY KEY(provider,ticker,window_start_ns)
    );
    CREATE INDEX IF NOT EXISTS idx_historical_bars_ticker_time
      ON historical_bars(ticker,window_start_ns);
    ''')
    rows = []
    with open_text(args.file) as fh:
        reader = csv.DictReader(fh)
        required = {"ticker","volume","open","close","high","low","window_start","transactions"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise SystemExit(f"Missing columns: {sorted(missing)}")
        for r in reader:
            if r["ticker"].upper() != ticker:
                continue
            item = (
                "massive", "us_stocks_sip/minute_aggs_v1", ticker,
                int(r["window_start"]), float(r["open"]), float(r["high"]),
                float(r["low"]), float(r["close"]), float(r["volume"]),
                int(r["transactions"]), args.file.name,
            )
            rows.append(item)
    db.executemany('''INSERT OR REPLACE INTO historical_bars
      (provider,dataset,ticker,window_start_ns,open,high,low,close,volume,transactions,source_file)
      VALUES(?,?,?,?,?,?,?,?,?,?,?)''', rows)
    db.commit()
    print(f"Imported {len(rows)} {ticker} bars into {args.db}")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        payload = {"provider":"Massive Flat Files","ticker":ticker,"bars":[{
            "timestampNs":r[3],"open":r[4],"high":r[5],"low":r[6],"close":r[7],"volume":r[8],"transactions":r[9]
        } for r in rows]}
        args.json.write_text(json.dumps(payload, separators=(",",":")))
        print(f"Wrote {args.json}")
    db.close()
if __name__ == "__main__": main()
