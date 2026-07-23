/* eslint-disable react-refresh/only-export-components */
import {
  createContext,
  useContext,
  useMemo,
  useState,
  type ReactNode,
} from "react";

import {
  useMarketSocket,
  type BboHistoryPoint,
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

  bboHistory:
    BboHistoryPoint[];

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
    bboHistory,
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
        bboHistory,
        status,
        sendCommand,
      }),
      [
        mode,
        snapshot,
        bboHistory,
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
