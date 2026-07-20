export default function System() {
  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ENGINE INTERNALS</span>
        <h1>System</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">THROUGHPUT</span>
          <strong>12.64M</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P50</span>
          <strong>42 ns</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P99</span>
          <strong>167 ns</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P99.9</span>
          <strong>833 ns</strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Matching Pipeline</h2>
          </div>

          <div className="risk-item">
            <span>SPSC QUEUE</span>
            <strong className="safe">HEALTHY</strong>
          </div>

          <div className="risk-item">
            <span>DROPPED COMMANDS</span>
            <strong>0</strong>
          </div>

          <div className="risk-item">
            <span>MAX QUEUE DEPTH</span>
            <strong>55,744</strong>
          </div>

          <div className="risk-item">
            <span>MATCHING THREAD</span>
            <strong className="safe">ACTIVE</strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Persistence</h2>
          </div>

          <div className="risk-item">
            <span>EVENT JOURNAL</span>
            <strong className="safe">SYNCED</strong>
          </div>

          <div className="risk-item">
            <span>SNAPSHOT</span>
            <strong>AVAILABLE</strong>
          </div>

          <div className="risk-item">
            <span>EXECUTION STORE</span>
            <strong className="safe">HEALTHY</strong>
          </div>

          <div className="risk-item">
            <span>FIX STORE</span>
            <strong className="safe">HEALTHY</strong>
          </div>
        </div>
      </div>
    </section>
  );
}
