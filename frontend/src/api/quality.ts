import { api } from "./client";

export interface ExecutionQuality {
  parentOrderId: string;
  symbol: string;
  requestedQuantity: number;
  filledQuantity: number;
  arrivalPrice: number;
  averageFillPrice: number;
  totalNotional: number;
  totalFees: number;
  implementationShortfallBps: number;
  feeAdjustedShortfallBps: number;
}

export interface ParentReconciliation {
  parentOrderId: string;
  symbol: string;
  reconciliationStatus: string;
  reason: string;

  parentFilledQuantity: number;
  omsFillQuantity: number;
  childFilledQuantity: number;

  omsFillNotional: number;
  totalFees: number;

  analyticsFilledQuantity: number;
  analyticsTotalNotional: number;
  analyticsTotalFees: number;

  fillCount: number;
  dropCopyFillCount: number;
  childCount: number;

  quantityDifference: number;

  quantityConsistent: boolean;
  childQuantityConsistent: boolean;
  analyticsQuantityConsistent: boolean;
  analyticsNotionalConsistent: boolean;
  analyticsFeesConsistent: boolean;
  dropCopyConsistent: boolean;

  consistent: boolean;
}

export async function getExecutionQuality(
  parentId: string
): Promise<ExecutionQuality> {
  return (
    await api.get(
      `/execution-quality/${parentId}`
    )
  ).data;
}

export async function getParentReconciliation(
  parentId: string
): Promise<ParentReconciliation> {
  return (
    await api.get(
      `/reconciliation/parents/${parentId}`
    )
  ).data;
}
