import {
  useState,
} from "react";
import {
  useQuery,
  useQueryClient,
} from "@tanstack/react-query";
import { submitOrder } from "../api/orders";
import {
  previewRoute,
} from "../api/executions";

import {
  getSystemStatus,
} from "../api/systemControl";

import {
  useMarketData,
} from "../context/MarketDataContext";
import StatusBadge from "../components/StatusBadge";
import {
  cancelOrder,
  replaceOrder,
  getReports,
  getClientRisk,
} from "../api/trading";

function reportStatus(
  value: number
) {
  const names: Record<number, string> = {
    0: "ACCEPTED",
    1: "PARTIALLY FILLED",
    2: "FILLED",
    3: "CANCELLED",
    4: "REPLACED",
    5: "REJECTED",
    6: "EXPIRED",
  };

  return names[value] ?? `STATUS ${value}`;
}

function routerSymbol(
  symbol: number
) {
  switch (symbol) {
    case 1:
      return "BTC-USD";
    default:
      return null;
  }
}

function rejectReason(
  value: number
) {
  return value === 0
    ? "NONE"
    : `REJECT ${value}`;
}

export default function Trading({
  quickSide,
  focusCancel,
}: {
  quickSide:
    | "BUY"
    | "SELL"
    | null;
  focusCancel: boolean;
}) {
  const [orderId, setOrderId] = useState(4001);
  const [manageOrderId, setManageOrderId] =
    useState(4001);

  const [clientId, setClientId] = useState(30);
  const [symbol, setSymbol] = useState(1);

  const [side, setSide] = useState<"BUY" | "SELL">(
    quickSide ?? "BUY"
  );
  const [type, setType] = useState<
    "LIMIT" | "MARKET" | "IOC" | "FOK" | "POST_ONLY"
  >("LIMIT");

  const [quantity, setQuantity] = useState(100);
  const [price, setPrice] = useState(10000);

  const [replacePrice, setReplacePrice] = useState(9999);
  const [replaceQuantity, setReplaceQuantity] = useState(80);

  const [status, setStatus] = useState("READY");

  const queryClient =
    useQueryClient();


  const {
    snapshot,
    status: marketStatus,
  } = useMarketData();

  const bbo =
    snapshot?.bbo;


  const { data: reports = [] } = useQuery({
    queryKey: ["reports"],
    queryFn: getReports,
    refetchInterval: 1000,
  });


  const { data: risk } = useQuery({
    queryKey: [
      "client-risk",
      clientId,
      symbol,
    ],
    queryFn: () =>
      getClientRisk(
        clientId,
        symbol
      ),
    refetchInterval: 1000,
  });

  const {
    data: systemStatus,
  } = useQuery({
    queryKey: [
      "system-trading-status",
    ],
    queryFn: getSystemStatus,
    refetchInterval: 1000,
  });

  const routeSymbol =
    routerSymbol(symbol);

  const {
    data: routePreview,
    isFetching: routePreviewLoading,
  } = useQuery({
    queryKey: [
      "trading-route-preview",
      routeSymbol,
      side,
      quantity,
      price,
      type,
    ],
    queryFn: () =>
      previewRoute({
        symbol: routeSymbol!,
        side:
          side === "BUY"
            ? "buy"
            : "sell",
        quantity,
        limitPrice:
          type === "MARKET"
            ? undefined
            : price,
        maxSlippageBps: 25,
        maxVenueCount: 3,
        allOrNone:
          type === "FOK",
      }),
    enabled:
      routeSymbol !== null &&
      quantity > 0 &&
      (
        type === "MARKET" ||
        price > 0
      ),
    refetchInterval: 1000,
  });

  const globallyHalted =
    systemStatus?.globallyHalted ??
    false;

  const estimatedNotional =
    routePreview?.estimatedNotional ??
    (
      bbo?.valid
        ? quantity *
          (
            side === "BUY"
              ? bbo.askPrice
              : bbo.bidPrice
          )
        : 0
    );

  const projectedPosition =
    risk
      ? risk.position +
        (
          side === "BUY"
            ? quantity
            : -quantity
        )
      : null;

  async function handleSubmit(event: React.FormEvent) {
    event.preventDefault();

    try {
      setStatus("SUBMITTING...");

      const result = await submitOrder({
        orderId,
        clientId,
        symbol,
        side,
        type,
        quantity,
        price:
          type === "MARKET"
            ? undefined
            : price,
      });

      setStatus(
        result.ok
          ? `ORDER ${result.orderId} ACCEPTED`
          : "ORDER REJECTED"
      );

      if (result.ok) {
        setManageOrderId(
          result.orderId
        );

        await queryClient.invalidateQueries({
          queryKey: ["reports"],
        });
      }
    } catch {
      setStatus("ORDER REJECTED");
    }
  }

  async function handleCancel() {
    try {
      const result = await cancelOrder(
        manageOrderId,
        clientId,
        symbol
      );

      setStatus(
        result.ok
          ? `ORDER ${manageOrderId} CANCELLED`
          : "CANCEL REJECTED"
      );

      if (result.ok) {
        await queryClient.invalidateQueries({
          queryKey: ["reports"],
        });
      }
    } catch {
      setStatus("CANCEL FAILED");
    }
  }

  async function handleReplace() {
    try {
      const result = await replaceOrder(
        manageOrderId,
        clientId,
        replacePrice,
        replaceQuantity,
        symbol
      );

      setStatus(
        result.ok
          ? `ORDER ${manageOrderId} REPLACED`
          : "REPLACE REJECTED"
      );

      if (result.ok) {
        await queryClient.invalidateQueries({
          queryKey: ["reports"],
        });
      }
    } catch {
      setStatus("REPLACE FAILED");
    }
  }

  return (
    <section className="page">
      <div className="page-heading trading-heading">
        <div>
          <span className="eyebrow">
            ORDER ENTRY + LIFECYCLE
          </span>

          <h1>Trading</h1>
        </div>

        <div className="trading-heading__status">
          <div>
            <span>SYSTEM</span>
            <StatusBadge
              value={
                globallyHalted
                  ? "HALTED"
                  : "ACTIVE"
              }
            />
          </div>

          <div>
            <span>MARKET</span>
            <StatusBadge
              value={marketStatus}
            />
          </div>

          <div>
            <span>CLIENT RISK</span>
            <StatusBadge
              value={
                risk?.killed
                  ? "KILLED"
                  : "SAFE"
              }
            />
          </div>
        </div>
      </div>

      <div className="trading-info">
        <div className="trading-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Trading is the direct order-entry interface for the
            MiniMatch matching engine. Submit new orders, modify
            resting orders, cancel open orders, and inspect the
            resulting execution reports.
          </p>
        </div>

        <div className="trading-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Choose a side, order type, quantity, and price, then
            submit the order. Use the order ID in Cancel / Replace
            to manage an existing order. Execution reports below
            show every accepted, replaced, cancelled, filled, or
            rejected lifecycle event.
          </p>
        </div>

        <div className="trading-info__section">
          <span className="eyebrow">
            ORDER TYPES
          </span>

          <p>
            LIMIT can rest on the book. MARKET executes immediately.
            IOC fills available quantity and cancels the remainder.
            FOK requires the full quantity immediately. POST_ONLY
            rejects orders that would immediately take liquidity.
          </p>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>New Order</h2>
          </div>

          <form className="order-form" onSubmit={handleSubmit}>
            <label>
              ORDER ID
              <input
                type="number"
                value={orderId}
                onChange={(e) => setOrderId(Number(e.target.value))}
              />
            </label>

            <label>
              CLIENT ID
              <input
                type="number"
                value={clientId}
                onChange={(e) => setClientId(Number(e.target.value))}
              />
            </label>

            <label>
              SYMBOL
              <input
                type="number"
                value={symbol}
                onChange={(e) => setSymbol(Number(e.target.value))}
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
                onChange={(e) =>
                  setType(e.target.value as typeof type)
                }
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
                autoFocus={quickSide !== null}
                value={quantity}
                onChange={(e) =>
                  setQuantity(Number(e.target.value))
                }
              />
            </label>

            <label>
              PRICE
              <input
                type="number"
                disabled={type === "MARKET"}
                value={price}
                onChange={(e) =>
                  setPrice(Number(e.target.value))
                }
              />
            </label>

            <button type="submit">
              SUBMIT ORDER
            </button>
          </form>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel trading-pretrade-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                LIVE CONTEXT
              </span>

              <h2>Market / Pre-Trade</h2>
            </div>

            <span>
              {marketStatus}
            </span>
          </div>

          <div className="trading-pretrade-grid">
            <div className="risk-item">
              <span>MARKET STATE</span>
              <strong
                className={
                  bbo?.crossed
                    ? "market-state market-state--crossed"
                    : bbo?.locked
                      ? "market-state market-state--locked"
                      : bbo?.valid
                        ? "market-state market-state--normal"
                        : "market-state market-state--invalid"
                }
              >
                {bbo?.crossed
                  ? "CROSSED"
                  : bbo?.locked
                    ? "LOCKED"
                    : bbo?.valid
                      ? "NORMAL"
                      : "NO BBO"}
              </strong>
            </div>

            <div className="risk-item">
              <span>BEST BID</span>
              <strong className="bid">
                {bbo &&
                Number.isFinite(bbo.bidPrice) &&
                bbo.bidPrice > 0
                  ? bbo.bidPrice
                  : "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>BEST ASK</span>
              <strong className="ask">
                {bbo &&
                Number.isFinite(bbo.askPrice) &&
                bbo.askPrice > 0
                  ? bbo.askPrice
                  : "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>SPREAD</span>
              <strong>
                {bbo &&
                Number.isFinite(bbo.spread)
                  ? bbo.spread
                  : "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>POSITION</span>
              <strong>
                {risk?.position ?? "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>PROJECTED POSITION</span>
              <strong>
                {projectedPosition ?? "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>OPEN NOTIONAL</span>
              <strong>
                {risk?.openNotional ?? "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>EST NOTIONAL</span>
              <strong>
                {estimatedNotional > 0
                  ? estimatedNotional.toFixed(2)
                  : "—"}
              </strong>
            </div>

            <div className="risk-item">
              <span>CLIENT RISK</span>
              <strong>
                {risk?.killed
                  ? "KILLED"
                  : "SAFE"}
              </strong>
            </div>

            <div className="risk-item">
              <span>TRADING STATUS</span>
              <strong
                className={
                  globallyHalted
                    ? "market-state market-state--crossed"
                    : "market-state market-state--normal"
                }
              >
                {globallyHalted
                  ? "HALTED"
                  : "ACTIVE"}
              </strong>
            </div>
          </div>

          <div className="trading-route-preview">
            <div className="panel-title">
              <h2>Route Preview</h2>

              <span>
                {routePreviewLoading
                  ? "UPDATING"
                  : routePreview?.complete
                    ? "COMPLETE"
                    : "PARTIAL"}
              </span>
            </div>

            <div className="tape-row trading-route-summary">
              <span>
                AVG{" "}
                {routePreview?.averagePrice
                  ? routePreview.averagePrice.toFixed(2)
                  : "—"}
              </span>

              <span>
                FEES{" "}
                {routePreview?.estimatedFees !== undefined
                  ? routePreview.estimatedFees.toFixed(4)
                  : "—"}
              </span>

              <span>
                ROUTED{" "}
                {routePreview?.routedQuantity ?? "—"}
              </span>
            </div>

            <div className="trading-route-legs">
              {(routePreview?.legs ?? []).map(
                (leg, index) => (
                  <div
                    className="tape-row"
                    key={`${leg.venue}-${leg.levelIndex}-${index}`}
                  >
                    <span>{leg.venue}</span>
                    <span>
                      {leg.quantity}
                    </span>
                    <span>
                      @ {leg.price}
                    </span>
                    <span>
                      FEE {leg.estimatedFee.toFixed(4)}
                    </span>
                  </div>
                )
              )}

              {(routePreview?.legs.length ?? 0) === 0 && (
                <div className="feedback">
                  No executable route for the current order.
                </div>
              )}
            </div>
          </div>
        </div>

        <div className="panel trading-manage-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                ORDER MANAGEMENT
              </span>
              <h2>Cancel / Replace</h2>
            </div>
          </div>

          <div className="trading-manage-section">
            <span className="trading-manage-section__title">
              ORDER TO MODIFY
            </span>

            <label className="trading-field trading-field--wide">
              <span>ORDER ID</span>
              <input
                id="manage-order-id"
                type="number"
                autoFocus={focusCancel}
                value={manageOrderId}
                onChange={(e) =>
                  setManageOrderId(
                    Number(e.target.value)
                  )
                }
              />
            </label>
          </div>

          <div className="trading-manage-section">
            <span className="trading-manage-section__title">
              NEW VALUES
            </span>

            <div className="trading-manage-grid">
              <label className="trading-field">
                <span>NEW PRICE</span>
                <input
                  type="number"
                  value={replacePrice}
                  onChange={(e) =>
                    setReplacePrice(
                      Number(e.target.value)
                    )
                  }
                />
              </label>

              <label className="trading-field">
                <span>NEW QUANTITY</span>
                <input
                  type="number"
                  value={replaceQuantity}
                  onChange={(e) =>
                    setReplaceQuantity(
                      Number(e.target.value)
                    )
                  }
                />
              </label>
            </div>
          </div>

          <div className="trading-manage-actions">
            <button
              className="trading-manage-actions__cancel"
              type="button"
              onClick={handleCancel}
            >
              CANCEL ORDER
            </button>

            <button
              className="trading-manage-actions__replace"
              type="button"
              onClick={handleReplace}
            >
              REPLACE ORDER
            </button>
          </div>
        </div>
      </div>

      <div className="panel trading-reports-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              ORDER LIFECYCLE
            </span>

            <h2>Execution Reports</h2>
          </div>

          <span>
            {reports.length} EVENTS
          </span>
        </div>

        <div className="trading-reports-table">
          <div className="trading-reports-table__header">
            <span>SEQ</span>
            <span>ORDER</span>
            <span>SYMBOL</span>
            <span>STATUS</span>
            <span>REMAINING</span>
            <span>REASON / INFO</span>
          </div>

          <div className="trading-reports-table__body">
            {reports.length === 0 && (
              <div className="feedback">
                No execution reports yet.
              </div>
            )}

            {reports.map((report) => (
              <div
                className="trading-reports-table__row"
                key={report.sequence}
              >
                <span>
                  #{report.sequence}
                </span>

                <strong>
                  {report.orderId}
                </strong>

                <span>
                  {report.symbol}
                </span>

                <strong
                  className={
                    report.status === 2
                      ? "status-good"
                      : report.status === 5
                        ? "status-bad"
                        : report.status === 3
                          ? "status-warn"
                          : ""
                  }
                >
                  {reportStatus(
                    report.status
                  )}
                </strong>

                <span>
                  {report.remaining}
                </span>

                <span>
                  {report.rejectReason === 0
                    ? "—"
                    : rejectReason(
                        report.rejectReason
                      )}
                </span>
              </div>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
}
