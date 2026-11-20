'use strict';

/* ======================================================================
   AeroStruct v2: role gate + passenger portal.
   Every write goes through the same validated functions defined in app.js
   (enqueueById, registerBaggageRecord, addPriorityRequest, ...), so the
   passenger side follows exactly the same rules as the operations console.
   ====================================================================== */

const SESSION_KEY = 'aerostruct-session-v1';
// Dedicated entry pages enforce which account type may sign in here.
// index.html remains the combined landing/demo entry point.
const PORTAL_MODE = document.body.dataset.portalMode || 'combined';
const REQUIRED_ROLE = PORTAL_MODE === 'admin' ? 'admin' : PORTAL_MODE === 'passenger' ? 'user' : null;
const U_NAV = [
  { id: 'trip', label: 'My Trip', icon: 'ticket' },
  { id: 'baggage', label: 'My Baggage', icon: 'bag' },
  { id: 'flights', label: 'Flights & Options', icon: 'plane' },
  { id: 'assist', label: 'Assistance', icon: 'lifebuoy' },
  { id: 'navigator', label: 'Airport Navigator', icon: 'compass' },
  { id: 'emergency', label: 'Emergency Help', icon: 'alert' },
  { id: 'notes', label: 'Notifications', icon: 'bell' }
];
const U_GROUPS = [
  { label: 'My journey', items: ['trip', 'baggage', 'flights'] },
  { label: 'Services', items: ['assist', 'navigator'] },
  { label: 'Safety & updates', items: ['emergency', 'notes'] }
];
const JOURNEY_STEPS = ['Registered', 'Checked-In', 'Security-Cleared', 'Boarded'];
const JOURNEY_ICONS = ['user', 'check', 'shield', 'plane'];
const BAG_STEPS = ['Registered', 'Loaded', 'Arrived', 'Collected'];

let session = readSession();
let userPage = 'trip';
let userRoute = { path: [], meters: null };
let userNavOut = '';
let userNavInit = false;
let crInitialised = false;
let lastMyStatus = null;
let lastNoteId = 0;
let gateOpen = false;
let passengerSuggestionRequest = 0;
let passengerAssistRequest = 0;

/* ---------------------------- authenticated session ---------------------------- */
function readSession() {
  try {
    const raw = window.sessionStorage.getItem(SESSION_KEY);
    const parsed = raw ? JSON.parse(raw) : null;
    if (parsed && typeof parsed.token === 'string' && parsed.token.length === 64 &&
        (parsed.role === 'admin' || (parsed.role === 'user' && parsed.passengerId))) {
      return parsed;
    }
  } catch (error) { /* storage unavailable */ }
  return null;
}
function writeSession(next) {
  session = next;
  try {
    if (next) window.sessionStorage.setItem(SESSION_KEY, JSON.stringify(next));
    else window.sessionStorage.removeItem(SESSION_KEY);
  } catch (error) { /* storage unavailable */ }
}
window.getAeroStructToken = () => session && session.token ? session.token : '';
function me() { return session && session.role === 'user' ? getPassenger(session.passengerId) : null; }
function myNotifications() {
  const p = me();
  return p ? state.notifications.filter((n) => norm(n.passengerId) === norm(p.passengerId)) : [];
}
function initials(name) {
  return String(name || '?').split(/\s+/).filter(Boolean).slice(0, 2).map((part) => part[0].toUpperCase()).join('') || '?';
}
function minutesToClock(total) {
  const wrapped = ((total % 1440) + 1440) % 1440;
  return `${String(Math.floor(wrapped / 60)).padStart(2, '0')}:${String(wrapped % 60).padStart(2, '0')}`;
}

function gateLocationFor(flight) {
  const wanted = `gate ${clean(flight && flight.gate)}`;

  const match = NAV_GRAPH.locations.find(
    location => norm(location.name) === norm(wanted)
  );

  if (match) return match.name;

  // Fall back to a gate available at the selected airport.
  const firstGate = NAV_GRAPH.locations.find(
    location => norm(location.type) === 'gate'
  );

  if (firstGate) return firstGate.name;

  const fallback = NAV_GRAPH.locations.find(
    location =>
      norm(location.type).includes('lounge') ||
      norm(location.type).includes('concourse')
  );

  return fallback?.name || NAV_GRAPH.locations[0]?.name || '';
}

/* ---------------------------- authenticated role gate ---------------------------- */
function gateError(message, role = 'user') {
  const holder = role === 'admin' ? $('#adminGateError') : $('#gateError');
  const input = role === 'admin' ? $('#adminUsername') : $('#gateUsername');
  if (!holder || !input) return;
  holder.textContent = message || '';
  holder.hidden = !message;
  input.classList.toggle('invalid', !!message);
  input.setAttribute('aria-invalid', message ? 'true' : 'false');
  if (message) {
    const card = input.closest('.gate-card');
    if (card) { card.classList.remove('shake'); void card.offsetWidth; card.classList.add('shake'); }
    input.focus();
  }
}

async function authenticateWithRole(expectedRole, username, password, errorRole) {
  if (REQUIRED_ROLE && expectedRole !== REQUIRED_ROLE) {
    gateError(REQUIRED_ROLE === 'admin'
      ? 'Use the administrator sign-in page for this account.'
      : 'Use the passenger sign-in page for this account.', errorRole);
    return;
  }
  const cleanUsername = clean(username).toLowerCase();
  if (!cleanUsername || !password) {
    gateError('Enter your username and password.', errorRole);
    return;
  }

  try {
    const response = await window.aeroApiFetch(`${window.AEROSTRUCT_API_BASE_URL}/auth/login`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' },
      body: new URLSearchParams({ username: cleanUsername, password }),
      skipAuth: true
    });
    const result = await response.json().catch(() => ({}));
    if (!response.ok || !result.token || !result.user) {
      throw new Error(result.message || `Sign-in failed (HTTP ${response.status}).`);
    }

    const actualRole = result.user.role === 'passenger' ? 'user' : result.user.role;
    if (actualRole !== expectedRole) {
      await window.aeroApiFetch(`${window.AEROSTRUCT_API_BASE_URL}/auth/logout`, {
        method: 'POST',
        headers: { Authorization: `Bearer ${result.token}` },
        skipAuth: true
      }).catch(() => {});
      throw new Error(expectedRole === 'admin'
        ? 'This account is not an administrator account.'
        : 'This account is not a passenger account.');
    }

    writeSession({
      role: actualRole,
      username: result.user.username,
      passengerId: result.user.passengerId || null,
      token: result.token
    });
    try { localStorage.removeItem('aerostruct-validated-ops-v1'); } catch (error) { /* storage unavailable */ }

    await window.synchronizeAeroStructForRole(actualRole);
    gateError('', errorRole);
    $('#adminPassword').value = '';
    $('#gatePassword').value = '';
    $('#roleGate').classList.add('leaving');
    window.setTimeout(() => {
      userPage = 'trip';
      userRoute = { path: [], meters: null };
      userNavOut = '';
      userNavInit = false;
      crInitialised = false;
      applyRole();
    }, prefersReducedMotion() ? 0 : 300);
  } catch (error) {
    gateError(error.message || 'Could not sign in. Please try again.', errorRole);
  }
}

