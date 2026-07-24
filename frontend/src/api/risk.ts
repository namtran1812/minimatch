import { api } from "./client";
import {
  asArray,
  asBoolean,
  asNumber,
  asObject,
  asString,
} from "./normalize";

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
  const response =
    await api.get("/portfolio");

  const raw =
    asObject(response.data);

  const nested =
    Object.keys(
      asObject(raw.portfolio)
    ).length > 0
      ? asObject(raw.portfolio)
      : raw;

  return {
    grossExposure:
      asNumber(nested.grossExposure),

    netExposure:
      asNumber(nested.netExposure),

    realizedPnl:
      asNumber(nested.realizedPnl),

    unrealizedPnl:
      asNumber(nested.unrealizedPnl),

    totalPnl:
      asNumber(nested.totalPnl),

    positionCount:
      asNumber(nested.positionCount),

    largestPositionSymbol:
      asString(
        nested.largestPositionSymbol
      ),

    largestPositionExposure:
      asNumber(
        nested.largestPositionExposure
      ),

    largestConcentrationPercent:
      asNumber(
        nested.largestConcentrationPercent
      ),
  };
}

export async function getPortfolioRisk():
  Promise<PortfolioRisk> {
  const response =
    await api.get("/portfolio/risk");

  const raw =
    asObject(response.data);

  const nested =
    Object.keys(
      asObject(raw.risk)
    ).length > 0
      ? asObject(raw.risk)
      : raw;

  const limits =
    asObject(nested.limits);

  return {
    breached:
      asBoolean(nested.breached),

    grossExposureBreached:
      asBoolean(
        nested.grossExposureBreached
      ),

    netExposureBreached:
      asBoolean(
        nested.netExposureBreached
      ),

    concentrationBreached:
      asBoolean(
        nested.concentrationBreached
      ),

    dailyLossBreached:
      asBoolean(
        nested.dailyLossBreached
      ),

    grossExposure:
      asNumber(nested.grossExposure),

    netExposure:
      asNumber(nested.netExposure),

    totalPnl:
      asNumber(nested.totalPnl),

    largestConcentrationPercent:
      asNumber(
        nested.largestConcentrationPercent
      ),

    limits: {
      maxGrossExposure:
        asNumber(
          limits.maxGrossExposure
        ),

      maxNetExposure:
        asNumber(
          limits.maxNetExposure
        ),

      maxConcentrationPercent:
        asNumber(
          limits.maxConcentrationPercent
        ),

      maxDailyLoss:
        asNumber(
          limits.maxDailyLoss
        ),
    },
  };
}

export async function getPositions():
  Promise<Position[]> {
  const response =
    await api.get("/positions");

  return asArray<Position>(
    response.data
  );
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
