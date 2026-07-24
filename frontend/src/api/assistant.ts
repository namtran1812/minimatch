import axios from "axios";

export interface AssistantMessage {
  role: "user" | "assistant";
  content: string;
}

export interface AssistantRequest {
  page: string;
  question: string;
  history: AssistantMessage[];
  context?: Record<string, unknown>;
}

export interface AssistantResponse {
  answer: string;
}

const assistantBaseUrl =
  import.meta.env
    .VITE_ASSISTANT_API_URL ??
  (
    import.meta.env.DEV
      ? "http://127.0.0.1:8090/api"
      : undefined
  );

const assistantApi =
  axios.create({
    baseURL: assistantBaseUrl,
  });

export async function askAssistant(
  request: AssistantRequest
): Promise<AssistantResponse> {
  if (!assistantBaseUrl) {
    throw new Error(
      "Assistant service is not configured."
    );
  }

  return (
    await assistantApi.post(
      "/assistant",
      request
    )
  ).data;
}
