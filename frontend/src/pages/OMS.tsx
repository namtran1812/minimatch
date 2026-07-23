import {
  useState,
  useEffect,
} from "react";
import {
  useQuery,
  useQueryClient,
} from "@tanstack/react-query";
import {
  getParentOrders,
  getChildOrders,
  getFills,
  cancelChildOrder,
} from "../api/oms";

import {
  submitAlgoOrder,
  type ExecutionAlgorithm,
} from "../api/algos";

import {
  getExecutionQuality,
  getParentReconciliation,
} from "../api/quality";

import StatusBadge from "../components/StatusBadge";
import OmsExecutionProgressChart from "../components/charts/OmsExecutionProgressChart";
import OmsTcaChart from "../components/charts/OmsTcaChart";

import {
  useProductContext,
} from "../context/ProductContext";

export default function OMS() {
  const {
    setSelectedOmsParentId,
  } = useProductContext();
  const [selectedParent, setSelectedParent] =
    useState<string | null>(null);

  const [status, setStatus] =
    useState("READY");

  const [algoSymbol, setAlgoSymbol] =
    useState("BTC-USD");

  const [algoSide, setAlgoSide] =
    useState<"BUY" | "SELL">("BUY");

  const [algorithm, setAlgorithm] =
    useState<ExecutionAlgorithm>("TWAP");

  const [algoQuantity, setAlgoQuantity] =
    useState(1);

  const [slices, setSlices] =
    useState(5);

  const [durationSeconds, setDurationSeconds] =
    useState(60);

  const [participationRate, setParticipationRate] =
    useState(0.1);

  const [displayedQuantity, setDisplayedQuantity] =
    useState(0.1);

  const [maxSlippageBps, setMaxSlippageBps] =
    useState(25);

  const [maxVenueCount, setMaxVenueCount] =
    useState(3);

  const queryClient =
    useQueryClient();

  const { data: parents = [] } = useQuery({
    queryKey: ["oms-parents"],
    queryFn: getParentOrders,
    refetchInterval: 2000,
  });

  const { data: children = [] } = useQuery({
    queryKey: ["oms-children", selectedParent],
    queryFn: () => getChildOrders(selectedParent!),
    enabled: !!selectedParent,
    refetchInterval: 1000,
  });

  const { data: fills = [] } = useQuery({
    queryKey: ["oms-fills", selectedParent],
    queryFn: () => getFills(selectedParent!),
    enabled: !!selectedParent,
    refetchInterval: 1000,
  });


  const {
    data: executionQuality,
  } = useQuery({
    queryKey: [
      "execution-quality",
      selectedParent,
    ],
    queryFn: () =>
      getExecutionQuality(
        selectedParent!
      ),
    enabled: !!selectedParent,
    refetchInterval: 1000,
  });

  const {
    data: reconciliation,
  } = useQuery({
    queryKey: [
      "parent-reconciliation",
      selectedParent,
    ],
    queryFn: () =>
      getParentReconciliation(
        selectedParent!
      ),
    enabled: !!selectedParent,
    refetchInterval: 1000,
  });

  const selectedParentOrder =
    parents.find(
      (order) =>
        order.id === selectedParent
    ) ?? null;

  const activeParentCount =
    parents.filter((order) =>
      ![
        "FILLED",
        "CANCELLED",
        "REJECTED",
        "COMPLETE",
      ].includes(
        order.status.toUpperCase()
      )
    ).length;

  const liveChildCount =
    children.filter((child) =>
      [
        "NEW",
        "PARTIALLY_FILLED",
        "WORKING",
      ].includes(
        child.status.toUpperCase()
      )
    ).length;

  const totalFillNotional =
    fills.reduce(
      (sum, fill) =>
        sum + fill.notional,
      0
    );

  const executionProgress =
    [...fills]
      .sort(
        (left, right) =>
          left.timestamp -
          right.timestamp
      )
      .reduce<
        Array<{
          time: number;
          cumulativeQuantity: number;
        }>
      >(
        (points, fill) => {
          const previous =
            points.at(-1)
              ?.cumulativeQuantity ??
            0;

          const firstTimestamp =
            fills.length > 0
              ? Math.min(
                  ...fills.map(
                    (item) =>
                      item.timestamp
                  )
                )
              : fill.timestamp;

          points.push({
            time:
              (
                fill.timestamp -
                firstTimestamp
              ) /
              1_000_000,
            cumulativeQuantity:
              previous +
              fill.quantity,
          });

          return points;
        },
        []
      );

  useEffect(() => {
    setSelectedOmsParentId(
      selectedParent
        ? String(selectedParent)
        : null
    );
  }, [
    selectedParent,
    setSelectedOmsParentId,
  ]);


  async function handleAlgoSubmit(
    event: React.FormEvent
  ) {
    event.preventDefault();

    try {
      setStatus("SUBMITTING ALGO...");

      const result =
        await submitAlgoOrder({
          symbol: algoSymbol,
          side: algoSide,
          quantity: algoQuantity,
          algorithm,
          slices,
          durationSeconds,
          participationRate,
          displayedQuantity,
          maxSlippageBps,
          maxVenueCount,
        });

      setStatus(
        result.ok
          ? `${algorithm} ${result.parentOrderId} RUNNING`
          : "ALGO REJECTED"
      );

      if (result.ok) {
        setSelectedParent(
          result.parentOrderId
        );

        await queryClient.invalidateQueries({
          queryKey: ["oms-parents"],
        });
      }
    } catch {
      setStatus("ALGO SUBMISSION FAILED");
    }
  }

  async function handleCancelChild(
    childId: string
  ) {
    try {
      setStatus(
        `CANCELLING CHILD ${childId}...`
      );

      const result =
        await cancelChildOrder(
          childId
        );

      setStatus(
        result.ok
          ? `CHILD ${childId} CANCELLED`
          : `CHILD ${childId} CANCEL REJECTED`
      );

      await queryClient.invalidateQueries({
        queryKey: [
          "oms-children",
          selectedParent,
        ],
      });

      await queryClient.invalidateQueries({
        queryKey: ["oms-parents"],
      });
    } catch {
      setStatus(
        `CHILD ${childId} CANCEL FAILED`
      );
    }
  }

  return (
    <section className="page">
      <div className="page-heading oms-page-heading">
        <div className="oms-page-heading__title">
          <span className="eyebrow">
            ORDER MANAGEMENT SYSTEM
          </span>

          <h1>OMS</h1>
        </div>

        <div className="oms-header-stats">
          <div className="oms-header-stat">
            <span>PARENTS ACTIVE</span>
            <strong>
              {activeParentCount}
            </strong>
          </div>

          <div className="oms-header-stat">
            <span>PARENTS TOTAL</span>
            <strong>
              {parents.length}
            </strong>
          </div>

          <div className="oms-header-stat">
            <span>CHILD ORDERS LIVE</span>
            <strong>
              {liveChildCount}
            </strong>
          </div>

          <div className="oms-header-stat">
            <span>SELECTED FILLS</span>
            <strong>
              {fills.length}
            </strong>
          </div>

          <div className="oms-header-stat">
            <span>FILL NOTIONAL</span>
            <strong>
              {totalFillNotional.toLocaleString(
                undefined,
                {
                  maximumFractionDigits: 2,
                }
              )}
            </strong>
          </div>
        </div>
      </div>

      <div className="oms-info">
        <div className="oms-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>
          <p>
            OMS manages parent orders, algorithmic execution,
            child orders, fills, and post-trade lifecycle state
            across MiniMatch.
          </p>
        </div>

        <div className="oms-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>
          <p>
            Launch an execution algorithm, select the resulting
            parent order, then inspect its child orders, fills,
            execution quality, and reconciliation status.
          </p>
        </div>

        <div className="oms-info__section">
          <span className="eyebrow">
            ALGO TYPES
          </span>
          <p>
            MARKET executes immediately. TWAP spreads execution
            across time. VWAP follows volume participation. POV
            targets a participation rate. ICEBERG exposes only
            part of the total quantity.
          </p>
        </div>
      </div>

      <div className="panel oms-algo-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              EXECUTION SCHEDULER
            </span>
            <h2>Algo Execution</h2>
          </div>

          <strong>{algorithm}</strong>
        </div>

        <form
          className="oms-algo-form"
          onSubmit={handleAlgoSubmit}
        >
          <label>
            SYMBOL
            <input
              value={algoSymbol}
              onChange={(e) =>
                setAlgoSymbol(e.target.value)
              }
            />
          </label>

          <label>
            SIDE
            <select
              value={algoSide}
              onChange={(e) =>
                setAlgoSide(
                  e.target.value as "BUY" | "SELL"
                )
              }
            >
              <option>BUY</option>
              <option>SELL</option>
            </select>
          </label>

          <label>
            ALGORITHM
            <select
              value={algorithm}
              onChange={(e) =>
                setAlgorithm(
                  e.target.value as ExecutionAlgorithm
                )
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
              step="any"
              value={algoQuantity}
              onChange={(e) =>
                setAlgoQuantity(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            SLICES
            <input
              type="number"
              value={slices}
              onChange={(e) =>
                setSlices(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            DURATION (SEC)
            <input
              type="number"
              value={durationSeconds}
              onChange={(e) =>
                setDurationSeconds(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            PARTICIPATION RATE
            <input
              type="number"
              step="0.01"
              value={participationRate}
              onChange={(e) =>
                setParticipationRate(
                  Number(e.target.value)
                )
              }
            />
          </label>

          <label>
            DISPLAY QTY
            <input
              type="number"
              step="any"
              value={displayedQuantity}
              onChange={(e) =>
                setDisplayedQuantity(
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

          <button type="submit">
            LAUNCH {algorithm}
          </button>
        </form>
      </div>

      <div className="oms-order-workspace">
        <div className="panel oms-parent-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                ORDER LIFECYCLE
              </span>
              <h2>Parent Orders</h2>
            </div>

            <span>
              {parents.length} TOTAL
            </span>
          </div>

          <div className="oms-table oms-parent-table">
            <div className="oms-table__header">
              <span>ID</span>
              <span>SYMBOL</span>
              <span>SIDE</span>
              <span>ALGO</span>
              <span>QTY</span>
              <span>FILLED</span>
              <span>STATUS</span>
            </div>

            <div className="oms-table__body">
              {parents.length === 0 && (
                <div className="feedback">
                  No OMS parent orders available.
                </div>
              )}

              {parents.map((order) => (
                <button
                  key={order.id}
                  type="button"
                  className={
                    selectedParent === order.id
                      ? "oms-table__row oms-table__row--selected"
                      : "oms-table__row"
                  }
                  onClick={() =>
                    setSelectedParent(
                      order.id
                    )
                  }
                >
                  <strong>{order.id}</strong>
                  <span>{order.symbol}</span>
                  <span>{order.side}</span>
                  <span>{order.strategy}</span>
                  <span>{order.quantity}</span>
                  <span>
                    {order.filledQuantity}
                  </span>

                  <StatusBadge
                    value={order.status}
                  />
                </button>
              ))}
            </div>
          </div>
        </div>

        <div className="panel oms-child-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                SELECTED PARENT
              </span>
              <h2>Child Orders</h2>
            </div>

            <span>
              {selectedParent ?? "—"}
            </span>
          </div>

          <div className="oms-selected-parent-strip">
            <div>
              <span>SYMBOL</span>
              <strong>
                {selectedParentOrder?.symbol ?? "—"}
              </strong>
            </div>

            <div>
              <span>ALGO</span>
              <strong>
                {selectedParentOrder?.strategy ?? "—"}
              </strong>
            </div>

            <div>
              <span>FILLED</span>
              <strong>
                {selectedParentOrder
                  ? `${selectedParentOrder.filledQuantity}/${selectedParentOrder.quantity}`
                  : "—"}
              </strong>
            </div>
          </div>

          <div className="oms-table oms-child-table">
            <div className="oms-table__header">
              <span>ID</span>
              <span>VENUE</span>
              <span>PRICE</span>
              <span>QTY</span>
              <span>FILLED</span>
              <span>STATUS</span>
              <span>ACTION</span>
            </div>

            <div className="oms-table__body">
              {children.length === 0 && (
                <div className="feedback">
                  Select a parent order.
                </div>
              )}

              {children.map((child) => (
                <div
                  className="oms-table__row"
                  key={child.id}
                >
                  <strong>{child.id}</strong>
                  <span>{child.venue}</span>
                  <span>
                    {child.price.toFixed(2)}
                  </span>
                  <span>{child.quantity}</span>
                  <span>
                    {child.filledQuantity}
                  </span>

                  <StatusBadge
                    value={child.status}
                  />

                  <span>
                    {(
                      child.status === "NEW" ||
                      child.status ===
                        "PARTIALLY_FILLED"
                    ) ? (
                      <button
                        type="button"
                        className="oms-cancel-child"
                        onClick={() =>
                          handleCancelChild(
                            child.id
                          )
                        }
                      >
                        CANCEL
                      </button>
                    ) : (
                      "—"
                    )}
                  </span>
                </div>
              ))}
            </div>
          </div>

          <div className="feedback oms-status-line">
            {status}
          </div>
        </div>

        <div className="panel oms-fills-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                EXECUTIONS
              </span>
              <h2>Fills</h2>
            </div>

            <span>{fills.length} FILLS</span>
          </div>

          <div className="oms-table oms-fill-table">
            <div className="oms-table__header">
              <span>VENUE</span>
              <span>QTY</span>
              <span>PRICE</span>
              <span>FEE</span>
              <span>NOTIONAL</span>
            </div>

            <div className="oms-table__body">
              {fills.length === 0 && (
                <div className="feedback">
                  No fills for selected parent.
                </div>
              )}

              {fills.map((fill) => (
                <div
                  className="oms-table__row"
                  key={fill.id}
                >
                  <strong>{fill.venue}</strong>

                  <span>
                    {fill.quantity}
                  </span>

                  <span>
                    {fill.price.toFixed(2)}
                  </span>

                  <span>
                    {fill.fee.toFixed(6)}
                  </span>

                  <span>
                    {fill.notional.toFixed(2)}
                  </span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>

      {(executionProgress.length > 0 ||
        executionQuality) && (
        <div className="oms-visualization-grid">
          {executionProgress.length > 0 &&
            selectedParentOrder && (
              <OmsExecutionProgressChart
                data={executionProgress}
                requestedQuantity={
                  selectedParentOrder.quantity
                }
              />
            )}

          {executionQuality && (
            <OmsTcaChart
              implementationShortfallBps={
                executionQuality.implementationShortfallBps
              }
              feeAdjustedShortfallBps={
                executionQuality.feeAdjustedShortfallBps
              }
            />
          )}
        </div>
      )}

      <div className="terminal-grid oms-analysis-grid">
        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                POST-TRADE ANALYSIS
              </span>
              <h2>Execution Quality / TCA</h2>
            </div>

            <span>
              {selectedParent ?? "NO PARENT"}
            </span>
          </div>

          {!executionQuality && (
            <div className="feedback">
              Select a parent order to inspect execution quality.
            </div>
          )}

          {executionQuality && (
            <div className="oms-tca-grid">
              <div className="risk-item">
                <span>REQUESTED</span>
                <strong>
                  {executionQuality.requestedQuantity}
                </strong>
              </div>

              <div className="risk-item">
                <span>FILLED</span>
                <strong>
                  {executionQuality.filledQuantity}
                </strong>
              </div>

              <div className="risk-item">
                <span>ARRIVAL PRICE</span>
                <strong>
                  {executionQuality.arrivalPrice.toFixed(2)}
                </strong>
              </div>

              <div className="risk-item">
                <span>AVG FILL</span>
                <strong>
                  {executionQuality.averageFillPrice.toFixed(2)}
                </strong>
              </div>

              <div className="risk-item">
                <span>TOTAL NOTIONAL</span>
                <strong>
                  {executionQuality.totalNotional.toFixed(2)}
                </strong>
              </div>

              <div className="risk-item">
                <span>TOTAL FEES</span>
                <strong>
                  {executionQuality.totalFees.toFixed(4)}
                </strong>
              </div>

              <div className="risk-item">
                <span>IMPL SHORTFALL</span>
                <strong>
                  {executionQuality.implementationShortfallBps.toFixed(2)} bps
                </strong>
              </div>

              <div className="risk-item">
                <span>FEE-ADJ SHORTFALL</span>
                <strong>
                  {executionQuality.feeAdjustedShortfallBps.toFixed(2)} bps
                </strong>
              </div>
            </div>
          )}
        </div>

        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                DATA INTEGRITY
              </span>
              <h2>Parent Reconciliation</h2>
            </div>

            {reconciliation && (
              <StatusBadge
                value={
                  reconciliation.consistent
                    ? "CONSISTENT"
                    : reconciliation.reconciliationStatus
                }
              />
            )}
          </div>

          {!reconciliation && (
            <div className="feedback">
              Select a parent order to inspect reconciliation.
            </div>
          )}

          {reconciliation && (
            <>
              <div className="oms-reconciliation-grid">
                <div className="risk-item">
                  <span>PARENT FILLED</span>
                  <strong>
                    {reconciliation.parentFilledQuantity}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>OMS FILLS</span>
                  <strong>
                    {reconciliation.omsFillQuantity}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>CHILD FILLS</span>
                  <strong>
                    {reconciliation.childFilledQuantity}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>QTY DIFFERENCE</span>
                  <strong>
                    {reconciliation.quantityDifference}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>FILL EVENTS</span>
                  <strong>
                    {reconciliation.fillCount}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>DROP COPY</span>
                  <strong>
                    {reconciliation.dropCopyFillCount}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>CHILD COUNT</span>
                  <strong>
                    {reconciliation.childCount}
                  </strong>
                </div>

                <div className="risk-item">
                  <span>ANALYTICS FEES</span>
                  <strong>
                    {reconciliation.analyticsTotalFees.toFixed(4)}
                  </strong>
                </div>
              </div>

              {reconciliation.reason && (
                <div className="feedback">
                  {reconciliation.reason}
                </div>
              )}
            </>
          )}
        </div>
      </div>

    </section>
  );
}