function submitGate(event) {
  event.preventDefault();
  return authenticateWithRole('user', $('#gateUsername').value, $('#gatePassword').value, 'user');
}
function submitAdminGate(event) {
  event.preventDefault();
  return authenticateWithRole('admin', $('#adminUsername').value, $('#adminPassword').value, 'admin');
}

async function signOut() {
  const current = session;
  if (current && current.token) {
    try {
      await window.aeroApiFetch(`${window.AEROSTRUCT_API_BASE_URL}/auth/logout`, {
        method: 'POST',
        headers: { Authorization: `Bearer ${current.token}` },
        skipAuth: true
      });
    } catch (error) { /* local sign-out still completes if the API is unavailable */ }
  }
  writeSession(null);
  gateOpen = false;
  try { localStorage.removeItem('aerostruct-validated-ops-v1'); } catch (error) { /* storage unavailable */ }
  window.location.reload();
}

async function restoreAuthSession() {
  if (!session || !session.token) {
    writeSession(null);
    gateOpen = !!REQUIRED_ROLE;
    applyRole();
    return;
  }

  try {
    const response = await window.aeroApiFetch(`${window.AEROSTRUCT_API_BASE_URL}/auth/me`);
    const result = await response.json().catch(() => ({}));
    if (!response.ok || !result.user) {
      throw new Error(result.message || 'Session expired. Please sign in again.');
    }
    const actualRole = result.user.role === 'passenger' ? 'user' : result.user.role;
    if (actualRole !== session.role) {
      throw new Error('Account role changed. Please sign in again.');
    }
    if (REQUIRED_ROLE && actualRole !== REQUIRED_ROLE) {
      throw new Error(REQUIRED_ROLE === 'admin'
        ? 'This session belongs to a passenger account. Sign in with an administrator account.'
        : 'This session belongs to an administrator account. Sign in with a passenger account.');
    }
    session = {
      ...session,
      username: result.user.username,
      passengerId: result.user.passengerId || null
    };
    writeSession(session);
    await window.synchronizeAeroStructForRole(actualRole);
    applyRole();
  } catch (error) {
    const rejectedToken = session && session.token;
    if (rejectedToken) {
      await window.aeroApiFetch(`${window.AEROSTRUCT_API_BASE_URL}/auth/logout`, {
        method: 'POST',
        headers: { Authorization: `Bearer ${rejectedToken}` },
        skipAuth: true
      }).catch(() => {});
    }
    writeSession(null);
    gateOpen = !!REQUIRED_ROLE;
    try { localStorage.removeItem('aerostruct-validated-ops-v1'); } catch (storageError) { /* storage unavailable */ }
    applyRole();
    toast('Session ended', error.message || 'Please sign in again.', true);
  }
}

function openGate() { gateOpen = true; applyRole(); }
function closeGate() {
  // Dedicated portal pages must not be able to fall back to a role chooser.
  if (REQUIRED_ROLE) return;
  gateOpen = false;
  applyRole();
}

function applyRole() {
  const role = session ? session.role : null;
  if (role) gateOpen = false;
  const landingVisible = !role && !gateOpen;
  document.body.classList.toggle('is-landing', landingVisible);
  document.body.classList.toggle('is-gate', !role && gateOpen);
  document.body.classList.toggle('is-admin', role === 'admin');
  document.body.classList.toggle('is-user', role === 'user');
  document.body.classList.remove('nav-open');

  const landing = $('#landing');
  landing.hidden = !!role || !!REQUIRED_ROLE;
  if (gateOpen || REQUIRED_ROLE) landing.setAttribute('inert', ''); else landing.removeAttribute('inert');
  const gate = $('#roleGate');
  gate.classList.remove('leaving');
  gate.hidden = !(gateOpen && !role);
  $('#adminApp').hidden = role !== 'admin';
  $('#userApp').hidden = role !== 'user';

  if (role === 'admin') {
    document.title = 'Operations Console · AeroStruct';
    donutAnimated = false;
    Object.keys(countMemory).forEach((key) => { delete countMemory[key]; });
    renderAll();
    showPage(currentPage);
    window.scrollTo(0, 0);
  } else if (role === 'user') {
    const passenger = me();
    if (!passenger) {
      writeSession(null);
      applyRole();
      toast('Session ended', 'Your passenger record is unavailable. Please contact the operations desk.', true);
      return;
    }
    document.title = `${passenger.name} · AeroStruct Passenger Portal`;
    lastMyStatus = passenger.status;
    lastNoteId = Math.max(0, ...myNotifications().map((n) => Number(n.notificationId) || 0));
    renderPortal();
    showUserPage(userPage);
    window.scrollTo(0, 0);
  } else if (gateOpen) {
    document.title = 'Sign in · AeroStruct';
    window.setTimeout(() => {
      const input = $('#adminUsername');
      if (input && window.innerWidth > 960) input.focus({ preventScroll: true });
    }, 60);
  } else {
    document.title = 'AeroStruct · Airport Management System';
    if (typeof window.onLandingShown === 'function') window.onLandingShown();
  }
}

