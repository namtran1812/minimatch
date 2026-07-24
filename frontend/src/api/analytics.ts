import { api } from "./client";
import {
  asBoolean,
  asNumber,
  asObject,
} from "./normalize";

export interface PortfolioAnalytics {
  maxDrawdown: number;
  winRate: number;
  annualizedReturn: number;
  sharpeRatio: number;
  var: number;
  expectedShortfall: number;
}

export interface PairsAnalytics {
  beta: number;
  zscore: number;
  adfTStatistic: number;
  cointegrated: boolean;
  stationary: boolean;
}

export interface OptionsAnalytics {
  price: number;
  monteCarloPrice: number;
  impliedVolatility: number;
  delta: number;
  gamma: number;
  vega: number;
}

export async function getPortfolioAnalytics():
  Promise<PortfolioAnalytics> {
  const response =
    await api.get(
      "/analytics/portfolio"
    );

  const raw =
    asObject(response.data);

  const data =
    Object.keys(
      asObject(raw.portfolio)
    ).length > 0
      ? asObject(raw.portfolio)
      : raw;

  return {
    maxDrawdown:
      asNumber(
        data.maxDrawdown
      ),

    winRate:
      asNumber(
        data.winRate
      ),

    annualizedReturn:
      asNumber(
        data.annualizedReturn
      ),

    sharpeRatio:
      asNumber(
        data.sharpeRatio
      ),

    var:
      asNumber(
        data.var
      ),

    expectedShortfall:
      asNumber(
        data.expectedShortfall
      ),
  };
}

export async function getPairsAnalytics():
  Promise<PairsAnalytics> {
  const response =
    await api.get(
      "/analytics/pairs"
    );

  const raw =
    asObject(response.data);

  const data =
    Object.keys(
      asObject(raw.pairs)
    ).length > 0
      ? asObject(raw.pairs)
      : raw;

  return {
    beta:
      asNumber(
        data.beta
      ),

    zscore:
      asNumber(
        data.zscore
      ),

    adfTStatistic:
      asNumber(
        data.adfTStatistic
      ),

    cointegrated:
      asBoolean(
        data.cointegrated
      ),

    stationary:
      asBoolean(
        data.stationary
      ),
  };
}

export async function getOptionsAnalytics():
  Promise<OptionsAnalytics> {
  const response =
    await api.get(
      "/analytics/options"
    );

  const raw =
    asObject(response.data);

  const data =
    Object.keys(
      asObject(raw.options)
    ).length > 0
      ? asObject(raw.options)
      : raw;

  return {
    price:
      asNumber(
        data.price
      ),

    monteCarloPrice:
      asNumber(
        data.monteCarloPrice
      ),

    impliedVolatility:
      asNumber(
        data.impliedVolatility
      ),

    delta:
      asNumber(
        data.delta
      ),

    gamma:
      asNumber(
        data.gamma
      ),

    vega:
      asNumber(
        data.vega
      ),
  };
}
