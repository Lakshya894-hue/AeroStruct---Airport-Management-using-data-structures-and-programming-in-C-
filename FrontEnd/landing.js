'use strict';

/* ======================================================================
   AeroStruct landing page: cinematic hero, scene crossfades, parallax,
   live figures, section navigation and reveal-on-scroll.
   Loads after app.js and portal.js, so it can use their globals
   (state, NAV_GRAPH, photoImg, prefersReducedMotion).
   Performance notes
   - Only the first hero image is requested up front (preloaded in <head>).
   - Other hero scenes load after the first paint, and not at all when the
     browser has Data Saver on.
   - Below-the-fold images use loading="lazy" and responsive srcset.
   - Motion (crossfade timer, Ken Burns, parallax, counters) is switched
     off for prefers-reduced-motion; static fallbacks stay readable.
   ====================================================================== */

(function landingModule() {
  const root = document.getElementById('landing');
  if (!root) return;
  document.documentElement.classList.add('js');

  const reduced = () => prefersReducedMotion();
  const saveData = !!(navigator.connection && navigator.connection.saveData);
  const scenes = [...root.querySelectorAll('.l-scene')];
  const dotsHost = document.getElementById('lSceneDots');
  const nav = document.getElementById('lNav');
  const hero = document.getElementById('home');
  let sceneIndex = 0;
  let timer = 0;
  let heroVisible = true;

  /* ---------- lazy media ---------- */
  function mountPhoto(host, key, options) {
    if (!host || host.querySelector('img')) return;
    host.insertAdjacentHTML('afterbegin', photoImg(key, options));
  }
  function loadExtraScenes() {
    if (saveData) return;
    scenes.forEach((scene) => {
      const key = scene.dataset.photo;
      if (key) mountPhoto(scene, key, { widths: [800, 1400, 2000], sizes: '100vw', lazy: false });
    });
  }
  function mountLazyMedia() {
    const targets = [
      ['lStrip', 'strip', { widths: [900, 1500, 2200], sizes: '100vw' }],
      ['lAssistMedia', 'assist', { widths: [700, 1200, 1800], sizes: '(min-width: 1100px) 1100px, 100vw' }],
      ['lTrackMedia', 'track', { widths: [700, 1200, 1800], sizes: '(min-width: 1100px) 1100px, 100vw' }],
      ['lAboutMedia', 'about', { widths: [800, 1400, 2000], sizes: '100vw' }]
    ];
    const mount = (id, key, options) => {
      const host = document.getElementById(id);
      if (host) mountPhoto(host, key, options);
    };
    if (!('IntersectionObserver' in window)) { targets.forEach((t) => mount(...t)); return; }
    const io = new IntersectionObserver((entries) => {
      entries.forEach((entry) => {
        if (!entry.isIntersecting) return;
        const target = targets.find((t) => document.getElementById(t[0]) === entry.target);
        if (target) mount(...target);
        io.unobserve(entry.target);
      });
    }, { rootMargin: '600px 0px' });
    targets.forEach(([id]) => { const el = document.getElementById(id); if (el) io.observe(el); });
  }

  /* ---------- scene crossfade ---------- */
  function usableScenes() { return scenes.map((s, i) => ({ s, i })).filter(({ s }) => s.querySelector('img')); }
  function showScene(index) {
    sceneIndex = index;
    scenes.forEach((scene, i) => scene.classList.toggle('is-active', i === index));
    if (dotsHost) [...dotsHost.children].forEach((dot, i) => { dot.classList.toggle('is-active', i === index); dot.setAttribute('aria-pressed', i === index ? 'true' : 'false'); });
  }
  function nextScene() {
    const usable = usableScenes();
    if (usable.length < 2) return;
    const pos = usable.findIndex(({ i }) => i === sceneIndex);
    showScene(usable[(pos + 1) % usable.length].i);
  }
  function startTimer() {
    stopTimer();
    if (reduced() || saveData || document.hidden || !heroVisible) return;
    timer = window.setInterval(nextScene, 7500);
  }
  function stopTimer() { if (timer) { window.clearInterval(timer); timer = 0; } }
  function buildDots() {
    if (!dotsHost) return;
    dotsHost.innerHTML = scenes.map((_, i) => `<button type="button" aria-label="Show scene ${i + 1}" aria-pressed="${i === 0}" data-scene="${i}"></button>`).join('');
    dotsHost.firstElementChild && dotsHost.firstElementChild.classList.add('is-active');
    dotsHost.addEventListener('click', (event) => {
      const button = event.target.closest('[data-scene]');
      if (!button) return;
      const index = Number(button.dataset.scene);
      if (!scenes[index].querySelector('img')) return;
      showScene(index); startTimer();
    });
  }

  /* ---------- live figures ---------- */
  function liveFigures() {
    const flights = state.flights || [];
    return {
      flights: flights.length,
      passengers: (state.passengers || []).length,
      delayed: flights.filter((f) => norm(f.status).includes('delay')).length,
      cancelled: flights.filter((f) => norm(f.status).includes('cancel')).length
    };
  }
  function tween(el, target) {
    const from = Number(el.dataset.shown || 0);
    el.dataset.shown = String(target);
    if (reduced() || from === target) { el.textContent = target.toLocaleString('en-IN'); return; }
    const start = performance.now(), span = 900;
    const step = (now) => {
      const t = Math.min(1, (now - start) / span);
      const eased = 1 - Math.pow(1 - t, 3);
      el.textContent = Math.round(from + (target - from) * eased).toLocaleString('en-IN');
      if (t < 1) requestAnimationFrame(step);
    };
    requestAnimationFrame(step);
  }
  function renderStats() {
    const figures = liveFigures();
    root.querySelectorAll('[data-lstat]').forEach((el) => tween(el, figures[el.dataset.lstat] || 0));
  }
  function renderFacts() {
    const facts = { locations: NAV_GRAPH.locations.length, stages: 4, priorities: 5, algorithms: 3 };
    root.querySelectorAll('[data-lcount]').forEach((el) => tween(el, facts[el.dataset.lcount] || 0));
  }

  /* ---------- navigation ---------- */
  function setMenu(open) {
    root.classList.toggle('menu-open', open);
    const burger = root.querySelector('.l-burger');
    if (burger) { burger.setAttribute('aria-expanded', open ? 'true' : 'false'); burger.setAttribute('aria-label', open ? 'Close menu' : 'Open menu'); }
  }
  function scrollToSection(id) {
    const target = document.getElementById(id);
    if (!target) return;
    const offset = id === 'home' ? 0 : (nav ? nav.offsetHeight : 64) - 1;
    window.scrollTo({ top: Math.max(0, target.getBoundingClientRect().top + window.scrollY - offset), behavior: reduced() ? 'auto' : 'smooth' });
  }
  root.addEventListener('click', (event) => {
    const link = event.target.closest('[data-lscroll]');
    if (link) { event.preventDefault(); setMenu(false); scrollToSection(link.dataset.lscroll); return; }
    const burger = event.target.closest('[data-action="toggle-lmenu"]');
    if (burger) { setMenu(!root.classList.contains('menu-open')); return; }
    if (event.target.closest('[data-action="open-gate"]')) setMenu(false);
  });
  document.addEventListener('keydown', (event) => { if (event.key === 'Escape') setMenu(false); });

  const sectionIds = ['home', 'features', 'showcase', 'modules', 'technology', 'about', 'portals'];
  function updateActiveLink() {
    const probe = window.scrollY + (nav ? nav.offsetHeight : 64) + window.innerHeight * 0.28;
    let active = 'home';
    sectionIds.forEach((id) => { const el = document.getElementById(id); if (el && el.offsetTop <= probe) active = id; });
    root.querySelectorAll('.l-links a').forEach((a) => a.classList.toggle('active', a.dataset.lscroll === active));
  }

  /* ---------- parallax + scroll state (one rAF per frame) ---------- */
  let ticking = false;
  function frame() {
    ticking = false;
    if (document.body.classList.contains('is-landing') === false) return;
    const y = window.scrollY;
    if (nav) nav.classList.toggle('scrolled', y > 40);
    updateActiveLink();
    if (reduced()) return;
    const vh = window.innerHeight;
    const scenesEl = document.getElementById('lScenes');
    if (scenesEl && y < vh * 1.3) {
      scenesEl.style.transform = `translate3d(0, ${(y * 0.3).toFixed(1)}px, 0)`;
      const inner = root.querySelector('.l-hero-inner');
      if (inner) { inner.style.transform = `translate3d(0, ${(y * -0.1).toFixed(1)}px, 0)`; inner.style.opacity = String(Math.max(0, 1 - y / (vh * 0.85))); }
      const paths = root.querySelector('.l-paths');
      if (paths) paths.style.transform = `translate3d(0, ${(y * 0.16).toFixed(1)}px, 0)`;
    }
    root.querySelectorAll('[data-lparallax]').forEach((band) => {
      const rect = band.getBoundingClientRect();
      if (rect.bottom < -200 || rect.top > vh + 200) return;
      const media = band.querySelector('.l-band-media');
      if (media) media.style.transform = `translate3d(0, ${((rect.top + rect.height / 2 - vh / 2) * -0.12).toFixed(1)}px, 0)`;
    });
    const about = document.getElementById('about');
    if (about) {
      const rect = about.getBoundingClientRect();
      const media = about.querySelector('.l-about-media');
      if (media && rect.bottom > -200 && rect.top < vh + 200) media.style.transform = `translate3d(0, ${((rect.top + rect.height / 2 - vh / 2) * -0.1).toFixed(1)}px, 0)`;
    }
  }
  function onScroll() { if (!ticking) { ticking = true; requestAnimationFrame(frame); } }
  window.addEventListener('scroll', onScroll, { passive: true });
  window.addEventListener('resize', onScroll, { passive: true });

  /* ---------- reveal on scroll + counters ---------- */
  function setupReveal() {
    const items = [...root.querySelectorAll('.reveal')];
    if (!('IntersectionObserver' in window) || reduced()) { items.forEach((el) => el.classList.add('in')); renderFacts(); return; }
    const io = new IntersectionObserver((entries) => {
      entries.forEach((entry) => {
        if (!entry.isIntersecting) return;
        const el = entry.target;
        const index = items.indexOf(el);
        el.style.transitionDelay = `${(index % 6) * 70}ms`;
        el.classList.add('in');
        io.unobserve(el);
      });
    }, { threshold: 0.14, rootMargin: '0px 0px -6% 0px' });
    items.forEach((el) => io.observe(el));
    const facts = root.querySelector('.l-facts');
    if (facts) {
      const factIo = new IntersectionObserver((entries) => { if (entries[0].isIntersecting) { renderFacts(); factIo.disconnect(); } }, { threshold: 0.4 });
      factIo.observe(facts);
    } else renderFacts();
  }

  /* ---------- lifecycle ---------- */
  if ('IntersectionObserver' in window && hero) {
    new IntersectionObserver((entries) => { heroVisible = entries[0].isIntersecting; startTimer(); }, { threshold: 0.05 }).observe(hero);
  }
  document.addEventListener('visibilitychange', startTimer);
  window.addEventListener('storage', () => { if (document.body.classList.contains('is-landing')) renderStats(); });
  if (window.matchMedia) {
    const mq = window.matchMedia('(prefers-reduced-motion: reduce)');
    const apply = () => {
      root.classList.toggle('is-calm', mq.matches);
      root.querySelectorAll('svg').forEach((svg) => {
        try { if (mq.matches) svg.pauseAnimations(); else svg.unpauseAnimations(); } catch (e) { /* SVG animation control unavailable */ }
      });
      if (mq.matches) stopTimer(); else startTimer();
    };
    if (mq.addEventListener) mq.addEventListener('change', apply); else if (mq.addListener) mq.addListener(apply);
    apply();
  }
  window.onLandingShown = function onLandingShown() {
    renderStats(); startTimer(); onScroll();
  };

  buildDots();
  setupReveal();
  mountLazyMedia();
  renderStats();
  onScroll();
  // Secondary hero scenes load after the first paint so the opening image is never delayed.
  const idle = window.requestIdleCallback || ((fn) => window.setTimeout(fn, 900));
  if (document.readyState === 'complete') idle(loadExtraScenes); else window.addEventListener('load', () => idle(loadExtraScenes), { once: true });
  startTimer();
})();
