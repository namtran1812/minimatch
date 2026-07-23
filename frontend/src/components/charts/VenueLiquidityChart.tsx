import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface VenueLiquidityPoint {
  venue: string;
  bidLiquidity: number;
  askLiquidity: number;
}

interface Props {
  data: VenueLiquidityPoint[];
}

export default function VenueLiquidityChart({
  data,
}: Props) {
  return (
    <div className="panel chart-panel">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            VENUE COMPARISON
          </span>
          <h2>Visible Liquidity</h2>
        </div>
      </div>

      <div className="chart-panel__body">
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
              dataKey="venue"
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
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
            />

            <Bar
              dataKey="bidLiquidity"
              fill="#5ee6b6"
              isAnimationActive={false}
            />

            <Bar
              dataKey="askLiquidity"
              fill="#ff7188"
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
