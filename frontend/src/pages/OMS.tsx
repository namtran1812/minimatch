import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import {
  getParentOrders,
  getChildOrders,
  getFills,
} from "../api/oms";

export default function OMS() {
  const [selectedParent, setSelectedParent] = useState<string | null>(null);

  const { data: parents = [] } = useQuery({
    queryKey: ["oms-parents"],
    queryFn: getParentOrders,
    refetchInterval: 2000,
  });

  const { data: children = [] } = useQuery({
    queryKey: ["oms-children", selectedParent],
    queryFn: () => getChildOrders(selectedParent!),
    enabled: !!selectedParent,
  });

  const { data: fills = [] } = useQuery({
    queryKey: ["oms-fills", selectedParent],
    queryFn: () => getFills(selectedParent!),
    enabled: !!selectedParent,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">ORDER MANAGEMENT SYSTEM</span>
        <h1>OMS</h1>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Parent Orders</h2>
        </div>

        {parents.length === 0 && (
          <div className="feedback">No OMS parent orders available.</div>
        )}

        {parents.map((order) => (
          <button
            key={order.id}
            className="oms-row"
            onClick={() => setSelectedParent(order.id)}
          >
            <span>{order.id}</span>
            <span>{order.symbol}</span>
            <span>{order.status}</span>
            <span>
              {order.filledQuantity}/{order.quantity}
            </span>
            <span>{order.strategy}</span>
          </button>
        ))}
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Child Orders</h2>
          </div>

          {children.length === 0 && (
            <div className="feedback">Select a parent order.</div>
          )}

          {children.map((child) => (
            <div className="risk-item" key={child.id}>
              <span>{child.venue}</span>
              <strong>
                {child.status} · {child.filledQuantity}/{child.quantity}
              </strong>
            </div>
          ))}
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Fills</h2>
          </div>

          {fills.length === 0 && (
            <div className="feedback">No fills for selected parent.</div>
          )}

          {fills.map((fill) => (
            <div className="risk-item" key={fill.id}>
              <span>{fill.venue}</span>
              <strong>
                {fill.quantity} @ {fill.price}
              </strong>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
