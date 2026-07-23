import { api } from "./client";

export interface SubmitOrderRequest {
  orderId: number;
  clientId: number;
  symbol: number;
  side: "BUY" | "SELL";
  type:
    | "LIMIT"
    | "MARKET"
    | "IOC"
    | "FOK"
    | "POST_ONLY";
  quantity: number;
  price?: number;
}

export interface SubmitOrderResponse {
  ok: boolean;
  orderId: number;
  activeOrders?: number;
  stateHash?: string;
}

export async function submitOrder(
  order: SubmitOrderRequest
): Promise<SubmitOrderResponse> {
  return (
    await api.post(
      "/orders",
      order
    )
  ).data;
}
