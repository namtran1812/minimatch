import { useEffect, useMemo, useState } from "react";
import {
  Activity,
  BarChart3,
  CircleDot,
  Gauge,
  Pause,
  Play,
  RotateCcw,
  Send,
  ShieldCheck,
  Wifi,
} from "lucide-react";
import "./index.css";

type BookLevel = {
  price: number;
  quantity: number;
  total: number;
};

type Trade = {
  id: number;
  time: string;
  side: "BUY" | "SELL";
  price: number;
  quantity: number;
  venue: string;
};

const initialBids: BookLevel[] = [
  { price: 67842.0, quantity: 1.21, total: 1.21 },
  { price: 67841.5, quantity: 0.84, total: 2.05 },
  { price: 67841.0, quantity: 2.44, total: 4.49 },
  { price: 67840.5, quantity: 1.09, total: 5.58 },
  { price: 67840.0, quantity: 3.3, total: 8.88 },
  { price: 67839.5, quantity: 1.75, total: 10.63 },
];

const initialAsks: BookLevel[] = [
  { price: 67845.0, quantity: 1.42, total: 8.71 },
  { price: 67844.5, quantity: 0.71, total: 7.29 },
  { price: 67844.0, quantity: 2.18, total: 6.58 },
  { price: 67843.5, quantity: 1.13, total: 4.4 },
  { price: 67843.0, quantity: 2.06, total: 3.27 },
  { price: 67842.5, quantity: 1.21, total: 1.21 },
];

const seedTrades: Trade[] = [
  {
    id: 1,
    time: "19:42:18.331",
    side: "BUY",
    price: 67842.5,
    quantity: 0.42,
    venue: "COINBASE",
  },
  {
    id: 2,
    time: "19:42:18.104",
    side: "SELL",
    price: 67842.0,
    quantity: 0.16,
    venue: "KRAKEN",
  },
  {
    id: 3,
    time: "19:42:17.954",
    side: "BUY",
    price: 67843.0,
    quantity: 0.74,
    venue: "BINANCE",
  },
];

function Metric({
  label,
  value,
  unit,
}: {
  label: string;
  value: string;
  unit: string;
}) {
  return (
    <div className="metric-card">
      <span className="eyebrow">{label}</span>
      <div>
        <strong>{value}</strong>
        <span className="metric-unit">{unit}</span>
      </div>
    </div>
  );
}

function BookSide({
  levels,
  side,
}: {
  levels: BookLevel[];
  side: "bid" | "ask";
}) {
  return (
    <div className="book-side">
      {levels.map((level) => {
        const width = Math.min(level.total * 9, 100);

        return (
          <div className="book-row" key={`${side}-${level.price}`}>
            <div
              className={`depth-bg ${side}`}
              style={{ width: `${width}%` }}
            />
            <span className={side}>{level.price.toFixed(2)}</span>
            <span>{level.quantity.toFixed(2)}</span>
            <span>{level.total.toFixed(2)}</span>
          </div>
        );
      })}
    </div>
  );
}

