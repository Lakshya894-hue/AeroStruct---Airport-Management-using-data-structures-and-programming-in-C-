'use strict';

const STORAGE_KEY = 'aerostruct-validated-ops-v1';
// AeroStruct C++ backend API
const API_BASE_URL = 'http://127.0.0.1:8080/api';
const DEBUG_LOGS = false;
const debugLog = (...args) => { if (DEBUG_LOGS) console.log(...args); };
const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];

// Shared API helper: attach the server-issued bearer token to protected requests.
async function apiFetch(resource, options = {}) {
  const { skipAuth = false, ...fetchOptions } = options;
  const headers = new Headers(fetchOptions.headers || {});
  const token = !skipAuth && typeof window.getAeroStructToken === 'function'
    ? window.getAeroStructToken()
    : '';
  if (token && !headers.has('Authorization')) {
    headers.set('Authorization', `Bearer ${token}`);
  }
  return window.fetch(resource, { ...fetchOptions, headers });
}
window.aeroApiFetch = apiFetch;
window.AEROSTRUCT_API_BASE_URL = API_BASE_URL;

/* ======================================================================
   v2 redesign helpers: icons, counters, formatting.
   Added on top of the original logic; nothing below changes any validation.
   ====================================================================== */
const ICON_PATHS = {
  dashboard: '<rect x="3" y="3" width="7" height="9" rx="1.6"/><rect x="14" y="3" width="7" height="5" rx="1.6"/><rect x="14" y="12" width="7" height="9" rx="1.6"/><rect x="3" y="16" width="7" height="5" rx="1.6"/>',
  plane: '<path d="M17.8 19.2 16 11l3.5-3.5C21 6 21.5 4 21 3c-1-.5-3 0-4.5 1.5L13 8 4.8 6.2c-.5-.1-.9.1-1.1.5l-.3.5c-.2.5-.1 1 .3 1.3L9 12l-2 3H4l-1 1 3 2 2 3 1-1v-3l3-2 3.5 5.3c.3.4.8.5 1.3.3l.5-.2c.4-.3.6-.7.5-1.2z"/>',
  users: '<path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M22 21v-2a4 4 0 0 0-3-3.9"/><path d="M16 3.1a4 4 0 0 1 0 7.8"/>',
  flow: '<path d="m16 3 4 4-4 4"/><path d="M20 7H4"/><path d="m8 21-4-4 4-4"/><path d="M4 17h16"/>',
  bag: '<rect x="5" y="7" width="14" height="14" rx="2.5"/><path d="M9 7V5a2 2 0 0 1 2-2h2a2 2 0 0 1 2 2v2"/><path d="M9.5 11v6M14.5 11v6"/>',
  heart: '<path d="M19 14c1.5-1.5 3-3.2 3-5.5A5.5 5.5 0 0 0 16.5 3c-1.8 0-3 .5-4.5 2-1.5-1.5-2.7-2-4.5-2A5.5 5.5 0 0 0 2 8.5c0 2.3 1.5 4 3 5.5l7 7z"/>',
  group: '<circle cx="8" cy="8" r="3"/><circle cx="17" cy="9" r="2.4"/><path d="M2.5 20c0-3.1 2.5-5 5.5-5s5.5 1.9 5.5 5"/><path d="M15 15.2c.6-.2 1.3-.3 2-.3 2.5 0 4.5 1.6 4.5 4.1"/>',
  sparkle: '<path d="m12 3 1.9 5.1L19 10l-5.1 1.9L12 17l-1.9-5.1L5 10l5.1-1.9z"/><path d="M19 3v4M17 5h4M5 17v4M3 19h4"/>',
  lifebuoy: '<circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="3.5"/><path d="m5.6 5.6 3.9 3.9M14.5 14.5l3.9 3.9M14.5 9.5l3.9-3.9M5.6 18.4l3.9-3.9"/>',
  compass: '<circle cx="12" cy="12" r="9"/><path d="m15.5 8.5-2 5-5 2 2-5z"/>',
  alert: '<path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/><path d="M12 9v4M12 17h.01"/>',
  bell: '<path d="M18 8a6 6 0 0 0-12 0c0 7-3 9-3 9h18s-3-2-3-9"/><path d="M10.3 21a1.9 1.9 0 0 0 3.4 0"/>',
  ticket: '<path d="M3 9a2 2 0 0 0 0 6v3a1 1 0 0 0 1 1h16a1 1 0 0 0 1-1v-3a2 2 0 0 1 0-6V6a1 1 0 0 0-1-1H4a1 1 0 0 0-1 1z"/><path d="M14 5v2M14 11v2M14 17v2"/>',
  menu: '<path d="M4 6h16M4 12h16M4 18h16"/>',
  close: '<path d="M18 6 6 18M6 6l12 12"/>',
  logout: '<path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><path d="m16 17 5-5-5-5"/><path d="M21 12H9"/>',
  shield: '<path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/><path d="m9 12 2 2 4-4"/>',
  undo: '<path d="M3 7v6h6"/><path d="M21 17a9 9 0 0 0-15-6.7L3 13"/>',
  reset: '<path d="M3 12a9 9 0 0 1 15-6.7L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-15 6.7L3 16"/><path d="M3 21v-5h5"/>',
  check: '<path d="m20 6-11 11-5-5"/>',
  info: '<circle cx="12" cy="12" r="9"/><path d="M12 11v5M12 8h.01"/>',
  clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
  inbox: '<path d="M22 12h-6l-2 3h-4l-2-3H2"/><path d="M5.5 5.1 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.5-6.9A2 2 0 0 0 16.7 4H7.3a2 2 0 0 0-1.8 1.1z"/>',
  user: '<circle cx="12" cy="8" r="4"/><path d="M4 21a8 8 0 0 1 16 0"/>',
  phone: '<path d="M22 16.9v3a2 2 0 0 1-2.2 2 19.8 19.8 0 0 1-8.6-3.1 19.5 19.5 0 0 1-6-6A19.8 19.8 0 0 1 2.1 4.2 2 2 0 0 1 4.1 2h3a2 2 0 0 1 2 1.7c.1 1 .4 1.9.7 2.8a2 2 0 0 1-.5 2.1L8.1 9.9a16 16 0 0 0 6 6l1.3-1.3a2 2 0 0 1 2.1-.4c.9.3 1.8.6 2.8.7a2 2 0 0 1 1.7 2z"/>',
  pin: '<path d="M20 10c0 6-8 12-8 12S4 16 4 10a8 8 0 0 1 16 0z"/><circle cx="12" cy="10" r="3"/>',
  arrow: '<path d="M5 12h14M13 6l6 6-6 6"/>',
  plus: '<path d="M12 5v14M5 12h14"/>',
  search: '<circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3"/>',
  gate: '<path d="M4 21V4h12l4 4v13"/><path d="M2 21h20"/><path d="M9 21v-6h6v6"/>',
  medical: '<rect x="3" y="3" width="18" height="18" rx="4"/><path d="M12 8v8M8 12h8"/>',
  edit: '<path d="M12 20h9"/><path d="M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4z"/>',
  trash: '<path d="M3 6h18"/><path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/>',
  command: '<path d="M18 3a3 3 0 0 0-3 3v12a3 3 0 0 0 3 3 3 3 0 0 0 3-3 3 3 0 0 0-3-3H6a3 3 0 0 0-3 3 3 3 0 0 0 3 3 3 3 0 0 0 3-3V6a3 3 0 0 0-3-3 3 3 0 0 0-3 3 3 3 0 0 0 3 3h12a3 3 0 0 0 3-3 3 3 0 0 0-3-3z"/>',
  wifiOff: '<path d="m2 2 20 20"/><path d="M8.5 16.5a5 5 0 0 1 7 0"/><path d="M2 8.8a15 15 0 0 1 4.2-2.6"/><path d="M10.7 5.1A15 15 0 0 1 22 8.8"/><path d="M5 12.9a10 10 0 0 1 5.2-2.8"/><path d="M16.9 11a10 10 0 0 1 2.1 1.9"/><path d="M12 20h.01"/>',
  hospital: '<path d="M3 21V8l9-5 9 5v13"/><path d="M12 10v6M9 13h6"/><path d="M9 21v-3h6v3"/>',
  send: '<path d="M22 2 11 13"/><path d="M22 2 15 22l-4-9-9-4z"/>',
  radar: '<circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="4.5"/><path d="M12 12 18 6"/>'
};
function icon(name, size) {
  const px = size || 20;
  return `<svg class="icon" viewBox="0 0 24 24" width="${px}" height="${px}" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" focusable="false">${ICON_PATHS[name] || ''}</svg>`;
}
function hydrateIcons(root) {
  (root || document).querySelectorAll('[data-icon]').forEach((el) => {
    if (!el.firstElementChild) el.innerHTML = icon(el.dataset.icon, Number(el.dataset.size) || 20);
  });
}
function prefersReducedMotion() {
  return !!(window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches);
}
const countMemory = {};
function animateCounters(root) {
  if (!root) return;
  root.querySelectorAll('[data-count]').forEach((el) => {
    const key = el.dataset.countKey || el.dataset.count;
    const to = Number(el.dataset.count);
    const from = Number.isFinite(countMemory[key]) ? countMemory[key] : 0;
    countMemory[key] = to;
    if (!Number.isFinite(to) || from === to || prefersReducedMotion()) { el.textContent = Number.isFinite(to) ? String(to) : el.textContent; return; }
    const start = performance.now();
    const duration = 650;
    const step = (now) => {
      const t = Math.min(1, (now - start) / duration);
      const eased = 1 - Math.pow(1 - t, 3);
      el.textContent = String(Math.round(from + (to - from) * eased));
      if (t < 1) window.requestAnimationFrame(step);
    };
    window.requestAnimationFrame(step);
  });
}
function nowMinutes() { const d = new Date(); return d.getHours() * 60 + d.getMinutes(); }
function formatCountdown(minutes) {
  if (!Number.isFinite(minutes)) return '—';
  if (minutes <= 0) return 'Departing now';
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  return h ? `in ${h}h ${m}m` : `in ${m} min`;
}
const AIRPORT_CODES = { delhi: 'DEL', mumbai: 'BOM', hyderabad: 'HYD', bengaluru: 'BLR', bangalore: 'BLR', kochi: 'COK', goa: 'GOI', chennai: 'MAA', kolkata: 'CCU', pune: 'PNQ', ahmedabad: 'AMD', jaipur: 'JAI', lucknow: 'LKO', dehradun: 'DED', chandigarh: 'IXC', varanasi: 'VNS', patna: 'PAT' };
function airportCode(city) {
  const key = String(city || '').trim().toLowerCase();
  return AIRPORT_CODES[key] || (String(city || '').replace(/[^A-Za-z]/g, '').slice(0, 3).toUpperCase() || '---');
}
/* ---------- Photography registry (Unsplash licence, remote URLs, responsive sizes) ---------- */
const PHOTOS = {
  hero1: '1629221731259-4f0760e3ee89', // aircraft on a lit apron at night
  hero2: '1721592873149-9823d9dc6b40', // aircraft landing at dusk
  hero3: '1683971336619-d445cbec0276', // runway at sunset, aerial
  hero4: '1609248189197-eb894d4283a1', // wing at sunset
  about: '1746129700182-bef672132578', // control tower at sunset
  assist: '1766955121786-b7259e29e69c', // travellers in a terminal
  track: '1647363377737-8d0ad7c2f494', // wing above clouds
  dash: '1683971113886-ca5883e598b6', // runway at sunset, aerial (second frame)
  nav: '1723059518530-1feec72f2771', // aircraft over the runway
  strip: '1670926774123-4e36ed8f5fd1' // row of aircraft on the tarmac
};
function photoUrl(key, width) {
  return `https://images.unsplash.com/photo-${PHOTOS[key]}?auto=format&fit=crop&w=${width}&q=${width > 1200 ? 66 : 72}`;
}
function photoSet(key, widths) { return widths.map((w) => `${photoUrl(key, w)} ${w}w`).join(', '); }
function photoImg(key, options) {
  const o = Object.assign({ widths: [640, 1100, 1600], sizes: '100vw', lazy: true, priority: false, cls: '' }, options || {});
  const loading = o.priority ? 'fetchpriority="high"' : `loading="${o.lazy ? 'lazy' : 'eager'}"`;
  return `<img class="${o.cls}" src="${photoUrl(key, o.widths[1] || o.widths[0])}" srcset="${photoSet(key, o.widths)}" sizes="${o.sizes}" alt="" ${loading} decoding="async" onerror="var b=this.closest('[data-photo-box]');if(b)b.classList.add('photo-failed');this.remove()">`;
}

/* ---------- Decorative aviation graphics (flight arcs, radar) ---------- */
let arcUid = 0;
function flightArcSvg() {
  const id = `hbArc${++arcUid}`;
  const moving = !prefersReducedMotion();
  const plane = moving
    ? `<g class="hb-plane"><use href="#plane-glyph"/><animateMotion dur="10s" repeatCount="indefinite" rotate="auto"><mpath href="#${id}"/></animateMotion></g>`
    : `<g class="hb-plane" transform="translate(470 38) rotate(18)"><use href="#plane-glyph"/></g>`;
  return `<svg class="hb-arc" viewBox="0 0 640 180" preserveAspectRatio="xMaxYMid slice" aria-hidden="true" focusable="false"><path id="${id}" class="hb-arc-line" d="M20 150 C 170 8, 400 4, 620 112"/><g class="hb-radar" transform="translate(620 112)"><circle r="5"/><circle class="ring r1" r="5"/><circle class="ring r2" r="5"/></g><g class="hb-radar" transform="translate(20 150)"><circle r="4"/><circle class="ring r1" r="4"/></g>${plane}</svg>`;
}
// Every page gets a photo banner; each banner slowly crossfades through a
// short playlist of aviation photos (enhance.js advances the slides).
const BANNER_SCENES = {
  dashboard: { photos: ['dash', 'hero1', 'hero2', 'strip'], pos: '50% 58%' },
  flights: { photos: ['strip', 'hero2', 'hero4'], pos: '50% 55%' },
  passengers: { photos: ['assist', 'hero1', 'about'], pos: '50% 45%' },
  queues: { photos: ['assist', 'strip', 'hero1'], pos: '50% 45%' },
  priority: { photos: ['assist', 'about', 'hero3'], pos: '50% 42%' },
  groups: { photos: ['hero3', 'assist', 'track'], pos: '50% 50%' },
  suggestions: { photos: ['track', 'hero4', 'hero2'], pos: '50% 55%' },
  navigator: { photos: ['nav', 'hero1', 'about'], pos: '50% 55%' },
  emergency: { photos: ['about', 'nav', 'hero1'], pos: '50% 50%' },
  baggage: { photos: ['track', 'strip', 'hero4'], pos: '50% 62%' },
  notes: { photos: ['hero2', 'track', 'dash'], pos: '50% 55%' },
  trip: { photos: ['dash', 'track', 'hero2', 'hero4'], pos: '50% 58%' },
  assist: { photos: ['assist', 'about', 'hero3'], pos: '50% 42%' }
};
const BANNER_SLIDE_MS = 7000;
function bannerHtml(page, inner) {
  const scene = BANNER_SCENES[page];
  if (!scene) return inner;
  const photos = scene.photos || [scene.photo];
  // Derive the active slide from the clock so re-renders don't restart the show.
  const active = Math.floor(Date.now() / BANNER_SLIDE_MS) % photos.length;
  const slides = photos.map((key, index) => photoImg(key, {
    widths: [640, 1100, 1700],
    sizes: '(min-width: 961px) calc(100vw - 258px), 100vw',
    lazy: index !== active,
    cls: `hb-slide${index === active ? ' is-active' : ''}`
  })).join('');
  return `<div class="heading-banner" data-photo-box style="--pos:${scene.pos}"><div class="hb-media" aria-hidden="true">${slides}</div><div class="hb-shade" aria-hidden="true"></div><div class="hb-clouds" aria-hidden="true"></div>${flightArcSvg()}<div class="hb-content">${inner}</div></div>`;
}

/* ---------- Illustrated terminal floor plan (same graph, same coordinates) ---------- */
const FP_STYLE = {
  'Main Entrance': ['pin', 'blue'], 'Check-In Counter': ['ticket', 'blue'], 'Security Check': ['shield', 'red'], 'Security Office': ['shield', 'red'],
  'Departure Lounge': ['plane', 'amber'], 'Gate A1': ['gate', 'green'], 'Gate A2': ['gate', 'green'], 'Medical Centre': ['medical', 'red'],
  'Baggage Claim': ['bag', 'blue'], 'Restrooms': ['users', 'teal'], 'Fire Extinguisher': ['alert', 'red'], 'Information & Help Desk': ['info', 'sky']
};
function edgeWeight(a, b) {
  const edge = NAV_GRAPH.edges.find(([x, y]) => (x === a && y === b) || (x === b && y === a));
  return edge ? edge[2] : 0;
}
function floorPlanPin(x, y, tone, label) {
  return `<g class="fp-pin fp-pin-${tone}" transform="translate(${x} ${y - 30})"><path d="M0 22 C-13 8 -13 -9 0 -9 C13 -9 13 8 0 22Z"/><circle cy="2" r="5"/></g><title>${esc(label)}</title>`;
}
function buildMapMarkup(fromName, toName, path, nodeAttr) {
  const zones = `<rect class="fp-zone fp-zone-land" x="14" y="14" width="420" height="382" rx="24"/>
    <rect class="fp-zone fp-zone-sec" x="438" y="14" width="30" height="382" rx="10"/>
    <rect class="fp-zone fp-zone-air" x="472" y="14" width="332" height="382" rx="24"/>
    <rect class="fp-zone fp-zone-apron" x="810" y="14" width="38" height="382" rx="14"/>
    <text class="fp-zone-label" x="34" y="40">LANDSIDE · CHECK-IN HALL</text>
    <text class="fp-zone-label" x="492" y="40" >AIRSIDE</text>
    <use class="fp-apron-plane" href="#plane-glyph" transform="translate(829 118) rotate(180) scale(1.55)"/>
    <use class="fp-apron-plane" href="#plane-glyph" transform="translate(829 318) rotate(180) scale(1.55)"/>`;
  const corridors = NAV_GRAPH.edges.map(([from, to]) => {
    const a = nodeByName(from), b = nodeByName(to);
    return `<line class="fp-corr" x1="${a.x}" y1="${a.y}" x2="${b.x}" y2="${b.y}"/>`;
  }).join('');
  const edges = NAV_GRAPH.edges.map(([from, to, weight]) => {
    const a = nodeByName(from), b = nodeByName(to);
    const mx = (a.x + b.x) / 2, my = (a.y + b.y) / 2;
    const label = `${weight} m`; const w = label.length * 6.6 + 12;
    return `<line class="fp-edge" x1="${a.x}" y1="${a.y}" x2="${b.x}" y2="${b.y}"/><g class="fp-weight" transform="translate(${mx} ${my})"><rect x="${-w / 2}" y="-10" width="${w}" height="18" rx="9"/><text y="3.5">${label}</text></g>`;
  }).join('');
  let route = '';
  if (path && path.length > 1) {
    const d = path.map((name, i) => { const n = nodeByName(name); return `${i ? 'L' : 'M'}${n.x} ${n.y}`; }).join(' ');
    route = `<path class="fp-route-casing" d="${d}"/><path class="fp-route" d="${d}"/><path class="fp-route-flow" d="${d}"/>`;
  }
  const nodes = NAV_GRAPH.locations.map((loc) => {
    const [iconName, tone] = FP_STYLE[loc.name] || ['pin', 'blue'];
    const classes = ['fp-node', `fp-${tone}`, loc.name === fromName ? 'from' : '', loc.name === toName ? 'to' : '', path && path.includes(loc.name) ? 'path' : ''].filter(Boolean).join(' ');
    return `<g class="${classes}" ${nodeAttr}="${esc(loc.name)}" role="button" tabindex="0" aria-label="Set destination to ${esc(loc.name)}"><circle class="fp-halo" cx="${loc.x}" cy="${loc.y}" r="25"/><rect class="fp-badge" x="${loc.x - 18}" y="${loc.y - 18}" width="36" height="36" rx="11"/><g class="fp-glyph" transform="translate(${loc.x - 9} ${loc.y - 9})">${icon(iconName, 18)}</g><text class="fp-label" x="${loc.x}" y="${loc.y + 36}">${esc(loc.name)}</text></g>`;
  }).join('');
  const fromNode = fromName && nodeByName(fromName);
  const toNode = toName && nodeByName(toName);
  const pins = `${fromNode ? `<g class="fp-here" transform="translate(${fromNode.x} ${fromNode.y})"><circle class="fp-pulse" r="22"/><circle class="fp-pulse p2" r="22"/></g>${floorPlanPin(fromNode.x, fromNode.y - 14, 'from', 'Start: ' + fromName)}` : ''}${toNode && toNode !== fromNode ? floorPlanPin(toNode.x, toNode.y - 14, 'to', 'Destination: ' + toName) : ''}`;
  const walker = path && path.length > 1 ? `<g class="fp-walker" transform="translate(${nodeByName(path[0]).x} ${nodeByName(path[0]).y})" hidden><circle class="fp-walker-ring" r="14"/><circle class="fp-walker-dot" r="8"/></g>` : '';
  const secLabel = '<rect class="fp-sec-chip" x="442" y="166" width="22" height="78" rx="8"/><text class="fp-zone-label fp-zone-label-v" transform="translate(458 205) rotate(-90)">SECURITY</text>';
  return `${zones}${corridors}${secLabel}${edges}${route}${nodes}${pins}${walker}`;
}
function routeStepsMarkup(path) {
  const steps = path.map((name, i) => {
    if (i === 0) return `Start at ${name}`;
    const meters = edgeWeight(path[i - 1], name);
    return i === path.length - 1 ? `Arrive at ${name} (${meters} m)` : `Walk to ${name} (${meters} m)`;
  });
  return `<ol class="ri-steps">${steps.map((text, i) => `<li data-step="${i}"><span class="ri-dot">${i + 1}</span><span class="ri-text">${esc(text)}</span></li>`).join('')}</ol>`;
}

function routeInfoMarkup(path, meters, walkingMinutes = null) {
  const timeLabel =
    walkingMinutes !== null && walkingMinutes !== undefined
      ? `about ${walkingMinutes} minute${walkingMinutes === 1 ? '' : 's'}`
      : formatWalkingTime(meters);

  return `<p class="ri-route"><strong>Fastest route:</strong> ${esc(path.join(' → '))}</p>
    <dl class="ri-rows">
      <div><dt>Distance</dt><dd>${meters} metres</dd></div>
      <div><dt>Est. Time</dt><dd>${esc(timeLabel)}</dd></div>
      <div><dt>Algorithm</dt><dd>Shortest Path (C++ Dijkstra)</dd></div>
    </dl>
    ${routeStepsMarkup(path)}
    <p class="ri-note">Illustrative map and distances.</p>`;
}

const navRuns = {};
function cancelNavRun(mapId) {
  const run = navRuns[mapId];
  if (run) { run.cancelled = true; run.active = false; cancelAnimationFrame(run.raf); }
}
function navRunActive(mapId) { return !!(navRuns[mapId] && navRuns[mapId].active); }
function runNavigation(mapId, outId, btnId, path) {
  cancelNavRun(mapId);
  const map = document.getElementById(mapId), out = document.getElementById(outId), btn = document.getElementById(btnId);
  const route = map && map.querySelector('.fp-route'), walker = map && map.querySelector('.fp-walker');
  if (!route || !walker || !path || path.length < 2) return;
  const steps = [...out.querySelectorAll('.ri-steps li')];
  const total = route.getTotalLength();
  const cumulative = [0];
  for (let i = 1; i < path.length; i += 1) {
    const a = nodeByName(path[i - 1]), b = nodeByName(path[i]);
    cumulative.push(cumulative[i - 1] + Math.hypot(a.x - b.x, a.y - b.y));
  }
  const run = { active: true, cancelled: false, raf: 0 };
  navRuns[mapId] = run;
  const duration = prefersReducedMotion() ? 0 : Math.min(11000, 3200 + total * 7);
  walker.removeAttribute('hidden');
  map.classList.add('is-navigating');
  if (btn) { btn.disabled = true; btn.classList.add('is-running'); }
  const start = performance.now();
  const frame = (now) => {
    if (run.cancelled) return;
    const t = duration ? Math.min(1, (now - start) / duration) : 1;
    const eased = t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
    const length = eased * total;
    const point = route.getPointAtLength(length);
    walker.setAttribute('transform', `translate(${point.x} ${point.y})`);
    let index = 0;
    cumulative.forEach((value, i) => { if (length >= value - 0.5) index = i; });
    steps.forEach((li, i) => { li.classList.toggle('done', t === 1 || i < index); li.classList.toggle('active', t < 1 && i === index); });
    if (t < 1) { run.raf = requestAnimationFrame(frame); return; }
    run.active = false;
    map.classList.remove('is-navigating'); map.classList.add('has-arrived');
    if (btn) { btn.disabled = false; btn.classList.remove('is-running'); }
    toast('Navigation complete', `You have arrived at ${path[path.length - 1]}.`);
  };
  run.raf = requestAnimationFrame(frame);
}

const NAV_ITEMS = [
  { id: 'dashboard', label: 'Dashboard', icon: 'dashboard' },
  { id: 'flights', label: 'Flights', icon: 'plane' },
  { id: 'passengers', label: 'Passengers', icon: 'users' },
  { id: 'queues', label: 'Check-In & Boarding', icon: 'flow' },
  { id: 'baggage', label: 'Baggage Management', icon: 'bag' },
  { id: 'priority', label: 'AeroAssist', icon: 'lifebuoy' },
  { id: 'groups', label: 'Family & Group Travel', icon: 'group' },
  { id: 'suggestions', label: 'Smart Flight Suggestions', icon: 'sparkle' },
  { id: 'navigator', label: 'Airport Navigator', icon: 'compass' },
  { id: 'emergency', label: 'Emergency Assistance', icon: 'alert' },
  { id: 'notes', label: 'Notifications', icon: 'bell' }
];
const NAV_GROUPS = [
  { label: 'Overview', items: ['dashboard'] },
  { label: 'Operations', items: ['flights', 'passengers', 'queues', 'baggage'] },
  { label: 'Passenger services', items: ['priority', 'groups', 'suggestions', 'navigator', 'notes'] }, 
  { label: 'Safety', items: ['emergency'] }
];

const WALKING_SPEED_METERS_PER_SECOND = 1.2;
const NAV_GRAPH = {
  locations: [
    { name: 'Main Entrance', x: 70, y: 185 },
    { name: 'Check-In Counter', x: 225, y: 85 },
    { name: 'Security Check', x: 390, y: 105 },
    { name: 'Departure Lounge', x: 545, y: 190 },
    { name: 'Gate A1', x: 745, y: 92 },
    { name: 'Gate A2', x: 745, y: 292 },
    { name: 'Medical Centre', x: 255, y: 200 },
    { name: 'Baggage Claim', x: 390, y: 350 },
    { name: 'Restrooms', x: 555, y: 345 },
    // Additional illustrative facilities used by the Emergency Assistance module.
    { name: 'Fire Extinguisher', x: 80, y: 55 },
    { name: 'Security Office', x: 600, y: 42 },
    { name: 'Information & Help Desk', x: 145, y: 350 }
  ],
  // All edges are undirected; weights are illustrative walking minutes.
  // Edge weights are illustrative distances in metres.
edges: [
  ['Main Entrance', 'Check-In Counter', 200],
  ['Check-In Counter', 'Security Check', 350],
  ['Security Check', 'Departure Lounge', 30],
  ['Departure Lounge', 'Gate A1', 250],
  ['Departure Lounge', 'Gate A2', 145],
  ['Main Entrance', 'Medical Centre', 225],
  ['Medical Centre', 'Departure Lounge', 400],
  ['Main Entrance', 'Baggage Claim', 250],
  ['Baggage Claim', 'Restrooms', 150],
  ['Departure Lounge', 'Restrooms', 200],
  ['Medical Centre', 'Baggage Claim', 235],
  ['Main Entrance', 'Fire Extinguisher', 250],
  ['Check-In Counter', 'Fire Extinguisher', 300],
  ['Security Check', 'Security Office', 25],
  ['Departure Lounge', 'Security Office', 130],
  ['Main Entrance', 'Information & Help Desk', 120],
  ['Medical Centre', 'Information & Help Desk', 125]
]
};

