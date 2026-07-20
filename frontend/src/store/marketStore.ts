import { create } from "zustand";
import type {
  LatencyMetrics,
  OrderBook,
  Trade,
  VenueHealth,
} from "../types/market";

interface MarketStore {
  book: OrderBook | null;
  trades: Trade[];
  venues: VenueHealth[];
  metrics: LatencyMetrics | null;

  setBook: (book: OrderBook) => void;
  addTrade: (trade: Trade) => void;
  setVenues: (venues: VenueHealth[]) => void;
  setMetrics: (metrics: LatencyMetrics) => void;
}

export const useMarketStore = create<MarketStore>((set) => ({
  book: null,
  trades: [],
  venues: [],
  metrics: null,

  setBook: (book) => set({ book }),

  addTrade: (trade) =>
    set((state) => ({
      trades: [trade, ...state.trades].slice(0, 100),
    })),

  setVenues: (venues) => set({ venues }),

  setMetrics: (metrics) => set({ metrics }),
}));
