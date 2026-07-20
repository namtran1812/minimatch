import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { getClientRisk } from "../api/trading";

export default function Risk() {
  const [clientId, setClientId] = useState(20);
  const [symbol, setSymbol] = useState(1);

  const { data: risk } = useQuery({
    queryKey: ["client-risk", clientId, symbol],
    queryFn: () => getClientRisk(clientId, symbol),
    refetchInterval: 1000,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">REAL-TIME RISK</span>
        <h1>Risk</h1>
      </div>

      <div className="panel">
        <label>
          CLIENT ID
          <input
            type="number"
            value={clientId}
            onChange={(e) => setClientId(Number(e.target.value))}
          />
        </label>

        <label>
          SYMBOL
          <input
            type="number"
            value={symbol}
            onChange={(e) => setSymbol(Number(e.target.value))}
          />
        </label>
      </div>

      <div className="metrics">
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

        <div className="metric-card">
          <span className="eyebrow">KILL SWITCH</span>
          <strong>
            {risk?.killed ? "ACTIVE" : "SAFE"}
          </strong>
        </div>
      </div>
    </section>
  );
}