// ==============================================
// MULTI-AIRPORT NAVIGATOR SYNCHRONIZATION
// ==============================================

let availableAirports = [];
let currentAirportCode = 'DEL';
let activeRouteMinutes = null;


// Load the airport directory from the C++ API.
async function loadAirportDirectoryFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/airports`);
    const data = await response.json();

    if (!response.ok || !Array.isArray(data) || !data.length) {
      throw new Error(data.message || 'Could not load airport directory.');
    }

    availableAirports = data;

    const preferredAirport = availableAirports.some(
      airport => airport.airportCode === currentAirportCode
    )
      ? currentAirportCode
      : availableAirports[0].airportCode;

    await loadAirportGraphFromApi(preferredAirport);

    debugLog(
      `Loaded ${availableAirports.length} airports successfully.`
    );

  } catch (error) {
    console.error('Airport directory API error:', error);

    toast(
      'Airport loading failed',
      error.message || 'Could not load available airports.',
      true
    );
  }
}


// Load and activate one airport's graph.
async function loadAirportGraphFromApi(airportCode) {
  try {
    const code = String(airportCode || '').trim().toUpperCase();

    if (!code) {
      throw new Error('Select a valid airport.');
    }

    const response = await apiFetch(
      `${API_BASE_URL}/navigator/graph?airportCode=${encodeURIComponent(code)}`
    );

    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || 'Could not load airport graph.');
    }

    if (
      !data.airport ||
      data.airport.airportCode !== code ||
      !Array.isArray(data.locations) ||
      !Array.isArray(data.edges) ||
      !data.locations.length
    ) {
      throw new Error('Invalid airport graph received from the API.');
    }

    const locationNames = new Set(
      data.locations.map(location => location.name)
    );

    // Validate graph edges before replacing the active graph.
    for (const edge of data.edges) {
      if (
        !locationNames.has(edge.from) ||
        !locationNames.has(edge.to) ||
        !Number.isFinite(Number(edge.distanceMeters)) ||
        !Number.isFinite(Number(edge.walkingMinutes))
      ) {
        throw new Error('The airport graph contains an invalid path.');
      }
    }

    // Preserve the existing map layout and drawing functions.
    // Only its data is replaced by the selected airport's database data.
    NAV_GRAPH.locations = data.locations.map(location => ({
      name: location.name,
      type: location.type,
      x: Number(location.x),
      y: Number(location.y)
    }));

    // Third item remains the distance in metres because the
    // existing map and route-step renderer use edge[2] for metres.
    // Fourth item holds walking minutes for reference.
    NAV_GRAPH.edges = data.edges.map(edge => [
      edge.from,
      edge.to,
      Number(edge.distanceMeters),
      Number(edge.walkingMinutes)
    ]);

    NAV_GRAPH.airport = data.airport;
    currentAirportCode = code;

    // Reset the previous route because it belongs to another graph.
    activeRoutePath = [];
    activeRouteWeight = null;
    activeRouteMinutes = null;

    if ($('#navOut')) {
       $('#navOut').innerHTML = ''; }

    const firstLocation = NAV_GRAPH.locations[0].name;

    const firstGate =
      NAV_GRAPH.locations.find(
        location => norm(location.type) === 'gate'
      )?.name || NAV_GRAPH.locations.at(-1).name;

    // Preserve existing selections only when they exist in the new graph.
    if ($('#navFrom') && !locationNames.has($('#navFrom').value)) {
      $('#navFrom').value = firstLocation;
    }

    if ($('#navTo') && !locationNames.has($('#navTo').value)) {
      $('#navTo').value = firstGate;
    }

    if ($('#asLoc') && !locationNames.has($('#asLoc').value)) {
      $('#asLoc').value = firstLocation;
    }

    // Refresh admin interface and both airport dropdowns.
    renderAll();

    // Tell the passenger portal that the shared graph has changed.
    document.dispatchEvent(
      new CustomEvent('airport-graph-updated', {
        detail: { airportCode: code }
      })
    );

    debugLog(
      `Airport graph loaded: ${code} | ` +
      `${data.locations.length} locations | ${data.edges.length} paths`
    );

    return true;

  } catch (error) {
    console.error('Airport graph API error:', error);

    toast(
      'Airport graph loading failed',
      error.message || 'Could not load the selected airport.',
      true
    );

    return false;
  }
}


// Send a shortest-route request to the C++ backend.
async function requestNavigatorRoute(from, to) {
  const response = await apiFetch(
    `${API_BASE_URL}/navigator/route`,
    {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams({
        airportCode: currentAirportCode,
        from,
        to
      }).toString()
    }
  );

  const result = await response.json();

  if (!response.ok) {
    throw new Error(result.message || 'Route calculation failed.');
  }

  return result;
}


// Send BFS or DFS request to the C++ backend.
async function requestNavigatorTraversal(start, algorithm) {
  const response = await apiFetch(
    `${API_BASE_URL}/navigator/traverse`,
    {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams({
        airportCode: currentAirportCode,
        start,
        algorithm: algorithm.toUpperCase()
      }).toString()
    }
  );

  const result = await response.json();

  if (!response.ok) {
    throw new Error(result.message || 'Graph traversal failed.');
  }

  return result;
}


// Return the first location matching a facility type.
function getAirportLocationByType(type) {
  return NAV_GRAPH.locations.find(
    location => norm(location.type) === norm(type)
  ) || null;
}

function demoState() {
  const state = {
    flights: [
      { flightNumber: 'UK 811', airline: 'Vistara', source: 'Delhi', destination: 'Hyderabad', departureTime: '09:50', arrivalTime: '12:05', gate: 'G3', status: 'On Time', fare: 54000 },
      { flightNumber: 'AI 865', airline: 'Air India', source: 'Delhi', destination: 'Bengaluru', departureTime: '10:55', arrivalTime: '13:35', gate: 'G2', status: 'Boarding', fare: 62000 },
      { flightNumber: '6E 2134', airline: 'IndiGo', source: 'Delhi', destination: 'Mumbai', departureTime: '11:40', arrivalTime: '13:50', gate: 'G4', status: 'On Time', fare: 49000 },
      { flightNumber: 'SG 160', airline: 'SpiceJet', source: 'Delhi', destination: 'Kochi', departureTime: '12:25', arrivalTime: '15:10', gate: 'G1', status: 'Delayed', fare: 57000 },
      { flightNumber: 'QP 1342', airline: 'Akasa Air', source: 'Delhi', destination: 'Goa', departureTime: '13:05', arrivalTime: '15:40', gate: 'G1', status: 'Cancelled', fare: 43000 },
      { flightNumber: 'AI 202', airline: 'Air India', source: 'Delhi', destination: 'Mumbai', departureTime: '14:20', arrivalTime: '16:35', gate: 'A1', status: 'On Time', fare: 61000 },
      { flightNumber: 'UK 955', airline: 'Vistara', source: 'Delhi', destination: 'Mumbai', departureTime: '14:30', arrivalTime: '16:40', gate: 'G3', status: 'On Time', fare: 52000 },
      { flightNumber: '6E 6012', airline: 'IndiGo', source: 'Delhi', destination: 'Goa', departureTime: '15:30', arrivalTime: '18:00', gate: 'G3', status: 'On Time', fare: 47000 }
    ],
    passengers: [
      { passengerId: 'P101', name: 'Rahul Sharma', age: 26, gender: 'Male', phoneNumber: '9876543210', flightNumber: 'AI 202', seatNumber: '12A', assistanceType: 'None', status: 'Registered' },
      { passengerId: 'P102', name: 'Aditi Verma', age: 32, gender: 'Female', phoneNumber: '9876543211', flightNumber: 'AI 865', seatNumber: '14C', assistanceType: 'None', status: 'Security-Cleared' },
      { passengerId: 'P103', name: 'Imran Qureshi', age: 68, gender: 'Male', phoneNumber: '9876543212', flightNumber: 'UK 811', seatNumber: '2A', assistanceType: 'Elderly', status: 'Registered' },
      { passengerId: 'P104', name: 'Priya Nair', age: 29, gender: 'Female', phoneNumber: '9876543213', flightNumber: 'SG 160', seatNumber: '18B', assistanceType: 'Wheelchair', status: 'Registered' },
      { passengerId: 'P105', name: 'Karan Mehta', age: 41, gender: 'Male', phoneNumber: '9876543214', flightNumber: 'QP 1342', seatNumber: '9D', assistanceType: 'None', status: 'Registered' }
    ],
    checkinQ: ['P101'],
    boardQ: [{ passengerId: 'P102', gate: 'G2' }],
    priorityQ: [{ passengerId: 'P103', priority: 3, reason: 'Senior passenger assistance', sequence: 1 }],
    processedPriorityIds: [],
    groups: [],
    baggageRecords: [],
    baggageQueue: [],
    nextBaggageNumber: 1,
    emergencyCases: [],
    nextEmergencyCaseNumber: 1,
    notifications: [],
    deletedStack: [],
    nextNotificationId: 1,
    nextPrioritySequence: 2
  };
  const cancelledPassenger = state.passengers.find((p) => p.passengerId === 'P105');
  state.notifications.push({
    notificationId: state.nextNotificationId++, passengerId: cancelledPassenger.passengerId,
    passengerName: cancelledPassenger.name, flightNumber: cancelledPassenger.flightNumber,
    type: 'Cancellation', message: `Hi ${cancelledPassenger.name}, flight ${cancelledPassenger.flightNumber} has been cancelled. Please contact the passenger service desk.`,
    isRead: false, createdAt: new Date().toISOString()
  });
  return state;
}

function loadState() {
  try {
    if (window.sessionStorage && window.sessionStorage.getItem('aerostruct-session-v1')) {
      return demoState();
    }
    const saved = localStorage.getItem(STORAGE_KEY);
    if (!saved) return demoState();
    const parsed = JSON.parse(saved);
    if (!parsed || !Array.isArray(parsed.flights) || !Array.isArray(parsed.passengers)) return demoState();
    const loaded = {
      ...demoState(), ...parsed,
      flights: (parsed.flights || []).map((flight) => ({ ...flight, fare: (flight.fare !== undefined && flight.fare !== null && flight.fare !== '' && Number.isFinite(Number(flight.fare))) ? Number(flight.fare) : defaultFareFor(flight.flightNumber) })),
      passengers: parsed.passengers || [],
      checkinQ: parsed.checkinQ || [], boardQ: parsed.boardQ || [], priorityQ: parsed.priorityQ || [], processedPriorityIds: Array.isArray(parsed.processedPriorityIds)? parsed.processedPriorityIds
  : [],
      groups: Array.isArray(parsed.groups) ? parsed.groups : [],
      emergencyCases: Array.isArray(parsed.emergencyCases) ? parsed.emergencyCases : [],
      nextEmergencyCaseNumber: Number.isSafeInteger(parsed.nextEmergencyCaseNumber) && parsed.nextEmergencyCaseNumber > 0 ? parsed.nextEmergencyCaseNumber : 1,
      notifications: parsed.notifications || [], deletedStack: parsed.deletedStack || [],
      baggageRecords: Array.isArray(parsed.baggageRecords)? parsed.baggageRecords: [],
      baggageQueue: Array.isArray(parsed.baggageQueue)? parsed.baggageQueue: [],
      nextBaggageNumber: Number.isSafeInteger(parsed.nextBaggageNumber) && parsed.nextBaggageNumber > 0 ? parsed.nextBaggageNumber: 1,};
    
    loaded.groups = loaded.groups.map((group) => ({
      groupId: clean(group.groupId), groupName: clean(group.groupName || group.name),
      flightNumber: clean(group.flightNumber),
      members: Array.isArray(group.members) ? group.members.map((member) => typeof member === 'string' ? member : member?.passengerId).filter(Boolean) : []
    }));
    loaded.emergencyCases = loaded.emergencyCases
      .filter((incident) => incident && typeof incident === 'object' && clean(incident.caseId))
      .map((incident) => ({
        ...incident,
        caseId: clean(incident.caseId),
        primaryPassengerId: clean(incident.primaryPassengerId),
        primaryPassengerName: clean(incident.primaryPassengerName),
        flightNumber: clean(incident.flightNumber),
        emergencyType: clean(incident.emergencyType),
        incidentLocation: clean(incident.incidentLocation),
        description: clean(incident.description),
        reportedAt: incident.reportedAt || new Date().toISOString(),
        status: ['Open', 'In Progress', 'Resolved'].includes(incident.status) ? incident.status : 'Open',
        severity: [1, 2, 3].includes(Number(incident.severity)) ? Number(incident.severity) : 2,
        affectedPassengers: Array.isArray(incident.affectedPassengers) ? incident.affectedPassengers.filter((p) => p && clean(p.passengerId)).map((p) => ({ passengerId: clean(p.passengerId), passengerName: clean(p.passengerName || p.name) })) : [],
        recommendedFacility: clean(incident.recommendedFacility),
        route: Array.isArray(incident.route) ? incident.route.filter((location) => typeof location === 'string') : [],
        routeMeters: Number.isFinite(Number(incident.routeMeters)) && incident.routeMeters !== null && incident.routeMeters !== '' ? Number(incident.routeMeters) : null,
        routeMessage: incident.routeMeters == null && Array.isArray(incident.route) && incident.route.length? 'Refresh the facility route to recalculate its distance and walking time.': clean(incident.routeMessage),}));
    const lastCaseNumber = loaded.emergencyCases.reduce((max, incident) => {
      const match = incident.caseId.match(/^EMR(\d+)$/i);
      return match ? Math.max(max, Number(match[1])) : max;
    }, 0);
    loaded.nextEmergencyCaseNumber = Math.max(loaded.nextEmergencyCaseNumber, lastCaseNumber + 1);
    // Migrate saved pre-security-clearance sessions so existing boarding entries remain valid.
    loaded.boardQ.forEach((item) => {
      const passenger = loaded.passengers.find((p) => norm(p.passengerId) === norm(item.passengerId));
      if (passenger && passenger.status === 'Checked-In') passenger.status = 'Security-Cleared';
    });
    return loaded;
  } catch (error) {
    console.warn('Could not load AeroStruct browser state:', error);
    return demoState();
  }
}

let state = loadState();
let temporaryNotificationId = -1;
let currentPage = 'dashboard';
let activeRoutePath = [];
let activeRouteWeight = null;
let lastClockMinute = null;

function saveState() {
  try {
    if (typeof window.getAeroStructToken === 'function' && window.getAeroStructToken()) {
      localStorage.removeItem(STORAGE_KEY);
      return;
    }
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  } catch (error) {
    console.warn('Could not save AeroStruct browser state:', error);
    toast('Storage unavailable', 'Your browser could not save this change. Check local storage permissions.', true);
  }
}


async function loadFlightsFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/flights`);

    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    if (!Array.isArray(data)) {
      throw new Error('Invalid flight data received from API.');
    }

    // Replace browser-saved flight records with MySQL records.
    state.flights = data.map((flight) => ({
      flightNumber: String(flight.flightNumber || '').trim(),
      airline: String(flight.airline || '').trim(),
      source: String(flight.source || '').trim(),
      destination: String(flight.destination || '').trim(),
      departureTime: String(flight.departureTime || ''),
      arrivalTime: String(flight.arrivalTime || ''),
      gate: String(flight.gate || ''),
      status: String(flight.status || 'Scheduled'),
      fare: Number(flight.fare) || 0
    }));

    // Refresh the existing dashboard and flight directory.
    renderAll();

    debugLog(
      `Successfully loaded ${state.flights.length} flights from MySQL.`
    );

  } catch (error) {
    console.error('Flight API error:', error);

    toast(
      'Flight synchronization failed',
      'Could not retrieve flights from MySQL. Check the API server.',
      true
    );
  }
}


async function loadPassengersFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/passengers`);

    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    if (!Array.isArray(data)) {
      throw new Error('Invalid passenger data received from API.');
    }

    // Replace browser-saved passenger records with MySQL records.
    state.passengers = data.map((passenger) => ({
      passengerId: String(passenger.passengerId || '').trim(),
      name: String(passenger.name || '').trim(),
      age: Number(passenger.age),
      gender: String(passenger.gender || ''),
      phoneNumber: String(passenger.phoneNumber || ''),
      flightNumber: String(passenger.flightNumber || '').trim(),
      seatNumber: String(passenger.seatNumber || '').trim(),
      assistanceType: String(passenger.assistanceType || 'None'),
      status: String(passenger.status || 'Registered')
    }));

    // Refresh passenger management and dependent modules.
    renderAll();

    debugLog(
      `Successfully loaded ${state.passengers.length} passengers from MySQL.`
    );

  } catch (error) {
    console.error('Passenger API error:', error);

    toast(
      'Passenger synchronization failed',
      'Could not retrieve passengers from MySQL. Check the API server.',
      true
    );
  }
}

// ==============================================
// LOAD BAGGAGE RECORDS FROM MYSQL
// ==============================================

async function loadBaggageFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/baggage`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    if (!Array.isArray(data.records) || !Array.isArray(data.queue)) {
      throw new Error('Invalid baggage data received from API.');
    }

    state.baggageRecords = data.records.map((bag) => {
      const rawStatus = String(bag.status || 'Registered').trim();
      const rawIssueType = String(bag.issueType || '').trim();
      const rawIssueStatus = String(bag.issueStatus || '').trim();

      return {
        tag: String(bag.tag || bag.baggageTag || '').trim(),
        baggageTag: String(bag.baggageTag || bag.tag || '').trim(),
        passengerId: String(bag.passengerId || '').trim(),
        passengerName: String(bag.passengerName || '').trim(),
        flightNumber: String(bag.flightNumber || '').trim(),
        seatNumber: String(bag.seatNumber || '').trim(),
        weightKg: Number(bag.weightKg) || 0,
        description: String(bag.description || ''),
        registeredAt: String(bag.registeredAt || ''),
        status: norm(rawStatus) === 'queued for handling'
          ? 'Registered'
          : rawStatus,
        issueType: rawIssueType === 'None' ? '' : rawIssueType,
        issueDescription: String(bag.issueDescription || ''),
        issueStatus: rawIssueStatus === 'None' ? '' : rawIssueStatus
      };
    });

    state.baggageQueue = data.queue
      .map((item) => {
        if (typeof item === 'string') return item;

        return String(
          item.tag || item.baggageTag || ''
        );
      })
      .filter(Boolean);

    // Keep the next local tag counter in sync with database records.
    const highestTagNumber = state.baggageRecords.reduce(
      (highest, bag) => {
        const match = bag.tag.match(/^BG(\d+)$/i);

        return match
          ? Math.max(highest, Number(match[1]))
          : highest;
      },
      0
    );

    state.nextBaggageNumber = highestTagNumber + 1;

    renderAll();

    debugLog(
      `Successfully loaded ${state.baggageRecords.length} baggage records from MySQL.`
    );

    return true;

  } catch (error) {
    console.error('Baggage API error:', error);

    toast(
      'Baggage synchronization failed',
      'Could not retrieve baggage records from MySQL.',
      true
    );

    return false;
  }
}


// ==============================================
// SEND BAGGAGE OPERATIONS TO THE C++ API
// ==============================================

