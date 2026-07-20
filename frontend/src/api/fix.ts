import { api } from "./client";

export interface FixSessionState {
  sessionId: string;
  nextIncomingSequence: number;
  nextOutgoingSequence: number;
  lastReceivedTimeNs: number;
  lastSentTimeNs: number;
  messageCount: number;
}

export interface FixMessage {
  id: number;
  sessionId: string;
  direction: "IN" | "OUT";
  sequenceNumber: number;
  messageType: string;
  wireMessage: string;
  timestampNs: number;
}

export async function getFixSession():
  Promise<FixSessionState> {
  return (
    await api.get("/fix/session")
  ).data;
}

export async function getFixMessages():
  Promise<FixMessage[]> {
  return (
    await api.get("/fix/messages")
  ).data;
}
