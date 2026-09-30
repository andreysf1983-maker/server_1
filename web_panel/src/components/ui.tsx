import type { ReactNode } from "react";

export function PageHeader({ title, subtitle, right }: { title: string; subtitle?: string; right?: ReactNode }) {
  return (
    <div className="flex flex-wrap items-end justify-between gap-4 mb-6">
      <div>
        <h1 className="text-[26px] font-black tracking-tight text-slate-100">{title}</h1>
        {subtitle ? <p className="text-[13px] text-slate-400 mt-1 max-w-3xl leading-relaxed">{subtitle}</p> : null}
      </div>
      {right ? <div className="flex items-center gap-2">{right}</div> : null}
    </div>
  );
}

export function Panel({ title, hint, children, className = "" }: { title?: string; hint?: string; children: ReactNode; className?: string }) {
  return (
    <section className={`lf-panel p-5 ${className}`}>
      {title ? (
        <div className="flex items-baseline justify-between mb-4">
          <h2 className="text-[13px] font-bold uppercase tracking-[0.14em] text-slate-300">{title}</h2>
          {hint ? <span className="text-[11px] text-slate-500 lf-mono">{hint}</span> : null}
        </div>
      ) : null}
      {children}
    </section>
  );
}

export function Stat({ label, value, tone = "default", note }: { label: string; value: ReactNode; tone?: "default" | "amber" | "green" | "red" | "blue"; note?: string }) {
  const tones: Record<string, string> = {
    default: "text-slate-100",
    amber: "lf-amber",
    green: "text-emerald-400",
    red: "text-rose-400",
    blue: "text-sky-300",
  };
  return (
    <div className="lf-panel p-4">
      <div className="text-[11px] uppercase tracking-[0.12em] text-slate-500 font-semibold">{label}</div>
      <div className={`text-[26px] font-black mt-1 ${tones[tone]}`}>{value}</div>
      {note ? <div className="text-[11.5px] text-slate-500 mt-1">{note}</div> : null}
    </div>
  );
}

export function Badge({ children, tone = "neutral" }: { children: ReactNode; tone?: "neutral" | "ok" | "warn" | "bad" | "info" }) {
  const map: Record<string, string> = {
    neutral: "border-[#2a3648] text-slate-400",
    ok: "border-emerald-500/40 text-emerald-300 bg-emerald-500/10",
    warn: "border-amber-500/40 text-amber-300 bg-amber-500/10",
    bad: "border-rose-500/40 text-rose-300 bg-rose-500/10",
    info: "border-sky-500/40 text-sky-300 bg-sky-500/10",
  };
  return <span className={`lf-badge ${map[tone]}`}>{children}</span>;
}

export function BarChart({ data, height = 120 }: { data: Array<{ label: string; value: number }>; height?: number }) {
  const max = Math.max(1, ...data.map((d) => d.value));
  return (
    <div className="flex items-end gap-[3px]" style={{ height }}>
      {data.map((d, i) => (
        <div key={i} className="flex-1 flex flex-col items-center justify-end gap-1 group" title={`${d.label}: ${d.value}`}>
          <div
            className="w-full rounded-t-[3px] bg-amber-500/70 group-hover:bg-amber-400 transition-colors"
            style={{ height: `${Math.max(3, (d.value / max) * (height - 22))}px` }}
          />
          <span className="text-[9px] text-slate-600 lf-mono">{d.label}</span>
        </div>
      ))}
    </div>
  );
}

export function Toggle({ checked, onChange, label }: { checked: boolean; onChange: (v: boolean) => void; label?: string }) {
  return (
    <button
      type="button"
      onClick={() => onChange(!checked)}
      className="flex items-center gap-2 cursor-pointer select-none"
      aria-pressed={checked}
    >
      <span className={`h-5 w-9 rounded-full border transition-colors relative ${checked ? "bg-amber-500/80 border-amber-400" : "bg-[#141b26] border-[#2a3648]"}`}>
        <span className={`absolute top-[2px] h-3.5 w-3.5 rounded-full bg-slate-100 transition-all ${checked ? "left-[19px]" : "left-[2px]"}`} />
      </span>
      {label ? <span className="text-[12.5px] text-slate-400">{label}</span> : null}
    </button>
  );
}
