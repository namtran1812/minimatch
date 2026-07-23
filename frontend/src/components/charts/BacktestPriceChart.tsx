import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface Props {
  arrivalPrice: number;
  averageFillPrice: number;
  marketVwap: number;
}

export default function BacktestPriceChart({
  arrivalPrice,
  averageFillPrice,
  marketVwap,
}: Props) {
  const data = [
    {
      metric: "ARRIVAL",
      price: arrivalPrice,
    },
    {
      metric: "AVG FILL",
      price: averageFillPrice,
    },
    {
      metric: "MARKET VWAP",
      price: marketVwap,
    },
  ];

  return (
    <div className="panel chart-panel backtest-price-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            EXECUTION BENCHMARK
          </span>

          <h2>Price Comparison</h2>
        </div>

        <span>HISTORICAL</span>
      </div>

      <div className="chart-panel__body backtest-price-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <BarChart
            data={data}
            margin={{
              top: 8,
              right: 16,
              bottom: 4,
              left: 4,
            }}
          >
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey="metric"
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={{
                stroke: "#202b2e",
              }}
            />

            <YAxis
              domain={["dataMin", "dataMax"]}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={72}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              formatter={(value) => [
                Number(value).toLocaleString(
                  undefined,
                  {
                    maximumFractionDigits: 4,
                  }
                ),
                "PRICE",
              ]}
            />

            <Bar
              dataKey="price"
              fill="#5ee6b6"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
