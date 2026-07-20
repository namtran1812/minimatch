import { api } from "./client";

export interface BacktestRequest {
  symbol: string;
  side: "buy" | "sell";
  quantity: number;
  algorithm: "MARKET" | "TWAP" | "VWAP" | "POV" | "ICEBERG";
  slices?: number;
  durationSeconds?: number;
  participationRate?: number;
  displayedQuantity?: number;
  takerFeeBps?: number;
}

export interface BacktestResult {
  ok: boolean;
  complete: boolean;
  parentOrderId: string;
  requestedQuantity: number;
  filledQuantity: number;
  remainingQuantity: number;
  arrivalPrice: number;
  averageFillPrice: number;
  marketVwap: number;
  implementationShortfallBps: number;
  vwapSlippageBps: number;
  totalNotional: number;
  totalFees: number;
  acceptedChildren: number;
  rejectedChildren: number;
  fillCount: number;
}

export async function runBacktest(
  request: BacktestRequest
): Promise<BacktestResult> {
  return (await api.post("/backtest", request)).data;
}
