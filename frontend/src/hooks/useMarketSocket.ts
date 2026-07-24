import {
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

import type {
  VenueHealth,
} from "../types/market";

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
  locked: boolean;
  crossed: boolean;
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

export interface BboHistoryPoint {
  sequence: number;
  bid: number;
  midpoint: number | null;
  ask: number;
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
  | "STALE"
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

  const [bboHistory, setBboHistory] =
    useState<BboHistoryPoint[]>([]);

  const socketRef =
    useRef<WebSocket | null>(
      null
    );

  const staleTimerRef =
    useRef<
      ReturnType<typeof setTimeout>
      | null
    >(null);

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
        ?? (
          import.meta.env.DEV
            ? "ws://127.0.0.1:8089"
            : undefined
        );

      const replayUrl =
        import.meta.env
          .VITE_REPLAY_WS_URL
        ?? (
          import.meta.env.DEV
            ? "ws://127.0.0.1:8091"
            : undefined
        );

      const url =
        mode === "replay"
          ? replayUrl
          : liveUrl;

      if (!url) {
        setStatus(
          "DISCONNECTED"
        );
        return;
      }

      const socket =
        new WebSocket(url);

      socketRef.current =
        socket;

      socket.onopen = () => {
        if (
          socketRef.current !== socket
        ) {
          return;
        }

        setStatus(
          "CONNECTED"
        );
      };

      socket.onmessage = (
        event
      ) => {
        if (
          socketRef.current !== socket
        ) {
          return;
        }

        if (staleTimerRef.current) {
          clearTimeout(
            staleTimerRef.current
          );
        }

        setStatus(
          "CONNECTED"
        );

        staleTimerRef.current =
          setTimeout(() => {
            if (
              socketRef.current !== socket
            ) {
              return;
            }

            setStatus(
              "STALE"
            );
          }, 5000);

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

            if (
              Number.isFinite(
                data.bbo.bidPrice
              ) &&
              data.bbo.bidPrice > 0 &&
              Number.isFinite(
                data.bbo.askPrice
              ) &&
              data.bbo.askPrice > 0
            ) {
              setBboHistory(
                (current) => [
                  ...current.slice(-119),
                  {
                    sequence:
                      current.length > 0
                        ? current[
                            current.length - 1
                          ].sequence + 1
                        : 1,
                    bid:
                      data.bbo.bidPrice,
                    midpoint:
                      data.bbo.valid
                        ? data.bbo.midpoint
                        : null,
                    ask:
                      data.bbo.askPrice,
                  },
                ]
              );
            }
          }
        } catch (error) {
          console.error(
            "Invalid market snapshot",
            error
          );
        }
      };

      socket.onerror = () => {
        if (
          socketRef.current !== socket
        ) {
          return;
        }

        setStatus(
          "ERROR"
        );
      };

      socket.onclose = () => {
        if (
          socketRef.current !== socket
        ) {
          return;
        }

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

      if (
        staleTimerRef.current
      ) {
        clearTimeout(
          staleTimerRef.current
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

  const snapshotMatchesMode =
    snapshot !== null &&
    (
      (
        mode === "live" &&
        snapshot.type !==
          "replay_l2_snapshot"
      ) ||
      (
        mode === "replay" &&
        snapshot.type ===
          "replay_l2_snapshot"
      )
    );

  return {
    snapshot:
      snapshotMatchesMode
        ? snapshot
        : null,
    bboHistory,
    status,
    sendCommand,
  };
}
