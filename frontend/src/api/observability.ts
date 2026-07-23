export interface ServiceHealth {
  ok: boolean;
  status: string;
}

async function probe(
  url: string
): Promise<ServiceHealth> {
  try {
    const response =
      await fetch(
        url,
        {
          method: "GET",
        }
      );

    return {
      ok: response.ok,
      status: response.ok
        ? "HEALTHY"
        : `HTTP ${response.status}`,
    };
  } catch {
    return {
      ok: false,
      status: "OFFLINE",
    };
  }
}

export function getPrometheusHealth() {
  return probe(
    "http://127.0.0.1:9090/-/ready"
  );
}

export function getGrafanaHealth() {
  return probe(
    "http://127.0.0.1:3000/api/health"
  );
}
