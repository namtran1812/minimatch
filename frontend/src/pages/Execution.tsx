import { useState } from "react";

type Strategy = "TWAP" | "VWAP" | "POV" | "ICEBERG";

export default function Execution() {
  const [strategy, setStrategy] = useState<Strategy>("TWAP");
  const [quantity, setQuantity] = useState(1000);
  const [slices, setSlices] = useState(10);
  const [participation, setParticipation] = useState(10);
  const [displayQty, setDisplayQty] = useState(100);

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ALGORITHMIC EXECUTION</span>
        <h1>Execution</h1>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Execution Strategy</h2>
          </div>

          <label>
            ALGORITHM
            <select
              value={strategy}
              onChange={(e) => setStrategy(e.target.value as Strategy)}
            >
              <option>TWAP</option>
              <option>VWAP</option>
              <option>POV</option>
              <option>ICEBERG</option>
            </select>
          </label>

          <label>
            PARENT QUANTITY
            <input
              type="number"
              value={quantity}
              onChange={(e) => setQuantity(Number(e.target.value))}
            />
          </label>

          {(strategy === "TWAP" || strategy === "VWAP") && (
            <label>
              SLICES
              <input
                type="number"
                value={slices}
                onChange={(e) => setSlices(Number(e.target.value))}
              />
            </label>
          )}

          {strategy === "POV" && (
            <label>
              PARTICIPATION %
              <input
                type="number"
                value={participation}
                onChange={(e) => setParticipation(Number(e.target.value))}
              />
            </label>
          )}

          {strategy === "ICEBERG" && (
            <label>
              DISPLAY QUANTITY
              <input
                type="number"
                value={displayQty}
                onChange={(e) => setDisplayQty(Number(e.target.value))}
              />
            </label>
          )}

          <button>START EXECUTION</button>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Parent Order</h2>
          </div>

          <div className="risk-item">
            <span>STATUS</span>
            <strong>READY</strong>
          </div>

          <div className="risk-item">
            <span>FILLED</span>
            <strong>0 / {quantity}</strong>
          </div>

          <div className="risk-item">
            <span>AVG PRICE</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>FEES</span>
            <strong>—</strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Child Orders</h2>
          </div>

          <div className="feedback">
            Child-order timeline will populate from the execution API.
          </div>
        </div>
      </div>
    </section>
  );
}
