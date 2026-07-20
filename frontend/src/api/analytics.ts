import { api } from "./client";

export async function getPortfolioAnalytics() {
  return (await api.get("/analytics/portfolio")).data;
}

export async function getPairsAnalytics() {
  return (await api.get("/analytics/pairs")).data;
}

export async function getOptionsAnalytics() {
  return (await api.get("/analytics/options")).data;
}
