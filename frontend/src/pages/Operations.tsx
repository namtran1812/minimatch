import {
  useMarketData,
} from "../context/MarketDataContext";

import StatusBadge from "../components/StatusBadge";

import {
  useQuery,
} from "@tanstack/react-query";

import {
  getGrafanaHealth,
  getPrometheusHealth,
} from "../api/observability";

import OperationsLatencyChart from "../components/charts/OperationsLatencyChart";

import type {
  VenueHealth,
} from "../types/market";

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

  const {
    data: prometheusHealth,
  } = useQuery({
    queryKey: [
      "prometheus-health",
    ],
    queryFn:
      getPrometheusHealth,
    refetchInterval: 5000,
  });

  const {
    data: grafanaHealth,
  } = useQuery({
    queryKey: [
      "grafana-health",
    ],
    queryFn:
      getGrafanaHealth,
    refetchInterval: 5000,
  });

  const venues =
    Array.isArray(snapshot?.venueHealth)
      ? snapshot.venueHealth
      : [];

  const latency =
    Array.isArray(snapshot?.latency)
      ? snapshot.latency
      : [];

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
        Number(
          venue.messagesPerSecond ?? 0
        ),
      0
    );

  const totalGaps =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.sequenceGapCount,
      0
    );

  const totalRejects =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.rejectedCount,
      0
    );

  const totalChecksumErrors =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.checksumErrorCount,
      0
    );

  const totalReconnects =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.reconnectCount,
      0
    );

  const worstP99 =
    latency.reduce(
      (maximum, item) =>
        Math.max(
          maximum,
          nsToMs(item.p99Ns)
        ),
      0
    );

  const allVenuesHealthy =
    venues.length > 0 &&
    healthyCount ===
      venues.length;

  const integrityHealthy =
    totalGaps === 0 &&
    totalRejects === 0 &&
    totalChecksumErrors === 0;

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          LIVE MARKET OPERATIONS
        </span>

        <h1>Operations</h1>
      </div>

      <div className="operations-info">
        <div className="operations-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Operations monitors the live market-data
            infrastructure behind MiniMatch, including
            venue synchronization, message throughput,
            sequence integrity, pipeline latency, and
            market recording.
          </p>
        </div>

        <div className="operations-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Start with stream and venue health, then
            inspect sequence gaps, rejects, reconnects,
            and latency percentiles. Venue panels expose
            the source-level telemetry behind aggregate
            system health.
          </p>
        </div>

        <div className="operations-info__section">
          <span className="eyebrow">
            INCIDENT SIGNALS
          </span>

          <p>
            Unsynchronized venues, sequence gaps,
            checksum errors, elevated P99 latency, or
            reconnect activity indicate degraded market
            data quality and should be investigated
            before relying on downstream state.
          </p>
        </div>
      </div>

      <div className="operations-status-strip">
        <div>
          <span>MARKET STREAM</span>

          <StatusBadge
            value={status}
          />
        </div>

        <div>
          <span>VENUES</span>

          <StatusBadge
            value={
              allVenuesHealthy
                ? "HEALTHY"
                : venues.length === 0
                  ? "WAITING"
                  : "DEGRADED"
            }
          />
        </div>

        <div>
          <span>INTEGRITY</span>

          <StatusBadge
            value={
              integrityHealthy
                ? "NORMAL"
                : "ATTENTION"
            }
          />
        </div>

        <div>
          <span>RECORDER</span>

          <StatusBadge
            value={
              snapshot
                ?.recordingEnabled
                ? "RECORDING"
                : "IDLE"
            }
          />
        </div>
      </div>

      <div className="metrics operations-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            HEALTHY VENUES
          </span>

          <strong>
            {healthyCount}/
            {venues.length}
          </strong>

          <span className="metric-detail">
            SYNCHRONIZED
          </span>
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

          <span className="metric-detail">
            AGGREGATE
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            SEQUENCE GAPS
          </span>

          <strong>
            {totalGaps}
          </strong>

          <span className="metric-detail">
            ALL VENUES
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            WORST P99
          </span>

          <strong>
            {formatNumber(
              worstP99
            )} ms
          </strong>

          <span className="metric-detail">
            PIPELINE LATENCY
          </span>
        </div>
      </div>

      <div className="operations-section-heading">
        <div>
          <span className="eyebrow">
            MARKET DATA
          </span>

          <h2>Venue Health</h2>
        </div>

        <span>
          {healthyCount}
          {" / "}
          {venues.length}
          {" HEALTHY"}
        </span>
      </div>

      {venues.length === 0 ? (
        <div className="panel">
          <div className="feedback">
            Waiting for venue telemetry...
          </div>
        </div>
      ) : (
        <div className="operations-venue-grid">
          {venues.map(
            (venue) => (
              <div
                className="panel operations-venue-panel"
                key={venue.venue}
              >
                <div className="panel-title">
                  <div>
                    <span className="eyebrow">
                      VENUE
                    </span>

                    <h2>
                      {venue.venue.toUpperCase()}
                    </h2>
                  </div>

                  <StatusBadge
                    value={
                      healthLabel(
                        venue
                      )
                    }
                  />
                </div>

                <div className="operations-detail-row">
                  <span>
                    SYNCHRONIZED
                  </span>

                  <strong>
                    {venue.synchronized
                      ? "YES"
                      : "NO"}
                  </strong>
                </div>

                <div className="operations-detail-row">
                  <span>
                    QUOTE AGE
                  </span>

                  <strong>
                    {formatNumber(
                      nsToMs(
                        Number(
                          venue.quoteAgeNs ?? 0
                        )
                      )
                    )} ms
                  </strong>
                </div>

                <div className="operations-detail-row">
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

                <div className="operations-detail-row">
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

                <div className="operations-detail-row">
                  <span>SNAPSHOTS</span>

                  <strong>
                    {formatNumber(
                      venue.snapshotCount,
                      0
                    )}
                  </strong>
                </div>

                <div className="operations-detail-row">
                  <span>UPDATES</span>

                  <strong>
                    {formatNumber(
                      venue.updateCount,
                      0
                    )}
                  </strong>
                </div>

                <div className="operations-venue-incidents">
                  <div>
                    <span>RECONNECTS</span>
                    <strong>
                      {venue.reconnectCount}
                    </strong>
                  </div>

                  <div>
                    <span>GAPS</span>
                    <strong>
                      {
                        venue
                          .sequenceGapCount
                      }
                    </strong>
                  </div>

                  <div>
                    <span>REJECTS</span>
                    <strong>
                      {venue.rejectedCount}
                    </strong>
                  </div>

                  <div>
                    <span>CHECKSUM</span>
                    <strong>
                      {
                        venue
                          .checksumErrorCount
                      }
                    </strong>
                  </div>
                </div>
              </div>
            )
          )}
        </div>
      )}

      <div className="operations-section-heading">
        <div>
          <span className="eyebrow">
            PERFORMANCE
          </span>

          <h2>Pipeline Latency</h2>
        </div>

        <span>
          {latency.length}
          {" STAGES"}
        </span>
      </div>

      {latency.length > 0 && (
        <div className="operations-chart-grid">
          <OperationsLatencyChart
            data={latency}
          />
        </div>
      )}

      <div className="panel operations-latency-panel">
        {latency.length === 0 && (
          <div className="feedback">
            Waiting for latency samples...
          </div>
        )}

        {latency.length > 0 && (
          <div className="operations-latency-table">
            <div className="operations-latency-row operations-latency-row--header">
              <span>STAGE</span>
              <span>P50</span>
              <span>P95</span>
              <span>P99</span>
              <span>SAMPLES</span>
            </div>

            {latency.map(
              (item) => {
                const p99 =
                  nsToMs(
                    item.p99Ns
                  );

                return (
                  <div
                    className="operations-latency-row"
                    key={item.name}
                  >
                    <strong>
                      {item.name.toUpperCase()}
                    </strong>

                    <span>
                      {formatNumber(
                        nsToMs(
                          item.p50Ns
                        )
                      )} ms
                    </span>

                    <span>
                      {formatNumber(
                        nsToMs(
                          item.p95Ns
                        )
                      )} ms
                    </span>

                    <strong
                      className={
                        p99 >= 10
                          ? "operations-latency--hot"
                          : p99 >= 5
                            ? "operations-latency--warn"
                            : ""
                      }
                    >
                      {formatNumber(
                        p99
                      )} ms
                    </strong>

                    <span>
                      {formatNumber(
                        item.count,
                        0
                      )}
                    </span>
                  </div>
                );
              }
            )}
          </div>
        )}
      </div>

      <div className="operations-section-heading">
        <div>
          <span className="eyebrow">
            OBSERVABILITY
          </span>

          <h2>Prometheus & Grafana</h2>
        </div>

        <span>
          EXTERNAL TELEMETRY
        </span>
      </div>

      <div className="operations-observability-grid">
        <div className="panel operations-observability-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                METRICS
              </span>

              <h2>Prometheus</h2>
            </div>

            <StatusBadge
              value={
                prometheusHealth?.ok
                  ? "HEALTHY"
                  : "OFFLINE"
              }
            />
          </div>

          <div className="operations-detail-row">
            <span>SCRAPE INTERVAL</span>

            <strong>5s</strong>
          </div>

          <div className="operations-detail-row">
            <span>MINIMATCH TARGET</span>

            <strong>
              /metrics
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>RECONCILIATION TARGET</span>

            <strong>
              :8081/metrics
            </strong>
          </div>

          <a
            className="operations-observability-link"
            href="http://127.0.0.1:9090"
            target="_blank"
            rel="noreferrer"
          >
            OPEN PROMETHEUS
          </a>
        </div>

        <div className="panel operations-observability-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                DASHBOARDS
              </span>

              <h2>Grafana</h2>
            </div>

            <StatusBadge
              value={
                grafanaHealth?.ok
                  ? "HEALTHY"
                  : "OFFLINE"
              }
            />
          </div>

          <div className="operations-detail-row">
            <span>DASHBOARD</span>

            <strong>
              RECONCILIATION & RECOVERY
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>DATA SOURCE</span>

            <strong>
              PROMETHEUS
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>UID</span>

            <strong>
              minimatch-reconciliation
            </strong>
          </div>

          <a
            className="operations-observability-link"
            href="http://127.0.0.1:3000/d/minimatch-reconciliation"
            target="_blank"
            rel="noreferrer"
          >
            OPEN GRAFANA
          </a>
        </div>
      </div>

      <div className="operations-observability-capabilities">
        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                PROMETHEUS
              </span>

              <h2>Metric Surface</h2>
            </div>
          </div>

          <div className="operations-capability-grid">
            <div>
              <span>ENGINE METRICS</span>
              <strong>/metrics</strong>
            </div>

            <div>
              <span>RECONCILIATION</span>
              <strong>:8081/metrics</strong>
            </div>

            <div>
              <span>SCRAPE INTERVAL</span>
              <strong>5s</strong>
            </div>

            <div>
              <span>RULE EVALUATION</span>
              <strong>5s</strong>
            </div>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                GRAFANA
              </span>

              <h2>Dashboard Surface</h2>
            </div>
          </div>

          <div className="operations-capability-grid">
            <div>
              <span>DASHBOARD</span>
              <strong>RECONCILIATION</strong>
            </div>

            <div>
              <span>RECOVERY</span>
              <strong>ENABLED</strong>
            </div>

            <div>
              <span>DROP COPY</span>
              <strong>MONITORED</strong>
            </div>

            <div>
              <span>POSITIONS</span>
              <strong>MONITORED</strong>
            </div>
          </div>
        </div>
      </div>

      <div className="operations-bottom-grid">
        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                DATA INTEGRITY
              </span>

              <h2>
                Incident Counters
              </h2>
            </div>

            <StatusBadge
              value={
                integrityHealthy
                  ? "NORMAL"
                  : "ATTENTION"
              }
            />
          </div>

          <div className="operations-detail-row">
            <span>
              SEQUENCE GAPS
            </span>

            <strong>
              {totalGaps}
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>
              REJECTED MESSAGES
            </span>

            <strong>
              {totalRejects}
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>
              CHECKSUM ERRORS
            </span>

            <strong>
              {totalChecksumErrors}
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>
              RECONNECTS
            </span>

            <strong>
              {totalReconnects}
            </strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                MARKET RECORDER
              </span>

              <h2>
                Capture State
              </h2>
            </div>

            <StatusBadge
              value={
                snapshot
                  ?.recordingEnabled
                  ? "RECORDING"
                  : "IDLE"
              }
            />
          </div>

          <div className="operations-detail-row">
            <span>STATUS</span>

            <strong>
              {snapshot
                ?.recordingEnabled
                ? "RECORDING"
                : "DISABLED"}
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>
              RECORDS WRITTEN
            </span>

            <strong>
              {formatNumber(
                snapshot
                  ?.recordedRecords ??
                  0,
                0
              )}
            </strong>
          </div>

          <div className="operations-detail-row">
            <span>
              BYTES WRITTEN
            </span>

            <strong>
              {formatNumber(
                snapshot
                  ?.recordedBytes ??
                  0,
                0
              )}
            </strong>
          </div>
        </div>
      </div>
    </section>
  );
}
