#pragma once
// Principal.h — Página de USUARIO (mobile-first, estética renovada)
// Consume /status: fase ("abierto","funcion","espera"), t_restante_ms, temp, hum, aforo_actual, aforo_max, fansOn

static const char MAIN_page[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover" />
<title>Anfiteatro — Estado</title>
<style>
  :root{
    --bg: #0b1020;
    --grad1:#121a39;
    --grad2:#0f1630;
    --card:#141a33;
    --glass: rgba(255,255,255,.06);
    --txt:#eef1f7;
    --muted:#9aa3b5;
    --accent:#7c9cf4;
    --accent2:#79e2f2;
    --good:#31d491;
    --warn:#f2c84b;
    --ring:#1e2445;
    --stroke:#232a4d;
  }
  *{box-sizing:border-box}
  html,body{margin:0;background:linear-gradient(135deg,var(--grad1),var(--grad2));min-height:100%;color:var(--txt);font-family:system-ui,-apple-system,Segoe UI,Roboto}
  .wrap{max-width:820px;margin:auto;padding:16px}
  header{display:flex;align-items:center;gap:10px;margin-bottom:14px}
  .logo{width:34px;height:34px;border-radius:12px;background:linear-gradient(135deg,#5b7cf9,#7cdbf4);box-shadow:0 10px 30px rgba(124,156,244,.35)}
  h1{font-size:18px;margin:0}
  .sub{font-size:13px;color:var(--muted)}
  .card{
    background:linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
    border:1px solid var(--stroke);
    border-radius:16px;padding:14px;margin-bottom:12px;
    box-shadow:0 8px 28px rgba(0,0,0,.25), inset 0 1px 0 rgba(255,255,255,.05);
    backdrop-filter: blur(6px);
  }
  .row{display:flex;align-items:center;justify-content:space-between;gap:10px}
  .kpi{display:flex;align-items:baseline;gap:6px}
  .big{font-size:30px;font-weight:800}
  .unit{font-size:12px;color:var(--muted)}
  .pill{display:inline-flex;align-items:center;gap:8px;padding:6px 10px;border-radius:999px;background:var(--glass);border:1px solid var(--stroke);font-size:13px;color:#dbe2ff}
  .dot{width:10px;height:10px;border-radius:50%}
  .dot-open{background:var(--good)} .dot-func{background:var(--accent)} .dot-wait{background:var(--warn)}
  .count{font-variant-numeric:tabular-nums}
  .grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  @media (max-width:580px){ .grid2{grid-template-columns:1fr} .big{font-size:26px}}
  /* Ring */
  .ring{width:180px;height:180px;display:grid;place-items:center;margin:auto}
  .ring svg{transform:rotate(-90deg)}
  .ring-wrap{position:relative;display:grid;place-items:center}
  .afn{position:absolute;text-align:center}
  .afn .n{font-size:38px;font-weight:900}
  .afn .lbl{font-size:12px;color:var(--muted)}
  .meter{height:10px;background:var(--ring);border-radius:999px;overflow:hidden}
  .fill{height:100%;width:0%;background:linear-gradient(90deg,var(--accent),var(--accent2));transition:width .35s ease}
  /* Cronograma chips */
  .timeline{display:grid;gap:10px}
  .step{display:flex;align-items:center;gap:10px}
  .badge{min-width:88px;text-align:center;border-radius:12px;padding:6px 10px;font-weight:700;border:1px solid var(--stroke);background:var(--glass)}
  .b-now{border-color:#5b7cf9;color:#cfe1ff}
  .b-next{color:#cdd6ef}
  .b-later{color:#cdd6ef}
  .mini{font-size:12px;color:var(--muted)}
  .progress{height:8px;border-radius:999px;background:var(--ring);overflow:hidden}
  .progress .pfill{height:100%;width:0%;background:linear-gradient(90deg,#5b7cf9,#7cdbf4);transition:width .35s ease}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="logo" aria-hidden="true"></div>
    <div>
      <h1>Anfiteatro — Información en vivo</h1>
      <div class="sub">Actualiza cada 1 s · misma red WiFi</div>
    </div>
  </header>

  <!-- Estado -->
  <section class="card">
    <div class="row">
      <div class="pill">
        <span id="dot" class="dot dot-open"></span>
        <span>Fase: <b id="faseTxt">—</b></span>
      </div>
      <div class="pill"><span>Queda</span> <b class="count" id="countTxt">00:00</b></div>
    </div>
  </section>

  <!-- Aforo + Ambiente -->
  <section class="card">
    <div class="grid2">
      <!-- Aforo -->
      <div>
        <div class="ring-wrap" style="margin-bottom:10px">
          <div class="ring">
            <svg viewBox="0 0 120 120" width="100%" height="100%">
              <circle cx="60" cy="60" r="52" fill="none" stroke="var(--ring)" stroke-width="12"></circle>
              <circle id="afArc" cx="60" cy="60" r="52" fill="none" stroke="url(#grad)" stroke-linecap="round" stroke-width="12" stroke-dasharray="327" stroke-dashoffset="327"></circle>
              <defs>
                <linearGradient id="grad" x1="0" y1="0" x2="1" y2="1">
                  <stop offset="0%" stop-color="#5b7cf9"/>
                  <stop offset="100%" stop-color="#7cdbf4"/>
                </linearGradient>
              </defs>
            </svg>
          </div>
          <div class="afn">
            <div class="n"><span id="afAct">0</span>/<span id="afMax2">40</span></div>
            <div class="lbl">personas</div>
          </div>
        </div>
        <div class="row mini" style="margin-bottom:6px"><span>Ocupación</span><span><b id="afPct">0</b>%</span></div>
        <div class="meter"><div id="afBar" class="fill"></div></div>
      </div>

      <!-- Ambiente -->
      <div>
        <div class="row"><div class="kpi"><span class="big" id="tempVal">--</span><span class="unit">°C</span></div><div class="mini">Ventilación: <b id="fansTxt">—</b></div></div>
        <div class="meter" style="margin:6px 0 10px 0"><div id="tBar" class="fill"></div></div>
        <div class="row"><div class="kpi"><span class="big" id="humVal">--</span><span class="unit">%</span></div><div class="mini">Humedad relativa</div></div>
        <div class="meter"><div id="hBar" class="fill"></div></div>
      </div>
    </div>
  </section>

  <!-- Cronograma (más claro) -->
  <section class="card">
    <div class="row" style="margin-bottom:8px">
      <div class="kpi"><span>📅</span><span>Cronograma</span></div>
      <div class="mini">Rotación automática</div>
    </div>
    <div class="timeline">
      <div class="step">
        <div class="badge b-now"   id="nowBadge">Ahora</div>
        <div>
          <div><b id="nowName">—</b></div>
          <div class="mini">Restan <span class="count" id="nowLeft">00:00</span></div>
          <div class="progress" style="margin-top:6px"><div id="nowProg" class="pfill"></div></div>
        </div>
      </div>
      <div class="step">
        <div class="badge b-next">Luego</div>
        <div><div><b id="nextName">—</b></div><div class="mini">Duración <span class="count" id="nextDur">00:00</span></div></div>
      </div>
      <div class="step">
        <div class="badge b-later">Después</div>
        <div><div><b id="laterName">—</b></div><div class="mini">Duración <span class="count" id="laterDur">00:00</span></div></div>
      </div>
    </div>
  </section>

  <section class="card">
    <div class="mini">Información para el público. Para atención, consulte al personal.</div>
  </section>
</div>

<script>
  // Deben coincidir con el firmware (.ino). Si cambiás allí, actualizá aquí:
  const DUR_ABIERTO = 30000, DUR_FUNCION = 35000, DUR_ESPERA = 30000;

  const $ = id => document.getElementById(id);
  const arc = $('afArc'), CIRC = 2*Math.PI*52;
  arc.style.strokeDasharray = String(CIRC); arc.style.strokeDashoffset = String(CIRC);

  function mmss(ms){ const s=Math.max(0,Math.floor(ms/1000)); const m=Math.floor(s/60), r=s%60; return (m<10?'0':'')+m+':' + (r<10?'0':'')+r; }
  function setRing(pct){
    const p = Math.max(0, Math.min(100, pct||0));
    arc.style.strokeDashoffset = String(CIRC*(1-p/100));
    $('afPct').textContent = p.toFixed(0);
    $('afBar').style.width = p+'%';
  }
  function setPhaseUI(fase, remMs){
    const dot=$('dot'); let cls='dot-open', name='abierto';
    if(fase==='funcion'){ cls='dot-func'; name='en función'; }
    if(fase==='espera'){ cls='dot-wait';  name='en espera'; }
    dot.className = 'dot ' + cls;
    $('faseTxt').textContent = name;
    $('countTxt').textContent = mmss(remMs);
  }
  function setFans(on){ $('fansTxt').textContent = on ? 'encendidos' : 'apagados'; $('fansTxt').style.color = on ? '#31d491' : '#9aa3b5'; }

  // Cronograma
  const order = ['abierto','funcion','espera'];
  const dur = f => (f==='abierto'?DUR_ABIERTO: f==='funcion'?DUR_FUNCION: DUR_ESPERA);
  const label = f => (f==='abierto'?'abierto': f==='funcion'?'en función':'en espera');

  function setTimeline(fase, remMs){
    const idx = order.indexOf(fase);
    const next  = order[(idx+1)%3];
    const later = order[(idx+2)%3];

    $('nowName').textContent  = label(fase);
    $('nowLeft').textContent  = mmss(remMs);
    $('nextName').textContent = label(next);
    $('laterName').textContent= label(later);
    $('nextDur').textContent  = mmss(dur(next));
    $('laterDur').textContent = mmss(dur(later));

    // Progreso de "Ahora"
    const total = dur(fase) || 1;
    const done  = total - (remMs||0);
    const pct   = Math.max(0, Math.min(100, (done/total)*100));
    $('nowProg').style.width = pct + '%';
  }

  async function tick(){
    try{
      const r = await fetch('/status',{cache:'no-store'});
      const d = await r.json();

      const fase = d.fase || (d.estado==='en-funcion'?'funcion':'abierto');
      const rem  = d.t_restante_ms || 0;

      // Estado
      setPhaseUI(fase, rem);
      setTimeline(fase, rem);

      // Aforo
      const act=Number(d.aforo_actual||0), max=Number(d.aforo_max||40);
      $('afAct').textContent = act;
      $('afMax2').textContent = max;
      const pct = Math.round((act*100)/(max||1));
      setRing(isFinite(pct)?pct:0);

      // Sensores
      const t=(d.temp!=null)?Number(d.temp):NaN, h=(d.hum!=null)?Number(d.hum):NaN;
      $('tempVal').textContent = isFinite(t)? t.toFixed(1) : '--';
      $('humVal').textContent  = isFinite(h)? h.toFixed(0) : '--';
      $('tBar').style.width = isFinite(t)? Math.max(0,Math.min(100,(t/45)*100))+'%' : '0%';
      $('hBar').style.width  = isFinite(h)? Math.max(0,Math.min(100,h))+'%' : '0%';
      setFans(!!d.fansOn);
    }catch(e){
      // Silencioso para el público
    }
  }

  tick(); setInterval(tick,1000);
</script>
</body>
</html>
)HTMLPAGE";
