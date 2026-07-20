import { api } from "./client";

export async function getSystemStats() {
  return (await api.get("/system")).data;
}
