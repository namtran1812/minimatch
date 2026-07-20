import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { submitOrder } from "../api/orders";
import {
  cancelOrder,
  replaceOrder,
  getReports,
} from "../api/trading";

export default function Trading() {
  const [orderId, setOrderId] = useState(4001);
  const [clientId, setClientId] = useState(30);
  const [symbol, setSymbol] = useState(1);

  const [side, setSide] = useState<"BUY" | "SELL">("BUY");
  const [type, setType] = useState<
    "LIMIT" | "MARKET" | "IOC" | "FOK" | "POST_ONLY"
  >("LIMIT");

  const [quantity, setQuantity] = useState(100);
  const [price, setPrice] = useState(10000);

  const [replacePrice, setReplacePrice] = useState(9999);
  const [replaceQuantity, setReplaceQuantity] = useState(80);

  const [status, setStatus] = useState("READY");

  const { data: reports = [] } = useQuery({
    queryKey: ["reports"],
    queryFn: getReports,
    refetchInterval: 1000,
  });

  async function handleSubmit(event: React.FormEvent) {
    event.preventDefault();

    try {
      setStatus("SUBMITTING...");

      const result = await submitOrder({
        orderId,
        clientId,
        symbol,
        side,
        type,
        quantity,
        price: type === "MARKET" ? 0 : price,
      } as any);

      setStatus(
        result.ok
          ? `ORDER ${result.orderId} ACCEPTED`
          : "ORDER REJECTED"
      );
    } catch {
      setStatus("ORDER REJECTED");
    }
  }

  async function handleCancel() {
    try {
      const result = await cancelOrder(
        orderId,
        clientId,
        symbol
      );

      setStatus(
        result.ok
          ? `ORDER ${orderId} CANCELLED`
          : "CANCEL REJECTED"
      );
    } catch {
      setStatus("CANCEL FAILED");
    }
  }

  async function handleReplace() {
    try {
      const result = await replaceOrder(
        orderId,
        clientId,
        replacePrice,
        replaceQuantity,
        symbol
      );

      setStatus(
        result.ok
          ? `ORDER ${orderId} REPLACED`
          : "REPLACE REJECTED"
      );
    } catch {
      setStatus("REPLACE FAILED");
    }
  }

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ORDER ENTRY + LIFECYCLE</span>
        <h1>Trading</h1>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>New Order</h2>
          </div>

          <form className="order-form" onSubmit={handleSubmit}>
            <label>
              ORDER ID
              <input
                type="number"
                value={orderId}
                onChange={(e) => setOrderId(Number(e.target.value))}
              />
            </label>

            <label>
              CLIENT ID
              <input
                type="number"
                value={clientId}
                onChange={(e) => setClientId(Number(e.target.value))}
              />
            </label>

            <label>
              SYMBOL
              <input
                type="number"
                value={symbol}
                onChange={(e) => setSymbol(Number(e.target.value))}
              />
            </label>

            <label>
              SIDE
              <select
                value={side}
                onChange={(e) =>
                  setSide(e.target.value as "BUY" | "SELL")
                }
              >
                <option>BUY</option>
                <option>SELL</option>
              </select>
            </label>

            <label>
              TYPE
              <select
                value={type}
                onChange={(e) =>
                  setType(e.target.value as typeof type)
                }
              >
                <option>LIMIT</option>
                <option>MARKET</option>
                <option>IOC</option>
                <option>FOK</option>
                <option>POST_ONLY</option>
              </select>
            </label>

            <label>
              QUANTITY
              <input
                type="number"
                value={quantity}
                onChange={(e) =>
                  setQuantity(Number(e.target.value))
                }
              />
            </label>

            <label>
              PRICE
              <input
                type="number"
                disabled={type === "MARKET"}
                value={price}
                onChange={(e) =>
                  setPrice(Number(e.target.value))
                }
              />
            </label>

            <button type="submit">
              SUBMIT ORDER
            </button>
          </form>

          <div className="feedback">{status}</div>
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Cancel / Replace</h2>
          </div>

          <label>
            NEW PRICE
            <input
              type="number"
              value={replacePrice}
              onChange={(e) =>
                setReplacePrice(Number(e.target.value))
              }
            />
          </label>

          <label>
            NEW QUANTITY
            <input
              type="number"
              value={replaceQuantity}
              onChange={(e) =>
                setReplaceQuantity(Number(e.target.value))
              }
            />
          </label>

          <button onClick={handleReplace}>
            REPLACE ORDER
          </button>

          <button onClick={handleCancel}>
            CANCEL ORDER
          </button>
        </div>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Execution Reports</h2>
        </div>

        {reports.length === 0 && (
          <div className="feedback">
            No execution reports yet.
          </div>
        )}

        {reports.map((report) => (
          <div className="tape-row" key={report.sequence}>
            <span>#{report.sequence}</span>
            <span>ORDER {report.orderId}</span>
            <span>STATUS {report.status}</span>
            <span>REM {report.remaining}</span>
            <span>SYM {report.symbol}</span>
          </div>
        ))}
      </div>
    </section>
  );
}
