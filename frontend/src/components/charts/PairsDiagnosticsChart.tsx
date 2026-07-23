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
  beta: number;
  zscore: number;
  adfTStatistic: number;
}

export default function PairsDiagnosticsChart({
  beta,
  zscore,
  adfTStatistic,
}: Props) {
  const data = [
    {
      metric: "HEDGE RATIO",
      value: beta,
    },
    {
      metric: "Z-SCORE",
      value: zscore,
    },
    {
      metric: "ADF T-STAT",
      value: adfTStatistic,
    },
  ];

  return (
    <div className="panel chart-panel analytics-pairs-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            STATISTICAL ARBITRAGE
          </span>

          <h2>Pairs Diagnostics</h2>
        </div>

        <span>MODEL OUTPUT</span>
      </div>

      <div className="chart-panel__body analytics-pairs-chart__body">
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
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={50}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              formatter={(value) => [
                Number(value).toFixed(4),
                "VALUE",
              ]}
            />

            <Bar
              dataKey="value"
              fill="#5ee6b6"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
