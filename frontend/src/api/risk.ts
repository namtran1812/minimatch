import { api } from "./client";

export interface PortfolioSummary {
  grossExposure: number;
  netExposure: number;
  realizedPnl: number;
  unrealizedPnl: number;
  totalPnl: number;
  positionCount: number;
  largestPositionSymbol: string;
  largestPositionExposure: number;
  largestConcentrationPercent: number;
}

export interface PortfolioRiskLimits {
  maxGrossExposure: number;
  maxNetExposure: number;
  maxConcentrationPercent: number;
  maxDailyLoss: number;
}

export interface PortfolioRisk {
  breached: boolean;
  grossExposureBreached: boolean;
  netExposureBreached: boolean;
  concentrationBreached: boolean;
  dailyLossBreached: boolean;

  grossExposure: number;
  netExposure: number;
  totalPnl: number;
  largestConcentrationPercent: number;

  limits: PortfolioRiskLimits;
}

export interface Position {
  symbol: string;
  netQuantity: number;
  averageCost: number;
  markPrice: number;
  realizedPnl: number;
  unrealizedPnl: number;
  totalPnl: number;
}

export async function getPortfolio():
  Promise<PortfolioSummary> {
  return (
    await api.get("/portfolio")
  ).data;
}

export async function getPortfolioRisk():
  Promise<PortfolioRisk> {
  return (
    await api.get("/portfolio/risk")
  ).data;
}

export async function getPositions():
  Promise<Position[]> {
  return (
    await api.get("/positions")
  ).data;
}

export async function updatePortfolioRiskLimits(
  limits: Partial<PortfolioRiskLimits>
) {
  return (
    await api.post(
      "/portfolio/risk/limits",
      limits
    )
  ).data;
}

export interface SymbolTradingStatus {
  symbol: number;
  halted: boolean;
  globallyHalted: boolean;
}

export interface CircuitBreakerConfig {
  enabled?: boolean;
  thresholdPercent?: number;
  windowSeconds?: number;
}

export async function getSymbolStatus(
  symbol: number
): Promise<SymbolTradingStatus> {
  return (
    await api.get(
      `/symbols/${symbol}/status`
    )
  ).data;
}

export async function haltSymbol(
  symbol: number
) {
  return (
    await api.post(
      `/symbols/${symbol}/halt`
    )
  ).data;
}

export async function resumeSymbol(
  symbol: number
) {
  return (
    await api.post(
      `/symbols/${symbol}/resume`
    )
  ).data;
}

export async function configureCircuitBreaker(
  config: CircuitBreakerConfig
) {
  return (
    await api.post(
      "/system/circuit-breakers/config",
      config
    )
  ).data;
}

export type StpPolicy =
  | "NONE"
  | "CANCEL_NEWEST"
  | "CANCEL_OLDEST"
  | "CANCEL_BOTH";

export interface StpStatus {
  policy: StpPolicy;
}

export interface CircuitBreakerStatus {
  enabled: boolean;
  thresholdPercent: number;
  windowSeconds: number;
  currentMidpoint: number;
  referenceMidpoint: number;
  movePercent: number;
  symbolHalted: boolean;
  observations: number;
}

export interface PriceBandRequest {
  referencePrice: number;
  bandPercent: number;
}

export async function getStpStatus():
  Promise<StpStatus> {
  return (
    await api.get("/system/stp")
  ).data;
}

export async function setStpPolicy(
  policy: StpPolicy
): Promise<StpStatus> {
  return (
    await api.post(
      "/system/stp",
      { policy }
    )
  ).data;
}

export async function getCircuitBreaker():
  Promise<CircuitBreakerStatus> {
  return (
    await api.get(
      "/system/circuit-breakers"
    )
  ).data;
}

export async function setPriceBand(
  symbol: number,
  request: PriceBandRequest
) {
  return (
    await api.post(
      `/symbols/${symbol}/price-band`,
      request
    )
  ).data;
}

export async function clearPriceBand(
  symbol: number
) {
  return (
    await api.delete(
      `/symbols/${symbol}/price-band`
    )
  ).data;
}