async function baggageApiPost(endpoint, fields = {}) {
  let response;

  try {
    response = await apiFetch(`${API_BASE_URL}${endpoint}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
      },
      body: new URLSearchParams(fields)
    });
  } catch (error) {
    throw new Error(
      'Could not connect to the AeroStruct API. Check whether the server is running.'
    );
  }

  const data = await response.json().catch(() => ({}));

  if (!response.ok || data.success === false) {

    // Processing can move unavailable-flight baggage to On Hold,
    // even when the operation returns an unsuccessful response.
    if (endpoint === '/baggage/process-next') {
      await loadBaggageFromApi();
    }

    throw new Error(
      data.message || `Baggage API request failed: HTTP ${response.status}`
    );
  }

  // Refresh records and FIFO queue after every successful mutation.
  await loadBaggageFromApi();

  return data;
}

// ==============================================
// LOAD TRAVEL GROUPS FROM MYSQL
// ==============================================

async function loadGroupsFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/groups`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    // Accept either a plain JSON array or a wrapped array.
    const groups = Array.isArray(data)
      ? data
      : Array.isArray(data.value)
        ? data.value
        : null;

    if (!groups) {
      throw new Error('Invalid travel group data received from API.');
    }

    // MySQL is the source of truth for travel groups.
    state.groups = groups.map((group) => ({
      groupId: String(group.groupId || '').trim(),
      groupName: String(group.groupName || '').trim(),
      flightNumber: String(group.flightNumber || '').trim(),
      members: Array.isArray(group.members)
        ? group.members.map((member) =>
            typeof member === 'string'
              ? member.trim()
              : String(member?.passengerId || '').trim()
          ).filter(Boolean)
        : []
    }));

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.groups.length} travel groups from MySQL.`
    );

  } catch (error) {
    console.error('Travel Group API error:', error);

    toast(
      'Group synchronization failed',
      error.message || 'Could not retrieve travel groups from MySQL.',
      true
    );
  }
}


async function loadCheckInQueueFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/checkin/queue`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    // Support both a plain array and the MySQL response wrapper.
    const entries = Array.isArray(data)
      ? data
      : Array.isArray(data.value)
        ? data.value
        : null;

    if (!entries) {
      throw new Error('Invalid check-in queue data received.');
    }

    // Extract passenger IDs in MySQL FIFO order.
    const queueIds = entries.map((entry) =>
      String(entry.passenger?.passengerId || '').trim()
    );

    if (queueIds.some((id) => !id)) {
      throw new Error('Invalid passenger entry in check-in queue.');
    }

    state.checkinQ = queueIds;

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.checkinQ.length} check-in queue entries from MySQL.`
    );

  } catch (error) {
    console.error('Check-in Queue API error:', error);

    toast(
      'Check-in queue synchronization failed',
      error.message || 'Could not retrieve the check-in queue from MySQL.',
      true
    );
  }
}



// Load Boarding Queue from MySQL.
async function loadBoardingQueueFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/boarding/queue`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    if (!Array.isArray(data)) {
      throw new Error('Invalid Boarding Queue data received.');
    }

    state.boardQ = data.map((entry) => ({
      passengerId: String(entry.passenger?.passengerId || '').trim(),
      gate: String(entry.gate || '').trim()
    }));

    if (state.boardQ.some((item) => !item.passengerId || !item.gate)) {
      throw new Error('Invalid passenger entry in Boarding Queue.');
    }

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.boardQ.length} Boarding Queue entries from MySQL.`
    );

  } catch (error) {
    console.error('Boarding Queue API error:', error);

    toast(
      'Boarding Queue synchronization failed',
      error.message || 'Could not retrieve Boarding Queue from MySQL.',
      true
    );
  }
}


// Load Priority Queue from MySQL.
async function loadPriorityQueueFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/priority/queue`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    if (!Array.isArray(data)) {
      throw new Error('Invalid Priority Queue data received.');
    }

    state.priorityQ = data.map((entry, index) => ({
      passengerId: String(entry.passenger?.passengerId || '').trim(),
      priority: Number(entry.priority),
      reason: String(entry.reason || ''),
      sequence: Number(entry.sequence ?? entry.queuePosition ?? index + 1)
    }));

    if (state.priorityQ.some((item) =>
      !item.passengerId ||
      !Number.isInteger(item.priority) ||
      item.priority < 1 ||
      item.priority > 5 ||
      !Number.isFinite(item.sequence)
    )) {
      throw new Error('Invalid assistance request in Priority Queue.');
    }

    const maxSequence = state.priorityQ.reduce(
      (max, item) => Math.max(max, item.sequence),
      0
    );

    state.nextPrioritySequence = Math.max(
      Number(state.nextPrioritySequence) || 1,
      maxSequence + 1
    );

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.priorityQ.length} Priority Queue entries from MySQL.`
    );

  } catch (error) {
    console.error('Priority Queue API error:', error);

    toast(
      'Priority Queue synchronization failed',
      error.message || 'Could not retrieve Priority Queue from MySQL.',
      true
    );
  }
}
// Load emergency cases from MySQL through EmergencyAssistanceManager.
async function loadEmergencyCasesFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/emergencies`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    const records = Array.isArray(data)
      ? data
      : Array.isArray(data.value)
        ? data.value
        : null;

    if (!records) {
      throw new Error('Invalid emergency case data received from API.');
    }

    // MySQL is the source of truth for emergency cases.
    state.emergencyCases = records.map((incident) => ({
      caseId: String(incident.caseId || ''),
      primaryPassengerId: String(incident.primaryPassengerId || ''),
      primaryPassengerName: String(incident.primaryPassengerName || ''),
      flightNumber: String(incident.flightNumber || ''),
      emergencyType: String(incident.emergencyType || ''),
      severity: Number(incident.severity) || 2,
      incidentLocation: String(incident.incidentLocation || ''),
      description: String(incident.description || ''),
      reportedAt: incident.reportedAt || new Date().toISOString(),
      status: String(incident.status || 'Open'),
      affectedPassengers: Array.isArray(incident.affectedPassengers)
        ? incident.affectedPassengers.map((passenger) => ({
            passengerId: String(passenger.passengerId || ''),
            passengerName: String(passenger.passengerName || '')
          }))
        : [],
      recommendedFacility: String(incident.recommendedFacility || ''),
      route: Array.isArray(incident.route) ? incident.route : [],
      routeMinutes:
        incident.routeMinutes == null ? null : Number(incident.routeMinutes),
      routeMeters:
        incident.routeMeters == null ? null : Number(incident.routeMeters),
      routeMessage: String(incident.routeMessage || '')
    }));

    // Generate the next case ID from the highest case ID in MySQL.
    const highestCaseNumber = state.emergencyCases.reduce((max, incident) => {
      const match = incident.caseId.match(/^EMR(\d+)$/i);
      return match ? Math.max(max, Number(match[1])) : max;
    }, 0);

    state.nextEmergencyCaseNumber = highestCaseNumber + 1;

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.emergencyCases.length} emergency cases from MySQL.`
    );

  } catch (error) {
    console.error('Emergency API error:', error);

    toast(
      'Emergency synchronization failed',
      error.message || 'Could not retrieve emergency cases from MySQL.',
      true
    );
  }
}

// Create a notification in MySQL through NotificationManager.
async function postNotificationRecord(notification) {
  const body = new URLSearchParams({
    passengerId: notification.passengerId,
    flightNumber: notification.flightNumber || '',
    type: notification.type,
    message: notification.message
  });

  const response = await apiFetch(`${API_BASE_URL}/notifications`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
    },
    body: body.toString()
  });

  const data = await response.json();

  if (!response.ok || data.status !== 'success' || !data.notification) {
    throw new Error(data.message || 'Could not save notification.');
  }

  return data.notification;
}


// Load notification history from MySQL.
async function loadNotificationsFromApi() {
  try {
    const response = await apiFetch(`${API_BASE_URL}/notifications`);
    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    let records = Array.isArray(data)
      ? data
      : Array.isArray(data.value)
        ? data.value
        : null;

    if (!records) {
      throw new Error('Invalid notification data received from API.');
    }

    // Preserve old browser-only notifications when the database is
    // completely empty. This also migrates the original demo notice.
    const previousNotifications = [...state.notifications];

    if (records.length === 0 && previousNotifications.length > 0 && !(window.getAeroStructToken && window.getAeroStructToken())) {
      for (const oldNotification of previousNotifications) {
        try {
          await postNotificationRecord(oldNotification);
        } catch (migrationError) {
          console.warn(
            'Could not migrate an old notification:',
            migrationError
          );
        }
      }

      // Re-fetch after migration.
      const refreshedResponse = await apiFetch(
        `${API_BASE_URL}/notifications`
      );

      const refreshedData = await refreshedResponse.json();

      if (!refreshedResponse.ok) {
        throw new Error(
          refreshedData.message || 'Could not reload notification history.'
        );
      }

      records = Array.isArray(refreshedData)
        ? refreshedData
        : Array.isArray(refreshedData.value)
          ? refreshedData.value
          : null;

      if (!records) {
        throw new Error('Invalid notification history received.');
      }
    }

    state.notifications = records.map((notification) => ({
      notificationId: Number(notification.notificationId),
      passengerId: String(notification.passengerId || ''),
      passengerName: String(notification.passengerName || ''),
      flightNumber: String(notification.flightNumber || ''),
      type: String(notification.type || 'Flight Status'),
      message: String(notification.message || ''),
      isRead: Boolean(notification.isRead),
      createdAt: notification.createdAt || new Date().toISOString()
    }));

    const highestNotificationId = state.notifications.reduce(
      (max, notification) =>
        Math.max(max, Number(notification.notificationId) || 0),
      0
    );

    state.nextNotificationId = highestNotificationId + 1;

    saveState();
    renderAll();

    debugLog(
      `Successfully loaded ${state.notifications.length} notifications from MySQL.`
    );

  } catch (error) {
    console.error('Notification API error:', error);

    toast(
      'Notification synchronization failed',
      error.message || 'Could not retrieve notification history from MySQL.',
      true
    );
  }
}

function esc(value) {
  return String(value ?? '').replace(/[&<>"']/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
}
function norm(value) { return String(value ?? '').trim().toLocaleLowerCase(); }
function clean(value) { return String(value ?? '').trim(); }
function byId(list, id, key = 'passengerId') { return list.find((item) => norm(item[key]) === norm(id)); }
function getFlight(flightNumber) { return byId(state.flights, flightNumber, 'flightNumber'); }
function getPassenger(passengerId) { return byId(state.passengers, passengerId); }
function activeFlightForPassenger(passenger) {
  if (!passenger) return null;
  const flight = getFlight(passenger.flightNumber);
  if (!flight || isUnavailableFlight(flight.status)) return null;
  return flight;
}
function isCancelled(status) { return ['cancelled', 'canceled'].includes(norm(status)); }
function isDelayed(status) { return norm(status).includes('delay'); }
function timeNumber(value) {
  if (!/^\d{2}:\d{2}$/.test(String(value))) return Number.MAX_SAFE_INTEGER;
  const [hours, minutes] = value.split(':').map(Number);
  return hours * 60 + minutes;
}
function defaultFareFor(flightNumber) {
  const fares = { 'UK 811': 5400, 'AI 865': 6200, '6E 2134': 4900, 'SG 160': 5700, 'QP 1342': 4300, 'AI 202': 6100, 'UK 955': 5200, '6E 6012': 4700 };
  return fares[String(flightNumber || '').toUpperCase()] ?? 0;
}
function flightDurationMinutes(flight) {
  if (!flight || !/^([01]\d|2[0-3]):[0-5]\d$/.test(String(flight.departureTime)) || !/^([01]\d|2[0-3]):[0-5]\d$/.test(String(flight.arrivalTime))) return null;
  const departure = timeNumber(flight.departureTime);
  let arrival = timeNumber(flight.arrivalTime);
  if (arrival < departure) arrival += 24 * 60;
  return arrival - departure;
}
function formatDuration(minutes) {
  if (minutes === null || !Number.isFinite(minutes)) return 'Duration unavailable';
  const hours = Math.floor(minutes / 60);
  const mins = minutes % 60;
  return hours ? `${hours}h ${mins}m` : `${mins}m`;
}
function formatFare(value) {
  return new Intl.NumberFormat('en-IN', { style: 'currency', currency: 'INR', maximumFractionDigits: 0 }).format(Number(value) || 0);
}
function isUnavailableFlight(status) { return isCancelled(status) || norm(status) === 'completed'; }
function statusClass(status) {
  const value = norm(status);
  if (isCancelled(value)) return 'bad';
  if (isDelayed(value) || value === 'boarding') return 'warn';
  if (value === 'checked-in' || value === 'security-cleared' || value === 'boarded' || value === 'on time') return 'ok';
  return 'info';
}
const FLIGHT_STATUS_CHOICES = ['On Time', 'Boarding', 'Delayed', 'Cancelled'];
function flightStatusOptions(current) {
  const list = FLIGHT_STATUS_CHOICES.includes(current) ? FLIGHT_STATUS_CHOICES : [current, ...FLIGHT_STATUS_CHOICES];
  return list.map((value) => `<option value="${esc(value)}"${value === current ? ' selected' : ''}>${esc(value)}</option>`).join('');
}
function statusBadge(status) { return `<span class="status-badge ${statusClass(status)}">${esc(status)}</span>`; }
function emptyState(title, detail) { return `<div class="empty-state"><span class="empty-icon">${icon('inbox', 26)}</span><strong>${esc(title)}</strong>${esc(detail || '')}</div>`; }
function optionMarkup(items, selected, placeholder) {
  let html = placeholder ? `<option value="">${esc(placeholder)}</option>` : '';
  html += items.map((item) => `<option value="${esc(item.value)}" ${norm(item.value) === norm(selected) ? 'selected' : ''}>${esc(item.label)}</option>`).join('');
  return html;
}
function fillSelect(id, items, placeholder = 'Choose an option', selectedValue = undefined) {
  const select = document.getElementById(id);
  if (!select) return;
  const previous = selectedValue !== undefined ? selectedValue : select.value;
  select.innerHTML = optionMarkup(items, previous, placeholder);
  if (previous && !items.some((item) => norm(item.value) === norm(previous))) select.value = '';
}
function notify(passenger, type, message, flightNumberOverride = '') {
  if (!passenger) return;

  // Use a temporary negative ID until MySQL assigns the real ID.
  const temporaryId = temporaryNotificationId--;

  const notification = {
    notificationId: temporaryId,
    passengerId: passenger.passengerId,
    passengerName: passenger.name,
    flightNumber: flightNumberOverride || passenger.flightNumber,
    type,
    message,
    isRead: false,
    createdAt: new Date().toISOString()
  };

  state.notifications.unshift(notification);

  saveState();
  renderAll();

  // Persist asynchronously so existing operations remain synchronous.
  postNotificationRecord(notification)
    .then((savedNotification) => {
      const index = state.notifications.findIndex(
        (item) => Number(item.notificationId) === temporaryId
      );

      const saved = {
        notificationId: Number(savedNotification.notificationId),
        passengerId: savedNotification.passengerId,
        passengerName: savedNotification.passengerName,
        flightNumber: savedNotification.flightNumber,
        type: savedNotification.type,
        message: savedNotification.message,
        isRead: Boolean(savedNotification.isRead),
        createdAt: savedNotification.createdAt
      };

      if (index >= 0) {
        state.notifications[index] = saved;
      } else if (!state.notifications.some(
        (item) => Number(item.notificationId) === saved.notificationId
      )) {
        state.notifications.unshift(saved);
      }

      state.nextNotificationId = Math.max(
        Number(state.nextNotificationId) || 1,
        saved.notificationId + 1
      );

      saveState();
      renderAll();
    })
    .catch((error) => {
      console.error('Notification persistence error:', error);

      // Do not keep a notification that failed to reach MySQL.
      state.notifications = state.notifications.filter(
        (item) => Number(item.notificationId) !== temporaryId
      );

      saveState();
      renderAll();

      toast(
        'Notification could not be saved',
        error.message || 'MySQL notification persistence failed.',
        true
      );
    });
}
function notifyFlightPassengers(flightNumber, type, messageFactory) {
  const affected = state.passengers.filter((p) => norm(p.flightNumber) === norm(flightNumber));
  affected.forEach((p) => notify(p, type, messageFactory(p)));
  return affected.length;
}
function removeFlightFromQueues(flightNumber) {
  const beforeCheckin = state.checkinQ.length;
  state.checkinQ = state.checkinQ.filter((id) => {
    const p = getPassenger(id); return !p || norm(p.flightNumber) !== norm(flightNumber);
  });
  const beforeBoard = state.boardQ.length;
  state.boardQ = state.boardQ.filter((item) => {
    const p = getPassenger(item.passengerId); return !p || norm(p.flightNumber) !== norm(flightNumber);
  });
  const beforePriority = state.priorityQ.length;
  state.priorityQ = state.priorityQ.filter((item) => {
    const p = getPassenger(item.passengerId); return !p || norm(p.flightNumber) !== norm(flightNumber);
  });
  return {
    checkin: beforeCheckin - state.checkinQ.length,
    boarding: beforeBoard - state.boardQ.length,
    priority: beforePriority - state.priorityQ.length
  };
}
function toast(title, message, isError = false) {
  const region = $('#toastRegion');
  if (!region) return;
  const node = document.createElement('div');
  node.className = `toast${isError ? ' error' : ''}`;
  node.setAttribute('role', isError ? 'alert' : 'status');
  node.innerHTML = `<span class="toast-icon">${icon(isError ? 'alert' : 'check', 18)}</span><div class="toast-copy"><strong>${esc(title)}</strong><p>${esc(message)}</p></div><button type="button" class="toast-close" aria-label="Dismiss notification">${icon('close', 14)}</button><i class="toast-progress"></i>`;
  region.appendChild(node);
  while (region.children.length > 4) region.firstElementChild.remove();
  let removed = false;
  const dismiss = () => {
    if (removed) return;
    removed = true;
    node.classList.add('leaving');
    window.setTimeout(() => node.remove(), 240);
  };
  node.querySelector('.toast-close').addEventListener('click', dismiss);
  window.setTimeout(dismiss, 4300);
}
/* ---------- Styled dialogs (replace native prompt and confirm boxes) ---------- */
function dialogFieldMarkup(field) {
  const attrs = [
    `name="${esc(field.name)}"`,
    `data-label="${esc(field.label)}"`,
    field.required === false ? '' : 'required',
    field.min != null ? `min="${esc(field.min)}"` : '',
    field.max != null ? `max="${esc(field.max)}"` : '',
    field.step != null ? `step="${esc(field.step)}"` : '',
    field.minlength ? `minlength="${esc(field.minlength)}"` : '',
    field.maxlength ? `maxlength="${esc(field.maxlength)}"` : '',
    field.pattern ? `pattern="${esc(field.pattern)}"` : '',
    field.placeholder ? `placeholder="${esc(field.placeholder)}"` : '',
    `autocomplete="${esc(field.autocomplete || 'off')}"`
  ].filter(Boolean).join(' ');
  const control = Array.isArray(field.options)
    ? `<select ${attrs}>${field.options.map((option) => {
        const value = typeof option === 'object' ? option.value : option;
        const label = typeof option === 'object' ? option.label : option;
        return `<option value="${esc(value)}"${String(value) === String(field.value) ? ' selected' : ''}>${esc(label)}</option>`;
      }).join('')}</select>`
    : `<input type="${esc(field.type || 'text')}" value="${esc(field.value ?? '')}" ${attrs}>`;
  return `<label class="${field.full ? 'ui-span-2' : ''}">${esc(field.label)}${control}${field.hint ? `<span class="hint">${esc(field.hint)}</span>` : ''}</label>`;
}

// Opens an accessible modal. Resolves with the field values (or true when
// there are no fields) on submit, and with null when it is dismissed.
function uiDialog({ title, subtitle = '', message = '', fields = null, submitLabel = 'Save', cancelLabel = 'Cancel', tone = 'primary', iconName = 'info' }) {
  return new Promise((resolve) => {
    const dialog = document.createElement('dialog');
    dialog.className = `ui-dialog tone-${tone}`;
    dialog.setAttribute('aria-labelledby', 'uiDialogTitle');
    dialog.innerHTML = `<form class="ui-dialog-card" novalidate>
      <div class="ui-dialog-head">
        <span class="ui-dialog-icon" aria-hidden="true">${icon(iconName, 20)}</span>
        <div class="ui-dialog-titles"><h2 id="uiDialogTitle">${esc(title)}</h2>${subtitle ? `<p>${esc(subtitle)}</p>` : ''}</div>
        <button type="button" class="ui-dialog-x" data-dialog-cancel aria-label="Close">${icon('close', 16)}</button>
      </div>
      ${message ? `<p class="ui-dialog-message">${esc(message)}</p>` : ''}
      ${fields ? `<div class="ui-dialog-fields">${fields.map(dialogFieldMarkup).join('')}</div>` : ''}
      <p class="ui-dialog-error" role="alert" hidden></p>
      <div class="ui-dialog-actions">
        <button type="button" class="button button-outline" data-dialog-cancel>${esc(cancelLabel)}</button>
        <button type="submit" class="button ${tone === 'danger' ? 'button-danger-solid' : 'button-primary'}">${esc(submitLabel)}</button>
      </div>
    </form>`;
    document.body.appendChild(dialog);

    let settled = false;
    const finish = (value) => {
      if (settled) return;
      settled = true;
      dialog.classList.add('is-closing');
      window.setTimeout(() => { if (dialog.open) dialog.close(); dialog.remove(); }, prefersReducedMotion() ? 0 : 170);
      resolve(value);
    };
    dialog.addEventListener('cancel', (event) => { event.preventDefault(); finish(null); });
    dialog.addEventListener('click', (event) => {
      if (event.target === dialog || event.target.closest('[data-dialog-cancel]')) finish(null);
    });
    dialog.addEventListener('input', (event) => event.target.classList.remove('invalid'));

    const form = dialog.querySelector('form');
    form.addEventListener('submit', (event) => {
      event.preventDefault();
      if (!fields) { finish(true); return; }
      const invalid = [...form.elements].find((el) => el.willValidate && !el.checkValidity());
      const errorBox = form.querySelector('.ui-dialog-error');
      if (invalid) {
        invalid.classList.add('invalid');
        invalid.focus();
        errorBox.textContent = `${invalid.dataset.label}: ${invalid.validationMessage}`;
        errorBox.hidden = false;
        return;
      }
      const values = {};
      fields.forEach((field) => { values[field.name] = form.elements[field.name].value; });
      finish(values);
    });

    dialog.showModal();
    const first = dialog.querySelector('.ui-dialog-fields input, .ui-dialog-fields select') || dialog.querySelector('[type="submit"]');
    if (first) first.focus();
  });
}

async function askConfirm(message, { title = 'Please confirm', confirmLabel = 'Confirm', tone = 'danger' } = {}) {
  const result = await uiDialog({ title, message, submitLabel: confirmLabel, cancelLabel: 'Go back', tone, iconName: tone === 'danger' ? 'alert' : 'info' });
  return result === true;
}
function setCustomError(element, message) {
  element.setCustomValidity(message || '');
}

function navBadge(id) {
  let count = 0; let tone = '';
  if (id === 'notes') count = unreadCount();
  else if (id === 'emergency') { count = state.emergencyCases.filter((incident) => incident.status !== 'Resolved').length; tone = ' nav-count-alert'; }
  else if (id === 'queues') { count = state.checkinQ.length + state.boardQ.length; tone = ' nav-count-soft'; }
  return count ? `<span class="nav-count${tone}">${count}</span>` : '';
}
function renderNav() {
  const nav = $('#nav');
  const focusedPage = document.activeElement && nav.contains(document.activeElement) ? document.activeElement.dataset.page : null;
  nav.innerHTML = NAV_GROUPS.map((group) => `<div class="nav-group"><div class="nav-group-label">${esc(group.label)}</div>${group.items.map((id) => {
    const item = NAV_ITEMS.find((entry) => entry.id === id);
    const active = currentPage === item.id;
    return `<button type="button" data-page="${item.id}" class="${active ? 'active' : ''}" aria-current="${active ? 'page' : 'false'}"><span class="nav-icon" aria-hidden="true">${icon(item.icon, 20)}</span><span class="nav-label">${esc(item.label)}</span>${navBadge(item.id)}</button>`;
  }).join('')}</div>`).join('');
  if (focusedPage) nav.querySelector(`[data-page="${focusedPage}"]`)?.focus({ preventScroll: true });
}
function dateLabel() {
  return new Intl.DateTimeFormat('en-IN', { weekday: 'long', day: 'numeric', month: 'long' }).format(new Date());
}
function greeting() {
  const hour = new Date().getHours();
  return hour < 12 ? 'Good morning' : hour < 17 ? 'Good afternoon' : 'Good evening';
}
function updateClock() {
  const now = new Date();
  const hhmm = now.toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit', hour12: false });
  const [hh, mm] = hhmm.split(':');
  const chip = $('#clock');
  if (chip) chip.textContent = `${dateLabel()} · ${now.toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' })}`;
  const userChip = $('#uClock');
  if (userChip) userChip.textContent = `${dateLabel()} · ${now.toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' })}`;
  const boardTime = $('#boardTime');
  if (boardTime) boardTime.innerHTML = `${esc(hh)}<span class="colon">:</span>${esc(mm)}`;
  if (lastClockMinute !== mm) {
    const firstTick = lastClockMinute === null;
    lastClockMinute = mm;
    if (!firstTick) {
      renderNextDeparture();
      if (typeof window.onMinuteTick === 'function') window.onMinuteTick();
    }
  }
}
function headingInfo() {
  const headings = {
    dashboard: { kicker: `${dateLabel()} · Terminal 3, Delhi`, title: `${greeting()}, Ops Desk`, description: 'A clear view of flights, passenger flow, and operational issues that need attention.' },
    flights: { kicker: 'OPERATIONS · FLIGHT MANAGEMENT', title: 'Flight operations', description: 'Add, search, sort, update, and cancel flights, with fares and estimated flight durations shown for comparison.' },
    passengers: { kicker: 'OPERATIONS · PASSENGER RECORDS', title: 'Passenger management', description: 'Maintain passenger records, validate flight assignments, and undo the most recent deletion.' },
    queues: { kicker: 'OPERATIONS · PASSENGER FLOW', title: 'Check-in, security & boarding', description: 'Process FIFO check-in and boarding queues through the Registered → Checked-In → Security-Cleared → Boarded journey.' },
    groups: { kicker: 'PASSENGER SERVICES · FAMILY & GROUP TRAVEL', title: 'Family & group travel', description: 'Create travel groups, add passengers assigned to the same flight, and view live check-in and boarding progress.' },
    baggage: { kicker: 'OPERATIONS · BAGGAGE TRACKING', title: 'Baggage Management', description: 'Register, track, process, and manage baggage issues.'},
    priority: {kicker: 'PASSENGER SERVICES · AEROASSIST', title: 'AeroAssist', description: 'Manage assistance requests, prioritize passengers by urgency, and generate intelligent support recommendations.'},
    suggestions: { kicker: 'PASSENGER SERVICES · SMART FLIGHT SUGGESTIONS', title: 'Alternative flight finder', description: 'Rank active alternatives on the same route using fare and flight duration, with the highest-scoring option highlighted.' },
    assist: { kicker: 'PASSENGER SERVICES · AEROASSIST', title: 'Passenger assistance', description: 'Get a rule-based support recommendation from the passenger’s recorded assistance requirement.' },
    navigator: { kicker: 'PASSENGER SERVICES · AIRPORT NAVIGATOR', title: 'Airport Navigator', description: 'Explore the sample terminal graph and find lowest-time walking routes with Dijkstra’s algorithm.' },
    emergency: { kicker: 'SAFETY & PASSENGER SERVICES · INCIDENT MANAGEMENT', title: 'Emergency Assistance', description: 'Register simulated emergency cases, identify affected passengers, prioritise by severity, update case status, and route to the nearest mapped suitable facility.' },
    notes: { kicker: 'PASSENGER SERVICES · NOTIFICATION HISTORY', title: 'Personalized notifications', description: 'Review passenger notices and check whether a connecting journey has enough transfer buffer.' }
  };
  return headings[currentPage] || headings.dashboard;
}
function setHeading() {
  const h = headingInfo();
  $('#breadcrumbCurrent').textContent = h.title.replace(/^Good (morning|afternoon|evening), /, '');
  const actions = currentPage === 'flights' ? `<button class="button button-primary" data-action="focus-flight-form" type="button">＋ Add flight</button>`
    : currentPage === 'passengers' ? `<button class="button button-primary" data-action="focus-passenger-form" type="button">＋ Register passenger</button>`
    : currentPage === 'dashboard' ? `<button class="button button-outline" data-page="notes" type="button">${icon('bell', 16)} Notifications (${unreadCount()})</button><button class="button button-primary" data-page="flights" type="button">＋ New flight</button>` : '';
  $('#pageHeading').innerHTML = bannerHtml(currentPage, `<div class="page-kicker"><span class="kicker-marker"></span>${esc(h.kicker)}</div><div class="page-title-row"><div><h1 class="page-title">${esc(h.title)}</h1><p class="page-description">${esc(h.description)}</p></div><div class="heading-actions">${actions}</div></div>`);
}
function showPage(page) {
  if (!NAV_ITEMS.some((item) => item.id === page)) return;
  const changed = page !== currentPage;
  currentPage = page;
  $$('#adminApp .view').forEach((view) => { view.hidden = view.id !== page; });
  document.body.classList.remove('nav-open');
  if (changed && page === 'dashboard') donutAnimated = false;
  renderNav(); setHeading(); window.scrollTo({ top: 0, behavior: 'smooth' });
  if (page === 'dashboard') renderDashboard();
  const heading = $('#pageHeading');
  if (heading) { heading.classList.remove('heading-enter'); void heading.offsetWidth; heading.classList.add('heading-enter'); }
  if (window.innerWidth > 960) {
    if (page === 'flights') $('#fNo')?.focus({ preventScroll: true });
    if (page === 'passengers') $('#pId')?.focus({ preventScroll: true });
  }
}

