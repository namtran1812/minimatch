import {
  useEffect,
  useState,
} from "react";

import { useQuery } from "@tanstack/react-query";

import {
  getSystemStats,
} from "../api/system";

import StatusBadge from "../components/StatusBadge";
import SystemLatencyHistoryChart, {
  type SystemLatencyPoint,
} from "../components/charts/SystemLatencyHistoryChart";

import SystemLatencyDistributionChart from "../components/charts/SystemLatencyDistributionChart";

function formatNumber(
  value: number,
  digits = 0
) {
  return value.toLocaleString(
    undefined,
    {
      maximumFractionDigits:
        digits,
    }
  );
}

function formatLatency(
  value: number | null | undefined
) {
  const ns = Number(value ?? 0);

  if (!Number.isFinite(ns)) {
    return "—";
  }

  if (ns >= 1_000_000) {
    return `${(
      ns / 1_000_000
    ).toFixed(2)} ms`;
  }

  if (ns >= 1_000) {
    return `${(
      ns / 1_000
    ).toFixed(2)} µs`;
  }

  return `${ns.toFixed(0)} ns`;
}

export default function System() {
  const [
    latencyHistory,
    setLatencyHistory,
  ] = useState<SystemLatencyPoint[]>(
    []
  );

  const {
    data: stats,
  } = useQuery({
    queryKey: [
      "system-stats",
    ],
    queryFn: getSystemStats,
    refetchInterval: 1000,
  });

  const submitted =
    stats?.submitted ?? 0;

  const accepted =
    stats?.accepted ?? 0;

  const rejected =
    stats?.rejected ?? 0;

  const trades =
    stats?.trades ?? 0;

  const reports =
    stats?.reports ?? 0;

  const activeOrders =
    stats?.activeOrders ?? 0;

  const p50 =
    stats?.p50Ns ?? 0;

  const p95 =
    stats?.p95Ns ?? 0;

  const p99 =
    stats?.p99Ns ?? 0;

  const acceptanceRate =
    submitted > 0
      ? accepted /
        submitted *
        100
      : 0;

  const rejectionRate =
    submitted > 0
      ? rejected /
        submitted *
        100
      : 0;

  const engineActive =
    stats !== undefined;

  const latencyHealthy =
    p99 > 0
      ? p99 < 10_000_000
      : true;

  useEffect(() => {
    if (!stats) {
      return;
    }

    const timer =
      window.setTimeout(
        () => {
          const nextPoint = {
            sample: 0,

            label:
              new Date()
                .toLocaleTimeString(
                  [],
                  {
                    minute:
                      "2-digit",
                    second:
                      "2-digit",
                  }
                ),

            p50:
              stats.p50Ns /
              1_000_000,

            p95:
              stats.p95Ns /
              1_000_000,

            p99:
              stats.p99Ns /
              1_000_000,
          };

          setLatencyHistory(
            (current) => [
              ...current,
              {
                ...nextPoint,

                sample:
                  (
                    current[
                      current.length -
                        1
                    ]?.sample ?? 0
                  ) + 1,
              },
            ].slice(-60)
          );
        },
        0
      );

    return () => {
      window.clearTimeout(
        timer
      );
    };
  }, [
    stats,
  ]);

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          ENGINE INTERNALS
        </span>

        <h1>System</h1>
      </div>

      <div className="system-info">
        <div className="system-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            System exposes the internal state of the
            MiniMatch matching engine, including order
            throughput, acceptance and rejection counts,
            active orders, execution reports, latency,
            and deterministic state hash.
          </p>
        </div>

        <div className="system-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Use the order counters to verify engine
            activity, inspect latency percentiles for
            performance degradation, and compare the
            state hash when validating deterministic
            engine behavior across runs.
          </p>
        </div>

        <div className="system-info__section">
          <span className="eyebrow">
            ENGINE SEMANTICS
          </span>

          <p>
            Submitted orders may be accepted or rejected.
            Accepted orders can remain active or generate
            trades and execution reports. P50, P95, and
            P99 describe order-submit latency.
          </p>
        </div>
      </div>

      <div className="system-status-strip">
        <div>
          <span>ENGINE</span>

          <StatusBadge
            value={
              engineActive
                ? "ACTIVE"
                : "OFFLINE"
            }
          />
        </div>

        <div>
          <span>LATENCY</span>

          <StatusBadge
            value={
              latencyHealthy
                ? "NORMAL"
                : "ATTENTION"
            }
          />
        </div>

        <div>
          <span>ACTIVE ORDERS</span>

          <strong>
            {formatNumber(
              activeOrders
            )}
          </strong>
        </div>

        <div>
          <span>STATE</span>

          <StatusBadge
            value={
              stats?.stateHash
                ? "VALID"
                : "WAITING"
            }
          />
        </div>
      </div>

      <div className="metrics system-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            SUBMITTED
          </span>

          <strong>
            {formatNumber(
              submitted
            )}
          </strong>

          <span className="metric-detail">
            ORDER REQUESTS
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            ACCEPTED
          </span>

          <strong>
            {formatNumber(
              accepted
            )}
          </strong>

          <span className="metric-detail">
            {acceptanceRate.toFixed(
              1
            )}
            % RATE
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            REJECTED
          </span>

          <strong>
            {formatNumber(
              rejected
            )}
          </strong>

          <span className="metric-detail">
            {rejectionRate.toFixed(
              1
            )}
            % RATE
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            TRADES
          </span>

          <strong>
            {formatNumber(
              trades
            )}
          </strong>

          <span className="metric-detail">
            EXECUTIONS
          </span>
        </div>
      </div>

      <div className="system-section-heading">
        <div>
          <span className="eyebrow">
            PERFORMANCE
          </span>

          <h2>
            Order Submit Latency
          </h2>
        </div>

        <span>
          ORDER_SUBMIT
        </span>
      </div>

      <div className="metrics system-latency-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            P50
          </span>

          <strong>
            {formatLatency(
              p50
            )}
          </strong>

          <span className="metric-detail">
            MEDIAN
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            P95
          </span>

          <strong>
            {formatLatency(
              p95
            )}
          </strong>

          <span className="metric-detail">
            TAIL
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            P99
          </span>

          <strong
            className={
              latencyHealthy
                ? ""
                : "system-latency--hot"
            }
          >
            {formatLatency(
              p99
            )}
          </strong>

          <span className="metric-detail">
            WORST TAIL
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            ACTIVE ORDERS
          </span>

          <strong>
            {formatNumber(
              activeOrders
            )}
          </strong>

          <span className="metric-detail">
            RESTING
          </span>
        </div>
      </div>

      <div className="system-chart-grid">
        <SystemLatencyHistoryChart
          data={latencyHistory}
        />

        <SystemLatencyDistributionChart
          p50={
            p50 / 1_000_000
          }
          p95={
            p95 / 1_000_000
          }
          p99={
            p99 / 1_000_000
          }
        />
      </div>

      <div className="system-state-grid">
        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                ENGINE STATE
              </span>

              <h2>
                Lifecycle Counters
              </h2>
            </div>

            <StatusBadge
              value="LIVE"
            />
          </div>

          <div className="system-detail-row">
            <span>
              SUBMITTED ORDERS
            </span>

            <strong>
              {formatNumber(
                submitted
              )}
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              ACCEPTED ORDERS
            </span>

            <strong>
              {formatNumber(
                accepted
              )}
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              REJECTED ORDERS
            </span>

            <strong>
              {formatNumber(
                rejected
              )}
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              TRADES
            </span>

            <strong>
              {formatNumber(
                trades
              )}
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              EXECUTION REPORTS
            </span>

            <strong>
              {formatNumber(
                reports
              )}
            </strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                DETERMINISTIC STATE
              </span>

              <h2>
                Engine Identity
              </h2>
            </div>

            <StatusBadge
              value={
                stats?.stateHash
                  ? "VALID"
                  : "WAITING"
              }
            />
          </div>

          <div className="system-detail-row">
            <span>
              ACTIVE ORDERS
            </span>

            <strong>
              {formatNumber(
                activeOrders
              )}
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              STATE HASH
            </span>

            <code className="system-state-hash">
              {stats?.stateHash ??
                "—"}
            </code>
          </div>

          <div className="system-detail-row">
            <span>
              ACCEPTANCE RATE
            </span>

            <strong>
              {acceptanceRate.toFixed(
                2
              )}
              %
            </strong>
          </div>

          <div className="system-detail-row">
            <span>
              REJECTION RATE
            </span>

            <strong>
              {rejectionRate.toFixed(
                2
              )}
              %
            </strong>
          </div>
        </div>
      </div>
    </section>
  );
}
