import os
import json
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Literal

from fastapi import FastAPI
from dotenv import load_dotenv
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel


load_dotenv(dotenv_path=Path(__file__).resolve().parents[2] / ".env")


class Message(BaseModel):
    role: Literal["user", "assistant"]
    content: str


class AssistantRequest(BaseModel):
    page: str
    question: str
    history: list[Message] = []
    context: dict[str, Any] | None = None


class AssistantResponse(BaseModel):
    answer: str


app = FastAPI(
    title="MiniMatch Product Assistant"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173",
        "http://127.0.0.1:5173",
        "http://localhost:8080",
        "http://127.0.0.1:8080",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

PRODUCT_CONTEXT = """
MiniMatch is an electronic trading systems lab.

Core product areas:

Overview
- Cross-system control-plane dashboard.
- Shows engine health, market-data health, latency,
  venue connectivity, portfolio risk and operations state.

Markets
- Live multi-venue market data.
- Consolidated BBO and depth.
- Venue synchronization, quote age, sequence gaps,
  reconnect state and message throughput.

Trading
- Direct order-entry interface.
- Supports limit, market, IOC, FOK and Post-Only orders.
- Supports cancel and replace workflows.

Execution
- Smart-order-routing and execution inspection.

OMS
- Parent and child order lifecycle management.
- Supports MARKET, TWAP, VWAP, POV and ICEBERG execution.
- Includes fills, execution quality and reconciliation.

Risk
- Client and portfolio risk.
- Positions and mark-to-market.
- STP, circuit breakers, symbol controls,
  price bands, portfolio limits and global halt.

FIX
- FIX 4.4 session state and message inspection.

Replay
- Deterministic historical event replay.
- Play, pause, seek, restart and speed controls.

Backtest
- Historical execution simulation.
- Includes arrival price, VWAP, implementation shortfall,
  execution fees and child/fill inspection.

Analytics
- Portfolio analytics, pairs analytics and options pricing.

System
- Matching engine lifecycle counters.
- P50/P95/P99 order-submit latency.
- Active orders and deterministic state hash.

Operations
- Market infrastructure and operational health.
- Prometheus/Grafana observability.
- Reconciliation, recovery and integrity monitoring.

Engineering Journal
- Reflections on the design and build process.

Important product principles:
- Determinism before optimization.
- Market-data freshness and synchronization are part
  of correctness.
- Orders are stateful lifecycles.
- Risk controls are architectural boundaries.
- Replay is used as a debugging tool.
- Post-trade reconciliation provides independent
  evidence of execution correctness.
"""


@app.get("/health")
def health():
    return {
        "ok": True,
        "service": "minimatch-assistant",
    }


@app.post(
    "/api/assistant",
    response_model=AssistantResponse,
)
def assistant(
    request: AssistantRequest,
):
    history_text = "\n".join(
        f"{message.role.upper()}: "
        f"{message.content}"
        for message in request.history[-8:]
    )

    live_context = json.dumps(
        request.context or {},
        indent=2,
        default=str,
    )

    prompt = f"""
You are MiniMatch Assistant, the product copilot for
MiniMatch.

Your job is to help a user understand the product,
trading-system terminology, the current page, and how
MiniMatch components interact.

Be technically precise but concise.

Never claim a live MiniMatch value unless it is explicitly
provided in the prompt.

If a question requires live state that you do not have,
tell the user which page or metric they should inspect.

Current page:
{request.page}

MiniMatch product context:
{PRODUCT_CONTEXT}

Live MiniMatch context:
{live_context}

Treat the live context above as the source of truth for
current market state. Do not invent values that are absent.

Recent conversation:
{history_text or "None"}

User question:
{request.question}
"""

    ollama_url = os.environ.get(
        "OLLAMA_URL",
        "http://127.0.0.1:11434/api/generate",
    )

    ollama_model = os.environ.get(
        "OLLAMA_MODEL",
        "qwen3:4b",
    )

    payload = json.dumps(
        {
            "model": ollama_model,
            "prompt": prompt,
            "stream": False,
        }
    ).encode("utf-8")

    ollama_request = urllib.request.Request(
        ollama_url,
        data=payload,
        headers={
            "Content-Type": "application/json",
        },
        method="POST",
    )

    try:
        with urllib.request.urlopen(
            ollama_request,
            timeout=120,
        ) as response:
            result = json.loads(
                response.read().decode("utf-8")
            )
    except urllib.error.URLError as exc:
        raise RuntimeError(
            "Unable to reach local Ollama service"
        ) from exc

    answer = result.get(
        "response",
        "",
    ).strip()

    if not answer:
        answer = (
            "The local model returned an empty response."
        )

    return AssistantResponse(
        answer=answer
    )
