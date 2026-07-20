import { useQuery } from "@tanstack/react-query";
import { getRiskState } from "../api/market";

export default function Risk() {
  const { data: risk } = useQuery({
    queryKey: ["risk"],
    queryFn: getRiskState,
    refetchInterval: 2000,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">PRE-TRADE + EXECUTION RISK</span>
        <h1>Risk</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">KILL SWITCH</span>
          <strong>{risk?.killed ? "ACTIVE" : "SAFE"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">POSITION</span>
          <strong>{risk?.position ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">OPEN NOTIONAL</span>
          <strong>{risk?.openNotional ?? "—"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">REALIZED PNL</span>
          <strong>{risk?.realizedPnl ?? "—"}</strong>
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Risk Controls</h2>
        </div>

        <div className="risk-item">
          <span>MAX POSITION</span>
          <strong>{risk?.maxPosition ?? "—"}</strong>
        </div>

        <div className="risk-item">
          <span>MAX OPEN NOTIONAL</span>
          <strong>{risk?.maxOpenNotional ?? "—"}</strong>
        </div>

        <div className="risk-item">
          <span>MAX DAILY LOSS</span>
          <strong>{risk?.maxDailyLoss ?? "—"}</strong>
        </div>
      </div>
    </section>
  );
}
