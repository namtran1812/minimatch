export interface BookLevel {
  price: number;
  quantity: number;
  orderCount?: number;
}

export interface OrderBook {
  symbol: string;
  bids: BookLevel[];
  asks: BookLevel[];
  sequence: number;
}

export interface Trade {
  id: string;
  timestamp: number;
  symbol: string;
  side: "BUY" | "SELL";
  price: number;
  quantity: number;
  venue: string;
}

export interface VenueHealth {
  venue: string;
  status: "HEALTHY" | "DELAYED" | "STALE" | "DISCONNECTED";
  latencyMs: number;
  messageRate: number;
}

export interface LatencyMetrics {
  throughput: number;
  p50Ns: number;
  p95Ns: number;
  p99Ns: number;
  p999Ns: number;
}

export interface RiskState {
  killed: boolean;
  position: number;
  maxPosition: number;
  openNotional: number;
  maxOpenNotional: number;
  realizedPnl: number;
  maxDailyLoss: number;
}
