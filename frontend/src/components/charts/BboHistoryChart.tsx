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

import type {
  BboHistoryPoint,
} from "../../hooks/useMarketSocket";

interface Props {
  data: BboHistoryPoint[];
}

export default function BboHistoryChart({
  data,
}: Props) {
  return (
    <div className="panel chart-panel bbo-history-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            CONSOLIDATED BBO
          </span>

          <h2>Bid / Mid / Ask History</h2>
        </div>

        <span>
          LAST {data.length} UPDATES
        </span>
      </div>

      <div className="chart-panel__body bbo-history-chart__body">
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
              dataKey="sequence"
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
              domain={["dataMin", "dataMax"]}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={68}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              labelStyle={{
                color: "#8b989a",
              }}
            />

            <Legend
              iconType="line"
              wrapperStyle={{
                fontSize: 9,
              }}
            />

            <Line
              type="monotone"
              dataKey="ask"
              name="ASK"
              stroke="#ff7188"
              strokeWidth={1}
              dot={false}
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="midpoint"
              name="MID"
              stroke="#d8dfdf"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="bid"
              name="BID"
              stroke="#5ee6b6"
              strokeWidth={1}
              dot={false}
              isAnimationActive={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
