import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface DepthPoint {
  price: number;
  bid: number;
  ask: number;
}

interface Props {
  data: DepthPoint[];
}

export default function MarketDepthChart({
  data,
}: Props) {
  return (
    <div className="panel chart-panel">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            CONSOLIDATED LIQUIDITY
          </span>

          <h2>Market Depth</h2>
        </div>

        <span>
          {data.length} LEVELS
        </span>
      </div>

      <div className="chart-panel__body chart-panel__body--depth">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <BarChart data={data}>
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey="price"
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
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border:
                  "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
            />

            <Bar
              dataKey="bid"
              fill="#5ee6b6"
              isAnimationActive={false}
            />

            <Bar
              dataKey="ask"
              fill="#ff7188"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
