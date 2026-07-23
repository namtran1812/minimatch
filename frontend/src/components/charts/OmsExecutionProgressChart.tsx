import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface FillPoint {
  time: number;
  cumulativeQuantity: number;
}

export default function OmsExecutionProgressChart({
  data,
  requestedQuantity,
}: {
  data: FillPoint[];
  requestedQuantity: number;
}) {
  return (
    <div className="panel chart-panel oms-progress-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            EXECUTION PROGRESS
          </span>

          <h2>Cumulative Fill Quantity</h2>
        </div>

        <span>
          {data.length} FILLS
        </span>
      </div>

      <div className="chart-panel__body oms-progress-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <LineChart
            data={data}
            margin={{
              top: 8,
              right: 18,
              bottom: 4,
              left: 4,
            }}
          >
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey="time"
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
              domain={[
                0,
                Math.max(
                  requestedQuantity,
                  ...data.map(
                    (point) =>
                      point.cumulativeQuantity
                  )
                ),
              ]}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={56}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border:
                  "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              labelFormatter={(value) =>
                `${value} ms`
              }
              formatter={(value) => [
                Number(value).toFixed(6),
                "Filled",
              ]}
            />

            <Line
              type="stepAfter"
              dataKey="cumulativeQuantity"
              name="FILLED"
              stroke="#5ee6b6"
              strokeWidth={1.5}
              dot={{
                r: 2,
                fill: "#5ee6b6",
              }}
              activeDot={{
                r: 3,
              }}
              isAnimationActive={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
