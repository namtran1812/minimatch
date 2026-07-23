import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

interface Props<T extends Record<string, string | number>> {
  title: string;
  data: T[];
  xKey: keyof T;
  yKey: keyof T;
  unit?: string;
}

export default function TerminalLineChart<
  T extends Record<string, string | number>
>({
  title,
  data,
  xKey,
  yKey,
  unit = "",
}: Props<T>) {
  return (
    <div className="panel chart-panel">
      <div className="panel-title">
        <h2>{title}</h2>
      </div>

      <div className="chart-panel__body">
        <ResponsiveContainer
          width="100%"
          height="100%"
        >
          <LineChart data={data}>
            <CartesianGrid
              stroke="#182124"
              vertical={false}
            />

            <XAxis
              dataKey={String(xKey)}
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              axisLine={{
                stroke: "#202b2e",
              }}
              tickLine={false}
            />

            <YAxis
              tick={{
                fill: "#667174",
                fontSize: 9,
              }}
              axisLine={false}
              tickLine={false}
              width={54}
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

            <Line
              type="monotone"
              dataKey={String(yKey)}
              stroke="#5ee6b6"
              strokeWidth={1.5}
              dot={false}
              isAnimationActive={false}
              name={`${String(yKey)}${unit ? ` (${unit})` : ""}`}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