function renderStats() {
  const activeFlights = state.flights.filter((flight) => !isUnavailableFlight(flight.status)).length;
  const waiting = state.checkinQ.length + state.boardQ.length;
  const stats = [
    { key: 'flights', label: 'Scheduled flights', value: state.flights.length, meta: `${activeFlights} active · ${state.flights.length - activeFlights} cancelled`, icon: 'plane', tone: 'green' },
    { key: 'passengers', label: 'Passenger records', value: state.passengers.length, meta: `${state.passengers.filter((p) => p.status === 'Checked-In').length} checked in`, icon: 'users', tone: 'blue' },
    { key: 'queues', label: 'Queue entries', value: waiting, meta: `${state.checkinQ.length} check-in · ${state.boardQ.length} boarding`, icon: 'flow', tone: 'gold' },
    { key: 'notices', label: 'Unread notices', value: unreadCount(), meta: `${state.priorityQ.length} assistance requests`, icon: 'bell', tone: 'red' }
  ];
  $('#stats').innerHTML = stats.map((s) => `<div class="stat-card tone-${s.tone}"><div class="stat-top"><span>${esc(s.label)}</span><span class="stat-icon">${icon(s.icon, 20)}</span></div><div class="stat-value" data-count="${s.value}" data-count-key="stat-${s.key}">${s.value}</div><div class="stat-meta">${esc(s.meta)}</div></div>`).join('');
  animateCounters($('#stats'));
}
const boardMemory = {};
function renderDepartures() {
  const flights = [...state.flights].sort((a, b) => timeNumber(a.departureTime) - timeNumber(b.departureTime));
  const rows = flights.map((f) => {
    const signature = `${f.status}|${f.gate}|${f.departureTime}`;
    const changed = f.flightNumber in boardMemory && boardMemory[f.flightNumber] !== signature;
    boardMemory[f.flightNumber] = signature;
    return `<tr class="${changed ? 'row-flash' : ''}"><td>${esc(f.departureTime)}</td><td class="flight-cell">${esc(f.flightNumber)}<span class="cell-sub">${esc(f.airline)}</span></td><td>${esc(f.destination.toUpperCase())}</td><td>${esc(f.gate)}</td><td>${statusBadge(f.status)}</td></tr>`;
  }).join('');
  $('#depTable').innerHTML = `<thead><tr><th>Time</th><th>Flight</th><th>To</th><th>Gate</th><th>Status</th></tr></thead><tbody>${flights.length ? rows : `<tr><td colspan="5">No flights scheduled.</td></tr>`}</tbody>`;
}
function attentionItems() {
  const items = [];
  state.flights.filter((f) => isCancelled(f.status)).forEach((f) => {
    const passengerCount = state.passengers.filter((p) => norm(p.flightNumber) === norm(f.flightNumber)).length;
    items.push({ kind: 'bad', title: `${f.flightNumber} to ${f.destination} is cancelled`, detail: `${passengerCount} passenger(s) may need rebooking.` , page: 'flights' });
  });
  state.flights.filter((f) => isDelayed(f.status)).forEach((f) => items.push({ kind: 'warn', title: `${f.flightNumber} to ${f.destination} is delayed`, detail: `Scheduled departure ${f.departureTime} · Gate ${f.gate}.`, page: 'flights' }));
  const waitingPassengers = state.boardQ.map((q) => getPassenger(q.passengerId)).filter(Boolean);
  waitingPassengers.forEach((p) => {
    const f = getFlight(p.flightNumber);
    items.push({ kind: 'warn', title: `Boarding queue: ${p.name}`, detail: `Waiting for ${p.flightNumber} at gate ${state.boardQ.find((q) => q.passengerId === p.passengerId)?.gate || f?.gate || '—'}.`, page: 'queues' });
  });
  state.notifications.filter((n) => !n.isRead).slice(0, 3).forEach((n) => items.push({ kind: n.type === 'Cancellation' ? 'bad' : 'info', title: n.type, detail: `${n.passengerName} · ${n.flightNumber}`, page: 'notes' }));
  return items.slice(0, 7);
}
function renderAttention() {
  const items = attentionItems();
  $('#attentionCount').textContent = items.length;
  $('#alerts').innerHTML = items.length ? items.map((item) => `<button class="attention-item" type="button" data-page="${item.page}"><span class="attention-symbol ${item.kind}">${item.kind === 'bad' ? '!' : item.kind === 'warn' ? '▲' : 'i'}</span><span class="attention-copy"><strong>${esc(item.title)}</strong><small>${esc(item.detail)}</small></span><span class="attention-arrow">→</span></button>`).join('') : emptyState('All clear', 'No immediate operational alerts.');
}
function renderJourneySnapshot() {
  const total = Math.max(1, state.passengers.length);
  const steps = [
    { title: 'Registered', value: state.passengers.filter((p) => p.status === 'Registered').length, note: 'Awaiting check-in or queue processing', color: 'var(--navy)' },
    { title: 'Checked-In', value: state.passengers.filter((p) => p.status === 'Checked-In').length, note: 'Awaiting security clearance', color: 'var(--green)' },
    { title: 'Security-Cleared', value: state.passengers.filter((p) => p.status === 'Security-Cleared').length, note: 'Ready for boarding', color: 'var(--blue)' },
    { title: 'Boarded', value: state.passengers.filter((p) => p.status === 'Boarded').length, note: 'Journey process completed', color: 'var(--navy)' }
  ];
  $('#journeySnapshot').innerHTML = steps.map((s, index) => `<div class="journey-step"><strong style="color:${s.color}" data-count="${s.value}" data-count-key="journey-${index}">${s.value}</strong><b>${esc(s.title)}</b><span>${esc(s.note)}</span><i class="journey-bar" aria-hidden="true"><i style="width:${Math.round((s.value / total) * 100)}%;background:${s.color}"></i></i></div>`).join('');
  animateCounters($('#journeySnapshot'));
}
let donutAnimated = false;
function renderStatusDonut() {
  const host = $('#statusDonut');
  if (!host) return;
  const groups = [['On Time', 'var(--green)'], ['Boarding', 'var(--blue)'], ['Delayed', 'var(--orange)'], ['Cancelled', 'var(--red)']];
  const counts = groups.map(([label, color]) => ({ label, color, value: state.flights.filter((f) => norm(f.status) === norm(label)).length }));
  const other = state.flights.length - counts.reduce((sum, item) => sum + item.value, 0);
  if (other > 0) counts.push({ label: 'Other', color: 'var(--muted-light)', value: other });
  const total = state.flights.length;
  const radius = 42; const circumference = 2 * Math.PI * radius;
  let offset = 0;
  const segments = total ? counts.filter((item) => item.value > 0).map((item) => {
    const length = (item.value / total) * circumference;
    const markup = `<circle class="donut-seg" cx="60" cy="60" r="${radius}" fill="none" stroke-width="14" style="stroke:${item.color}" stroke-dasharray="${length.toFixed(2)} ${(circumference - length).toFixed(2)}" stroke-dashoffset="${(-offset).toFixed(2)}" transform="rotate(-90 60 60)"><title>${esc(item.label)}: ${item.value}</title></circle>`;
    offset += length;
    return markup;
  }).join('') : '';
  host.innerHTML = `<div class="donut"><svg viewBox="0 0 120 120" role="img" aria-label="Flights by status"><circle cx="60" cy="60" r="${radius}" fill="none" stroke-width="14" class="donut-track"/>${segments}</svg><div class="donut-center"><strong>${total}</strong><span>flights</span></div></div><ul class="donut-legend">${counts.map((item) => `<li><i style="background:${item.color}"></i><span>${esc(item.label)}</span><strong>${item.value}</strong></li>`).join('')}</ul>`;
  host.classList.toggle('donut-animate', !donutAnimated);
  donutAnimated = true;
}
function renderNextDeparture() {
  const host = $('#nextDeparture');
  if (!host) return;
  const now = nowMinutes();
  const candidates = state.flights
    .filter((f) => !isUnavailableFlight(f.status) && timeNumber(f.departureTime) !== Number.MAX_SAFE_INTEGER)
    .sort((a, b) => timeNumber(a.departureTime) - timeNumber(b.departureTime));
  const next = candidates.find((f) => timeNumber(f.departureTime) >= now);
  if (!next) {
    host.classList.add('hero-idle');
    host.innerHTML = `<div class="hero-main"><span class="eyebrow">NEXT DEPARTURE</span><h2>No more departures today</h2><p>Every active flight on the demo schedule has already left. Add a flight to see the live countdown.</p></div><div class="hero-actions"><button class="button button-secondary" data-page="flights" type="button">Manage flights</button></div>`;
    return;
  }
  host.classList.remove('hero-idle');
  const diff = timeNumber(next.departureTime) - now;
  const progress = Math.max(0, Math.min(100, Math.round(((120 - Math.min(diff, 120)) / 120) * 100)));
  const pax = state.passengers.filter((p) => norm(p.flightNumber) === norm(next.flightNumber));
  const checkedIn = pax.filter((p) => ['checked-in', 'security-cleared', 'boarded'].includes(norm(p.status))).length;
  const boarded = pax.filter((p) => p.status === 'Boarded').length;
  host.innerHTML = `<div class="hero-main"><span class="eyebrow">NEXT DEPARTURE</span>
    <h2><span class="hero-flight">${esc(next.flightNumber)}</span> ${esc(next.source)} <span class="hero-arrow">→</span> ${esc(next.destination)}</h2>
    <p>${esc(next.airline)} · Gate ${esc(next.gate)} · Departs <strong>${esc(next.departureTime)}</strong> ${statusBadge(next.status)}</p>
    <div class="hero-track" role="img" aria-label="Time until departure"><i class="hero-fill" style="width:${progress}%"></i><span class="hero-plane" style="left:${progress}%">${icon('plane', 18)}</span></div>
    <div class="hero-track-labels"><span>2 h before departure</span><strong>${esc(formatCountdown(diff))}</strong></div></div>
    <div class="hero-stats"><div><strong>${pax.length}</strong><span>Passengers</span></div><div><strong>${checkedIn}</strong><span>Checked in</span></div><div><strong>${boarded}</strong><span>Boarded</span></div></div>
    <div class="hero-actions"><button class="button button-secondary" data-page="queues" type="button">Open queues</button><button class="button button-ghost" data-page="flights" type="button">Manage flights</button></div>`;
}
function renderDashboard() { renderNextDeparture(); renderStats(); renderDepartures(); renderAttention(); renderJourneySnapshot(); renderStatusDonut(); }

function renderFlights() {
  const term = norm($('#fSearch').value);
  const sort = $('#fSort').value;
  let flights = state.flights.filter((f) => [f.flightNumber, f.airline, f.source, f.destination, f.status, f.gate, f.fare].some((v) => norm(v).includes(term)));
  flights.sort(sort === 'no' ? (a, b) => a.flightNumber.localeCompare(b.flightNumber) : sort === 'to' ? (a, b) => a.destination.localeCompare(b.destination) : sort === 'fare' ? (a, b) => Number(a.fare) - Number(b.fare) : sort === 'duration' ? (a, b) => (flightDurationMinutes(a) ?? Number.MAX_SAFE_INTEGER) - (flightDurationMinutes(b) ?? Number.MAX_SAFE_INTEGER) : (a, b) => timeNumber(a.departureTime) - timeNumber(b.departureTime));
  $('#flightCount').textContent = `${state.flights.length} flight${state.flights.length === 1 ? '' : 's'}`;
  $('#flightTable').innerHTML = `<thead><tr><th>Departure</th><th>Flight</th><th>Airline / Route</th><th>Duration</th><th>Fare</th><th>Gate</th><th>Status</th><th>Actions</th></tr></thead><tbody>${flights.length ? flights.map((f) => `<tr><td>${esc(f.departureTime)}<span class="cell-sub">Arrives ${esc(f.arrivalTime)}</span></td><td class="cell-strong">${esc(f.flightNumber)}</td><td>${esc(f.airline)}<span class="cell-sub">${esc(f.source)} → ${esc(f.destination)}</span></td><td>${esc(formatDuration(flightDurationMinutes(f)))}</td><td class="fare-cell">${esc(formatFare(f.fare))}</td><td>${esc(f.gate)}</td><td>${statusBadge(f.status)}</td><td><div class="table-actions"><label class="status-select"><span class="sr-only">Status for ${esc(f.flightNumber)}</span><select data-flight-status-select data-id="${esc(f.flightNumber)}">${flightStatusOptions(f.status)}</select></label><button class="table-action icon-only" data-action="edit-flight" data-id="${esc(f.flightNumber)}" type="button" title="Edit ${esc(f.flightNumber)}" aria-label="Edit ${esc(f.flightNumber)}">${icon('edit', 15)}</button><button class="table-action icon-only danger" data-action="delete-flight" data-id="${esc(f.flightNumber)}" type="button" title="Delete ${esc(f.flightNumber)}" aria-label="Delete ${esc(f.flightNumber)}">${icon('trash', 15)}</button></div></td></tr>`).join('') : `<tr><td colspan="8">${emptyState('No matching flights', 'Clear the search or add a flight.')}</td></tr>`}</tbody>`;
}
function renderFlightOptions() {
  const items = state.flights.map((f) => ({ value: f.flightNumber, label: `${f.flightNumber} · ${f.source} → ${f.destination}${isCancelled(f.status) ? ' · CANCELLED' : ''}` }));
  fillSelect('pFlight', items, 'Select a flight');
  fillSelect('suggestFlight', state.flights.map((f) => ({ value: f.flightNumber, label: `${f.flightNumber} · ${f.source} → ${f.destination} · ${f.status}` })), 'Select a flight');
  const activeFlights = state.flights.filter((f) => !isUnavailableFlight(f.status));
  const connectionItems = activeFlights.map((f) => ({ value: f.flightNumber, label: `${f.flightNumber} · ${f.source} → ${f.destination} · ${f.departureTime}–${f.arrivalTime}` }));
  fillSelect('crArrFlight', connectionItems, 'Select arriving flight');
  fillSelect('crNextFlight', connectionItems, 'Select connecting flight');
  fillSelect('crPassenger', state.passengers.map((p) => ({ value: p.passengerId, label: `${p.passengerId} · ${p.name} · ${p.flightNumber}` })), 'Select a passenger');
}

async function addFlight(event) {
  event.preventDefault();

  const form = event.currentTarget;

  if (!form.reportValidity()) return;

  const flightNumber = clean($('#fNo').value)
    .toUpperCase()
    .replace(/\s+/g, ' ');

  const airline = clean($('#fAir').value);
  const source = clean($('#fFrom').value);
  const destination = clean($('#fTo').value);
  const departureTime = $('#fDep').value;
  const arrivalTime = $('#fArr').value;
  const gate = clean($('#fGate').value).toUpperCase();
  const fare = Number($('#fFare').value);
  const status = $('#fStatus').value;

  // Frontend validation.
  if (state.flights.some(
    (f) => norm(f.flightNumber) === norm(flightNumber)
  )) {
    return toast(
      'Duplicate flight number',
      `${flightNumber} already exists.`,
      true
    );
  }

  if (norm(source) === norm(destination)) {
    return toast(
      'Invalid route',
      'Source and destination must be different.',
      true
    );
  }

  if (!departureTime || !arrivalTime) {
    return toast(
      'Time required',
      'Enter both departure and arrival times.',
      true
    );
  }

  if (!Number.isFinite(fare) || fare < 0) {
    return toast(
      'Invalid fare',
      'Enter a valid non-negative fare in INR.',
      true
    );
  }

  // Send the flight to the C++ API.
  try {
    const response = await apiFetch(`${API_BASE_URL}/flights`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams({flightNumber,airline,source,destination,departureTime,
        arrivalTime,gate,status,fare: String(fare)})});

    const result = await response.json();

    if (response.status === 409) {
      return toast(
        'Duplicate flight',
        result.message || 'This flight number already exists.',
        true
      );
    }

    if (!response.ok) {
      throw new Error(
        result.message || `Server error: ${response.status}`
      );
    }

    // Use the record returned by the backend.
    state.flights.push(result);

    // Refresh the existing dashboard and flight directory.
    form.reset();
    renderAll();

    toast(
      'Flight added successfully',
      `${result.flightNumber} has been saved to MySQL.`
    );

    debugLog('Flight saved in database:', result);

  } catch (error) {
    console.error('Add flight API error:', error);

    toast(
      'Flight could not be saved',
      error.message || 'Check your API server connection.',
      true
    );
  }
}


async function updateFlightDetails(id) {
  const flight = getFlight(id);
  if (!flight) return;

  // Collect updated flight details in one styled form.
  const values = await uiDialog({
    title: `Edit flight ${flight.flightNumber}`,
    subtitle: 'Changes are validated and saved to MySQL through the API.',
    submitLabel: 'Save flight',
    iconName: 'plane',
    fields: [
      { name: 'airline', label: 'Airline', value: flight.airline, maxlength: 50, full: true },
      { name: 'source', label: 'Source', value: flight.source, maxlength: 60 },
      { name: 'destination', label: 'Destination', value: flight.destination, maxlength: 60 },
      { name: 'departureTime', label: 'Departure time', type: 'time', value: flight.departureTime },
      { name: 'arrivalTime', label: 'Arrival time', type: 'time', value: flight.arrivalTime },
      { name: 'gate', label: 'Gate', value: flight.gate, maxlength: 10 },
      { name: 'fare', label: 'Fare (INR)', type: 'number', min: 0, step: '0.01', value: String(flight.fare ?? 0) }
    ]
  });
  if (!values) return;

  const { airline, source, destination, departureTime, arrivalTime, gate } = values;
  const fareInput = values.fare;

  const fare = Number(fareInput);

  // Validate the entered information.
  if (!clean(airline) || !clean(source) || !clean(destination) || !clean(gate) ||fareInput.trim() === '') {
    return toast(
      'Missing details',
      'All flight fields are required.',
      true
    );
  }

  if (norm(source) === norm(destination)) {
    return toast(
      'Invalid route',
      'Source and destination must be different.',
      true
    );
  }

  if (!Number.isFinite(fare) || fare < 0 || fare > 99999999.99) {
    return toast(
      'Invalid fare',
      'Enter a valid non-negative fare.',
      true
    );
  }

  const validTime = (value) =>
    /^([01]\d|2[0-3]):[0-5]\d$/.test(clean(value));

  if (!validTime(departureTime) || !validTime(arrivalTime)) {
    return toast(
      'Invalid time',
      'Use 24-hour HH:MM format.',
      true
    );
  }

  // Prepare the update request.
  const updatedDetails = {
    flightNumber: flight.flightNumber,
    airline: clean(airline),
    source: clean(source),
    destination: clean(destination),
    departureTime: clean(departureTime),
    arrivalTime: clean(arrivalTime),
    gate: clean(gate).toUpperCase(),
    fare: String(fare)
  };

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/flights/update`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: new URLSearchParams(updatedDetails)
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `Server error: ${response.status}`
      );
    }

    // Update frontend state using the API response.
    Object.assign(flight, result);

    renderAll();

    toast(
      'Flight updated',
      `${flight.flightNumber} was updated in MySQL.`
    );

    debugLog('Updated flight:', result);

  } catch (error) {
    console.error('Flight update failed:', error);

    toast(
      'Update failed',
      error.message || 'Could not update flight in the database.',
      true
    );
  }
}

async function setFlightStatus(id, newStatus) {
  const flight = getFlight(id);
  if (!flight) return;

  if (flight.status === newStatus) {
    return toast('No change', `${flight.flightNumber} is already ${newStatus}.`);
  }

  if (
    newStatus === 'Cancelled' &&
    !(await askConfirm(
      `Affected passengers will be notified and removed from check-in, boarding and priority queues.`,
      { title: `Cancel ${flight.flightNumber} to ${flight.destination}?`, confirmLabel: 'Cancel flight' }
    ))
  ) {
    return;
  }

  const oldStatus = flight.status;

  // First update the flight status in MySQL.
  try {
    const formData = new URLSearchParams({
      flightNumber: flight.flightNumber,
      airline: flight.airline,
      source: flight.source,
      destination: flight.destination,
      departureTime: flight.departureTime,
      arrivalTime: flight.arrivalTime,
      gate: flight.gate || '',
      status: newStatus,
      fare: String(flight.fare ?? 0)
    });

    const response = await apiFetch(`${API_BASE_URL}/flights/update`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: formData.toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || result.error || `HTTP error: ${response.status}`
      );
    }

    // Update the frontend only after the database confirms success.
    Object.assign(flight, result);

  } catch (error) {
    console.error('Flight status update error:', error);

    return toast(
      'Status update failed',
      error.message || 'Could not update the flight in MySQL.',
      true
    );
  }

  // Preserve the existing passenger notification functionality.
  const affectedCount = notifyFlightPassengers(
    flight.flightNumber,
    newStatus === 'Cancelled'
      ? 'Cancellation'
      : isDelayed(newStatus)
        ? 'Delay'
        : 'Flight Status',
    (p) => {
      if (newStatus === 'Cancelled') {
        return `Hi ${p.name}, flight ${flight.flightNumber} has been cancelled. Please contact the passenger service desk for assistance.`;
      }

      if (isDelayed(newStatus)) {
        return `Hi ${p.name}, flight ${flight.flightNumber} is delayed. Current scheduled departure is ${flight.departureTime}. Please check the latest departure information.`;
      }

      return `Hi ${p.name}, flight ${flight.flightNumber} status changed from ${oldStatus} to ${newStatus}.`;
    }
  );

  if (newStatus === 'Cancelled') {
    const removed = removeFlightFromQueues(flight.flightNumber);

    saveState();
    renderAll();

    toast(
      'Flight cancelled',
      `${flight.flightNumber}: ${affectedCount} passenger notice(s), ${removed.checkin} check-in, ${removed.boarding} boarding and ${removed.priority} priority queue entry/entries removed.`
    );
  } else {
    saveState();
    renderAll();

    toast(
      'Flight status updated',
      `${flight.flightNumber} is now ${newStatus}. ${affectedCount} passenger notification(s) created.`
    );
  }
}

async function deleteFlight(id) {
  const flight = getFlight(id);
  if (!flight) return;

  const assigned = state.passengers.filter(
    (p) => norm(p.flightNumber) === norm(id)
  ).length;

  if (assigned) {
    return toast(
      'Cannot delete flight',
      `${assigned} passenger record(s) still reference this flight. Cancel the flight instead, or reassign/delete passengers first.`,
      true
    );
  }

  if (!(await askConfirm('This permanently removes the flight record from MySQL.', { title: `Delete flight ${id}?`, confirmLabel: 'Delete flight' }))) return;

  // Delete the flight from MySQL first.
  try {
    const response = await apiFetch(`${API_BASE_URL}/flights/delete`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams({
        flightNumber: flight.flightNumber
      }).toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || result.error || `HTTP error: ${response.status}`
      );
    }

  } catch (error) {
    console.error('Flight deletion error:', error);

    return toast(
      'Deletion failed',
      error.message || 'Could not delete the flight from MySQL.',
      true
    );
  }

  // Update the frontend only after successful database deletion.
  state.flights = state.flights.filter(
    (f) => norm(f.flightNumber) !== norm(id)
  );

  saveState();
  renderAll();

  toast('Flight deleted', `${id} was removed from the schedule.`);
}

function renderPassengers() {
  const term = norm($('#pSearch').value);
  const statusFilter = $('#pStatusFilter').value;
  const passengers = state.passengers.filter((p) => {
    const textMatch = [p.passengerId, p.name, p.flightNumber, p.phoneNumber, p.seatNumber].some((v) => norm(v).includes(term));
    return textMatch && (statusFilter === 'all' || p.status === statusFilter);
  });
  $('#passengerCount').textContent = `${state.passengers.length} passenger${state.passengers.length === 1 ? '' : 's'}`;
  $('#paxTable').innerHTML = `<thead><tr><th>Passenger</th><th>Flight / Seat</th><th>Assistance</th><th>Status</th><th>Actions</th></tr></thead><tbody>${passengers.length ? passengers.map((p) => `<tr><td class="cell-strong">${esc(p.name)}<span class="cell-sub">${esc(p.passengerId)} · Age ${esc(p.age)}</span></td><td>${esc(p.flightNumber)}<span class="cell-sub">Seat ${esc(p.seatNumber)}</span></td><td>${esc(p.assistanceType || 'None')}</td><td>${statusBadge(p.status)}</td><td><div class="table-actions"><button class="table-action" data-action="edit-passenger" data-id="${esc(p.passengerId)}" type="button">Edit</button><button class="table-action danger" data-action="delete-passenger" data-id="${esc(p.passengerId)}" type="button">Delete</button></div></td></tr>`).join('') : `<tr><td colspan="5">${emptyState('No matching passengers', 'Register a passenger or clear the current filter.')}</td></tr>`}</tbody>`;
}

async function addPassenger(event) {
  event.preventDefault();

  const form = event.currentTarget;
  if (!form.reportValidity()) return;

  const passengerId = clean($('#pId').value).toUpperCase();
  const name = clean($('#pName').value);
  const age = Number($('#pAge').value);
  const gender = $('#pGender').value;
  const phoneNumber = clean($('#pPhone').value);
  const flightNumber = $('#pFlight').value;
  const seatNumber = clean($('#pSeat').value).toUpperCase();
  const assistanceType = $('#pAssist').value || 'None';

  // Existing frontend validations.
  if (state.passengers.some(
    (p) => norm(p.passengerId) === norm(passengerId)
  )) {
    return toast(
      'Duplicate passenger ID',
      `${passengerId} already exists. Use a unique ID.`,
      true
    );
  }

  if (!/^[A-Z0-9_-]+$/.test(passengerId)) {
    return toast(
      'Invalid passenger ID',
      'Use letters, numbers, hyphens or underscores only.',
      true
    );
  }

  if (name.length < 2) {
    return toast(
      'Invalid name',
      'Enter a passenger name containing at least two characters.',
      true
    );
  }

  if (!Number.isInteger(age) || age < 0 || age > 120) {
    return toast(
      'Invalid age',
      'Age must be a whole number between 0 and 120.',
      true
    );
  }

  if (!/^[+0-9() -]{7,16}$/.test(phoneNumber) || phoneNumber.replace(/\D/g, '').length < 7) {
    return toast(
      'Invalid phone number',
      'Enter a valid phone number.',
      true
    );
  }

  const flight = getFlight(flightNumber);

  if (!flight) {
    return toast(
      'Flight not found',
      'Register the flight before assigning a passenger.',
      true
    );
  }

  if (isUnavailableFlight(flight.status)) {
    return toast(
      'Registration denied',
      `Flight ${flight.flightNumber} is ${flight.status.toLowerCase()}.`,
      true
    );
  }

  if (state.passengers.some(
    (p) =>
      norm(p.flightNumber) === norm(flightNumber) &&
      norm(p.seatNumber) === norm(seatNumber)
  )) {
    return toast(
      'Seat already assigned',
      `Seat ${seatNumber} is already assigned on ${flightNumber}.`,
      true
    );
  }

  // Prepare passenger data for the C++ API.
  const formData = new URLSearchParams({
    passengerId,
    name,
    age: String(age),
    gender,
    phoneNumber,
    flightNumber,
    seatNumber,
    assistanceType
  });

  // Register passenger in MySQL.
  try {
    const response = await apiFetch(`${API_BASE_URL}/passengers`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: formData.toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Use the record returned by the backend.
    state.passengers.push(result);

    form.reset();
    saveState();
    renderAll();

    toast(
      'Passenger registered',
      `${result.name} was assigned to ${result.flightNumber} with status ${result.status}.`
    );

    debugLog('Passenger saved in MySQL:', result);

  } catch (error) {
    console.error('Passenger registration error:', error);

    toast(
      'Registration failed',
      error.message || 'Could not register passenger in MySQL.',
      true
    );
  }
}


async function updatePassengerDetails(id) {
  const p = getPassenger(id);
  if (!p) return;

  if (p.status !== 'Registered') {
    return toast(
      'Update restricted',
      'Passenger details can only be reassigned before check-in. This passenger has already progressed through the journey.',
      true
    );
  }

  // Collect updated details in one styled form.
  const flightChoices = state.flights
    .filter((f) => !isCancelled(f.status) || norm(f.flightNumber) === norm(p.flightNumber))
    .map((f) => ({ value: f.flightNumber, label: `${f.flightNumber} · ${f.source} → ${f.destination} · ${f.departureTime}` }));
  const assistanceChoices = ['None', 'Emergency', 'Medical', 'Wheelchair', 'Accessibility', 'Elderly', 'VIP', 'Other'];
  if (p.assistanceType && !assistanceChoices.includes(p.assistanceType)) assistanceChoices.push(p.assistanceType);

  const values = await uiDialog({
    title: `Edit passenger ${p.passengerId}`,
    subtitle: 'Details can be changed until the passenger checks in.',
    submitLabel: 'Save passenger',
    iconName: 'user',
    fields: [
      { name: 'name', label: 'Full name', value: p.name, minlength: 2, maxlength: 80, full: true },
      { name: 'age', label: 'Age', type: 'number', min: 0, max: 120, step: 1, value: String(p.age) },
      { name: 'phoneNumber', label: 'Phone number', type: 'tel', value: p.phoneNumber, pattern: '[+0-9() -]{7,16}', maxlength: 16 },
      { name: 'flightNumber', label: 'Flight', value: p.flightNumber, options: flightChoices, full: true },
      { name: 'seatNumber', label: 'Seat number', value: p.seatNumber, maxlength: 6 },
      { name: 'assistanceType', label: 'Assistance type', value: p.assistanceType || 'None', options: assistanceChoices }
    ]
  });
  if (!values) return;

  const { name, phoneNumber, flightNumber, seatNumber, assistanceType } = values;
  const ageText = values.age;

  const age = Number(ageText);
  const flight = getFlight(flightNumber);

  // Existing frontend validation.
  if (clean(name).length < 2) {
    return toast('Invalid name', 'Name must contain at least two characters.', true);
  }

  if (!Number.isInteger(age) || age < 0 || age > 120) {
    return toast('Invalid age', 'Age must be between 0 and 120.', true);
  }

  if (
    !/^[+0-9() -]{7,16}$/.test(clean(phoneNumber)) ||
    clean(phoneNumber).replace(/\D/g, '').length < 7
  ) {
    return toast('Invalid phone', 'Enter a valid phone number.', true);
  }

  if (!flight) {
    return toast('Flight not found', 'The selected flight does not exist.', true);
  }

  if (isUnavailableFlight(flight.status)) {
    return toast(
      'Update denied',
      'Passengers cannot be reassigned to a cancelled or completed flight.',
      true
    );
  }

  const belongsToGroup = state.groups.some(
    (group) =>
      group.members.some(
        (memberId) => norm(memberId) === norm(p.passengerId)
      )
  );

  if (
    belongsToGroup &&
    norm(flight.flightNumber) !== norm(p.flightNumber)
  ) {
    return toast(
      'Reassignment denied',
      'This passenger belongs to a travel group. Keep the passenger on the same flight.',
      true
    );
  }

  const seat = clean(seatNumber).toUpperCase();

  if (!seat) {
    return toast('Seat required', 'Seat number cannot be empty.', true);
  }

  if (state.passengers.some(
    (other) =>
      norm(other.passengerId) !== norm(id) &&
      norm(other.flightNumber) === norm(flight.flightNumber) &&
      norm(other.seatNumber) === norm(seat)
  )) {
    return toast(
      'Seat already assigned',
      `Seat ${seat} is already used on ${flight.flightNumber}.`,
      true
    );
  }

  // Prepare the updated passenger for MySQL.
  const formData = new URLSearchParams({
    passengerId: p.passengerId,
    name: clean(name),
    age: String(age),
    gender: p.gender,
    phoneNumber: clean(phoneNumber),
    flightNumber: flight.flightNumber,
    seatNumber: seat,
    assistanceType: clean(assistanceType) || 'None'
  });

  // Update the database before changing frontend state.
  try {
    const response = await apiFetch(`${API_BASE_URL}/passengers/update`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: formData.toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Update frontend using the backend response.
    Object.assign(p, result);

    saveState();
    renderAll();

    toast(
      'Passenger updated',
      `${p.name}'s record was successfully updated in MySQL.`
    );

    debugLog('Passenger updated in MySQL:', result);

  } catch (error) {
    console.error('Passenger update error:', error);

    toast(
      'Passenger update failed',
      error.message || 'Could not update passenger in MySQL.',
      true
    );
  }
}


