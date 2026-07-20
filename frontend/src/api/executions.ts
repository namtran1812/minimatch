import { api } from "./client";

export interface RouteLeg {
  venue: string;
  price: number;
  quantity: number;
  estimatedFee: number;
  effectivePrice: number;
  latencyMs: number;
  takerFeeBps: number;
  latencyCostBpsPerMs: number;
  levelIndex: number;
}

export interface RoutePreview {
  symbol: string;
  side: "BUY" | "SELL";
  requestedQuantity: number;
  routedQuantity: number;
  complete: boolean;
  averagePrice: number;
  estimatedNotional: number;
  estimatedFees: number;
  legs: RouteLeg[];
}

export interface RoutedChildExecution {
  venue: string;
  levelIndex: number;
  requestedQuantity: number;
  filledQuantity: number;
  remainingQuantity: number;
  price: number;
  notional: number;
  fee: number;
  latencyMs: number;
  status: string;
}

export interface RoutedExecution {
  executionId: number;
  createdAt?: string;
  symbol: string;
  side: string;
  requestedQuantity: number;
  filledQuantity: number;
  remainingQuantity: number;
  averageFillPrice: number;
  totalNotional: number;
  totalFees: number;
  totalLatencyMs: number;
  complete: boolean;
  children: RoutedChildExecution[];
}

export interface RouteRequest {
  symbol: string;
  side: "buy" | "sell";
  quantity: number;
  limitPrice?: number;
  maxSlippageBps?: number;
  maxVenueCount?: number;
  allOrNone?: boolean;
}

export interface ExecutionRequest extends RouteRequest {
  fillRatio?: number;
  rejectionProbability?: number;
  baseLatencyMs?: number;
  latencyJitterMs?: number;
  seed?: number;
}

export async function previewRoute(
  request: RouteRequest
): Promise<RoutePreview> {
  return (
    await api.post(
      "/router/preview",
      request
    )
  ).data;
}

export async function createExecution(
  request: ExecutionRequest
): Promise<RoutedExecution> {
  return (
    await api.post(
      "/executions",
      request
    )
  ).data;
}

export async function getExecutions():
  Promise<RoutedExecution[]> {
  return (
    await api.get("/executions")
  ).data;
}

export async function getExecution(
  id: number
): Promise<RoutedExecution> {
  return (
    await api.get(
      `/executions/${id}`
    )
  ).data;
}
