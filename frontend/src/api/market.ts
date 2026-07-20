import { api } from "./client";
import type {
  LatencyMetrics,
  OrderBook,
  RiskState,
  VenueHealth,
} from "../types/market";

export async function getHealth() {
  const response = await api.get("/health");
  return response.data;
}

export async function getOrderBook(symbol: string): Promise<OrderBook> {
  const response = await api.get(`/market/book/${symbol}`);
  return response.data;
}

export async function getVenueHealth(): Promise<VenueHealth[]> {
  const response = await api.get("/venues");
  return response.data;
}

export async function getLatencyMetrics(): Promise<LatencyMetrics> {
  const response = await api.get("/metrics/latency");
  return response.data;
}

export async function getRiskState(): Promise<RiskState> {
  const response = await api.get("/risk");
  return response.data;
}
