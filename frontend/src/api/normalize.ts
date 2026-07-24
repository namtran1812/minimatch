export function asArray<T>(
  value: unknown
): T[] {
  if (Array.isArray(value)) {
    return value as T[];
  }

  if (
    value &&
    typeof value === "object"
  ) {
    const object =
      value as Record<string, unknown>;

    for (const key of [
      "items",
      "data",
      "results",
      "executions",
      "parents",
      "children",
      "fills",
      "positions",
      "messages",
    ]) {
      if (Array.isArray(object[key])) {
        return object[key] as T[];
      }
    }
  }

  return [];
}

export function asObject(
  value: unknown
): Record<string, unknown> {
  if (
    value &&
    typeof value === "object" &&
    !Array.isArray(value)
  ) {
    return value as Record<string, unknown>;
  }

  return {};
}

export function asNumber(
  value: unknown,
  fallback = 0
): number {
  const result = Number(value);

  return Number.isFinite(result)
    ? result
    : fallback;
}

export function asString(
  value: unknown,
  fallback = ""
): string {
  return typeof value === "string"
    ? value
    : fallback;
}

export function asBoolean(
  value: unknown,
  fallback = false
): boolean {
  return typeof value === "boolean"
    ? value
    : fallback;
}
