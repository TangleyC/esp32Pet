import { Activity } from "lucide-react";
import type { PetState } from "../services/api";

const states: Array<{ value: PetState; label: string; preview: string }> = [
  { value: "idle", label: "idle", preview: "(•‿•) 待机中" },
  { value: "sleep", label: "sleep", preview: "(-_-) Zzz" },
  { value: "coding", label: "coding", preview: "(⌐■_■) 正在写代码..." },
  { value: "error", label: "error", preview: "(⊙﹏⊙) 程序出错啦" },
  { value: "success", label: "success", preview: "(≧▽≦) 任务完成" },
  { value: "happy", label: "happy", preview: "(^o^) 今天真开心" },
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