async function deletePassenger(id) {
  const p = getPassenger(id);
  if (!p) return;

  if (!(await askConfirm(
    'The record can be restored with Undo delete.',
    { title: `Delete ${p.name} (${p.passengerId})?`, confirmLabel: 'Delete passenger' }
  ))) return;

  try {
    const response = await apiFetch(`${API_BASE_URL}/passengers/delete`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams({
        passengerId: p.passengerId
      }).toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Save the complete deleted passenger in the Undo Stack.
    state.deletedStack.push({ ...result });

    // Remove passenger from the frontend.
    state.passengers = state.passengers.filter(
      (item) => norm(item.passengerId) !== norm(id)
    );

    // Remove passenger from existing queues.
    state.checkinQ = state.checkinQ.filter(
      (item) => norm(item) !== norm(id)
    );

    state.boardQ = state.boardQ.filter(
      (item) => norm(item.passengerId) !== norm(id)
    );

    state.priorityQ = state.priorityQ.filter(
      (item) => norm(item.passengerId) !== norm(id)
    );

    saveState();
    renderAll();

    toast(
      'Passenger deleted',
      `${p.name} was deleted from MySQL. Use Undo to restore the record.`
    );

    debugLog('Deleted passenger:', result);

  } catch (error) {
    console.error('Passenger deletion error:', error);

    toast(
      'Passenger deletion failed',
      error.message || 'Could not delete passenger from MySQL.',
      true
    );
  }
}


async function undoDelete() {
  // Read the most recently deleted passenger.
  const passenger =
    state.deletedStack[state.deletedStack.length - 1];

  if (!passenger) {
    return toast(
      'Nothing to undo',
      'No passenger deletion is available to restore.',
      true
    );
  }

  if (getPassenger(passenger.passengerId)) {
    return toast(
      'Cannot restore passenger',
      `ID ${passenger.passengerId} is already in use.`,
      true
    );
  }

  // Prepare the original passenger details.
  const formData = new URLSearchParams({
    passengerId: passenger.passengerId,
    name: passenger.name,
    age: String(passenger.age),
    gender: passenger.gender,
    phoneNumber: passenger.phoneNumber,
    flightNumber: passenger.flightNumber,
    seatNumber: passenger.seatNumber,
    assistanceType: passenger.assistanceType || 'None',
    status: passenger.status
  });

  try {
    const response = await apiFetch(`${API_BASE_URL}/passengers/restore`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: formData.toString()
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Remove the snapshot only after successful database restoration.
    state.deletedStack.pop();

    // Restore the passenger in the frontend.
    state.passengers.push(result);

    saveState();
    renderAll();

    toast(
      'Passenger restored',
      `${result.name} was successfully restored in MySQL. Queue positions are not automatically restored.`
    );

    debugLog('Restored passenger:', result);

  } catch (error) {
    console.error('Passenger restoration error:', error);

    toast(
      'Restoration failed',
      error.message || 'Could not restore passenger in MySQL.',
      true
    );
  }
}


function renderQueueOptions() {
  const ciItems = state.passengers.filter((p) => p.status === 'Registered' && activeFlightForPassenger(p) && !state.checkinQ.some((id) => norm(id) === norm(p.passengerId))).map((p) => ({ value: p.passengerId, label: `${p.passengerId} · ${p.name} · ${p.flightNumber}` }));
  const securityItems = state.passengers.filter((p) => p.status === 'Checked-In' && activeFlightForPassenger(p) && !state.boardQ.some((item) => norm(item.passengerId) === norm(p.passengerId))).map((p) => ({ value: p.passengerId, label: `${p.passengerId} · ${p.name} · ${p.flightNumber}` }));
  const prItems = state.passengers.filter((p) =>
  activeFlightForPassenger(p) &&
  !state.priorityQ.some(
    (item) => norm(item.passengerId) === norm(p.passengerId)
  ) &&
  !state.processedPriorityIds.some(
    (id) => norm(id) === norm(p.passengerId)
  )
).map((p) => ({
  value: p.passengerId,
  label: `${p.passengerId} · ${p.name} · ${p.flightNumber}`
}));
  fillSelect('ciSel', ciItems, ciItems.length ? 'Select a registered passenger' : 'No eligible passengers');
  fillSelect('scSel', securityItems, securityItems.length ? 'Select a checked-in passenger' : 'No eligible passengers');
  fillSelect('prPassenger', prItems, prItems.length ? 'Select a passenger on an active flight' : 'No available passengers');
  fillSelect('asPax', state.passengers.map((p) => ({ value: p.passengerId, label: `${p.passengerId} · ${p.name}` })), 'Select a passenger');
}
function enqueue(queueName, selectId) {
  return enqueueById(queueName, document.getElementById(selectId).value);
}


async function enqueueById(queueName, id) {
  if (!id) {
    return toast('Select a passenger', 'Choose a passenger before joining the queue.', true);
  }

  const p = getPassenger(id);

  if (!p) {
    return toast('Passenger not found', 'Register the passenger before queueing.', true);
  }

  const flight = getFlight(p.flightNumber);

  if (!flight) {
    return toast('Flight not found', 'The passenger is assigned to a flight that does not exist.', true);
  }

  if (isUnavailableFlight(flight.status)) {
    return toast('Queue entry denied', `${p.name} cannot enter a queue for unavailable flight ${flight.flightNumber}.`, true);
  }

  // Check-in Queue: save through the C++ API and MySQL.
  if (queueName === 'checkinQ') {
    if (state.checkinQ.some((item) => norm(item) === norm(id))) {
      return toast('Duplicate queue entry', `${p.name} is already in the check-in queue.`, true);
    }

    if (p.status !== 'Registered') {
      return toast('Check-in denied', `Only Registered passengers can join the check-in queue. Current status: ${p.status}.`, true);
    }

    try {
      const response = await apiFetch(
        `${API_BASE_URL}/checkin/enqueue`,
        {
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
          },
          body: new URLSearchParams({
            passengerId: p.passengerId
          })
        }
      );

      const result = await response.json();

      if (!response.ok) {
        throw new Error(result.message || `HTTP error: ${response.status}`);
      }

      // Synchronize the queue with MySQL.
      await loadCheckInQueueFromApi();

      notify(
        p,
        'Check-In Reminder',
        `Hi ${p.name}, please proceed to check in for flight ${p.flightNumber}.`
      );

      saveState();
      renderAll();

      toast(
        'Added to check-in queue',
        `${p.name} was added at queue position ${result.queuePosition}.`
      );

    } catch (error) {
      console.error('Check-in enqueue error:', error);

      toast(
        'Check-in enqueue failed',
        error.message || 'Could not add the passenger to the queue.',
        true
      );
    }

    return;
  }
}


async function markSecurityCleared() {
  const id = $('#scSel').value;

  if (!id) {
    return toast(
      'Select a passenger',
      'Choose a Checked-In passenger before clearing security.',
      true
    );
  }

  const passenger = getPassenger(id);

  if (!passenger) {
    return toast('Passenger not found', 'Register the passenger before security clearance.', true);
  }

  if (passenger.status !== 'Checked-In') {
    return toast(
      'Security clearance denied',
      `Only Checked-In passengers can be cleared. Current status: ${passenger.status}.`,
      true
    );
  }

  const flight = getFlight(passenger.flightNumber);

  if (!flight) {
    return toast('Flight not found', 'The passenger is assigned to a flight that does not exist.', true);
  }

  if (isUnavailableFlight(flight.status)) {
    return toast(
      'Security clearance denied',
      `Flight ${flight.flightNumber} is ${flight.status.toLowerCase()}.`,
      true
    );
  }

  const gate = clean($('#scGate').value || flight.gate).toUpperCase();

  if (!gate || gate.length > 10) {
    return toast(
      'Gate required',
      'Enter a valid boarding gate (maximum 10 characters).',
      true
    );
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/security/clear`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: new URLSearchParams({
          passengerId: passenger.passengerId,
          gate: gate
        })
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(result.message || `HTTP error: ${response.status}`);
    }

    const processed = result.passenger;
    // Remember passengers whose assistance request has been processed.
    if (!Array.isArray(state.processedPriorityIds)) {
     state.processedPriorityIds = [];
    }

   if (!state.processedPriorityIds.some(
  (id) => norm(id) === norm(processed.passengerId)
   )) {
  state.processedPriorityIds.push(processed.passengerId);
   }

    if (
      !processed ||
      norm(processed.passengerId) !== norm(passenger.passengerId) ||
      processed.status !== 'Security-Cleared'
    ) {
      throw new Error('Invalid security clearance response received.');
    }

    // Update the frontend passenger record.
    passenger.status = processed.status;

    // Refresh Boarding Queue directly from MySQL.
    await loadBoardingQueueFromApi();

    notify(
      passenger,
      'Flight Status',
      `Security clearance completed for ${passenger.name} on flight ${passenger.flightNumber}.`
    );

    notify(
      passenger,
      'Boarding Reminder',
      `Hi ${passenger.name}, security clearance is complete. Please proceed to gate ${gate} for flight ${passenger.flightNumber}.`
    );

    $('#scGate').value = '';

    saveState();
    renderAll();

    toast(
      'Security cleared',
      `${passenger.name} was cleared successfully and added to the Boarding Queue at gate ${gate}.`
    );

  } catch (error) {
    console.error('Security clearance API error:', error);

    toast(
      'Security clearance failed',
      error.message || 'Could not complete security clearance.',
      true
    );
  }
}



async function dequeue(queueName) {

  // Process Check-in Queue through C++ API and MySQL.
  if (queueName === 'checkinQ') {

    if (!state.checkinQ.length) {
      return toast('Queue is empty', 'There are no passengers waiting for check-in.', true);
    }

    const id = state.checkinQ[0];
    const p = getPassenger(id);

    if (!p) {
      await loadCheckInQueueFromApi();
      return toast('Queue refreshed', 'The queue was refreshed from MySQL. Please try again.', true);
    }

    const flight = getFlight(p.flightNumber);

    if (!flight || isUnavailableFlight(flight.status)) {
      return toast('Cannot process passenger', 'The passenger flight is missing, cancelled or completed.', true);
    }

    if (p.status !== 'Registered') {
      return toast('Cannot process passenger', `Passenger status is ${p.status}.`, true);
    }

    try {
      const response = await apiFetch(
        `${API_BASE_URL}/checkin/dequeue`,
        {
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
          }
        }
      );

      const result = await response.json();

      if (!response.ok) {
        throw new Error(result.message || `HTTP error: ${response.status}`);
      }

      const processed = result.passenger;

      if (
        !processed ||
        !processed.passengerId ||
        processed.status !== 'Checked-In'
      ) {
        throw new Error('Invalid dequeue response received.');
      }

      // Update the passenger in frontend state.
      const updatedPassenger = getPassenger(processed.passengerId);

      if (updatedPassenger) {
        updatedPassenger.status = processed.status;
      }

      // Refresh remaining queue entries from MySQL.
      await loadCheckInQueueFromApi();

      notify(
        updatedPassenger || processed,
        'Flight Status',
        `Check-in completed for ${processed.name} on flight ${processed.flightNumber}.`
      );

      saveState();
      renderAll();

      toast(
        'Check-in complete',
        `${processed.name}'s status is now Checked-In.`
      );

    } catch (error) {
      console.error('Check-in dequeue error:', error);

      toast(
        'Check-in failed',
        error.message || 'Could not process the passenger.',
        true
      );
    }

    return;
  }

  // Boarding Queue: original behaviour preserved.
  if (queueName === 'boardQ') {

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/boarding/dequeue`,
      {
        method: 'POST'
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    const processed = result.passenger;

    if (
      !processed ||
      !processed.passengerId ||
      processed.status !== 'Boarded'
    ) {
      throw new Error('Invalid boarding response received.');
    }

    // Update the passenger in frontend state.
    const updatedPassenger = getPassenger(processed.passengerId);

    if (updatedPassenger) {
      updatedPassenger.status = processed.status;
    }

    // Reload the remaining queue from MySQL.
    await loadBoardingQueueFromApi();

    notify(
      updatedPassenger || processed,
      'Flight Status',
      `${processed.name} has boarded flight ${processed.flightNumber} at gate ${result.gate}.`
    );

    saveState();
    renderAll();

    toast(
      'Passenger boarded',
      `${processed.name} has boarded successfully at gate ${result.gate}.`
    );

  } catch (error) {
    console.error('Boarding dequeue API error:', error);

    toast(
      'Boarding failed',
      error.message || 'Could not process the next passenger.',
      true
    );
  }

  return;
  }
}

function renderQueues() {
  $('#ciCount').textContent = `${state.checkinQ.length} waiting`;
  $('#boCount').textContent = `${state.boardQ.length} waiting`;
  $('#ciList').innerHTML = state.checkinQ.length ? state.checkinQ.map((id, index) => { const p = getPassenger(id); return p ? `<div class="queue-item"><span class="queue-number">${index + 1}</span><div class="queue-item-copy"><strong>${esc(p.name)}</strong><small>${esc(p.passengerId)} · ${esc(p.flightNumber)} · ${esc(p.status)}</small></div>${index === 0 ? '<span class="status-badge info">Next</span>' : '<span class="muted small-text">Waiting</span>'}</div>` : ''; }).join('') : emptyState('Check-in queue is empty', 'Register a passenger and add them to the queue.');
  $('#boList').innerHTML = state.boardQ.length ? state.boardQ.map((item, index) => { const p = getPassenger(item.passengerId); return p ? `<div class="queue-item"><span class="queue-number">${index + 1}</span><div class="queue-item-copy"><strong>${esc(p.name)}</strong><small>${esc(p.passengerId)} · ${esc(p.flightNumber)} · Gate ${esc(item.gate)}</small></div>${index === 0 ? '<span class="status-badge info">Next</span>' : '<span class="muted small-text">Waiting</span>'}</div>` : ''; }).join('') : emptyState('Boarding queue is empty', 'Complete check-in before joining boarding.');
  $('#ciWait').textContent = state.checkinQ.length ? `Approx. ${state.checkinQ.length * 4} minutes of queue processing at 4 minutes per passenger (demo estimate).` : '';
  $('#boWait').textContent = state.boardQ.length ? `Approx. ${state.boardQ.length * 2} minutes of boarding processing at 2 minutes per passenger (demo estimate).` : '';
  const tbody = state.passengers.map((p) => {
    const queue = state.checkinQ.some((id) => norm(id) === norm(p.passengerId)) ? 'Check-in queue' : state.boardQ.some((item) => norm(item.passengerId) === norm(p.passengerId)) ? 'Boarding queue' : '—';
    return `<tr><td class="cell-strong">${esc(p.name)}<span class="cell-sub">${esc(p.passengerId)}</span></td><td>${esc(p.flightNumber)}</td><td>${statusBadge(p.status)}</td><td>${esc(queue)}</td></tr>`;
  }).join('');
  $('#journeyTable').innerHTML = `<thead><tr><th>Passenger</th><th>Flight</th><th>Current status</th><th>Queue</th></tr></thead><tbody>${tbody || `<tr><td colspan="4">${emptyState('No passenger records', 'Register passengers first.')}</td></tr>`}</tbody>`;
}

// Stores completed AeroAssist requests retrieved from MySQL.
let assistanceHistoryRecords = [];

// Load completed assistance requests from the API.
async function loadAssistanceHistoryFromApi() {
  const table = $('#assistanceHistoryTable');

  if (table) {
    table.innerHTML =
      '<tbody><tr><td colspan="7">Loading assistance history...</td></tr></tbody>';
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/assistance/history`
    );

    const data = await response.json();

    if (!response.ok) {
      throw new Error(
        data.message || `HTTP error: ${response.status}`
      );
    }

    if (!Array.isArray(data)) {
      throw new Error('Invalid assistance history data received.');
    }

    assistanceHistoryRecords = data;

    renderAssistanceHistory();

    debugLog(
      `Loaded ${data.length} completed AeroAssist requests.`
    );

  } catch (error) {
    console.error('Assistance history API error:', error);

    assistanceHistoryRecords = [];

    if (table) {
      table.innerHTML = `
        <tbody>
          <tr>
            <td colspan="7">
              ${emptyState(
                'Could not load request history',
                'Check the API connection and try refreshing.'
              )}
            </td>
          </tr>
        </tbody>
      `;
    }

    $('#historyCount').textContent = 'Unavailable';

    toast(
      'History loading failed',
      error.message || 'Could not retrieve assistance history.',
      true
    );
  }
}

// Render history records and apply the Passenger ID search.
function renderAssistanceHistory() {
  const searchValue = clean(
    $('#historyPassengerSearch').value
  ).toLowerCase();

  const filteredRecords = assistanceHistoryRecords.filter(
    (record) =>
      String(record.passengerId || '')
        .toLowerCase()
        .includes(searchValue)
  );

  $('#historyCount').textContent =
    `${filteredRecords.length} record${filteredRecords.length === 1 ? '' : 's'}`;

  const rows = filteredRecords.map((record) => `
    <tr>
      <td>${esc(record.historyId)}</td>
      <td class="cell-strong">${esc(record.passengerId)}</td>
      <td>${esc(record.reason)}</td>
      <td>
        <span class="priority-rank">
          P${esc(record.priority)}
        </span>
        ${esc(priorityLabel(record.priority))}
      </td>
      <td>${esc(record.createdAt || '—')}</td>
      <td>${esc(record.processedAt || '—')}</td>
      <td>${esc(record.status)}</td>
    </tr>
  `).join('');

  $('#assistanceHistoryTable').innerHTML = `
    <thead>
      <tr>
        <th>History ID</th>
        <th>Passenger ID</th>
        <th>Reason</th>
        <th>Priority</th>
        <th>Request Created</th>
        <th>Processed At</th>
        <th>Status</th>
      </tr>
    </thead>
    <tbody>
      ${
        rows ||
        `<tr>
          <td colspan="7">
            ${
              searchValue
                ? 'No history records found for this Passenger ID.'
                : 'No completed assistance requests yet.'
            }
          </td>
        </tr>`
      }
    </tbody>
  `;
}

function priorityLabel(priority) { return ({ 1: 'Emergency', 2: 'Medical Assistance', 3: 'Wheelchair / Accessibility', 4: 'VIP', 5: 'Normal' })[Number(priority)] || 'General'; }


// Add a priority request through the C++ API and MySQL.
async function addPriority(event) {
  event.preventDefault();

  const form = event.currentTarget;

  if (!form.reportValidity()) return;

  const added = await addPriorityRequest(
    $('#prPassenger').value,
    Number($('#prType').value),
    $('#prReason').value
  );

  // Reset only after a successful API operation.
  if (added) {
    form.reset();
  }
}


async function addPriorityRequest(passengerId, priority, reasonText) {
  const passenger = getPassenger(passengerId);

  if (!passenger) {
    toast(
      'Passenger not found',
      'Register a passenger before adding an assistance request.',
      true
    );
    return false;
  }

  const assignedFlight = getFlight(passenger.flightNumber);

  if (!assignedFlight || isUnavailableFlight(assignedFlight.status)) {
    toast(
      'Assistance request denied',
      'The passenger must be assigned to an existing active flight.',
      true
    );
    return false;
  }

  if (state.priorityQ.some(
    (item) => norm(item.passengerId) === norm(passengerId)
  )) {
    toast(
      'Already queued',
      `${passenger.name} already has an active priority request.`,
      true
    );
    return false;
  }

  if (!Number.isInteger(priority) || priority < 1 || priority > 5) {
    toast('Invalid priority', 'Choose a priority from 1 to 5.', true);
    return false;
  }

  const reason = clean(reasonText) || priorityLabel(priority);

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/priority/enqueue`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: new URLSearchParams({
          passengerId: passenger.passengerId,
          priority: String(priority),
          reason: reason
        })
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    if (
      result.status !== 'success' ||
      !result.passenger ||
      norm(result.passenger.passengerId) !== norm(passenger.passengerId)
    ) {
      throw new Error('Invalid Priority Queue response received.');
    }

    // Synchronize Priority Queue with MySQL.
    await loadPriorityQueueFromApi();

    notify(
      passenger,
      'Assistance Request',
      `${passenger.name}, your ${priorityLabel(priority).toLowerCase()} assistance request has been registered.`
    );

    saveState();
    renderAll();

    toast(
      'Assistance request added',
      `${passenger.name} entered priority level ${priority} (${priorityLabel(priority)}).`
    );

    return true;

  } catch (error) {
    console.error('Priority enqueue API error:', error);

    toast(
      'Assistance request failed',
      error.message || 'Could not add the assistance request.',
      true
    );

    return false;
  }
}


// Sort by priority, retaining FIFO order for equal priorities.
function sortedPriorityQueue() {
  return [...state.priorityQ].sort(
    (a, b) => a.priority - b.priority || a.sequence - b.sequence
  );
}


// Process the highest-priority request through MySQL.
async function servePriority() {
  if (!state.priorityQ.length) {
    return toast(
      'Priority queue is empty',
      'There are no assistance requests to process.',
      true
    );
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/priority/dequeue`,
      {
        method: 'POST'
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    const processed = result.passenger;

    if (
      !processed ||
      !processed.passengerId ||
      !Number.isInteger(Number(result.priority))
    ) {
      throw new Error('Invalid Priority Queue dequeue response.');
    }

    // Reload the remaining requests from MySQL.
    await loadPriorityQueueFromApi();

    // Update completed request history.
    await loadAssistanceHistoryFromApi();

    saveState();
    renderAll();

    toast(
      'Assistance request processed',
      `${processed.name} · ${priorityLabel(result.priority)} (priority ${result.priority}).`
    );

  } catch (error) {
    console.error('Priority dequeue API error:', error);

    toast(
      'Priority processing failed',
      error.message || 'Could not process the highest-priority request.',
      true
    );
  }
}

function renderPriority() {
  const sorted = sortedPriorityQueue();
  $('#prCount').textContent = `${sorted.length} request${sorted.length === 1 ? '' : 's'}`;
  $('#prList').innerHTML = sorted.length ? sorted.map((item, index) => {
    const p = getPassenger(item.passengerId);
    return p ? `<div class="queue-item"><span class="priority-rank">P${item.priority}</span><div class="queue-item-copy"><strong>${esc(p.name)}</strong><small>${esc(priorityLabel(item.priority))} · ${esc(p.flightNumber)} · ${esc(item.reason)}</small></div>${index === 0 ? '<span class="status-badge warn">Next</span>' : `<span class="muted small-text">#${index + 1}</span>`}</div>` : '';
  }).join('') : emptyState('No assistance requests', 'Add a passenger request to populate the priority queue.');
}

function findGroup(groupId) {
  return state.groups.find((group) => norm(group.groupId) === norm(groupId));
}
function passengerAlreadyGrouped(passengerId) {
  return state.groups.some((group) => group.members.some((memberId) => norm(memberId) === norm(passengerId)));
}
function renderGroupOptions() {
  const activeFlights = state.flights.filter((flight) => !isUnavailableFlight(flight.status));
  fillSelect('gtFlight', activeFlights.map((flight) => ({ value: flight.flightNumber, label: `${flight.flightNumber} · ${flight.source} → ${flight.destination} · ${flight.status}` })), activeFlights.length ? 'Select an active flight' : 'No active flights');
  fillSelect('gtTargetGroup', state.groups.map((group) => ({ value: group.groupId, label: `${group.groupId} · ${group.groupName} · ${group.flightNumber}` })), state.groups.length ? 'Select a group' : 'Create a group first');
  const ungroupedPassengers = state.passengers.filter((passenger) => !passengerAlreadyGrouped(passenger.passengerId));
  fillSelect('gtPassenger', ungroupedPassengers.map((passenger) => ({ value: passenger.passengerId, label: `${passenger.passengerId} · ${passenger.name} · ${passenger.flightNumber} · ${passenger.status}` })), ungroupedPassengers.length ? 'Select an ungrouped passenger' : 'No ungrouped passengers');
}

async function createTravelGroup(event) {
  event.preventDefault();

  const form = event.currentTarget;

  if (!form.reportValidity()) return;

  const groupId = clean($('#gtGroupId').value).toUpperCase();
  const groupName = clean($('#gtGroupName').value);
  const flightNumber = $('#gtFlight').value;

  if (!groupId || !groupName || !flightNumber) {
    return toast(
      'Missing group details',
      'Enter the group ID and name, then select a flight.',
      true
    );
  }

  if (findGroup(groupId)) {
    return toast(
      'Duplicate group ID',
      `Group ${groupId} already exists. Choose a unique ID.`,
      true
    );
  }

  const flight = getFlight(flightNumber);

  if (!flight) {
    return toast(
      'Flight not found',
      'Create the flight before creating a travel group.',
      true
    );
  }

  if (isUnavailableFlight(flight.status)) {
    return toast(
      'Group creation denied',
      `Flight ${flight.flightNumber} is ${flight.status.toLowerCase()}. Choose an active flight.`,
      true
    );
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/groups`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: new URLSearchParams({
          groupId,
          groupName,
          flightNumber: flight.flightNumber
        }).toString()
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Reload the persisted group list from MySQL.
    await loadGroupsFromApi();

    form.reset();
    renderAll();

    toast(
      'Travel group created',
      `${groupName} (${groupId}) is linked to flight ${flight.flightNumber}.`
    );

    debugLog('Group saved in MySQL:', result.group);

  } catch (error) {
    console.error('Group creation API error:', error);

    toast(
      'Group creation failed',
      error.message || 'Could not create the travel group.',
      true
    );
  }
}

async function addPassengerToTravelGroup(event) {
  event.preventDefault();

  const form = event.currentTarget;

  if (!form.reportValidity()) return;

  const groupId = $('#gtTargetGroup').value;
  const passengerId = $('#gtPassenger').value;

  if (!groupId || !passengerId) {
    return toast(
      'Selection required',
      'Choose both a group and a passenger.',
      true
    );
  }

  const group = findGroup(groupId);

  if (!group) {
    return toast(
      'Group not found',
      `Group ${groupId} does not exist.`,
      true
    );
  }

  const flight = getFlight(group.flightNumber);

  if (!flight || isUnavailableFlight(flight.status)) {
    return toast(
      'Group flight unavailable',
      `Group ${group.groupId} is linked to a missing, cancelled or completed flight.`,
      true
    );
  }

  const passenger = getPassenger(passengerId);

  if (!passenger) {
    return toast(
      'Passenger not found',
      `Passenger ${passengerId} is not registered.`,
      true
    );
  }

  if (norm(passenger.flightNumber) !== norm(group.flightNumber)) {
    return toast(
      'Flight mismatch',
      `${passenger.name} is assigned to ${passenger.flightNumber}; group ${group.groupId} is travelling on ${group.flightNumber}.`,
      true
    );
  }

  if (passengerAlreadyGrouped(passenger.passengerId)) {
    return toast(
      'Passenger already grouped',
      `${passenger.name} already belongs to a travel group.`,
      true
    );
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/groups/add-passenger`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: new URLSearchParams({
          groupId,
          passengerId: passenger.passengerId
        }).toString()
      }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(
        result.message || `HTTP error: ${response.status}`
      );
    }

    // Refresh group membership from MySQL.
    await loadGroupsFromApi();

    form.reset();
    renderAll();

    toast(
      'Passenger added to group',
      `${passenger.name} was added to ${group.groupName}.`
    );

    debugLog('Group membership saved in MySQL:', result);

  } catch (error) {
    console.error('Group member API error:', error);

    toast(
      'Could not add passenger',
      error.message || 'The passenger could not be added to this group.',
      true
    );
  }
}

