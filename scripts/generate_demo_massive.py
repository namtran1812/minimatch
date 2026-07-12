#!/usr/bin/env python3
from __future__ import annotations
import csv,gzip,math,random,time
from pathlib import Path
out=Path('data/massive/demo/2026-07-10.csv.gz'); out.parent.mkdir(parents=True,exist_ok=True)
rng=random.Random(42); start=1783690200000000000; px=210.0
with gzip.open(out,'wt',newline='') as f:
 w=csv.writer(f); w.writerow(['ticker','volume','open','close','high','low','window_start','transactions'])
 for i in range(390):
  o=px; px=max(1,px+rng.gauss(0,0.18)+0.03*math.sin(i/25)); c=px; h=max(o,c)+abs(rng.gauss(0,.06)); l=min(o,c)-abs(rng.gauss(0,.06)); v=int(max(100,rng.lognormvariate(8.2,.55))); tx=max(1,int(v/45)); w.writerow(['AAPL',v,f'{o:.4f}',f'{c:.4f}',f'{h:.4f}',f'{l:.4f}',start+i*60_000_000_000,tx])
print(out)
