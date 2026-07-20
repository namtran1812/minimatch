import {
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

export type MarketMode =
  | "live"
  | "replay";

export interface MarketLevel {
  price: number;
  quantity: number;
}

export interface MarketVenue {
  venue: string;
  sequence: number;
  synchronized: boolean;
  bids: MarketLevel[];
  asks: MarketLevel[];
}

export interface MarketBbo {
  valid: boolean;
  bidPrice: number;
  bidQuantity: number;
  bidVenue: string;
  askPrice: number;
  askQuantity: number;
  askVenue: string;
  midpoint: number;
  spread: number;
}

export interface RouteLeg {
  venue: string;
  price: number;
  quantity: number;
  effectivePrice: number;
  estimatedFee?: number;
}

export interface RoutePlan {
  complete: boolean;
  requestedQuantity: number;
  routedQuantity: number;
  averagePrice: number;
  estimatedFees: number;
  legs: RouteLeg[];
}

export interface VenueHealth {
  venue: string;
  status:
    | "unknown"
    | "healthy"
    | "delayed"
    | "stale"
    | "disconnected";
  synchronized: boolean;
  lastMessageNs: number;
  quoteAgeNs: number;
  messagesPerSecond: number;
  messageCount: number;
  snapshotCount: number;
  updateCount: number;
  reconnectCount: number;
  rejectedCount: number;
  sequenceGapCount: number;
  checksumErrorCount: number;
}

export interface LatencySnapshot {
  name: string;
  count: number;
  minimumNs: number;
  maximumNs: number;
  averageNs: number;
  p50Ns: number;
  p95Ns: number;
  p99Ns: number;
}

export interface ReplayState {
  status: string;
  recordIndex: number;
  totalRecords: number;
  rejectedRecords: number;
  speed: number;
  generation: number;
  complete: boolean;
  checksum: number;
  firstTimestampNs: number;
  currentTimestampNs: number;
}

export interface MarketSnapshot {
  type:
    | "live_l2_snapshot"
    | "replay_l2_snapshot"
    | "l2_snapshot";

  mode?: MarketMode;
  symbol: string;
  timestampNs: number;

  bbo: MarketBbo;
  venues: MarketVenue[];

  venueHealth?: VenueHealth[];
  latency?: LatencySnapshot[];

  recordingEnabled?: boolean;
  recordedRecords?: number;
  recordedBytes?: number;

  replay?: ReplayState;

  routing: {
    buy: RoutePlan;
    sell: RoutePlan;
  };
}

export type SocketStatus =
  | "CONNECTING"
  | "CONNECTED"
  | "DISCONNECTED"
  | "ERROR";

export function useMarketSocket(
  mode: MarketMode = "live"
) {
  const [snapshot, setSnapshot] =
    useState<MarketSnapshot | null>(
      null
    );

  const [status, setStatus] =
    useState<SocketStatus>(
      "CONNECTING"
    );

  const socketRef =
    useRef<WebSocket | null>(
      null
    );

  useEffect(() => {
    let reconnectTimer:
      | ReturnType<typeof setTimeout>
      | undefined;

    let closed = false;

    function connect() {
      if (closed) {
        return;
      }

      setStatus(
        "CONNECTING"
      );

      const liveUrl =
        import.meta.env
          .VITE_MARKET_WS_URL
        ?? "ws://127.0.0.1:8090";

      const replayUrl =
        import.meta.env
          .VITE_REPLAY_WS_URL
        ?? "ws://127.0.0.1:8091";

      const url =
        mode === "replay"
          ? replayUrl
          : liveUrl;

      const socket =
        new WebSocket(url);

      socketRef.current =
        socket;

      socket.onopen = () => {
        setStatus(
          "CONNECTED"
        );
      };

      socket.onmessage = (
        event
      ) => {
        try {
          const data =
            JSON.parse(
              event.data
            ) as MarketSnapshot;

          if (
            data.type ===
              "l2_snapshot" ||
            data.type ===
              "live_l2_snapshot" ||
            data.type ===
              "replay_l2_snapshot"
          ) {
            setSnapshot(data);
          }
        } catch (error) {
          console.error(
            "Invalid market snapshot",
            error
          );
        }
      };

      socket.onerror = () => {
        setStatus(
          "ERROR"
        );
      };

      socket.onclose = () => {
        socketRef.current =
          null;

        if (closed) {
          return;
        }

        setStatus(
          "DISCONNECTED"
        );

        reconnectTimer =
          setTimeout(
            connect,
            2000
          );
      };
    }

    setSnapshot(null);

    connect();

    return () => {
      closed = true;

      if (
        reconnectTimer
      ) {
        clearTimeout(
          reconnectTimer
        );
      }

      socketRef.current
        ?.close();
    };
  }, [mode]);

  const sendCommand =
    useCallback(
      (
        command:
          Record<
            string,
            unknown
          >
      ) => {
        const socket =
          socketRef.current;

        if (
          !socket ||
          socket.readyState !==
            WebSocket.OPEN
        ) {
          return false;
        }

        socket.send(
          JSON.stringify(
            command
          )
        );

        return true;
      },
      []
    );

  return {
    snapshot,
    status,
    sendCommand,
  };
}
