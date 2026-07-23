import axios from "axios";

export interface AssistantMessage {
  role: "user" | "assistant";
  content: string;
}

export interface AssistantRequest {
  page: string;
  question: string;
  history: AssistantMessage[];

  context?: {
    marketStreamStatus: string;
    marketSnapshot: unknown;
    systemStats?: unknown;
    portfolio?: unknown;
    portfolioRisk?: unknown;
    positions?: unknown;
    stpStatus?: unknown;
    circuitBreaker?: unknown;

    omsParents?: unknown;
    selectedParentId?: string | null;
    omsChildren?: unknown;
    omsFills?: unknown;
    executionQuality?: unknown;
    reconciliation?: unknown;
  };
}

export interface AssistantResponse {
  answer: string;
}

const assistantApi =
  axios.create({
    baseURL:
      import.meta.env
        .VITE_ASSISTANT_API_URL ??
      "http://127.0.0.1:8090/api",
  });

export async function askAssistant(
  request: AssistantRequest
): Promise<AssistantResponse> {
  return (
    await assistantApi.post(
      "/assistant",
      request
    )
  ).data;
}
