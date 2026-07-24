import { api } from "./client";
import { asArray } from "./normalize";

export interface FixSessionState {
  sessionId: string;
  nextIncomingSequence: number;
  nextOutgoingSequence: number;
  lastReceivedTimeNs: number;
  lastSentTimeNs: number;
  messageCount: number;

  inboundCount: number;
  outboundCount: number;

  lastInboundSequence: number;
  lastOutboundSequence: number;

  resendRequestCount: number;
  sequenceResetCount: number;
  rejectCount: number;
  executionReportCount: number;
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
  const response =
    await api.get("/fix/messages");

  return asArray<FixMessage>(
    response.data
  );
}