function groupBoardingStatus(total, checkedIn, boarded, flight) {
  if (!flight || isUnavailableFlight(flight.status)) return 'Flight Unavailable';
  if (total === 0) return 'No Members Yet';
  if (boarded === total) return 'All Members Boarded';
  if (boarded > 0) return 'Partially Boarded';
  if (checkedIn === total) return 'All Checked-In — Awaiting Boarding';
  if (checkedIn > 0) return 'Partially Checked-In';
  return 'Check-In Pending';
}
function renderGroupTravel() {
  const host = $('#groupList');
  if (!host) return;
  $('#groupCount').textContent = `${state.groups.length} group${state.groups.length === 1 ? '' : 's'}`;
  if (!state.groups.length) {
    host.innerHTML = emptyState('No travel groups yet', 'Create a group, choose its active flight, and add registered passengers.');
    return;
  }
  host.innerHTML = state.groups.map((group) => {
    const flight = getFlight(group.flightNumber);
    let total = 0, checkedIn = 0, pending = 0, boarded = 0, missing = 0;
    const memberLines = group.members.map((memberId) => {
      const passenger = getPassenger(memberId);
      if (!passenger) { missing += 1; return `<div class="group-member"><span class="group-member-id">${esc(memberId)}</span><span class="group-member-copy">Passenger record unavailable</span>${statusBadge('Missing')}</div>`; }
      total += 1;
      const statusKey = norm(passenger.status).replace(/[\s_]+/g, '-');
      if (['checked-in', 'security-cleared', 'boarded'].includes(statusKey)) checkedIn += 1;
      if (statusKey === 'boarded') boarded += 1;
      return `<div class="group-member"><span class="group-member-id">${esc(passenger.passengerId)}</span><span class="group-member-copy"><strong>${esc(passenger.name)}</strong><small>${esc(passenger.flightNumber)} · ${esc(passenger.status)}</small></span>${statusBadge(passenger.status)}</div>`;
    }).join('');
    pending = total - checkedIn;
    const boardingStatus = groupBoardingStatus(total, checkedIn, boarded, flight);
    const route = flight ? `${flight.source} → ${flight.destination}` : 'Flight record unavailable';
    const flightStatus = flight ? flight.status : 'Missing';
    return `<article class="group-status-card">
      <div class="group-card-head"><div><span class="eyebrow">${esc(group.groupId)}</span><h3>${esc(group.groupName)}</h3><p>${esc(group.flightNumber)} · ${esc(route)}</p></div><div class="group-badges">${statusBadge(flightStatus)}${statusBadge(boardingStatus)}</div></div>
      <div class="group-metrics"><div><strong>${total}</strong><span>Members</span></div><div><strong>${checkedIn}</strong><span>Checked-in</span></div><div><strong>${pending}</strong><span>Pending check-in</span></div><div><strong>${boarded}</strong><span>Boarded</span></div></div>
      <div class="group-members-heading">Group members</div>
      <div class="group-members">${memberLines || emptyState('No members added', 'Add passengers assigned to this flight.')}</div>
      ${missing ? `<p class="group-warning">${missing} member reference(s) no longer have a passenger record.</p>` : ''}
    </article>`;
  }).join('');
}

const EMERGENCY_STATUS_VALUES = ['Open', 'In Progress', 'Resolved'];
function emergencySeverityLabel(value) {
  return ({ 1: 'Critical', 2: 'High', 3: 'Moderate' })[Number(value)] || 'High';
}
function emergencySeverityClass(value) {
  return ({ 1: 'critical', 2: 'high', 3: 'moderate' })[Number(value)] || 'high';
}
function renderEmergencyOptions() {
  const previousPrimary = $('#emPrimaryPassenger')?.value || '';
  const activePassengers = state.passengers
    .filter((passenger) => !!activeFlightForPassenger(passenger))
    .map((passenger) => ({ value: passenger.passengerId, label: `${passenger.passengerId} · ${passenger.name} · ${passenger.flightNumber}` }));
  fillSelect('emPrimaryPassenger', activePassengers, activePassengers.length ? 'Select a passenger on an active flight' : 'No passengers on active flights', previousPrimary);

  const currentPrimary = $('#emPrimaryPassenger')?.value || '';
  const affectedSelect = $('#emAffectedPassengers');
  if (affectedSelect) {
    const previouslySelected = [...affectedSelect.selectedOptions].map((option) => option.value);
    const affectedOptions = state.passengers
      .filter((passenger) => norm(passenger.passengerId) !== norm(currentPrimary))
      .map((passenger) => ({ value: passenger.passengerId, label: `${passenger.passengerId} · ${passenger.name} · ${passenger.flightNumber}` }));
    affectedSelect.innerHTML = affectedOptions.map((item) => `<option value="${esc(item.value)}">${esc(item.label)}</option>`).join('');
    [...affectedSelect.options].forEach((option) => { option.selected = previouslySelected.some((id) => norm(id) === norm(option.value)); });
    affectedSelect.disabled = affectedOptions.length === 0;
  }
  const locationSelect = $('#emLocation');
  if (locationSelect) {
    const previousLocation = locationSelect.value;
    locationSelect.innerHTML = optionMarkup(NAV_GRAPH.locations.map((location) => ({ value: location.name, label: location.name })), previousLocation, 'Select incident location');
  }
}
function emergencyCasesByPriority() {
  return [...state.emergencyCases].sort((a, b) => Number(a.severity) - Number(b.severity) ||
    (Number(String(a.caseId).replace(/\D/g, '')) || 0) - (Number(String(b.caseId).replace(/\D/g, '')) || 0));
}
function registerEmergencyCase(event) {
  event.preventDefault();
  const form = event.currentTarget;
  if (!form.reportValidity()) return;
  registerEmergencyIncident({
    primaryPassengerId: $('#emPrimaryPassenger').value,
    emergencyType: $('#emType').value,
    severity: Number($('#emSeverity').value),
    incidentLocation: $('#emLocation').value,
    description: $('#emDescription').value,
    affectedIds: [...$('#emAffectedPassengers').selectedOptions].map((option) => option.value)
  }, () => form.reset());
}

async function registerEmergencyIncident(input, onSuccess) {
  const primaryPassenger = getPassenger(input.primaryPassengerId);
  const flight = activeFlightForPassenger(primaryPassenger);
  const emergencyType = input.emergencyType;
  const severity = Number(input.severity);
  const incidentLocation = clean(input.incidentLocation);
  const description = clean(input.description);
  const selectedAffectedIds = input.affectedIds || [];

  if (!primaryPassenger || !flight) {
    return toast(
      'Primary passenger unavailable',
      'Choose a registered passenger assigned to an existing, active flight.',
      true
    );
  }

  if (![
    'Medical Emergency',
    'Fire / Smoke',
    'Security Threat',
    'Missing / Separated Passenger',
    'Other Emergency'
  ].includes(emergencyType)) {
    return toast('Invalid emergency type', 'Choose one of the available emergency types.', true);
  }

  if (![1, 2, 3].includes(severity)) {
    return toast('Invalid severity', 'Choose Critical, High or Moderate.', true);
  }

  if (!nodeByName(incidentLocation)) {
    return toast('Invalid location', 'Select an incident location from Airport Navigator.', true);
  }

  if (description.length < 3 || description.length > 300) {
    return toast('Invalid description', 'Enter a description between 3 and 300 characters.', true);
  }

  if (selectedAffectedIds.length > 20) {
    return toast('Too many affected passengers', 'Select no more than 20 additional passengers.', true);
  }

  if (selectedAffectedIds.some(
    (id) => norm(id) === norm(primaryPassenger.passengerId)
  )) {
    return toast('Duplicate affected passenger', 'The primary passenger is included automatically; remove them from the additional affected list.', true);
  }

  // Validate additional affected passengers before sending the request.
  const seen = new Set([norm(primaryPassenger.passengerId)]);

  for (const passengerId of selectedAffectedIds) {
    const key = norm(passengerId);

    if (!key || seen.has(key)) {
      return toast('Duplicate affected passenger', 'Each passenger can appear only once in an incident.', true);
    }

    if (!getPassenger(passengerId)) {
      return toast('Passenger not found', `Passenger ${passengerId} is not registered.`, true);
    }

    seen.add(key);
  }

  // The next number is synchronized from MySQL during startup.
  const number = Math.max(
    1,
    Number(state.nextEmergencyCaseNumber) || 1
  );

  const caseId = `EMR${String(number).padStart(6, '0')}`;

  if (state.emergencyCases.some(
    (incident) => norm(incident.caseId) === norm(caseId)
  )) {
    return toast('Duplicate case ID', 'Refresh the emergency register and try again.', true);
  }

  // Send the form data to the C++ API.
  const body = new URLSearchParams({
    caseId,
    primaryPassengerId: primaryPassenger.passengerId,
    emergencyType,
    severity: String(severity),
    incidentLocation,
    description,
    affectedPassengerIds: selectedAffectedIds.join(',')
  });

  try {
    const response = await apiFetch(`${API_BASE_URL}/emergencies`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
      },
      body: body.toString()
    });

    const data = await response.json();

    if (!response.ok || data.status !== 'success' || !data.incident) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    // Only update the browser after MySQL confirms successful registration.
    const savedIncident = data.incident;

    const existingIndex = state.emergencyCases.findIndex(
      (incident) => norm(incident.caseId) === norm(savedIncident.caseId)
    );

    if (existingIndex >= 0) {
      state.emergencyCases[existingIndex] = savedIncident;
    } else {
      state.emergencyCases.push(savedIncident);
    }

    state.nextEmergencyCaseNumber = number + 1;

    if (onSuccess) onSuccess();

    saveState();
    renderAll();

    toast(
      'Emergency case registered',
      `${savedIncident.caseId} saved to MySQL · ${emergencySeverityLabel(severity)} severity · ${savedIncident.affectedPassengers.length} affected passenger(s).`
    );

  } catch (error) {
    console.error('Emergency registration API error:', error);

    toast(
      'Emergency registration failed',
      error.message || 'Could not save the emergency case to MySQL.',
      true
    );
  }
}

function matchingEmergencyFacilities(emergencyType) {
  const type = norm(emergencyType);
  return NAV_GRAPH.locations.filter((location) => {
    const name = norm(location.name);
    if (type.includes('medical')) return ['medical', 'clinic', 'first aid', 'health centre', 'health center'].some((term) => name.includes(term));
    if (type.includes('fire') || type.includes('smoke')) {
  return [
    'fire station',
    'fire extinguisher',
    'fire response',
    'emergency response',
    'emergency services'
  ].some((term) => name.includes(term));
}
    if (type.includes('security')) return ['security office', 'security centre', 'security center', 'police', 'emergency response'].some((term) => name.includes(term));
    if (type.includes('missing') || type.includes('separated')) return ['information desk', 'security', 'help desk'].some((term) => name.includes(term));
    return [
  'emergency',
  'medical',
  'clinic',
  'first aid',
  'fire station',
  'fire extinguisher',
  'security office',
  'police'
].some((term) => name.includes(term));
  });
}
function findEmergencyRoute(caseId) {
  const incident = state.emergencyCases.find(
    (item) => norm(item.caseId) === norm(caseId)
  );

  if (!incident) {
    return toast(
      'Incident not found',
      `No emergency case matches ${caseId}.`,
      true
    );
  }

  const origin = nodeByName(incident.incidentLocation);

  if (!origin) {
    incident.recommendedFacility = '';
    incident.route = [];
    incident.routeMeters = null;
    incident.routeMessage =
      'Incident location is no longer available in Airport Navigator.';

    saveState();
    renderEmergencyRegister();

    return toast(
      'Incident location unavailable',
      incident.routeMessage,
      true
    );
  }

  const facilities = matchingEmergencyFacilities(
    incident.emergencyType
  );

  if (!facilities.length) {
    incident.recommendedFacility = '';
    incident.route = [];
    incident.routeMeters = null;
    incident.routeMessage =
      `No suitable facility is mapped for ${incident.emergencyType}.`;

    saveState();
    renderEmergencyRegister();

    return toast(
      'No suitable facility mapped',
      incident.routeMessage,
      true
    );
  }

  // Find the nearest suitable facility using distances in metres.
  let best = null;

  for (const facility of facilities) {
    const candidate =
      norm(origin.name) === norm(facility.name)
        ? { path: [origin.name], meters: 0 }
        : dijkstra(origin.name, facility.name);

    if (
      candidate &&
      (!best || candidate.meters < best.meters)
    ) {
      best = {
        facility: facility.name,
        path: candidate.path,
        meters: candidate.meters
      };
    }
  }

  if (!best) {
    incident.recommendedFacility = '';
    incident.route = [];
    incident.routeMeters = null;
    incident.routeMessage =
      `No route from ${origin.name} to a suitable facility was found.`;

    saveState();
    renderEmergencyRegister();

    return toast(
      'No reachable suitable facility',
      incident.routeMessage,
      true
    );
  }

  // Store the calculated route and distance.
  incident.recommendedFacility = best.facility;
  incident.route = best.path;
  incident.routeMeters = best.meters;
  incident.routeMessage =
    'Route is calculated from illustrative terminal distances.';

  saveState();
  renderEmergencyRegister();

  toast(
    'Facility route found',
    `${incident.caseId}: ${best.facility} · ` +
      `${best.meters} metres · ${formatWalkingTime(best.meters)}.`
  );
}

