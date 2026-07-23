import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export default function SystemLatencyDistributionChart({
  p50,
  p95,
  p99,
}: {
  p50: number;
  p95: number;
  p99: number;
}) {
  const data = [
    {
      label: "P50",
      value: p50,
    },
    {
      label: "P95",
      value: p95,
    },
    {
      label: "P99",
      value: p99,
    },
  ];

  return (
    <div className="panel chart-panel system-latency-distribution-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            CURRENT DISTRIBUTION
          </span>

          <h2>
            Latency Percentiles
          </h2>
        </div>
      </div>

      <div className="chart-panel__body system-latency-distribution-chart__body">
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
              dataKey="label"
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
              width={48}
              tickFormatter={(value) =>
                `${value}ms`
              }
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
                `${Number(value).toFixed(3)} ms`,
              ]}
            />

            <Bar
              dataKey="value"
              name="LATENCY"
              fill="#5ee6b6"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
