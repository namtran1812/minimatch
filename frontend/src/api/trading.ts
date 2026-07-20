import { api } from "./client";

export interface Trade {
  sequence: number;
  makerOrderId: number;
  takerOrderId: number;
  price: number;
  quantity: number;
  side: "BUY" | "SELL";
  symbol: number;
}

export interface ExecutionReport {
  sequence: number;
  orderId: number;
  remaining: number;
  symbol: number;
  status: number;
  rejectReason: number;
}

export interface RiskState {
  clientId: number;
  symbol: number;
  position: number;
  openNotional: number;
  realizedPnl: number;
  killed: boolean;
}

export async function getTrades(): Promise<Trade[]> {
  return (await api.get("/trades")).data;
}

export async function getReports(): Promise<ExecutionReport[]> {
  return (await api.get("/reports")).data;
}

export async function getClientRisk(
  clientId: number,
  symbol: number
): Promise<RiskState> {
  return (
    await api.get(
      `/risk/${clientId}/${symbol}`
    )
  ).data;
}

export async function cancelOrder(
  orderId: number,
  clientId: number,
  symbol: number
) {
  return (
    await api.post("/cancel", {
      orderId,
      clientId,
      symbol,
    })
  ).data;
}

export async function replaceOrder(
  orderId: number,
  clientId: number,
  price: number,
  quantity: number,
  symbol: number
) {
  return (
    await api.post("/replace", {
      orderId,
      clientId,
      price,
      quantity,
      symbol,
    })
  ).data;
}
