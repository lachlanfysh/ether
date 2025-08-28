import React, { useMemo, useState, useRef } from "react";

// === Groovebox GUI Mock — Compact 720×720 (≈72mm×72mm) with SHIFT Map (P1–P16) ===
// Transport-4 (PLAY, REC, WRITE, SHIFT).
// SHIFT Map rows:
//  Row1 Palette (P1–P4): Sound Select, Pattern Select/Chain, Clone Pattern→Slot, FX/Repeat Pick
//  Row2 Tools (P5–P8): Copy, Cut, Delete, Nudge (sticky while SHIFT held/latched)
//  Row3 Stamps (P9–P12): Accent, Ratchet, Tie, Micro (apply to new steps)
//  Row4 Other  (P13–P16): Quantize, Swing Nudge, Pattern Length, Scene Snapshot

// ---- Sizing (px) ----
const UI = {
  W: 720,
  H: 720,
  header: 36,
  footer: 30,
  gap: 6,
  step: 80,      // STEP cell size
  padMini: 54,   // KIT mini pad size
  knob: 60,      // knob diameter box
};

// ---- Palette (Platinum + pastels) ----
const clr = {
  window: "#d9d9d9",
  borderDark: "#7f7f7f",
  borderDarker: "#5a5a5a",
  borderLight: "#ffffff",
  text: "#000000",
};
const pastel = [
  "#FDE2E4", "#E2F0CB", "#CDE7F0", "#EAD5E6",
  "#FFF1BA", "#E2ECF9", "#F3D1DC", "#D6F4D6",
  "#EAD9C8", "#D9E8E3", "#E6D5F7", "#FDE7C8",
  "#D2E3FC", "#F3E8FF", "#E5FAD6", "#FEE4E2",
];

// ---- Bevel helpers ----
function bevelOut(){
  return {
    boxShadow:
      `inset -1px -1px 0 ${clr.borderDark}, inset 1px 1px 0 ${clr.borderLight},\n       inset -2px -2px 0 ${clr.borderDarker}, inset 2px 2px 0 ${clr.window}`,
  };
}
function bevelIn(){
  return {
    boxShadow:
      `inset -1px -1px 0 ${clr.borderLight}, inset 1px 1px 0 ${clr.borderDark},\n       inset -2px -2px 0 ${clr.window}, inset 2px 2px 0 ${clr.borderDarker}`,
  };
}

