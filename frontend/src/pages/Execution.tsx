import { useState } from "react";
import { useQuery } from "@tanstack/react-query";

import {
  createExecution,
  getExecutions,
  previewRoute,
  type RoutePreview,
  type RoutedExecution,
} from "../api/executions";

export default function Execution() {
  const [symbol, setSymbol] =
    useState("btcusd");

  const [side, setSide] =
    useState<"buy" | "sell">("buy");

  const [quantity, setQuantity] =
    useState(0.2);

  const [maxSlippageBps, setMaxSlippageBps] =
    useState(100);

  const [maxVenueCount, setMaxVenueCount] =
    useState(3);

  const [fillRatio, setFillRatio] =
    useState(0.8);

  const [
    rejectionProbability,
    setRejectionProbability,
  ] = useState(0);

  const [
    baseLatencyMs,
    setBaseLatencyMs,
  ] = useState(1);

  const [
    latencyJitterMs,
    setLatencyJitterMs,
  ] = useState(2);

  const [seed, setSeed] =
    useState(42);

  const [status, setStatus] =
    useState("READY");

  const [preview, setPreview] =
    useState<RoutePreview | null>(null);

  const [execution, setExecution] =
    useState<RoutedExecution | null>(null);

  const {
    data: history = [],
    refetch,
  } = useQuery({
    queryKey: ["executions"],
    queryFn: getExecutions,
    refetchInterval: 3000,
  });

  async function handlePreview() {
    try {
      setStatus("ROUTING...");

      const result =
        await previewRoute({
          symbol,
          side,
          quantity,
          maxSlippageBps,
          maxVenueCount,
          allOrNone: false,
        });

      setPreview(result);
      setStatus("ROUTE READY");
    } catch {
      setStatus("ROUTE FAILED");
    }
  }

  async function handleExecute() {
    try {
      setStatus("EXECUTING...");

      const result =
        await createExecution({
          symbol,
          side,
          quantity,
          maxSlippageBps,
          maxVenueCount,
          fillRatio,
          rejectionProbability,
          baseLatencyMs,
          latencyJitterMs,
          seed,
        });

      setExecution(result);

      setStatus(
        result.complete
          ? "EXECUTION COMPLETE"
          : "PARTIAL EXECUTION"
      );

      await refetch();

    } catch {
      setStatus("EXECUTION FAILED");
    }
  }

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          SMART ORDER ROUTER
        </span>

        <h1>Execution</h1>
      </div>

      <div className="execution-info">
        <div className="execution-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Execution models how an order is routed across available
            venues. The smart order router evaluates price, fees,
            latency, available liquidity, slippage constraints, and
            venue limits before constructing child executions.
          </p>
        </div>

        <div className="execution-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Configure the parent execution request and preview the
            proposed route first. Then configure fill probability
            and latency assumptions and execute the route to inspect
            simulated child fills, fees, latency, and completion.
          </p>
        </div>

        <div className="execution-info__section">
          <span className="eyebrow">
            ROUTING MODEL
          </span>

          <p>
            Lower effective cost is preferred for buys and higher
            effective proceeds for sells. Maximum slippage and venue
            count constrain the route, while execution simulation
            models partial fills, rejects, and latency.
          </p>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Route Configuration</h2>
          </div>

          <label>
            SYMBOL
            <input
              value={symbol}
              onChange={(e) =>
                setSymbol(e.target.value)
              }
            />
          </label>

          <label>
            SIDE
            <select
              value={side}
              onChange={(e) =>
                setSide(
                  e.target.value as
                    | "buy"
                    | "sell"
                )
              }
            >
              <option value="buy">
                BUY
              </option>

              <option value="sell">
                SELL
              </option>
            </select>
          </label>

          <label>
            QUANTITY
            <input
              type="number"
              step="0.01"
              value={quantity}
              onChange={(e) =>
                setQuantity(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            MAX SLIPPAGE (BPS)
            <input
              type="number"
              value={maxSlippageBps}
              onChange={(e) =>
                setMaxSlippageBps(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            MAX VENUES
            <input
              type="number"
              value={maxVenueCount}
              onChange={(e) =>
                setMaxVenueCount(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <button onClick={handlePreview}>
            PREVIEW ROUTE
          </button>

          <div className="feedback">
            {status}
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>
              Execution Simulation
            </h2>
          </div>

          <label>
            FILL RATIO
            <input
              type="number"
              step="0.1"
              min="0"
              max="1"
              value={fillRatio}
              onChange={(e) =>
                setFillRatio(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            REJECTION PROBABILITY
            <input
              type="number"
              step="0.1"
              min="0"
              max="1"
              value={
                rejectionProbability
              }
              onChange={(e) =>
                setRejectionProbability(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            BASE LATENCY (MS)
            <input
              type="number"
              value={baseLatencyMs}
              onChange={(e) =>
                setBaseLatencyMs(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            LATENCY JITTER (MS)
            <input
              type="number"
              value={latencyJitterMs}
              onChange={(e) =>
                setLatencyJitterMs(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            RANDOM SEED
            <input
              type="number"
              value={seed}
              onChange={(e) =>
                setSeed(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <button onClick={handleExecute}>
            EXECUTE ROUTE
          </button>
        </div>
      </div>

      {preview && (
        <div className="panel">
          <div className="panel-title">
            <h2>Route Preview</h2>
          </div>

          <div className="metrics">
            <div className="metric-card">
              <span className="eyebrow">
                REQUESTED
              </span>

              <strong>
                {
                  preview.requestedQuantity
                }
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                ROUTED
              </span>

              <strong>
                {preview.routedQuantity}
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                AVG PRICE
              </span>

              <strong>
                {preview.averagePrice}
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                EST FEES
              </span>

              <strong>
                {preview.estimatedFees}
              </strong>
            </div>
          </div>

          {preview.legs.map(
            (leg, index) => (
              <div
                className="risk-item"
                key={`${leg.venue}-${index}`}
              >
                <span>
                  {leg.venue.toUpperCase()}
                </span>

                <strong>
                  {leg.quantity} @{" "}
                  {leg.price}
                  {" · "}
                  EFFECTIVE{" "}
                  {leg.effectivePrice}
                  {" · "}
                  {leg.latencyMs} ms
                </strong>
              </div>
            )
          )}
        </div>
      )}

      {execution && (
        <div className="panel">
          <div className="panel-title">
            <h2>
              Latest Execution #
              {execution.executionId}
            </h2>
          </div>

          <div className="metrics">
            <div className="metric-card">
              <span className="eyebrow">
                FILLED
              </span>

              <strong>
                {
                  execution.filledQuantity
                }
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                REMAINING
              </span>

              <strong>
                {
                  execution.remainingQuantity
                }
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                AVG FILL
              </span>

              <strong>
                {
                  execution.averageFillPrice
                }
              </strong>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                TOTAL LATENCY
              </span>

              <strong>
                {
                  execution.totalLatencyMs
                }{" "}
                ms
              </strong>
            </div>
          </div>

          {execution.children.map(
            (child, index) => (
              <div
                className="risk-item"
                key={`${child.venue}-${index}`}
              >
                <span>
                  {child.venue.toUpperCase()}
                </span>

                <strong>
                  {child.status}
                  {" · "}
                  {child.filledQuantity}/
                  {
                    child.requestedQuantity
                  }
                  {" @ "}
                  {child.price}
                  {" · "}
                  {child.latencyMs} ms
                </strong>
              </div>
            )
          )}
        </div>
      )}

      <div className="panel execution-history-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              EXECUTION BLOTTER
            </span>
            <h2>Persistent Execution History</h2>
          </div>

          <span>
            {history.length} RECORDS
          </span>
        </div>

        <div className="execution-history-table">
          <div className="execution-history-table__header">
            <span>ID</span>
            <span>SYMBOL</span>
            <span>SIDE</span>
            <span>FILLED</span>
            <span>AVG FILL</span>
            <span>FEES</span>
            <span>LATENCY</span>
            <span>STATUS</span>
          </div>

          <div className="execution-history-table__body">
            {history.length === 0 && (
              <div className="feedback">
                No executions stored.
              </div>
            )}

            {history.map((item) => (
              <div
                className="execution-history-table__row"
                key={item.executionId}
              >
                <span>#{item.executionId}</span>

                <strong>
                  {item.symbol.toUpperCase()}
                </strong>

                <span>
                  {item.side.toUpperCase()}
                </span>

                <span>
                  {item.filledQuantity}/
                  {item.requestedQuantity}
                </span>

                <span>
                  {item.averageFillPrice.toFixed(2)}
                </span>

                <span>
                  {item.totalFees.toFixed(4)}
                </span>

                <span>
                  {item.totalLatencyMs.toFixed(2)} ms
                </span>

                <strong
                  className={
                    item.complete
                      ? "status-good"
                      : "status-warn"
                  }
                >
                  {item.complete
                    ? "COMPLETE"
                    : "PARTIAL"}
                </strong>
              </div>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
}
