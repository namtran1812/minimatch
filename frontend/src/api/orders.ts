import { api } from "./client";

export interface SubmitOrderRequest {
  symbol: string;
  side: "BUY" | "SELL";
  type: "LIMIT" | "MARKET" | "IOC" | "FOK" | "POST_ONLY";
  quantity: number;
  price?: number;
  strategy: "DIRECT" | "TWAP" | "VWAP" | "POV" | "ICEBERG";
}

export async function submitOrder(order: SubmitOrderRequest) {
  const response = await api.post("/orders", order);
  return response.data;
}

export async function cancelOrder(orderId: string) {
  const response = await api.delete(`/orders/${orderId}`);
  return response.data;
}

export async function getOrders() {
  const response = await api.get("/orders");
  return response.data;
}
