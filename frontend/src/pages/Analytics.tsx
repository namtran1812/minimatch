export default function Analytics() {
  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">QUANTITATIVE RESEARCH</span>
        <h1>Analytics</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">MAX DRAWDOWN</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">WIN RATE</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">VALUE AT RISK</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">EXPECTED SHORTFALL</span>
          <strong>—</strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Pairs Trading</h2>
          </div>

          <div className="risk-item">
            <span>HEDGE RATIO</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>SPREAD Z-SCORE</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>ADF T-STAT</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>STATIONARY</span>
            <strong>—</strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Options</h2>
          </div>

          <div className="risk-item">
            <span>BLACK-SCHOLES</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>IMPLIED VOL</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>DELTA</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>GAMMA</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>VEGA</span>
            <strong>—</strong>
          </div>

          <div className="risk-item">
            <span>MONTE CARLO</span>
            <strong>—</strong>
          </div>
        </div>
      </div>
    </section>
  );
}
