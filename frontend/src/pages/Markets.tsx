import { useQuery } from "@tanstack/react-query";
import { getOrderBook } from "../api/market";

export default function Markets() {
  const symbol = "BTC-USD";

  const { data: book } = useQuery({
    queryKey: ["book", symbol],
    queryFn: () => getOrderBook(symbol),
    refetchInterval: 1000,
  });

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">MARKET DATA</span>
        <h1>{symbol}</h1>
      </div>

      <div className="terminal-grid">
        <div className="panel">
          <div className="panel-title">
            <h2>Bids</h2>
          </div>

          {book?.bids?.map((level) => (
            <div className="book-row" key={level.price}>
              <span className="bid">{level.price}</span>
              <span>{level.quantity}</span>
              <span>{level.orderCount ?? "—"}</span>
            </div>
          ))}
        </div>

        <div className="panel">
          <div className="panel-title">
            <h2>Asks</h2>
          </div>

          {book?.asks?.map((level) => (
            <div className="book-row" key={level.price}>
              <span className="ask">{level.price}</span>
              <span>{level.quantity}</span>
              <span>{level.orderCount ?? "—"}</span>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
