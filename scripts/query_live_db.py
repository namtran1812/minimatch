#!/usr/bin/env python3
import argparse, sqlite3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(); p.add_argument('--limit',type=int,default=20); p.add_argument('--session'); a=p.parse_args()
db=sqlite3.connect(root/'data/live/market_events.db'); db.row_factory=sqlite3.Row
if a.session:
 rows=db.execute('SELECT * FROM market_events WHERE session_id=? ORDER BY id DESC LIMIT?',(a.session,a.limit)).fetchall()
else:
 rows=db.execute('SELECT * FROM market_events ORDER BY id DESC LIMIT?',(a.limit,)).fetchall()
for r in reversed(rows): print(dict(r))