/* ---------------------------- chrome ---------------------------- */
function renderUserNav() {
  const nav = $('#uNav');
  const focusedPage = document.activeElement && nav.contains(document.activeElement) ? document.activeElement.dataset.upage : null;
  const unread = myNotifications().filter((n) => !n.isRead).length;
  const openBagIssues = state.baggageRecords.filter((b) => me() && norm(b.passengerId) === norm(me().passengerId) && b.issueType && b.issueStatus !== 'Resolved').length;
  nav.innerHTML = U_GROUPS.map((group) => `<div class="nav-group"><div class="nav-group-label">${esc(group.label)}</div>${group.items.map((id) => {
    const item = U_NAV.find((entry) => entry.id === id);
    const active = userPage === id;
    const badge = id === 'notes' && unread ? `<span class="nav-count">${unread}</span>` : id === 'baggage' && openBagIssues ? `<span class="nav-count nav-count-soft">${openBagIssues}</span>` : '';
    return `<button type="button" data-upage="${id}" class="${active ? 'active' : ''}" aria-current="${active ? 'page' : 'false'}"><span class="nav-icon" aria-hidden="true">${icon(item.icon, 20)}</span><span class="nav-label">${esc(item.label)}</span>${badge}</button>`;
  }).join('')}</div>`).join('');
  if (focusedPage) nav.querySelector(`[data-upage="${focusedPage}"]`)?.focus({ preventScroll: true });
}
function userHeadingInfo() {
  const passenger = me();
  const first = passenger ? String(passenger.name).split(' ')[0] : '';
  return ({
    trip: { kicker: `${dateLabel()} · Passenger portal`, title: `${greeting()}, ${first}`, description: 'Your boarding pass, live flight status and next steps, all in one place.' },
    baggage: { kicker: 'MY JOURNEY · BAGGAGE', title: 'My baggage', description: 'Register your bags, follow them from loading to collection, and report any problem.' },
    flights: { kicker: 'MY JOURNEY · FLIGHTS', title: 'Flights & options', description: 'Check your flight, compare alternatives on the same route and watch the live departure board.' },
    assist: { kicker: 'SERVICES · AEROASSIST', title: 'Assistance', description: 'Get support suggestions for your assistance type and request help from the priority desk.' },
    navigator: { kicker: 'SERVICES · AIRPORT NAVIGATOR', title: 'Airport Navigator', description: 'Explore the terminal map and find the lowest-walking-time route with Dijkstra’s algorithm.' },
    emergency: { kicker: 'SAFETY · EMERGENCY HELP', title: 'Emergency help', description: 'Find hospitals, read first-aid guidance and alert airport staff.' },
    notes: { kicker: 'UPDATES · NOTIFICATIONS', title: 'Notifications', description: 'Your flight updates and reminders, plus a quick check for tight connections.' }
  })[userPage];
}
function renderUserChrome() {
  const passenger = me();
  if (!passenger) return;
  $('#uUserCard').innerHTML = `<span class="avatar" aria-hidden="true">${esc(initials(passenger.name))}</span><span class="user-card-copy"><strong>${esc(passenger.name)}</strong><small>${esc(passenger.passengerId)} · ${esc(passenger.flightNumber)}</small></span>`;
  renderUserNav();
  const heading = userHeadingInfo();
  $('#uBreadcrumb').textContent = U_NAV.find((item) => item.id === userPage).label;
  $('#uPageHeading').innerHTML = bannerHtml(userPage, `<div class="page-kicker"><span class="kicker-marker"></span>${esc(heading.kicker)}</div><div class="page-title-row"><div><h1 class="page-title">${esc(heading.title)}</h1><p class="page-description">${esc(heading.description)}</p></div></div>`);
  const unread = myNotifications().filter((n) => !n.isRead).length;
  const bell = $('#uBell');
  bell.textContent = String(unread); bell.hidden = unread === 0;
  updateClock();
}
function showUserPage(page) {
  if (!U_NAV.some((item) => item.id === page)) return;
  userPage = page;
  const viewId = `u${page.charAt(0).toUpperCase()}${page.slice(1)}`;
  $$('#userApp .uview').forEach((view) => { view.hidden = view.id !== viewId; });
  document.body.classList.remove('nav-open');
  renderUserChrome();
  window.scrollTo({ top: 0, behavior: 'smooth' });
  const heading = $('#uPageHeading');
  heading.classList.remove('heading-enter'); void heading.offsetWidth; heading.classList.add('heading-enter');
  if (page === 'navigator') renderNavigatorPage();
}

