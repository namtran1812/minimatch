import { useState } from "react";

export default function Backtest() {
  const [symbol, setSymbol] = useState("BTC-USD");
  const [strategy, setStrategy] = useState("TWAP");
  const [quantity, setQuantity] = useState(1000);
  const [status, setStatus] = useState("READY");

  function runBacktest() {
    setStatus("RUNNING...");

    setTimeout(() => {
      setStatus("COMPLETE");
    }, 800);
  }

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">HISTORICAL EXECUTION</span>
        <h1>Backtest</h1>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Configuration</h2>
          </div>

          <label>
            SYMBOL
            <input
              value={symbol}
              onChange={(e) => setSymbol(e.target.value)}
            />
          </label>

          <label>
            STRATEGY
            <select
              value={strategy}
              onChange={(e) => setStrategy(e.target.value)}
            >
              <option>TWAP</option>
              <option>VWAP</option>
              <option>POV</option>
              <option>ICEBERG</option>
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

          <button onClick={runBacktest}>RUN BACKTEST</button>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Results</h2>
          </div>

          <div className="risk-item">
            <span>FILLED</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>AVG EXECUTION PRICE</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>MARKET VWAP</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>SLIPPAGE</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>FILL RATE</span>
            <strong>—</strong>
          </div>
        </div>
      </div>
    </section>
  );
}