// ---- Utilities ----
function describeArc(x, y, radius, startAngle, endAngle){
  const start = polarToCartesian(x,y,radius,endAngle);
  const end = polarToCartesian(x,y,radius,startAngle);
  const largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
  return ["M", start.x, start.y, "A", radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(" ");
}
function polarToCartesian(cx, cy, r, angle){
  const a = (angle) * Math.PI/180; return { x: cx + r * Math.cos(a), y: cy + r * Math.sin(a) };
}

// ---- Controls ----
function PlatinumButton({ active, children, onClick, title, small, onMouseDown, onMouseUp, onDoubleClick }){
  return (
    <button
      title={title}
      onClick={onClick}
      onMouseDown={onMouseDown}
      onMouseUp={onMouseUp}
      onDoubleClick={onDoubleClick}
      className={`select-none ${small?"px-2 py-[1px] text-[10px]":"px-3 py-1 text-[11px]"}`}
      style={{ background: clr.window, color: clr.text, border: "1px solid #0000", ...(active?bevelIn():bevelOut()) }}
    >
      {children}
    </button>
  );
}

function TinyField({ label, value }){
  return (
    <div className="flex items-center gap-1">
      <div className="text-[10px]" style={{ color: clr.text }}>{label}</div>
      <div className="px-1 text-[10px]" style={{ background: clr.window, ...bevelIn() }}>{value}</div>
    </div>
  );
}

function Knob({ label, value, onChange, min=0, max=1, step=0.01 }){
  const box = UI.knob; // 60
  return (
    <div className="flex flex-col items-center gap-1" style={{ width: box+8 }}>
      <div className="relative" style={{ width: box, height: box, background: clr.window, ...bevelOut(), borderRadius: 6 }}>
        <svg viewBox="0 0 100 100" className="absolute inset-1">
          <circle cx="50" cy="50" r="44" fill={clr.window} stroke={clr.borderDarker} strokeWidth="2" />
          <path d={`M50 6 A 44 44 0 1 1 49.99 6`} fill="none" stroke={clr.borderDark} strokeWidth="3" strokeLinecap="round"/>
          <path d={describeArc(50,50,44, -140, -140 + value*(280))} fill="none" stroke={clr.text} strokeWidth="3" strokeLinecap="round"/>
          {(() => { const ang=(-140+value*280)*Math.PI/180; const x=50+Math.cos(ang)*34; const y=50+Math.sin(ang)*34; return <line x1="50" y1="50" x2={x} y2={y} stroke={clr.text} strokeWidth="3" strokeLinecap="round"/>; })()}
        </svg>
        <input type="range" min={min} max={max} step={step} value={value} onChange={(e)=>onChange(Number(e.target.value))} className="absolute inset-0 opacity-0 cursor-pointer"/>
      </div>
      <div className="text-[10px]" style={{ color: clr.text }}>{label}</div>
    </div>
  )
}

// ---- Content helpers ----
const kitNames = [
  "Kick","Snare","Closed Hat","Open Hat","Low Tom","Mid Tom","High Tom","Clap",
  "Rim","Ride","Crash","Perc 1","Perc 2","Shaker","Cowbell","FX"
];
const noteNames = [
  "C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3","C4","C#4","D4","D#4","E4"
];
const patternLabels = "ABCDEFGHIJKLMNOP".split("");

export default function GrooveboxMock(){
  const [mode, setMode] = useState("STEP"); // "STEP"|"PAD"|"FX"|"MIX"|"SEQ"|"CHOP"
  const [play, setPlay] = useState(false);
  const [write, setWrite] = useState(false);
  const [bpm, setBpm] = useState(128);
  const [swing, setSwing] = useState(56);
  const [quant, setQuant] = useState(50);
  const [pattern, setPattern] = useState(0); // 0..15
  const [perfLock, setPerfLock] = useState(false);
  const [instrType, setInstrType] = useState('KIT'); // 'KIT'|'CHROMA'|'SLICED'
  const [armedInstrIx, setArmedInstrIx] = useState(3); // I4 demo
  const [patternLength, setPatternLength] = useState(16); // 16|32|48|64

  const [perf] = useState([
    { label: "I4:Cut", value: 0.35, pickup: false },
    { label: "I3:Start", value: 0.6, pickup: true },
    { label: "I7:Send", value: 0.2, pickup: false },
  ]);

  const [stamps, setStamps] = useState({ accent:false, ratchet:0, tie:false, micro:0 });
  const [steps, setSteps] = useState(Array(16).fill(false));
  const [stepNotes, setStepNotes] = useState(Array(16).fill(0).map(()=>[]));
  const [stepMeta, setStepMeta] = useState(Array.from({length:16}, ()=>({ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false })));
  const [page, setPage] = useState(0);
  const [editingStep, setEditingStep] = useState(null);
  const holdTimer = useRef(null);

  // SHIFT + Map
  const [shiftHeld, setShiftHeld] = useState(false);
  const [shiftLatched, setShiftLatched] = useState(false);
  const shiftActive = shiftHeld || shiftLatched;
  const [shiftAction, setShiftAction] = useState(null); // null|"SOUND"|"PATT"|"CLONE"|"FXPICK"
  const [toolsMode, setToolsMode] = useState(null); // null|"COPY"|"CUT"|"DELETE"|"NUDGE"
  const [chainSel, setChainSel] = useState([]);
  const [toast, setToast] = useState(null);
  const [swingSign, setSwingSign] = useState(1); // 1|-1

  // FX state
  const [fx, setFx] = useState({ type: "Stutter", wet: 0.2, rate: 0.5, latched:false, repeat:"1/16"});

  // Scenes
  const [sceneSlot, setSceneSlot] = useState(null); // 'A'|'B'|'C'|'D'|null

  const pages = useMemo(()=> Array.from({length: Math.ceil(patternLength/16)}, (_,i)=>i), [patternLength]);

  function showToast(msg){ setToast(msg); window.setTimeout(()=>setToast(null), 1200); }

  // Helpers
  function currentSubLabel(){
    if(instrType==='KIT') return kitNames[0] || `Pad 1`;
    if(instrType==='CHROMA') return noteNames[0] || 'C3';
    return `S1`;
  }

  function placeOrToggleStep(i, multi){
    // If a Tool is active, route to tool behavior
    if(toolsMode){
      if(toolsMode==='DELETE'){
        const arr = steps.slice(); const notes = stepNotes.map(n=> n.slice()); const meta = stepMeta.map(m=>({...m}));
        arr[i]=false; notes[i]=[]; meta[i]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false };
        setSteps(arr); setStepNotes(notes); setStepMeta(meta);
      } else if(toolsMode==='COPY' || toolsMode==='CUT'){
        // simple mock: copy from first active step to this index
        const srcIx = steps.findIndex(v=>v);
        if(srcIx>=0){
          const arr = steps.slice(); const notes = stepNotes.map(n=> n.slice()); const meta = stepMeta.map(m=>({...m}));
          arr[i]=true; notes[i] = [...(stepNotes[srcIx]||[])]; meta[i] = {...stepMeta[srcIx]};
          setSteps(arr); setStepNotes(notes); setStepMeta(meta);
          if(toolsMode==='CUT'){
            const a2 = steps.slice(); const n2 = stepNotes.map(n=> n.slice()); const m2 = stepMeta.map(m=>({...m}));
            a2[srcIx]=false; n2[srcIx]=[]; m2[srcIx]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false };
            setSteps(a2); setStepNotes(n2); setStepMeta(m2);
          }
        } else { showToast('Copy: no source (mock uses first active step).'); }
      } else if(toolsMode==='NUDGE'){
        const meta = stepMeta.map((m,ix)=> ix===i? {...m, micro: Math.max(-60, Math.min(60, m.micro + 15)) } : m);
        setStepMeta(meta);
      }
      return;
    }

    const arr = steps.slice();
    const notesArr = stepNotes.map(n=> n.slice());
    const metaArr = stepMeta.map(m=> ({...m}));
    if(!arr[i]){
      arr[i] = true;
      const label = instrType==='KIT' ? (kitNames[0]||`Pad 1`)
                  : instrType==='CHROMA' ? (noteNames[0]||'C3')
                  : `S1`;
      if(multi && instrType==='CHROMA') notesArr[i] = [...(notesArr[i]||[]), label];
      else notesArr[i] = [label];
      metaArr[i] = {
        lenSteps: 1,
        micro: stamps.micro===1 ? 30 : (stamps.micro===-1 ? -30 : 0),
        ratchet: stamps.ratchet>0 ? (stamps.ratchet+1) : 1,
        tie: stamps.tie,
        accent: stamps.accent
      };
    } else {
      if(multi && instrType==='CHROMA'){
        const label = noteNames[0]||'C3';
        const existed = notesArr[i]||[];
        notesArr[i] = Array.from(new Set([...existed, label]));
      } else {
        arr[i] = false; notesArr[i] = []; metaArr[i] = { lenSteps:1, micro:0, ratchet:1, tie:false, accent:false };
      }
    }
    setSteps(arr); setStepNotes(notesArr); setStepMeta(metaArr);
  }

  function PerfLabel({ix}){
    const p = perf[ix];
    return (
      <div className="flex items-center gap-1">
        <div className="text-[10px] w-20 truncate" style={{ color: clr.text }}>P{ix+1} {p.label}</div>
        <div className="h-2.5 w-24" style={{ background: clr.window, ...bevelIn() }}>
          <div className="h-2.5" style={{ width:`${Math.round(p.value*100)}%`, background: clr.text }} />
        </div>
        {p.pickup && <span className="text-[9px]" style={{ color: clr.borderDarker }}>pickup</span>}
      </div>
    );
  }

  // --- Header ---
  const Header = (
    <div style={{ height: UI.header, background: clr.window, ...bevelOut() }} className="w-full flex items-center justify-between px-2">
      <div className="flex items-center gap-1">
        <PlatinumButton small active={play} onClick={()=>{
          if(shiftActive){ // SHIFT+PLAY => tap tempo
            setBpm(b=> Math.max(60, Math.min(200, b + 1)));
            showToast('Tap tempo');
          } else { setPlay(p=>!p); }
        }}>{play?"■":"▶"}</PlatinumButton>
        <PlatinumButton small onClick={()=>{ setMode('CHOP'); showToast('REC→CHOP'); }}>REC</PlatinumButton>
        <PlatinumButton small active={write} onClick={()=>{ if(shiftActive){ setMode(m=> m==='STEP'? 'PAD':'STEP'); } else { setWrite(w=>!w);} }}>{write?"WRITE*":"WRITE"}</PlatinumButton>
        <PlatinumButton small active={shiftLatched||shiftHeld}
          onMouseDown={()=> setShiftHeld(true)}
          onMouseUp={()=> setShiftHeld(false)}
          onDoubleClick={()=> setShiftLatched(v=>!v)}
        >SHIFT</PlatinumButton>
        <div className="text-[10px]" style={{ color: clr.text }}>Pat <b>{patternLabels[pattern]}</b></div>
        <PlatinumButton small onClick={()=>setBpm(b=>b+1)}>BPM {bpm}</PlatinumButton>
        <PlatinumButton small title="Swing">Sw {swing}%</PlatinumButton>
        <PlatinumButton small title="Quantize">Q {quant}%</PlatinumButton>
      </div>
      <div className="flex items-center gap-2">
        <TinyField label="CPU" value="14%" />
        <TinyField label="V" value="9" />
        <PerfLabel ix={0}/>
      </div>
    </div>
  );

  // --- Footer ---
  const Footer = (
    <div style={{ height: UI.footer, background: clr.window, ...bevelOut() }} className="w-full flex items-center justify-between px-2">
      <div className="flex items-center gap-2">
        <div className="text-[9px]" style={{ color: clr.borderDarker }}>Stamps:</div>
        <PlatinumButton small active={stamps.accent} onClick={()=>setStamps(s=>({...s,accent:!s.accent}))}>Accent</PlatinumButton>
        <PlatinumButton small active={stamps.ratchet>0} onClick={()=>setStamps(s=>({...s,ratchet:(s.ratchet+1)%4}))}>{`Rat ${stamps.ratchet===0?"off":"×"+ (stamps.ratchet+1)}`}</PlatinumButton>
        <PlatinumButton small active={stamps.tie} onClick={()=>setStamps(s=>({...s,tie:!s.tie}))}>Tie</PlatinumButton>
        <PlatinumButton small active={stamps.micro!==0} onClick={()=>setStamps(s=>({...s, micro: s.micro===0?1: (s.micro===1?-1:0)}))}>{`Micro ${stamps.micro===0?"0": stamps.micro===1?"+25%":"-25%"}`}</PlatinumButton>
      </div>
      <div className="flex items-center gap-1">
        <PlatinumButton small active={perfLock} onClick={()=>setPerfLock(!perfLock)} title="Lock perf encoders across patterns">Perf Lock</PlatinumButton>
      </div>
    </div>
  );

  // --- Views ---
  function StepView(){
    const accentColor = pastel[armedInstrIx % pastel.length];
    const labelFS = 10, badgeFS = 9, idxFS = 9;
    return (
      <div className="flex flex-col h-full" style={{ padding: UI.gap }}>
        <div className="flex items-center justify-between">
          <div className="text-[10px]" style={{ color: clr.text }}>
            STEP — I{armedInstrIx+1} ({instrType === 'KIT' ? 'Kit' : instrType === 'CHROMA' ? 'Chrom' : 'Slice'}) — {currentSubLabel()}
          </div>
          <div className="flex gap-1">
            {pages.map((p)=> (
              <div key={p} style={{ width:6, height:6, background: p===page? clr.text : clr.borderDark, borderRadius: 3 }} onClick={()=>setPage(p)} />
            ))}
          </div>
        </div>
        <div className="mt-1 grid grid-cols-4" style={{ gap: UI.gap }}>
          {Array.from({length:16}).map((_,i)=> {
            const active = steps[i];
            const notes = stepNotes[i] || [];
            const label = notes.length === 0 ? '' : (notes.length === 1 ? notes[0] : `Chord ${notes.length}`);
            return (
              <button
                key={i}
                onClick={(e)=>placeOrToggleStep(i, e.shiftKey)}
                onMouseDown={()=>{ if(holdTimer.current) clearTimeout(holdTimer.current); holdTimer.current = window.setTimeout(()=> setEditingStep(i), 450); }}
                onMouseUp={()=>{ if(holdTimer.current) { clearTimeout(holdTimer.current); holdTimer.current=null; } }}
                onMouseLeave={()=>{ if(holdTimer.current) { clearTimeout(holdTimer.current); holdTimer.current=null; } }}
                className="relative flex items-center justify-center"
                style={{ width: UI.step, height: UI.step, background: clr.window, ...(active?bevelIn():bevelOut()) }}
              >
                <div className="absolute inset-0 pointer-events-none" style={{ background: accentColor, opacity: active?0.25:0.12 }} />
                <div className="absolute" style={{ top:4, left:4, fontSize: idxFS, color: clr.text }}>{i+1+page*16}</div>
                {active && (<div className="font-semibold" style={{ color: clr.text, fontSize: labelFS }}>{label}</div>)}
                {stepMeta[i].accent && <div className="absolute" style={{ top:4, right:4, fontSize: badgeFS, color: clr.text }}>›</div>}
                {stepMeta[i].tie && <div className="absolute left-1 right-1" style={{ bottom:4, height:2, background: clr.text }}/>} 
                {stepMeta[i].ratchet>1 && <div className="absolute" style={{ bottom:4, right:4, fontSize: badgeFS, color: clr.text }}>×{stepMeta[i].ratchet}</div>}
                {stepMeta[i].micro!==0 && <div className="absolute" style={{ top:4, right:18, width:5, height:5, background: stepMeta[i].micro>0? clr.text : clr.borderDark }} />}
              </button>
            );
          })}
        </div>
      </div>
    );
  }

  function PadView(){
    const accentColor = pastel[armedInstrIx % pastel.length];
    return (
      <div className="flex h-full gap-2" style={{ padding: UI.gap }}>
        {/* Left: demo instrument selector */}
        <div style={{ width: 300 }} className="flex flex-col gap-2">
          <div className="flex gap-1">
            <PlatinumButton small active={instrType==='KIT'} onClick={()=>setInstrType('KIT')}>Kit</PlatinumButton>
            <PlatinumButton small active={instrType==='CHROMA'} onClick={()=>setInstrType('CHROMA')}>Chrom</PlatinumButton>
            <PlatinumButton small active={instrType==='SLICED'} onClick={()=>setInstrType('SLICED')}>Slice</PlatinumButton>
          </div>
          {/* Simple representation; real sound choose via SHIFT P1 */}
          <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
            {Array.from({length:16}).map((_,i)=> (
              <div key={i} className="relative cursor-pointer" onClick={()=>setArmedInstrIx(i)} style={{ width: UI.padMini, height: UI.padMini, background: clr.window, ...(armedInstrIx===i?bevelIn():bevelOut()) }}>
                <div className="absolute inset-0" style={{ background: pastel[i%pastel.length], opacity: 0.18 }} />
                <div className="absolute" style={{ top:4, left:4, fontSize:10, color: clr.text }}>I{i+1}</div>
              </div>
            ))}
          </div>
        </div>
        {/* Right: Macros & info */}
        <div className="flex-1 flex flex-col">
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>
            PAD — I{armedInstrIx+1} ({instrType === 'KIT' ? 'Kit' : instrType === 'CHROMA' ? 'Chrom' : 'Slice'}) — {currentSubLabel()}
          </div>
          <div className="flex gap-4">
            <div className="flex gap-3">
              <div className="flex flex-col items-center gap-1">
                <div className="text-[9px]" style={{ color: clr.borderDarker }}>Macros</div>
                <div className="flex gap-3">
                  <Knob label="M1 Cutoff" value={0.35} onChange={()=>{}} />
                  <Knob label="M2 Res/Drive" value={0.2} onChange={()=>{}} />
                  <Knob label="M3 EnvAmt" value={0.5} onChange={()=>{}} />
                  <Knob label="M4 Vib" value={0.1} onChange={()=>{}} />
                </div>
              </div>
            </div>
            <div className="ml-auto flex flex-col gap-1">
              <PlatinumButton small>Scale: D Dorian</PlatinumButton>
              <PlatinumButton small>Repeat: {fx.repeat}</PlatinumButton>
            </div>
          </div>
          <div className="mt-2 grid grid-cols-2" style={{ gap: UI.gap }}>
            <div className="p-2" style={{ background: clr.window, ...bevelOut() }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>OSC / Sample</div>
              <div style={{ height: 84, background: clr.window, ...bevelIn(), position:'relative' }}>
                <div className="absolute inset-0" style={{ background: accentColor, opacity: 0.15 }} />
              </div>
            </div>
            <div className="p-2" style={{ background: clr.window, ...bevelOut() }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>FILTER</div>
              <div className="flex gap-3">
                <Knob label="Cutoff" value={0.5} onChange={()=>{}} />
                <Knob label="Res" value={0.3} onChange={()=>{}} />
              </div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  function FxView(){
    return (
      <div className="h-full flex flex-col" style={{ padding: UI.gap }}>
        <div className="text-[10px] mb-1" style={{ color: clr.text }}>FX — Punch‑ins & Repeat</div>
        <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
          {["Stutter","Tape","Filter","Delay"].map(name=> (
            <div key={name} className="h-[64px] flex items-center justify-center" style={{ background: clr.window, ...(fx.type===name?bevelIn():bevelOut()) }} onClick={()=>setFx({...fx, type:name})}>
              <div className="text-[10px]" style={{ color: clr.text }}>{name}</div>
            </div>
          ))}
        </div>
        <div className="mt-2 flex items-center gap-4">
          <Knob label="Rate" value={fx.rate} onChange={(v)=>setFx({...fx, rate:v})} />
          <Knob label="Wet" value={fx.wet} onChange={(v)=>setFx({...fx, wet:v})} />
          <PlatinumButton small active={fx.latched} onClick={()=>setFx({...fx,latched:!fx.latched})}>Latch</PlatinumButton>
          <PlatinumButton small> Bake… </PlatinumButton>
        </div>
        <div className="mt-2">
          <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Note‑Repeat</div>
          <div className="flex flex-wrap" style={{ gap: UI.gap }}>
            {["1/4","1/8","1/8T","1/16","1/16T","1/32","1/32T","1/64"].map(r=> (
              <PlatinumButton key={r} small active={fx.repeat===r} onClick={()=>setFx({...fx, repeat:r})}>{r}</PlatinumButton>
            ))}
          </div>
        </div>
      </div>
    );
  }

  function MixView(){
    return (
      <div className="h-full flex flex-col" style={{ padding: UI.gap }}>
        <div className="flex items-center justify-between mb-1">
          <div className="text-[10px]" style={{ color: clr.text }}>MIX — Scene {sceneSlot??"(unsaved)"}</div>
          <div className="flex items-center" style={{ gap: UI.gap }}>
            {["A","B","C","D"].map(s=> (
              <PlatinumButton key={s} small active={sceneSlot===s} onClick={()=>setSceneSlot(s)}>Scene {s}</PlatinumButton>
            ))}
          </div>
        </div>
        <div className="grid grid-cols-8 flex-1 overflow-y-auto" style={{ gap: UI.gap }}>
          {Array.from({length:16}).map((_,i)=> (
            <div key={i} className="p-2 flex flex-col items-center" style={{ background: clr.window, ...bevelOut(), position:'relative' }}>
              <div className="absolute inset-0" style={{ background: pastel[i%pastel.length], opacity: 0.12 }} />
              <div className="text-[9px] mb-1 relative" style={{ color: clr.text }}>I{i+1}</div>
              <div className="relative" style={{ height: 120, width: 16, background: clr.window, ...bevelIn() }}>
                <div className="absolute bottom-0 left-0 right-0" style={{ height:`${(i%8+1)*10}%`, background: clr.text, opacity:.3 }}/>
              </div>
              <div className="mt-1 text-[9px]" style={{ color: clr.borderDarker }}>Pan</div>
              <input className="w-14" type="range" min={0} max={1} step={0.01} />
              <div className="mt-1 text-[9px]" style={{ color: clr.borderDarker }}>Send</div>
              <input className="w-14" type="range" min={0} max={1} step={0.01} />
              <div className="mt-1 flex" style={{ gap: 4 }}>
                <PlatinumButton small>M</PlatinumButton>
                <PlatinumButton small>S</PlatinumButton>
              </div>
            </div>
          ))}
        </div>
      </div>
    );
  }

  function SeqView(){
    const [chain,setChain] = useState([{label:"A", reps:4},{label:"B", reps:8}]);
    return (
      <div className="h-full grid grid-cols-3" style={{ gap: UI.gap, padding: UI.gap }}>
        <div className="col-span-2">
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Patterns</div>
          <div className="grid grid-cols-8" style={{ gap: UI.gap }}>
            {patternLabels.map((L,ix)=> (
              <div key={L} className="h-8 flex items-center justify-center" style={{ background: clr.window, ...(pattern===ix?bevelIn():bevelOut()) }} onClick={()=>setPattern(ix)}>{L}</div>
            ))}
          </div>
          <div className="mt-2 flex items-center" style={{ gap: UI.gap }}>
            <PlatinumButton small active={perfLock} onClick={()=>setPerfLock(!perfLock)}>Perf Lock</PlatinumButton>
            <div className="ml-auto"><PlatinumButton small>Queue Next</PlatinumButton></div>
          </div>
        </div>
        <div>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Song Chain</div>
          <div className="space-y-2">
            {chain.map((c, i)=> (
              <div key={i} className="flex items-center" style={{ gap: UI.gap, background: clr.window, ...bevelOut(), padding: 6 }}>
                <div className="w-7 h-7 flex items-center justify-center" style={{ background: clr.window, ...bevelIn() }}>{c.label}</div>
                <div className="text-[10px]" style={{ color: clr.text }}>×</div>
                <input type="number" min={1} max={16} value={c.reps}
                  className="w-14 px-1 py-1 text-[10px]"
                  style={{ background: clr.window, ...bevelIn(), color: clr.text }}
                  onChange={(e)=>{
                    const v = Math.max(1,Math.min(16, Number(e.target.value)||1));
                    const arr = chain.slice(); arr[i] = {...c, reps:v}; setChain(arr);
                  }}
                />
                <div className="ml-auto"><PlatinumButton small>Del</PlatinumButton></div>
              </div>
            ))}
            <PlatinumButton small>+ Add Pattern</PlatinumButton>
          </div>
        </div>
      </div>
    );
  }

  function ChopView(){
    const [zoom, setZoom] = useState(0.3);
    const [norm, setNorm] = useState(true);
    return (
      <div className="h-full flex flex-col" style={{ padding: UI.gap }}>
        <div className="flex items-center justify-between mb-1">
          <div className="text-[10px]" style={{ color: clr.text }}>CHOP — Sample_01.wav</div>
          <div className="flex items-center" style={{ gap: UI.gap }}>
            <PlatinumButton small active={norm} onClick={()=>setNorm(!norm)}>Normalize</PlatinumButton>
            <PlatinumButton small>Snap</PlatinumButton>
          </div>
        </div>
        <div className="flex" style={{ gap: UI.gap }}>
          <div className="flex-1 relative overflow-hidden" style={{ height: 160, background: clr.window, ...bevelIn() }}>
            <div className="absolute inset-0" style={{backgroundImage:"repeating-linear-gradient(90deg, rgba(0,0,0,0.2) 0 1px, transparent 1px 6px)"}}/>
            <div className="absolute top-0 bottom-0 left-[15%]" style={{ width:2, background: clr.text }}/>
            <div className="absolute top-0 bottom-0 left-[48%]" style={{ width:2, background: clr.text }}/>
            <div className="absolute top-0 bottom-0 left-[80%]" style={{ width:2, background: clr.text }}/>
          </div>
          <div className="w-52 p-2" style={{ background: clr.window, ...bevelOut() }}>
            <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Controls</div>
            <div className="space-y-2">
              <div className="flex items-center justify-between text-[10px]" style={{ color: clr.text }}>
                <span>Zoom</span>
                <input type="range" min={0.05} max={1} step={0.01} value={zoom} onChange={(e)=>setZoom(Number(e.target.value))}/>
              </div>
              <PlatinumButton small onClick={()=>{ setInstrType('SLICED'); setMode('PAD'); }}>Assign 16</PlatinumButton>
            </div>
          </div>
        </div>
      </div>
    );
  }

  function StepEditorOverlay(){
    if(editingStep===null) return null;
    const i = editingStep;
    const meta = stepMeta[i];
    const close = ()=> setEditingStep(null);
    const setMeta = (m)=>{ const arr = stepMeta.map((x,ix)=> ix===i? {...x, ...m} : x); setStepMeta(arr); };
    const del = ()=>{ const arr = steps.slice(); const notes = stepNotes.map(n=> n.slice()); const metas = stepMeta.slice(); arr[i]=false; notes[i]=[]; metas[i]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false }; setSteps(arr); setStepNotes(notes); setStepMeta(metas); close(); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[320px]" style={{ background: clr.window, ...bevelOut(), padding: 10 }}>
          <div className="flex items-center justify-between mb-1">
            <div className="text-[10px]" style={{ color: clr.text }}>Step {i+1} — Editor</div>
            <PlatinumButton small onClick={close}>Close</PlatinumButton>
          </div>
          <div className="grid grid-cols-2" style={{ gap: UI.gap }}>
            <div style={{ background: clr.window, ...bevelOut(), padding: 6 }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Length (steps)</div>
              <input type="range" min={1} max={8} step={1} value={meta.lenSteps} onChange={(e)=>setMeta({lenSteps:Number(e.target.value)})} />
            </div>
            <div style={{ background: clr.window, ...bevelOut(), padding: 6 }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Micro (ticks)</div>
              <input type="range" min={-60} max={60} step={1} value={meta.micro} onChange={(e)=>setMeta({micro:Number(e.target.value)})} />
            </div>
            <div style={{ background: clr.window, ...bevelOut(), padding: 6 }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Ratchet</div>
              <div className="flex" style={{ gap: UI.gap }}>
                {[1,2,3,4].map(r=> (
                  <PlatinumButton key={r} small active={meta.ratchet===r} onClick={()=>setMeta({ratchet:r})}>×{r}</PlatinumButton>
                ))}
              </div>
            </div>
            <div style={{ background: clr.window, ...bevelOut(), padding: 6 }}>
              <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Flags</div>
              <div className="flex" style={{ gap: UI.gap }}>
                <PlatinumButton small active={meta.tie} onClick={()=>setMeta({tie:!meta.tie})}>Tie</PlatinumButton>
                <PlatinumButton small active={meta.accent} onClick={()=>setMeta({accent:!meta.accent})}>Accent</PlatinumButton>
              </div>
            </div>
          </div>
          <div className="mt-2 flex items-center justify-between">
            <PlatinumButton small onClick={del}>Delete Step</PlatinumButton>
            <PlatinumButton small onClick={close}>Done</PlatinumButton>
          </div>
        </div>
      </div>
    );
  }

  // --- SHIFT Map Legend & Actions ---
  function ShiftLegend(){
    if(!shiftActive || shiftAction) return null; // hide legend when in a Palette sub-action
    const microLabel = stamps.micro===0? "0" : (stamps.micro===1? "+25%" : "-25%");
    const cell = (label, onClick, active)=> (
      <div className="h-12 flex items-center justify-center cursor-pointer select-none"
           style={{ background: clr.window, ...(active?bevelIn():bevelOut()), color: clr.text, fontSize: 10 }}
           onClick={onClick}>{label}</div>
    );
    return (
      <div className="absolute inset-0 z-20 flex items-center justify-center" style={{ background: "rgba(0,0,0,0.35)" }}>
        <div className="w-[86%]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
            {/* Row 1 — Palette */}
            {cell("P1 Sound Select", ()=> setShiftAction('SOUND'))}
            {cell("P2 Pattern/Chain", ()=> setShiftAction('PATT'))}
            {cell("P3 Clone→Slot", ()=> setShiftAction('CLONE'))}
            {cell("P4 FX/Repeat", ()=> setShiftAction('FXPICK'))}
            {/* Row 2 — Tools */}
            {cell("P5 Copy", ()=> setToolsMode('COPY'), toolsMode==='COPY')}
            {cell("P6 Cut",  ()=> setToolsMode('CUT'), toolsMode==='CUT')}
            {cell("P7 Delete", ()=> setToolsMode('DELETE'), toolsMode==='DELETE')}
            {cell("P8 Nudge", ()=> setToolsMode('NUDGE'), toolsMode==='NUDGE')}
            {/* Row 3 — Stamps */}
            {cell(`P9 Accent ${stamps.accent?"on":"off"}`, ()=> setStamps(s=>({...s,accent:!s.accent})))}
            {cell(`P10 Ratchet ×${stamps.ratchet===0?1:stamps.ratchet+1}`, ()=> setStamps(s=>({...s,ratchet:(s.ratchet+1)%4})))}
            {cell(`P11 Tie ${stamps.tie?"on":"off"}`, ()=> setStamps(s=>({...s,tie:!s.tie})))}
            {cell(`P12 Micro ${microLabel}`, ()=> setStamps(s=>({...s, micro: s.micro===0?1: (s.micro===1?-1:0)})))}
            {/* Row 4 — Other */}
            {cell("P13 Quantize", ()=> { showToast(`Quantize applied (${quant}%)`); if(!shiftLatched) setTimeout(()=> setShiftHeld(false), 50); })}
            {cell("P14 Swing ±1%", ()=> { setSwingSign(sign=> (sign===1?-1:1)); setSwing(v=> Math.max(45, Math.min(75, v + swingSign))); })}
            {cell(`P15 Len ${patternLength}`, ()=> { const order = [16,32,48,64]; const ix = order.indexOf(patternLength); setPatternLength(order[(ix+1)%order.length]); })}
            {cell("P16 Scene A–D", ()=> setSceneOverlay(true))}
          </div>
          <div className="mt-1 text-[9px]" style={{ color: clr.borderDarker }}>SHIFT is {shiftLatched? 'latched (double‑tap to release)':'held'}. Palette actions are one‑shot; Tools are sticky while SHIFT is active.</div>
        </div>
      </div>
    )
  }

  // Palette Overlays
  function SoundSelectOverlay(){
    if(shiftAction!=="SOUND") return null;
    const pick = (ix)=>{ setArmedInstrIx(ix); setShiftAction(null); if(!shiftLatched) setShiftHeld(false); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[70%]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Select Instrument I1–I16</div>
          <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
            {Array.from({length:16}).map((_,i)=> (
              <div key={i} className="h-12 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...(armedInstrIx===i?bevelIn():bevelOut()) }} onClick={()=>pick(i)}>I{i+1}</div>
            ))}
          </div>
        </div>
      </div>
    );
  }

  function PatternSelectOverlay(){
    if(shiftAction!=="PATT") return null;
    const toggle = (ix)=>{
      setPattern(ix);
      setChainSel(prev=> prev.includes(ix)? prev.filter(x=>x!==ix): [...prev, ix]);
    };
    const done = ()=>{ showToast(`Chain: ${chainSel.map(i=>patternLabels[i]).join('→')||patternLabels[pattern]}`); setChainSel([]); setShiftAction(null); if(!shiftLatched) setShiftHeld(false); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[70%]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="flex items-center justify-between mb-1">
            <div className="text-[10px]" style={{ color: clr.text }}>Select Pattern A–P (tap more while SHIFT held to chain)</div>
            <PlatinumButton small onClick={done}>Done</PlatinumButton>
          </div>
          <div className="grid grid-cols-8" style={{ gap: UI.gap }}>
            {patternLabels.map((L,ix)=> (
              <div key={L} className="h-8 flex items-center justify-center cursor-pointer"
                   style={{ background: clr.window, ...(chainSel.includes(ix)||pattern===ix?bevelIn():bevelOut()) }} onClick={()=>toggle(ix)}>{L}</div>
            ))}
          </div>
        </div>
      </div>
    );
  }

  function ClonePatternOverlay(){
    if(shiftAction!=="CLONE") return null;
    const cloneTo = (ix)=>{ showToast(`Cloned to ${patternLabels[ix]} & queued`); setPattern(ix); setShiftAction(null); if(!shiftLatched) setShiftHeld(false); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[70%]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Clone Pattern → choose destination A–P</div>
          <div className="grid grid-cols-8" style={{ gap: UI.gap }}>
            {patternLabels.map((L,ix)=> (
              <div key={L} className="h-8 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...bevelOut() }} onClick={()=>cloneTo(ix)}>{L}</div>
            ))}
          </div>
        </div>
      </div>
    );
  }

  function FxRepeatOverlay(){
    if(shiftAction!=="FXPICK") return null;
    const pickFx = (name)=>{ setFx(f=>({...f, type:name, latched: shiftActive })); showToast(shiftActive? `${name} latched` : `${name} picked`); setShiftAction(null); if(!shiftLatched) setShiftHeld(false); };
    const pickRate = (r)=>{ setFx(f=>({...f, repeat:r })); showToast(`Repeat ${r}`); setShiftAction(null); if(!shiftLatched) setShiftHeld(false); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[70%]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Pick FX (1–4) or Repeat (9–16). Hold SHIFT while picking FX to latch.</div>
          <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
            {["Stutter","Tape","Filter","Delay"].map(name=> (
              <div key={name} className="h-10 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...bevelOut() }} onClick={()=>pickFx(name)}>{name}</div>
            ))}
          </div>
          <div className="mt-2 flex flex-wrap" style={{ gap: UI.gap }}>
            {["1/4","1/8","1/8T","1/16","1/16T","1/32","1/32T","1/64"].map(r=> (
              <PlatinumButton key={r} small onClick={()=>pickRate(r)}>{r}</PlatinumButton>
            ))}
          </div>
        </div>
      </div>
    );
  }

  const [sceneOverlay, setSceneOverlay] = useState(false);
  function SceneOverlay(){
    if(!sceneOverlay) return null;
    const pick = (s)=>{ setSceneSlot(s); setSceneOverlay(false); if(!shiftLatched) setShiftHeld(false); showToast(`Scene ${s} saved/selected`); };
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.35)' }}>
        <div className="w-[360px]" style={{ background: clr.window, ...bevelOut(), padding: 8 }}>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>Scene Snapshot — tap A–D to Save/Recall</div>
          <div className="grid grid-cols-4" style={{ gap: UI.gap }}>
            {["A","B","C","D"].map(s=> (
              <div key={s} className="h-10 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...(sceneSlot===s?bevelIn():bevelOut()) }} onClick={()=>pick(s)}>{s}</div>
            ))}
          </div>
          <div className="mt-2 flex items-center justify-end"><PlatinumButton small onClick={()=>{ setSceneOverlay(false); }}>Close</PlatinumButton></div>
        </div>
      </div>
    );
  }

  // --- App root ---
  return (
    <div className="w-full min-h-screen flex items-center justify-center" style={{ background: "#c0c0c0" }}>
      <div className="relative" style={{ width: UI.W, height: UI.H, background: clr.window, ...bevelOut(), overflow: 'hidden', outline: '1px solid #000', outlineOffset: '-1px', fontFamily: 'Geneva, Verdana, sans-serif', fontSize: 10 }}>
        {Header}
        <div className="flex" style={{ height: UI.H - UI.header - UI.footer }}>
          <div className="flex-1 relative">
            {mode==="STEP" && <StepView/>}
            {mode==="PAD" && <PadView/>}
            {mode==="FX" && <FxView/>}
            {mode==="MIX" && <MixView/>}
            {mode==="SEQ" && <SeqView/>}
            {mode==="CHOP" && <ChopView/>}

            {/* SHIFT Map legend and overlays */}
            <ShiftLegend/>
            <SoundSelectOverlay/>
            <PatternSelectOverlay/>
            <ClonePatternOverlay/>
            <FxRepeatOverlay/>
            <SceneOverlay/>

            <StepEditorOverlay/>

            {toast && (
              <div className="absolute bottom-[40%] left-1/2 -translate-x-1/2 z-50 px-3 py-1"
                   style={{ background: clr.window, ...bevelOut(), color: clr.text }}>{toast}</div>
            )}
          </div>
        </div>
        {Footer}
      </div>
    </div>
  );
}
