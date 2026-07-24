import { useQuery } from "@tanstack/react-query";

import {
  getPortfolioAnalytics,
  getPairsAnalytics,
  getOptionsAnalytics,
} from "../api/analytics";

import OptionsPricingChart from "../components/charts/OptionsPricingChart";
import PairsDiagnosticsChart from "../components/charts/PairsDiagnosticsChart";

export default function Analytics() {
  const { data: portfolio } = useQuery({
    queryKey: ["portfolio-analytics"],
    queryFn: getPortfolioAnalytics,
  });

  const { data: pairs } = useQuery({
    queryKey: ["pairs-analytics"],
    queryFn: getPairsAnalytics,
  });

  const { data: options } = useQuery({
    queryKey: ["options-analytics"],
    queryFn: getOptionsAnalytics,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">QUANTITATIVE RESEARCH</span>
        <h1>Analytics</h1>
      </div>

      <div className="analytics-info">
        <div className="analytics-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Analytics provides quantitative research tools for
            portfolio risk, statistical arbitrage, and derivatives
            pricing across MiniMatch.
          </p>
        </div>

        <div className="analytics-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Start with portfolio risk metrics, then inspect pairs
            diagnostics for statistical relationships and options
            analytics for pricing, volatility, and Greeks.
          </p>
        </div>

        <div className="analytics-info__section">
          <span className="eyebrow">
            MODEL SCOPE
          </span>

          <p>
            Current research endpoints use deterministic datasets
            for portfolio returns, synthetic paired assets, and
            option-pricing inputs so model output remains
            reproducible.
          </p>
        </div>
      </div>

      <div className="analytics-status-strip">
        <div>
          <span>PORTFOLIO</span>

          <strong>
            {portfolio
              ? "READY"
              : "LOADING"}
          </strong>
        </div>

        <div>
          <span>PAIRS</span>

          <strong>
            {pairs
              ? "READY"
              : "LOADING"}
          </strong>
        </div>

        <div>
          <span>OPTIONS</span>

          <strong>
            {options
              ? "READY"
              : "LOADING"}
          </strong>
        </div>

        <div>
          <span>MODEL</span>

          <strong>
            DETERMINISTIC
          </strong>
        </div>
      </div>

      <div className="metrics analytics-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            MAX DRAWDOWN
          </span>

          <strong>
            {portfolio
              ? `${(
                  portfolio.maxDrawdown *
                  100
                ).toFixed(2)}%`
              : "—"}
          </strong>

          <span className="metric-detail">
            PEAK-TO-TROUGH
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            WIN RATE
          </span>

          <strong>
            {portfolio
              ? `${(
                  portfolio.winRate *
                  100
                ).toFixed(1)}%`
              : "—"}
          </strong>

          <span className="metric-detail">
            POSITIVE PERIODS
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            VALUE AT RISK
          </span>

          <strong>
            {portfolio
              ? `${(
                  portfolio.var *
                  100
                ).toFixed(2)}%`
              : "—"}
          </strong>

          <span className="metric-detail">
            95% HISTORICAL
          </span>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            EXPECTED SHORTFALL
          </span>

          <strong>
            {portfolio
              ? `${(
                  portfolio.expectedShortfall *
                  100
                ).toFixed(2)}%`
              : "—"}
          </strong>

          <span className="metric-detail">
            TAIL LOSS
          </span>
        </div>
      </div>

      <div className="terminal-grid analytics-model-grid">
        <div className="panel analytics-pairs-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                STATISTICAL ARBITRAGE
              </span>

              <h2>Pairs Model</h2>
            </div>

            <strong
              className={
                pairs?.stationary
                  ? "analytics-state analytics-state--good"
                  : "analytics-state analytics-state--warn"
              }
            >
              {pairs
                ? pairs.stationary
                  ? "STATIONARY"
                  : "NON-STATIONARY"
                : "WAITING"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>HEDGE RATIO</span>

            <strong>
              {pairs
                ? Number(pairs.beta ?? 0).toFixed(4)
                : "—"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>SPREAD Z-SCORE</span>

            <strong>
              {pairs
                ? Number(pairs.zscore ?? 0).toFixed(4)
                : "—"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>ADF T-STAT</span>

            <strong>
              {pairs
                ? Number(pairs.adfTStatistic ?? 0).toFixed(4)
                : "—"}
            </strong>
          </div>
        </div>

        <div className="panel analytics-options-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                DERIVATIVES
              </span>

              <h2>Options Model</h2>
            </div>

            <strong className="analytics-state analytics-state--good">
              {options
                ? "PRICED"
                : "WAITING"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>BLACK-SCHOLES</span>

            <strong>
              {options
                ? Number(options.price ?? 0).toFixed(4)
                : "—"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>MONTE CARLO</span>

            <strong>
              {options
                ? Number(options.monteCarloPrice ?? 0).toFixed(4)
                : "—"}
            </strong>
          </div>

          <div className="analytics-detail-row">
            <span>IMPLIED VOL</span>

            <strong>
              {options
                ? `${(
                    options.impliedVolatility *
                    100
                  ).toFixed(2)}%`
                : "—"}
            </strong>
          </div>

          <div className="analytics-options-greeks">
            <div>
              <span>DELTA</span>
              <strong>
                {options
                  ? Number(options.delta ?? 0).toFixed(4)
                  : "—"}
              </strong>
            </div>

            <div>
              <span>GAMMA</span>
              <strong>
                {options
                  ? Number(options.gamma ?? 0).toFixed(4)
                  : "—"}
              </strong>
            </div>

            <div>
              <span>VEGA</span>
              <strong>
                {options
                  ? Number(options.vega ?? 0).toFixed(4)
                  : "—"}
              </strong>
            </div>
          </div>
        </div>
      </div>

      {pairs && (
        <div className="analytics-pairs-visualization">
          <PairsDiagnosticsChart
            beta={pairs.beta}
            zscore={pairs.zscore}
            adfTStatistic={
              pairs.adfTStatistic
            }
          />
        </div>
      )}

      {options && (
        <div className="analytics-visualization-grid">
          <OptionsPricingChart
            blackScholes={
              options.price
            }
            monteCarlo={
              options.monteCarloPrice
            }
          />

          <div className="panel analytics-model-validation">
            <div className="panel-title">
              <div>
                <span className="eyebrow">
                  CROSS-CHECK
                </span>

                <h2>Model Agreement</h2>
              </div>
            </div>

            <div className="analytics-validation-value">
              <span>ABSOLUTE DIFFERENCE</span>

              <strong>
                {Math.abs(
                  options.price -
                    options.monteCarloPrice
                ).toFixed(4)}
              </strong>
            </div>

            <div className="analytics-detail-row">
              <span>BLACK-SCHOLES</span>

              <strong>
                {Number(options.price ?? 0).toFixed(4)}
              </strong>
            </div>

            <div className="analytics-detail-row">
              <span>MONTE CARLO</span>

              <strong>
                {Number(options.monteCarloPrice ?? 0).toFixed(4)}
              </strong>
            </div>

            <div className="analytics-detail-row">
              <span>RELATIVE ERROR</span>

              <strong>
                {options.price !== 0
                  ? `${(
                      Math.abs(
                        options.price -
                          options.monteCarloPrice
                      ) /
                      Math.abs(options.price) *
                      100
                    ).toFixed(3)}%`
                  : "—"}
              </strong>
            </div>
          </div>
        </div>
      )}
    </section>
  );
}
