import React from "react";
import ReactDOM from "react-dom/client";
import {
  QueryClient,
  QueryClientProvider,
} from "@tanstack/react-query";

import App from "./App";
import AppErrorBoundary from "./components/system/AppErrorBoundary";
import {
  ProductContextProvider,
} from "./context/ProductContext";
import { MarketDataProvider } from "./context/MarketDataContext";
import "./index.css";

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      retry: 2,
      refetchOnWindowFocus: false,
    },
  },
});

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <QueryClientProvider client={queryClient}>
      <MarketDataProvider>
      <AppErrorBoundary>
        <ProductContextProvider>
          <App />
        </ProductContextProvider>
      </AppErrorBoundary>
    </MarketDataProvider>
    </QueryClientProvider>
  </React.StrictMode>
);
