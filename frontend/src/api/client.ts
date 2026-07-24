import axios from "axios";

const apiBaseUrl =
  import.meta.env.VITE_API_BASE_URL ??
  (
    import.meta.env.DEV
      ? "http://127.0.0.1:8080/api"
      : undefined
  );

export const api = axios.create({
  baseURL: apiBaseUrl,
});
