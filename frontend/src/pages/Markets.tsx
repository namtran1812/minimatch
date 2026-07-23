import { useEffect } from "react";
import {
  useMarketData,
} from "../context/MarketDataContext";

import type {
  Page,
} from "../components/layout/Sidebar";

import {
  useTradeSocket,
} from "../hooks/useTradeSocket";
import MarketDepthChart from "../components/charts/MarketDepthChart";
import VenueLiquidityChart from "../components/charts/VenueLiquidityChart";
import BboHistoryChart from "../components/charts/BboHistoryChart";

export default function Markets({
  onQuickTrade,
  onOpenPage,
  onOpenRiskHalt,
}: {
  onQuickTrade: (
    side: "BUY" | "SELL"
  ) => void;
  onOpenPage: (
    page: Page
  ) => void;
  onOpenRiskHalt: () => void;
}) {
  const {
    mode,
    setMode,
    snapshot,
    bboHistory,
    status,
  } = useMarketData();

  const bbo =
    snapshot?.bbo;

  useEffect(() => {
    function handleKeyDown(
      event: KeyboardEvent
    ) {
      const target =
        event.target as HTMLElement | null;

      if (
        target?.tagName === "INPUT" ||
        target?.tagName === "SELECT" ||
        target?.tagName === "TEXTAREA" ||
        target?.isContentEditable
      ) {
        return;
      }

      if (
        event.key === "b" ||
        event.key === "B"
      ) {
        event.preventDefault();
        onQuickTrade("BUY");
      }

      if (
        event.key === "s" ||
        event.key === "S"
      ) {
        event.preventDefault();
        onQuickTrade("SELL");
        return;
      }

      if (
        event.key === "c" ||
        event.key === "C"
      ) {
        event.preventDefault();
        onOpenPage("trading");
        return;
      }

      if (
        event.key === "r" ||
        event.key === "R"
      ) {
        event.preventDefault();
        onOpenPage("replay");
        return;
      }

      if (
        event.key === "k" ||
        event.key === "K"
      ) {
        event.preventDefault();
        onOpenRiskHalt();
      }
    }

    window.addEventListener(
      "keydown",
      handleKeyDown
    );

    return () => {
      window.removeEventListener(
        "keydown",
        handleKeyDown
      );
    };
  }, [
    onQuickTrade,
    onOpenPage,
    onOpenRiskHalt,
  ]);

  const {
    trades,
    connected: tradeStreamConnected,
  } = useTradeSocket();

  const depthByPrice =
    new Map<
      number,
      {
        price: number;
        bid: number;
        ask: number;
      }
    >();

  for (const venue of snapshot?.venues ?? []) {
    for (const level of venue.bids) {
      const point =
        depthByPrice.get(level.price) ?? {
          price: level.price,
          bid: 0,
          ask: 0,
        };

      point.bid += level.quantity;
      depthByPrice.set(level.price, point);
    }

    for (const level of venue.asks) {
      const point =
        depthByPrice.get(level.price) ?? {
          price: level.price,
          bid: 0,
          ask: 0,
        };

      point.ask += level.quantity;
      depthByPrice.set(level.price, point);
    }
  }

  const depthData =
    [...depthByPrice.values()]
      .sort(
        (left, right) =>
          left.price - right.price
      );

  const venueLiquidity =
    (snapshot?.venues ?? []).map(
      (venue) => ({
        venue: venue.venue,
        bidLiquidity:
          venue.bids.reduce(
            (sum, level) =>
              sum + level.quantity,
            0
          ),
        askLiquidity:
          venue.asks.reduce(
            (sum, level) =>
              sum + level.quantity,
            0
          ),
      })
    );

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          CONSOLIDATED MARKET DATA
        </span>

        <h1>
          {snapshot?.symbol
            ?? "BTCUSD"}
        </h1>

        <div className="markets-source-row">
          <div className={`stream-status stream-status--${status.toLowerCase()}`}>
            <span className="stream-status__pixel" />
            MARKET STREAM: {status}
          </div>

          <div className="markets-source-toggle">
            <button
              onClick={() => setMode("live")}
              disabled={mode === "live"}
            >
              LIVE
            </button>

            <button
              onClick={() => setMode("replay")}
              disabled={mode === "replay"}
            >
              REPLAY
            </button>
          </div>
        </div>
      </div>

      <div className="markets-info">
        <div className="markets-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Markets consolidates live Level 2 order books across
            participating venues into a single view of BTC-USD.
            It shows the best bid and ask, market state, venue
            liquidity, latency, order-book depth, routing previews,
            and executed trades.
          </p>
        </div>

        <div className="markets-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Start with the BBO and market state, then use the depth
            and venue-liquidity charts to understand where liquidity
            is available. Inspect individual venue books for price
            levels and use BUY / SELL route previews to see how a
            0.1 BTC order would be distributed across venues after
            fees and latency costs.
          </p>
        </div>

        <div className="markets-info__section">
          <span className="eyebrow">
            MARKET STATES
          </span>

          <p>
            NORMAL means bid is below ask. LOCKED means bid equals
            ask. CROSSED means the best bid is above the best ask
            across venues; the consolidated midpoint is intentionally
            unavailable until the market becomes valid again.
          </p>
        </div>
      </div>

      <div className="markets-ticker">
        <div className="markets-ticker__item">
          <span>BEST BID</span>
          <strong className="bid">
            {bbo &&
            Number.isFinite(bbo.bidPrice) &&
            bbo.bidPrice > 0
              ? bbo.bidPrice
              : "—"}
          </strong>
          <small>
            {bbo?.bidVenue ?? "—"}
          </small>
        </div>

        <div className="markets-ticker__item">
          <span>BEST ASK</span>
          <strong className="ask">
            {bbo &&
            Number.isFinite(bbo.askPrice) &&
            bbo.askPrice > 0
              ? bbo.askPrice
              : "—"}
          </strong>
          <small>
            {bbo?.askVenue ?? "—"}
          </small>
        </div>

        <div className="markets-ticker__item">
          <span>MID</span>
          <strong>
            {bbo?.valid
              ? bbo.midpoint
              : "—"}
          </strong>
        </div>

        <div className="markets-ticker__item">
          <span>SPREAD</span>
          <strong
            className={
              bbo?.crossed
                ? "market-spread market-spread--crossed"
                : bbo?.locked
                  ? "market-spread market-spread--locked"
                  : "market-spread"
            }
          >
            {bbo &&
            Number.isFinite(bbo.spread) &&
            (
              bbo.valid ||
              bbo.crossed ||
              bbo.locked
            )
              ? bbo.spread
              : "—"}
          </strong>
        </div>

        <div className="markets-ticker__item">
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

        <div className="markets-ticker__item">
          <span>TRADES</span>
          <strong>
            {trades.length}
          </strong>
        </div>
      </div>

      <div className="markets-visualization-grid">
        <BboHistoryChart
          data={bboHistory}
        />

        <MarketDepthChart
          data={depthData}
        />

        <VenueLiquidityChart
          data={venueLiquidity}
        />
      </div>

      <div className="markets-health-strip">
        <div>
          <span>MARKET</span>
          <strong
            className={
              status === "CONNECTED"
                ? "venue-status venue-status--live"
                : "venue-status venue-status--stale"
            }
          >
            <span className="venue-status__pixel" />
            {status}
          </strong>
        </div>

        {(snapshot?.venueHealth ?? []).map((venue) => (
          <div key={venue.venue}>
            <span>{venue.venue}</span>
            <strong
              className={
                venue.synchronized &&
                venue.status === "healthy"
                  ? "venue-status venue-status--live"
                  : "venue-status venue-status--stale"
              }
            >
              <span className="venue-status__pixel" />
              {venue.status.toUpperCase()}
            </strong>
          </div>
        ))}

        <div>
          <span>RECORDER</span>
          <strong
            className={
              snapshot?.recordingEnabled
                ? "recorder-status recorder-status--active"
                : "recorder-status recorder-status--idle"
            }
          >
            <span className="recorder-status__pixel" />
            {snapshot?.recordingEnabled
              ? "RECORDING"
              : "OFF"}
          </strong>
        </div>
      </div>

      <div className="markets-latency-strip">
        {(snapshot?.latency ?? []).slice(0, 4).map((item) => {
          const p50 = item.p50Ns / 1_000_000;
          const p95 = item.p95Ns / 1_000_000;
          const p99 = item.p99Ns / 1_000_000;

          const max = Math.max(p99, 0.001);

          return (
            <div
              className="markets-latency-item"
              key={item.name}
            >
              <div className="markets-latency-item__header">
                <span>
                  {item.name.toUpperCase()}
                </span>

                <strong>
                  {p99.toFixed(2)} ms
                </strong>
              </div>

              <div className="markets-latency-bars">
                <div
                  className="markets-latency-bar"
                  style={{
                    width: `${Math.max(
                      4,
                      (p50 / max) * 100
                    )}%`,
                  }}
                />

                <div
                  className="markets-latency-bar markets-latency-bar--p95"
                  style={{
                    width: `${Math.max(
                      4,
                      (p95 / max) * 100
                    )}%`,
                  }}
                />

                <div
                  className={
                    p99 >= 10
                      ? "markets-latency-bar markets-latency-bar--p99 markets-latency-bar--hot"
                      : p99 >= 5
                        ? "markets-latency-bar markets-latency-bar--p99 markets-latency-bar--warn"
                        : "markets-latency-bar markets-latency-bar--p99"
                  }
                  style={{
                    width: "100%",
                  }}
                />
              </div>

              <div className="markets-latency-labels">
                <span>P50 {p50.toFixed(2)}</span>
                <span>P95 {p95.toFixed(2)}</span>
                <span>P99 {p99.toFixed(2)}</span>
              </div>
            </div>
          );
        })}
      </div>



      <div className="markets-workspace">
        <div className="markets-books">
          {snapshot?.venues.map(
          (venue) => (
            <div
              className="panel"
              key={venue.venue}
            >
              <div className="panel-title">
                <h2>
                  {venue.venue}
                </h2>

                <span>
                  SEQ{" "}
                  {venue.sequence}
                </span>
              </div>

              <div className="risk-item">
                <span>
                  BOOK STATUS
                </span>

                <strong
                  className={
                    venue.synchronized
                      ? "venue-status venue-status--live"
                      : "venue-status venue-status--stale"
                  }
                >
                  <span className="venue-status__pixel" />
                  {venue.synchronized
                    ? "SYNCHRONIZED"
                    : "STALE"}
                </strong>
              </div>

              {(() => {
                const bidDepth =
                  venue.bids.reduce(
                    (sum, level) =>
                      sum + level.quantity,
                    0
                  );

                const askDepth =
                  venue.asks.reduce(
                    (sum, level) =>
                      sum + level.quantity,
                    0
                  );

                const totalDepth =
                  bidDepth + askDepth;

                const bidPercent =
                  totalDepth > 0
                    ? (bidDepth /
                        totalDepth) *
                      100
                    : 50;

                return (
                  <div className="book-imbalance">
                    <div className="book-imbalance__labels">
                      <span>
                        BID {bidDepth.toFixed(2)}
                      </span>

                      <strong>
                        {bidPercent.toFixed(0)}%
                      </strong>

                      <span>
                        ASK {askDepth.toFixed(2)}
                      </span>
                    </div>

                    <div className="book-imbalance__track">
                      <div
                        className="book-imbalance__bid"
                        style={{
                          width: `${bidPercent}%`,
                        }}
                      />

                      <div
                        className="book-imbalance__ask"
                        style={{
                          width: `${100 - bidPercent}%`,
                        }}
                      />
                    </div>
                  </div>
                );
              })()}

              <div className="markets-book-grid">
                <div>
                  <span className="eyebrow">
                    BIDS
                  </span>

                  {venue.bids.map(
                    (
                      level,
                      index
                    ) => (
                      <div
                        className="book-row"
                        key={
                          `bid-${index}`
                        }
                      >
                        <div
                          className="depth-bg bid"
                          style={{
                            width: `${Math.min(
                              100,
                              level.quantity / 10
                            )}%`,
                          }}
                        />
                        <span className="bid">
                          {
                            level.price
                          }
                        </span>

                        <span>
                          {
                            level.quantity
                          }
                        </span>
                      </div>
                    )
                  )}
                </div>

                <div>
                  <span className="eyebrow">
                    ASKS
                  </span>

                  {venue.asks.map(
                    (
                      level,
                      index
                    ) => (
                      <div
                        className="book-row"
                        key={
                          `ask-${index}`
                        }
                      >
                        <div
                          className="depth-bg ask"
                          style={{
                            width: `${Math.min(
                              100,
                              level.quantity / 10
                            )}%`,
                          }}
                        />
                        <span className="ask">
                          {
                            level.price
                          }
                        </span>

                        <span>
                          {
                            level.quantity
                          }
                        </span>
                      </div>
                    )
                  )}
                </div>
              </div>
            </div>
          )
          )}
        </div>

        <div className="markets-tape">
          <div className="panel trade-tape-panel">


        <div className="panel-title">
          <h2>LIVE TRADE TAPE</h2>

          <span
            className={
              tradeStreamConnected
                ? "venue-status venue-status--live"
                : "venue-status venue-status--stale"
            }
          >
            <span className="venue-status__pixel" />
            {tradeStreamConnected
              ? "LIVE"
              : "OFFLINE"}
            {" · "}
            {trades.length} PRINTS
          </span>
        </div>

        <div className="trade-tape-header">
          <span>SEQ</span>
          <span>SIDE</span>
          <span>PRICE</span>
          <span>QTY</span>
        </div>

        <div className="trade-tape">
          {trades.length === 0 && (
            <div className="feedback">
              Waiting for trades...
            </div>
          )}

          {trades
            .slice()
            .reverse()
            .slice(0, 20)
            .map((trade, index) => (
              <div
                className={
                  index === 0
                    ? "trade-tape-row trade-tape-row--new"
                    : "trade-tape-row"
                }
                key={trade.sequence}
              >
                <span>
                  #{trade.sequence}
                </span>

                <strong
                  className={
                    trade.side === "BUY"
                      ? "buy"
                      : "sell"
                  }
                >
                  {trade.side}
                </strong>

                <span>
                  {trade.price}
                </span>

                <span>
                  {trade.quantity}
                </span>
              </div>
            ))}
            </div>
          </div>
        </div>
      </div>

      <div className="terminal-grid markets-routes">
        <RoutePanel
          title="BUY ROUTE"
          route={
            snapshot?.routing.buy
          }
        />

        <RoutePanel
          title="SELL ROUTE"
          route={
            snapshot?.routing.sell
          }
        />
      </div>
    </section>
  );
}

