export default function FIX() {
  const rows = [
    ["SESSION", "ACTIVE"],
    ["INBOUND SEQ", "12842"],
    ["OUTBOUND SEQ", "12839"],
    ["HEARTBEAT", "30s"],
    ["RESEND REQUESTS", "0"],
    ["PERSISTENCE", "HEALTHY"],
  ];

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">FIX SESSION LAYER</span>
        <h1>FIX</h1>
      </div>

      <div className="metrics">
        {rows.map(([label, value]) => (
          <div className="metric-card" key={label}>
            <span className="eyebrow">{label}</span>
            <strong>{value}</strong>
          </div>
        ))}
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Session State</h2>
        </div>

        <div className="risk-item">
          <span>LOGON</span>
          <strong className="safe">ACCEPTED</strong>
        </div>

        <div className="risk-item">
          <span>SEQUENCE STATE</span>
          <strong>IN SYNC</strong>
        </div>

        <div className="risk-item">
          <span>LAST HEARTBEAT</span>
          <strong>2s AGO</strong>
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Message Store</h2>
        </div>

        <div className="feedback">
          Wire persistent inbound/outbound FIX messages and resend history here.
        </div>
      </div>
    </section>
  );
}
