import { useQuery } from "@tanstack/react-query";
import {
  getPortfolioAnalytics,
  getPairsAnalytics,
  getOptionsAnalytics,
} from "../api/analytics";

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

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">MAX DRAWDOWN</span>
          <strong>{portfolio?.maxDrawdown ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">WIN RATE</span>
          <strong>{portfolio?.winRate ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">VALUE AT RISK</span>
          <strong>{portfolio?.var ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">EXPECTED SHORTFALL</span>
          <strong>{portfolio?.expectedShortfall ?? "—"}</strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="risk-item">
            <span>HEDGE RATIO</span>
            <strong>{pairs?.beta ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>SPREAD Z-SCORE</span>
            <strong>{pairs?.zscore ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>ADF T-STAT</span>
            <strong>{pairs?.adfTStatistic ?? "—"}</strong>
          </div>
        </div>

        <div className="panel">
          <div className="risk-item">
            <span>BLACK-SCHOLES</span>
            <strong>{options?.price ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>IMPLIED VOL</span>
            <strong>{options?.impliedVolatility ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>DELTA</span>
            <strong>{options?.delta ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>GAMMA</span>
            <strong>{options?.gamma ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>VEGA</span>
            <strong>{options?.vega ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>MONTE CARLO</span>
            <strong>{options?.monteCarloPrice ?? "—"}</strong>
          </div>
        </div>
      </div>
    </section>
  );
}
