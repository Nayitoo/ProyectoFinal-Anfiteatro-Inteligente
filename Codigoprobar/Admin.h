#pragma once
// Admin.h generado a partir de admintest.html
const char ADMIN_page[] PROGMEM = R"ADMIN(<!-- admin.html -->
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Administraci&oacute;n | Anfiteatro</title>
<style>
  :root{
    --bg:#f2f3f7;
    --text:#1f2433;
    --muted:#5c6272;
    --card:#ffffff;
    --border:#ced3df;
    --accent:#1b4d8a;
    --accent-soft:#e2ecff;
    --good:#1a7f54;
    --warn:#d97706;
    --danger:#c53030;
  }
  *{box-sizing:border-box;}
  body{
    margin:0;
    background:var(--bg);
    color:var(--text);
    font-family:"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    line-height:1.5;
  }
  header{
    background:#1f2433;
    color:#f5f7ff;
    padding:20px clamp(16px,5vw,48px);
    display:flex;
    flex-direction:column;
    gap:16px;
  }
  .header-wrap{
    display:flex;
    justify-content:space-between;
    align-items:flex-start;
    gap:12px;
    flex-wrap:wrap;
  }
  header h1{margin:0;font-size:24px;}
  header p{margin:4px 0 0;font-size:15px;color:#cdd4ff;}
  .home-link{
    color:#f5f7ff;
    text-decoration:none;
    font-weight:600;
    background:rgba(255,255,255,0.1);
    padding:8px 14px;
    border-radius:8px;
    border:1px solid rgba(255,255,255,0.24);
  }
  .state-wrap{
    display:flex;
    flex-wrap:wrap;
    gap:12px;
    align-items:center;
  }
  .badge{
    display:flex;
    align-items:center;
    gap:12px;
    padding:12px 16px;
    border-radius:12px;
    background:var(--card);
    color:var(--text);
    border:1px solid var(--border);
  }
  .badge.phase-abierto{border-color:rgba(26,127,84,0.4);}
  .badge.phase-funcion{border-color:rgba(27,77,138,0.45);}
  .badge.phase-espera{border-color:rgba(217,119,6,0.45);}
  .badge .block{display:flex;flex-direction:column;gap:2px;}
  .badge .block span:first-child{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:0.05em;}
  .badge .block span:last-child{font-size:18px;font-weight:600;}
  .dot{width:12px;height:12px;border-radius:50%;}
  .dot-open{background:var(--good);}
  .dot-func{background:var(--accent);}
  .dot-wait{background:var(--warn);}
  .status-extra{font-size:14px;color:#cfd5f8;}
  main{
    max-width:1080px;
    margin:0 auto;
    padding:24px clamp(16px,5vw,48px);
    display:flex;
    flex-direction:column;
    gap:20px;
  }
  .status-message{
    padding:12px 16px;
    border-radius:10px;
    border:1px solid transparent;
    background:var(--card);
    font-size:14px;
  }
  .status-message[data-state="info"]{border-color:var(--border);}
  .status-message[data-state="success"]{border-color:rgba(26,127,84,0.45);color:var(--good);}
  .status-message[data-state="error"]{border-color:rgba(197,48,48,0.45);color:var(--danger);}
  .panel{
    background:var(--card);
    border-radius:14px;
    border:1px solid var(--border);
    padding:18px 20px;
    display:flex;
    flex-direction:column;
    gap:16px;
  }
  .panel h2{margin:0;font-size:20px;}
  .panel p{margin:0;font-size:14px;color:var(--muted);}
  .grid{
    display:grid;
    gap:16px;
  }
  @media(min-width:720px){
    .grid.two{grid-template-columns:repeat(2,minmax(0,1fr));}
    .grid.three{grid-template-columns:repeat(3,minmax(0,1fr));}
  }
  .summary-card{
    border:1px solid var(--border);
    border-radius:10px;
    padding:14px;
    display:flex;
    flex-direction:column;
    gap:6px;
  }
  .summary-card .label{font-size:13px;color:var(--muted);text-transform:uppercase;letter-spacing:0.05em;}
  .summary-card .value{font-size:22px;font-weight:700;}
  .progress{
    width:100%;
    height:10px;
    border-radius:999px;
    background:#edf0f7;
    overflow:hidden;
  }
  .progress.mini{height:6px;}
  .progress-fill{
    height:100%;
    width:0%;
    border-radius:999px;
    background:linear-gradient(90deg,var(--accent),#4c7ed9);
    transition:width .35s ease;
  }
  table{
    width:100%;
    border-collapse:collapse;
  }
  th,td{
    padding:8px 0;
    font-size:14px;
    border-bottom:1px solid var(--border);
    text-align:left;
  }
  th{color:var(--muted);font-weight:600;}
  .btn{
    border:1px solid var(--accent);
    background:var(--accent);
    color:#fff;
    padding:10px 16px;
    border-radius:8px;
    font-weight:600;
    cursor:pointer;
  }
  .btn.secondary{
    background:#f2f6ff;
    color:var(--accent);
  }
  .btn.danger{
    border-color:var(--danger);
    background:var(--danger);
  }
  .actions{
    display:flex;
    flex-wrap:wrap;
    gap:12px;
  }
  form .field{
    display:flex;
    flex-direction:column;
    gap:6px;
  }
  input, textarea{
    font:inherit;
    padding:10px 12px;
    border-radius:8px;
    border:1px solid var(--border);
    background:#f9fafe;
  }
  textarea{min-height:120px;resize:vertical;}
  .form-grid{
    display:grid;
    gap:12px;
  }
  @media(min-width:640px){
    .form-grid{grid-template-columns:repeat(2,minmax(0,1fr));}
  }
  .note{font-size:13px;color:var(--muted);}
  .log-table th, .log-table td{padding:8px 6px;}
  .chips{display:flex;gap:10px;flex-wrap:wrap;font-size:13px;color:var(--muted);}
  .chip{padding:4px 8px;border-radius:999px;background:#f0f2f8;border:1px solid var(--border);}
  .metric-cell{display:flex;flex-direction:column;gap:6px;}
</style>
</head>
<body>
<header>
  <div class="header-wrap">
    <div>
      <h1>Administraci&oacute;n del anfiteatro</h1>
      <p id="subtitle">Esperando conexi&oacute;n...</p>
    </div>
    <a class="home-link" href="/">Ver panel p&uacute;blico</a>
  </div>
  <div class="state-wrap">
    <div class="badge phase-abierto" id="stateBadge">
      <span id="dot" class="dot dot-open" aria-hidden="true"></span>
      <div class="block">
        <span>Fase</span>
        <span id="faseTxt">--</span>
      </div>
      <div class="block">
        <span>Tiempo restante</span>
        <span class="count" id="countTxt">00:00</span>
      </div>
    </div>
    <div class="status-extra">Actualizado: <span id="lastUpdate">sin datos</span></div>
  </div>
</header>

<main>
  <div class="status-message" id="statusMessage" data-state="info">Panel listo.</div>

  <section class="panel">
    <h2>Resumen r&aacute;pido</h2>
    <div class="grid three">
      <div class="summary-card">
        <span class="label">Ocupaci&oacute;n</span>
        <span class="value"><span id="afAct">0</span> / <span id="afMax2">0</span></span>
        <div class="progress"><div id="afBar" class="progress-fill"></div></div>
        <span class="note">Capacidad: <span id="afMax">0</span> personas &bull; <span id="afPct">0</span>% utilizado</span>
      </div>
      <div class="summary-card">
        <span class="label">Ingresos</span>
        <span class="value" id="ingresos">0</span>
        <span class="note">Lecturas RFID acumuladas</span>
      </div>
      <div class="summary-card">
        <span class="label">Egresos</span>
        <span class="value" id="egresos">0</span>
        <span class="note">Cortes IR detectados</span>
      </div>
    </div>
    <div class="chips">
      <span class="chip">Duraci&oacute;n abierto: <b id="durAbierto">30</b> s</span>
      <span class="chip">Duraci&oacute;n funci&oacute;n: <b id="durFuncion">35</b> s</span>
      <span class="chip">Duraci&oacute;n espera: <b id="durEspera">30</b> s</span>
    </div>
  </section>

  <section class="panel">
    <h2>Sensores y actuadores</h2>
    <table>
      <tbody>
        <tr>
          <th>Temperatura</th>
          <td>
            <div class="metric-cell">
              <span><span id="tempVal">--</span> &deg;C</span>
              <div class="progress mini"><div id="tBar" class="progress-fill"></div></div>
            </div>
          </td>
        </tr>
        <tr>
          <th>Humedad</th>
          <td>
            <div class="metric-cell">
              <span><span id="humVal">--</span> %</span>
              <div class="progress mini"><div id="hBar" class="progress-fill"></div></div>
            </div>
          </td>
        </tr>
        <tr><th>Ventiladores</th><td><span id="fansTxt">--</span></td></tr>
        <tr><th>RFID</th><td><span id="rfidStatus">sin datos</span></td></tr>
      </tbody>
    </table>
  </section>

  <section class="panel">
    <h2>Acciones inmediatas</h2>
    <div class="actions">
      <button class="btn danger" id="btnReset">Resetear aforo</button>
      <button class="btn secondary" id="btnSync">Sincronizar ahora</button>
    </div>
    <p class="note">El reset vuelve los contadores de ingreso/egreso a cero.</p>
  </section>

  <section class="panel">
    <h2>Controles manuales</h2>
    <div class="actions">
      <button class="btn secondary" data-control="avanzar">Avanzar fase</button>
      <button class="btn secondary" data-control="abrir">Forzar apertura</button>
      <button class="btn secondary" data-control="cerrar">Forzar cierre</button>
      <button class="btn secondary" data-control="toggle-fans">Alternar ventiladores</button>
    </div>
    <p class="note">Estos comandos llaman a <code>POST /control</code> con <code>{"action": "..."} </code>. Implement&aacute; las rutas en el firmware para que surtan efecto.</p>
  </section>

  <section class="panel">
    <h2>Configurar tiempos y aforo</h2>
    <form id="configForm">
      <div class="form-grid">
        <div class="field">
          <label for="cfgAforo">Capacidad m&aacute;xima (personas)</label>
          <input type="number" id="cfgAforo" min="1" placeholder="Ej. 40" />
        </div>
        <div class="field">
          <label for="cfgAbierto">Duraci&oacute;n abierto (segundos)</label>
          <input type="number" id="cfgAbierto" min="1" placeholder="Ej. 30" />
        </div>
        <div class="field">
          <label for="cfgFuncion">Duraci&oacute;n funci&oacute;n (segundos)</label>
          <input type="number" id="cfgFuncion" min="1" placeholder="Ej. 35" />
        </div>
        <div class="field">
          <label for="cfgEspera">Duraci&oacute;n espera (segundos)</label>
          <input type="number" id="cfgEspera" min="1" placeholder="Ej. 30" />
        </div>
      </div>
      <div class="actions" style="margin-top:12px;">
        <button class="btn" type="submit">Enviar configuraci&oacute;n</button>
      </div>
    </form>
    <p class="note">Se env&iacute;a un JSON a <code>POST /config</code> con los valores definidos. Si la ruta no existe todav&iacute;a, ver&aacute;s un mensaje de error.</p>
  </section>

  <section class="panel">
    <h2>Historial de actualizaciones</h2>
    <table class="log-table">
      <thead>
        <tr>
          <th>Hora</th>
          <th>Fase</th>
          <th>Ocupaci&oacute;n</th>
          <th>Temp (&deg;C)</th>
          <th>Humedad (%)</th>
          <th>Ventiladores</th>
        </tr>
      </thead>
      <tbody id="logBody">
        <tr><td colspan="6">Sin datos todav&iacute;a.</td></tr>
      </tbody>
    </table>
  </section>

  <section class="panel">
    <h2>Notas del turno</h2>
    <textarea id="noteField" placeholder="Anot&aacute; novedades importantes para el pr&oacute;ximo turno."></textarea>
    <div class="actions">
      <button class="btn" id="btnSaveNote">Guardar nota</button>
      <button class="btn secondary" id="btnClearNote">Borrar</button>
    </div>
    <p class="note">Las notas se guardan localmente en este navegador.</p>
  </section>
</main>

<script>
  const durations = {
    abierto: 30000,
    funcion: 35000,
    espera: 30000
  };
  const order = ['abierto','funcion','espera'];
  const $ = id => document.getElementById(id);

  const badge = $('stateBadge');
  const sub = $('subtitle');
  const lastUpdate = $('lastUpdate');
  const statusMessage = $('statusMessage');
  const logs = [];
  let ws;

  function setStatus(text, state='info'){
    statusMessage.textContent = text;
    statusMessage.dataset.state = state;
  }

  function mmss(ms){
    const total = Math.max(0, Number(ms) || 0);
    const s = Math.floor(total / 1000);
    const m = Math.floor(s / 60);
    const r = s % 60;
    return (m < 10 ? '0' : '') + m + ':' + (r < 10 ? '0' : '') + r;
  }

  function label(fase){
    if(fase === 'funcion') return 'En funci\u00f3n';
    if(fase === 'espera') return 'En espera';
    return 'Abierto';
  }

  function setPhaseUI(fase, remMs){
    const normalized = order.includes(fase) ? fase : 'abierto';
    badge.classList.remove('phase-abierto','phase-funcion','phase-espera');
    badge.classList.add('phase-' + normalized);
    let dotClass = 'dot dot-open';
    if(normalized === 'funcion') dotClass = 'dot dot-func';
    if(normalized === 'espera') dotClass = 'dot dot-wait';
    $('dot').className = dotClass;
    $('faseTxt').textContent = label(normalized);
    $('countTxt').textContent = mmss(remMs);
  }

  function setOccupancy(actual, max){
    const act = Number.isFinite(actual) ? actual : 0;
    const mx = Number.isFinite(max) && max > 0 ? max : 0;
    $('afAct').textContent = act;
    $('afMax2').textContent = mx;
    $('afMax').textContent = mx;
    const pct = mx > 0 ? Math.round((act * 100) / mx) : 0;
    $('afPct').textContent = Math.max(0, Math.min(100, pct));
    $('afBar').style.width = Math.max(0, Math.min(100, pct)) + '%';
  }

  function setEnvironment(temp, hum, fansOn){
    if(Number.isFinite(temp)){
      $('tempVal').textContent = temp.toFixed(1);
      $('tBar').style.width = Math.max(0, Math.min(100, (temp / 45) * 100)) + '%';
    } else {
      $('tempVal').textContent = '--';
      $('tBar').style.width = '0%';
    }
    if(Number.isFinite(hum)){
      $('humVal').textContent = hum.toFixed(0);
      $('hBar').style.width = Math.max(0, Math.min(100, hum)) + '%';
    } else {
      $('humVal').textContent = '--';
      $('hBar').style.width = '0%';
    }
    const fans = $('fansTxt');
    if(fansOn){
      fans.textContent = 'encendidos';
      fans.style.color = 'var(--good)';
    } else {
      fans.textContent = 'apagados';
      fans.style.color = 'var(--muted)';
    }
  }

  function setHardwareStatus(alive){
    const el = $('rfidStatus');
    const ok = alive === true || alive === 'true';
    el.textContent = ok ? 'enlazado' : 'sin respuesta';
    el.style.color = ok ? 'var(--good)' : 'var(--danger)';
  }

  function updateDurations(){
    $('durAbierto').textContent = Math.round(durations.abierto / 1000);
    $('durFuncion').textContent = Math.round(durations.funcion / 1000);
    $('durEspera').textContent = Math.round(durations.espera / 1000);
  }

  function addLogEntry(fase, act, max, temp, hum, fansOn){
    const entry = {
      time: new Date(),
      fase,
      ocup: `${act}/${max}`,
      temp: Number.isFinite(temp) ? temp.toFixed(1) : '--',
      hum: Number.isFinite(hum) ? hum.toFixed(0) : '--',
      fans: fansOn ? 'Encendidos' : 'Apagados'
    };
    logs.unshift(entry);
    if(logs.length > 20) logs.pop();
    renderLog();
  }

  function renderLog(){
    const tbody = $('logBody');
    if(!logs.length){
      tbody.innerHTML = '<tr><td colspan="6">Sin datos todav\u00eda.</td></tr>';
      return;
    }
    tbody.innerHTML = logs.map(entry => {
      const time = entry.time.toLocaleTimeString('es-AR', {hour:'2-digit', minute:'2-digit', second:'2-digit'});
      return `<tr>
        <td>${time}</td>
        <td>${label(entry.fase)}</td>
        <td>${entry.ocup}</td>
        <td>${entry.temp}</td>
        <td>${entry.hum}</td>
        <td>${entry.fans}</td>
      </tr>`;
    }).join('');
  }

  function render(data){
    if(!data) return;
    applyStatusHints(data);
    const fase = typeof data.fase === 'string' ? data.fase : (data.estado === 'en-funcion' ? 'funcion' : 'abierto');
    const rem = Number(data.t_restante_ms) || 0;

    setPhaseUI(fase, rem);
    setOccupancy(Number(data.aforo_actual), Number(data.aforo_max));
    setEnvironment(data.temp != null ? Number(data.temp) : NaN,
                   data.hum != null ? Number(data.hum) : NaN,
                   !!data.fansOn);
    setHardwareStatus(data.rfid_alive);
    $('ingresos').textContent = data.ingresos ?? 0;
    $('egresos').textContent = data.egresos ?? 0;

    addLogEntry(fase,
      Number(data.aforo_actual) || 0,
      Number(data.aforo_max) || 0,
      data.temp != null ? Number(data.temp) : NaN,
      data.hum != null ? Number(data.hum) : NaN,
      !!data.fansOn
    );

    lastUpdate.textContent = new Date().toLocaleTimeString('es-AR', {hour:'2-digit', minute:'2-digit', second:'2-digit'});
  }

  function applyStatusHints(data){
    const af = Number(data.aforo_max);
    if(Number.isFinite(af) && af > 0){
      $('cfgAforo').placeholder = af;
    }

    const ab = Number(data.dur_abierto_ms);
    if(Number.isFinite(ab) && ab >= 1000){
      durations.abierto = ab;
      $('cfgAbierto').placeholder = Math.round(ab / 1000);
    }

    const fn = Number(data.dur_funcion_ms);
    if(Number.isFinite(fn) && fn >= 1000){
      durations.funcion = fn;
      $('cfgFuncion').placeholder = Math.round(fn / 1000);
    }

    const es = Number(data.dur_espera_ms);
    if(Number.isFinite(es) && es >= 1000){
      durations.espera = es;
      $('cfgEspera').placeholder = Math.round(es / 1000);
    }

    updateDurations();
  }

  function loadNotes(){
    const stored = localStorage.getItem('admin_notes');
    $('noteField').value = stored || '';
  }

  function saveNotes(){
    localStorage.setItem('admin_notes', $('noteField').value);
    setStatus('Nota guardada localmente.', 'success');
  }

  function clearNotes(){
    $('noteField').value = '';
    localStorage.removeItem('admin_notes');
    setStatus('Nota eliminada.', 'info');
  }

  function connectWS(){
    try{
      ws = new WebSocket(`ws://${location.hostname}:81/`);
      ws.onopen = () => { sub.textContent = 'Conectado en vivo'; };
      ws.onclose = () => {
        sub.textContent = 'Reconectando...';
        setTimeout(connectWS, 1500);
      };
      ws.onmessage = (ev) => {
        try{
          const payload = JSON.parse(ev.data);
          render(payload);
          sub.textContent = 'Datos en vivo';
        }catch(err){
          console.error('WS parse', err);
        }
      };
    }catch(err){
      sub.textContent = 'Error de conexi\u00f3n';
      setTimeout(connectWS, 2500);
    }
  }

  function loadOnce(){
    setStatus('Solicitando estado...', 'info');
    fetch('/status', {cache:'no-store'})
      .then(r => r.json())
      .then(data => {
        render(data);
        setStatus('Estado actualizado desde HTTP.', 'success');
      })
      .catch(() => {
        setStatus('No se pudo obtener /status.', 'error');
      });
  }

  async function resetAforo(){
    setStatus('Enviando reset de aforo...', 'info');
    try{
      let res = await fetch('/reset', {method:'POST'});
      if(!res.ok) res = await fetch('/reset');
      if(!res.ok) throw new Error('HTTP ' + res.status);
      setStatus('Aforo reseteado. Esperando actualizaci\u00f3n...', 'success');
      loadOnce();
    }catch(err){
      setStatus('Error al resetear: ' + err.message, 'error');
    }
  }

  async function sendControl(action){
    setStatus('Enviando comando "' + action + '"...', 'info');
    try{
      const res = await fetch('/control', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({action})
      });
      if(!res.ok) throw new Error('HTTP ' + res.status);
      setStatus('Comando enviado, aguardando respuesta del controlador.', 'success');
    }catch(err){
      setStatus('No se pudo ejecutar el comando. Revis\u00e1 la conexi\u00f3n con el equipo. Detalle: ' + err.message, 'error');
    }
  }

  async function submitConfig(ev){
    ev.preventDefault();
    const payload = {};
    const af = Number($('cfgAforo').value);
    const ab = Number($('cfgAbierto').value);
    const fn = Number($('cfgFuncion').value);
    const es = Number($('cfgEspera').value);

    if(Number.isFinite(af) && af > 0) payload.aforo_max = af;
    if(Number.isFinite(ab) && ab > 0) payload.dur_abierto_ms = Math.round(ab * 1000);
    if(Number.isFinite(fn) && fn > 0) payload.dur_funcion_ms = Math.round(fn * 1000);
    if(Number.isFinite(es) && es > 0) payload.dur_espera_ms = Math.round(es * 1000);

    if(Object.keys(payload).length === 0){
      setStatus('Ingres&aacute; al menos un valor antes de enviar.', 'error');
      return;
    }

    setStatus('Enviando configuraci\u00f3n...', 'info');
    try{
      const res = await fetch('/config', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify(payload)
      });
      if(!res.ok) throw new Error('HTTP ' + res.status);
      setStatus('Configuraci\u00f3n enviada. Esper&aacute; la confirmaci\u00f3n del firmware.', 'success');
    }catch(err){
      setStatus('No se pudo enviar configuraci\u00f3n. Revis\u00e1 la respuesta del firmware. Detalle: ' + err.message, 'error');
    }
  }

  document.getElementById('btnReset').addEventListener('click', resetAforo);
  document.getElementById('btnSync').addEventListener('click', loadOnce);
  document.querySelectorAll('[data-control]').forEach(btn => {
    btn.addEventListener('click', () => sendControl(btn.getAttribute('data-control')));
  });
  document.getElementById('configForm').addEventListener('submit', submitConfig);
  document.getElementById('btnSaveNote').addEventListener('click', saveNotes);
  document.getElementById('btnClearNote').addEventListener('click', clearNotes);

  loadNotes();
  updateDurations();
  loadOnce();
  connectWS();
</script>
</body>
</html>
)ADMIN";
