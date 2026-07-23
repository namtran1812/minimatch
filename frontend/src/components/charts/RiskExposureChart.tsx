import {
  Bar,
  BarChart,
  CartesianGrid,
  Cell,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface Props {
  grossExposure: number;
  netExposure: number;
  maxGrossExposure: number;
  maxNetExposure: number;
}

export default function RiskExposureChart({
  grossExposure,
  netExposure,
  maxGrossExposure,
  maxNetExposure,
}: Props) {
  const data = [
    {
      metric: "GROSS",
      current: Math.abs(grossExposure),
      limit: maxGrossExposure,
      breached:
        maxGrossExposure > 0 &&
        Math.abs(grossExposure) >
          maxGrossExposure,
    },
    {
      metric: "NET",
      current: Math.abs(netExposure),
      limit: maxNetExposure,
      breached:
        maxNetExposure > 0 &&
        Math.abs(netExposure) >
          maxNetExposure,
    },
  ];

  return (
    <div className="panel chart-panel risk-exposure-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            PORTFOLIO RISK
          </span>

          <h2>Exposure vs Limits</h2>
        </div>

        <span>REAL TIME</span>
      </div>

      <div className="chart-panel__body risk-exposure-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <BarChart
            data={data}
            margin={{
              top: 10,
              right: 18,
              bottom: 4,
              left: 8,
            }}
          >
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey="metric"
              tick={{
                fill: "#8b989a",
                fontSize: 10,
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
              width={72}
            />

            <Tooltip
              contentStyle={{
                background: "#0a0f11",
                border: "1px solid #293437",
                borderRadius: 2,
                fontSize: 10,
              }}
              formatter={(value) =>
                `$${Number(value).toLocaleString()}`
              }
            />

            <Bar
              dataKey="limit"
              name="LIMIT"
              fill="#343e41"
              isAnimationActive={false}
            />

            <Bar
              dataKey="current"
              name="CURRENT"
              isAnimationActive={false}
            >
              {data.map((entry) => (
                <Cell
                  key={entry.metric}
                  fill={
                    entry.breached
                      ? "#ff7188"
                      : "#5ee6b6"
                  }
                />
              ))}
            </Bar>
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
