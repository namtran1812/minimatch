import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface Point {
  type: string;
  count: number;
}

export default function FixMessageTypeChart({
  data,
}: {
  data: Point[];
}) {
  return (
    <div className="panel chart-panel fix-type-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            PROTOCOL MIX
          </span>

          <h2>Message Types</h2>
        </div>
      </div>

      <div className="chart-panel__body fix-type-chart__body">
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
              dataKey="type"
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
              allowDecimals={false}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={36}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
            />

            <Bar
              dataKey="count"
              fill="#5ee6b6"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