// Calculate and persist the nearest suitable emergency facility route.
async function findEmergencyRoute(caseId) {
  const incident = state.emergencyCases.find(
    (item) => norm(item.caseId) === norm(caseId)
  );

  if (!incident) {
    return toast(
      'Incident not found',
      `No emergency case matches ${caseId}.`,
      true
    );
  }

  let facility = '';
  let route = [];
  let routeMeters = null;
  let routeMessage = '';

  const origin = nodeByName(incident.incidentLocation);

  // Check whether the incident location exists in Airport Navigator.
  if (!origin) {
    routeMessage =
      'Incident location is no longer available in Airport Navigator.';
  } else {
    const facilities = matchingEmergencyFacilities(
      incident.emergencyType
    );

    if (!facilities.length) {
      routeMessage =
        `No suitable facility is mapped for ${incident.emergencyType}.`;
    } else {
      // Find the nearest suitable facility using Dijkstra's algorithm.
      let best = null;

      for (const candidateFacility of facilities) {
        const candidate =
          norm(origin.name) === norm(candidateFacility.name)
            ? { path: [origin.name], meters: 0 }
            : dijkstra(origin.name, candidateFacility.name);

        if (
          candidate &&
          (!best || candidate.meters < best.meters)
        ) {
          best = {
            facility: candidateFacility.name,
            path: candidate.path,
            meters: candidate.meters
          };
        }
      }

      if (!best) {
        routeMessage =
          `No route from ${origin.name} to a suitable facility was found.`;
      } else {
        facility = best.facility;
        route = best.path;
        routeMeters = best.meters;

        routeMessage =
          'Route is calculated from illustrative terminal distances.';
      }
    }
  }

  // Send the calculated route to the C++ API.
  const body = new URLSearchParams({
    caseId: incident.caseId,
    facility: facility,
    routeJson: JSON.stringify(route),
    routeMeters: routeMeters === null
      ? ''
      : String(routeMeters),
    routeMessage: routeMessage
  });

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/emergencies/route`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
        },
        body: body.toString()
      }
    );

    const data = await response.json();

    if (!response.ok || data.status !== 'success' || !data.incident) {
      throw new Error(
        data.message || 'Could not save the emergency facility route.'
      );
    }

    // Update browser state only after MySQL confirms the save.
    const updatedIncident = data.incident;

    const index = state.emergencyCases.findIndex(
      (item) => norm(item.caseId) === norm(updatedIncident.caseId)
    );

    if (index >= 0) {
      state.emergencyCases[index] = updatedIncident;
    }

    saveState();
    renderEmergencyRegister();

    if (updatedIncident.route.length > 0) {
      toast(
        'Facility route saved',
        `${updatedIncident.caseId}: ${updatedIncident.recommendedFacility} · ` +
        `${updatedIncident.routeMeters} metres · ` +
        `${formatWalkingTime(updatedIncident.routeMeters)}.`
      );
    } else {
      toast(
        'Route result saved',
        updatedIncident.routeMessage
      );
    }

  } catch (error) {
    console.error('Emergency route API error:', error);

    toast(
      'Route saving failed',
      error.message || 'Could not persist the calculated route in MySQL.',
      true
    );
  }
}

function renderEmergencyRegister() {
  const host = $('#emergencyList');
  if (!host) return;
  $('#emergencyCount').textContent = `${state.emergencyCases.length} case${state.emergencyCases.length === 1 ? '' : 's'}`;
  const term = norm($('#emergencySearch')?.value || '');
  const statusFilter = $('#emergencyStatusFilter')?.value || 'all';
  const cases = emergencyCasesByPriority().filter((incident) => {
    const affectedNames = (incident.affectedPassengers || []).flatMap((passenger) => [passenger.passengerId, passenger.passengerName]);
    const searchable = [incident.caseId, incident.primaryPassengerId, incident.primaryPassengerName, incident.flightNumber, incident.emergencyType, incident.incidentLocation, incident.description, incident.status, ...affectedNames].some((value) => norm(value).includes(term));
    return searchable && (statusFilter === 'all' || incident.status === statusFilter);
  });
  if (!cases.length) {
    host.innerHTML = emptyState(state.emergencyCases.length ? 'No matching incidents' : 'No emergency cases yet', state.emergencyCases.length ? 'Try a different search term or status filter.' : 'Register an emergency case above to build the incident register.');
    return;
  }
  host.innerHTML = cases.map((incident) => {
    const statusOptions = EMERGENCY_STATUS_VALUES.map((status) => `<option value="${status}" ${incident.status === status ? 'selected' : ''}>${status}</option>`).join('');
    const affected = (incident.affectedPassengers || []).map((passenger) => {
      const livePassenger = getPassenger(passenger.passengerId);
      return `<span class="emergency-person-chip"><strong>${esc(livePassenger?.name || passenger.passengerName || passenger.passengerId)}</strong><small>${esc(passenger.passengerId)}${livePassenger ? ` · ${esc(livePassenger.flightNumber)} · ${esc(livePassenger.status)}` : ' · Record unavailable'}</small></span>`;
    }).join('');
    const routeMarkup =
  incident.recommendedFacility &&
  incident.route?.length &&
  Number.isFinite(incident.routeMeters)
    ? `<div class="emergency-route-result">
        <strong>Nearest suitable facility: ${esc(incident.recommendedFacility)}</strong>
        <p><strong>Route:</strong> ${esc(incident.route.join(' → '))}</p>
        <p><strong>Approximate distance:</strong> ${incident.routeMeters} metres</p>
        <p><strong>Estimated walking time:</strong> ${formatWalkingTime(incident.routeMeters)}
        <span class="muted">(illustrative map data)</span></p>
      </div>`
    : incident.routeMessage
      ? `<p class="emergency-route-message">${esc(incident.routeMessage)}</p>`
      : '';
    return `<article class="emergency-case-card severity-${emergencySeverityClass(incident.severity)}">
      <div class="emergency-case-head"><div class="emergency-case-title"><span class="eyebrow">${esc(incident.caseId)} · ${esc(formatDate(incident.reportedAt))}</span><h3>${esc(incident.emergencyType)}</h3><p>${esc(incident.primaryPassengerName)} (${esc(incident.primaryPassengerId)}) · Flight ${esc(incident.flightNumber)}</p></div><div class="emergency-badges"><span class="emergency-severity-badge ${emergencySeverityClass(incident.severity)}">${esc(emergencySeverityLabel(incident.severity))} · Priority ${Number(incident.severity)}</span>${statusBadge(incident.status)}</div></div>
      <div class="emergency-case-details"><p><strong>Incident location:</strong> ${esc(incident.incidentLocation)}</p><p><strong>Description:</strong> ${esc(incident.description)}</p></div>
      <div class="emergency-affected-block"><strong>Affected passengers (${(incident.affectedPassengers || []).length})</strong><div class="emergency-people-list">${affected || '<span class="muted">No affected passengers listed.</span>'}</div></div>
      ${routeMarkup}
      <div class="emergency-case-actions"><label class="emergency-status-control">Update case status<select data-emergency-status data-case-id="${esc(incident.caseId)}" aria-label="Update status for ${esc(incident.caseId)}">${statusOptions}</select></label><button class="button button-outline" type="button" data-action="emergency-route" data-id="${esc(incident.caseId)}" ${incident.status === 'Resolved' ? 'disabled' : ''}>${incident.recommendedFacility ? 'Refresh facility route' : 'Find nearest suitable facility'}</button></div>
    </article>`;
  }).join('');
}

function suggestionsMarkup(flightNumber) {
  const selected = getFlight(flightNumber);
  if (!selected) { return emptyState('Select a flight', 'Choose a flight above to find alternatives on the same route.'); }
  const rawAlternatives = state.flights.filter((f) =>
    norm(f.flightNumber) !== norm(selected.flightNumber) &&
    norm(f.source) === norm(selected.source) &&
    norm(f.destination) === norm(selected.destination) &&
    !isUnavailableFlight(f.status) && flightDurationMinutes(f) !== null &&
    Number.isFinite(Number(f.fare)) && Number(f.fare) >= 0
  );
  if (!rawAlternatives.length) {
    return `<div class="inline-notice">Selected flight: <strong>${esc(selected.flightNumber)}</strong> · ${esc(selected.source)} → ${esc(selected.destination)} · ${esc(selected.status)} · Fare ${esc(formatFare(selected.fare))}.</div>` + emptyState('No alternatives found', 'There are no other active flights on the same route with valid fare and time values.');
  }
  const fares = rawAlternatives.map((f) => Number(f.fare));
  const durations = rawAlternatives.map((f) => flightDurationMinutes(f));
  const minFare = Math.min(...fares), maxFare = Math.max(...fares);
  const minDuration = Math.min(...durations), maxDuration = Math.max(...durations);
  const alternatives = rawAlternatives.map((f) => {
    const fare = Number(f.fare);
    const duration = flightDurationMinutes(f);
    const fareScore = maxFare === minFare ? 1 : (maxFare - fare) / (maxFare - minFare);
    const durationScore = maxDuration === minDuration ? 1 : (maxDuration - duration) / (maxDuration - minDuration);
    return { ...f, computedDuration: duration, recommendationScore: (fareScore * 0.5 + durationScore * 0.5) * 100 };
  }).sort((a, b) => b.recommendationScore - a.recommendationScore || Number(a.fare) - Number(b.fare) || a.computedDuration - b.computedDuration || timeNumber(a.departureTime) - timeNumber(b.departureTime));
  const selectedDuration = flightDurationMinutes(selected);
  const cards = alternatives.map((f, index) => `<div class="suggestion-card ${index === 0 ? 'top-recommendation' : ''}">
    <div class="suggestion-main"><div class="suggestion-heading"><strong>${esc(f.flightNumber)} · ${esc(f.airline)}</strong>${index === 0 ? '<span class="recommendation-badge">Top recommended</span>' : `<span class="rank-badge">#${index + 1}</span>`}</div>
      <small>${esc(f.source)} → ${esc(f.destination)} · Gate ${esc(f.gate)} · ${esc(f.status)}</small>
      <div class="suggestion-metrics"><span><small>Fare</small><strong>${esc(formatFare(f.fare))}</strong></span><span><small>Flight time</small><strong>${esc(formatDuration(f.computedDuration))}</strong></span><span><small>Departs</small><strong>${esc(f.departureTime)}</strong></span><span><small>Score</small><strong>${f.recommendationScore.toFixed(1)} / 100</strong></span></div>
    </div>
  </div>`).join('');
  return `<div class="inline-notice">Selected flight: <strong>${esc(selected.flightNumber)}</strong> · ${esc(selected.source)} → ${esc(selected.destination)} · ${esc(selected.status)} · Fare ${esc(formatFare(selected.fare))} · Flight time ${esc(formatDuration(selectedDuration))}. Alternatives are ranked by a 50% fare score + 50% duration score; higher score is better.</div><div class="suggestion-list">${cards}</div><p class="muted recommendation-footnote">Score is normalized among the alternatives displayed for this route. It is a demo recommendation, not a live fare quote.</p>`;
}


async function renderSuggestions() {
  const output = $('#suggestOut');
  const flightNumber = $('#suggestFlight').value;

  if (!flightNumber) {
    output.innerHTML = emptyState(
      'Select a flight',
      'Choose a flight to find alternatives.'
    );
    return;
  }

  output.innerHTML = '<div class="inline-notice">Finding smart flight suggestions...</div>';

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/flights/suggestions?flightNumber=${encodeURIComponent(flightNumber)}`
    );

    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    const selected = getFlight(flightNumber);

    if (!selected) {
      throw new Error('Selected flight was not found in the local flight records.');
    }

    const alternatives = data.alternatives || [];

    if (!alternatives.length) {
      output.innerHTML =
        `<div class="inline-notice">
          Selected flight: <strong>${esc(selected.flightNumber)}</strong>
          · ${esc(selected.source)} → ${esc(selected.destination)}
        </div>` +
        emptyState(
          'No alternatives found',
          'There are no other eligible flights on this route.'
        );
      return;
    }

    const cards = alternatives.map((f, index) => `
      <div class="suggestion-card ${index === 0 ? 'top-recommendation' : ''}">
        <div class="suggestion-main">

          <div class="suggestion-heading">
            <strong>${esc(f.flightNumber)} · ${esc(f.airline)}</strong>
            ${index === 0
              ? '<span class="recommendation-badge">Top recommended</span>'
              : `<span class="rank-badge">#${index + 1}</span>`}
          </div>

          <small>
            ${esc(f.source)} → ${esc(f.destination)}
            · ${esc(f.status)}
          </small>

          <div class="suggestion-metrics">
            <span>
              <small>Fare</small>
              <strong>${esc(formatFare(f.fare))}</strong>
            </span>

            <span>
              <small>Flight time</small>
              <strong>${esc(formatDuration(Number(f.durationMinutes)))}</strong>
            </span>

            <span>
              <small>Departs</small>
              <strong>${esc(f.departureTime)}</strong>
            </span>

            <span>
              <small>Score</small>
              <strong>${Number(f.recommendationScore).toFixed(1)} / 100</strong>
            </span>
          </div>

        </div>
      </div>
    `).join('');

    output.innerHTML = `
      <div class="inline-notice">
        Selected flight:
        <strong>${esc(selected.flightNumber)}</strong>
        · ${esc(selected.source)} → ${esc(selected.destination)}
        · ${esc(selected.status)}
        · Fare ${esc(formatFare(selected.fare))}
      </div>

      <div class="suggestion-list">
        ${cards}
      </div>

      <p class="muted recommendation-footnote">
        Recommendations are generated by the C++ SmartFlightSuggestion
        algorithm using fare and flight duration.
      </p>
    `;

  } catch (error) {
    console.error('SmartFlightSuggestion API error:', error);

    output.innerHTML = emptyState(
      'Unable to load suggestions',
      'Could not retrieve recommendations from the AeroStruct API. Check whether the API server is running.'
    );
  }
}



let adminAssistRequest = 0;

function assistIcon(type) {
  const lower = norm(type);

  if (lower.includes('emergency')) return '!';
  if (lower.includes('medical') || lower.includes('medicine')) return '+';
  if (lower.includes('wheelchair') || lower.includes('accessibility') ||
      lower.includes('mobility')) return '♿';
  if (lower.includes('elderly') || lower.includes('senior')) return '♡';
  if (lower.includes('vip')) return '★';

  return 'i';
}

async function getAeroAssistAdvice(assistanceType) {
  const response = await apiFetch(
    `${API_BASE_URL}/assistance/advice?assistanceType=${encodeURIComponent(assistanceType || 'General')}`
  );

  const data = await response.json();

  if (!response.ok) {
    throw new Error(data.message || `HTTP error: ${response.status}`);
  }

  if (
    data.priority === undefined ||
    !data.category ||
    !data.supportTeam ||
    !data.recommendedAction
  ) {
    throw new Error('Invalid AeroAssist response received from API.');
  }

  return data;
}

function assistMarkup(p, loc, advice) {
  if (!p) {
    return emptyState(
      'Select a passenger',
      'Register a passenger to view an assistance recommendation.'
    );
  }

  const statusIndex = ({
    Registered: 0,
    'Checked-In': 1,
    'Security-Cleared': 1,
    Boarded: 2
  })[p.status] ?? 0;

  const flight = getFlight(p.flightNumber);
  const routeStatus = flight ? flight.status : 'Flight not found';

  return `
    <div class="advice-card">
      <div class="advice-top">
        <span class="advice-icon">${assistIcon(p.assistanceType)}</span>

        <div>
          <span class="eyebrow">
            SUGGESTED SUPPORT · PRIORITY ${esc(advice.priority)}
          </span>

          <h3>${esc(advice.category)}</h3>

          <p>
            Recommended team:
            <strong>${esc(advice.supportTeam)}</strong>
          </p>
        </div>
      </div>

      <p>${esc(advice.recommendedAction)}</p>

      <p>
        <strong>Passenger:</strong>
        ${esc(p.name)} (${esc(p.passengerId)})
        · <strong>Flight:</strong> ${esc(p.flightNumber)}
        · <strong>Location selected:</strong> ${esc(loc)}
      </p>

      <p>
        <strong>Flight status:</strong>
        ${statusBadge(routeStatus)}
        &nbsp;
        <strong>Journey:</strong> ${statusBadge(p.status)}
      </p>

      <ol class="timeline">
        <li class="${statusIndex >= 0 ? 'done' : ''}">Registered</li>
        <li class="${statusIndex >= 1 ? 'done' : ''}">Checked-In</li>
        <li class="${statusIndex >= 2 ? 'done' : ''}">Boarded</li>
      </ol>

      <p class="muted">
        Recommendations are generated by the C++ AeroAssist module.
        This is guidance, not a medical triage system, and does not
        automatically contact a support team.
      </p>
    </div>
  `;
}

async function renderAssist() {
  const output = $('#asOut');
  const requestId = ++adminAssistRequest;
  const passenger = getPassenger($('#asPax').value);
  const location = $('#asLoc').value;

  if (!passenger) {
    output.innerHTML = assistMarkup(null, location);
    return;
  }

  output.innerHTML =
    '<div class="inline-notice">Generating AeroAssist recommendation...</div>';

  try {
    const advice = await getAeroAssistAdvice(passenger.assistanceType);

    if (requestId !== adminAssistRequest) return;

    output.innerHTML = assistMarkup(passenger, location, advice);

  } catch (error) {
    if (requestId !== adminAssistRequest) return;

    console.error('Admin AeroAssist API error:', error);

    output.innerHTML = emptyState(
      'AeroAssist unavailable',
      error.message || 'Could not retrieve assistance advice from the API.'
    );
  }
}


function nodeByName(name) { return NAV_GRAPH.locations.find((loc) => norm(loc.name) === norm(name)); }
function adjacency() {
  const map = new Map(NAV_GRAPH.locations.map((l) => [l.name, []]));
  NAV_GRAPH.edges.forEach(([from, to, weight]) => {
    map.get(from).push({ to, weight }); map.get(to).push({ to: from, weight });
  });
  return map;
}
function dijkstra(source, destination) {
  const graph = adjacency();
  if (!graph.has(source) || !graph.has(destination)) return null;
  const dist = new Map(NAV_GRAPH.locations.map((l) => [l.name, Infinity]));
  const prev = new Map(); const unvisited = new Set(dist.keys()); dist.set(source, 0);
  while (unvisited.size) {
    let current = null; let best = Infinity;
    for (const name of unvisited) if (dist.get(name) < best) { best = dist.get(name); current = name; }
    if (current === null || best === Infinity) break;
    unvisited.delete(current);
    if (current === destination) break;
    for (const edge of graph.get(current)) {
      if (!unvisited.has(edge.to)) continue;
      const candidate = best + edge.weight;
      if (candidate < dist.get(edge.to)) { dist.set(edge.to, candidate); prev.set(edge.to, current); }
    }
  }
  if (dist.get(destination) === Infinity) return null;
  const path = []; let current = destination;
  while (current !== undefined) { path.unshift(current); if (current === source) break; current = prev.get(current); }
  if (path[0] !== source) return null;
  return { path, meters: dist.get(destination) };
}

function formatWalkingTime(distanceMeters) {
  const totalSeconds = Math.round(
    distanceMeters / WALKING_SPEED_METERS_PER_SECOND
  );

  if (totalSeconds < 60) {
    return `about ${totalSeconds} seconds`;
  }

  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;

  if (seconds === 0) {
    return `about ${minutes} minute${minutes === 1 ? '' : 's'}`;
  }

  return `about ${minutes} min ${seconds} sec`;
}

function renderNavigator() {
  const airportItems = availableAirports.map(airport => ({
    value: airport.airportCode,
    label: `${airport.airportCode} · ${airport.city}`
  }));

  fillSelect(
    'navAirport',
    airportItems,
    'Choose airport',
    currentAirportCode
  );

  const items = NAV_GRAPH.locations.map(location => ({
    value: location.name,
    label: location.name
  }));

  fillSelect('navFrom', items, 'Choose starting location');
  fillSelect('navTo', items, 'Choose destination');
  fillSelect('asLoc', items, 'Choose current location');

  const firstLocation = NAV_GRAPH.locations[0]?.name || '';

  const firstGate =
    NAV_GRAPH.locations.find(
      location => norm(location.type) === 'gate'
    )?.name || NAV_GRAPH.locations.at(-1)?.name || firstLocation;

  if (!nodeByName($('#navFrom').value)) {
    $('#navFrom').value = firstLocation;
  }

  if (!nodeByName($('#navTo').value)) {
    $('#navTo').value = firstGate;
  }

  if (!nodeByName($('#asLoc').value)) {
    $('#asLoc').value = firstLocation;
  }

  if (navRunActive('map')) return;

  const mapEl = $('#map');

  mapEl.classList.remove('is-navigating', 'has-arrived');

  mapEl.innerHTML = buildMapMarkup(
    $('#navFrom').value,
    $('#navTo').value,
    activeRoutePath,
    'data-node'
  );

  if (activeRoutePath.length) {
    $('#navOut').innerHTML = routeInfoMarkup(
      activeRoutePath,
      activeRouteWeight,
      activeRouteMinutes
    );
  }
}


// Find shortest route through the C++ Dijkstra API.
async function findRoute() {
  const from = $('#navFrom').value;
  const to = $('#navTo').value;
  const requestedAirport = currentAirportCode;

  if (!from || !to) {
    return toast(
      'Locations required',
      'Choose both a starting point and destination.',
      true
    );
  }

  if (norm(from) === norm(to)) {
    return toast(
      'Same location',
      'Choose two different locations to find a route.',
      true
    );
  }

  cancelNavRun('map');

  try {
    const result = await requestNavigatorRoute(from, to);

    // Ignore a response if the user switched airports while it was running.
    if (requestedAirport !== currentAirportCode) return;

    activeRoutePath = result.path;
    activeRouteWeight = Number(result.distanceMeters);
    activeRouteMinutes = Number(result.totalMinutes);

    renderNavigator();

    toast(
      'Route found',
      `${requestedAirport}: ${activeRouteMinutes} minutes · ` +
      `${activeRouteWeight} metres.`
    );

  } catch (error) {
    activeRoutePath = [];
    activeRouteWeight = null;
    activeRouteMinutes = null;

    $('#navOut').textContent = error.message;

    renderNavigator();

    toast('Route calculation failed', error.message, true);
  }
}


// Run BFS or DFS through the C++ backend.
async function traverseGraph(type) {
  const start = $('#navFrom').value;
  const requestedAirport = currentAirportCode;
  const algorithm = type.toUpperCase();

  if (!start) {
    return toast(
      'Starting location required',
      'Select the starting location first.',
      true
    );
  }

  cancelNavRun('map');

  try {
    const result = await requestNavigatorTraversal(start, algorithm);

    if (requestedAirport !== currentAirportCode) return;

    activeRoutePath = [];
    activeRouteWeight = null;
    activeRouteMinutes = null;

    $('#navOut').innerHTML =
      `<strong>${esc(algorithm)} traversal order:</strong> ` +
      `${esc(result.order.join(' → '))}` +
      `<br><span class="muted">Traversal performed by the C++ ` +
      `${esc(algorithm)} algorithm on ${esc(requestedAirport)}.</span>`;

    renderNavigator();

    toast(
      `${algorithm} completed`,
      `${result.order.length} reachable locations visited in ${requestedAirport}.`
    );

  } catch (error) {
    toast('Traversal failed', error.message, true);
  }
}


function setNavigatorDestination(locationName) {
  if (!nodeByName(locationName)) return;
  $('#navTo').value = locationName;
  findRoute();
}
const EMERGENCY_HELP_GUIDANCE = {
  general: {
    title: "General medical emergency",
    alert: "Call 112 immediately for collapse, severe breathing difficulty, severe chest pain or other life-threatening symptoms.",
    steps: [
      "Check that the area is safe before approaching.",
      "Ask someone to call 112 and bring an AED or first-aid kit if available.",
      "Stay with the person and monitor responsiveness and breathing.",
      "Follow the emergency operator's instructions. If the person is unresponsive and not breathing normally, provide CPR or use an AED if trained or guided to do so."
    ],
    source: "https://www.redcross.org/take-a-class/first-aid/performing-first-aid/first-aid-steps"
  },

  stroke: {
    title: "Possible stroke",
    alert: "Sudden facial drooping, arm weakness or difficulty speaking may indicate a stroke. Call 112 immediately.",
    steps: [
      "Note the time symptoms started, or when the person was last known to be well.",
      "Keep the person safe and stay with them.",
      "Do not give food, drink or medication while awaiting professional help.",
      "Monitor breathing and responsiveness, and follow the operator's instructions."
    ],
    source: "https://www.redcross.org/take-a-class/resources/learn-first-aid/stroke"
  },

  choking: {
    title: "Choking",
    alert: "If the person cannot cough, speak or breathe, treat it as an emergency and call 112.",
    steps: [
      "Ask someone nearby to call for emergency help and get a trained first aider.",
      "Use age-appropriate choking first aid only if trained, or follow instructions from the emergency operator.",
      "Do not perform a blind finger sweep inside the mouth.",
      "If the person becomes unresponsive and is not breathing normally, follow emergency instructions and provide CPR if trained or guided."
    ],
    source: "https://www.redcross.org/take-a-class/resources/learn-first-aid/adult-child-choking"
  },

  bleeding: {
    title: "Severe bleeding",
    alert: "Heavy, continuous or spurting bleeding needs immediate emergency help. Call 112.",
    steps: [
      "Make sure the scene is safe and use gloves or a barrier if available.",
      "Apply firm, continuous direct pressure to the wound with clean gauze or cloth.",
      "Do not remove an object embedded in the wound; apply pressure around it.",
      "Stay with the person and follow the emergency operator's instructions."
    ],
    source: "https://www.redcross.org/take-a-class/resources/learn-first-aid/bleeding-life-threatening-external"
  },

  burns: {
    title: "Burns",
    alert: "Call 112 for severe or extensive burns, breathing problems, or serious electrical or chemical burns.",
    steps: [
      "Move away from the source of the burn if it is safe.",
      "Cool the burn with clean, cool running water as soon as possible.",
      "Remove jewellery or clothing near the burn only if it is not stuck to the skin.",
      "Do not apply ice, butter or toothpaste. Seek medical help for serious burns."
    ],
    source: "https://www.redcross.org/take-a-class/resources/learn-first-aid/burns"
  },

  seizure: {
    title: "Seizure",
    alert: "Call 112 for a first seizure, a seizure lasting more than five minutes, repeated seizures, injury or breathing difficulty.",
    steps: [
      "Move nearby objects away to help prevent injury.",
      "Protect the person's head if possible and time the seizure.",
      "Do not restrain the person and do not put anything in their mouth.",
      "Stay with them, monitor breathing, and follow emergency operator instructions."
    ],
    source: "https://www.redcross.org/take-a-class/resources/learn-first-aid/seizures"
  },

  fire: {
    title: "Fire or smoke",
    alert: "Alert airport emergency staff and call 112 if there is immediate danger.",
    steps: [
      "Move away from fire and smoke using a safe exit.",
      "Raise the alarm and follow airport evacuation instructions.",
      "Use stairs rather than lifts during evacuation.",
      "Do not re-enter the affected area or attempt to fight a fire unless trained and it is safe."
    ]
  },

  security: {
    title: "Security threat",
    alert: "If anyone is in immediate danger, move to safety and call 112.",
    steps: [
      "Move away from the threat without putting yourself or others at greater risk.",
      "Notify airport security or the nearest authorised staff member.",
      "Do not approach or touch suspicious objects.",
      "Follow instructions from airport security and emergency responders."
    ]
  },

  missing: {
    title: "Missing or separated passenger",
    alert: "Notify the nearest airport help desk or security team promptly, especially if a child or vulnerable person is missing.",
    steps: [
      "Provide the person's name, description and last known location to authorised staff.",
      "Share when and where the person was last seen.",
      "Follow instructions from airport security or the help desk.",
      "Avoid publicly sharing private contact details or sensitive personal information."
    ]
  }
};


function renderEmergencyGuidance(selectId = "emHelpType", hostId = "emFirstAidAdvice") {
  const select = document.getElementById(selectId);
  const host = document.getElementById(hostId);

  if (!select || !host) return;

  const guidance =
    EMERGENCY_HELP_GUIDANCE[select.value] ||
    EMERGENCY_HELP_GUIDANCE.general;

  const steps = guidance.steps
    .map((step) => `<li>${esc(step)}</li>`)
    .join("");

  const sourceLink = guidance.source
    ? `<p><a href="${esc(guidance.source)}"
         target="_blank"
         rel="noopener noreferrer">
         Read detailed first-aid guidance
       </a></p>`
    : "";

  host.innerHTML = `
    <article class="emergency-first-aid-card">
      <h3>${esc(guidance.title)}</h3>

      <p class="emergency-guide-alert">
        <strong>Immediate action:</strong>
        ${esc(guidance.alert)}
      </p>

      <h4>What to do</h4>
      <ol>${steps}</ol>

      ${sourceLink}
    </article>
  `;
}


function openNearbyHospitalMap(inputId = "emHospitalArea") {
  const area = clean(
    document.getElementById(inputId).value
  );

  const query = area
    ? `hospitals in ${area}`
    : "hospitals near me";

  const mapUrl = new URL(
    "https://www.google.com/maps/search/"
  );

  mapUrl.searchParams.set("api", "1");
  mapUrl.searchParams.set("query", query);

  window.open(
    mapUrl.toString(),
    "_blank",
    "noopener,noreferrer"
  );
}
function unreadCount() { return state.notifications.filter((n) => !n.isRead).length; }
function formatDate(value) {
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return 'Recent';
  return date.toLocaleString('en-IN', { day: '2-digit', month: 'short', hour: '2-digit', minute: '2-digit' });
}
function validClockTime(value) { return /^([01]\d|2[0-3]):[0-5]\d$/.test(String(value || '')); }
function updateConnectionTimeFromFlight(selectId, timeInputId, fieldName) {
  const flight = getFlight(document.getElementById(selectId).value);
  if (flight && validClockTime(flight[fieldName])) document.getElementById(timeInputId).value = flight[fieldName];
}
function syncPassengerArrivalFlight() {
  const passenger = getPassenger($('#crPassenger').value);
  if (!passenger) return;
  const arrivingFlight = getFlight(passenger.flightNumber);
  if (arrivingFlight && !isUnavailableFlight(arrivingFlight.status)) $('#crArrFlight').value = arrivingFlight.flightNumber;
  updateConnectionTimeFromFlight('crArrFlight', 'crArrivalTime', 'arrivalTime');
}
function checkConnectionRisk(event) {
  event.preventDefault();
  const form = event.currentTarget;
  if (!form.reportValidity()) return;
  runConnectionRisk({
    passengerId: $('#crPassenger').value,
    arrivingFlight: $('#crArrFlight').value,
    connectingFlight: $('#crNextFlight').value,
    arrivalTime: $('#crArrivalTime').value,
    nextDepartureTime: $('#crNextDeparture').value,
    transferMinutes: $('#crTransfer').value,
    minimumBufferMinutes: $('#crMinBuffer').value
  }, $('#connectionRiskResult'));
}
function runConnectionRisk(vals, resultEl) {
  const passenger = getPassenger(vals.passengerId);
  const arrivingFlight = getFlight(vals.arrivingFlight);
  const connectingFlight = getFlight(vals.connectingFlight);
  const arrivalTime = vals.arrivalTime;
  const nextDepartureTime = vals.nextDepartureTime;
  const transferMinutes = Number(vals.transferMinutes);
  const minimumBufferMinutes = Number(vals.minimumBufferMinutes);
  if (!passenger || !arrivingFlight || !connectingFlight) return toast('Missing connection details', 'Select a passenger, arriving flight and connecting flight.', true);
  if (isUnavailableFlight(arrivingFlight.status) || isUnavailableFlight(connectingFlight.status)) return toast('Flight unavailable', 'Choose active flights for the connection check.', true);
  if (norm(arrivingFlight.flightNumber) === norm(connectingFlight.flightNumber)) return toast('Choose two flights', 'The arriving and connecting flights must be different.', true);
  if (!validClockTime(arrivalTime) || !validClockTime(nextDepartureTime)) return toast('Invalid time', 'Use valid 24-hour times in HH:MM format.', true);
  if (!Number.isInteger(transferMinutes) || transferMinutes < 0 || !Number.isInteger(minimumBufferMinutes) || minimumBufferMinutes < 0) return toast('Invalid minutes', 'Transfer time and minimum buffer must be non-negative whole minutes.', true);
  const arrival = timeNumber(arrivalTime);
  let departure = timeNumber(nextDepartureTime);
  // Like the C++ NotificationManager check, an earlier clock time is treated as the next day.
  if (departure < arrival) departure += 24 * 60;
  const availableBufferMinutes = departure - (arrival + transferMinutes);
  const result = resultEl;
  if (availableBufferMinutes >= minimumBufferMinutes) {
    result.innerHTML = `<div class="risk-result risk-safe"><strong>✓ Connection buffer looks sufficient</strong><p>Estimated available buffer: <b>${availableBufferMinutes} minute(s)</b>. Minimum configured buffer: ${minimumBufferMinutes} minute(s). No risk notification was created.</p></div>`;
    toast('Connection check complete', `Available buffer: ${availableBufferMinutes} minutes. No alert required.`);
    return;
  }
  let detail;
  if (availableBufferMinutes < 0) detail = `The estimated transfer would finish ${Math.abs(availableBufferMinutes)} minute(s) after the scheduled departure time.`;
  else detail = `Only ${availableBufferMinutes} minute(s) remain after the estimated transfer.`;
  const message = `Hello ${passenger.name}, a tight connection has been detected between arriving flight ${arrivingFlight.flightNumber} and connecting flight ${connectingFlight.flightNumber}. Arrival time: ${arrivalTime}. Estimated transfer time: ${transferMinutes} minute(s). Next departure: ${nextDepartureTime}. ${detail} Minimum required buffer: ${minimumBufferMinutes} minute(s). Please contact the airline or service desk for connection assistance.`;
  notify(
  passenger,
  'Connection Risk',
  message,
  connectingFlight.flightNumber
   );
  saveState(); renderAll();
  resultEl.innerHTML = `<div class="risk-result risk-danger"><strong>⚠ Connection risk detected — notification created</strong><p>${esc(detail)} Available buffer: <b>${availableBufferMinutes} minute(s)</b>. Minimum required: ${minimumBufferMinutes} minute(s).</p><p>Open the notification history below to review the alert for ${esc(passenger.name)}.</p></div>`;
  toast('Connection risk detected', `Created an in-app alert for ${passenger.name} on connecting flight ${connectingFlight.flightNumber}.`, true);
}

function notificationIcon(type) {
  if (type === 'Connection Risk' || type === 'Cancellation') return icon('alert', 18);
  if (type === 'Delay') return icon('clock', 18);
  if (type === 'Boarding Reminder') return icon('plane', 18);
  if (type === 'Check-In Reminder') return icon('check', 18);
  if (type === 'Gate Change') return icon('flow', 18);
  return icon('info', 18);
}
function renderNotifications() {
  const term = norm($('#noteSearch').value);
  const filter = $('#noteFilter').value;
  const notes = state.notifications.filter((note) => {
    const matchText = [note.passengerName, note.passengerId, note.flightNumber, note.type, note.message].some((value) => norm(value).includes(term));
    return matchText && (filter === 'all' || (filter === 'unread' && !note.isRead) || (filter === 'read' && note.isRead));
  });
  $('#noteList').innerHTML = notes.length ? notes.map((n) => `<div class="notification-item ${n.isRead ? '' : 'unread'}"><span class="notification-icon">${notificationIcon(n.type)}</span><div class="notification-copy"><strong>${esc(n.type)} · ${esc(n.passengerName)}</strong><p>${esc(n.message)}</p><small>${esc(n.passengerId)} · ${esc(n.flightNumber)} · ${formatDate(n.createdAt)} · ${n.isRead ? 'Read' : 'Unread'}</small></div><div class="notification-actions">
  ${n.isRead
  ? '<span class="status-badge ok">Read</span>'
  : Number(n.notificationId) > 0
    ? `<button class="row-action" type="button" data-action="mark-read" data-id="${n.notificationId}">Mark read</button>`
    : '<span class="status-badge warn">Saving...</span>'}</div></div>`).join('') : emptyState('No notifications found', 'Change the filter or perform an operation that creates a notification.');
}
async function markNotificationRead(id) 
{
  const note = state.notifications.find(
    (item) => Number(item.notificationId) === Number(id)
  );

  if (!note) return;

  if (Number(note.notificationId) <= 0) {
    return toast(
      'Notification is being saved',
      'Try marking it as read after the database save completes.'
    );
  }

  if (note.isRead) return;

  try {
    const body = new URLSearchParams({
      notificationId: String(note.notificationId)
    });

    const response = await apiFetch(
      `${API_BASE_URL}/notifications/read`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
        },
        body: body.toString()
      }
    );

    const data = await response.json();

    if (!response.ok || data.status !== 'success') {
      throw new Error(data.message || 'Could not update notification.');
    }

    note.isRead = true;

    saveState();
    renderAll();

    toast(
      'Notification marked read',
      `${note.passengerName}'s notification was marked as read.`
    );

  } catch (error) {
    console.error('Mark notification read error:', error);

    toast(
      'Notification update failed',
      error.message || 'Could not update notification in MySQL.',
      true
    );
  }
}

async function markAllRead() {
  const count = state.notifications.filter(
    (notification) => !notification.isRead
  ).length;

  if (!count) {
    return toast(
      'No unread notifications',
      'All notifications have already been read.'
    );
  }

  try {
    const response = await apiFetch(
      `${API_BASE_URL}/notifications/read-all`,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
        },
        body: ''
      }
    );

    const data = await response.json();

    if (!response.ok || data.status !== 'success') {
      throw new Error(data.message || 'Could not update notifications.');
    }

    state.notifications.forEach((notification) => {
      notification.isRead = true;
    });

    saveState();
    renderAll();

    toast(
      'Notifications updated',
      `${Number(data.markedCount) || count} notification(s) marked as read.`
    );

  } catch (error) {
    console.error('Mark all notifications read error:', error);

    toast(
      'Notification update failed',
      error.message || 'Could not update notification history in MySQL.',
      true
    );
  }
}

const BAGGAGE_STATUS_VALUES = [
  'Registered',
  'Loaded',
  'Arrived',
  'Collected',
  'On Hold'
];

const BAGGAGE_ISSUE_TYPES = [
  'Delayed',
  'Missing',
  'Damaged',
  'Other'
];

const BAGGAGE_ISSUE_STATUSES = [
  'Reported',
  'Under Review',
  'Resolved'
];

function renderBaggageOptions() {
  const eligiblePassengers = state.passengers.filter((passenger) => {
    return (
      ['checked-in', 'security-cleared'].includes(norm(passenger.status)) &&
      Boolean(activeFlightForPassenger(passenger))
    );
  });

  fillSelect(
    'baggagePassenger',
    eligiblePassengers.map((passenger) => ({
      value: passenger.passengerId,
      label: `${passenger.passengerId} · ${passenger.name} · ${passenger.flightNumber}`
    })),
    'Select an eligible passenger'
  );

  const activeFlights = state.flights.filter(
    (flight) => !isUnavailableFlight(flight.status)
  );

  fillSelect(
    'baggageFlight',
    activeFlights.map((flight) => ({
      value: flight.flightNumber,
      label: `${flight.flightNumber} · ${flight.destination} · ${flight.status}`
    })),
    'Select an active flight'
  );

  fillSelect(
    'baggageIssueTag',
    state.baggageRecords
      .filter((bag) => !bag.issueType)
      .map((bag) => ({
        value: bag.tag,
        label: `${bag.tag} · ${bag.passengerName}`
      })),
    'Select baggage tag'
  );
}

function registerBaggage(event) {
  event.preventDefault();
  registerBaggageRecord({
    passengerId: $('#baggagePassenger').value,
    flightNumber: $('#baggageFlight').value,
    weight: $('#baggageWeight').value,
    description: $('#baggageDescription').value
  }, () => $('#baggageForm').reset());
}
async function registerBaggageRecord(input, onSuccess) {
  if (state.baggageRecords.length >= 500) {
    return toast(
      'Baggage limit reached',
      'This demo supports up to 500 baggage records.',
      true
    );
  }

  const passengerId = clean(input.passengerId);
  const flightNumber = clean(input.flightNumber);
  const weight = Number(input.weight);
  const description = clean(input.description);

  const passenger = getPassenger(passengerId);
  const flight = getFlight(flightNumber);

  if (!passenger) {
    return toast('Invalid passenger', 'Select an existing passenger.', true);
  }

  if (!['checked-in', 'security-cleared'].includes(norm(passenger.status))) {
    return toast(
      'Passenger not eligible',
      'The passenger must be Checked-In or Security-Cleared.',
      true
    );
  }

  if (!flight || isUnavailableFlight(flight.status)) {
    return toast(
      'Flight unavailable',
      'Select an existing, active flight.',
      true
    );
  }

  if (norm(passenger.flightNumber) !== norm(flight.flightNumber)) {
    return toast(
      'Flight mismatch',
      'Baggage must be registered against the passenger’s assigned flight.',
      true
    );
  }

  if (!Number.isFinite(weight) || weight < 0.1 || weight > 50) {
    return toast(
      'Invalid baggage weight',
      'Enter a weight between 0.1 and 50 kg.',
      true
    );
  }

  if (!description || description.length > 100) {
    return toast(
      'Invalid description',
      'Enter a baggage description of up to 100 characters.',
      true
    );
  }

  try {
    const data = await baggageApiPost('/baggage', {
      passengerId: passenger.passengerId,
      flightNumber: flight.flightNumber,
      weightKg: String(weight),
      description: description
    });

    if (onSuccess) onSuccess();

    const record = data.record || {};
    const tag = record.tag || record.baggageTag || 'New baggage';

    toast(
      'Baggage registered',
      `${tag} registered successfully and added to the FIFO queue.`
    );

  } catch (error) {
    toast(
      'Baggage registration failed',
      error.message,
      true
    );
  }
}


