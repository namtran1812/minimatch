import { api } from "./client";

export type ExecutionAlgorithm =
  | "MARKET"
  | "TWAP"
  | "VWAP"
  | "POV"
  | "ICEBERG";

export interface AlgoOrderRequest {
  symbol: string;
  side: "BUY" | "SELL";
  quantity: number;
  algorithm: ExecutionAlgorithm;
  slices?: number;
  durationSeconds?: number;
  participationRate?: number;
  displayedQuantity?: number;
  maxSlippageBps?: number;
  maxVenueCount?: number;
}

export interface AlgoOrderResponse {
  ok: boolean;
  parentOrderId: string;
  status: string;
}

export async function submitAlgoOrder(
  request: AlgoOrderRequest
): Promise<AlgoOrderResponse> {
  return (
    await api.post(
      "/algo-orders",
      request
    )
  ).data;
}
