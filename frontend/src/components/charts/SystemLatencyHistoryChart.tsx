import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export interface SystemLatencyPoint {
  sample: number;
  label: string;
  p50: number;
  p95: number;
  p99: number;
}

export default function SystemLatencyHistoryChart({
  data,
}: {
  data: SystemLatencyPoint[];
}) {
  return (
    <div className="panel chart-panel system-latency-history-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            LATENCY HISTORY
          </span>

          <h2>
            Latency Over Time
          </h2>
        </div>

        <span>
          LAST {data.length} SAMPLES
        </span>
      </div>

      <div className="chart-panel__body system-latency-history-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <LineChart
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
              minTickGap={24}
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

            <Legend
              iconType="line"
              wrapperStyle={{
                fontSize: 9,
              }}
            />

            <Line
              type="monotone"
              dataKey="p50"
              name="P50"
              stroke="#5ee6b6"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="p95"
              name="P95"
              stroke="#e4c66a"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="p99"
              name="P99"
              stroke="#ff7188"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
