'use strict';

/* ======================================================================
   AeroStruct enhance.js: interface polish only.
   - API activity bar + "API offline" banner (wraps window.fetch, never
     changes requests or responses)
   - Command palette (Ctrl/Cmd + K or "/") for pages and records
   - Cursor spotlight on cards, button ripple, sticky topbar shadow
   - Row flash when a flight status changes on the live boards
   Loads before app.js so the fetch wrapper also sees the first sync.
   Every feature switches its motion off for prefers-reduced-motion.
   ====================================================================== */

(function enhanceModule() {
  /* ---------------- theme: "light" (mixed: dark sidebar + photo banners, light workspace) or "dark" ---------------- */
  const THEME_KEY = 'aerostruct-theme';
  const readTheme = () => { try { return localStorage.getItem(THEME_KEY) === 'dark' ? 'dark' : 'light'; } catch (error) { return 'light'; } };
  const applyTheme = (theme) => {
    document.documentElement.dataset.theme = theme;
    document.querySelectorAll('.theme-toggle').forEach((button) => {
      button.setAttribute('aria-pressed', String(theme === 'dark'));
      button.title = theme === 'dark' ? 'Switch to light workspace' : 'Switch to dark workspace';
    });
  };
  applyTheme(readTheme());

  const calm = () => window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  const escapeHtml = (value) => String(value ?? '').replace(/[&<>"']/g, (ch) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[ch]));
  const svg = (paths, size = 18) => `<svg width="${size}" height="${size}" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${paths}</svg>`;
  const ICONS = {
    search: '<circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3"/>',
    page: '<rect x="3" y="3" width="18" height="18" rx="3"/><path d="M8 9h8M8 13h8M8 17h5"/>',
    plane: '<path d="M17.8 19.2 16 11l3.5-3.5C21 6 21.5 4 21 3c-1-.5-3 0-4.5 1.5L13 8 4.8 6.2c-.5-.1-.9.1-1.1.5l-.3.5c-.2.5-.1 1 .3 1.3L9 12l-2 3H4l-1 1 3 2 2 3 1-1v-3l3-2 3.5 5.3c.3.4.8.5 1.3.3l.5-.2c.4-.3.6-.7.5-1.2z"/>',
    user: '<circle cx="12" cy="8" r="4"/><path d="M4 21a8 8 0 0 1 16 0"/>',
    bolt: '<path d="M13 2 3 14h9l-1 8 10-12h-9z"/>',
    offline: '<path d="m2 2 20 20"/><path d="M8.5 16.5a5 5 0 0 1 7 0"/><path d="M2 8.8a15 15 0 0 1 4.2-2.6"/><path d="M10.7 5.1A15 15 0 0 1 22 8.8"/><path d="M5 12.9a10 10 0 0 1 5.2-2.8"/><path d="M16.9 11a10 10 0 0 1 2.1 1.9"/><path d="M12 20h.01"/>',
    enter: '<path d="M9 10 4 15l5 5"/><path d="M20 4v7a4 4 0 0 1-4 4H4"/>',
    sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>',
    moon: '<path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8z"/>'
  };

  /* ---------------- API activity bar + offline banner ---------------- */
  const bar = document.createElement('div');
  bar.className = 'api-activity';
  bar.setAttribute('aria-hidden', 'true');
  document.body.appendChild(bar);

  const banner = document.createElement('div');
  banner.className = 'api-offline';
  banner.setAttribute('role', 'status');
  banner.hidden = true;
  banner.innerHTML = `<span class="api-offline-icon">${svg(ICONS.offline, 18)}</span>
    <div><strong>Can't reach the AeroStruct API</strong><span>Check that the C++ server is running on port 8080. Changes won't be saved until it's back.</span></div>
    <button type="button" class="button button-outline" data-api-retry>Retry</button>`;
  document.body.appendChild(banner);
  banner.addEventListener('click', (event) => {
    if (event.target.closest('[data-api-retry]')) window.location.reload();
  });

  let inFlight = 0;
  let hideTimer = 0;
  const isApi = (resource) => {
    const url = typeof resource === 'string' ? resource : (resource && resource.url) || '';
    return /\/api(\/|$|\?)/.test(url);
  };
  const setBusy = () => {
    window.clearTimeout(hideTimer);
    if (inFlight > 0) { document.body.classList.add('api-busy'); return; }
    hideTimer = window.setTimeout(() => document.body.classList.remove('api-busy'), 220);
  };
  const nativeFetch = window.fetch.bind(window);
  window.fetch = function trackedFetch(resource, options) {
    if (!isApi(resource)) return nativeFetch(resource, options);
    inFlight += 1; setBusy();
    return nativeFetch(resource, options).then((response) => {
      banner.hidden = true;
      document.body.classList.remove('api-down');
      return response;
    }, (error) => {
      // Network-level failure only (server down / CORS). HTTP errors still resolve.
      banner.hidden = false;
      document.body.classList.add('api-down');
      throw error;
    }).finally(() => { inFlight = Math.max(0, inFlight - 1); setBusy(); });
  };

  /* ---------------- cursor spotlight on cards ---------------- */
  const SPOT = '.panel, .stat-card, .gate-card, .l-card, .l-tcard, .l-tile, .u-metric, .baggage-summary-card';
  let spotFrame = 0;
  let lastPoint = null;
  document.addEventListener('pointermove', (event) => {
    if (event.pointerType !== 'mouse' || calm()) return;
    lastPoint = event;
    if (spotFrame) return;
    spotFrame = window.requestAnimationFrame(() => {
      spotFrame = 0;
      const card = lastPoint.target.closest && lastPoint.target.closest(SPOT);
      if (!card) return;
      const rect = card.getBoundingClientRect();
      card.style.setProperty('--mx', `${lastPoint.clientX - rect.left}px`);
      card.style.setProperty('--my', `${lastPoint.clientY - rect.top}px`);
    });
  }, { passive: true });

  /* ---------------- button ripple ---------------- */
  document.addEventListener('pointerdown', (event) => {
    const button = event.target.closest('.button, .l-btn, .table-action');
    if (!button || button.disabled || calm()) return;
    const rect = button.getBoundingClientRect();
    const size = Math.max(rect.width, rect.height) * 1.6;
    const ripple = document.createElement('span');
    ripple.className = 'ripple';
    ripple.style.cssText = `width:${size}px;height:${size}px;left:${event.clientX - rect.left - size / 2}px;top:${event.clientY - rect.top - size / 2}px`;
    button.appendChild(ripple);
    window.setTimeout(() => ripple.remove(), 620);
  });

  /* ---------------- sticky topbar shadow ---------------- */
  const onScroll = () => document.body.classList.toggle('is-scrolled', window.scrollY > 6);
  window.addEventListener('scroll', onScroll, { passive: true });
  onScroll();

  /* ---------------- live board: flash rows whose status changed ---------------- */
  const boardMemory = {};
  function watchBoard(id, keyCell, statusCell) {
    const table = document.getElementById(id);
    if (!table) return;
    const snapshot = () => {
      const seen = boardMemory[id] || null;
      const next = {};
      [...table.querySelectorAll('tbody tr')].forEach((row) => {
        const key = row.cells[keyCell] && row.cells[keyCell].textContent.trim();
        const status = row.cells[statusCell] && row.cells[statusCell].textContent.trim();
        if (!key || !status) return;
        next[key] = status;
        if (seen && seen[key] && seen[key] !== status && !calm()) {
          row.classList.remove('row-flash'); void row.offsetWidth; row.classList.add('row-flash');
        }
      });
      boardMemory[id] = next;
    };
    new MutationObserver(snapshot).observe(table, { childList: true });
  }

  /* ---------------- command palette ---------------- */
  const palette = document.createElement('div');
  palette.className = 'cmdk';
  palette.hidden = true;
  palette.innerHTML = `<div class="cmdk-backdrop" data-cmdk-close></div>
    <div class="cmdk-panel" role="dialog" aria-modal="true" aria-label="Command palette">
      <div class="cmdk-search">${svg(ICONS.search, 18)}<input type="text" placeholder="Search pages, flights, passengers…" aria-label="Search" autocomplete="off" spellcheck="false"><kbd>Esc</kbd></div>
      <ul class="cmdk-list" role="listbox"></ul>
      <div class="cmdk-foot"><span><kbd>↑</kbd><kbd>↓</kbd> to move</span><span><kbd>↵</kbd> to open</span><span><kbd>Ctrl</kbd><kbd>K</kbd> to toggle</span></div>
    </div>`;
  document.body.appendChild(palette);
  const cmdInput = palette.querySelector('input');
  const cmdList = palette.querySelector('.cmdk-list');
  let cmdItems = [];
  let cmdIndex = 0;
  let cmdReturnFocus = null;

  const role = () => document.body.classList.contains('is-admin') ? 'admin' : document.body.classList.contains('is-user') ? 'user' : null;

  function collectItems(query) {
    const items = [];
    const currentRole = role();
    const navSelector = currentRole === 'admin' ? '#nav [data-page]' : '#uNav [data-upage]';
    document.querySelectorAll(navSelector).forEach((button) => {
      const label = button.textContent.replace(/\s+/g, ' ').replace(/\d+$/, '').trim();
      items.push({ group: 'Go to', icon: ICONS.page, label, hint: 'Page', run: () => button.click() });
    });

    if (currentRole === 'admin') {
      items.push({ group: 'Actions', icon: ICONS.bolt, label: 'Add a flight', hint: 'Flights', run: () => { document.querySelector('#nav [data-page="flights"]')?.click(); window.setTimeout(() => document.getElementById('fNo')?.focus(), 60); } });
      items.push({ group: 'Actions', icon: ICONS.bolt, label: 'Register a passenger', hint: 'Passengers', run: () => { document.querySelector('#nav [data-page="passengers"]')?.click(); window.setTimeout(() => document.getElementById('pId')?.focus(), 60); } });
      items.push({ group: 'Actions', icon: ICONS.bolt, label: 'Register baggage', hint: 'Baggage', run: () => { document.querySelector('#nav [data-page="baggage"]')?.click(); window.setTimeout(() => document.getElementById('baggagePassenger')?.focus(), 60); } });

      const snapshot = window.AeroStructDemo && typeof window.AeroStructDemo.getState === 'function' ? window.AeroStructDemo.getState() : null;
      if (snapshot && query) {
        (snapshot.flights || []).forEach((f) => items.push({
          group: 'Flights', icon: ICONS.plane, label: `${f.flightNumber} · ${f.source} → ${f.destination}`, hint: `${f.departureTime} · ${f.status}`,
          run: () => openWithSearch('flights', 'fSearch', f.flightNumber)
        }));
        (snapshot.passengers || []).forEach((p) => items.push({
          group: 'Passengers', icon: ICONS.user, label: `${p.name}`, hint: `${p.passengerId} · ${p.flightNumber} · ${p.status}`,
          run: () => openWithSearch('passengers', 'pSearch', p.passengerId)
        }));
      }
    }

    const q = query.trim().toLowerCase();
    const filtered = q ? items.filter((item) => `${item.label} ${item.hint} ${item.group}`.toLowerCase().includes(q)) : items;
    return filtered.slice(0, 40);
  }

  function openWithSearch(page, inputId, value) {
    document.querySelector(`#nav [data-page="${page}"]`)?.click();
    window.setTimeout(() => {
      const input = document.getElementById(inputId);
      if (!input) return;
      input.value = value;
      input.dispatchEvent(new Event('input', { bubbles: true }));
      input.focus();
    }, 60);
  }

  function renderPalette() {
    cmdItems = collectItems(cmdInput.value);
    cmdIndex = Math.min(cmdIndex, Math.max(0, cmdItems.length - 1));
    if (!cmdItems.length) {
      cmdList.innerHTML = '<li class="cmdk-empty">No matches. Try a flight number, passenger name or page.</li>';
      return;
    }
    let lastGroup = '';
    cmdList.innerHTML = cmdItems.map((item, index) => {
      const head = item.group !== lastGroup ? `<li class="cmdk-group" role="presentation">${escapeHtml(item.group)}</li>` : '';
      lastGroup = item.group;
      return `${head}<li class="cmdk-item${index === cmdIndex ? ' is-active' : ''}" role="option" aria-selected="${index === cmdIndex}" data-index="${index}"><span class="cmdk-icon">${svg(item.icon, 16)}</span><span class="cmdk-label">${escapeHtml(item.label)}</span><span class="cmdk-hint">${escapeHtml(item.hint)}</span><span class="cmdk-enter">${svg(ICONS.enter, 14)}</span></li>`;
    }).join('');
    const active = cmdList.querySelector('.is-active');
    if (active) active.scrollIntoView({ block: 'nearest' });
  }

  function openPalette() {
    if (!role()) return;
    cmdReturnFocus = document.activeElement;
    palette.hidden = false;
    document.body.classList.add('cmdk-open');
    cmdInput.value = '';
    cmdIndex = 0;
    renderPalette();
    window.requestAnimationFrame(() => cmdInput.focus());
  }
  function closePalette() {
    if (palette.hidden) return;
    palette.hidden = true;
    document.body.classList.remove('cmdk-open');
    if (cmdReturnFocus && cmdReturnFocus.focus) cmdReturnFocus.focus();
  }
  function runItem(index) {
    const item = cmdItems[index];
    if (!item) return;
    closePalette();
    item.run();
  }

  cmdInput.addEventListener('input', () => { cmdIndex = 0; renderPalette(); });
  cmdInput.addEventListener('keydown', (event) => {
    if (event.key === 'ArrowDown') { event.preventDefault(); cmdIndex = (cmdIndex + 1) % Math.max(1, cmdItems.length); renderPalette(); }
    else if (event.key === 'ArrowUp') { event.preventDefault(); cmdIndex = (cmdIndex - 1 + cmdItems.length) % Math.max(1, cmdItems.length); renderPalette(); }
    else if (event.key === 'Enter') { event.preventDefault(); runItem(cmdIndex); }
    else if (event.key === 'Escape') { event.preventDefault(); closePalette(); }
    else if (event.key === 'Tab') { event.preventDefault(); }
  });
  cmdList.addEventListener('click', (event) => {
    const item = event.target.closest('.cmdk-item');
    if (item) runItem(Number(item.dataset.index));
  });
  cmdList.addEventListener('pointermove', (event) => {
    const item = event.target.closest('.cmdk-item');
    if (!item) return;
    const index = Number(item.dataset.index);
    if (index !== cmdIndex) { cmdIndex = index; cmdList.querySelectorAll('.cmdk-item').forEach((el, i) => { el.classList.toggle('is-active', i === index); el.setAttribute('aria-selected', String(i === index)); }); }
  });
  palette.addEventListener('click', (event) => { if (event.target.closest('[data-cmdk-close]')) closePalette(); });

  document.addEventListener('keydown', (event) => {
    const typing = /^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement && document.activeElement.tagName);
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'k') {
      if (!role() || document.querySelector('dialog[open]')) return;
      event.preventDefault();
      if (palette.hidden) openPalette(); else closePalette();
    } else if (event.key === '/' && !typing && palette.hidden && role() && !document.querySelector('dialog[open]')) {
      event.preventDefault();
      openPalette();
    }
  });

  function addPaletteTriggers() {
    const isMac = /Mac|iPhone|iPad/.test(navigator.platform || '');
    document.querySelectorAll('.topbar-actions').forEach((actions) => {
      if (actions.querySelector('.cmdk-trigger')) return;
      const trigger = document.createElement('button');
      trigger.type = 'button';
      trigger.className = 'cmdk-trigger';
      trigger.setAttribute('aria-label', 'Open command palette');
      trigger.innerHTML = `${svg(ICONS.search, 16)}<span>Search</span><kbd>${isMac ? '⌘' : 'Ctrl'} K</kbd>`;
      trigger.addEventListener('click', openPalette);
      actions.insertBefore(trigger, actions.firstChild);
    });
  }

  /* ---------------- theme toggle buttons ---------------- */
  function addThemeToggles() {
    document.querySelectorAll('.topbar-actions').forEach((actions) => {
      if (actions.querySelector('.theme-toggle')) return;
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'theme-toggle';
      button.setAttribute('aria-label', 'Dark workspace');
      button.innerHTML = `<span class="tt-sun">${svg(ICONS.sun, 17)}</span><span class="tt-moon">${svg(ICONS.moon, 17)}</span>`;
      button.addEventListener('click', () => {
        const next = document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark';
        try { localStorage.setItem(THEME_KEY, next); } catch (error) { /* storage unavailable */ }
        document.documentElement.classList.add('theme-switching');
        applyTheme(next);
        window.setTimeout(() => document.documentElement.classList.remove('theme-switching'), 450);
      });
      const trigger = actions.querySelector('.cmdk-trigger');
      actions.insertBefore(button, trigger ? trigger.nextSibling : actions.firstChild);
    });
    applyTheme(readTheme());
  }

  /* ---------------- banner slideshow (slides chosen from the clock, see bannerHtml in app.js) ---------------- */
  const SLIDE_MS = 7000;
  function advanceBanners() {
    if (document.hidden || calm()) return;
    const tick = Math.floor(Date.now() / SLIDE_MS);
    document.querySelectorAll('.hb-media').forEach((media) => {
      const slides = media.querySelectorAll('.hb-slide');
      if (slides.length < 2) return;
      const active = tick % slides.length;
      slides.forEach((slide, index) => {
        if (index === active && !slide.classList.contains('is-active')) {
          slide.loading = 'eager';
          slide.classList.remove('is-active'); void slide.offsetWidth; slide.classList.add('is-active');
        } else if (index !== active) {
          slide.classList.remove('is-active');
        }
      });
      // Warm the next photo so the crossfade never shows a blank frame.
      const next = slides[(active + 1) % slides.length];
      if (next) next.loading = 'eager';
    });
  }
  window.setTimeout(function alignSlides() {
    advanceBanners();
    window.setInterval(advanceBanners, SLIDE_MS);
  }, SLIDE_MS - (Date.now() % SLIDE_MS) + 30);

  /* ---------------- start-up (DOM is already parsed: scripts sit at the end of <body>) ---------------- */
  addPaletteTriggers();
  addThemeToggles();
  watchBoard('depTable', 1, 4);
  watchBoard('flightTable', 1, 6);
  watchBoard('uDepTable', 1, 4);
  document.documentElement.classList.add('enhanced');
})();
