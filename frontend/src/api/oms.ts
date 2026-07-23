import { api } from "./client";

export interface ParentOrder {
  id: string;
  symbol: string;
  side: string;
  status: string;
  quantity: number;
  filledQuantity: number;
  strategy: string;
}

export interface ChildOrder {
  id: string;
  parentId: string;
  venue: string;
  status: string;
  price: number;
  quantity: number;
  filledQuantity: number;
  remainingQuantity: number;
}

export interface Fill {
  id: string;
  childOrderId: string;
  parentId: string;
  price: number;
  quantity: number;
  notional: number;
  fee: number;
  venue: string;
  timestamp: number;
}

export async function getParentOrders(): Promise<ParentOrder[]> {
  return (await api.get("/oms/parents")).data;
}

export async function getChildOrders(parentId: string): Promise<ChildOrder[]> {
  return (await api.get(`/oms/parents/${parentId}/children`)).data;
}

export async function getFills(parentId: string): Promise<Fill[]> {
  return (await api.get(`/oms/parents/${parentId}/fills`)).data;
}

export async function cancelChildOrder(
  childId: string
): Promise<{ ok: boolean }> {
  return (
    await api.post(
      "/oms/children/cancel",
      {
        childId,
      }
    )
  ).data;
}