function RoutePanel({
  title,
  route,
}: {
  title: string;
  route:
    | {
        complete: boolean;
        requestedQuantity:
          number;
        routedQuantity:
          number;
        averagePrice: number;
        estimatedFees:
          number;
        legs: {
          venue: string;
          price: number;
          quantity: number;
          effectivePrice:
            number;
        }[];
      }
    | undefined;
}) {
  return (
    <div className="panel">
      <div className="panel-title">
        <h2>{title}</h2>
      </div>

      {!route && (
        <div className="feedback">
          Waiting for market
          data...
        </div>
      )}

      {route && (
        <>
          <div className="risk-item">
            <span>
              AVG PRICE
            </span>

            <strong>
              {route.averagePrice}
            </strong>
          </div>

          <div className="risk-item">
            <span>
              ROUTED
            </span>

            <strong>
              {
                route.routedQuantity
              }
              /
              {
                route.requestedQuantity
              }
            </strong>
          </div>

          <div className="risk-item">
            <span>
              EST FEES
            </span>

            <strong>
              {
                route.estimatedFees
              }
            </strong>
          </div>

          {route.legs.map(
            (leg, index) => (
              <div
                className="risk-item"
                key={
                  `${leg.venue}-${index}`
                }
              >
                <span>
                  {leg.venue}
                </span>

                <strong>
                  {leg.quantity}
                  {" @ "}
                  {leg.price}
                </strong>
              </div>
            )
          )}
        </>
      )}
    </div>
  );
}
