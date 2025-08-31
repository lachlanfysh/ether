import React, { useMemo, useRef, useState } from "react";

// === Groovebox GUI Mock — 960×320 (wide) ===
// Layout intent: Top header (Transport‑4), a single **line of 16 option buttons**, a compact view area,
// persistent **bottom 16‑step row**, and a slim footer.
// Option line maps to the 2×16 spec's Top Row quads:
// [INST, SOUND, PATT, CLONE, COPY, CUT, DEL, NUDGE, ACC, RATCH, TIE, MICRO, FX, QNT, SWG, SCENE]

// ---- Sizing ----
const UI = {
  W: 960,
  H: 320,
  header: 28,
  options: 48,
  footer: 24,
  gap: 4,
  stepW: 54,
  stepH: 56,
  padMini: 40,
  knob: 54,
};

// ---- Colors / bevel ----
const clr = {
  window: "#d9d9d9",
  borderDark: "#7f7f7f",
  borderDarker: "#5a5a5a",
  borderLight: "#ffffff",
  text: "#000000",
};
const pastel = ["#FDE2E4","#E2F0CB","#CDE7F0","#EAD5E6","#FFF1BA","#E2ECF9","#F3D1DC","#D6F4D6","#EAD9C8","#D9E8E3","#E6D5F7","#FDE7C8","#D2E3FC","#F3E8FF","#E5FAD6","#FEE4E2"];
const bevelOut = ()=>({ boxShadow:`inset -1px -1px 0 ${clr.borderDark}, inset 1px 1px 0 ${clr.borderLight}, inset -2px -2px 0 ${clr.borderDarker}, inset 2px 2px 0 ${clr.window}`});
const bevelIn  = ()=>({ boxShadow:`inset -1px -1px 0 ${clr.borderLight}, inset 1px 1px 0 ${clr.borderDark}, inset -2px -2px 0 ${clr.window}, inset 2px 2px 0 ${clr.borderDarker}`});

