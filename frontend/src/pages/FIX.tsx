import { useMemo } from "react";
import { useQuery } from "@tanstack/react-query";

import {
  getFixMessages,
  getFixSession,
} from "../api/fix";

import FixMessageActivityChart from "../components/charts/FixMessageActivityChart";
import FixMessageTypeChart from "../components/charts/FixMessageTypeChart";
import StatusBadge from "../components/StatusBadge";

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

function messageClass(type: string) {
  if (type === "3") {
    return "fix-message-type fix-message-type--danger";
  }

  if (
    type === "8" ||
    type === "A"
  ) {
    return "fix-message-type fix-message-type--success";
  }

  if (
    type === "2" ||
    type === "4"
  ) {
    return "fix-message-type fix-message-type--warning";
  }

  return "fix-message-type";
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

  const inbound = useMemo(
    () =>
      messages.filter(
        (message) =>
          message.direction === "IN"
      ),
    [messages]
  );

  const outbound = useMemo(
    () =>
      messages.filter(
        (message) =>
          message.direction === "OUT"
      ),
    [messages]
  );

  const recentMessages = useMemo(
    () =>
      [...messages]
        .reverse()
        .slice(0, 100),
    [messages]
  );

  const hasTraffic =
    (session?.messageCount ?? 0) > 0;

  const hasProtocolIssues =
    (session?.rejectCount ?? 0) > 0 ||
    (session?.resendRequestCount ?? 0) > 0;

  const activityData = useMemo(() => {
    if (messages.length === 0) {
      return [];
    }

    const bucketNs =
      5_000_000_000;

    const buckets =
      new Map<
        number,
        {
          inbound: number;
          outbound: number;
        }
      >();

    for (const message of messages) {
      const bucket =
        Math.floor(
          message.timestampNs /
          bucketNs
        ) *
        bucketNs;

      const current =
        buckets.get(bucket) ?? {
          inbound: 0,
          outbound: 0,
        };

      if (message.direction === "IN") {
        current.inbound += 1;
      } else {
        current.outbound += 1;
      }

      buckets.set(
        bucket,
        current
      );
    }

    return [...buckets.entries()]
      .sort(
        ([left], [right]) =>
          left - right
      )
      .slice(-30)
      .map(
        ([timestamp, counts]) => ({
          bucket:
            new Date(
              timestamp /
              1_000_000
            ).toLocaleTimeString(
              [],
              {
                minute: "2-digit",
                second: "2-digit",
              }
            ),
          ...counts,
        })
      );
  }, [messages]);

  const typeData = useMemo(() => {
    const counts =
      new Map<string, number>();

    for (const message of messages) {
      const name =
        messageTypeName(
          message.messageType
        );

      counts.set(
        name,
        (counts.get(name) ?? 0) + 1
      );
    }

    return [...counts.entries()]
      .map(([type, count]) => ({
        type,
        count,
      }))
      .sort(
        (left, right) =>
          right.count - left.count
      )
      .slice(0, 8);
  }, [messages]);

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          FIX 4.4 CONNECTIVITY
        </span>

        <h1>FIX</h1>
      </div>

      <div className="fix-info">
        <div className="fix-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            FIX monitors the protocol session used by
            MiniMatch for electronic order and execution
            messaging. It exposes sequence state, traffic,
            execution reports, rejects, and recovery events.
          </p>
        </div>

        <div className="fix-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Start with session health and sequence numbers,
            then inspect protocol diagnostics for rejects,
            resend requests, or sequence resets. The message
            stream provides the underlying FIX traffic in
            chronological protocol order.
          </p>
        </div>

        <div className="fix-info__section">
          <span className="eyebrow">
            PROTOCOL HEALTH
          </span>

          <p>
            FIX sessions maintain monotonically increasing
            inbound and outbound sequence numbers. Resend
            requests and sequence resets indicate recovery
            activity, while rejects identify invalid or
            unsupported protocol messages.
          </p>
        </div>
      </div>

      <div className="fix-status-strip">
        <div>
          <span>SESSION</span>
          <strong>
            {session?.sessionId ?? "—"}
          </strong>
        </div>

        <div>
          <span>STATE</span>
          <StatusBadge
            value={
              hasTraffic
                ? "ACTIVE"
                : "IDLE"
            }
          />
        </div>

        <div>
          <span>SEQUENCE</span>
          <StatusBadge
            value="HEALTHY"
          />
        </div>

        <div>
          <span>PROTOCOL</span>
          <StatusBadge
            value={
              hasProtocolIssues
                ? "ATTENTION"
                : "NORMAL"
            }
          />
        </div>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">
            NEXT INBOUND
          </span>

          <strong>
            {session?.nextIncomingSequence ?? "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            NEXT OUTBOUND
          </span>

          <strong>
            {session?.nextOutgoingSequence ?? "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            INBOUND
          </span>

          <strong>
            {session?.inboundCount ??
              inbound.length}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            OUTBOUND
          </span>

          <strong>
            {session?.outboundCount ??
              outbound.length}
          </strong>
        </div>
      </div>

      <div className="terminal-grid fix-session-grid">
        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                SESSION STATE
              </span>

              <h2>Sequence Control</h2>
            </div>

            <strong className="fix-state--good">
              LIVE
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>LAST INBOUND SEQUENCE</span>
            <strong>
              {session?.lastInboundSequence ?? "—"}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>LAST OUTBOUND SEQUENCE</span>
            <strong>
              {session?.lastOutboundSequence ?? "—"}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>LAST RECEIVED</span>
            <strong>
              {formatTime(
                session?.lastReceivedTimeNs ?? 0
              )}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>LAST SENT</span>
            <strong>
              {formatTime(
                session?.lastSentTimeNs ?? 0
              )}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>TOTAL MESSAGES</span>
            <strong>
              {session?.messageCount ?? 0}
            </strong>
          </div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                PROTOCOL DIAGNOSTICS
              </span>

              <h2>Recovery / Exceptions</h2>
            </div>

            <strong
              className={
                hasProtocolIssues
                  ? "fix-state--warning"
                  : "fix-state--good"
              }
            >
              {hasProtocolIssues
                ? "ATTENTION"
                : "NORMAL"}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>RESEND REQUESTS</span>
            <strong>
              {session?.resendRequestCount ?? 0}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>SEQUENCE RESETS</span>
            <strong>
              {session?.sequenceResetCount ?? 0}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>REJECTS</span>
            <strong
              className={
                (session?.rejectCount ?? 0) > 0
                  ? "fix-state--danger"
                  : ""
              }
            >
              {session?.rejectCount ?? 0}
            </strong>
          </div>

          <div className="fix-detail-row">
            <span>EXECUTION REPORTS</span>
            <strong>
              {session?.executionReportCount ?? 0}
            </strong>
          </div>
        </div>
      </div>

      {(activityData.length > 0 ||
        typeData.length > 0) && (
        <div className="fix-chart-grid">
          {activityData.length > 0 && (
            <FixMessageActivityChart
              data={activityData}
            />
          )}

          {typeData.length > 0 && (
            <FixMessageTypeChart
              data={typeData}
            />
          )}
        </div>
      )}

      <div className="panel fix-stream-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              PROTOCOL TAPE
            </span>

            <h2>FIX Message Stream</h2>
          </div>

          <span>
            {messages.length} MESSAGES
          </span>
        </div>

        <div className="fix-message-row fix-message-row--header">
          <span>SEQ</span>
          <span>DIR</span>
          <span>TYPE</span>
          <span>TIME</span>
          <span>WIRE MESSAGE</span>
        </div>

        {recentMessages.length === 0 ? (
          <div className="feedback">
            No FIX traffic recorded yet.
          </div>
        ) : (
          recentMessages.map((message) => (
            <div
              className="fix-message-row"
              key={message.id}
            >
              <strong>
                {message.sequenceNumber}
              </strong>

              <span
                className={
                  message.direction === "IN"
                    ? "fix-direction fix-direction--in"
                    : "fix-direction fix-direction--out"
                }
              >
                {message.direction}
              </span>

              <span
                className={messageClass(
                  message.messageType
                )}
              >
                {messageTypeName(
                  message.messageType
                )}
              </span>

              <span>
                {formatTime(
                  message.timestampNs
                )}
              </span>

              <code
                className="fix-wire-message"
                title={message.wireMessage}
              >
                {message.wireMessage}
              </code>
            </div>
          ))
        )}
      </div>
    </section>
  );
}
