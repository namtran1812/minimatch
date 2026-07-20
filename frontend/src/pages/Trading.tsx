import { useState } from "react";
import { submitOrder } from "../api/orders";

export default function Trading() {
  const [symbol, setSymbol] = useState("BTC-USD");
  const [side, setSide] = useState<"BUY" | "SELL">("BUY");
  const [type, setType] = useState<
    "LIMIT" | "MARKET" | "IOC" | "FOK" | "POST_ONLY"
  >("LIMIT");
  const [quantity, setQuantity] = useState(10);
  const [price, setPrice] = useState(67842);
  const [strategy, setStrategy] = useState<
    "DIRECT" | "TWAP" | "VWAP" | "POV" | "ICEBERG"
  >("DIRECT");
  const [maxVenues, setMaxVenues] = useState(3);
  const [slippageBps, setSlippageBps] = useState(20);
  const [allOrNone, setAllOrNone] = useState(false);
  const [status, setStatus] = useState("READY");

  async function handleSubmit(event: React.FormEvent) {
    event.preventDefault();

    try {
      setStatus("SUBMITTING...");

      const result = await submitOrder({
        symbol,
        side,
        type,
        quantity,
        price: type === "MARKET" ? undefined : price,
        strategy,
      });

      setStatus(`ACCEPTED · ${result.orderId ?? "ORDER CREATED"}`);
    } catch {
      setStatus("ORDER REJECTED");
    }
  }

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ORDER ENTRY + ROUTING</span>
        <h1>Trading</h1>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>New Order</h2>
          </div>

          <form className="order-form" onSubmit={handleSubmit}>
            <label>
              SYMBOL
              <input
                value={symbol}
                onChange={(e) => setSymbol(e.target.value)}
              />
            </label>

            <label>
              SIDE
              <select
                value={side}
                onChange={(e) =>
                  setSide(e.target.value as "BUY" | "SELL")
                }
              >
                <option>BUY</option>
                <option>SELL</option>
              </select>
            </label>

            <label>
              TYPE
              <select
                value={type}
                onChange={(e) => setType(e.target.value as typeof type)}
              >
                <option>LIMIT</option>
                <option>MARKET</option>
                <option>IOC</option>
                <option>FOK</option>
                <option>POST_ONLY</option>
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
                disabled={type === "MARKET"}
                value={price}
                onChange={(e) => setPrice(Number(e.target.value))}
              />
            </label>

            <label>
              STRATEGY
              <select
                value={strategy}
                onChange={(e) =>
                  setStrategy(e.target.value as typeof strategy)
                }
              >
                <option>DIRECT</option>
                <option>TWAP</option>
                <option>VWAP</option>
                <option>POV</option>
                <option>ICEBERG</option>
              </select>
            </label>

            <button type="submit">SUBMIT ORDER</button>
          </form>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Smart Router Controls</h2>
          </div>

          <label>
            MAX VENUES
            <input
              type="number"
              value={maxVenues}
              onChange={(e) => setMaxVenues(Number(e.target.value))}
            />
          </label>

          <label>
            MAX SLIPPAGE (BPS)
            <input
              type="number"
              value={slippageBps}
              onChange={(e) => setSlippageBps(Number(e.target.value))}
            />
          </label>

          <label>
            <input
              type="checkbox"
              checked={allOrNone}
              onChange={(e) => setAllOrNone(e.target.checked)}
            />
            ALL OR NONE
          </label>

          <div className="feedback">
            Router controls configured locally. Wire these into the backend route request next.
          </div>
        </div>
      </div>
    </section>
  );
}
