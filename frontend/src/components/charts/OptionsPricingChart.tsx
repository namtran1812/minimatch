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
  blackScholes: number;
  monteCarlo: number;
}

export default function OptionsPricingChart({
  blackScholes,
  monteCarlo,
}: Props) {
  const data = [
    {
      model: "BLACK-SCHOLES",
      price: blackScholes,
    },
    {
      model: "MONTE CARLO",
      price: monteCarlo,
    },
  ];

  return (
    <div className="panel chart-panel analytics-options-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            MODEL VALIDATION
          </span>

          <h2>Options Price Comparison</h2>
        </div>

        <span>CALL PRICE</span>
      </div>

      <div className="chart-panel__body analytics-options-chart__body">
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
              dataKey="model"
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
              width={65}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border:
                  "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              formatter={(value) => [
                Number(value).toFixed(4),
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
