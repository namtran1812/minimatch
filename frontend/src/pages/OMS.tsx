export default function OMS() {
  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ORDER MANAGEMENT SYSTEM</span>
        <h1>OMS</h1>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Parent Orders</h2>
        </div>

        <div className="tape-header">
          <span>ID</span>
          <span>STATUS</span>
          <span>SYMBOL</span>
          <span>FILLED</span>
          <span>STRATEGY</span>
        </div>

        <div className="feedback">
          Waiting for OMS API data.
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Child Orders & Fills</h2>
        </div>

        <div className="feedback">
          Select a parent order to inspect child lifecycle and fills.
        </div>
      </div>
    </section>
  );
}