async function processNextBaggage() {
  try {
    const data = await baggageApiPost('/baggage/process-next');

    const record = data.record || {};
    const tag = record.tag || record.baggageTag || 'Baggage';

    toast(
      'Baggage loaded',
      `${tag} has been processed from the FIFO queue.`
    );

  } catch (error) {
    toast(
      'Could not process baggage',
      error.message,
      true
    );
  }
}


async function markBaggageArrived(tag) {
  const baggage = state.baggageRecords.find(
    (bag) => norm(bag.tag) === norm(tag)
  );

  if (!baggage) {
    return toast('Baggage not found', `No bag matches ${tag}.`, true);
  }

  if (baggage.status !== 'Loaded') {
    return toast(
      'Invalid baggage status',
      'Only loaded baggage can be marked as arrived.',
      true
    );
  }

  const flight = getFlight(baggage.flightNumber);

  if (!flight || isCancelled(flight.status)) {
    return toast(
      'Flight unavailable',
      'Arrival cannot be recorded for a missing or cancelled flight.',
      true
    );
  }

  try {
    await baggageApiPost('/baggage/arrived', { tag });

    toast(
      'Baggage arrived',
      `${baggage.tag} is now marked as Arrived.`
    );

  } catch (error) {
    toast('Arrival update failed', error.message, true);
  }
}


async function markBaggageCollected(tag) {
  const baggage = state.baggageRecords.find(
    (bag) => norm(bag.tag) === norm(tag)
  );

  if (!baggage) {
    return toast('Baggage not found', `No bag matches ${tag}.`, true);
  }

  if (baggage.status !== 'Arrived') {
    return toast(
      'Baggage not ready',
      'Only arrived baggage can be collected.',
      true
    );
  }

  const verification = await uiDialog({
    title: `Release baggage ${baggage.tag}`,
    subtitle: 'Ask the passenger for their ID before handing over the bag.',
    submitLabel: 'Confirm collection',
    iconName: 'bag',
    fields: [{ name: 'passengerId', label: 'Passenger ID', placeholder: 'e.g. P101', maxlength: 30, full: true }]
  });

  if (!verification) return;
  const enteredPassengerId = verification.passengerId;

  if (norm(enteredPassengerId) !== norm(baggage.passengerId)) {
    return toast(
      'Passenger verification failed',
      'The passenger ID does not match this baggage record.',
      true
    );
  }

  try {
    await baggageApiPost('/baggage/collected', {
      tag: baggage.tag,
      passengerId: enteredPassengerId.trim()
    });

    toast(
      'Baggage collected',
      `${baggage.tag} has been collected by the registered passenger.`
    );

  } catch (error) {
    toast('Collection failed', error.message, true);
  }
}


async function requeueHeldBaggage(tag) {
  const baggage = state.baggageRecords.find(
    (bag) => norm(bag.tag) === norm(tag)
  );

  if (!baggage) {
    return toast('Baggage not found', `No bag matches ${tag}.`, true);
  }

  if (baggage.status !== 'On Hold') {
    return toast(
      'Invalid baggage status',
      'Only baggage currently On Hold can be requeued.',
      true
    );
  }

  const flight = getFlight(baggage.flightNumber);

  if (!flight || isUnavailableFlight(flight.status)) {
    return toast(
      'Flight still unavailable',
      'Resolve the flight issue before requeueing baggage.',
      true
    );
  }

  try {
    await baggageApiPost('/baggage/requeue', {
      tag: baggage.tag
    });

    toast(
      'Baggage requeued',
      `${baggage.tag} was added to the end of the handling queue.`
    );

  } catch (error) {
    toast('Baggage requeue failed', error.message, true);
  }
}


async function reportBaggageIssueRecord(
  rawTag,
  rawIssueType,
  rawDescription,
  onSuccess
) {
  const tag = clean(rawTag);
  const issueType = clean(rawIssueType);
  const description = clean(rawDescription);

  const baggage = state.baggageRecords.find(
    (bag) => norm(bag.tag) === norm(tag)
  );

  if (!baggage) {
    return toast(
      'Baggage not found',
      'Select an existing baggage record.',
      true
    );
  }

  if (baggage.issueType) {
    return toast(
      'Issue already reported',
      'This demo allows one issue report per baggage record.',
      true
    );
  }

  if (!BAGGAGE_ISSUE_TYPES.includes(issueType)) {
    return toast('Invalid issue type', 'Select a valid issue type.', true);
  }

  if (description.length < 3 || description.length > 200) {
    return toast(
      'Invalid issue description',
      'Enter a description of 3–200 characters.',
      true
    );
  }

  try {
    await baggageApiPost('/baggage/report-issue', {
      tag: tag,
      issueType: issueType,
      issueDescription: description
    });

    if (onSuccess) onSuccess();

    toast(
      'Baggage issue reported',
      `${tag}: ${issueType} issue recorded.`
    );

  } catch (error) {
    toast('Issue report failed', error.message, true);
  }
}


async function updateBaggageIssueStatus(tag, nextStatus) {
  const baggage = state.baggageRecords.find(
    (bag) => norm(bag.tag) === norm(tag)
  );

  if (!baggage || !baggage.issueType) {
    return toast(
      'Issue not found',
      `No reported issue exists for ${tag}.`,
      true
    );
  }

  if (!BAGGAGE_ISSUE_STATUSES.includes(nextStatus)) {
    return toast('Invalid issue status', 'Choose a valid issue status.', true);
  }

  if (baggage.issueStatus === 'Resolved') {
    return toast(
      'Issue already resolved',
      'Resolved baggage issues cannot be changed.',
      true
    );
  }

  if (baggage.issueStatus === nextStatus) return;

  const allowedNextStatuses =
    baggage.issueStatus === 'Reported'
      ? ['Under Review', 'Resolved']
      : baggage.issueStatus === 'Under Review'
        ? ['Resolved']
        : [];

  if (!allowedNextStatuses.includes(nextStatus)) {
    return toast(
      'Invalid status transition',
      'Issues can move from Reported to Under Review or Resolved, and from Under Review to Resolved.',
      true
    );
  }

  try {
    await baggageApiPost('/baggage/update-issue', {
      tag: baggage.tag,
      newStatus: nextStatus
    });

    toast(
      'Issue status updated',
      `${baggage.tag} issue is now ${nextStatus}.`
    );

  } catch (error) {
    toast('Issue update failed', error.message, true);
  }
}

function reportBaggageIssue(event) {
  event.preventDefault();
  reportBaggageIssueRecord($('#baggageIssueTag').value, $('#baggageIssueType').value, $('#baggageIssueDescription').value, () => $('#baggageIssueForm').reset());
}

function renderBaggage() {
  const list = $('#baggageList');
  if (!list) return;

  const bags = state.baggageRecords;

  const queuedBags = state.baggageQueue
    .map((tag) =>
      bags.find((bag) => norm(bag.tag) === norm(tag))
    )
    .filter((bag) => bag && bag.status === 'Registered');

  $('#baggageTotal').textContent = String(bags.length);
  $('#baggageQueued').textContent = String(queuedBags.length);
  $('#baggageLoaded').textContent = String(
    bags.filter((bag) => bag.status === 'Loaded').length
  );
  $('#baggageIssues').textContent = String(
    bags.filter(
      (bag) => bag.issueType && bag.issueStatus !== 'Resolved'
    ).length
  );
  $('#baggageCount').textContent =
    `${bags.length} bag${bags.length === 1 ? '' : 's'}`;

  $('#baggageProcessNext').disabled = queuedBags.length === 0;

  const queueHost = $('#baggageQueueList');

  queueHost.innerHTML = queuedBags.length
    ? queuedBags.map((bag, index) => `
        <div class="baggage-queue-item">
          <span class="baggage-queue-number">${index + 1}</span>
          <div class="baggage-queue-copy">
            <strong>${esc(bag.tag)} · ${esc(bag.description)}</strong>
            <small>${esc(bag.passengerName)} · Flight ${esc(bag.flightNumber)} · ${Number(bag.weightKg).toFixed(1)} kg</small>
          </div>
          ${statusBadge(bag.status)}
        </div>
      `).join('')
    : emptyState(
        'Handling queue is empty',
        'Register baggage to add it to the FIFO queue.'
      );

  const searchTerm = norm($('#baggageSearch').value);
  const statusFilter = $('#baggageStatusFilter').value;

  const filteredBags = bags.filter((bag) => {
    const searchableFields = [
      bag.tag,
      bag.passengerId,
      bag.passengerName,
      bag.flightNumber,
      bag.description,
      bag.status,
      bag.issueType
    ];

    const matchesSearch = searchableFields.some(
      (value) => norm(value).includes(searchTerm)
    );

    const matchesStatus =
      statusFilter === 'all' || bag.status === statusFilter;

    return matchesSearch && matchesStatus;
  });

  list.innerHTML = filteredBags.length
    ? filteredBags.map((bag) => {
        let actions = '';

        if (bag.status === 'Loaded') {
          actions = `
            <button class="button button-outline"
              type="button"
              data-action="baggage-arrived"
              data-id="${esc(bag.tag)}">
              Mark arrived
            </button>`;
        } else if (bag.status === 'Arrived') {
          actions = `
            <button class="button button-primary"
              type="button"
              data-action="baggage-collected"
              data-id="${esc(bag.tag)}">
              Collect baggage
            </button>`;
        } else if (bag.status === 'On Hold') {
          actions = `
            <button class="button button-outline"
              type="button"
              data-action="baggage-requeue"
              data-id="${esc(bag.tag)}">
              Requeue baggage
            </button>`;
        }

        const issueMarkup = bag.issueType
          ? `
            <div class="baggage-card-issue ${bag.issueStatus === 'Resolved' ? 'resolved' : ''}">
              <strong>${esc(bag.issueType)} issue · ${esc(bag.issueStatus)}</strong>
              <div>${esc(bag.issueDescription)}</div>
            </div>`
          : '';

        return `
          <article class="baggage-card status-${norm(bag.status).replace(/\s+/g, '-')}">
            <div class="baggage-card-head">
              <div class="baggage-card-title">
                <span class="eyebrow">${esc(bag.tag)} · ${esc(formatDate(bag.registeredAt))}</span>
                <h3>${esc(bag.description)}</h3>
                <p>${esc(bag.passengerName)} · ${esc(bag.passengerId)}</p>
              </div>
              <div>${statusBadge(bag.status)}</div>
            </div>

            <div class="baggage-card-details">
              <div><strong>Flight:</strong> ${esc(bag.flightNumber)}</div>
              <div><strong>Seat:</strong> ${esc(bag.seatNumber || 'Not available')}</div>
              <div><strong>Weight:</strong> ${Number(bag.weightKg).toFixed(1)} kg</div>
              <div><strong>Status:</strong> ${esc(bag.status)}</div>
            </div>

            ${issueMarkup}

            ${actions ? `<div class="baggage-card-actions">${actions}</div>` : ''}
          </article>`;
      }).join('')
    : emptyState(
        bags.length ? 'No matching baggage' : 'No baggage registered yet',
        bags.length
          ? 'Try another search term or status filter.'
          : 'Use the registration form above to register the first bag.'
      );

  const issueHost = $('#baggageIssueList');

  const issueBags = bags.filter((bag) => bag.issueType);

  issueHost.innerHTML = issueBags.length
    ? issueBags.map((bag) => {
        const options = (
          bag.issueStatus === 'Reported'
            ? ['Reported', 'Under Review', 'Resolved']
            : bag.issueStatus === 'Under Review'
              ? ['Under Review', 'Resolved']
              : ['Resolved']
        ).map((status) => `
          <option value="${esc(status)}" ${bag.issueStatus === status ? 'selected' : ''}>
            ${esc(status)}
          </option>
        `).join('');

        return `
          <article class="baggage-issue-row">
            <div class="baggage-issue-copy">
              <span class="eyebrow">${esc(bag.tag)} · ${esc(bag.passengerName)}</span>
              <h3>${esc(bag.issueType)} baggage issue</h3>
              <p><strong>Flight:</strong> ${esc(bag.flightNumber)}</p>
              <p>${esc(bag.issueDescription)}</p>
            </div>

            <label class="baggage-issue-status">
              Issue status
              <select
                data-baggage-issue-status
                data-tag="${esc(bag.tag)}"
                ${bag.issueStatus === 'Resolved' ? 'disabled' : ''}>
                ${options}
              </select>
            </label>
          </article>`;
      }).join('')
    : emptyState(
        'No baggage issues reported',
        'Reported baggage issues will appear here.'
      );
}

function fillAllOptions() {
  renderFlightOptions(); 
  renderQueueOptions();
  renderGroupOptions(); 
  renderEmergencyOptions();
  renderBaggageOptions();
  const currentAsPassenger = $('#asPax').value;
  fillSelect('asPax', state.passengers.map((p) => ({ value: p.passengerId, label: `${p.passengerId} · ${p.name}` })), 'Select a passenger', currentAsPassenger);
  if (!$('#asPax').value && state.passengers.length) $('#asPax').value = state.passengers[0].passengerId;
}
function renderAll() {
  updateClock(); renderNav(); setHeading(); fillAllOptions();
  renderDashboard(); renderFlights(); renderPassengers(); renderQueues(); renderPriority(); renderGroupTravel(); renderSuggestions(); renderNavigator(); renderAssist(); renderEmergencyRegister(); renderBaggage(); renderNotifications();
  $('#undoButton').disabled = state.deletedStack.length === 0;
  if (typeof window.onStateRendered === 'function') window.onStateRendered();
}

async function resetDemo() {
  if (!(await askConfirm('This will remove changes made in this browser.', { title: 'Reset demo data?', confirmLabel: 'Reset demo' }))) return;
  state = demoState(); activeRoutePath = []; activeRouteWeight = null; saveState();
  $('#flightForm').reset(); $('#passengerForm').reset(); $('#priorityForm').reset(); $('#groupCreateForm').reset(); $('#groupAddForm').reset(); $('#emergencyForm').reset();$('#baggageForm').reset();
$('#baggageIssueForm').reset();
  $('#fSearch').value = ''; $('#pSearch').value = ''; $('#noteSearch').value = '';
  $('#pStatusFilter').value = 'all'; $('#noteFilter').value = 'all'; $('#fSort').value = 'dep';
  renderAll(); toast('Demo reset', 'The starting AeroStruct sample records have been restored.');
}

// Global delegated click handling keeps untrusted input out of inline JavaScript.
document.addEventListener('click', (event) => {
  const pageButton = event.target.closest('[data-page]');
  if (pageButton) { showPage(pageButton.dataset.page); return; }
  const button = event.target.closest('[data-action]');
  if (button) {
    const { action, id, status } = button.dataset;
    if (action === 'edit-flight') updateFlightDetails(id);
    else if (action === 'set-flight-status') setFlightStatus(id, status);
    else if (action === 'delete-flight') deleteFlight(id);
    else if (action === 'edit-passenger') updatePassengerDetails(id);
    else if (action === 'delete-passenger') deletePassenger(id);
    else if (action === 'mark-read') markNotificationRead(id);
    else if (action === 'focus-flight-form') { showPage('flights'); $('#fNo').focus(); }
    else if (action === 'focus-passenger-form') { showPage('passengers'); $('#pId').focus(); }
    else if (action === 'emergency-route') findEmergencyRoute(id);
    else if (action === 'baggage-arrived') {markBaggageArrived(id);}
    else if (action === 'baggage-collected') {markBaggageCollected(id);}
    else if (action === 'baggage-requeue') {requeueHeldBaggage(id);}
    return;
  }
  const node = event.target.closest('[data-node]');
  if (node) { setNavigatorDestination(node.dataset.node); }
});
document.addEventListener('keydown', (event) => {
  const node = event.target.closest('[data-node]');
  if (node && (event.key === 'Enter' || event.key === ' ')) { event.preventDefault(); setNavigatorDestination(node.dataset.node); }
});

$('#flightForm').addEventListener('submit', addFlight);
$('#passengerForm').addEventListener('submit', addPassenger);
$('#priorityForm').addEventListener('submit', addPriority);
$('#groupCreateForm').addEventListener('submit', createTravelGroup);
$('#groupAddForm').addEventListener('submit', addPassengerToTravelGroup);
$('#emergencyForm').addEventListener('submit', registerEmergencyCase);
$('#emPrimaryPassenger').addEventListener('change', renderEmergencyOptions);
$('#emergencySearch').addEventListener('input', renderEmergencyRegister);
$('#emergencyStatusFilter').addEventListener('change', renderEmergencyRegister);
$('#emFindHospitals').addEventListener('click', () => openNearbyHospitalMap());
$('#emHelpType').addEventListener('change', () => renderEmergencyGuidance());
$('#baggageForm').addEventListener('submit', registerBaggage);
$('#baggageIssueForm').addEventListener('submit', reportBaggageIssue);
$('#baggageProcessNext').addEventListener('click',processNextBaggage);
$('#baggageSearch').addEventListener('input', renderBaggage);
$('#baggageStatusFilter').addEventListener('change',renderBaggage);
$('#baggagePassenger').addEventListener('change', () => {
  const passenger = getPassenger($('#baggagePassenger').value);
  if (passenger) {
    $('#baggageFlight').value = passenger.flightNumber || '';
  }
});
document.addEventListener('change', (event) => {
  const flightStatusSelect = event.target.closest('[data-flight-status-select]');
  if (flightStatusSelect) {
    flightStatusSelect.disabled = true;
    Promise.resolve(setFlightStatus(flightStatusSelect.dataset.id, flightStatusSelect.value)).finally(() => renderFlights());
    return;
  }
  const statusSelect = event.target.closest('[data-emergency-status]');
  if (statusSelect) updateEmergencyStatus(statusSelect.dataset.caseId, statusSelect.value);
  const baggageIssueSelect = event.target.closest('[data-baggage-issue-status]');
  if (baggageIssueSelect) {updateBaggageIssueStatus(baggageIssueSelect.dataset.tag,baggageIssueSelect.value);}
});
$('#fSearch').addEventListener('input', renderFlights);
$('#fSort').addEventListener('change', renderFlights);
$('#pSearch').addEventListener('input', renderPassengers);
$('#pStatusFilter').addEventListener('change', renderPassengers);
$('#noteSearch').addEventListener('input', renderNotifications);
$('#noteFilter').addEventListener('change', renderNotifications);
$('#ciJoin').addEventListener('click', () => enqueue('checkinQ', 'ciSel'));
$('#scClear').addEventListener('click', markSecurityCleared);
$('#scSel').addEventListener('change', () => { const passenger = getPassenger($('#scSel').value); const flight = passenger ? getFlight(passenger.flightNumber) : null; $('#scGate').value = flight?.gate || ''; });
$('#ciProcess').addEventListener('click', () => dequeue('checkinQ'));
$('#boProcess').addEventListener('click', () => dequeue('boardQ'));
$('#prProcess').addEventListener('click', servePriority);
$('#historyPassengerSearch').addEventListener(
  'input',
  renderAssistanceHistory
);

$('#refreshAssistanceHistory').addEventListener(
  'click',
  loadAssistanceHistoryFromApi
);
$('#findSuggestions').addEventListener('click', renderSuggestions);
$('#suggestFlight').addEventListener('change', renderSuggestions);
$('#connectionRiskForm').addEventListener('submit', checkConnectionRisk);
$('#crPassenger').addEventListener('change', () => { $('#connectionRiskResult').innerHTML = ''; syncPassengerArrivalFlight(); });
$('#crArrFlight').addEventListener('change', () => { $('#connectionRiskResult').innerHTML = ''; updateConnectionTimeFromFlight('crArrFlight', 'crArrivalTime', 'arrivalTime'); });
$('#crNextFlight').addEventListener('change', () => { $('#connectionRiskResult').innerHTML = ''; updateConnectionTimeFromFlight('crNextFlight', 'crNextDeparture', 'departureTime'); });
['crArrivalTime', 'crTransfer', 'crNextDeparture', 'crMinBuffer'].forEach((id) => document.getElementById(id).addEventListener('input', () => { $('#connectionRiskResult').innerHTML = ''; }));
$('#asPax').addEventListener('change', renderAssist);
$('#asLoc').addEventListener('change', renderAssist);
$('#navAirport').addEventListener('change', async () => {
  if ($('#navAirport').value) {
    await loadAirportGraphFromApi($('#navAirport').value);
  }
});

$('#navFrom').addEventListener('change', () => {
  cancelNavRun('map');
  activeRoutePath = [];
  activeRouteWeight = null;
  activeRouteMinutes = null;
  renderNavigator();
});
$('#navTo').addEventListener('change', findRoute);
$('#navFind').addEventListener('click', findRoute);
$('#navStart').addEventListener('click', async () => {
  if (!activeRoutePath.length) {
    await findRoute();
  }

  if (activeRoutePath.length) {
    runNavigation('map', 'navOut', 'navStart', activeRoutePath);
  }
});
$('#navMedical').addEventListener('click', () => {
  const medical = getAirportLocationByType('Medical');

  if (medical) {
    setNavigatorDestination(medical.name);
  } else {
    toast(
      'Medical facility unavailable',
      'No medical facility is mapped at this airport.',
      true
    );
  }
});
$('#navBfs').addEventListener('click', () => traverseGraph('bfs'));
$('#navDfs').addEventListener('click', () => traverseGraph('dfs'));
$('#markAllRead').addEventListener('click', markAllRead);
$('#undoButton').addEventListener('click', undoDelete);
$('#resetButton').addEventListener('click', resetDemo);
$('#flightForm').addEventListener('reset', () => window.setTimeout(() => { renderFlightOptions(); }, 0));
$('#passengerForm').addEventListener('reset', () => window.setTimeout(() => { renderFlightOptions(); }, 0));

// Native browser constraint validation plus a focused, accessible toast for invalid forms.
['fNo', 'pId', 'pPhone', 'pSeat', 'fGate', 'scGate'].forEach((id) => {
  const input = document.getElementById(id);
  if (!input) return;
  input.addEventListener('input', () => setCustomError(input, ''));
});

hydrateIcons();
renderAll();
renderEmergencyGuidance();
showPage('dashboard');

// Only public information is loaded before sign-in.
async function synchronizeDatabase() {
  await loadFlightsFromApi();
  await loadAirportDirectoryFromApi();
}

// Load data permitted for the authenticated role. Backend filtering remains authoritative.
async function synchronizeAeroStructForRole(role) {
  if (role !== 'admin' && role !== 'user') {
    throw new Error('A valid authenticated role is required.');
  }

  window.aeroStructSyncing = true;
  try {
    // Clear demo or previously loaded data before loading the caller's permitted records.
    state.passengers = [];
    state.checkinQ = [];
    state.boardQ = [];
    state.priorityQ = [];
    state.processedPriorityIds = [];
    state.groups = [];
    state.baggageRecords = [];
    state.baggageQueue = [];
    state.emergencyCases = [];
    state.notifications = [];
    state.deletedStack = [];
    assistanceHistoryRecords = [];

    await loadFlightsFromApi();
    await loadPassengersFromApi();
    await loadBaggageFromApi();
    await loadCheckInQueueFromApi();
    await loadPriorityQueueFromApi();
    await loadAssistanceHistoryFromApi();
    await loadGroupsFromApi();
    await loadEmergencyCasesFromApi();
    await loadNotificationsFromApi();

    if (role === 'admin') {
      await loadBoardingQueueFromApi();
    } else {
      state.boardQ = [];
    }

    await loadAirportDirectoryFromApi();
    if (role === 'admin') renderAll();
  } finally {
    window.aeroStructSyncing = false;
  }
}
window.synchronizeAeroStructForRole = synchronizeAeroStructForRole;

// Create a passenger portal account from the authenticated Operations Console.
const passengerAccountForm = $('#passengerAccountForm');
if (passengerAccountForm) {
  passengerAccountForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    const passengerId = String($('#paPassengerId').value || '').trim().toUpperCase();
    const username = String($('#paUsername').value || '').trim().toLowerCase();
    const password = String($('#paPassword').value || '');

    if (!passengerId || !username || password.length < 12) {
      toast('Account details required', 'Enter a Passenger ID, username and password of at least 12 characters.', true);
      return;
    }

    try {
      const response = await apiFetch(`${API_BASE_URL}/auth/passengers/provision`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' },
        body: new URLSearchParams({ passengerId, username, password })
      });
      const result = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(result.message || `HTTP ${response.status}`);
      passengerAccountForm.reset();
      toast('Passenger account created', `Login account ${username} is ready for Passenger ${passengerId}.`);
    } catch (error) {
      toast('Could not create passenger account', error.message || 'Please check the account details and try again.', true);
    }
  });
}

synchronizeDatabase();

window.setInterval(updateClock, 1000);

// Live sync: changes made in another tab (for example the admin console while a passenger tab is open) appear immediately.
window.addEventListener('storage', (event) => {
  if (typeof window.getAeroStructToken === 'function' && window.getAeroStructToken()) return;
  if (event.key !== STORAGE_KEY && event.key !== null) return;
  state = loadState();
  window.syncedFromStorage = true;
  try { renderAll(); } finally { window.syncedFromStorage = false; }
});

// Update an emergency case status through the API (Open → In Progress → Resolved).
async function updateEmergencyStatus(caseId, nextStatus) {
  const incident = state.emergencyCases.find((item) => norm(item.caseId) === norm(caseId));
  if (!incident) return toast('Case not found', `No emergency case matches ${caseId}.`, true);
  if (incident.status === nextStatus) return;

  try {
    const response = await apiFetch(`${API_BASE_URL}/emergencies/status`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' },
      body: new URLSearchParams({ caseId: incident.caseId, status: nextStatus }).toString()
    });
    const data = await response.json().catch(() => ({}));
    if (!response.ok || data.status === 'error') {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }
    const saved = data.incident && data.incident.caseId ? data.incident : { ...incident, status: nextStatus };
    const index = state.emergencyCases.findIndex((item) => norm(item.caseId) === norm(saved.caseId));
    if (index >= 0) state.emergencyCases[index] = { ...state.emergencyCases[index], ...saved };
    saveState();
    renderEmergencyRegister();
    toast('Case updated', `${incident.caseId} is now ${nextStatus}.`);
  } catch (error) {
    renderEmergencyRegister();
    toast('Case status not saved', error.message || 'Check your API server connection.', true);
  }
}

// Expose only common actions for progressive enhancement and browser console testing.
window.AeroStructDemo = {
  getState: () => JSON.parse(JSON.stringify(state)),
  reset: resetDemo,
  showPage,
  enqueue,
  dequeue,
  setFlightStatus,
  markSecurityCleared,
  createTravelGroup,
  addPassengerToTravelGroup,
  registerEmergencyCase,
  updateEmergencyStatus,
  findEmergencyRoute,
  findRoute,
  traverseGraph,
  registerBaggage,
  processNextBaggage,
  markBaggageArrived,
  markBaggageCollected,
  requeueHeldBaggage,
  reportBaggageIssue,
  updateBaggageIssueStatus,
};
