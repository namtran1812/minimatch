import { useEffect, useRef, useState } from "react";

export interface MarketMessage {
  type: "book" | "trade" | "venue" | "metrics";
  data: unknown;
}

export function useMarketSocket(
  symbol: string,
  onMessage: (message: MarketMessage) => void
) {
  const socketRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const base = import.meta.env.VITE_WS_URL ?? "ws://127.0.0.1:8081/ws";
    const socket = new WebSocket(`${base}/market/${symbol}`);

    socketRef.current = socket;

    socket.onopen = () => setConnected(true);
    socket.onclose = () => setConnected(false);

    socket.onerror = (event) => {
      console.error("MiniMatch websocket error", event);
    };

    socket.onmessage = (event) => {
      try {
        onMessage(JSON.parse(event.data));
      } catch (error) {
        console.error("Invalid market message", error);
      }
    };

    return () => socket.close();
  }, [symbol, onMessage]);

  return {
    connected,
    socket: socketRef.current,
  };
}
