import { api } from "./client";

export interface DropCopyEvent {
  id: number;
  timestampNs: number;
  orderId: number;
  symbol: number;
  eventType: string;
  status: string;
  remaining: number;
  rejectReason: string;
}

export async function getDropCopyEvents():
  Promise<DropCopyEvent[]> {
  return (
    await api.get("/drop-copy")
  ).data;
}
