import {
  Activity,
  BarChart3,
  BookOpen,
  Cpu,
  Gauge,
  History,
  Landmark,
  LineChart,
  Network,
  ShieldCheck,
  SlidersHorizontal,
} from "lucide-react";

export type Page =
  | "overview"
  | "markets"
  | "trading"
  | "execution"
  | "oms"
  | "risk"
  | "fix"
  | "replay"
  | "backtest"
  | "analytics"
  | "system";

interface Props {
  page: Page;
  onChange: (page: Page) => void;
}

const items: Array<{
  id: Page;
  label: string;
  icon: React.ReactNode;
}> = [
  { id: "overview", label: "Overview", icon: <Activity size={16} /> },
  { id: "markets", label: "Markets", icon: <BarChart3 size={16} /> },
  { id: "trading", label: "Trading", icon: <SlidersHorizontal size={16} /> },
  { id: "execution", label: "Execution", icon: <Gauge size={16} /> },
  { id: "oms", label: "OMS", icon: <BookOpen size={16} /> },
  { id: "risk", label: "Risk", icon: <ShieldCheck size={16} /> },
  { id: "fix", label: "FIX", icon: <Network size={16} /> },
  { id: "replay", label: "Replay", icon: <History size={16} /> },
  { id: "backtest", label: "Backtest", icon: <Landmark size={16} /> },
  { id: "analytics", label: "Analytics", icon: <LineChart size={16} /> },
  { id: "system", label: "System", icon: <Cpu size={16} /> },
];

export default function Sidebar({ page, onChange }: Props) {
  return (
    <aside className="sidebar">
      <div className="sidebar-brand">
        <div className="sidebar-logo">MM</div>
        <div>
          <strong>MiniMatch</strong>
          <span>Trading Systems Lab</span>
        </div>
      </div>

      <nav>
        {items.map((item) => (
          <button
            key={item.id}
            className={page === item.id ? "active" : ""}
            onClick={() => onChange(item.id)}
          >
            {item.icon}
            {item.label}
          </button>
        ))}
      </nav>
    </aside>
  );
}