/* ---------------------------- my trip ---------------------------- */
function barcodeSvg(seed) {
  let hash = 2166136261;
  for (const ch of String(seed)) { hash ^= ch.charCodeAt(0); hash = Math.imul(hash, 16777619) >>> 0; }
  let x = 0; const bars = [];
  for (let i = 0; i < 44; i += 1) {
    hash = (Math.imul(hash, 1664525) + 1013904223) >>> 0;
    const width = 1 + (hash % 3);
    if (i % 2 === 0) bars.push(`<rect x="${x}" y="0" width="${width}" height="40"/>`);
    x += width + 1;
  }
  return `<svg class="barcode" viewBox="0 0 ${x} 40" preserveAspectRatio="none" role="img" aria-label="Boarding pass barcode" fill="currentColor">${bars.join('')}</svg>`;
}
function boardingPass(passenger, flight) {
  if (!flight) {
    return `<article class="boarding-pass is-cancelled"><div class="bp-main"><span class="bp-badge">BOARDING PASS</span><h2>Flight record unavailable</h2><p class="muted">Your flight ${esc(passenger.flightNumber)} is no longer on the schedule. Please contact the passenger service desk.</p></div></article>`;
  }
  const unavailable = isUnavailableFlight(flight.status);
  const diff = timeNumber(flight.departureTime) - nowMinutes();
  const countdown = unavailable ? flight.status : diff >= 0 ? formatCountdown(diff) : 'Departure time passed';
  const closes = validClockTime(flight.departureTime) ? minutesToClock(timeNumber(flight.departureTime) - 20) : '—';
  return `<article class="boarding-pass${unavailable ? ' is-cancelled' : ''}${isDelayed(flight.status) ? ' is-delayed' : ''}">
    <div class="bp-main">
      <header class="bp-head"><span class="bp-airline">${icon('plane', 18)} ${esc(flight.airline)}</span><span class="bp-badge">BOARDING PASS</span>${statusBadge(flight.status)}</header>
      <div class="bp-route">
        <div class="bp-city"><strong>${esc(airportCode(flight.source))}</strong><span>${esc(flight.source)}</span><small>Departs ${esc(flight.departureTime)}</small></div>
        <div class="bp-path" aria-hidden="true"><span class="bp-line"></span><span class="bp-plane">${icon('plane', 22)}</span><small>${esc(formatDuration(flightDurationMinutes(flight)))}</small></div>
        <div class="bp-city bp-city-end"><strong>${esc(airportCode(flight.destination))}</strong><span>${esc(flight.destination)}</span><small>Arrives ${esc(flight.arrivalTime)}</small></div>
      </div>
      <dl class="bp-meta">
        <div><dt>Passenger</dt><dd>${esc(passenger.name)}</dd></div>
        <div><dt>Flight</dt><dd>${esc(flight.flightNumber)}</dd></div>
        <div><dt>Gate</dt><dd>${esc(flight.gate)}</dd></div>
        <div><dt>Seat</dt><dd>${esc(passenger.seatNumber)}</dd></div>
        <div><dt>Boarding closes</dt><dd>${esc(closes)}</dd></div>
        <div><dt>Fare</dt><dd>${esc(formatFare(flight.fare))}</dd></div>
      </dl>
    </div>
    <div class="bp-stub">
      <div class="bp-countdown"><small>${unavailable ? 'Flight status' : 'Departure'}</small><strong>${esc(countdown)}</strong></div>
      ${barcodeSvg(passenger.passengerId + flight.flightNumber)}
      <small class="bp-id">${esc(passenger.passengerId)} · Seat ${esc(passenger.seatNumber)}</small>
    </div>
    ${unavailable ? `<div class="bp-ribbon">${esc(flight.status).toUpperCase()}</div>` : ''}
  </article>`;
}
function journeyPanel(passenger, flight) {
  const index = Math.max(0, JOURNEY_STEPS.indexOf(passenger.status));
  const stepper = `<ol class="stepper" style="--p:${index / (JOURNEY_STEPS.length - 1)}">${JOURNEY_STEPS.map((label, i) => `<li class="${i < index ? 'done' : i === index ? 'current' : ''}"><span class="step-dot">${icon(i < index ? 'check' : JOURNEY_ICONS[i], 16)}</span><span class="step-label">${esc(label)}</span></li>`).join('')}</ol>`;
  const queuePosition = state.checkinQ.findIndex((id) => norm(id) === norm(passenger.passengerId));
  const boardIndex = state.boardQ.findIndex((item) => norm(item.passengerId) === norm(passenger.passengerId));
  let tone = 'info'; let title = ''; let text = ''; let actions = '';
  if (!flight) {
    tone = 'bad'; title = 'Flight record unavailable'; text = 'Please contact the passenger service desk.';
  } else if (isUnavailableFlight(flight.status)) {
    tone = 'bad'; title = `Flight ${flight.flightNumber} is ${flight.status.toLowerCase()}`;
    text = 'Please contact the passenger service desk. You can compare other flights on your route.';
    actions = `<button class="button button-primary" type="button" data-upage="flights">See alternatives</button>`;
  } else if (passenger.status === 'Registered') {
    if (queuePosition >= 0) {
      title = `You're #${queuePosition + 1} in the check-in queue`;
      text = `Approx. ${(queuePosition + 1) * 4} minutes at 4 minutes per passenger (demo estimate).`;
      actions = `<button class="button button-outline" type="button" data-action="u-route" data-from="Main Entrance" data-to="Check-In Counter">Show route to check-in</button>`;
    } else {
      title = 'Ready to check in';
      text = 'Join the check-in queue and a counter agent will process you in order.';
      actions = `<button class="button button-primary" type="button" data-action="u-join-checkin">Join check-in queue</button><button class="button button-outline" type="button" data-action="u-route" data-from="Main Entrance" data-to="Check-In Counter">Route to check-in</button>`;
    }
  } else if (passenger.status === 'Checked-In') {
    tone = 'ok'; title = 'Checked in — head to security';
    text = 'Proceed to the Security Check. An officer will clear you and add you to the boarding queue.';
    actions = `<button class="button button-primary" type="button" data-action="u-route" data-from="Check-In Counter" data-to="Security Check">Route to security</button>`;
  } else if (passenger.status === 'Security-Cleared') {
    const gate = (state.boardQ.find((item) => norm(item.passengerId) === norm(passenger.passengerId)) || {}).gate || flight.gate;
    tone = 'ok'; title = boardIndex >= 0 ? `You're #${boardIndex + 1} in the boarding queue` : 'Cleared for boarding';
    text = `Proceed to gate ${gate}.${boardIndex >= 0 ? ` Approx. ${(boardIndex + 1) * 2} minutes at 2 minutes per passenger (demo estimate).` : ''}`;
    actions = `<button class="button button-primary" type="button" data-action="u-route" data-from="Security Check" data-to="${esc(gateLocationFor(flight))}">Route to my gate</button>`;
  } else if (passenger.status === 'Boarded') {
    tone = 'ok'; title = 'You are on board'; text = `Have a pleasant flight to ${flight.destination}.`;
  }
  const delay = flight && isDelayed(flight.status) ? `<div class="u-callout tone-warn">${icon('clock', 18)}<span><strong>Flight delayed.</strong> Current scheduled departure is ${esc(flight.departureTime)}. Please check the live board for updates.</span></div>` : '';
  return `${stepper}${delay}<div class="u-status tone-${tone}"><div><h3>${esc(title)}</h3><p>${esc(text)}</p></div><div class="u-status-actions">${actions}</div></div>`;
}
function sideCards(passenger, flight) {
  const group = state.groups.find((g) => g.members.some((memberId) => norm(memberId) === norm(passenger.passengerId)));
  const details = `<section class="panel"><div class="panel-heading"><div><span class="eyebrow">YOUR DETAILS</span><h2>Profile</h2></div></div><dl class="kv-list"><div><dt>Passenger ID</dt><dd>${esc(passenger.passengerId)}</dd></div><div><dt>Age</dt><dd>${esc(passenger.age)}</dd></div><div><dt>Phone</dt><dd>${esc(passenger.phoneNumber)}</dd></div><div><dt>Seat</dt><dd>${esc(passenger.seatNumber)}</dd></div><div><dt>Assistance</dt><dd>${esc(passenger.assistanceType || 'None')}</dd></div></dl></section>`;
  let groupCard = '';
  if (group) {
    const members = group.members.map((memberId) => getPassenger(memberId)).filter(Boolean);
    groupCard = `<section class="panel"><div class="panel-heading"><div><span class="eyebrow">FAMILY & GROUP TRAVEL</span><h2>${esc(group.groupName)}</h2></div><span class="count-pill">${members.length} member${members.length === 1 ? '' : 's'}</span></div><div class="panel-body">${members.map((member) => `<div class="group-member"><span class="group-member-id">${esc(member.passengerId)}</span><span class="group-member-copy"><strong>${esc(member.name)}${norm(member.passengerId) === norm(passenger.passengerId) ? ' (you)' : ''}</strong><small>${esc(member.flightNumber)} · Seat ${esc(member.seatNumber)}</small></span>${statusBadge(member.status)}</div>`).join('')}</div></section>`;
  }
  const latest = myNotifications().slice(0, 3);
  const updates = `<section class="panel"><div class="panel-heading"><div><span class="eyebrow">LATEST UPDATES</span><h2>Notifications</h2></div><button class="button button-outline" type="button" data-upage="notes">View all</button></div><div class="notification-list">${latest.length ? latest.map((n) => `<div class="notification-item ${n.isRead ? '' : 'unread'}"><span class="notification-icon">${notificationIcon(n.type)}</span><div class="notification-copy"><strong>${esc(n.type)}</strong><p>${esc(n.message)}</p><small>${esc(formatDate(n.createdAt))}</small></div></div>`).join('') : emptyState('No notifications yet', 'Updates about your flight will appear here.')}</div></section>`;
  return `${details}${groupCard}${updates}`;
}
function renderTrip() {
  const passenger = me();
  if (!passenger) return;
  const flight = getFlight(passenger.flightNumber);
  $('#uPass').innerHTML = boardingPass(passenger, flight);
  $('#uJourneyBadge').innerHTML = statusBadge(passenger.status);
  $('#uJourney').innerHTML = journeyPanel(passenger, flight);
  $('#uSide').innerHTML = sideCards(passenger, flight);
}

