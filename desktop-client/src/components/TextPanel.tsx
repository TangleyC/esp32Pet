import { SendHorizontal } from "lucide-react";

type TextPanelProps = {
  value: string;
  busy: boolean;
  onChange: (value: string) => void;
  onSend: () => void;
};

export function TextPanel({ value, busy, onChange, onSend }: TextPanelProps) {
  return (
    <section className="panel">
      <div className="section-title">
        <SendHorizontal size={18} />
        <h2>文本发送</h2>
      </div>

      <textarea
        value={value}
        onChange={(event) => onChange(event.target.value)}
        placeholder="主人，你回来啦"
        rows={4}
      />

      <button type="button" onClick={onSend} disabled={busy || !value.trim()}>
        <SendHorizontal size={17} />
        发送文字
      </button>
    </section>
  );
}
