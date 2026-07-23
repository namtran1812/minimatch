import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface Point {
  bucket: string;
  inbound: number;
  outbound: number;
}

export default function FixMessageActivityChart({
  data,
}: {
  data: Point[];
}) {
  return (
    <div className="panel chart-panel fix-activity-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            FIX ACTIVITY
          </span>

          <h2>Message Rate</h2>
        </div>

        <span>
          LAST {data.length} BUCKETS
        </span>
      </div>

      <div className="chart-panel__body fix-activity-chart__body">
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
              dataKey="bucket"
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={{
                stroke: "#202b2e",
              }}
              minTickGap={28}
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

            <Line
              type="monotone"
              dataKey="inbound"
              name="IN"
              stroke="#5ee6b6"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="outbound"
              name="OUT"
              stroke="#8fa7ff"
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
