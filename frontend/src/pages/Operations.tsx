import {
  useMarketData,
} from "../context/MarketDataContext";

import type {
  VenueHealth,
} from "../hooks/useMarketSocket";

function nsToMs(ns: number) {
  return ns / 1_000_000;
}

function formatNumber(
  value: number,
  digits = 2
) {
  return value.toLocaleString(
    undefined,
    {
      maximumFractionDigits:
        digits,
    }
  );
}

function healthLabel(
  health: VenueHealth
) {
  return health.status
    .toUpperCase();
}

export default function Operations() {
  const {
    snapshot,
    status,
  } = useMarketData();

  const venues =
    snapshot?.venueHealth ?? [];

  const latency =
    snapshot?.latency ?? [];

  const healthyCount =
    venues.filter(
      (venue) =>
        venue.status ===
          "healthy" &&
        venue.synchronized
    ).length;

  const totalRate =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.messagesPerSecond,
      0
    );

  const totalGaps =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.sequenceGapCount,
      0
    );

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          LIVE MARKET OPERATIONS
        </span>

        <h1>
          Venue Health
        </h1>

        <div className="feedback">
          MARKET STREAM: {status}
        </div>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">
            HEALTHY VENUES
          </span>

          <strong>
            {healthyCount}/
            {venues.length}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            MESSAGE RATE
          </span>

          <strong>
            {formatNumber(
              totalRate
            )}/s
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            SEQUENCE GAPS
          </span>

          <strong>
            {totalGaps}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            RECORDED
          </span>

          <strong>
            {formatNumber(
              snapshot
                ?.recordedRecords
                ?? 0,
              0
            )}
          </strong>
        </div>
      </div>

      <div className="terminal-grid">
        {venues.map(
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
                  {healthLabel(
                    venue
                  )}
                </span>
              </div>

              <div className="risk-item">
                <span>
                  SYNCHRONIZED
                </span>

                <strong>
                  {venue.synchronized
                    ? "YES"
                    : "NO"}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  QUOTE AGE
                </span>

                <strong>
                  {formatNumber(
                    nsToMs(
                      venue.quoteAgeNs
                    )
                  )}{" "}
                  ms
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  MESSAGE RATE
                </span>

                <strong>
                  {formatNumber(
                    venue
                      .messagesPerSecond
                  )}
                  /s
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  TOTAL MESSAGES
                </span>

                <strong>
                  {formatNumber(
                    venue.messageCount,
                    0
                  )}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  SNAPSHOTS
                </span>

                <strong>
                  {formatNumber(
                    venue.snapshotCount,
                    0
                  )}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  UPDATES
                </span>

                <strong>
                  {formatNumber(
                    venue.updateCount,
                    0
                  )}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  RECONNECTS
                </span>

                <strong>
                  {venue.reconnectCount}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  SEQUENCE GAPS
                </span>

                <strong>
                  {
                    venue
                      .sequenceGapCount
                  }
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  REJECTIONS
                </span>

                <strong>
                  {venue.rejectedCount}
                </strong>
              </div>

              <div className="risk-item">
                <span>
                  CHECKSUM ERRORS
                </span>

                <strong>
                  {
                    venue
                      .checksumErrorCount
                  }
                </strong>
              </div>
            </div>
          )
        )}
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>
            Pipeline Latency
          </h2>
        </div>

        {latency.length === 0 && (
          <div className="feedback">
            Waiting for latency
            samples...
          </div>
        )}

        {latency.map(
          (item) => (
            <div
              className="risk-item"
              key={item.name}
            >
              <span>
                {item.name.toUpperCase()}
              </span>

              <strong>
                P50{" "}
                {formatNumber(
                  nsToMs(
                    item.p50Ns
                  )
                )}{" "}
                ms
                {" · "}
                P95{" "}
                {formatNumber(
                  nsToMs(
                    item.p95Ns
                  )
                )}{" "}
                ms
                {" · "}
                P99{" "}
                {formatNumber(
                  nsToMs(
                    item.p99Ns
                  )
                )}{" "}
                ms
                {" · "}
                N={item.count}
              </strong>
            </div>
          )
        )}
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>
            Market Recorder
          </h2>
        </div>

        <div className="risk-item">
          <span>
            STATUS
          </span>

          <strong>
            {snapshot
              ?.recordingEnabled
              ? "RECORDING"
              : "DISABLED"}
          </strong>
        </div>

        <div className="risk-item">
          <span>
            RECORDS WRITTEN
          </span>

          <strong>
            {formatNumber(
              snapshot
                ?.recordedRecords
                ?? 0,
              0
            )}
          </strong>
        </div>

        <div className="risk-item">
          <span>
            BYTES WRITTEN
          </span>

          <strong>
            {formatNumber(
              snapshot
                ?.recordedBytes
                ?? 0,
              0
            )}
          </strong>
        </div>
      </div>
    </section>
  );
}
