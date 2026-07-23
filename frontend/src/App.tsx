import {
  useEffect,
  useState,
} from "react";
import Sidebar, { type Page } from "./components/layout/Sidebar";
import ProductAssistant from "./components/assistant/ProductAssistant";

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
import Operations from "./pages/Operations";
import Journal from "./pages/Journal";

function App() {
  const [page, setPage] =
    useState<Page>("overview");

  const [commandOpen, setCommandOpen] =
    useState(false);

  const [commandQuery, setCommandQuery] =
    useState("");

  const [quickTradeSide, setQuickTradeSide] =
    useState<"BUY" | "SELL" | null>(null);

  const [focusCancel, setFocusCancel] =
    useState(false);

  const [focusGlobalHalt, setFocusGlobalHalt] =
    useState(false);

  function openQuickTrade(
    side: "BUY" | "SELL"
  ) {
    setQuickTradeSide(side);
    setFocusCancel(false);
    setFocusGlobalHalt(false);
    setPage("trading");
  }

  function openRiskHalt() {
    setQuickTradeSide(null);
    setFocusCancel(false);
    setFocusGlobalHalt(true);
    setPage("risk");
  }

  function openPage(
    nextPage: Page
  ) {
    setQuickTradeSide(null);
    setFocusCancel(
      nextPage === "trading"
    );
    setFocusGlobalHalt(false);
    setPage(nextPage);
  }

  function changePage(
    nextPage: Page
  ) {
    setQuickTradeSide(null);
    setPage(nextPage);
  }

  useEffect(() => {
    function handleCommandKey(
      event: KeyboardEvent
    ) {
      const target =
        event.target as HTMLElement | null;

      if (
        target?.tagName === "INPUT" ||
        target?.tagName === "SELECT" ||
        target?.tagName === "TEXTAREA" ||
        target?.isContentEditable
      ) {
        return;
      }

      if (
        event.key === "/" ||
        (
          event.ctrlKey &&
          event.key.toLowerCase() === "k"
        )
      ) {
        event.preventDefault();
        setCommandOpen(
          (current) => {
            const next = !current;

            if (next) {
              setCommandQuery("");
            }

            return next;
          }
        );
      }

      if (event.key === "Escape") {
        setCommandOpen(false);
        setCommandQuery("");
      }
    }

    window.addEventListener(
      "keydown",
      handleCommandKey
    );

    return () => {
      window.removeEventListener(
        "keydown",
        handleCommandKey
      );
    };
  }, []);

  const commands: Array<{
    label: string;
    keywords: string;
    run: () => void;
  }> = [
    {
      label: "Markets",
      keywords: "markets book quotes market data",
      run: () => openPage("markets"),
    },
    {
      label: "Buy Order",
      keywords: "buy trading order",
      run: () => openQuickTrade("BUY"),
    },
    {
      label: "Sell Order",
      keywords: "sell trading order",
      run: () => openQuickTrade("SELL"),
    },
    {
      label: "Cancel Order",
      keywords: "cancel order trading",
      run: () => openPage("trading"),
    },
    {
      label: "Execution",
      keywords: "execution routing fills",
      run: () => openPage("execution"),
    },
    {
      label: "Risk",
      keywords: "risk limits exposure",
      run: () => openPage("risk"),
    },
    {
      label: "Replay",
      keywords: "replay deterministic history",
      run: () => openPage("replay"),
    },
    {
      label: "Operations",
      keywords: "operations latency venues health",
      run: () => openPage("operations"),
    },
    {
      label: "Engineering Journal",
      keywords: "journal documentation reflections build engineering decisions",
      run: () => openPage("journal"),
    },
    {
      label: "Global Halt",
      keywords: "kill halt risk stop trading",
      run: openRiskHalt,
    },
  ];

  const normalizedCommandQuery =
    commandQuery.trim().toLowerCase();

  const filteredCommands =
    normalizedCommandQuery.length === 0
      ? commands
      : commands.filter((command) =>
          (
            command.label +
            " " +
            command.keywords
          )
            .toLowerCase()
            .includes(
              normalizedCommandQuery
            )
        );

  const content = {
    overview: <Overview />,
    markets: (
      <Markets
        onQuickTrade={openQuickTrade}
        onOpenPage={openPage}
        onOpenRiskHalt={openRiskHalt}
      />
    ),
    trading: (
      <Trading
        quickSide={quickTradeSide}
        focusCancel={focusCancel}
      />
    ),
    execution: <Execution />,
    oms: <OMS />,
    risk: (
      <Risk
        focusGlobalHalt={
          focusGlobalHalt
        }
      />
    ),
    fix: <FIX />,
    replay: <Replay />,
    backtest: <Backtest />,
    analytics: <Analytics />,
    system: <System />,
    operations: <Operations />,
    journal: <Journal />,
  };

  return (
    <div className="app-shell">
      <Sidebar
        page={page}
        onChange={changePage}
      />

      <main className="main-content">


        {commandOpen && (
          <div className="command-palette">
            <div className="command-palette__header">
              <span>COMMAND</span>
              <span>ESC TO CLOSE</span>
            </div>

            <input
              autoFocus
              className="command-palette__input"
              placeholder="TYPE A COMMAND..."
              value={commandQuery}
              onChange={(event) =>
                setCommandQuery(
                  event.target.value
                )
              }
            />

            <div className="command-palette__results">
              {filteredCommands.length === 0 && (
                <div className="command-palette__empty">
                  NO MATCHING COMMAND
                </div>
              )}

              {filteredCommands.map(
                (command) => (
                  <button
                    key={command.label}
                    onClick={() => {
                      command.run();
                      setCommandOpen(false);
                      setCommandQuery("");
                    }}
                  >
                    <span>
                      {command.label}
                    </span>

                    <kbd>↵</kbd>
                  </button>
                )
              )}
            </div>
          </div>
        )}

        {content[page]}

        <ProductAssistant
          page={page}
        />
      </main>
    </div>
  );
}

export default App;
