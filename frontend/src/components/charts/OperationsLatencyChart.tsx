import {
  Bar,
  BarChart,
  CartesianGrid,
  Legend,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import type {
  LatencySnapshot,
} from "../../hooks/useMarketSocket";

interface Props {
  data: LatencySnapshot[];
}

function nsToMs(
  value: number
) {
  return value / 1_000_000;
}

export default function OperationsLatencyChart({
  data,
}: Props) {
  const chartData =
    data.map((item) => ({
      stage: item.name
        .replaceAll("_", " ")
        .toUpperCase(),

      p50: nsToMs(
        item.p50Ns
      ),

      p95: nsToMs(
        item.p95Ns
      ),

      p99: nsToMs(
        item.p99Ns
      ),
    }));

  return (
    <div className="panel chart-panel operations-latency-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            PIPELINE PERFORMANCE
          </span>

          <h2>
            Latency Percentiles
          </h2>
        </div>

        <span>
          {data.length} STAGES
        </span>
      </div>

      <div className="chart-panel__body operations-latency-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <BarChart
            data={chartData}
            margin={{
              top: 8,
              right: 16,
              bottom: 22,
              left: 4,
            }}
          >
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey="stage"
              tick={{
                fill: "#667174",
                fontSize: 8,
              }}
              tickLine={false}
              axisLine={{
                stroke: "#202b2e",
              }}
              interval={0}
              angle={-18}
              textAnchor="end"
              height={55}
            />

            <YAxis
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={52}
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
              iconType="square"
              wrapperStyle={{
                fontSize: 9,
              }}
            />

            <Bar
              dataKey="p50"
              name="P50"
              fill="#5ee6b6"
              isAnimationActive={false}
            />

            <Bar
              dataKey="p95"
              name="P95"
              fill="#e4c66a"
              isAnimationActive={false}
            />

            <Bar
              dataKey="p99"
              name="P99"
              fill="#ff7188"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
