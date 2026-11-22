'use strict';

/* ======================================================================
   AeroStruct experience.js: the "living" parts of the interface.
   - Landing: split-flap departures board, route ticker, product preview
     (built from the browser's demo data), portal cards with photos
   - Admin / passenger sign-in: split screen with a photo showcase and
     a mini departures board
   - Console: rolling activity strip on the dashboard
   - Passenger portal: boarding-pass countdown that ticks every second
   Reads the shared `state` from app.js; never calls the API itself.
   All motion stops for prefers-reduced-motion.
   ====================================================================== */

(function experienceModule() {
  const calm = () => typeof prefersReducedMotion === 'function' ? prefersReducedMotion() : false;
  const data = () => (typeof state === 'object' && state) ? state : { flights: [], passengers: [], notifications: [] };
  const safe = (fn, fallback = '') => { try { return fn(); } catch (error) { return fallback; } };
  const code = (city) => safe(() => airportCode(city), String(city || '').slice(0, 3).toUpperCase());
  const tone = (status) => safe(() => statusClass(status), 'info');
  const mode = document.body.dataset.portalMode || (document.body.classList.contains('is-landing') ? 'landing' : '');
  const clock = () => new Date().toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit', hour12: false });

  function sortedFlights() {
    return [...(data().flights || [])].sort((a, b) => String(a.departureTime).localeCompare(String(b.departureTime)));
  }

  /* ================= split-flap departures board ================= */
  // Renders `rows` flights per page and flips to the next page every few seconds.
  function makeBoard({ rowsEl, clockEl, pageEl, rows = 5, interval = 6000 }) {
    if (!rowsEl) return null;
    let page = 0;
    let timer = 0;
    const flap = (text, cls = '') => `<span class="flap ${cls}">${[...String(text)].map((ch, i) => `<b style="--i:${i}">${ch === ' ' ? '&nbsp;' : esc(ch)}</b>`).join('')}</span>`;
    function render(animate) {
      const flights = sortedFlights();
      if (clockEl) clockEl.textContent = clock();
      if (!flights.length) { rowsEl.innerHTML = '<li class="board-empty">No departures scheduled</li>'; return; }
      const pages = Math.max(1, Math.ceil(flights.length / rows));
      page %= pages;
      const slice = flights.slice(page * rows, page * rows + rows);
      rowsEl.innerHTML = slice.map((f, index) => `<li class="board-row${animate && !calm() ? ' is-flipping' : ''}" style="--row:${index}">
          ${flap(f.departureTime, 'f-time')}${flap(f.flightNumber, 'f-no')}${flap(String(f.destination || '').toUpperCase().slice(0, 12), 'f-dest')}${flap(f.gate || '—', 'f-gate')}
          <span class="board-status status-${tone(f.status)}"><i></i>${esc(f.status)}</span></li>`).join('');
      if (pageEl) pageEl.textContent = pages > 1 ? `Page ${page + 1} of ${pages}` : '';
    }
    function start() {
      stop();
      render(false);
      if (calm()) return;
      timer = window.setInterval(() => { page += 1; render(true); }, interval);
    }
    function stop() { if (timer) window.clearInterval(timer); timer = 0; }
    document.addEventListener('visibilitychange', () => (document.hidden ? stop() : start()));
    window.setInterval(() => { if (clockEl) clockEl.textContent = clock(); }, 15000);
    start();
    return { render, start };
  }

  /* ================= landing ================= */
  function landing() {
    const board = makeBoard({ rowsEl: document.getElementById('lBoardRows'), clockEl: document.getElementById('lBoardClock'), pageEl: document.getElementById('lBoardPage'), rows: 5 });

    // Route ticker: every flight as "AI 202 DEL → BOM 21:10 · On Time"
    const ticker = document.getElementById('lTicker');
    function renderTicker() {
      if (!ticker) return;
      const items = sortedFlights().map((f) => `<span class="tick"><b>${esc(f.flightNumber)}</b> ${esc(code(f.source))} <i>→</i> ${esc(code(f.destination))} <em>${esc(f.departureTime)}</em><span class="board-status status-${tone(f.status)}"><i></i>${esc(f.status)}</span></span>`).join('');
      // Two copies make the loop seamless.
      ticker.innerHTML = items ? `<div class="tick-set">${items}</div><div class="tick-set" aria-hidden="true">${items}</div>` : '';
      ticker.style.setProperty('--ticker-duration', `${Math.max(30, sortedFlights().length * 7)}s`);
    }
    renderTicker();

    // Portal cards get their photographs.
    document.querySelectorAll('[data-photo-host]').forEach((card) => {
      const host = card.querySelector('.l-portal-media');
      if (host && !host.querySelector('img') && typeof photoImg === 'function') {
        host.innerHTML = photoImg(card.dataset.photoHost, { widths: [700, 1100, 1600], sizes: '(min-width: 900px) 50vw, 100vw' });
      }
    });

    // Product preview
    const opsPane = document.getElementById('lPaneOps');
    const paxPane = document.getElementById('lPanePax');
    function renderPreview() {
      const d = data();
      const flights = sortedFlights();
      const passengers = d.passengers || [];
      const delayed = flights.filter((f) => /delay/i.test(f.status)).length;
      const cancelled = flights.filter((f) => /cancel/i.test(f.status)).length;
      const checked = passengers.filter((p) => p.status && p.status !== 'Registered').length;
      const assisted = passengers.filter((p) => p.assistanceType && p.assistanceType !== 'None').slice(0, 3);
      if (opsPane) {
        opsPane.innerHTML = `<div class="pv-ops">
          <aside class="pv-side"><span class="pv-logo">${icon('plane', 16)}</span>${['dashboard', 'plane', 'users', 'flow', 'heart', 'bag', 'compass', 'bell'].map((name, i) => `<span class="pv-nav${i === 0 ? ' on' : ''}">${icon(name, 15)}</span>`).join('')}</aside>
          <div class="pv-main">
            <div class="pv-banner"><small>OPERATIONS · LIVE</small><strong>Good ${new Date().getHours() < 12 ? 'morning' : new Date().getHours() < 17 ? 'afternoon' : 'evening'}, Ops Desk</strong></div>
            <div class="pv-kpis">
              <div><small>Flights</small><strong>${flights.length}</strong><i style="--w:${Math.min(100, flights.length * 12)}%"></i></div>
              <div><small>Passengers</small><strong>${passengers.length}</strong><i style="--w:${Math.min(100, passengers.length * 10)}%"></i></div>
              <div class="warn"><small>Delayed</small><strong>${delayed}</strong><i style="--w:${flights.length ? (delayed / flights.length) * 100 : 0}%"></i></div>
              <div class="bad"><small>Cancelled</small><strong>${cancelled}</strong><i style="--w:${flights.length ? (cancelled / flights.length) * 100 : 0}%"></i></div>
            </div>
            <div class="pv-grid">
              <div class="pv-card"><small>DEPARTURES</small>${flights.slice(0, 5).map((f) => `<div class="pv-row"><span>${esc(f.departureTime)}</span><b>${esc(f.flightNumber)}</b><span>${esc(f.destination)}</span><span class="board-status status-${tone(f.status)}"><i></i>${esc(f.status)}</span></div>`).join('')}</div>
              <div class="pv-card"><small>PRIORITY DESK</small>${assisted.length ? assisted.map((p, i) => `<div class="pv-pq"><span class="pv-p p${i + 2}">${i + 2}</span><b>${esc(p.name)}</b><span>${esc(p.assistanceType)}</span></div>`).join('') : '<p class="pv-empty">No assistance requests</p>'}
                <small style="margin-top:12px">CHECK-IN PROGRESS</small><div class="pv-bar"><i style="--w:${passengers.length ? (checked / passengers.length) * 100 : 0}%"></i></div><span class="pv-note">${checked} of ${passengers.length} passengers checked in</span></div>
            </div>
          </div></div>`;
      }
      if (paxPane) {
        const p = passengers.find((x) => flights.some((f) => f.flightNumber === x.flightNumber && !/cancel/i.test(f.status))) || passengers[0];
        const f = p && flights.find((x) => x.flightNumber === p.flightNumber);
        const steps = ['Registered', 'Checked-In', 'Security-Cleared', 'Boarded'];
        const at = p ? Math.max(0, steps.indexOf(p.status)) : 0;
        paxPane.innerHTML = p && f ? `<div class="pv-pax">
          <div class="pv-pass">
            <div class="pv-pass-main">
              <div class="pv-pass-head"><span>${icon('plane', 14)} ${esc(f.airline)}</span><span class="board-status status-${tone(f.status)}"><i></i>${esc(f.status)}</span></div>
              <div class="pv-route"><div><strong>${esc(code(f.source))}</strong><span>${esc(f.source)} · ${esc(f.departureTime)}</span></div><div class="pv-path"><i></i><span>${icon('plane', 18)}</span></div><div class="end"><strong>${esc(code(f.destination))}</strong><span>${esc(f.destination)} · ${esc(f.arrivalTime)}</span></div></div>
              <div class="pv-meta"><div><small>Passenger</small><b>${esc(p.name)}</b></div><div><small>Flight</small><b>${esc(f.flightNumber)}</b></div><div><small>Gate</small><b>${esc(f.gate)}</b></div><div><small>Seat</small><b>${esc(p.seatNumber)}</b></div></div>
            </div>
            <div class="pv-pass-stub"><small>BOARDING</small><strong>${esc(f.departureTime)}</strong><span class="pv-barcode">${typeof barcodeSvg === 'function' ? barcodeSvg(p.passengerId + f.flightNumber) : ''}</span></div>
          </div>
          <div class="pv-steps">${steps.map((s, i) => `<div class="${i < at ? 'done' : i === at ? 'now' : ''}"><i>${i < at ? icon('check', 12) : ''}</i><span>${esc(s)}</span></div>`).join('')}</div>
        </div>` : '<p class="pv-empty">Register a passenger to see the portal preview.</p>';
      }
    }
    renderPreview();

    // Tabs: auto-rotate every 6 s, pause while the pointer is over the device.
    const tabs = [...document.querySelectorAll('[data-show-tab]')];
    const panes = [...document.querySelectorAll('[data-show-pane]')];
    const device = document.getElementById('lDevice');
    const url = document.getElementById('lDeviceUrl');
    const tabRow = document.querySelector('.l-show-tabs');
    let current = 'ops';
    let rotate = 0;
    function show(name) {
      current = name;
      tabs.forEach((t) => t.setAttribute('aria-selected', String(t.dataset.showTab === name)));
      panes.forEach((p) => p.classList.toggle('is-active', p.dataset.showPane === name));
      if (url) url.textContent = name === 'ops' ? 'aerostruct / operations-console' : 'aerostruct / passenger-portal';
      if (tabRow) { tabRow.dataset.active = name; tabRow.classList.remove('is-timing'); void tabRow.offsetWidth; if (rotate) tabRow.classList.add('is-timing'); }
    }
    function startRotate() { stopRotate(); if (calm()) return; rotate = window.setInterval(() => show(current === 'ops' ? 'pax' : 'ops'), 6000); show(current); }
    function stopRotate() { if (rotate) window.clearInterval(rotate); rotate = 0; if (tabRow) tabRow.classList.remove('is-timing'); }
    tabs.forEach((t) => t.addEventListener('click', () => { show(t.dataset.showTab); startRotate(); }));
    if (device) {
      device.addEventListener('pointerenter', stopRotate);
      device.addEventListener('pointerleave', () => { device.style.removeProperty('--rx'); device.style.removeProperty('--ry'); startRotate(); });
      device.addEventListener('pointermove', (event) => {
        if (calm() || event.pointerType !== 'mouse') return;
        const r = device.getBoundingClientRect();
        device.style.setProperty('--ry', `${((event.clientX - r.left) / r.width - .5) * 6}deg`);
        device.style.setProperty('--rx', `${((event.clientY - r.top) / r.height - .5) * -5}deg`);
      });
    }
    startRotate();

    window.addEventListener('storage', () => { board && board.render(false); renderTicker(); renderPreview(); });
  }

  /* ================= admin / passenger sign-in showcase ================= */
  function gateShowcase() {
    const gate = document.getElementById('roleGate');
    if (!gate || gate.querySelector('.gate-showcase')) return;
    const isAdmin = mode === 'admin';
    const photos = isAdmin ? ['hero1', 'strip', 'hero2'] : ['dash', 'hero2', 'assist'];
    const points = isAdmin
      ? [['plane', 'Flights & live departures board'], ['flow', 'Check-in, security and boarding queues'], ['alert', 'Severity-ordered emergency desk']]
      : [['ticket', 'Your boarding pass and live journey'], ['bag', 'Baggage tracking from drop to belt'], ['heart', 'Request assistance in one tap']];
    const aside = document.createElement('aside');
    aside.className = 'gate-showcase';
    aside.setAttribute('aria-hidden', 'true');
    aside.innerHTML = `<div class="gs-media">${photos.map((key, i) => typeof photoImg === 'function' ? photoImg(key, { widths: [900, 1400, 2000], sizes: '55vw', lazy: i > 0, cls: `gs-slide${i === 0 ? ' is-active' : ''}` }) : '').join('')}</div>
      <div class="gs-shade"></div>
      <div class="gs-content">
        <span class="gs-eyebrow"><i class="live-dot"></i> ${isAdmin ? 'OPERATIONS CONSOLE' : 'PASSENGER PORTAL'}</span>
        <h2>${isAdmin ? 'Every flight, queue and passenger, in one calm control room.' : 'Your whole journey, from check-in to gate, in your pocket.'}</h2>
        <ul class="gs-points">${points.map(([ic, text]) => `<li><span>${icon(ic, 16)}</span>${esc(text)}</li>`).join('')}</ul>
        <div class="gs-board">
          <header><span>${icon('plane', 14)} Departures</span><span class="gs-clock">${clock()}</span></header>
          <ol class="gs-rows"></ol>
          <footer>Demo data · live after sign-in</footer>
        </div>
      </div>`;
    gate.appendChild(aside);
    gate.classList.add('has-showcase');
    makeBoard({ rowsEl: aside.querySelector('.gs-rows'), clockEl: aside.querySelector('.gs-clock'), rows: 4, interval: 5000 });

    const slides = [...aside.querySelectorAll('.gs-slide')];
    let index = 0;
    window.setInterval(() => {
      if (calm() || document.hidden || gate.hidden || slides.length < 2) return;
      slides[index].classList.remove('is-active');
      index = (index + 1) % slides.length;
      slides[index].loading = 'eager';
      void slides[index].offsetWidth;
      slides[index].classList.add('is-active');
    }, 7000);
  }

  /* ================= admin dashboard: rolling activity strip ================= */
  function opsTicker() {
    const host = document.getElementById('opsTicker');
    if (!host) return;
    let signature = '';
    function render() {
      const d = data();
      const items = [];
      (d.notifications || []).slice(-10).reverse().forEach((n) => items.push({ kind: /cancel/i.test(n.message) ? 'bad' : /delay/i.test(n.message) ? 'warn' : 'info', text: n.message, who: n.passengerId }));
      sortedFlights().forEach((f) => { if (!/on time/i.test(f.status)) items.push({ kind: tone(f.status), text: `${f.flightNumber} to ${f.destination} is ${String(f.status).toLowerCase()} · Gate ${f.gate}`, who: 'Flight ops' }); });
      const next = JSON.stringify(items);
      if (next === signature) return;
      signature = next;
      if (!items.length) { host.innerHTML = ''; host.hidden = true; return; }
      host.hidden = false;
      const list = items.map((it) => `<span class="ot-item ot-${esc(it.kind)}"><i></i><b>${esc(it.who || '')}</b>${esc(it.text)}</span>`).join('');
      host.innerHTML = `<span class="ot-label"><i class="live-dot"></i> Live activity</span><div class="ot-viewport"><div class="ot-track" style="--ot-duration:${Math.max(28, items.length * 8)}s"><div class="ot-set">${list}</div><div class="ot-set" aria-hidden="true">${list}</div></div></div>`;
    }
    render();
    window.setInterval(render, 5000);
  }

  /* ================= boarding pass: ticking countdown ================= */
  function boardingCountdown() {
    window.setInterval(() => {
      document.querySelectorAll('.bp-countdown [data-departs]').forEach((el) => {
        const [h, m] = String(el.dataset.departs).split(':').map(Number);
        if (!Number.isFinite(h) || !Number.isFinite(m)) return;
        const now = new Date();
        const target = new Date(now); target.setHours(h, m, 0, 0);
        const secs = Math.round((target - now) / 1000);
        if (secs < 0) { el.textContent = 'Departure time passed'; el.removeAttribute('data-departs'); return; }
        const hh = Math.floor(secs / 3600), mm = Math.floor((secs % 3600) / 60), ss = secs % 60;
        el.textContent = hh ? `in ${hh}h ${String(mm).padStart(2, '0')}m ${String(ss).padStart(2, '0')}s` : `in ${mm}m ${String(ss).padStart(2, '0')}s`;
        el.closest('.boarding-pass')?.classList.toggle('is-soon', secs < 30 * 60);
      });
    }, 1000);
  }

  /* ================= start ================= */
  if (mode === 'landing') landing();
  if (mode === 'admin' || mode === 'passenger') gateShowcase();
  opsTicker();
  boardingCountdown();
  if (typeof hydrateIcons === 'function') hydrateIcons(document.getElementById('landing') || document);
})();
