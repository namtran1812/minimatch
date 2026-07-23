import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export interface ReplayProgressPoint {
  sample: number;
  recordIndex: number;
  progress: number;
}

export default function ReplayProgressChart({
  data,
}: {
  data: ReplayProgressPoint[];
}) {
  return (
    <div className="panel chart-panel replay-progress-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            REPLAY ENGINE
          </span>

          <h2>Replay Progress</h2>
        </div>

        <span>
          LAST {data.length} UPDATES
        </span>
      </div>

      <div className="chart-panel__body replay-progress-chart__body">
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
              dataKey="sample"
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
              domain={[0, 100]}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={42}
              tickFormatter={(value) =>
                `${value}%`
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
              formatter={(
                value,
                name
              ) => {
                if (
                  name === "PROGRESS"
                ) {
                  return [
                    `${Number(
                      value
                    ).toFixed(2)}%`,
                    "PROGRESS",
                  ];
                }

                return [
                  value,
                  name,
                ];
              }}
            />

            <Line
              type="monotone"
              dataKey="progress"
              name="PROGRESS"
              stroke="#5ee6b6"
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