/* ---------------------------- baggage ---------------------------- */
function renderBaggagePage() {
  const passenger = me();
  if (!passenger) return;
  const flight = getFlight(passenger.flightNumber);
  const bags = state.baggageRecords.filter((bag) => norm(bag.passengerId) === norm(passenger.passengerId));
  const summary = [
    { label: 'MY BAGS', value: bags.length, note: 'Registered baggage' },
    { label: 'IN QUEUE', value: bags.filter((b) => b.status === 'Registered').length, note: 'Awaiting handling' },
    { label: 'IN TRANSIT', value: bags.filter((b) => b.status === 'Loaded' || b.status === 'Arrived').length, note: 'Loaded or arrived' },
    { label: 'OPEN ISSUES', value: bags.filter((b) => b.issueType && b.issueStatus !== 'Resolved').length, note: 'Requiring attention' }
  ];
  $('#uBagSummary').innerHTML = summary.map((item) => `<div class="panel baggage-summary-card"><span class="eyebrow">${item.label}</span><strong>${item.value}</strong><span>${item.note}</span></div>`).join('');
  const eligible = ['checked-in', 'security-cleared'].includes(norm(passenger.status));
  $('#uBagHint').textContent = eligible ? 'Baggage is registered against your own flight.' : `Baggage can be registered once you are Checked-In or Security-Cleared. Your status: ${passenger.status}.`;
  const flightLabel = flight ? `${flight.flightNumber} · ${flight.source} → ${flight.destination}` : passenger.flightNumber;
  const flightInput = $('#uBagFlight');
  flightInput.defaultValue = flightLabel; flightInput.value = flightLabel;
  $('#uBagCount').textContent = `${bags.length} bag${bags.length === 1 ? '' : 's'}`;
  const taggable = bags.filter((bag) => !bag.issueType).map((bag) => ({ value: bag.tag, label: `${bag.tag} · ${bag.description}` }));
  fillSelect('uBagIssueTag', taggable, taggable.length ? 'Select baggage tag' : 'No bags without a reported issue');
  $('#uBagList').innerHTML = bags.length ? bags.map((bag) => {
    const stepIndex = BAG_STEPS.indexOf(bag.status);
    const tracker = bag.status === 'On Hold'
      ? `<div class="u-callout tone-warn">${icon('alert', 18)}<span><strong>On hold.</strong> Your flight is currently unavailable. Staff will requeue this bag once the flight can operate.</span></div>`
      : `<ol class="mini-steps" style="--p:${Math.max(0, stepIndex) / (BAG_STEPS.length - 1)}">${BAG_STEPS.map((label, i) => `<li class="${i < stepIndex ? 'done' : i === stepIndex ? 'current' : ''}"><span class="step-dot">${i <= stepIndex ? icon('check', 12) : ''}</span><span>${esc(label)}</span></li>`).join('')}</ol>`;
    const issue = bag.issueType ? `<div class="baggage-card-issue ${bag.issueStatus === 'Resolved' ? 'resolved' : ''}"><strong>${esc(bag.issueType)} issue · ${esc(bag.issueStatus)}</strong><div>${esc(bag.issueDescription)}</div></div>` : '';
    const actions = bag.status === 'Arrived' ? `<div class="baggage-card-actions"><button class="button button-primary" type="button" data-action="baggage-collected" data-id="${esc(bag.tag)}">Collect baggage</button></div>` : '';
    return `<article class="baggage-card status-${norm(bag.status).replace(/\s+/g, '-')}"><div class="baggage-card-head"><div class="baggage-card-title"><span class="eyebrow">${esc(bag.tag)} · ${esc(formatDate(bag.registeredAt))}</span><h3>${esc(bag.description)}</h3><p>${Number(bag.weightKg).toFixed(1)} kg · Flight ${esc(bag.flightNumber)}</p></div><div>${statusBadge(bag.status)}</div></div><div class="baggage-card-tracker">${tracker}</div>${issue}${actions}</article>`;
  }).join('') : emptyState('No baggage registered yet', 'Use the form to register your first bag.');
}


async function renderPassengerSuggestions(flightNumber) {
  const host = $('#uSuggestOut');
  const requestId = ++passengerSuggestionRequest;
  const selected = getFlight(flightNumber);

  if (!selected) {
    host.innerHTML = emptyState(
      'Flight unavailable',
      'Your assigned flight could not be found.'
    );
    return;
  }

  host.innerHTML = '<p class="muted">Finding the best flight alternatives...</p>';

  try {
    const response = await window.aeroApiFetch(
      `${API_BASE_URL}/flights/suggestions?flightNumber=${encodeURIComponent(flightNumber)}`
    );

    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.message || `HTTP error: ${response.status}`);
    }

    if (!Array.isArray(data.alternatives)) {
      throw new Error('Invalid suggestion data received from API.');
    }

    // Ignore an older response if a newer request has started.
    if (requestId !== passengerSuggestionRequest) return;

    const selectedDuration = flightDurationMinutes(selected);

    const notice = `<div class="inline-notice">
      Selected flight: <strong>${esc(selected.flightNumber)}</strong>
      · ${esc(selected.source)} → ${esc(selected.destination)}
      · ${esc(selected.status)}
      · Fare ${esc(formatFare(selected.fare))}
      · Flight time ${esc(formatDuration(selectedDuration))}.
      Alternatives are ranked by fare and flight duration.
    </div>`;

    if (!data.alternatives.length) {
      host.innerHTML = notice + emptyState(
        'No alternatives found',
        'There are no active alternative flights available on your route.'
      );
      return;
    }

    const cards = data.alternatives.map((flight, index) => `
      <div class="suggestion-card ${index === 0 ? 'top-recommendation' : ''}">
        <div class="suggestion-main">
          <div class="suggestion-heading">
            <strong>${esc(flight.flightNumber)} · ${esc(flight.airline)}</strong>
            ${index === 0
              ? '<span class="recommendation-badge">Top recommended</span>'
              : `<span class="rank-badge">#${index + 1}</span>`}
          </div>

          <small>
            ${esc(flight.source)} → ${esc(flight.destination)}
            · ${esc(flight.status)}
          </small>

          <div class="suggestion-metrics">
            <span>
              <small>Fare</small>
              <strong>${esc(formatFare(flight.fare))}</strong>
            </span>
            <span>
              <small>Flight time</small>
              <strong>${esc(formatDuration(Number(flight.durationMinutes)))}</strong>
            </span>
            <span>
              <small>Departs</small>
              <strong>${esc(flight.departureTime)}</strong>
            </span>
            <span>
              <small>Score</small>
              <strong>${Number(flight.recommendationScore).toFixed(1)} / 100</strong>
            </span>
          </div>
        </div>
      </div>
    `).join('');

    host.innerHTML = `
      ${notice}
      <div class="suggestion-list">${cards}</div>
      <p class="muted recommendation-footnote">
        Recommendations are generated by the C++ SmartFlightSuggestion
        module using fare and flight duration.
      </p>
    `;

  } catch (error) {
    if (requestId !== passengerSuggestionRequest) return;

    console.error('Passenger flight suggestions error:', error);

    host.innerHTML = emptyState(
      'Suggestions unavailable',
      error.message || 'Could not retrieve flight suggestions from the server.'
    );
  }
}


/* ---------------------------- flights ---------------------------- */
function renderFlightsPage() {
  const passenger = me();
  if (!passenger) return;
  const flight = getFlight(passenger.flightNumber);
  if (flight) {
    const diff = timeNumber(flight.departureTime) - nowMinutes();
    const countdown = isUnavailableFlight(flight.status) ? flight.status : diff >= 0 ? formatCountdown(diff) : 'Departure time passed';
    $('#uFlightSummary').innerHTML = `<div class="u-metrics"><div class="panel u-metric"><span class="eyebrow">YOUR FLIGHT</span><strong>${esc(flight.flightNumber)}</strong><span>${esc(flight.source)} → ${esc(flight.destination)}</span></div><div class="panel u-metric"><span class="eyebrow">STATUS</span><strong>${statusBadge(flight.status)}</strong><span>${esc(flight.airline)}</span></div><div class="panel u-metric"><span class="eyebrow">GATE</span><strong>${esc(flight.gate)}</strong><span>Boarding closes ${validClockTime(flight.departureTime) ? esc(minutesToClock(timeNumber(flight.departureTime) - 20)) : '—'}</span></div><div class="panel u-metric"><span class="eyebrow">DEPARTS</span><strong>${esc(flight.departureTime)}</strong><span>${esc(countdown)}</span></div></div>`;
  } else {
    $('#uFlightSummary').innerHTML = '';
  }
  renderPassengerSuggestions(passenger.flightNumber);
  const flights = [...state.flights].sort((a, b) => timeNumber(a.departureTime) - timeNumber(b.departureTime));
  $('#uDepTable').innerHTML = `<thead><tr><th>Time</th><th>Flight</th><th>To</th><th>Gate</th><th>Status</th></tr></thead><tbody>${flights.length ? flights.map((f) => `<tr class="${norm(f.flightNumber) === norm(passenger.flightNumber) ? 'row-mine' : ''}"><td>${esc(f.departureTime)}</td><td class="flight-cell">${esc(f.flightNumber)}<span class="cell-sub">${esc(f.airline)}</span></td><td>${esc(f.destination.toUpperCase())}</td><td>${esc(f.gate)}</td><td>${statusBadge(f.status)}</td></tr>`).join('') : '<tr><td colspan="5">No flights scheduled.</td></tr>'}</tbody>`;
}

/* ---------------------------- assistance ---------------------------- */

async function renderAssistPage() {
  const passenger = me();
  if (!passenger) return;

  const locations = NAV_GRAPH.locations.map((l) => ({
    value: l.name,
    label: l.name
  }));

  fillSelect('uAsLoc', locations, 'Choose current location');

  if (!$('#uAsLoc').value) {
    $('#uAsLoc').value = 'Main Entrance';
  }

  const output = $('#uAsOut');
  const requestId = ++passengerAssistRequest;
  const location = $('#uAsLoc').value;

  output.innerHTML =
    '<div class="inline-notice">Generating your AeroAssist recommendation...</div>';

  try {
    const advice = await getAeroAssistAdvice(passenger.assistanceType);

    if (requestId !== passengerAssistRequest) return;

    output.innerHTML = assistMarkup(passenger, location, advice);

  } catch (error) {
    if (requestId !== passengerAssistRequest) return;

    console.error('Passenger AeroAssist API error:', error);

    output.innerHTML = emptyState(
      'AeroAssist unavailable',
      error.message || 'Could not retrieve assistance advice from the server.'
    );
  }

  const sorted = sortedPriorityQueue();

  const position = sorted.findIndex(
    (item) => norm(item.passengerId) === norm(passenger.passengerId)
  );

  $('#uPrStatus').innerHTML = position >= 0
    ? `<div class="u-callout tone-ok">${icon('check', 18)}<span><strong>Request active.</strong> Priority ${sorted[position].priority} (${esc(priorityLabel(sorted[position].priority))}) · position #${position + 1} of ${sorted.length}. ${esc(sorted[position].reason)}</span></div>`
    : `<div class="u-callout">${icon('info', 18)}<span>You have no active assistance request.</span></div>`;
}


