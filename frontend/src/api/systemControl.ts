import { api } from "./client";

export interface SystemStatus {
  globallyHalted: boolean;
}

export async function getSystemStatus(): Promise<SystemStatus> {
  return (await api.get("/system/trading-status")).data;
}

export async function haltSystem(): Promise<SystemStatus> {
  return (await api.post("/system/halt")).data;
}

export async function resumeSystem(): Promise<SystemStatus> {
  return (await api.post("/system/resume")).data;
}
