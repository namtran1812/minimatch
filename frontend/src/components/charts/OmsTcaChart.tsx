import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export default function OmsTcaChart({
  implementationShortfallBps,
  feeAdjustedShortfallBps,
}: {
  implementationShortfallBps: number;
  feeAdjustedShortfallBps: number;
}) {
  const data = [
    {
      metric: "IMPL SHORTFALL",
      bps: implementationShortfallBps,
    },
    {
      metric: "FEE-ADJ SHORTFALL",
      bps: feeAdjustedShortfallBps,
    },
  ];

  return (
    <div className="panel chart-panel oms-tca-chart">
      <div className="panel-title">
        <div>
          <span className="eyebrow">
            EXECUTION QUALITY
          </span>

          <h2>Transaction Cost Analysis</h2>
        </div>

        <span>BPS</span>
      </div>

      <div className="chart-panel__body oms-tca-chart__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <BarChart
            data={data}
            layout="vertical"
            margin={{
              top: 8,
              right: 20,
              bottom: 4,
              left: 8,
            }}
          >
            <CartesianGrid
              stroke="#182124"
              horizontal={false}
            />

            <XAxis
              type="number"
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
              type="category"
              dataKey="metric"
              tick={{
                fill: "#8b989a",
                fontSize: 9,
              }}
              tickLine={false}
              axisLine={false}
              width={120}
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
                `${Number(value).toFixed(2)} bps`,
                "SHORTFALL",
              ]}
            />

            <Bar
              dataKey="bps"
              fill="#5ee6b6"
              radius={[0, 2, 2, 0]}
              isAnimationActive={false}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
