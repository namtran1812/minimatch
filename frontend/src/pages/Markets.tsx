import {
  useMarketData,
} from "../context/MarketDataContext";

export default function Markets() {
  const {
    snapshot,
    status,
  } = useMarketData();

  const bbo =
    snapshot?.bbo;

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

        <div className="feedback">
          MARKET STREAM: {status}
        </div>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">
            BEST BID
          </span>

          <strong>
            {bbo?.valid
              ? bbo.bidPrice
              : "—"}
          </strong>

          <span>
            {bbo?.bidVenue
              ?? "—"}
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            BEST ASK
          </span>

          <strong>
            {bbo?.valid
              ? bbo.askPrice
              : "—"}
          </strong>

          <span>
            {bbo?.askVenue
              ?? "—"}
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            MIDPOINT
          </span>

          <strong>
            {bbo?.valid
              ? bbo.midpoint
              : "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            SPREAD
          </span>

          <strong>
            {bbo?.valid
              ? bbo.spread
              : "—"}
          </strong>
        </div>
      </div>

      <div className="terminal-grid">
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

                <strong>
                  {venue.synchronized
                    ? "SYNCHRONIZED"
                    : "STALE"}
                </strong>
              </div>

              <div className="terminal-grid">
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

      <div className="terminal-grid">
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
