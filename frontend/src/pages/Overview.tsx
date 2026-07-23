import { useQuery } from "@tanstack/react-query";
import { getHealth } from "../api/market";
import { getSystemStats } from "../api/system";
import {
  getPortfolio,
  getPortfolioRisk,
} from "../api/risk";
import { useMarketData } from "../context/MarketDataContext";
import BboHistoryChart from "../components/charts/BboHistoryChart";

export default function Overview() {
  const {
    snapshot,
    bboHistory,
    status,
  } = useMarketData();

  const { data: health } = useQuery({
    queryKey: ["health"],
    queryFn: getHealth,
    refetchInterval: 2000,
  });

  const { data: system } = useQuery({
    queryKey: ["system-stats"],
    queryFn: getSystemStats,
    refetchInterval: 2000,
  });

  const { data: portfolio } = useQuery({
    queryKey: ["overview-portfolio"],
    queryFn: getPortfolio,
    refetchInterval: 2000,
  });

  const { data: portfolioRisk } = useQuery({
    queryKey: ["overview-portfolio-risk"],
    queryFn: getPortfolioRisk,
    refetchInterval: 2000,
  });

  const venues = snapshot?.venueHealth ?? [];

  const healthyVenues = venues.filter(
    (venue) =>
      venue.status === "healthy" &&
      venue.synchronized
  ).length;

  const messageRate = venues.reduce(
    (sum, venue) =>
      sum + venue.messagesPerSecond,
    0
  );

  const totalSequenceGaps =
    venues.reduce(
      (sum, venue) =>
        sum +
        venue.sequenceGapCount,
      0
    );

  const recorderEnabled =
    snapshot?.recordingEnabled ??
    false;

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          SYSTEM OVERVIEW
        </span>

        <h1>MiniMatch Control Plane</h1>

        <div className="feedback">
          MARKET STREAM: {status}
        </div>
      </div>

      <div className="overview-info">
        <div className="overview-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Overview summarizes the most important live
            MiniMatch state across matching, market data,
            latency, venue health, and trading activity.
          </p>
        </div>

        <div className="overview-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Use this page as the control-plane landing view.
            Confirm engine and market-data health first, then
            drill into Trading, Risk, Operations, or System
            when a metric needs deeper investigation.
          </p>
        </div>

        <div className="overview-info__section">
          <span className="eyebrow">
            LIVE SOURCES
          </span>

          <p>
            Engine counters come from the system API while
            venue health and throughput are driven by the
            shared live market-data stream.
          </p>
        </div>
      </div>

      <div className="overview-status-strip">
        <div>
          <span>MARKET STREAM</span>

          <strong
            className={
              status === "CONNECTED"
                ? "overview-state--good"
                : "overview-state--warn"
            }
          >
            {status.toUpperCase()}
          </strong>
        </div>

        <div>
          <span>ENGINE</span>

          <strong
            className={
              health?.engine === "healthy"
                ? "overview-state--good"
                : "overview-state--warn"
            }
          >
            {health?.engine?.toUpperCase()
              ?? "OFFLINE"}
          </strong>
        </div>

        <div>
          <span>VENUES</span>

          <strong
            className={
              venues.length > 0 &&
              healthyVenues === venues.length
                ? "overview-state--good"
                : "overview-state--warn"
            }
          >
            {healthyVenues}/{venues.length}
          </strong>
        </div>

        <div>
          <span>STATE</span>

          <strong
            className={
              status === "CONNECTED" &&
              health?.engine === "healthy" &&
              venues.length > 0 &&
              healthyVenues === venues.length
                ? "overview-state--good"
                : "overview-state--warn"
            }
          >
            {status === "CONNECTED" &&
            health?.engine === "healthy" &&
            venues.length > 0 &&
            healthyVenues === venues.length
              ? "NORMAL"
              : "ATTENTION"}
          </strong>
        </div>
      </div>

      <div className="metrics overview-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            ENGINE
          </span>

          <strong>
            {health?.engine?.toUpperCase()
              ?? "OFFLINE"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            ORDERS
          </span>

          <strong>
            {system?.submitted ?? 0}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            TRADES
          </span>

          <strong>
            {system?.trades ?? 0}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            ACTIVE ORDERS
          </span>

          <strong>
            {system?.activeOrders ?? 0}
          </strong>
        </div>
      </div>

      <div className="metrics overview-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            P50 ORDER LATENCY
          </span>

          <strong>
            {system?.p50Ns ?? 0}
            <span className="metric-unit">
              {" "}ns
            </span>
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            P99 ORDER LATENCY
          </span>

          <strong>
            {system?.p99Ns ?? 0}
            <span className="metric-unit">
              {" "}ns
            </span>
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            HEALTHY VENUES
          </span>

          <strong>
            {healthyVenues}/{venues.length}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            MARKET MSG RATE
          </span>

          <strong>
            {messageRate.toLocaleString(
              undefined,
              {
                maximumFractionDigits: 0,
              }
            )}
            <span className="metric-unit">
              {" "}/s
            </span>
          </strong>
        </div>
      </div>

      <div className="overview-chart-grid">
        <BboHistoryChart
          data={bboHistory}
        />
      </div>

      <div className="overview-health-grid">
        <div className="panel overview-health-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                MATCHING ENGINE
              </span>

              <h2>Engine Health</h2>
            </div>

            <strong
              className={
                health?.engine === "healthy"
                  ? "overview-state--good"
                  : "overview-state--warn"
              }
            >
              {health?.engine?.toUpperCase() ?? "OFFLINE"}
            </strong>
          </div>

          <div className="overview-health-row">
            <span>SUBMITTED</span>
            <strong>{system?.submitted ?? 0}</strong>
          </div>

          <div className="overview-health-row">
            <span>ACCEPTED</span>
            <strong>{system?.accepted ?? 0}</strong>
          </div>

          <div className="overview-health-row">
            <span>REJECTED</span>
            <strong>{system?.rejected ?? 0}</strong>
          </div>

          <div className="overview-health-row">
            <span>P99 LATENCY</span>
            <strong>
              {system?.p99Ns ?? 0} ns
            </strong>
          </div>
        </div>

        <div className="panel overview-health-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                PORTFOLIO
              </span>

              <h2>Risk Health</h2>
            </div>

            <strong
              className={
                portfolioRisk?.breached
                  ? "overview-state--warn"
                  : "overview-state--good"
              }
            >
              {portfolioRisk
                ? portfolioRisk.breached
                  ? "BREACHED"
                  : "WITHIN LIMITS"
                : "WAITING"}
            </strong>
          </div>

          <div className="overview-health-row">
            <span>GROSS EXPOSURE</span>
            <strong>
              {portfolio?.grossExposure?.toLocaleString() ?? "—"}
            </strong>
          </div>

          <div className="overview-health-row">
            <span>NET EXPOSURE</span>
            <strong>
              {portfolio?.netExposure?.toLocaleString() ?? "—"}
            </strong>
          </div>

          <div className="overview-health-row">
            <span>TOTAL PNL</span>
            <strong>
              {portfolio?.totalPnl?.toLocaleString() ?? "—"}
            </strong>
          </div>

          <div className="overview-health-row">
            <span>CONCENTRATION</span>
            <strong>
              {portfolio
                ? `${portfolio.largestConcentrationPercent.toFixed(2)}%`
                : "—"}
            </strong>
          </div>
        </div>
      </div>

      <div className="panel overview-operations-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              OPERATIONS
            </span>

            <h2>Market Infrastructure</h2>
          </div>

          <strong
            className={
              venues.length > 0 &&
              healthyVenues === venues.length &&
              totalSequenceGaps === 0
                ? "overview-state--good"
                : "overview-state--warn"
            }
          >
            {venues.length === 0
              ? "WAITING"
              : healthyVenues === venues.length &&
                  totalSequenceGaps === 0
                ? "NORMAL"
                : "ATTENTION"}
          </strong>
        </div>

        <div className="overview-operations-grid">
          <div>
            <span>HEALTHY VENUES</span>

            <strong>
              {healthyVenues}/{venues.length}
            </strong>
          </div>

          <div>
            <span>MESSAGE RATE</span>

            <strong>
              {messageRate.toLocaleString(
                undefined,
                {
                  maximumFractionDigits: 0,
                }
              )}
              /s
            </strong>
          </div>

          <div>
            <span>SEQUENCE GAPS</span>

            <strong>
              {totalSequenceGaps}
            </strong>
          </div>

          <div>
            <span>RECORDER</span>

            <strong>
              {recorderEnabled
                ? "RECORDING"
                : "IDLE"}
            </strong>
          </div>
        </div>
      </div>

      <div className="panel overview-venue-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              VENUE CONNECTIVITY
            </span>

            <h2>Market Data Feeds</h2>
          </div>

          <span>
            {healthyVenues}/{venues.length} HEALTHY
          </span>
        </div>

        {venues.length === 0 ? (
          <div className="feedback">
            Waiting for live market data...
          </div>
        ) : (
          <div className="overview-venue-table">
            <div className="overview-venue-row overview-venue-row--header">
              <span>VENUE</span>
              <span>STATE</span>
              <span>SYNC</span>
              <span>QUOTE AGE</span>
              <span>MSG RATE</span>
            </div>

            {venues.map((venue) => (
              <div
                className="overview-venue-row"
                key={venue.venue}
              >
                <strong>
                  {venue.venue.toUpperCase()}
                </strong>

                <span>
                  {venue.status.toUpperCase()}
                </span>

                <span>
                  {venue.synchronized
                    ? "YES"
                    : "NO"}
                </span>

                <span>
                  {(
                    venue.quoteAgeNs /
                    1_000_000
                  ).toFixed(2)}
                  {" ms"}
                </span>

                <strong>
                  {venue.messagesPerSecond.toFixed(2)}
                  {"/s"}
                </strong>
              </div>
            ))}
          </div>
        )}
      </div>
    </section>
  );
}
