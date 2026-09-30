import type { Metadata } from "next";
import type { ReactNode } from "react";
import Link from "next/link";
import "./globals.css";

export const metadata: Metadata = {
  title: "LEGIONFORGE :: Панель администратора 7.3.5 (26124)",
  description:
    "Автономная серверная платформа World of Warcraft: Legion 7.3.5 Build 26124 — конфигуратор сборки, магазин BattlePay, мировые боссы, игроки и файлы.",
};

const NAV = [
  { href: "/", label: "Дашборд", icon: "▦" },
  { href: "/config", label: "Конфигуратор", icon: "⚙" },
  { href: "/shop", label: "Магазин «W»", icon: "🛒" },
  { href: "/bosses", label: "Мировые боссы", icon: "🐉" },
  { href: "/legacy", label: "Фолианты и квесты", icon: "📜" },
  { href: "/players", label: "Игроки и боты", icon: "👥" },
  { href: "/files", label: "Файлы платформы", icon: "🗂" },
  { href: "/docs", label: "Документация", icon: "📘" },
  { href: "/promo", label: "Промо-текст", icon: "📣" },
];

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="ru">
      <body className="min-h-screen antialiased">
        <div className="flex min-h-screen">
          <aside className="w-[248px] shrink-0 border-r border-[#1b2331] bg-[#0a0e16]/80 backdrop-blur px-4 py-5 sticky top-0 h-screen overflow-y-auto">
            <div className="flex items-center gap-3 mb-6 px-2">
              <div className="h-10 w-10 rounded-xl bg-amber-500/15 border border-amber-500/40 grid place-items-center text-xl">⚒</div>
              <div>
                <div className="font-black tracking-wide text-[15px] lf-amber">LEGIONFORGE</div>
                <div className="text-[11px] text-slate-500 lf-mono">7.3.5 · build 26124</div>
              </div>
            </div>
            <nav className="lf-nav flex flex-col gap-1">
              {NAV.map((item) => (
                <Link key={item.href} href={item.href}>
                  <span className="w-5 text-center opacity-80">{item.icon}</span>
                  {item.label}
                </Link>
              ))}
            </nav>
            <div className="mt-8 px-2 text-[11px] text-slate-600 leading-relaxed">
              <div className="lf-mono">platform v3.1.0</div>
              <div>Валюта: Сущность пробуждения (1533)</div>
              <div className="mt-3 text-slate-500">
                START.bat — сборка и запуск<br />PANEL.bat — эта панель
              </div>
            </div>
          </aside>
          <main className="flex-1 min-w-0 px-7 py-7">{children}</main>
        </div>
      </body>
    </html>
  );
}
