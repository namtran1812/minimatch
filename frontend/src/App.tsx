import { useState } from "react";
import Sidebar, { type Page } from "./components/layout/Sidebar";

import Overview from "./pages/Overview";
import Markets from "./pages/Markets";
import Trading from "./pages/Trading";
import Execution from "./pages/Execution";
import OMS from "./pages/OMS";
import Risk from "./pages/Risk";
import FIX from "./pages/FIX";
import Replay from "./pages/Replay";
import Backtest from "./pages/Backtest";
import Analytics from "./pages/Analytics";
import System from "./pages/System";

function App() {
  const [page, setPage] = useState<Page>("overview");

  const content = {
    overview: <Overview />,
    markets: <Markets />,
    trading: <Trading />,
    execution: <Execution />,
    oms: <OMS />,
    risk: <Risk />,
    fix: <FIX />,
    replay: <Replay />,
    backtest: <Backtest />,
    analytics: <Analytics />,
    system: <System />,
  };

  return (
    <div className="app-shell">
      <Sidebar page={page} onChange={setPage} />

      <main className="main-content">
        {content[page]}
      </main>
    </div>
  );
}

export default App;
