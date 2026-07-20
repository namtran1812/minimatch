import { useQuery } from "@tanstack/react-query";
import { getSystemStats } from "../api/system";

export default function System() {
  const { data: stats } = useQuery({
    queryKey: ["system-stats"],
    queryFn: getSystemStats,
    refetchInterval: 2000,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ENGINE INTERNALS</span>
        <h1>System</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">THROUGHPUT</span>
          <strong>{stats?.throughput ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P50</span>
          <strong>{stats?.p50Ns ?? "—"} ns</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P99</span>
          <strong>{stats?.p99Ns ?? "—"} ns</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P99.9</span>
          <strong>{stats?.p999Ns ?? "—"} ns</strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="risk-item">
            <span>QUEUE DEPTH</span>
            <strong>{stats?.queueDepth ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>MAX QUEUE DEPTH</span>
            <strong>{stats?.maxQueueDepth ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>DROPPED COMMANDS</span>
            <strong>{stats?.droppedCommands ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>ACTIVE ORDERS</span>
            <strong>{stats?.activeOrders ?? "—"}</strong>
          </div>
        </div>

        <div className="panel">
          <div className="risk-item">
            <span>EVENT SEQUENCE</span>
            <strong>{stats?.eventSequence ?? "—"}</strong>
          </div>

          <div className="risk-item">
            <span>STATE HASH</span>
            <strong>{stats?.stateHash ?? "—"}</strong>
          </div>
        </div>
      </div>
    </section>
  );
}
