import { Activity } from "lucide-react";
import type { PetState } from "../services/api";

const states: Array<{ value: PetState; label: string; preview: string }> = [
  { value: "online", label: "online", preview: "在线" },
  { value: "thinking", label: "thinking", preview: "思考中" },
  { value: "working", label: "working", preview: "工作中" },
  { value: "message", label: "message", preview: "收到消息" },
  { value: "fishing", label: "fishing", preview: "摸鱼中" },
  { value: "sleep", label: "sleep", preview: "休眠中" },
  { value: "happy", label: "happy", preview: "开心" },
  { value: "cool", label: "cool", preview: "酷" },
  { value: "confused", label: "confused", preview: "疑惑" },
  { value: "offline", label: "offline", preview: "离线" },
];

type StatePanelProps = {
  selected: PetState;
  busy: boolean;
  onSelect: (state: PetState) => void;
  onApply: () => void;
};

export function StatePanel({ selected, busy, onSelect, onApply }: StatePanelProps) {
  return (
    <section className="panel">
      <div className="section-title">
        <Activity size={18} />
        <h2>状态控制</h2>
      </div>

      <div className="state-grid">
        {states.map((state) => (
          <button
            type="button"
            key={state.value}
            className={selected === state.value ? "state-option selected" : "state-option"}
            onClick={() => onSelect(state.value)}
          >
            <strong>{state.label}</strong>
            <span>{state.preview}</span>
          </button>
        ))}
      </div>

      <button type="button" onClick={onApply} disabled={busy}>
        <Activity size={17} />
        应用状态
      </button>
    </section>
  );
}
