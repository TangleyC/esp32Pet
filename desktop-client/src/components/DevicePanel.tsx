import { CheckCircle2, PlugZap, Save } from "lucide-react";

type DevicePanelProps = {
  ip: string;
  name: string;
  connected: boolean;
  busy: boolean;
  statusText: string;
  onIpChange: (value: string) => void;
  onNameChange: (value: string) => void;
  onPing: () => void;
  onSave: () => void;
};

export function DevicePanel({
  ip,
  name,
  connected,
  busy,
  statusText,
  onIpChange,
  onNameChange,
  onPing,
  onSave,
}: DevicePanelProps) {
  return (
    <section className="panel">
      <div className="section-title">
        <PlugZap size={18} />
        <h2>设备连接</h2>
        <span className={connected ? "badge connected" : "badge"}>{connected ? "已连接" : "未连接"}</span>
      </div>

      <label>
        设备名称
        <input value={name} onChange={(event) => onNameChange(event.target.value)} placeholder="DeskPet ESP32" />
      </label>

      <label>
        设备 IP
        <input value={ip} onChange={(event) => onIpChange(event.target.value)} placeholder="192.168.1.88" />
      </label>

      <div className="button-row">
        <button type="button" onClick={onPing} disabled={busy}>
          <CheckCircle2 size={17} />
          测试连接
        </button>
        <button type="button" className="secondary" onClick={onSave}>
          <Save size={17} />
          保存设备
        </button>
      </div>

      <p className="status-line">{statusText}</p>
    </section>
  );
}
