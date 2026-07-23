/* eslint-disable react-refresh/only-export-components */
import {
  createContext,
  useContext,
  useMemo,
  useState,
  type ReactNode,
} from "react";

interface ProductContextValue {
  selectedOmsParentId:
    | string
    | null;

  setSelectedOmsParentId: (
    parentId:
      | string
      | null
  ) => void;
}

const ProductContext =
  createContext<
    ProductContextValue
    | undefined
  >(undefined);

export function ProductContextProvider({
  children,
}: {
  children: ReactNode;
}) {
  const [
    selectedOmsParentId,
    setSelectedOmsParentId,
  ] = useState<
    string | null
  >(null);

  const value =
    useMemo(
      () => ({
        selectedOmsParentId,
        setSelectedOmsParentId,
      }),
      [
        selectedOmsParentId,
      ]
    );

  return (
    <ProductContext.Provider
      value={value}
    >
      {children}
    </ProductContext.Provider>
  );
}

export function useProductContext() {
  const context =
    useContext(
      ProductContext
    );

  if (!context) {
    throw new Error(
      "useProductContext must be used inside ProductContextProvider"
    );
  }

  return context;
}
