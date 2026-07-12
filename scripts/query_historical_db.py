#!/usr/bin/env python3
from __future__ import annotations
import argparse, sqlite3
from pathlib import Path
p=argparse.ArgumentParser(); p.add_argument('--ticker',default='AAPL'); p.add_argument('--limit',type=int,default=20); p.add_argument('--db',type=Path,default=Path('data/historical/market_history.db')); a=p.parse_args()
if not a.db.exists(): raise SystemExit(f"Missing {a.db}; import a Massive file first.")
db=sqlite3.connect(a.db)
for row in db.execute('SELECT ticker,window_start_ns,open,high,low,close,volume,transactions FROM historical_bars WHERE ticker=? ORDER BY window_start_ns DESC LIMIT ?', (a.ticker.upper(),a.limit)):
    print(*row, sep=' | ')
