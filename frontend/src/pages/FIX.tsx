import { useQuery } from "@tanstack/react-query";
import {
  getFixMessages,
  getFixSession,
} from "../api/fix";

function messageTypeName(code: string) {
  const names: Record<string, string> = {
    A: "LOGON",
    "5": "LOGOUT",
    "0": "HEARTBEAT",
    "1": "TEST REQUEST",
    "2": "RESEND REQUEST",
    "4": "SEQUENCE RESET",
    D: "NEW ORDER SINGLE",
    F: "ORDER CANCEL REQUEST",
    "8": "EXECUTION REPORT",
    "3": "REJECT",
  };

  return names[code] ?? code;
}

function formatTime(ns: number) {
  if (!ns) {
    return "—";
  }

  return new Date(
    ns / 1_000_000
  ).toLocaleTimeString();
}

export default function FIX() {
  const {
    data: session,
  } = useQuery({
    queryKey: ["fix-session"],
    queryFn: getFixSession,
    refetchInterval: 1000,
  });

  const {
    data: messages = [],
  } = useQuery({
    queryKey: ["fix-messages"],
    queryFn: getFixMessages,
    refetchInterval: 1000,
  });

  const inbound =
    messages.filter(
      (message) =>
        message.direction === "IN"
    );

  const outbound =
    messages.filter(
      (message) =>
        message.direction === "OUT"
    );

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          FIX 4.4 SESSION MONITOR
        </span>

        <h1>FIX</h1>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">
            SESSION
          </span>

          <strong>
            {session?.sessionId ?? "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            NEXT INBOUND
          </span>

          <strong>
            {
              session?.nextIncomingSequence
              ?? "—"
            }
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            NEXT OUTBOUND
          </span>

          <strong>
            {
              session?.nextOutgoingSequence
              ?? "—"
            }
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            MESSAGES
          </span>

          <strong>
            {session?.messageCount ?? 0}
          </strong>
        </div>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Session State</h2>
          </div>

          <div className="risk-item">
            <span>
              LAST RECEIVED
            </span>

            <strong>
              {formatTime(
                session?.lastReceivedTimeNs
                  ?? 0
              )}
            </strong>
          </div>

          <div className="risk-item">
            <span>
              LAST SENT
            </span>

            <strong>
              {formatTime(
                session?.lastSentTimeNs
                  ?? 0
              )}
            </strong>
          </div>

          <div className="risk-item">
            <span>
              INBOUND MESSAGES
            </span>

            <strong>
              {inbound.length}
            </strong>
          </div>

          <div className="risk-item">
            <span>
              OUTBOUND MESSAGES
            </span>

            <strong>
              {outbound.length}
            </strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>
              Sequence Health
            </h2>
          </div>

          <div className="risk-item">
            <span>
              EXPECTED INBOUND
            </span>

            <strong>
              {
                session?.nextIncomingSequence
                ?? "—"
              }
            </strong>
          </div>

          <div className="risk-item">
            <span>
              NEXT OUTBOUND
            </span>

            <strong>
              {
                session?.nextOutgoingSequence
                ?? "—"
              }
            </strong>
          </div>

          <div className="risk-item">
            <span>
              SESSION HEALTH
            </span>

            <strong>
              {
                session &&
                session.messageCount > 0
                  ? "ACTIVE"
                  : "IDLE"
              }
            </strong>
          </div>
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>FIX Message Stream</h2>
        </div>

        {messages.length === 0 && (
          <div className="feedback">
            No FIX traffic recorded yet.
          </div>
        )}

        {[...messages]
          .reverse()
          .map((message) => (
            <div
              className="tape-row"
              key={message.id}
            >
              <span>
                #{message.sequenceNumber}
              </span>

              <span>
                {message.direction}
              </span>

              <span>
                {messageTypeName(
                  message.messageType
                )}
              </span>

              <span>
                {formatTime(
                  message.timestampNs
                )}
              </span>

              <span
                title={
                  message.wireMessage
                }
              >
                {
                  message.wireMessage
                }
              </span>
            </div>
          ))}
      </div>
    </section>
  );
}