/* ---------------------------- navigator ---------------------------- */
function renderNavigatorPage() {
  const passenger = me();
  if (!passenger) return;

  const airportItems = availableAirports.map(airport => ({
    value: airport.airportCode,
    label: `${airport.airportCode} · ${airport.city}`
  }));

  fillSelect(
    'uNavAirport',
    airportItems,
    'Choose airport',
    currentAirportCode
  );

  const locations = NAV_GRAPH.locations.map(location => ({
    value: location.name,
    label: location.name
  }));

  fillSelect('uNavFrom', locations, 'Choose starting location');
  fillSelect('uNavTo', locations, 'Choose destination');

  if (!userNavInit) {
    const firstLocation = NAV_GRAPH.locations[0]?.name || '';

    const preferredGate = gateLocationFor(
      getFlight(passenger.flightNumber)
    );

    $('#uNavFrom').value = firstLocation;
    $('#uNavTo').value = preferredGate;

    userNavInit = true;
  }

  if (!nodeByName($('#uNavFrom').value)) {
    $('#uNavFrom').value = NAV_GRAPH.locations[0]?.name || '';
  }

  if (!nodeByName($('#uNavTo').value)) {
    $('#uNavTo').value = gateLocationFor(
      getFlight(passenger.flightNumber)
    );
  }

  if (navRunActive('uMap')) return;

  const mapEl = $('#uMap');

  mapEl.classList.remove('is-navigating', 'has-arrived');

  mapEl.innerHTML = buildMapMarkup(
    $('#uNavFrom').value,
    $('#uNavTo').value,
    userRoute.path,
    'data-unode'
  );

  $('#uNavOut').innerHTML = userNavOut;
}

async function userFindRoute() {
  const from = $('#uNavFrom').value;
  const to = $('#uNavTo').value;
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

  cancelNavRun('uMap');

  try {
    const result = await requestNavigatorRoute(from, to);

    if (requestedAirport !== currentAirportCode) return;

    userRoute = {
      path: result.path,
      meters: Number(result.distanceMeters),
      minutes: Number(result.totalMinutes)
    };

    userNavOut = routeInfoMarkup(
      userRoute.path,
      userRoute.meters,
      userRoute.minutes
    );

    renderNavigatorPage();

  } catch (error) {
    userRoute = { path: [], meters: null, minutes: null };
    userNavOut = esc(error.message);

    renderNavigatorPage();

    toast('Route calculation failed', error.message, true);
  }
}


async function userTraverse(type) {
  const start = $('#uNavFrom').value;
  const requestedAirport = currentAirportCode;
  const algorithm = type.toUpperCase();

  if (!start) {
    return toast(
      'Starting location required',
      'Select the starting location first.',
      true
    );
  }

  cancelNavRun('uMap');

  try {
    const result = await requestNavigatorTraversal(
      start,
      algorithm
    );

    if (requestedAirport !== currentAirportCode) return;

    userRoute = {
      path: [],
      meters: null,
      minutes: null
    };

    userNavOut =
      `<strong>${esc(algorithm)} traversal order:</strong> ` +
      `${esc(result.order.join(' → '))}` +
      `<br><span class="muted">Traversal performed by the C++ ` +
      `${esc(algorithm)} algorithm on ${esc(requestedAirport)}.</span>`;

    renderNavigatorPage();

  } catch (error) {
    toast('Traversal failed', error.message, true);
  }
}

function userTraverse(type) {
  const start = $('#uNavFrom').value;
  if (!start) return toast('Starting location required', 'Select the starting location first.', true);
  cancelNavRun('uMap');
  const graph = adjacency(); const visited = new Set(); const order = [];
  if (type === 'bfs') {
    const queue = [start]; visited.add(start);
    while (queue.length) {
      const current = queue.shift(); order.push(current);
      for (const edge of graph.get(current)) if (!visited.has(edge.to)) { visited.add(edge.to); queue.push(edge.to); }
    }
  } else {
    const stack = [start];
    while (stack.length) {
      const current = stack.pop(); if (visited.has(current)) continue;
      visited.add(current); order.push(current);
      [...graph.get(current)].reverse().forEach((edge) => { if (!visited.has(edge.to)) stack.push(edge.to); });
    }
  }
  userRoute = { path: [], meters: null };
  userNavOut = `<strong>${type.toUpperCase()} traversal order:</strong> ${esc(order.join(' → '))}<br><span class="muted">Traversal visits each reachable location once in this sample graph.</span>`;
  renderNavigatorPage();
}
function setUserDestination(name) {
  if (!nodeByName(name)) return;
  $('#uNavTo').value = name;
  userFindRoute();
}

