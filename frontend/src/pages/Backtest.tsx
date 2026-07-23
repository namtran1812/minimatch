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

import BacktestPriceChart from "../components/charts/BacktestPriceChart";

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

      <div className="backtest-info">
        <div className="backtest-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Backtest runs MiniMatch execution algorithms
            against recorded market data and measures how
            the resulting fills compare with arrival price
            and historical market VWAP.
          </p>
        </div>

        <div className="backtest-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Choose a symbol, side, algorithm, quantity,
            schedule settings, and fees. Run the simulation,
            then inspect fills, slippage, execution cost,
            and child-order behavior.
          </p>
        </div>

        <div className="backtest-info__section">
          <span className="eyebrow">
            MODEL ASSUMPTIONS
          </span>

          <p>
            The run uses recorded replay market data.
            VWAP and POV currently use an equal synthetic
            volume profile across execution slices.
          </p>
        </div>
      </div>

      <div className="backtest-status-strip">
        <div>
          <span>STATUS</span>

          <strong>
            {status}
          </strong>
        </div>

        <div>
          <span>ALGORITHM</span>

          <strong>
            {algorithm}
          </strong>
        </div>

        <div>
          <span>SIDE</span>

          <strong>
            {side.toUpperCase()}
          </strong>
        </div>

        <div>
          <span>PARENT</span>

          <strong>
            {result?.parentOrderId ?? "—"}
          </strong>
        </div>
      </div>

      <div className="backtest-workspace">
        <div className="panel backtest-config-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                RUN CONFIGURATION
              </span>

              <h2>Execution Model</h2>
            </div>

            <strong>
              {algorithm}
            </strong>
          </div>

          <div className="backtest-config-fields">

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

          </div>

          <button
            type="button"
            className="backtest-run-button"
            onClick={handleRun}
          >
            RUN BACKTEST
          </button>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel backtest-summary-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                RUN RESULT
              </span>

              <h2>Execution Summary</h2>
            </div>

            <strong>
              {result
                ? result.complete
                  ? "COMPLETE"
                  : "PARTIAL"
                : "WAITING"}
            </strong>
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

          <div className="backtest-completion">
            <div className="backtest-completion__header">
              <span>FILL COMPLETION</span>

              <strong>
                {result && result.requestedQuantity > 0
                  ? `${(
                      result.filledQuantity /
                      result.requestedQuantity *
                      100
                    ).toFixed(1)}%`
                  : "—"}
              </strong>
            </div>

            <div className="backtest-completion__track">
              <div
                className="backtest-completion__fill"
                style={{
                  width: `${
                    result && result.requestedQuantity > 0
                      ? Math.min(
                          100,
                          result.filledQuantity /
                            result.requestedQuantity *
                            100
                        )
                      : 0
                  }%`,
                }}
              />
            </div>

            <div className="backtest-completion__labels">
              <span>
                FILLED {result?.filledQuantity ?? "—"}
              </span>

              <span>
                REM {result?.remainingQuantity ?? "—"}
              </span>
            </div>
          </div>
        </div>
      </div>

      <div className="metrics backtest-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            IMPLEMENTATION SHORTFALL
          </span>

          <strong>
            {result
              ? `${result.implementationShortfallBps.toFixed(2)} bps`
              : "—"}
          </strong>

          <span className="metric-detail">
            VS ARRIVAL PRICE
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            VWAP SLIPPAGE
          </span>

          <strong>
            {result
              ? `${result.vwapSlippageBps.toFixed(2)} bps`
              : "—"}
          </strong>

          <span className="metric-detail">
            VS MARKET VWAP
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            TOTAL FEES
          </span>

          <strong>
            {result
              ? result.totalFees.toLocaleString(
                  undefined,
                  {
                    maximumFractionDigits: 4,
                  }
                )
              : "—"}
          </strong>

          <span className="metric-detail">
            EXECUTION COST
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            FILLS
          </span>

          <strong>
            {result?.fillCount ?? "—"}
          </strong>

          <span className="metric-detail">
            EXECUTIONS
          </span>
        </div>
      </div>

      {result && (
        <div className="backtest-chart-grid">
          <BacktestPriceChart
            arrivalPrice={
              result.arrivalPrice
            }
            averageFillPrice={
              result.averageFillPrice
            }
            marketVwap={
              result.marketVwap
            }
          />

          <div className="panel backtest-quality-panel">
            <div className="panel-title">
              <div>
                <span className="eyebrow">
                  EXECUTION QUALITY
                </span>

                <h2>Run Diagnostics</h2>
              </div>

              <strong>
                {result.complete
                  ? "COMPLETE"
                  : "PARTIAL"}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>TOTAL NOTIONAL</span>

              <strong>
                {result.totalNotional.toLocaleString(
                  undefined,
                  {
                    maximumFractionDigits: 2,
                  }
                )}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>ACCEPTED CHILDREN</span>

              <strong>
                {result.acceptedChildren}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>REJECTED CHILDREN</span>

              <strong>
                {result.rejectedChildren}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>CHILD ACCEPT RATE</span>

              <strong>
                {(
                  result.acceptedChildren +
                  result.rejectedChildren
                ) > 0
                  ? `${(
                      result.acceptedChildren /
                      (
                        result.acceptedChildren +
                        result.rejectedChildren
                      ) *
                      100
                    ).toFixed(1)}%`
                  : "—"}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>FILL COUNT</span>

              <strong>
                {result.fillCount}
              </strong>
            </div>

            <div className="backtest-quality-row">
              <span>TOTAL FEES</span>

              <strong>
                {result.totalFees.toLocaleString(
                  undefined,
                  {
                    maximumFractionDigits: 4,
                  }
                )}
              </strong>
            </div>
          </div>
        </div>
      )}

      <div className="backtest-results-grid">
        <div className="panel backtest-results-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                EXECUTION TREE
              </span>

              <h2>Child Orders</h2>
            </div>

            <span>
              {children.length}
            </span>
          </div>

          <div className="backtest-table">
            <div className="backtest-table-row backtest-table-row--header">
              <span>ID</span>
              <span>VENUE</span>
              <span>STATUS</span>
              <span>FILLED</span>
            </div>

            {children.length === 0 ? (
              <div className="feedback">
                No child orders.
              </div>
            ) : (
              children.map((child) => (
                <div
                  className="backtest-table-row"
                  key={child.id}
                >
                  <strong>
                    #{child.id}
                  </strong>

                  <span>
                    {child.venue}
                  </span>

                  <strong>
                    {child.status}
                  </strong>

                  <span>
                    {child.filledQuantity}
                    /
                    {child.quantity}
                  </span>
                </div>
              ))
            )}
          </div>
        </div>

        <div className="panel backtest-results-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                EXECUTIONS
              </span>

              <h2>Fills</h2>
            </div>

            <span>
              {fills.length}
            </span>
          </div>

          <div className="backtest-table">
            <div className="backtest-table-row backtest-table-row--header">
              <span>VENUE</span>
              <span>QTY</span>
              <span>PRICE</span>
              <span>NOTIONAL</span>
            </div>

            {fills.length === 0 ? (
              <div className="feedback">
                No fills.
              </div>
            ) : (
              fills.map((fill) => (
                <div
                  className="backtest-table-row"
                  key={fill.id}
                >
                  <strong>
                    {fill.venue}
                  </strong>

                  <span>
                    {fill.quantity}
                  </span>

                  <span>
                    {fill.price}
                  </span>

                  <strong>
                    {(
                      fill.quantity *
                      fill.price
                    ).toLocaleString(
                      undefined,
                      {
                        maximumFractionDigits: 2,
                      }
                    )}
                  </strong>
                </div>
              ))
            )}
          </div>
        </div>
      </div>
    </section>
  );
}
