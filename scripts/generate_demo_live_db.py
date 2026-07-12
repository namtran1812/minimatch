#!/usr/bin/env python3
import json, math, random, sqlite3, time
from pathlib import Path
root=Path(__file__).resolve().parents[1]; out=root/'data/live'; out.mkdir(parents=True,exist_ok=True)
db=sqlite3.connect(out/'market_events.db', timeout=30); db.execute('PRAGMA busy_timeout=30000'); db.execute('PRAGMA journal_mode=WAL')
db.executescript('''
CREATE TABLE IF NOT EXISTS sessions(session_id TEXT PRIMARY KEY,venue TEXT,symbol TEXT,started_ns INTEGER,ended_ns INTEGER,event_count INTEGER);
CREATE TABLE IF NOT EXISTS market_events(id INTEGER PRIMARY KEY AUTOINCREMENT,session_id TEXT,receive_ts_ns INTEGER,exchange_ts_ns INTEGER,sequence_number INTEGER,venue TEXT,symbol TEXT,event_type TEXT,side TEXT,price REAL,quantity REAL,payload_json TEXT);
''')
for symbol in ('btcusd','btcusdt'):
    session=f'demo-binance-{symbol}'; db.execute('DELETE FROM market_events WHERE session_id=?',(session,)); db.execute('DELETE FROM sessions WHERE session_id=?',(session,))
    start=time.time_ns(); rng=random.Random(20260712 if symbol=='btcusd' else 20260713); mid=63750.0 if symbol=='btcusd' else 60000.0; trades=[]; count=0
    jsonl=out/f'{session}.jsonl'; f=jsonl.open('w')
    for seq in range(1,501):
        mid += math.sin(seq/25)*0.7 + rng.gauss(0,0.35)
        side='BUY' if rng.random()<.5 else 'SELL'; price=round(mid+(0.5 if side=='BUY' else -0.5),2); qty=round(rng.uniform(.001,.08),6)
        ev={'receiveTsNs':start+seq*1_000_000,'exchangeTsNs':start+seq*900_000,'sequence':seq,'type':'trade','side':side,'price':price,'quantity':qty}
        payload=json.dumps(ev,separators=(',',':')); f.write(payload+'\n')
        db.execute('INSERT INTO market_events(session_id,receive_ts_ns,exchange_ts_ns,sequence_number,venue,symbol,event_type,side,price,quantity,payload_json) VALUES(?,?,?,?,?,?,?,?,?,?,?)',(session,ev['receiveTsNs'],ev['exchangeTsNs'],seq,'binance',symbol,'trade',side,price,qty,payload))
        trades.insert(0,{'sequence':seq,'side':side,'price':price,'quantity':qty,'timestampNs':ev['exchangeTsNs']}); trades=trades[:100]; count+=1
    f.close(); end=time.time_ns(); db.execute('INSERT INTO sessions VALUES(?,?,?,?,?,?)',(session,'binance',symbol,start,end,count))
    bids=[{'price':round(mid-i*.5,2),'quantity':round(1.5/(i+1)+rng.random(),6),'orders':1} for i in range(1,21)]
    asks=[{'price':round(mid+i*.5,2),'quantity':round(1.5/(i+1)+rng.random(),6),'orders':1} for i in range(1,21)]
    state={'mode':'DEMO_RECORDED','venue':'binance','symbol':symbol.upper(),'connected':False,'transportConnected':False,'status':'demo database loaded','sessionId':session,'timestampNs':end,'lastEventTsNs':end,'ageMs':0,'lastSequence':500,'sequenceGaps':0,'bids':bids,'asks':asks,'trades':trades,'metrics':{'events':500,'trades':500,'updatesPerSecond':100,'spread':asks[0]['price']-bids[0]['price'],'mid':(asks[0]['price']+bids[0]['price'])/2,'microprice':mid,'uptimeSeconds':5,'reconnects':0,'droppedEvents':0,'duplicateEvents':0}}
    (out/f'binance_{symbol}.json').write_text(json.dumps(state,separators=(',',':')))
db.commit(); db.close()
print(out/'market_events.db'); print(out/'binance_btcusd.json'); print(out/'binance_btcusdt.json')