function resetPassengerNavigatorForAirport() {
  userNavInit = false;

  userRoute = {
    path: [],
    meters: null,
    minutes: null
  };

  userNavOut = '';

  if (me()) {
    renderNavigatorPage();
    renderEmergencyPage();
  }
}


// Refresh the passenger portal whenever the selected airport changes.
document.addEventListener('airport-graph-updated', () => {
  resetPassengerNavigatorForAirport();
});


function routeTo(from, to) {
  showUserPage('navigator');
  $('#uNavFrom').value = from; $('#uNavTo').value = to;
  userFindRoute();
}

/* ---------------------------- emergency ---------------------------- */
function renderEmergencyPage() {
  const passenger = me();
  if (!passenger) return;
  fillSelect('uEmLocation', NAV_GRAPH.locations.map((l) => ({ value: l.name, label: l.name })), 'Select your location');
  const mine = state.emergencyCases
    .filter((incident) => norm(incident.primaryPassengerId) === norm(passenger.passengerId) || (incident.affectedPassengers || []).some((a) => norm(a.passengerId) === norm(passenger.passengerId)))
    .sort((a, b) => new Date(b.reportedAt) - new Date(a.reportedAt));
  $('#uEmCount').textContent = `${mine.length} case${mine.length === 1 ? '' : 's'}`;
  $('#uEmList').innerHTML = mine.length ? mine.map((incident) => `<article class="emergency-case-card severity-${emergencySeverityClass(incident.severity)}"><div class="emergency-case-head"><div class="emergency-case-title"><span class="eyebrow">${esc(incident.caseId)} · ${esc(formatDate(incident.reportedAt))}</span><h3>${esc(incident.emergencyType)}</h3></div><div class="emergency-badges"><span class="emergency-severity-badge ${emergencySeverityClass(incident.severity)}">${esc(emergencySeverityLabel(incident.severity))}</span>${statusBadge(incident.status)}</div></div><div class="emergency-case-details"><p><strong>Location:</strong> ${esc(incident.incidentLocation)}</p><p><strong>Description:</strong> ${esc(incident.description)}</p></div>${incident.recommendedFacility && incident.route && incident.route.length ? `<div class="emergency-route-result"><strong>Nearest suitable facility: ${esc(incident.recommendedFacility)}</strong><p><strong>Route:</strong> ${esc(incident.route.join(' → '))}</p></div>` : ''}</article>`).join('') : emptyState('No emergency cases', 'Cases you report or are included in will appear here.');
}

/* ---------------------------- notifications ---------------------------- */
function renderNotesPage() {
  const passenger = me();
  if (!passenger) return;
  const activeFlights = state.flights.filter((f) => !isUnavailableFlight(f.status));
  const items = activeFlights.map((f) => ({ value: f.flightNumber, label: `${f.flightNumber} · ${f.source} → ${f.destination} · ${f.departureTime}–${f.arrivalTime}` }));
  fillSelect('uCrArrFlight', items, 'Select arriving flight');
  fillSelect('uCrNextFlight', items, 'Select connecting flight');
  if (!crInitialised) {
    if (activeFlightForPassenger(passenger)) { $('#uCrArrFlight').value = passenger.flightNumber; updateConnectionTimeFromFlight('uCrArrFlight', 'uCrArrivalTime', 'arrivalTime'); }
    crInitialised = true;
  }
  const term = norm($('#uNoteSearch').value);
  const filter = $('#uNoteFilter').value;
  const notes = myNotifications().filter((note) => {
    const matchText = [note.type, note.message, note.flightNumber].some((value) => norm(value).includes(term));
    return matchText && (filter === 'all' || (filter === 'unread' && !note.isRead) || (filter === 'read' && note.isRead));
  });
  $('#uNoteList').innerHTML = notes.length ? notes.map((n) => `<div class="notification-item ${n.isRead ? '' : 'unread'}"><span class="notification-icon">${notificationIcon(n.type)}</span><div class="notification-copy"><strong>${esc(n.type)}</strong><p>${esc(n.message)}</p><small>${esc(n.flightNumber)} · ${esc(formatDate(n.createdAt))} · ${n.isRead ? 'Read' : 'Unread'}</small></div><div class="notification-actions">${n.isRead ? '<span class="status-badge ok">Read</span>' : `<button class="row-action" type="button" data-action="mark-read" data-id="${n.notificationId}">Mark read</button>`}</div></div>`).join('') : emptyState('No notifications found', 'Change the filter or wait for new flight updates.');
}
async function markMyNotificationsRead() {
  const passenger = me();

  if (!passenger) return;

  const unread = myNotifications().filter(
    (notification) => !notification.isRead
  );

  if (!unread.length) {
    return toast(
      'No unread notifications',
      'All your notifications have already been read.'
    );
  }

  try {
    const body = new URLSearchParams({
      passengerId: passenger.passengerId
    });

    const response = await window.aeroApiFetch(
      `${API_BASE_URL}/notifications/read-all`,
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
      throw new Error(data.message || 'Could not update your notifications.');
    }

    // Update only this passenger's notifications in shared frontend state.
    state.notifications.forEach((notification) => {
      if (
        norm(notification.passengerId) ===
        norm(passenger.passengerId)
      ) {
        notification.isRead = true;
      }
    });

    saveState();
    renderAll();

    toast(
      'Notifications updated',
      `${Number(data.markedCount) || unread.length} of your notification(s) marked as read.`
    );

  } catch (error) {
    console.error('Passenger notification update error:', error);

    toast(
      'Notification update failed',
      error.message || 'Could not update your notifications in MySQL.',
      true
    );
  }
}

/* ---------------------------- render orchestration ---------------------------- */
function renderPortal() {
  if (!me()) return;
  renderUserChrome();
  renderTrip(); renderBaggagePage(); renderFlightsPage(); renderAssistPage();
  renderNavigatorPage(); renderEmergencyPage(); renderNotesPage();
}
function announceLiveChanges(passenger) {
  const newestNote = Math.max(0, ...myNotifications().map((n) => Number(n.notificationId) || 0));
  if (window.syncedFromStorage) {
    if (passenger.status !== lastMyStatus) toast('Journey updated', `Your status is now ${passenger.status}.`);
    if (newestNote > lastNoteId) {
      const fresh = myNotifications().find((n) => Number(n.notificationId) === newestNote);
      if (fresh) toast(fresh.type, fresh.message, fresh.type === 'Cancellation' || fresh.type === 'Connection Risk');
    }
  }
  lastMyStatus = passenger.status;
  lastNoteId = Math.max(lastNoteId, newestNote);
}
window.onStateRendered = function onStateRendered() {
  if (window.aeroStructSyncing) return;
  if (!session || session.role !== 'user') return;
  const passenger = me();
  if (!passenger) {
    writeSession(null); applyRole();
    toast('Session ended', 'Your passenger record is no longer available. Please sign in again.', true);
    return;
  }
  announceLiveChanges(passenger);
  renderPortal();
};
window.onMinuteTick = function onMinuteTick() {
  if (!session || session.role !== 'user' || !me()) return;
  renderTrip(); renderFlightsPage();
};