function App() {
  const [price, setPrice] = useState(67842.5);
  const [trades, setTrades] = useState<Trade[]>(seedTrades);
  const [replayActive, setReplayActive] = useState(false);
  const [replayProgress, setReplayProgress] = useState(37);
  const [feedback, setFeedback] = useState("READY");
  const [quantity, setQuantity] = useState(10);
  const [orderPrice, setOrderPrice] = useState(67842);
  const [side, setSide] = useState("BUY");
  const [orderType, setOrderType] = useState("LIMIT");
  const [strategy, setStrategy] = useState("DIRECT");

  const spread = useMemo(() => 0.5, []);

  useEffect(() => {
    const timer = setInterval(() => {
      setPrice((old) => {
        const delta = (Math.random() - 0.5) * 1.5;
        return Math.round((old + delta) * 2) / 2;
      });

      const newTrade: Trade = {
        id: Date.now(),
        time: new Date().toLocaleTimeString("en-US", {
          hour12: false,
        }),
        side: Math.random() > 0.5 ? "BUY" : "SELL",
        price: Math.round((67842 + (Math.random() - 0.5) * 8) * 2) / 2,
        quantity: Math.round(Math.random() * 100) / 100,
        venue: ["COINBASE", "KRAKEN", "BINANCE"][
          Math.floor(Math.random() * 3)
        ],
      };

      setTrades((old) => [newTrade, ...old].slice(0, 12));
    }, 1600);

    return () => clearInterval(timer);
  }, []);

  useEffect(() => {
    if (!replayActive) return;

    const timer = setInterval(() => {
      setReplayProgress((old) => {
        if (old >= 100) return 0;
        return old + 1;
      });
    }, 200);

    return () => clearInterval(timer);
  }, [replayActive]);

  function submitOrder(event: React.FormEvent) {
    event.preventDefault();

    setFeedback(
      `${side} ${quantity} ${orderType} @ ${
        orderType === "MARKET" ? "MKT" : orderPrice.toFixed(2)
      } · ${strategy}`
    );

    setTimeout(() => {
      setFeedback("ORDER ACCEPTED · RISK CHECK PASSED");
    }, 350);
  }

  return (
    <div className="app">
      <header className="topbar">
        <div className="brand">
          <div className="logo">MM</div>
          <div>
            <h1>MiniMatch</h1>
            <span>Electronic Trading Systems Lab</span>
          </div>
        </div>

        <div className="ticker">
          <span>BTC-USD</span>
          <strong>${price.toLocaleString()}</strong>
          <em>+0.74%</em>
        </div>

        <div className="connection">
          <span className="live-dot" />
          <Wifi size={15} />
          MARKET DATA ONLINE
        </div>
      </header>

      <main>
        <section className="metrics">
          <Metric label="THROUGHPUT" value="12.64M" unit="ops/sec" />
          <Metric label="P50 LATENCY" value="42" unit="ns" />
          <Metric label="P99 LATENCY" value="167" unit="ns" />
          <Metric label="ACTIVE ORDERS" value="20,037" unit="orders" />
        </section>

        <section className="terminal-grid">
          <div className="left-stack">
            <section className="panel book-panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">MARKET MICROSTRUCTURE</span>
                  <h2>Order Book</h2>
                </div>
                <CircleDot size={17} />
              </div>

              <div className="book-header">
                <span>PRICE</span>
                <span>SIZE</span>
                <span>TOTAL</span>
              </div>

              <BookSide levels={initialAsks} side="ask" />

              <div className="spread">
                <span>SPREAD</span>
                <strong>{spread.toFixed(2)}</strong>
                <span>MID {price.toFixed(2)}</span>
              </div>

              <BookSide levels={initialBids} side="bid" />
            </section>

            <section className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">EXECUTION STREAM</span>
                  <h2>Trade Tape</h2>
                </div>
                <Activity size={17} />
              </div>

              <div className="tape-header">
                <span>TIME</span>
                <span>SIDE</span>
                <span>PRICE</span>
                <span>QTY</span>
                <span>VENUE</span>
              </div>

              <div className="tape">
                {trades.map((trade) => (
                  <div className="tape-row" key={trade.id}>
                    <span>{trade.time}</span>
                    <span className={trade.side.toLowerCase()}>
                      {trade.side}
                    </span>
                    <span>{trade.price.toFixed(2)}</span>
                    <span>{trade.quantity.toFixed(2)}</span>
                    <span>{trade.venue}</span>
                  </div>
                ))}
              </div>
            </section>
          </div>

          <div className="center-stack">
            <section className="panel market-overview">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">REAL-TIME MARKET</span>
                  <h2>Market Overview</h2>
                </div>
                <BarChart3 size={18} />
              </div>

              <div className="chart">
                <svg viewBox="0 0 800 260" preserveAspectRatio="none">
                  <defs>
                    <linearGradient id="chartFill" x1="0" y1="0" x2="0" y2="1">
                      <stop offset="0%" stopColor="#65f1c6" stopOpacity=".24" />
                      <stop offset="100%" stopColor="#65f1c6" stopOpacity="0" />
                    </linearGradient>
                  </defs>

                  <path
                    className="chart-fill"
                    d="M0 210 C80 180 110 195 170 130 C220 78 275 156 330 104 C385 60 420 126 470 88 C530 40 560 96 610 61 C665 20 700 68 800 36 L800 260 L0 260 Z"
                  />

                  <path
                    className="chart-line"
                    d="M0 210 C80 180 110 195 170 130 C220 78 275 156 330 104 C385 60 420 126 470 88 C530 40 560 96 610 61 C665 20 700 68 800 36"
                  />
                </svg>

                <div className="chart-value">
                  <span>MID PRICE</span>
                  <strong>${price.toLocaleString()}</strong>
                </div>
              </div>
            </section>

            <section className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">ORDER ENTRY</span>
                  <h2>Execution Console</h2>
                </div>
                <Send size={17} />
              </div>

              <form className="order-form" onSubmit={submitOrder}>
                <label>
                  SIDE
                  <select
                    value={side}
                    onChange={(e) => setSide(e.target.value)}
                  >
                    <option>BUY</option>
                    <option>SELL</option>
                  </select>
                </label>

                <label>
                  TYPE
                  <select
                    value={orderType}
                    onChange={(e) => setOrderType(e.target.value)}
                  >
                    <option>LIMIT</option>
                    <option>MARKET</option>
                    <option>IOC</option>
                    <option>FOK</option>
                    <option>POST ONLY</option>
                  </select>
                </label>

                <label>
                  QUANTITY
                  <input
                    type="number"
                    value={quantity}
                    onChange={(e) => setQuantity(Number(e.target.value))}
                  />
                </label>

                <label>
                  PRICE
                  <input
                    type="number"
                    value={orderPrice}
                    disabled={orderType === "MARKET"}
                    onChange={(e) => setOrderPrice(Number(e.target.value))}
                  />
                </label>

                <label className="strategy">
                  EXECUTION STRATEGY
                  <select
                    value={strategy}
                    onChange={(e) => setStrategy(e.target.value)}
                  >
                    <option>DIRECT</option>
                    <option>TWAP</option>
                    <option>VWAP</option>
                    <option>POV</option>
                    <option>ICEBERG</option>
                  </select>
                </label>

                <button type="submit">SEND ORDER</button>
              </form>

              <div className="feedback">{feedback}</div>
            </section>

            <section className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">DETERMINISTIC REPLAY</span>
                  <h2>Session Replay</h2>
                </div>
              </div>

              <div className="replay">
                <button onClick={() => setReplayProgress(0)}>
                  <RotateCcw size={16} />
                </button>

                <button onClick={() => setReplayActive(true)}>
                  <Play size={16} />
                </button>

                <button onClick={() => setReplayActive(false)}>
                  <Pause size={16} />
                </button>

                <input
                  type="range"
                  min="0"
                  max="100"
                  value={replayProgress}
                  onChange={(e) =>
                    setReplayProgress(Number(e.target.value))
                  }
                />

                <span>{replayProgress}%</span>
              </div>
            </section>
          </div>

          <aside className="right-stack">
            <section className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">VENUE HEALTH</span>
                  <h2>Connectivity</h2>
                </div>
                <Gauge size={17} />
              </div>

              {["COINBASE", "KRAKEN", "BINANCE.US"].map((venue, i) => (
                <div className="venue" key={venue}>
                  <div>
                    <span className="live-dot" />
                    <strong>{venue}</strong>
                  </div>
                  <span>{[18, 26, 31][i]} ms</span>
                </div>
              ))}
            </section>

            <section className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">PRE-TRADE RISK</span>
                  <h2>Risk State</h2>
                </div>
                <ShieldCheck size={18} />
              </div>

              <div className="risk-item">
                <span>KILL SWITCH</span>
                <strong className="safe">ARMED / SAFE</strong>
              </div>

              <div className="risk-item">
                <span>POSITION</span>
                <strong>28%</strong>
              </div>
              <div className="progress">
                <i style={{ width: "28%" }} />
              </div>

              <div className="risk-item">
                <span>OPEN NOTIONAL</span>
                <strong>41%</strong>
              </div>
              <div className="progress">
                <i style={{ width: "41%" }} />
              </div>

              <div className="risk-item">
                <span>DAILY LOSS</span>
                <strong>-0.18%</strong>
              </div>
              <div className="progress danger">
                <i style={{ width: "18%" }} />
              </div>
            </section>

            <section className="panel system-panel">
              <span className="eyebrow">SYSTEM</span>
              <div className="system-row">
                <span>ENGINE</span>
                <strong>ONLINE</strong>
              </div>
              <div className="system-row">
                <span>EVENT JOURNAL</span>
                <strong>SYNCED</strong>
              </div>
              <div className="system-row">
                <span>OMS</span>
                <strong>HEALTHY</strong>
              </div>
              <div className="system-row">
                <span>FIX SESSION</span>
                <strong>ACTIVE</strong>
              </div>
            </section>
          </aside>
        </section>
      </main>
    </div>
  );
}

export default App;