// ---- Small controls ----
function Btn({label, active, onClick, title, w, h, tiny}){
  return (
    <button title={title} onClick={onClick}
      className={`select-none ${tiny?"px-2 py-[1px] text-[10px]":"px-2 py-[3px] text-[11px]"}`}
      style={{ background: clr.window, color: clr.text, ...(active?bevelIn():bevelOut()), width:w, height:h }}>
      {label}
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

function describeArc(x, y, radius, startAngle, endAngle){
  const start = polarToCartesian(x,y,radius,endAngle);
  const end = polarToCartesian(x,y,radius,startAngle);
  const largeArc = endAngle - startAngle <= 180 ? 0 : 1;
  return ["M",start.x,start.y,"A",radius,radius,0,largeArc,0,end.x,end.y].join(" ");
}
function polarToCartesian(cx, cy, r, angle){ const a = angle*Math.PI/180; return {x:cx+r*Math.cos(a), y:cy+r*Math.sin(a)}; }

function Knob({ label, value, onChange }){
  const box = UI.knob;
  return (
    <div className="flex flex-col items-center gap-1" style={{ width: box+8 }}>
      <div className="relative" style={{ width: box, height: box, background: clr.window, ...bevelOut(), borderRadius: 6 }}>
        <svg viewBox="0 0 100 100" className="absolute inset-1">
          <circle cx="50" cy="50" r="44" fill={clr.window} stroke={clr.borderDarker} strokeWidth="2" />
          <path d={`M50 6 A 44 44 0 1 1 49.99 6`} fill="none" stroke={clr.borderDark} strokeWidth="3" strokeLinecap="round"/>
          <path d={describeArc(50,50,44,-140,-140+value*280)} fill="none" stroke={clr.text} strokeWidth="3" strokeLinecap="round"/>
          {(()=>{const ang=(-140+value*280)*Math.PI/180; const x=50+Math.cos(ang)*34; const y=50+Math.sin(ang)*34; return <line x1="50" y1="50" x2={x} y2={y} stroke={clr.text} strokeWidth="3" strokeLinecap="round"/>})()}
        </svg>
        <input type="range" min={0} max={1} step={0.01} value={value} onChange={(e)=>onChange(Number(e.target.value))} className="absolute inset-0 opacity-0 cursor-pointer"/>
      </div>
      <div className="text-[10px]" style={{ color: clr.text }}>{label}</div>
    </div>
  );
}

// ---- Demo content ----
const patternLabels = "ABCDEFGHIJKLMNOP".split("");
const kitNames = ["Kick","Snare","CHat","OHat","LTom","MTom","HTom","Clap","Rim","Ride","Crash","Perc1","Perc2","Shkr","Cowb","FX"];
const noteNames = ["C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3","C4","C#4","D4","D#4","E4"];

export default function GrooveboxWide(){
  // Transport & globals
  const [play, setPlay] = useState(false);
  const [write, setWrite] = useState(false);
  const [bpm, setBpm]   = useState(128);
  const [swing, setSwing] = useState(56);
  const [quant, setQuant] = useState(50);
  const [pattern, setPattern] = useState(0);

  // Modes & state
  const [mode, setMode] = useState("STEP"); // STEP|PAD|FX|MIX|SEQ|CHOP
  const [instrType, setInstrType] = useState('KIT'); // KIT|CHROMA|SLICED
  const [armedInstrIx, setArmedInstrIx] = useState(3);
  const [patternLength, setPatternLength] = useState(16);
  const pages = useMemo(()=> Array.from({length: Math.ceil(patternLength/16)}, (_,i)=>i), [patternLength]);
  const [page, setPage] = useState(0);

  // Steps
  const [steps, setSteps] = useState(Array(16).fill(false));
  const [stepNotes, setStepNotes] = useState(Array(16).fill(0).map(()=>[]));
  const [stepMeta, setStepMeta] = useState(Array.from({length:16}, ()=>({ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false })));
  const [editingStep, setEditingStep] = useState(null);
  const holdTimer = useRef(null);

  // Stamps & Tools
  const [stamps, setStamps] = useState({ accent:false, ratchet:0, tie:false, micro:0 });
  const [tool, setTool] = useState(null); // COPY|CUT|DELETE|NUDGE|null
  const [scope, setScope] = useState('STEP'); // STEP|INSTR|PATT

  // Shift / chords
  const [shiftHeld, setShiftHeld] = useState(false);
  const [shiftLatched, setShiftLatched] = useState(false);
  const shiftActive = shiftHeld || shiftLatched;

  // Overlays
  const [ovSound, setOvSound] = useState(false);
  const [ovPattern, setOvPattern] = useState(false);
  const [ovClone, setOvClone] = useState(false);
  const [ovFX, setOvFX] = useState(false);
  const [ovScene, setOvScene] = useState(false);
  const [ovInst, setOvInst] = useState(false);
  const [toast, setToast] = useState(null);

  // FX state
  const [fx, setFx] = useState({ type:"Stutter", wet:0.2, rate:0.5, latched:false, repeat:"1/16"});

  function showToast(msg){ setToast(msg); window.setTimeout(()=>setToast(null), 1100); }

  function currentSubLabel(){
    if(instrType==='KIT') return kitNames[0];
    if(instrType==='CHROMA') return noteNames[0];
    return 'S1';
  }

  function placeOrToggleStep(i){
    if(tool){
      if(tool==='DELETE'){
        const arr = steps.slice(); const notes = stepNotes.map(n=> n.slice()); const meta = stepMeta.map(m=>({...m}));
        arr[i]=false; notes[i]=[]; meta[i]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false };
        setSteps(arr); setStepNotes(notes); setStepMeta(meta); return;
      }
      if(tool==='NUDGE'){
        const meta = stepMeta.map((m,ix)=> ix===i? {...m, micro: Math.max(-60, Math.min(60, m.micro + 15)) } : m);
        setStepMeta(meta); return;
      }
      if(tool==='COPY' || tool==='CUT'){
        const src = steps.findIndex(v=>v);
        if(src>=0){
          const arr = steps.slice(); const notes = stepNotes.map(n=> n.slice()); const meta = stepMeta.map(m=>({...m}));
          arr[i]=true; notes[i]=[...(stepNotes[src]||[])]; meta[i]={...stepMeta[src]};
          setSteps(arr); setStepNotes(notes); setStepMeta(meta);
          if(tool==='CUT'){
            const a2 = steps.slice(); const n2 = stepNotes.map(n=> n.slice()); const m2 = stepMeta.map(m=>({...m}));
            a2[src]=false; n2[src]=[]; m2[src]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false };
            setSteps(a2); setStepNotes(n2); setStepMeta(m2);
          }
        } else { showToast('Copy: no source (mock uses first active step).'); }
        return;
      }
    }
    const arr = steps.slice(); const notesArr = stepNotes.map(n=> n.slice()); const metaArr = stepMeta.map(m=> ({...m}));
    if(!arr[i]){
      arr[i]=true; notesArr[i] = [instrType==='KIT'? kitNames[0] : instrType==='CHROMA'? noteNames[0] : 'S1'];
      metaArr[i] = { lenSteps:1, micro: stamps.micro===1?30:(stamps.micro===-1?-30:0), ratchet: stamps.ratchet? stamps.ratchet+1:1, tie: stamps.tie, accent: stamps.accent };
    } else { arr[i]=false; notesArr[i]=[]; metaArr[i]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false }; }
    setSteps(arr); setStepNotes(notesArr); setStepMeta(metaArr);
  }

  // ---- Option line behavior ----
  function triggerOption(ix){
    switch(ix){
      case 0: // INST
        setOvInst(true); break;
      case 1: // SOUND
        setOvSound(true); break;
      case 2: // PATT
        setOvPattern(true); break;
      case 3: // CLONE
        setOvClone(true); break;
      case 4: setTool('COPY'); break;
      case 5: setTool('CUT'); break;
      case 6: setTool('DELETE'); break;
      case 7: setTool('NUDGE'); break;
      case 8: setStamps(s=>({...s, accent: !s.accent})); break;
      case 9: setStamps(s=>({...s, ratchet: (s.ratchet+1)%4 })); break;
      case 10: setStamps(s=>({...s, tie: !s.tie})); break;
      case 11: setStamps(s=>({...s, micro: s.micro===0?1:(s.micro===1?-1:0)})); break;
      case 12: setOvFX(true); break;
      case 13: showToast(`Quantize ${quant}%`); break;
      case 14: setSwing(v=> Math.max(45, Math.min(75, v + (v%2?1:-1)))); break;
      case 15: setOvScene(true); break;
      default: break;
    }
  }

  // ---- Header ----
  const Header = (
    <div style={{ height:UI.header, background:clr.window, ...bevelOut() }} className="w-full flex items-center justify-between px-2">
      <div className="flex items-center gap-1">
        <Btn tiny label={play?"■":"▶"} active={play} onClick={()=>{
          if(shiftActive){ setBpm(b=> Math.max(60, Math.min(200, b+1))); showToast('Tap tempo'); }
          else setPlay(p=>!p);
        }}/>
        <Btn tiny label="REC" onClick={()=>{ if(shiftActive){ setOvInst(true);} else { setMode('CHOP'); showToast('REC→CHOP'); } }}/>
        <Btn tiny label={write?"WRITE*":"WRITE"} active={write} onClick={()=>{ if(shiftActive){ setMode(m=> m==='STEP'?'PAD':'STEP'); } else setWrite(w=>!w); }}/>
        <Btn tiny label="SHIFT" active={shiftActive}
             onMouseDown={()=>setShiftHeld(true)} onMouseUp={()=>setShiftHeld(false)} onDoubleClick={()=>setShiftLatched(v=>!v)}/>
        <Btn tiny label={`Pat ${patternLabels[pattern]}`} onClick={()=>setOvPattern(true)} />
        <Btn tiny label={`BPM ${bpm}`} onClick={()=>setBpm(b=>b+1)} />
        <Btn tiny label={`Sw ${swing}%`} onClick={()=>{}} />
        <Btn tiny label={`Q ${quant}%`} onClick={()=>{}} />
      </div>
      <div className="flex items-center gap-2">
        <TinyField label="CPU" value="14%"/>
        <TinyField label="V" value="9"/>
      </div>
    </div>
  );

  // ---- Option Line ----
  const optLabels = ["INST","SOUND","PATT","CLONE","COPY","CUT","DEL","NUDGE","ACC","RATCH","TIE","MICRO","FX","QNT","SWG","SCENE"];
  const optActive  = [false,false,false,false, tool==='COPY', tool==='CUT', tool==='DELETE', tool==='NUDGE', stamps.accent, stamps.ratchet>0, stamps.tie, stamps.micro!==0, false, false, false, false];
  const OptionLine = (
    <div style={{ height:UI.options, background:clr.window, ...bevelOut() }} className="w-full flex items-center px-2" >
      <div className="grid" style={{ gridTemplateColumns:'repeat(16, 1fr)', gap:UI.gap, width:'100%' }}>
        {optLabels.map((L,ix)=> (
          <button key={L} title={L}
            onClick={()=>triggerOption(ix)}
            className="text-[10px]"
            style={{ background:clr.window, ...(optActive[ix]?bevelIn():bevelOut()), padding:'4px 4px' }}>
            {L}
          </button>
        ))}
      </div>
    </div>
  );

  // ---- View (compact) ----
  function ViewArea(){
    const h = UI.H - UI.header - UI.options - UI.footer - (UI.stepH + 6); // remaining space above step row
    const accentColor = pastel[armedInstrIx % pastel.length];
    if(mode==='STEP'){
      return (
        <div className="px-2 pt-1" style={{ height:h }}>
          <div className="flex items-center justify-between mb-1">
            <div className="text-[10px]" style={{ color: clr.text }}>STEP — I{armedInstrIx+1} ({instrType}) — {currentSubLabel()}</div>
            <div className="flex items-center gap-1">
              {pages.map(p=> <div key={p} onClick={()=>setPage(p)} style={{ width:6, height:6, borderRadius:3, background:p===page?clr.text:clr.borderDark }}/>) }
            </div>
          </div>
          {/* Compact macro strip */}
          <div className="flex items-center gap-3">
            <Knob label="M1" value={0.35} onChange={()=>{}}/>
            <Knob label="M2" value={0.2} onChange={()=>{}}/>
            <Knob label="M3" value={0.5} onChange={()=>{}}/>
            <Knob label="M4" value={0.1} onChange={()=>{}}/>
            <div className="ml-auto text-[10px]" style={{ color: clr.borderDarker }}>Scope: {tool?scope:'—'}</div>
          </div>
        </div>
      );
    }
    if(mode==='PAD'){
      return (
        <div className="px-2 pt-1" style={{ height:h }}>
          <div className="flex items-center justify-between mb-1">
            <div className="text-[10px]" style={{ color: clr.text }}>PAD — I{armedInstrIx+1} — Macros</div>
            <button className="text-[10px] px-2" style={{ background:clr.window, ...bevelOut() }} onClick={()=>setOvInst(true)}>Setup…</button>
          </div>
          <div className="flex items-center gap-3">
            <Knob label="Cutoff" value={0.35} onChange={()=>{}}/>
            <Knob label="Res" value={0.2} onChange={()=>{}}/>
            <Knob label="Env" value={0.5} onChange={()=>{}}/>
            <Knob label="Vib" value={0.1} onChange={()=>{}}/>
            <div className="ml-auto" style={{ width: 220, height: 80, background: clr.window, ...bevelIn(), position:'relative' }}>
              <div className="absolute inset-0" style={{ background: accentColor, opacity: 0.15 }} />
            </div>
          </div>
        </div>
      );
    }
    if(mode==='FX'){
      return (
        <div className="px-2 pt-1" style={{ height:h }}>
          <div className="text-[10px] mb-1" style={{ color: clr.text }}>FX — {fx.type}</div>
          <div className="flex items-center gap-3">
            <Knob label="Rate" value={fx.rate} onChange={(v)=>setFx({...fx, rate:v})}/>
            <Knob label="Wet" value={fx.wet} onChange={(v)=>setFx({...fx, wet:v})}/>
            <button className="text-[10px] px-2" style={{ background:clr.window, ...(fx.latched?bevelIn():bevelOut()) }} onClick={()=>setFx({...fx, latched:!fx.latched})}>Latch</button>
          </div>
        </div>
      );
    }
    if(mode==='CHOP'){
      return (
        <div className="px-2 pt-1" style={{ height:h }}>
          <div className="flex items-center justify-between mb-1">
            <div className="text-[10px]" style={{ color: clr.text }}>CHOP — Sample_01.wav</div>
            <div className="flex items-center gap-1">
              <Btn tiny label="Normalize"/>
              <Btn tiny label="Snap"/>
            </div>
          </div>
          <div className="relative" style={{ height: 90, background: clr.window, ...bevelIn() }}>
            <div className="absolute inset-0" style={{backgroundImage:"repeating-linear-gradient(90deg, rgba(0,0,0,0.2) 0 1px, transparent 1px 6px)"}}/>
            <div className="absolute top-0 bottom-0 left-[15%]" style={{ width:2, background: clr.text }}/>
            <div className="absolute top-0 bottom-0 left-[48%]" style={{ width:2, background: clr.text }}/>
            <div className="absolute top-0 bottom-0 left-[80%]" style={{ width:2, background: clr.text }}/>
          </div>
        </div>
      );
    }
    return <div className="px-2" style={{ height:h }} />
  }

  // ---- Bottom Step Row ----
  function StepRow(){
    const accentColor = pastel[armedInstrIx % pastel.length];
    return (
      <div className="px-2 pb-1" style={{ height: UI.stepH+6 }}>
        <div className="flex items-center gap-[4px]">
          {Array.from({length:16}).map((_,i)=> (
            <button key={i}
              onClick={()=>placeOrToggleStep(i)}
              onMouseDown={()=>{ if(holdTimer.current) clearTimeout(holdTimer.current); holdTimer.current = window.setTimeout(()=> setEditingStep(i), 450); }}
              onMouseUp={()=>{ if(holdTimer.current) { clearTimeout(holdTimer.current); holdTimer.current=null; } }}
              onMouseLeave={()=>{ if(holdTimer.current) { clearTimeout(holdTimer.current); holdTimer.current=null; } }}
              className="relative flex items-center justify-center"
              style={{ width:UI.stepW, height:UI.stepH, background:clr.window, ...(steps[i]?bevelIn():bevelOut()) }}>
              <div className="absolute inset-0" style={{ background: accentColor, opacity: steps[i]?0.25:0.1 }} />
              <div className="absolute" style={{ top:3, left:4, fontSize:9, color:clr.text }}>{i+1+page*16}</div>
              {steps[i] && (
                <div className="font-semibold" style={{ fontSize:10, color:clr.text }}>
                  {(stepNotes[i]||[]).length===1? stepNotes[i][0] : (stepNotes[i]||[]).length>1? `Chord ${(stepNotes[i]||[]).length}`:''}
                </div>
              )}
              {stepMeta[i].accent && <div className="absolute" style={{ top:3, right:4, fontSize:9, color: clr.text }}>›</div>}
              {stepMeta[i].tie    && <div className="absolute left-1 right-1" style={{ bottom:3, height:2, background: clr.text }}/>} 
              {stepMeta[i].ratchet>1 && <div className="absolute" style={{ bottom:3, right:4, fontSize:9, color: clr.text }}>×{stepMeta[i].ratchet}</div>}
              {stepMeta[i].micro!==0 && <div className="absolute" style={{ top:3, right:16, width:5, height:5, background: stepMeta[i].micro>0? clr.text : clr.borderDark }} />}
            </button>
          ))}
        </div>
      </div>
    );
  }

  // ---- Footer ----
  const Footer = (
    <div style={{ height:UI.footer, background:clr.window, ...bevelOut() }} className="w-full flex items-center justify-between px-2">
      <div className="flex items-center gap-2">
        <span className="text-[9px]" style={{ color: clr.borderDarker }}>Stamps:</span>
        <Btn tiny label="Accent" active={stamps.accent} onClick={()=>setStamps(s=>({...s,accent:!s.accent}))}/>
        <Btn tiny label={`Rat ${stamps.ratchet===0?"off":"×"+(stamps.ratchet+1)}`} active={stamps.ratchet>0} onClick={()=>setStamps(s=>({...s,ratchet:(s.ratchet+1)%4}))}/>
        <Btn tiny label="Tie" active={stamps.tie} onClick={()=>setStamps(s=>({...s,tie:!s.tie}))}/>
        <Btn tiny label={`Micro ${stamps.micro===0?"0":(stamps.micro===1?"+25%":"-25%")}`} active={stamps.micro!==0} onClick={()=>setStamps(s=>({...s, micro: s.micro===0?1:(s.micro===1?-1:0)}))}/>
      </div>
      <div className="text-[9px]" style={{ color: clr.borderDarker }}>{tool?`Tool: ${tool} (${scope}) — PLAY to exit`:'—'}</div>
    </div>
  );

  // ---- Overlays ----
  function OverlayFrame({ children, onClose, w='86%' }){
    return (
      <div className="absolute inset-0 z-30 flex items-center justify-center" style={{ background:'rgba(0,0,0,0.35)' }}>
        <div className="relative" style={{ width: w, background: clr.window, ...bevelOut(), padding: 8 }}>
          {children}
          <button className="absolute top-1 right-1 text-[10px] px-2" style={{ background: clr.window, ...bevelOut() }} onClick={onClose}>Close</button>
        </div>
      </div>
    );
  }

  function SoundOverlay(){ if(!ovSound) return null; return (
    <OverlayFrame onClose={()=>setOvSound(false)}>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>Select Instrument I1–I16</div>
      <div className="grid" style={{ gridTemplateColumns:'repeat(16, 1fr)', gap:UI.gap }}>
        {Array.from({length:16}).map((_,i)=> (
          <div key={i} className="h-8 flex items-center justify-center text-[10px] cursor-pointer" style={{ background: clr.window, ...(armedInstrIx===i?bevelIn():bevelOut()) }} onClick={()=>{ setArmedInstrIx(i); setOvSound(false); }}>I{i+1}</div>
        ))}
      </div>
    </OverlayFrame>
  ); }

  function PatternOverlay(){ if(!ovPattern) return null; return (
    <OverlayFrame onClose={()=>setOvPattern(false)}>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>Select Pattern A–P</div>
      <div className="grid" style={{ gridTemplateColumns:'repeat(16, 1fr)', gap:UI.gap }}>
        {patternLabels.map((L,ix)=> (
          <div key={L} className="h-8 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...(pattern===ix?bevelIn():bevelOut()) }} onClick={()=>{ setPattern(ix); setOvPattern(false); }}>{L}</div>
        ))}
      </div>
      <div className="mt-2 flex items-center gap-2">
        <Btn tiny label={`Len ${patternLength}`} onClick={()=>{ const order=[16,32,48,64]; const ix=order.indexOf(patternLength); setPatternLength(order[(ix+1)%order.length]); }}/>
        <div className="text-[10px]" style={{ color: clr.borderDarker }}>Pages: {pages.length}</div>
      </div>
    </OverlayFrame>
  ); }

  function CloneOverlay(){ if(!ovClone) return null; return (
    <OverlayFrame onClose={()=>setOvClone(false)}>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>Clone → Pick destination A–P</div>
      <div className="grid" style={{ gridTemplateColumns:'repeat(16, 1fr)', gap:UI.gap }}>
        {patternLabels.map((L,ix)=> (
          <div key={L} className="h-8 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...bevelOut() }} onClick={()=>{ setPattern(ix); setOvClone(false); showToast(`Cloned to ${L} & queued`); }}>{L}</div>
        ))}
      </div>
    </OverlayFrame>
  ); }

  function FxOverlay(){ if(!ovFX) return null; return (
    <OverlayFrame onClose={()=>setOvFX(false)}>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>FX / Repeat</div>
      <div className="grid grid-cols-4" style={{ gap:UI.gap }}>
        {["Stutter","Tape","Filter","Delay"].map(name=> (
          <div key={name} className="h-8 flex items-center justify-center text-[10px] cursor-pointer" style={{ background: clr.window, ...(fx.type===name?bevelIn():bevelOut()) }} onClick={()=>setFx({...fx, type:name})}>{name}</div>
        ))}
      </div>
      <div className="mt-2 flex flex-wrap" style={{ gap:UI.gap }}>
        {["1/4","1/8","1/8T","1/16","1/16T","1/32","1/32T","1/64"].map(r=> (
          <Btn key={r} tiny label={r} onClick={()=>setFx({...fx, repeat:r})} />
        ))}
      </div>
      <div className="mt-2 flex items-center gap-2">
        <Btn tiny label={fx.latched?"Latched":"Latch"} active={fx.latched} onClick={()=>setFx({...fx, latched:!fx.latched})}/>
        <Btn tiny label="Bake…" onClick={()=>{ showToast('FX baked'); setOvFX(false); }}/>
      </div>
    </OverlayFrame>
  ); }

  function SceneOverlay(){ if(!ovScene) return null; return (
    <OverlayFrame onClose={()=>setOvScene(false)} w='60%'>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>Scene Snapshot — A–D</div>
      <div className="grid grid-cols-4" style={{ gap:UI.gap }}>
        {["A","B","C","D"].map(s=> (
          <div key={s} className="h-8 flex items-center justify-center cursor-pointer" style={{ background: clr.window, ...bevelOut() }} onClick={()=>{ showToast(`Scene ${s} recalled/saved`); setOvScene(false); }}>{s}</div>
        ))}
      </div>
    </OverlayFrame>
  ); }

  function InstOverlay(){ if(!ovInst) return null; return (
    <OverlayFrame onClose={()=>setOvInst(false)}>
      <div className="text-[10px] mb-1" style={{ color: clr.text }}>Instrument Setup — I{armedInstrIx+1}</div>
      <div className="grid grid-cols-3" style={{ gap:UI.gap }}>
        <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
          <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Engine</div>
          <div className="grid grid-cols-2" style={{ gap:UI.gap }}>
            {["Analog","Wavetable","FM","Granular","Drum","Sampler","Slice","Chrom"].map(e=> (
              <Btn key={e} tiny label={e} onClick={()=>{}}/>
            ))}
          </div>
          <div className="text-[9px] mt-2 mb-1" style={{ color: clr.borderDarker }}>Preset</div>
          <div className="h-20 overflow-auto text-[10px]" style={{ background: clr.window, ...bevelIn(), padding:4 }}>
            {["Basic Kit","808","909","Glass Keys","Wobble","Pluck 01"].map(p=> (
              <div key={p} className="px-1 py-[2px] cursor-pointer hover:opacity-80">{p}</div>
            ))}
          </div>
        </div>
        <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
          <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Quick Params</div>
          <div className="flex flex-wrap items-center" style={{ gap:8 }}>
            <Knob label="Tone" value={0.5} onChange={()=>{}}/>
            <Knob label="Drive" value={0.2} onChange={()=>{}}/>
            <Knob label="Env" value={0.3} onChange={()=>{}}/>
            <Knob label="Atk" value={0.05} onChange={()=>{}}/>
            <Knob label="Dec" value={0.4} onChange={()=>{}}/>
          </div>
        </div>
        <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
          <div className="text-[9px] mb-1" style={{ color: clr.borderDarker }}>Macro Map</div>
          <div className="grid grid-cols-2 text-[10px]" style={{ gap: UI.gap }}>
            {['M1','M2','M3','M4'].map(k=> (
              <div key={k} className="flex items-center justify-between">
                <div>{k}</div>
                <select className="text-[10px] px-1 py-[1px]">
                  <option>Cutoff</option>
                  <option>Resonance</option>
                  <option>Env Amt</option>
                  <option>Vibrato</option>
                  <option>Start</option>
                  <option>Drive</option>
                </select>
              </div>
            ))}
          </div>
          <div className="mt-2 flex items-center gap-2">
            <Btn tiny label="Poly"/>
            <div className="flex items-center gap-1"><span className="text-[10px]">Glide</span><input type="range" min={0} max={1} step={0.01}/></div>
          </div>
          <div className="mt-2 flex items-center gap-2">
            <Btn tiny label="Load WAV…"/>
            <Btn tiny label="Edit in CHOP…" onClick={()=>{ setMode('CHOP'); setOvInst(false); }}/>
          </div>
        </div>
      </div>
    </OverlayFrame>
  ); }

  function StepEditor(){ if(editingStep===null) return null; const i=editingStep; const meta = stepMeta[i]; const setMeta = (m)=>{ const arr = stepMeta.map((x,ix)=> ix===i? {...x, ...m} : x); setStepMeta(arr);};
    return (
      <OverlayFrame onClose={()=>setEditingStep(null)} w='420px'>
        <div className="text-[10px] mb-1" style={{ color: clr.text }}>Step {i+1} — Editor</div>
        <div className="grid grid-cols-2" style={{ gap:UI.gap }}>
          <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
            <div className="text-[9px] mb-1">Length</div>
            <input type="range" min={1} max={8} step={1} value={meta.lenSteps} onChange={(e)=>setMeta({lenSteps:Number(e.target.value)})}/>
          </div>
          <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
            <div className="text-[9px] mb-1">Micro</div>
            <input type="range" min={-60} max={60} step={1} value={meta.micro} onChange={(e)=>setMeta({micro:Number(e.target.value)})}/>
          </div>
          <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
            <div className="text-[9px] mb-1">Ratchet</div>
            <div className="flex items-center gap-2">
              {[1,2,3,4].map(r=> <Btn key={r} tiny label={`×${r}`} active={meta.ratchet===r} onClick={()=>setMeta({ratchet:r})}/>) }
            </div>
          </div>
          <div style={{ background: clr.window, ...bevelOut(), padding:6 }}>
            <div className="text-[9px] mb-1">Flags</div>
            <div className="flex items-center gap-2">
              <Btn tiny label="Tie" active={meta.tie} onClick={()=>setMeta({tie:!meta.tie})}/>
              <Btn tiny label="Accent" active={meta.accent} onClick={()=>setMeta({accent:!meta.accent})}/>
            </div>
          </div>
        </div>
        <div className="mt-2 flex items-center justify-between">
          <Btn tiny label="Delete Step" onClick={()=>{ const arr=steps.slice(); const n=stepNotes.map(x=>x.slice()); const m=stepMeta.slice(); arr[i]=false; n[i]=[]; m[i]={ lenSteps:1, micro:0, ratchet:1, tie:false, accent:false}; setSteps(arr); setStepNotes(n); setStepMeta(m); setEditingStep(null); }}/>
          <Btn tiny label="Done" onClick={()=>setEditingStep(null)}/>
        </div>
      </OverlayFrame>
    );
  }

  return (
    <div className="w-full min-h-screen flex items-center justify-center" style={{ background: "#c0c0c0" }}>
      <div className="relative" style={{ width: UI.W, height: UI.H, background: clr.window, ...bevelOut(), outline:'1px solid #000', outlineOffset:'-1px', fontFamily:'Geneva, Verdana, sans-serif' }}>
        {Header}
        {OptionLine}
        <ViewArea/>
        <StepRow/>
        {Footer}

        {/* Overlays */}
        <SoundOverlay/>
        <PatternOverlay/>
        <CloneOverlay/>
        <FxOverlay/>
        <SceneOverlay/>
        <InstOverlay/>
        <StepEditor/>

        {toast && (
          <div className="absolute bottom-[38%] left-1/2 -translate-x-1/2 z-50 px-3 py-1 text-[10px]" style={{ background: clr.window, ...bevelOut(), color: clr.text }}>{toast}</div>
        )}
      </div>
    </div>
  );
}