/* ---------------------------- events ---------------------------- */
document.addEventListener('click', (event) => {
  const link = event.target.closest('a[data-page], a[data-upage]');
  if (link) event.preventDefault();
  const userPageButton = event.target.closest('[data-upage]');
  if (userPageButton) { showUserPage(userPageButton.dataset.upage); return; }
  const button = event.target.closest('[data-action]');
  if (button) {
    const { action, from, to } = button.dataset;
    if (action === 'sign-out') signOut();
    else if (action === 'open-gate') openGate();
    else if (action === 'close-gate') closeGate();
    else if (action === 'toggle-nav') document.body.classList.toggle('nav-open');
    else if (action === 'close-nav') document.body.classList.remove('nav-open');
    else if (action === 'u-join-checkin') { const p = me(); if (p) enqueueById('checkinQ', p.passengerId); }
    else if (action === 'u-route') routeTo(from, to);
    return;
  }
  const mapNode = event.target.closest('[data-unode]');
  if (mapNode) setUserDestination(mapNode.dataset.unode);
});
document.addEventListener('keydown', (event) => {
  if (event.key === 'Escape') { document.body.classList.remove('nav-open'); if (gateOpen && !session) closeGate(); }
  const node = event.target.closest && event.target.closest('[data-unode]');
  if (node && (event.key === 'Enter' || event.key === ' ')) { event.preventDefault(); setUserDestination(node.dataset.unode); }
});

$('#gateForm').addEventListener('submit', submitGate);
$('#adminLoginForm').addEventListener('submit', submitAdminGate);
$('#gateUsername').addEventListener('input', () => gateError('', 'user'));
$('#adminUsername').addEventListener('input', () => gateError('', 'admin'));

$('#uBagForm').addEventListener('submit', (event) => {
  event.preventDefault();
  const passenger = me(); if (!passenger) return;
  registerBaggageRecord({ passengerId: passenger.passengerId, flightNumber: passenger.flightNumber, weight: $('#uBagWeight').value, description: $('#uBagDesc').value }, () => $('#uBagForm').reset());
});
$('#uBagIssueForm').addEventListener('submit', (event) => {
  event.preventDefault();
  reportBaggageIssueRecord($('#uBagIssueTag').value, $('#uBagIssueType').value, $('#uBagIssueDesc').value, () => $('#uBagIssueForm').reset());
});
$('#uPrForm').addEventListener('submit', (event) => {
  event.preventDefault();
  const passenger = me(); if (!passenger) return;
  if (addPriorityRequest(passenger.passengerId, Number($('#uPrType').value), $('#uPrReason').value)) { $('#uPrReason').value = ''; $('#uPrType').value = '5'; }
});
$('#uAsLoc').addEventListener('change', renderAssistPage);
$('#uNavAirport').addEventListener('change', async () => {
  if ($('#uNavAirport').value) {
    await loadAirportGraphFromApi($('#uNavAirport').value);
  }
});

$('#uNavFrom').addEventListener('change', () => {
  cancelNavRun('uMap');

  userRoute = {
    path: [],
    meters: null,
    minutes: null
  };

  userNavOut = '';
  renderNavigatorPage();
});

$('#uNavStart').addEventListener('click', async () => {
  if (!userRoute.path.length) {
    await userFindRoute();
  }

  if (userRoute.path.length) {
    runNavigation(
      'uMap',
      'uNavOut',
      'uNavStart',
      userRoute.path
    );
  }
});
$('#uNavTo').addEventListener('change', userFindRoute);
$('#uNavFind').addEventListener('click', userFindRoute);
$('#uNavGate').addEventListener('click', () => { const p = me(); if (p) setUserDestination(gateLocationFor(getFlight(p.flightNumber))); });
$('#uNavMedical').addEventListener('click', () => {
  const medical = getAirportLocationByType('Medical');

  if (medical) {
    setUserDestination(medical.name);
  } else {
    toast(
      'Medical facility unavailable',
      'No medical facility is mapped at this airport.',
      true
    );
  }
});
$('#uNavBfs').addEventListener('click', () => userTraverse('bfs'));
$('#uNavDfs').addEventListener('click', () => userTraverse('dfs'));
$('#uEmFindHospitals').addEventListener('click', () => openNearbyHospitalMap('uEmHospitalArea'));
$('#uEmHelpType').addEventListener('change', () => renderEmergencyGuidance('uEmHelpType', 'uEmFirstAid'));
$('#uEmForm').addEventListener('submit', (event) => {
  event.preventDefault();
  const passenger = me(); if (!passenger) return;
  registerEmergencyIncident({
    primaryPassengerId: passenger.passengerId,
    emergencyType: $('#uEmType').value,
    severity: 2,
    incidentLocation: $('#uEmLocation').value,
    description: $('#uEmDesc').value,
    affectedIds: []
  }, () => $('#uEmForm').reset());
});
$('#uCrForm').addEventListener('submit', (event) => {
  event.preventDefault();
  const form = event.currentTarget;
  if (!form.reportValidity()) return;
  const passenger = me(); if (!passenger) return;
  runConnectionRisk({
    passengerId: passenger.passengerId,
    arrivingFlight: $('#uCrArrFlight').value,
    connectingFlight: $('#uCrNextFlight').value,
    arrivalTime: $('#uCrArrivalTime').value,
    nextDepartureTime: $('#uCrNextDeparture').value,
    transferMinutes: $('#uCrTransfer').value,
    minimumBufferMinutes: $('#uCrMinBuffer').value
  }, $('#uCrResult'));
});
$('#uCrArrFlight').addEventListener('change', () => { $('#uCrResult').innerHTML = ''; updateConnectionTimeFromFlight('uCrArrFlight', 'uCrArrivalTime', 'arrivalTime'); });
$('#uCrNextFlight').addEventListener('change', () => { $('#uCrResult').innerHTML = ''; updateConnectionTimeFromFlight('uCrNextFlight', 'uCrNextDeparture', 'departureTime'); });
['uCrArrivalTime', 'uCrTransfer', 'uCrNextDeparture', 'uCrMinBuffer'].forEach((id) => document.getElementById(id).addEventListener('input', () => { $('#uCrResult').innerHTML = ''; }));
$('#uNoteSearch').addEventListener('input', renderNotesPage);
$('#uNoteFilter').addEventListener('change', renderNotesPage);
$('#uMarkAllRead').addEventListener('click', markMyNotificationsRead);

/* ---------------------------- real-time photography ---------------------------- */
// Chooses the day / dusk / night photo from the viewer's local clock; style.css maps the value to an image.
function applyDaypart() {
  const hour = new Date().getHours();
  const part = hour >= 6 && hour < 16 ? 'day' : hour >= 16 && hour < 19 ? 'dusk' : 'night';
  if (document.documentElement.dataset.daypart !== part) document.documentElement.dataset.daypart = part;
}
applyDaypart();
window.setInterval(applyDaypart, 60000);

/* ---------------------------- start-up ---------------------------- */
hydrateIcons();
renderEmergencyGuidance('uEmHelpType', 'uEmFirstAid');
restoreAuthSession();

window.AeroStructPortal = { getSession: () => (session ? { username: session.username, role: session.role, passengerId: session.passengerId } : null), signOut, showUserPage, routeTo };