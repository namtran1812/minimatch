import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import {
  runBacktest,
  type BacktestResult,
} from "../api/backtest";
import {
  getChildOrders,
  getFills,
} from "../api/oms";

export default function Backtest() {
  const [symbol, setSymbol] = useState("btcusd");
  const [side, setSide] = useState<"buy" | "sell">("buy");
  const [algorithm, setAlgorithm] = useState<
    "MARKET" | "TWAP" | "VWAP" | "POV" | "ICEBERG"
  >("TWAP");

  const [quantity, setQuantity] = useState(2);
  const [slices, setSlices] = useState(2);
  const [duration, setDuration] = useState(2);
  const [participation, setParticipation] = useState(0.1);
  const [displayedQuantity, setDisplayedQuantity] = useState(1);
  const [feeBps, setFeeBps] = useState(5);

  const [status, setStatus] = useState("READY");
  const [result, setResult] = useState<BacktestResult | null>(null);

  const parentId = result?.parentOrderId ?? null;

  const { data: children = [] } = useQuery({
    queryKey: ["backtest-children", parentId],
    queryFn: () => getChildOrders(parentId!),
    enabled: !!parentId,
    refetchInterval: 1000,
  });

  const { data: fills = [] } = useQuery({
    queryKey: ["backtest-fills", parentId],
    queryFn: () => getFills(parentId!),
    enabled: !!parentId,
    refetchInterval: 1000,
  });

  async function handleRun() {
    try {
      setStatus("RUNNING...");

      const response = await runBacktest({
        symbol,
        side,
        quantity,
        algorithm,
        slices,
        durationSeconds: duration,
        participationRate: participation,
        displayedQuantity,
        takerFeeBps: feeBps,
      });

      setResult(response);
      setStatus(response.complete ? "COMPLETE" : "PARTIAL / CANCELLED");
    } catch {
      setStatus("FAILED");
    }
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
            SIDE
            <select
              value={side}
              onChange={(e) =>
                setSide(e.target.value as "buy" | "sell")
              }
            >
              <option value="buy">BUY</option>
              <option value="sell">SELL</option>
            </select>
          </label>

          <label>
            ALGORITHM
            <select
              value={algorithm}
              onChange={(e) =>
                setAlgorithm(e.target.value as typeof algorithm)
              }
            >
              <option>MARKET</option>
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
              step="0.1"
              value={quantity}
              onChange={(e) => setQuantity(Number(e.target.value))}
            />
          </label>

          {(algorithm === "TWAP" ||
            algorithm === "VWAP" ||
            algorithm === "POV") && (
            <>
              <label>
                SLICES
                <input
                  type="number"
                  value={slices}
                  onChange={(e) => setSlices(Number(e.target.value))}
                />
              </label>

              <label>
                DURATION (SECONDS)
                <input
                  type="number"
                  value={duration}
                  onChange={(e) => setDuration(Number(e.target.value))}
                />
              </label>
            </>
          )}

          {algorithm === "POV" && (
            <label>
              PARTICIPATION RATE
              <input
                type="number"
                step="0.01"
                value={participation}
                onChange={(e) =>
                  setParticipation(Number(e.target.value))
                }
              />
            </label>
          )}

          {algorithm === "ICEBERG" && (
            <label>
              DISPLAY QUANTITY
              <input
                type="number"
                step="0.1"
                value={displayedQuantity}
                onChange={(e) =>
                  setDisplayedQuantity(Number(e.target.value))
                }
              />
            </label>
          )}

          <label>
            TAKER FEE (BPS)
            <input
              type="number"
              step="0.1"
              value={feeBps}
              onChange={(e) => setFeeBps(Number(e.target.value))}
            />
          </label>

          <button onClick={handleRun}>
            RUN BACKTEST
          </button>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Execution Summary</h2>
          </div>

          <div className="risk-item">
            <span>PARENT ID</span>
            <strong>{result?.parentOrderId ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>COMPLETE</span>
            <strong>{result?.complete ? "YES" : "NO"}</strong>
          </div>

          <div className="risk-item">
            <span>REQUESTED</span>
            <strong>{result?.requestedQuantity ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>FILLED</span>
            <strong>{result?.filledQuantity ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>REMAINING</span>
            <strong>{result?.remainingQuantity ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>ARRIVAL PRICE</span>
            <strong>{result?.arrivalPrice ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>AVG FILL PRICE</span>
            <strong>{result?.averageFillPrice ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>MARKET VWAP</span>
            <strong>{result?.marketVwap ?? "—"}</strong>
          </div>
        </div>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">IMPLEMENTATION SHORTFALL</span>
          <strong>
            {result?.implementationShortfallBps ?? "—"} bps
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">VWAP SLIPPAGE</span>
          <strong>
            {result?.vwapSlippageBps ?? "—"} bps
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">TOTAL FEES</span>
          <strong>{result?.totalFees ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">FILLS</span>
          <strong>{result?.fillCount ?? "—"}</strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Child Orders</h2>
          </div>

          {children.length === 0 && (
            <div className="feedback">
              No child orders.
            </div>
          )}

          {children.map((child) => (
            <div className="risk-item" key={child.id}>
              <span>
                #{child.id} · {child.venue}
              </span>
              <strong>
                {child.status} · {child.filledQuantity}/{child.quantity}
              </strong>
            </div>
          ))}
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Fills</h2>
          </div>

          {fills.length === 0 && (
            <div className="feedback">
              No fills.
            </div>
          )}

          {fills.map((fill) => (
            <div className="risk-item" key={fill.id}>
              <span>{fill.venue}</span>
              <strong>
                {fill.quantity} @ {fill.price}
              </strong>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
