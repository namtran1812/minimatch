import {
  createContext,
  useContext,
  useMemo,
  useState,
  type ReactNode,
} from "react";

import {
  useMarketSocket,
  type MarketMode,
  type MarketSnapshot,
  type SocketStatus,
} from "../hooks/useMarketSocket";

interface MarketDataContextValue {
  mode: MarketMode;

  setMode: (
    mode: MarketMode
  ) => void;

  snapshot:
    | MarketSnapshot
    | null;

  status:
    SocketStatus;

  sendCommand: (
    command: Record<
      string,
      unknown
    >
  ) => boolean;
}

const MarketDataContext =
  createContext<
    MarketDataContextValue
    | undefined
  >(undefined);

export function MarketDataProvider({
  children,
}: {
  children: ReactNode;
}) {
  const [
    mode,
    setMode,
  ] =
    useState<MarketMode>(
      "live"
    );

  const {
    snapshot,
    status,
    sendCommand,
  } =
    useMarketSocket(
      mode
    );

  const value =
    useMemo(
      () => ({
        mode,
        setMode,
        snapshot,
        status,
        sendCommand,
      }),
      [
        mode,
        snapshot,
        status,
        sendCommand,
      ]
    );

  return (
    <MarketDataContext.Provider
      value={value}
    >
      {children}
    </MarketDataContext.Provider>
  );
}

export function useMarketData() {
  const context =
    useContext(
      MarketDataContext
    );

  if (!context) {
    throw new Error(
      "useMarketData must be used inside MarketDataProvider"
    );
  }

  return context;
}
