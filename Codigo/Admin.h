#pragma once
// Admin.h — Panel de administración (mobile-first, estética base preservada)
// Ahora: fetch inicial + WebSocket (puerto 81) para actualizaciones instantáneas.
// El botón “Reset Aforo” sigue funcionando igual y, además, el firmware hará broadcast por WS.

const char ADMIN_page[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover" />
<title>Admin · Anfiteatro</title>
<style>
  :root{
    --bg:#0b1022; --card:#151a33; --txt:#e7eaf3; --muted:#97a0b5;
    --accent:#7c9cf4; --accent2:#7cdbf4; --good:#40d07b; --warn:#f2c84b;
    --ring:#22294a; --stroke:#273157; --danger:#ef4444; --danger2:#dc2626;
  }
  *{box-sizing:border-box}
  html,body{margin:0;background:var(--bg);color:var(--txt);font-family:system-ui,-apple-system,Segoe UI,Roboto}
  header{position:sticky;top:0;z-index:5;background:#111533;border-bottom:1px solid var(--stroke)}
  .hwrap{padding:12px 16px;display:flex;align-items:center;gap:10px;justify-content:space-between}
  .title{font-size:18px;font-weight:700;color:#a9c1ff}
  .pill{display:inline-flex;align-items:center;gap:8px;padding:6px 10px;border:1px solid var(--stroke);border-radius:999px;background:#10142d;font-size:13px;color:#cdd5ea}
  .dot{width:10px;height:10px;border-radius:50%}
  .dot-open{background:var(--good)} .dot-func{background:var(--accent)} .dot-wait{background:var(--warn)}
  .wrap{max-width:860px;margin:auto;padding:12px}
  .card{background:var(--card);border-radius:16px;padding:14px;margin-bottom:12px;box-shadow:0 6px 26px rgba(0,0,0,.28)}
  h2{margin:0 0 8px 0;font-size:16px;color:#cfe0ff}
  .row{display:flex;align-items:center;justify-content:space-between;gap:10px}
  .kpi{display:flex;align-items:baseline;gap:6px}
  .big{font-size:30px;font-weight:800}
  .unit{font-size:12px;color:var(--muted)}
  .meter{height:12px;background:var(--ring);border-radius:999px;overflow:hidden}
  .fill{height:100%;width:0%;background:linear-gradient(90deg,var(--accent),var(--accent2));transition:width .35s ease}
  .grid2{display:grid;grid-template-columns:1fr;gap:10px}
  .pair{display:flex;align-items:center;justify-content:space-between}
  .btn{
    width:100%; padding:14px 16px; border:none; border-radius:14px;
    background:var(--danger); color:#fff; font-weight:800; font-size:16px;
    box-shadow:0 10px 28px rgba(239,68,68,.35); cursor:pointer; transition:filter .2s, transform .06s;
  }
  .btn:active{transform:translateY(1px)}
  .btn:hover{filter:brightness(1.05)}
  .hint{font-size:12px;color:var(--muted);margin-top:6px;text-align:center}
  .ring{width:180px;height:180px;margin:auto;display:grid;place-items:center}
  .ring svg{transform:rotate(-90deg)}
  .ring-box{display:grid;gap:8px;justify-items:center}
  .afn{font-size:40px;font-weight:900}
  .mini{font-size:12px;color:var(--muted)}
  table{width:100%;border-collapse:collapse;margin-top:8px}
  th,td{padding:10px;border-bottom:1px solid var(--stroke);font-size:14px}
  th{text-align:left;color:#cdd6ef}
  @media (min-width:720px){
    .grid2{grid-template-columns:1fr 1fr}
    .ring{width:200px;height:200px}
    .afn{font-size:44px}
  }
</style>
</head>
<body>
<header>
  <div class="hwrap">
    <div class="title">Panel de Administración</div>
    <div class="pill"><span id="dot" class="dot dot-open"></span><span>Fase: <b id="faseTxt">—</b></span> · <span>Queda: <b id="countTxt">00:00</b></span></div>
  </div>
</header>

<div class="wrap">

  <!-- AFORO -->
  <section class="card">
    <h2>Aforo</h2>
    <div class="grid2">
      <div class="ring-box">
        <div class="ring">
          <svg viewBox="0 0 120 120" width="100%" height="100%">
            <circle cx="60" cy="60" r="52" fill="none" stroke="#1a1f3a" stroke-width="12"></circle>
            <circle id="arc" cx="60" cy="60" r="52" fill="none" stroke="url(#grad)" stroke-linecap="round" stroke-width="12" stroke-dasharray="327" stroke-dashoffset="327"></circle>
            <defs>
              <linearGradient id="grad" x1="0" y1="0" x2="1" y2="1">
                <stop offset="0%" stop-color="#5b7cf9"/>
                <stop offset="100%" stop-color="#7cdbf4"/>
              </linearGradient>
            </defs>
          </svg>
        </div>
        <div class="afn"><span id="afAct">0</span>/<span id="afMax2">40</span></div>
        <div class="mini">personas</div>
      </div>

      <div>
        <div class="pair"><span class="mini">Ocupación</span><span class="mini"><b id="afPct">0</b>%</span></div>
        <div class="meter" style="margin-bottom:10px"><div id="afBar" class="fill"></div></div>
        <table>
          <tr><th>Ingresos RFID</th><td id="ingresos">0</td></tr>
          <tr><th>Egresos IR</th><td id="egresos">0</td></tr>
          <tr><th>Cap. máx.</th><td><span id="afMax">40</span></td></tr>
        </table>
      </div>
    </div>
  </section>

  <!-- AMBIENTE -->
  <section class="card">
    <h2>Ambiente</h2>
    <div class="pair"><div class="kpi"><span class="big" id="tempVal">--</span><span class="unit">°C</span></div><div class="mini">Ventiladores: <b id="fansTxt">—</b></div></div>
    <div class="meter" style="margin:6px 0 12px 0"><div id="tBar" class="fill"></div></div>
    <div class="pair"><div class="kpi"><span class="big" id="humVal">--</span><span class="unit">%</span></div><div class="mini">Humedad relativa</div></div>
    <div class="meter"><div id="hBar" class="fill"></div></div>
  </section>

  <!-- ACCIÓN -->
  <section class="card">
    <h2>Acciones</h2>
    <button class="btn" id="btnReset" onclick="resetAforo()">⟲ Reset Aforo</button>
    <div class="hint" id="hint">Restablece ocupación a 0 (ingresos = egresos)</div>
  </section>

</div>

<script>
  const $=id=>document.getElementById(id);
  const arc=$('arc'), CIRC=2*Math.PI*52;
  arc.style.strokeDasharray=String(CIRC); arc.style.strokeDashoffset=String(CIRC);

  function setRing(pct){
    const p=Math.max(0,Math.min(100, pct||0));
    arc.style.strokeDashoffset=String(CIRC*(1-p/100));
    $('afPct').textContent=p.toFixed(0);
    $('afBar').style.width=p+'%';
  }
  function mmss(ms){ const s=Math.max(0,Math.floor(ms/1000)); const m=Math.floor(s/60), r=s%60; return (m<10?'0':'')+m+':' + (r<10?'0':'')+r; }
  function setPhase(fase, ms){
    const dot=$('dot'); let cls='dot-open', name='abierto';
    if(fase==='funcion'){ cls='dot-func'; name='en función'; }
    if(fase==='espera'){ cls='dot-wait'; name='en espera'; }
    dot.className='dot '+cls; $('faseTxt').textContent=name; $('countTxt').textContent=mmss(ms||0);
  }
  function setFans(on){ $('fansTxt').textContent=on?'encendidos':'apagados'; $('fansTxt').style.color=on?'#40d07b':'#97a0b5'; }

  // Render único (fetch inicial + WS)
  function render(d){
    setPhase(d.fase || (d.estado==='en-funcion'?'funcion':'abierto'), d.t_restante_ms||0);

    const act=Number(d.aforo_actual||0), max=Number(d.aforo_max||40);
    $('afAct').textContent=act; $('afMax').textContent=max; $('afMax2').textContent=max;
    setRing(Math.round((act*100)/(max||1)));

    const t=(d.temp!=null)?Number(d.temp):NaN, h=(d.hum!=null)?Number(d.hum):NaN;
    $('tempVal').textContent=isFinite(t)?t.toFixed(1):'--';
    $('humVal').textContent =isFinite(h)?h.toFixed(0):'--';
    $('tBar').style.width=isFinite(t)?Math.max(0,Math.min(100,(t/45)*100))+'%':'0%';
    $('hBar').style.width=isFinite(h)?Math.max(0,Math.min(100,h))+'%':'0%';
    setFans(!!d.fansOn);

    $('ingresos').textContent=d.ingresos ?? 0;
    $('egresos').textContent=d.egresos ?? 0;
  }

  // Fetch inicial
  async function loadOnce(){
    try{ const r=await fetch('/status',{cache:'no-store'}); render(await r.json()); }catch(e){}
  }

  // WebSocket push
  let ws;
  function connectWS(){
    const url = `ws://${location.hostname}:81/`;
    ws = new WebSocket(url);
    ws.onopen = ()=>{ /* conectado */ };
    ws.onclose= ()=>{ setTimeout(connectWS, 1500); };
    ws.onmessage = (ev)=>{ try{ render(JSON.parse(ev.data)); }catch(e){} };
  }

  let resetting=false;
  async function resetAforo(){
    if(resetting) return;
    if(!confirm('¿Resetear aforo a 0?')) return;
    resetting=true;
    const btn=$('btnReset'), hint=$('hint'); const prev=btn.textContent;
    btn.textContent='Reseteando...'; btn.disabled=true;
    try{
      let r=await fetch('/reset',{method:'POST'});
      if(!r.ok){ r=await fetch('/reset'); } // fallback
      if(!r.ok) throw new Error('HTTP '+r.status);
      hint.textContent='Aforo reseteado correctamente.'; setTimeout(()=>hint.textContent='Restablece ocupación a 0 (ingresos = egresos)',3000);
      // El WS empujará el nuevo estado; igual pedimos uno por si acaso
      loadOnce();
    }catch(err){
      hint.textContent='Error: no se pudo resetear (¿existe /reset en el firmware?)';
    }finally{
      btn.textContent=prev; btn.disabled=false; resetting=false;
    }
  }

  loadOnce(); connectWS();
</script>
</body>
</html>
)HTMLPAGE";
