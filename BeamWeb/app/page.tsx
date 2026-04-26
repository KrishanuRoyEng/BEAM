'use client';

import { useState, useEffect } from 'react';

interface Reading {
  b_abs: number;
  r_abs: number;
  tcb: number;
  timestamp: string;
}

export default function Dashboard() {
  const [reading, setReading] = useState<Reading | null>(null);
  const [loading, setLoading] = useState(true);

  const fetchReading = async () => {
    try {
      const response = await fetch('/api/readings');
      const data = await response.json();
      setReading(data);
      setLoading(false);
    } catch (error) {
      console.error('Failed to fetch reading:', error);
    }
  };

  useEffect(() => {
    fetchReading();
    // Auto-refresh mechanism as requested (2 seconds interval)
    const interval = setInterval(fetchReading, 2000);
    return () => clearInterval(interval);
  }, []);

  if (loading) {
    return (
      <div className="flex min-h-screen items-center justify-center bg-black">
        <div className="h-12 w-12 animate-spin rounded-full border-t-2 border-cyan-oled shadow-[0_0_15px_rgba(0,229,255,0.5)]"></div>
      </div>
    );
  }

  return (
    <div className="relative min-h-screen bg-black clinical-grid overflow-hidden selection:bg-cyan-oled/30 selection:text-cyan-oled">
      {/* Background Decorative Glows */}
      <div className="absolute top-[-10%] left-[-10%] h-[500px] w-[500px] rounded-full bg-cyan-oled/5 blur-[120px]"></div>
      <div className="absolute bottom-[-10%] right-[-10%] h-[500px] w-[500px] rounded-full bg-magenta-oled/5 blur-[120px]"></div>

      <div className="container mx-auto max-w-lg px-6 py-12 md:py-24 relative z-10">
        {/* Device Header */}
        <div className="mb-12 flex items-center justify-between">
          <div>
            <h1 className="text-xs font-semibold tracking-[0.25em] uppercase text-zinc-500">
              Hardware Assessment
            </h1>
            <div className="mt-1 text-4xl font-extrabold tracking-tighter glow-cyan text-cyan-oled">
              BEAM<span className="text-zinc-600 font-thin ml-3">V-METER</span>
            </div>
          </div>
          <div className="flex flex-col items-end gap-1">
            <div className="flex items-center gap-2">
              <span className="relative flex h-2 w-2">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-oled opacity-75"></span>
                <span className="relative inline-flex rounded-full h-2 w-2 bg-green-oled"></span>
              </span>
              <span className="text-[10px] font-mono font-bold text-green-oled uppercase tracking-widest">
                Linked
              </span>
            </div>
            <span className="text-[9px] font-mono text-zinc-600 uppercase tracking-tighter">
              ESP32_HOST_01
            </span>
          </div>
        </div>

        {/* Main TCB Metric Card */}
        <div className="oled-border relative overflow-hidden rounded-[2.5rem] bg-zinc-950/70 backdrop-blur-xl p-10 border border-zinc-800/10 shadow-2xl">
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <p className="text-[10px] font-bold uppercase tracking-[0.2em] text-zinc-500">
                TcB Concentration
              </p>
              <div className="h-px flex-1 mx-4 bg-zinc-900"></div>
            </div>
            <div className="flex items-baseline gap-3">
              <span className="text-9xl font-black tabular-nums tracking-tighter glow-cyan text-cyan-oled transition-all duration-500">
                {reading?.tcb.toFixed(1)}
              </span>
              <span className="text-3xl font-bold text-zinc-600 tracking-tight">mg/dL</span>
            </div>
          </div>

          {/* Secondary Absorbance Metrics */}
          <div className="mt-12 mb-8 grid grid-cols-2 gap-5">
            <div className="rounded-3xl border border-zinc-800/50 bg-zinc-900/20 p-6 backdrop-blur-sm">
              <p className="text-[9px] font-black uppercase tracking-[0.25em] text-zinc-600 mb-2">B_Absorbance</p>
              <div className="flex items-baseline gap-1">
                <span className="text-3xl font-bold tabular-nums glow-magenta text-magenta-oled">
                  {reading?.b_abs.toFixed(4)}
                </span>
              </div>
            </div>
            <div className="rounded-3xl border border-zinc-800/50 bg-zinc-900/20 p-6 backdrop-blur-sm">
              <p className="text-[9px] font-black uppercase tracking-[0.25em] text-zinc-600 mb-2">R_Absorbance</p>
              <div className="flex items-baseline gap-1">
                <span className="text-3xl font-bold tabular-nums glow-green text-green-oled">
                  {reading?.r_abs.toFixed(4)}
                </span>
              </div>
            </div>
          </div>
          
          {/* Scan Info Footer */}
          <div className="pt-8 border-t border-zinc-900/50 flex justify-between items-center text-[10px] font-mono font-medium uppercase tracking-[0.2em] text-zinc-600">
            <span className="hover:text-cyan-oled transition-colors cursor-default">Status: Ready</span>
            <span className="text-zinc-500 font-bold">
              {reading ? new Date(reading.timestamp).toLocaleTimeString([], { hour12: false }) : '--:--:--'}
            </span>
          </div>
        </div>

        {/* Hardware Status Indicators */}
        <div className="mt-8 grid grid-cols-3 gap-5">
          <div className="rounded-2xl border border-zinc-900/80 bg-zinc-950/40 p-5 text-center transition-transform hover:scale-105 active:scale-95 cursor-default">
            <p className="text-[9px] font-black uppercase tracking-widest text-zinc-700 mb-2">Source</p>
            <p className="text-sm font-bold text-zinc-400">LED_ARRAY</p>
          </div>
          <div className="rounded-2xl border border-zinc-900/80 bg-zinc-950/40 p-5 text-center transition-transform hover:scale-105 active:scale-95 cursor-default">
            <p className="text-[9px] font-black uppercase tracking-widest text-zinc-700 mb-2">Optics</p>
            <p className="text-sm font-bold text-zinc-400 font-mono">CALIBRATED</p>
          </div>
           <div className="rounded-2xl border border-zinc-900/80 bg-zinc-950/40 p-5 text-center transition-transform hover:scale-105 active:scale-95 cursor-default">
            <p className="text-[9px] font-black uppercase tracking-widest text-zinc-700 mb-2">Temp</p>
            <p className="text-sm font-bold text-zinc-400">32.8°C</p>
          </div>
        </div>

        {/* Footer Credit */}
        <div className="mt-16 text-center">
            <p className="text-[9px] font-bold uppercase tracking-[0.4em] text-zinc-800">
                Precision Bilirubin Analyzer
            </p>
        </div>
      </div>
    </div>
  );
}
