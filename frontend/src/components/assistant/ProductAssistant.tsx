import {
  Bot,
  Send,
  X,
} from "lucide-react";

import {
  useState,
} from "react";

import {
  askAssistant,
  type AssistantMessage,
} from "../../api/assistant";

import {
  useMarketData,
} from "../../context/MarketDataContext";
import {
  useProductContext,
} from "../../context/ProductContext";

import {
  useQuery,
} from "@tanstack/react-query";

import {
  getSystemStats,
} from "../../api/system";

import {
  getCircuitBreaker,
  getPortfolio,
  getPortfolioRisk,
  getPositions,
  getStpStatus,
} from "../../api/risk";

import {
  getChildOrders,
  getFills,
  getParentOrders,
} from "../../api/oms";

import {
  getExecutionQuality,
  getParentReconciliation,
} from "../../api/quality";

interface Props {
  page: string;
}

export default function ProductAssistant({
  page,
}: Props) {
  const {
    snapshot,
    status: marketStreamStatus,
  } = useMarketData();

  const {
    selectedOmsParentId,
  } = useProductContext();

  const {
    data: systemStats,
  } = useQuery({
    queryKey: [
      "assistant-system-stats",
    ],
    queryFn: getSystemStats,
    enabled:
      page === "system",
    refetchInterval: 2000,
  });

  const {
    data: portfolio,
  } = useQuery({
    queryKey: [
      "assistant-portfolio",
    ],
    queryFn: getPortfolio,
    enabled:
      page === "risk",
    refetchInterval: 2000,
  });

  const {
    data: portfolioRisk,
  } = useQuery({
    queryKey: [
      "assistant-portfolio-risk",
    ],
    queryFn: getPortfolioRisk,
    enabled:
      page === "risk",
    refetchInterval: 2000,
  });

  const {
    data: positions,
  } = useQuery({
    queryKey: [
      "assistant-positions",
    ],
    queryFn: getPositions,
    enabled:
      page === "risk",
    refetchInterval: 2000,
  });

  const {
    data: stpStatus,
  } = useQuery({
    queryKey: [
      "assistant-stp",
    ],
    queryFn: getStpStatus,
    enabled:
      page === "risk",
    refetchInterval: 2000,
  });

  const {
    data: circuitBreaker,
  } = useQuery({
    queryKey: [
      "assistant-circuit-breaker",
    ],
    queryFn: getCircuitBreaker,
    enabled:
      page === "risk",
    refetchInterval: 2000,
  });

  const {
    data: omsParents = [],
  } = useQuery({
    queryKey: [
      "assistant-oms-parents",
    ],
    queryFn: getParentOrders,
    enabled:
      page === "oms",
    refetchInterval: 2000,
  });

  const selectedParentId =
    page === "oms"
      ? selectedOmsParentId
      : null;

  const {
    data: omsChildren = [],
  } = useQuery({
    queryKey: [
      "assistant-oms-children",
      selectedParentId,
    ],
    queryFn: () =>
      getChildOrders(
        selectedParentId!
      ),
    enabled:
      page === "oms" &&
      selectedParentId !== null,
    refetchInterval: 2000,
  });

  const {
    data: omsFills = [],
  } = useQuery({
    queryKey: [
      "assistant-oms-fills",
      selectedParentId,
    ],
    queryFn: () =>
      getFills(
        selectedParentId!
      ),
    enabled:
      page === "oms" &&
      selectedParentId !== null,
    refetchInterval: 2000,
  });

  const {
    data: executionQuality,
  } = useQuery({
    queryKey: [
      "assistant-execution-quality",
      selectedParentId,
    ],
    queryFn: () =>
      getExecutionQuality(
        selectedParentId!
      ),
    enabled:
      page === "oms" &&
      selectedParentId !== null,
    refetchInterval: 2000,
  });

  const {
    data: reconciliation,
  } = useQuery({
    queryKey: [
      "assistant-parent-reconciliation",
      selectedParentId,
    ],
    queryFn: () =>
      getParentReconciliation(
        selectedParentId!
      ),
    enabled:
      page === "oms" &&
      selectedParentId !== null,
    refetchInterval: 2000,
  });
  const [open, setOpen] =
    useState(false);

  const [input, setInput] =
    useState("");

  const [loading, setLoading] =
    useState(false);

  const [messages, setMessages] =
    useState<AssistantMessage[]>([
      {
        role: "assistant",
        content:
          "Ask me about MiniMatch, the current page, or any trading-system concept shown here.",
      },
    ]);

  async function handleSubmit(
    event: React.FormEvent
  ) {
    event.preventDefault();

    const question =
      input.trim();

    if (
      !question ||
      loading
    ) {
      return;
    }

    const history =
      messages.slice(-8);

    setMessages(
      (current) => [
        ...current,
        {
          role: "user",
          content: question,
        },
      ]
    );

    setInput("");
    setLoading(true);

    try {
      const response =
        await askAssistant({
          page,
          question,
          history,

          context: {
            marketStreamStatus,
            marketSnapshot:
              snapshot,

            ...(page === "system"
              ? {
                  systemStats,
                }
              : {}),

            ...(page === "risk"
              ? {
                  portfolio,
                  portfolioRisk,
                  positions,
                  stpStatus,
                  circuitBreaker,
                }
              : {}),

            ...(page === "oms"
              ? {
                  omsParents,
                  selectedParentId,
                  omsChildren,
                  omsFills,
                  executionQuality,
                  reconciliation,
                }
              : {}),
          },
        });

      setMessages(
        (current) => [
          ...current,
          {
            role: "assistant",
            content:
              response.answer,
          },
        ]
      );
    } catch {
      setMessages(
        (current) => [
          ...current,
          {
            role: "assistant",
            content:
              "I couldn't reach the MiniMatch assistant service.",
          },
        ]
      );
    } finally {
      setLoading(false);
    }
  }

  if (!open) {
    return (
      <button
        type="button"
        className="product-assistant-launcher"
        onClick={() =>
          setOpen(true)
        }
        aria-label="Open MiniMatch assistant"
      >
        <Bot size={17} />

        <span>ASK MM</span>
      </button>
    );
  }

  return (
    <aside className="product-assistant">
      <div className="product-assistant__header">
        <div>
          <span className="eyebrow">
            PRODUCT COPILOT
          </span>

          <strong>
            MiniMatch Assistant
          </strong>
        </div>

        <button
          type="button"
          onClick={() =>
            setOpen(false)
          }
          aria-label="Close assistant"
        >
          <X size={15} />
        </button>
      </div>

      <div className="product-assistant__context">
        <span>
          CURRENT CONTEXT
        </span>

        <strong>
          {page.toUpperCase()}
        </strong>
      </div>

      <div className="product-assistant__messages">
        {messages.map(
          (message, index) => (
            <div
              key={`${message.role}-${index}`}
              className={
                message.role ===
                "user"
                  ? "product-assistant__message product-assistant__message--user"
                  : "product-assistant__message product-assistant__message--assistant"
              }
            >
              <span>
                {message.role ===
                "user"
                  ? "YOU"
                  : "MM"}
              </span>

              <p>
                {message.content}
              </p>
            </div>
          )
        )}
      </div>

      {loading && (
        <div className="product-assistant__thinking">
          <span />
          MM IS THINKING...
        </div>
      )}

      <form
        className="product-assistant__composer"
        onSubmit={
          handleSubmit
        }
      >
        <input
          disabled={loading}
          value={input}
          onChange={(event) =>
            setInput(
              event.target.value
            )
          }
          placeholder="ASK ABOUT MINIMATCH..."
        />

        <button
          type="submit"
          disabled={loading}
          aria-label="Send question"
        >
          <Send size={14} />
        </button>
      </form>
    </aside>
  );
}
