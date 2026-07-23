import {
  useEffect,
  useState,
} from "react";

import type {
  Trade,
} from "../api/trading";

function isTrade(
  value: unknown
): value is Trade {
  if (
    typeof value !== "object" ||
    value === null
  ) {
    return false;
  }

  const candidate =
    value as Partial<Trade>;

  return (
    typeof candidate.sequence === "number" &&
    typeof candidate.price === "number" &&
    typeof candidate.quantity === "number" &&
    (
      candidate.side === "BUY" ||
      candidate.side === "SELL"
    )
  );
}

export function useTradeSocket() {
  const [trades, setTrades] =
    useState<Trade[]>([]);

  const [connected, setConnected] =
    useState(false);

  useEffect(() => {
    let closed = false;
    let reconnectTimer:
      | ReturnType<typeof setTimeout>
      | undefined;

    let socket: WebSocket | null =
      null;

    function connect() {
      if (closed) {
        return;
      }

      const baseUrl =
        import.meta.env
          .VITE_DROP_COPY_WS_URL
        ?? "ws://127.0.0.1:8092";

      socket = new WebSocket(
        baseUrl
      );

      socket.onopen = () => {
        setConnected(true);
      };

      socket.onmessage = (
        event
      ) => {
        try {
          const data: unknown =
            JSON.parse(
              event.data
            );

          // The drop-copy socket also
          // carries execution reports.
          // Ignore everything except
          // actual matching-engine trades.
          if (!isTrade(data)) {
            return;
          }

          setTrades(
            (current) => {
              if (
                current.some(
                  (trade) =>
                    trade.sequence ===
                    data.sequence
                )
              ) {
                return current;
              }

              return [
                data,
                ...current,
              ].slice(0, 100);
            }
          );
        } catch {
          // Ignore malformed frames.
        }
      };

      socket.onerror = () => {
        setConnected(false);
      };

      socket.onclose = () => {
        setConnected(false);

        if (!closed) {
          reconnectTimer =
            setTimeout(
              connect,
              1000
            );
        }
      };
    }

    connect();

    return () => {
      closed = true;

      if (reconnectTimer) {
        clearTimeout(
          reconnectTimer
        );
      }

      if (
        socket &&
        (
          socket.readyState ===
            WebSocket.OPEN ||
          socket.readyState ===
            WebSocket.CONNECTING
        )
      ) {
        socket.onopen = null;
        socket.onmessage = null;
        socket.onerror = null;
        socket.onclose = null;

        socket.close();
      }
    };
  }, []);

  return {
    trades,
    connected,
  };
}
