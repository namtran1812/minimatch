type StatusTone =
  | "good"
  | "warn"
  | "bad"
  | "neutral";

function toneForStatus(
  value: string
): StatusTone {
  const status =
    value.trim().toUpperCase();

  if (
    [
      "CONNECTED",
      "ACTIVE",
      "SAFE",
      "HEALTHY",
      "NORMAL",
      "SYNCHRONIZED",
      "COMPLETE",
      "FILLED",
      "ACCEPTED",
      "LOGGED ON",
      "RUNNING",
      "READY",
      "CONSISTENT",
      "WORKING",
    ].includes(status)
  ) {
    return "good";
  }

  if (
    [
      "PARTIAL",
      "PARTIALLY FILLED",
      "LOCKED",
      "STALE",
      "PAUSED",
      "REPLACED",
      "CANCELLED",
      "EXPIRED",
      "CONNECTING",
    ].includes(status)
  ) {
    return "warn";
  }

  if (
    [
      "CROSSED",
      "HALTED",
      "KILLED",
      "ERROR",
      "DISCONNECTED",
      "REJECTED",
      "FAILED",
      "UNHEALTHY",
      "INCONSISTENT",
    ].includes(status)
  ) {
    return "bad";
  }

  return "neutral";
}

export default function StatusBadge({
  value,
  label,
}: {
  value: string;
  label?: string;
}) {
  const tone =
    toneForStatus(value);

  return (
    <span
      className={`status-badge status-badge--${tone}`}
    >
      <span className="status-badge__pixel" />

      {label
        ? `${label}: ${value}`
        : value}
    </span>
  );
}
