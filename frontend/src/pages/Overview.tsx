import { useQuery } from "@tanstack/react-query";
import {
  getHealth,
  getLatencyMetrics,
  getVenueHealth,
} from "../api/market";

export default function Overview() {
  const { data: health } = useQuery({
    queryKey: ["health"],
    queryFn: getHealth,
    refetchInterval: 2000,
  });

  const { data: metrics } = useQuery({
    queryKey: ["metrics"],
    queryFn: getLatencyMetrics,
    refetchInterval: 2000,
  });

  const { data: venues } = useQuery({
    queryKey: ["venues"],
    queryFn: getVenueHealth,
    refetchInterval: 2000,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">SYSTEM OVERVIEW</span>
        <h1>MiniMatch Control Plane</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">ENGINE</span>
          <strong>{health?.status ?? "OFFLINE"}</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">THROUGHPUT</span>
          <strong>
            {metrics?.throughput?.toLocaleString() ?? "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P50</span>
          <strong>{metrics?.p50Ns ?? "—"} ns</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">P99</span>
          <strong>{metrics?.p99Ns ?? "—"} ns</strong>
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">VENUE CONNECTIVITY</span>
            <h2>Market Data Feeds</h2>
          </div>
        </div>

        {venues?.map((venue) => (
          <div className="venue" key={venue.venue}>
            <strong>{venue.venue}</strong>
            <span>
              {venue.status} · {venue.latencyMs} ms ·{" "}
              {venue.messageRate} msg/s
            </span>
          </div>
        )) ?? "No venue data"}
      </div>
    </section>
  );
}
