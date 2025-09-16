// Principal.h
#pragma once
static const char MAIN_page[] = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Anfiteatro – Vista Pública (cap=40)</title>
  <style>
    :root{
      --bg:#070b13; --card:#0c1220; --muted:#a9b4ca; --text:#ecf3ff;
      --ok:#22c55e; --warn:#f59e0b; --danger:#ef4444; --accent:#60a5fa; --pink:#f472b6; --vio:#8b5cf6;
    }
    *{box-sizing:border-box}
    html,body{height:100%}
    body{
      margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,Arial,sans-serif;color:var(--text);
      background:radial-gradient(1200px 800px at 10% -10%, #0b1530 0%, transparent 60%),
                 radial-gradient(900px 700px at 110% 20%, #061323 0%, transparent 60%), #070b13;
    }
    a{color:inherit}

    /* HERO */
    .hero{position:relative;min-height:68vh;display:grid;place-items:center;overflow:hidden}
    .hero-bg{position:absolute;inset:0;background:conic-gradient(from 180deg at 50% 50%, #071021, #0e1e3c, #071021);
      filter:blur(60px);opacity:.6;animation:spin 30s linear infinite}
    @keyframes spin{to{transform:rotate(360deg)}}
    .grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px}
    .wrap{max-width:1100px;margin:0 auto;padding:24px 16px}

    .status{position:relative;z-index:1;text-align:center}
    .badge{display:inline-flex;align-items:center;gap:10px;padding:10px 14px;border-radius:999px;
      background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.12);backdrop-filter:blur(6px);
      font-weight:800;letter-spacing:.3px}
    .dot{width:12px;height:12px;border-radius:50%;box-shadow:0 0 18px currentColor}

    .title{font-size:clamp(34px,6vw,64px);margin:14px 0 6px 0;line-height:1.05;
      background:linear-gradient(90deg,var(--text),#b9d2ff);-webkit-background-clip:text;background-clip:text;color:transparent}
    .sub{opacity:.85}

    /* COUNTDOWN (opcional si luego cargas horarios) */
    .countdown{margin-top:18px;display:flex;gap:10px;justify-content:center}
    .cube{min-width:86px;padding:12px;border-radius:16px;background:linear-gradient(180deg,#101a30,#0b1323);
      border:1px solid #213056;text-align:center;box-shadow:0 12px 40px rgba(0,0,0,.35)}
    .cube .num{font-size:32px;font-weight:900}
    .cube .lab{font-size:12px;color:var(--muted)}

    /* CURTAIN */
    .stage{position:relative;margin-top:22px;height:180px;border-radius:18px;border:1px solid #22314e;
      background:linear-gradient(180deg,#0f1b34,#0b1426);overflow:hidden}
    .curtain{position:absolute;inset:0;display:grid;grid-template-columns:1fr 1fr;pointer-events:none}
    .panel{background:repeating-linear-gradient(90deg,#5a0b1f 0 14px,#901b1b 14px 28px);box-shadow:inset -8px 0 16px rgba(0,0,0,.35)}
    .curtain.open .panel{animation:openL 1.2s cubic-bezier(.2,.8,.2,1) forwards}
    .curtain.open .panel:last-child{animation-name:openR}
    @keyframes openL{to{transform:translateX(-110%)}}@keyframes openR{to{transform:translateX(110%)}}
    .stage-floor{position:absolute;left:0;right:0;bottom:0;height:40px;background:linear-gradient(180deg,#23314e,#0a0f18)}

    /* CARDS */
    .cards{margin-top:-60px;position:relative;z-index:2}
    .card{background:linear-gradient(180deg,#0c1220,#0b1020);border:1px solid #1f2e4d;border-radius:18px;padding:16px;
      box-shadow:0 16px 50px rgba(0,0,0,.35)}
    .card h3{margin:0 0 10px 0}
    .desc{color:var(--muted);font-size:12px;margin-top:-2px;margin-bottom:6px}

    /* GAUGE */
    .gaugeWrap{display:grid;place-items:center}
    .gauge{--size:220px;width:var(--size);height:var(--size);border-radius:50%;display:grid;place-items:center;
      background:conic-gradient(var(--ok) 0deg, var(--ok) var(--angle), #1b2742 var(--angle) 360deg);
      position:relative;border:10px solid #0e1629}
    .gauge::after{content:"";position:absolute;inset:14px;border-radius:50%;background:#0a1221;border:1px solid #1c2b49}
    .gauge .val{position:relative;font-size:38px;font-weight:900}
    .gauge .cap{position:absolute;bottom:24px;font-size:12px;color:var(--muted)}

    /* THERMOMETER */
    .thermo{height:220px;width:30px;border-radius:999px;background:#0d1730;border:1px solid #213056;position:relative;overflow:hidden}
    .thermo .col{position:absolute;inset:auto 0 0 0;height:40%;background:linear-gradient(180deg,#3b82f6,#1d4ed8)}
    .thermo .lab{position:absolute;top:-8px;left:42px;color:var(--muted);font-size:12px}

    /* TIMELINE (opcional si usás /horarios) */
    .timeline{list-style:none;padding:0;margin:0}
    .timeline li{display:flex;justify-content:space-between;gap:12px;padding:12px;border:1px solid #213056;border-radius:12px;background:#0d172e}
    .timeline li+li{margin-top:8px}

    /* LEGEND */
    .legend{display:flex;gap:10px;align-items:center;color:var(--muted);font-size:12px;margin-top:8px}
    .legend .dot{width:10px;height:10px;border-radius:50%}

    /* MARQUEE */
    .marquee{overflow:hidden;white-space:nowrap;border-top:1px solid #1f2e4d;border-bottom:1px solid #1f2e4d;background:linear-gradient(90deg,#0c1426,#0a1324)}
    .marquee span{display:inline-block;padding:12px 0;animation:slide 16s linear infinite}
    @keyframes slide{from{transform:translateX(0)}to{transform:translateX(-50%)}}

    .footer{opacity:.7;margin:18px 0 28px 0;font-size:12px;text-align:center}

    /* Confetti */
    #confetti{position:fixed;inset:0;pointer-events:none}
  </style>
</head>
<body>
  <canvas id="confetti"></canvas>
  <section class="hero">
    <div class="hero-bg"></div>
    <div class="status wrap" aria-live="polite">
      <div class="badge" id="chip"><span class="dot" id="dot"></span><span id="estadoTxt">Cargando…</span></div>
      <h1 class="title">Anfiteatro – ¡Bienvenidos!</h1>
      <div class="sub">Capacidad <strong>40</strong> · Actualizado en vivo</div>

      <!-- COUNTDOWN (visible si luego cargas horarios desde /status) -->
      <div class="countdown" id="countdown" aria-label="Cuenta regresiva al próximo evento" hidden>
        <div class="cube"><div class="num" id="cdH">00</div><div class="lab">Horas</div></div>
        <div class="cube"><div class="num" id="cdM">00</div><div class="lab">Min</div></div>
        <div class="cube"><div class="num" id="cdS">00</div><div class="lab">Seg</div></div>
      </div>

      <!-- Escenario con cortina -->
      <div class="stage wrap" style="max-width:900px">
        <div class="curtain" id="curtain">
          <div class="panel"></div>
          <div class="panel"></div>
        </div>
        <div class="stage-floor"></div>
      </div>
    </div>
  </section>

  <!-- Cards -->
  <section class="wrap cards">
    <div class="grid">
      <div class="card" style="grid-column:span 6">
        <h3>Aforo</h3>
        <div class="desc">Personas dentro del anfiteatro en este momento. Capacidad máxima: 40.</div>
        <div class="gaugeWrap">
          <div class="gauge" id="gauge" style="--angle:0deg">
            <div class="val" id="gaugeVal">0%</div>
            <div class="cap">Ocupación</div>
          </div>
        </div>
        <div class="legend"><span class="dot" style="background:var(--ok)"></span> Ocupación actual · <span id="aforoTexto">0 / 40</span></div>
      </div>
      <div class="card" style="grid-column:span 6;display:flex;align-items:center;gap:20px;justify-content:center">
        <div>
          <h3>Temperatura</h3>
          <div class="desc">Medida por los sensores del escenario.</div>
          <div style="display:flex;align-items:center;gap:20px">
            <div class="thermo"><div class="col" id="thermoCol" style="height:40%"></div><div class="lab">Actual: <strong id="tempTxt">-- °C</strong></div></div>
          </div>
        </div>
      </div>

      <!-- Sección de horarios (opcional) -->
      <div class="card" style="grid-column:span 12">
        <h3>Horarios de hoy</h3>
        <div class="desc">Próximas actividades en el anfiteatro.</div>
        <ul class="timeline" id="timeline"></ul>
      </div>
    </div>
  </section>

  <div class="marquee"><span id="ticker">Siga las indicaciones del personal · Puntos de hidratación activos · Accesos por RFID · </span></div>
  <div class="footer">Vista pública – datos desde la maqueta en tiempo real.</div>

  <script>
    const CAPACITY = 40;

    // DOM
    const $ = (id)=>document.getElementById(id);
    const dot=$('dot'), estadoTxt=$('estadoTxt'), curtain=$('curtain'),
          gauge=$('gauge'), gaugeVal=$('gaugeVal'), aforoTexto=$('aforoTexto'),
          thermoCol=$('thermoCol'), tempTxt=$('tempTxt'), timeline=$('timeline');

    function applyStatus(j){
      // Estado → chip + cortina + confetti
      const map = { 'en-funcion': {txt:'En función', color:'#22c55e', open:true},
                    'abierto':    {txt:'Abierto',    color:'#f59e0b', open:false},
                    'cerrado':    {txt:'Cerrado',    color:'#ef4444', open:false} };
      const m = map[j.estado] || map['abierto'];
      dot.style.color = m.color;
      estadoTxt.textContent = m.txt;
      curtain.classList.toggle('open', !!m.open);
      toggleConfetti(j.estado === 'en-funcion');

      // Aforo
      const ocup = Math.max(0, Math.min(CAPACITY, Number(j.aforo_actual||0)));
      const pct = Math.round((ocup / CAPACITY) * 100);
      const angle = Math.round(pct/100*360);
      gauge.style.setProperty('--angle', angle+'deg');
      gaugeVal.textContent = pct + "%";
      aforoTexto.textContent = ocup + " / " + CAPACITY;

      // Temperatura
      const t = Number(j.temp || 0);
      tempTxt.textContent = t.toFixed(1) + " °C";
      const h = Math.max(0, Math.min(100, (t-10)/(45-10)*100));
      thermoCol.style.height = h + "%";

      // Horarios (opcional)
      if (Array.isArray(j.horarios)) renderHorarios(j.horarios);
    }

    // Polling al backend del ESP32
    async function refresh(){
      try{
        const r = await fetch('/status', {cache:'no-store'});
        if(!r.ok) throw new Error('HTTP ' + r.status);
        const j = await r.json();
        applyStatus(j);
      }catch(e){
        console.error(e);
      }
    }
    setInterval(refresh, 1000);
    refresh();

    // Horarios (opcional). Si no vienen en /status, se deja vacío.
    function renderHorarios(items){
      timeline.innerHTML = '';
      (items||[]).forEach(h=>{
        const li = document.createElement('li');
        li.innerHTML = '<span>' + (h.titulo||'Evento') + '</span><strong>' + (h.hora||'--:--') + '</strong>';
        timeline.appendChild(li);
      });
      setupCountdown(items||[]);
    }

    // Countdown al próximo horario (solo si hay lista)
    function parseTodayTime(hhmm){
      const p = (hhmm||'--:--').split(':'), h=Number(p[0]||0), m=Number(p[1]||0);
      const d=new Date(); d.setHours(h,m,0,0); return d;
    }
    function nextUpcoming(items){
      const now = new Date();
      const sorted = (items||[]).map(h=>({ ...h, date: parseTodayTime(h.hora) })).sort((a,b)=>a.date-b.date);
      return sorted.find(e=>e.date>now) || null;
    }
    function setupCountdown(items){
      const target = nextUpcoming(items);
      const cd = document.getElementById('countdown');
      if(!target){ cd.hidden = true; return; }
      cd.hidden = false;
      const tick=()=>{
        const now=new Date();
        let diff = Math.max(0, target.date-now);
        const H = String(Math.floor(diff/3.6e6)).padStart(2,'0');
        diff%=3.6e6; const M=String(Math.floor(diff/6e4)).padStart(2,'0');
        diff%=6e4;   const S=String(Math.floor(diff/1e3)).padStart(2,'0');
        document.getElementById('cdH').textContent=H;
        document.getElementById('cdM').textContent=M;
        document.getElementById('cdS').textContent=S;
      };
      clearInterval(window.__cdInt); window.__cdInt=setInterval(tick,1000); tick();
    }

    // Confetti
    const confettiCanvas = document.getElementById('confetti');
    const ctx = confettiCanvas.getContext('2d');
    let confettiParticles=[]; let confettiRAF=null;
    function resize(){ confettiCanvas.width=innerWidth; confettiCanvas.height=innerHeight }
    window.addEventListener('resize', resize); resize();
    function makeConfetti(){
      const colors=['#60a5fa','#34d399','#f472b6','#f59e0b','#a78bfa'];
      for(let i=0;i<80;i++) confettiParticles.push({
        x:Math.random()*innerWidth,y:-20-Math.random()*innerHeight,vy:2+Math.random()*3,
        vx:(Math.random()-.5)*1.2,size:4+Math.random()*4,color:colors[Math.floor(Math.random()*colors.length)],
        rot:Math.random()*360,vr:(Math.random()-.5)*8
      });
    }
    function drawConfetti(){
      ctx.clearRect(0,0,confettiCanvas.width,confettiCanvas.height);
      confettiParticles.forEach(p=>{
        ctx.save(); ctx.translate(p.x,p.y); ctx.rotate(p.rot*Math.PI/180);
        ctx.fillStyle=p.color; ctx.fillRect(-p.size/2,-p.size/2,p.size,p.size);
        ctx.restore(); p.y+=p.vy; p.x+=p.vx; p.rot+=p.vr; if(p.y>innerHeight+20) p.y=-20;
      });
      confettiRAF=requestAnimationFrame(drawConfetti);
    }
    function toggleConfetti(on){
      if(on){ if(!confettiRAF){ makeConfetti(); drawConfetti(); } }
      else { if(confettiRAF){ cancelAnimationFrame(confettiRAF); confettiRAF=null;
              ctx.clearRect(0,0,confettiCanvas.width,confettiCanvas.height); confettiParticles=[]; } }
    }
  </script>
</body>
</html>
)HTMLPAGE";
