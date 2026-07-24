interface Env {
  AI: Ai;
}

interface AssistantMessage {
  role: "user" | "assistant";
  content: string;
}

interface AssistantRequest {
  page?: string;
  question?: string;
  history?: AssistantMessage[];
  context?: unknown;
}

const ALLOWED_ORIGINS = new Set([
  "https://minimatch-six.vercel.app",
]);

function makeHeaders(origin: string): HeadersInit {
  const allowedOrigin =
    ALLOWED_ORIGINS.has(origin)
      ? origin
      : "https://minimatch-six.vercel.app";

  return {
    "Access-Control-Allow-Origin": allowedOrigin,
    "Access-Control-Allow-Methods": "POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
    "Content-Type": "application/json",
  };
}

export default {
  async fetch(
    request: Request,
    env: Env
  ): Promise<Response> {
    const origin =
      request.headers.get("Origin") ?? "";

    const headers =
      makeHeaders(origin);

    if (request.method === "OPTIONS") {
      return new Response(null, {
        status: 204,
        headers,
      });
    }

    const url = new URL(request.url);

    if (
      request.method !== "POST" ||
      url.pathname !== "/api/assistant"
    ) {
      return new Response(
        JSON.stringify({
          error: "Not found",
        }),
        {
          status: 404,
          headers,
        }
      );
    }

    let body: AssistantRequest;

    try {
      body =
        await request.json<AssistantRequest>();
    } catch {
      return new Response(
        JSON.stringify({
          error: "Invalid JSON",
        }),
        {
          status: 400,
          headers,
        }
      );
    }

    const question =
      body.question?.trim();

    if (!question) {
      return new Response(
        JSON.stringify({
          error: "Question is required",
        }),
        {
          status: 400,
          headers,
        }
      );
    }

    const history =
      Array.isArray(body.history)
        ? body.history.slice(-6)
        : [];

    const systemPrompt = `
You are Ask MM, the product copilot for MiniMatch.

MiniMatch is an educational electronic trading systems lab.

It includes:
- market data and order books
- trading and order entry
- smart order routing
- OMS parent and child orders
- execution analytics
- risk controls
- FIX monitoring
- deterministic replay
- backtesting
- quantitative analytics
- system and operations telemetry
- a technical build journal

Answer questions about MiniMatch accurately and concisely.

Use the supplied page and runtime context when relevant.

Never invent live values that are missing.

If runtime data is unavailable, explicitly say that it is unavailable.

Current page:
${body.page ?? "unknown"}

Runtime context:
${JSON.stringify(
  body.context ?? {},
  null,
  2
)}
`.trim();

    try {
      const result =
        await env.AI.run(
          "@cf/meta/llama-3.2-3b-instruct",
          {
            messages: [
              {
                role: "system",
                content: systemPrompt,
              },

              ...history,

              {
                role: "user",
                content: question,
              },
            ],

            max_tokens: 450,
          }
        );

      const answer =
        typeof result === "object" &&
        result !== null &&
        "response" in result
          ? String(result.response).trim()
          : "";

      return new Response(
        JSON.stringify({
          answer:
            answer ||
            "I could not generate a response.",
        }),
        {
          status: 200,
          headers,
        }
      );
    } catch (error) {
      console.error(
        "Workers AI error",
        error
      );

      return new Response(
        JSON.stringify({
          answer:
            "Ask MM is temporarily unavailable.",
        }),
        {
          status: 503,
          headers,
        }
      );
    }
  },
} satisfies ExportedHandler<Env>;
