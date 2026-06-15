import { MonitorDot, Power } from "lucide-react";
import type { ActiveWindowInfo } from "../services/activeWindow";
import type { PetState } from "../services/api";

type MonitorPanelProps = {
  enabled: boolean;
  available: boolean;
  windowInfo: ActiveWindowInfo | null;
  resolvedState: PetState | null;
  statusText: string;
  onToggle: (enabled: boolean) => void;
};

export function MonitorPanel({
  enabled,
  available,
  windowInfo,
  resolvedState,
  statusText,
  onToggle,
}: MonitorPanelProps) {
  return (
    <section className="panel">
      <div className="section-title">
        <MonitorDot size={18} />
        <h2>活动窗口联动</h2>
        <span className={enabled && available ? "badge connected" : "badge"}>{enabled && available ? "运行中" : "未运行"}</span>
      </div>

      <div className="monitor-row">
        <div>
          <strong>{windowInfo?.process ?? "等待窗口信息"}</strong>
          <span>{windowInfo?.title || "打开 Tauri 桌面客户端后可自动识别活动窗口"}</span>
        </div>
        <button type="button" className={enabled ? "secondary" : undefined} onClick={() => onToggle(!enabled)} disabled={!available}>
          <Power size={17} />
          {enabled ? "关闭联动" : "开启联动"}
        </button>
      </div>

      <p className="status-line">
        {resolvedState ? `当前映射：${resolvedState}。${statusText}` : statusText}
      </p>
    </section>
  );
}
