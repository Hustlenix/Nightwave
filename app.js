const root = document.documentElement;
const header = document.querySelector('[data-header]');
const progressBar = document.querySelector('.scroll-progress i');
const reveals = document.querySelectorAll('.reveal');
const device = document.querySelector('[data-device]');
const heroObject = device?.closest('.hero-object');
const wheel = document.querySelector('[data-wheel]');
const menuButton = document.querySelector('[data-menu]');
const mobileNav = document.querySelector('[data-mobile-nav]');
const navLinks = [...document.querySelectorAll('[data-nav] a')];
const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

let ticking = false;
const updateScrollState = () => {
  ticking = false;
  const scrollable = document.documentElement.scrollHeight - window.innerHeight;
  const ratio = scrollable > 0 ? window.scrollY / scrollable : 0;
  if (progressBar) progressBar.style.transform = 'scaleX(' + Math.min(1, Math.max(0, ratio)) + ')';
  header?.classList.toggle('scrolled', window.scrollY > 18);

  const probe = document.elementFromPoint(Math.min(window.innerWidth - 2, window.innerWidth / 2), Math.min(70, window.innerHeight - 2));
  const dark = probe?.closest?.('.dark-section, .chapter-dark, .cinematic-handoff');
  header?.classList.toggle('on-dark', Boolean(dark));
};

window.addEventListener('scroll', () => {
  if (!ticking) {
    ticking = true;
    requestAnimationFrame(updateScrollState);
  }
}, { passive: true });
window.addEventListener('resize', updateScrollState);
updateScrollState();

if (!reducedMotion && window.matchMedia('(pointer:fine)').matches) {
  window.addEventListener('pointermove', (event) => {
    root.style.setProperty('--mx', event.clientX + 'px');
    root.style.setProperty('--my', event.clientY + 'px');
  }, { passive: true });
}

const revealObserver = new IntersectionObserver((entries) => {
  for (const entry of entries) {
    if (entry.isIntersecting) {
      entry.target.classList.add('visible');
      revealObserver.unobserve(entry.target);
    }
  }
}, { threshold: 0.12, rootMargin: '0px 0px -6% 0px' });
reveals.forEach((node) => revealObserver.observe(node));

const sectionTargets = navLinks
  .map((link) => ({ link, section: document.querySelector(link.getAttribute('href')) }))
  .filter((item) => item.section);

const sectionObserver = new IntersectionObserver((entries) => {
  entries.forEach((entry) => {
    if (!entry.isIntersecting) return;
    sectionTargets.forEach(({ link, section }) => link.classList.toggle('active', section === entry.target));
  });
}, { rootMargin: '-25% 0px -60% 0px', threshold: 0 });
sectionTargets.forEach(({ section }) => sectionObserver.observe(section));

const closeMenu = () => {
  document.body.classList.remove('menu-open');
  header?.classList.remove('menu-active');
  mobileNav?.classList.remove('open');
  mobileNav?.setAttribute('aria-hidden', 'true');
  menuButton?.setAttribute('aria-expanded', 'false');
};
menuButton?.addEventListener('click', () => {
  const open = menuButton.getAttribute('aria-expanded') === 'true';
  if (open) return closeMenu();
  document.body.classList.add('menu-open');
  header?.classList.add('menu-active');
  mobileNav?.classList.add('open');
  mobileNav?.setAttribute('aria-hidden', 'false');
  menuButton.setAttribute('aria-expanded', 'true');
});
mobileNav?.querySelectorAll('a').forEach((link) => link.addEventListener('click', closeMenu));
window.addEventListener('keydown', (event) => {
  if (event.key === 'Escape') closeMenu();
});

if (device && heroObject && !reducedMotion && window.matchMedia('(pointer:fine)').matches) {
  heroObject.addEventListener('pointermove', (event) => {
    const box = heroObject.getBoundingClientRect();
    const x = Math.max(0, Math.min(1, (event.clientX - box.left) / box.width));
    const y = Math.max(0, Math.min(1, (event.clientY - box.top) / box.height));
    const rx = 9 - y * 7;
    const ry = -14 + x * 9;
    const rz = 1.2 + (x - .5) * 2.2;
    heroObject.style.setProperty('--rx', rx + 'deg');
    heroObject.style.setProperty('--ry', ry + 'deg');
    heroObject.style.setProperty('--rz', rz + 'deg');
    heroObject.style.setProperty('--gx', (x * 100) + '%');
    heroObject.style.setProperty('--gy', (y * 100) + '%');
  });
  heroObject.addEventListener('pointerleave', () => {
    heroObject.style.setProperty('--rx', '6deg');
    heroObject.style.setProperty('--ry', '-11deg');
    heroObject.style.setProperty('--rz', '2deg');
    heroObject.style.setProperty('--gx', '45%');
    heroObject.style.setProperty('--gy', '20%');
  });
}

const pipeline = document.querySelector('[data-pipeline]');
const pipeSteps = [...document.querySelectorAll('.pipe-step')];
if (pipeline && pipeSteps.length && !reducedMotion && !pipeline.closest('.cine-section')) {
  let pipelineTimer = null;
  const pipelineObserver = new IntersectionObserver(([entry]) => {
    if (!entry.isIntersecting) return;
    let i = 0;
    clearInterval(pipelineTimer);
    pipelineTimer = setInterval(() => {
      pipeSteps.forEach((step, index) => step.classList.toggle('signal-active', index === i));
      i += 1;
      if (i >= pipeSteps.length) {
        setTimeout(() => pipeSteps.forEach((step) => step.classList.remove('signal-active')), 700);
        clearInterval(pipelineTimer);
      }
    }, 360);
    pipelineObserver.unobserve(pipeline);
  }, { threshold: .35 });
  pipelineObserver.observe(pipeline);
}

const tracks = [
  { number: 47, title: 'MIDNIGHT PROTOCOL', duration: 240, mark: 'NS' },
  { number: 48, title: 'SIGNAL AFTER DARK', duration: 213, mark: 'SA' },
  { number: 49, title: 'NO NETWORK', duration: 268, mark: 'NN' },
  { number: 50, title: 'LOCAL FREQUENCY', duration: 225, mark: 'LF' }
];
let trackIndex = 0;
let elapsed = 138;
let volume = 64;
let playing = false;
let lastFrame = performance.now();

const refs = {
  title: document.querySelector('[data-track-title]'),
  number: document.querySelector('[data-track-number]'),
  volume: document.querySelector('[data-volume]'),
  progress: document.querySelector('[data-player-progress]'),
  elapsed: document.querySelector('[data-elapsed]'),
  remaining: document.querySelector('[data-remaining]'),
  state: document.querySelector('[data-player-state]'),
  uiTrack: document.querySelector('[data-ui-track]'),
  uiNumber: document.querySelector('[data-ui-number]'),
  uiVolume: document.querySelector('[data-ui-volume]'),
  uiProgress: document.querySelector('[data-ui-progress]'),
  uiElapsed: document.querySelector('[data-ui-elapsed]'),
  uiDuration: document.querySelector('[data-ui-duration]'),
  album: document.querySelector('[data-album-mark]'),
  uiDemo: document.querySelector('[data-ui-demo]')
};

const formatTime = (seconds) => {
  const value = Math.max(0, Math.round(seconds));
  const minutes = Math.floor(value / 60);
  const secs = String(value % 60).padStart(2, '0');
  return minutes + ':' + secs;
};
const renderPlayer = () => {
  const track = tracks[trackIndex];
  const ratio = Math.min(1, elapsed / track.duration);
  const splitTitle = track.title.split(' ');
  const midpoint = Math.ceil(splitTitle.length / 2);
  const heroTitle = splitTitle.slice(0, midpoint).join(' ') + (splitTitle.length > 1 ? '<br>' + splitTitle.slice(midpoint).join(' ') : '');

  if (refs.title) refs.title.innerHTML = heroTitle;
  if (refs.number) refs.number.textContent = 'SD / ' + String(track.number).padStart(3, '0');
  if (refs.volume) refs.volume.textContent = 'VOL ' + volume;
  if (refs.progress) refs.progress.style.width = (ratio * 100) + '%';
  if (refs.elapsed) refs.elapsed.textContent = formatTime(elapsed);
  if (refs.remaining) refs.remaining.textContent = '-' + formatTime(track.duration - elapsed);
  if (refs.state) refs.state.textContent = playing ? 'PLAYING' : 'PAUSED';

  if (refs.uiTrack) refs.uiTrack.textContent = track.title;
  if (refs.uiNumber) refs.uiNumber.textContent = 'TRACK ' + String(track.number).padStart(3, '0');
  if (refs.uiVolume) refs.uiVolume.textContent = 'VOL ' + volume;
  if (refs.uiProgress) refs.uiProgress.style.width = (ratio * 100) + '%';
  if (refs.uiElapsed) refs.uiElapsed.textContent = formatTime(elapsed);
  if (refs.uiDuration) refs.uiDuration.textContent = formatTime(track.duration);
  if (refs.album) refs.album.textContent = track.mark;
  refs.uiDemo?.classList.toggle('playing', playing);

  document.querySelectorAll('[data-action="play"]').forEach((button) => {
    button.textContent = playing ? 'Ⅱ' : '▶';
    button.setAttribute('aria-pressed', String(playing));
  });
  wheel?.setAttribute('aria-valuenow', String(volume));
  wheel?.style.setProperty('--wheel', (volume * 2.4 - 120) + 'deg');
};
const changeTrack = (direction) => {
  trackIndex = (trackIndex + direction + tracks.length) % tracks.length;
  elapsed = 0;
  renderPlayer();
};
const changeVolume = (delta) => {
  volume = Math.max(0, Math.min(100, volume + delta));
  renderPlayer();
};
const togglePlay = () => {
  playing = !playing;
  lastFrame = performance.now();
  renderPlayer();
};
document.addEventListener('click', (event) => {
  const action = event.target.closest('[data-action]')?.dataset.action;
  if (!action) return;
  if (action === 'play') togglePlay();
  if (action === 'next') changeTrack(1);
  if (action === 'previous') changeTrack(-1);
  if (action === 'volume-up') changeVolume(4);
  if (action === 'volume-down') changeVolume(-4);
});

let draggingWheel = false;
const wheelValueFromPointer = (event) => {
  if (!wheel) return;
  const r = wheel.getBoundingClientRect();
  const cx = r.left + r.width / 2;
  const cy = r.top + r.height / 2;
  let angle = Math.atan2(event.clientY - cy, event.clientX - cx) * 180 / Math.PI + 90;
  if (angle < 0) angle += 360;
  const clamped = Math.max(0, Math.min(240, angle));
  volume = Math.round(clamped / 240 * 100);
  renderPlayer();
};
wheel?.addEventListener('pointerdown', (event) => {
  draggingWheel = true;
  wheel.setPointerCapture(event.pointerId);
  wheelValueFromPointer(event);
});
wheel?.addEventListener('pointermove', (event) => {
  if (draggingWheel) wheelValueFromPointer(event);
});
wheel?.addEventListener('pointerup', () => { draggingWheel = false; });
wheel?.addEventListener('keydown', (event) => {
  if (event.key === 'ArrowRight' || event.key === 'ArrowUp') { event.preventDefault(); changeVolume(2); }
  if (event.key === 'ArrowLeft' || event.key === 'ArrowDown') { event.preventDefault(); changeVolume(-2); }
});

const animatePlayer = (now) => {
  if (playing) {
    const delta = (now - lastFrame) / 1000;
    elapsed += Math.min(delta, .1);
    const duration = tracks[trackIndex].duration;
    if (elapsed >= duration) {
      trackIndex = (trackIndex + 1) % tracks.length;
      elapsed = 0;
    }
    renderPlayer();
  }
  lastFrame = now;
  requestAnimationFrame(animatePlayer);
};
renderPlayer();
requestAnimationFrame(animatePlayer);


/* Scroll-driven cinematic hardware disassembly */
const cinematicSection = document.querySelector('[data-explode-section]');
const phaseKicker = document.querySelector('[data-phase-kicker]');
const phaseTitle = document.querySelector('[data-phase-title]');
const phaseCopy = document.querySelector('[data-phase-copy]');
const cinematicPercent = document.querySelector('[data-cinematic-percent]');
const railProgress = document.querySelector('[data-rail-progress]');
const railPhases = [...document.querySelectorAll('[data-rail-phase]')];

const cinClamp = (value, min = 0, max = 1) => Math.max(min, Math.min(max, value));
const cinSmooth = (value) => {
  const t = cinClamp(value);
  return t * t * (3 - 2 * t);
};

let cinematicFramePending = false;
let lastPhase = '';

const setCinematicPhase = (phase) => {
  if (phase === lastPhase) return;
  lastPhase = phase;

  const phases = {
    sealed: {
      kicker: '01 / SEALED',
      title: 'THE PLAYER',
      copy: 'A self-contained music player. Scroll to open the system.'
    },
    ignition: {
      kicker: '02 / DISASSEMBLY',
      title: 'OPEN IT UP',
      copy: 'The enclosure breaks away and the functional layers start separating.'
    },
    exploded: {
      kicker: '03 / EXPLODED VIEW',
      title: 'THE HARDWARE',
      copy: 'Display, controls, audio, compute, storage and power become separate systems.'
    },
    mapped: {
      kicker: '04 / SYSTEM MAP',
      title: 'INSIDE NIGHTWAVE',
      copy: 'Every visible layer has a job. The final device only counts when all of them work together.'
    }
  };

  const data = phases[phase];
  if (!data) return;
  railPhases.forEach((node) => node.classList.toggle('active', node.dataset.railPhase === phase));

  if (phaseKicker) phaseKicker.textContent = data.kicker;
  if (phaseTitle) phaseTitle.textContent = data.title;
  if (phaseCopy) phaseCopy.textContent = data.copy;

  const phaseBox = phaseTitle?.parentElement;
  if (phaseBox && !reducedMotion) {
    phaseBox.animate(
      [
        { opacity: .35, transform: 'translateY(8px)' },
        { opacity: 1, transform: 'translateY(0)' }
      ],
      { duration: 320, easing: 'cubic-bezier(.2,.8,.2,1)' }
    );
  }
};

const renderCinematicGeometry = (e, blast, labels, progress, copyOpacity) => {
  const part = (name) => cinematicSection?.querySelector('[data-part="' + name + '"]');
  const tx = (baseX, baseY, x, y, z, rx = 0, ry = 0, rz = 0) =>
    'translate3d(calc(-50% + ' + (baseX + x * e).toFixed(2) + 'px),calc(-50% + ' + (baseY + y * e).toFixed(2) + 'px),' + (z * e).toFixed(2) + 'px) rotateX(' + (rx * e).toFixed(2) + 'deg) rotateY(' + (ry * e).toFixed(2) + 'deg) rotateZ(' + (rz * e).toFixed(2) + 'deg)';

  const transforms = {
    front: tx(0, 0, -205, -88, 170, 0, -23, -10),
    rear: tx(0, 0, 220, 98, -170, 0, 22, 11),
    display: tx(0, -73, -26, -178, 225, -8, 0, 3),
    controls: tx(0, 118, -142, 185, 190, 0, 0, -13),
    pcb: tx(0, 0, -28, 18, 45, 0, 4, -2),
    battery: tx(0, 0, 186, -5, -28, 0, 0, 7),
    speaker: tx(0, 172, 215, 90, 95, 0, 0, 13),
    storage: tx(0, 0, 245, -205, 145, 0, 0, 16),
    jack: tx(0, 0, -246, 35, 105, 0, 0, -18)
  };
  Object.entries(transforms).forEach(([name, transform]) => {
    const el = part(name);
    if (el) el.style.transform = transform;
  });

  const scene = cinematicSection?.querySelector('[data-explode-scene]');
  if (scene) {
    const vw = window.innerWidth;
    const vh = window.innerHeight;
    let fit = 1;
    if (vw <= 720) fit = .70;
    else if (vw <= 1120) fit = .82;
    else if (vh <= 760) fit = .76;
    else if (vh <= 820) fit = .82;
    else if (vw <= 1500) fit = .90;

    const scale = 1 - e * (1 - fit);
    const shiftX = vw > 1120 ? (-4.5 * e) : 0;
    const shiftY = vh <= 820 ? (-20 * e) : (-8 * e);

    scene.style.transform =
      'translate(-50%,-50%) translate(' + shiftX.toFixed(2) + 'vw,' + shiftY.toFixed(1) + 'px) ' +
      'scale(' + scale.toFixed(4) + ') rotateX(' + (2 * e).toFixed(2) + 'deg) rotateY(' + (-4 * e).toFixed(2) + 'deg)';
  }

  const copy = cinematicSection?.querySelector('[data-cinematic-copy]');
  if (copy) {
    copy.style.opacity = copyOpacity.toFixed(3);
    copy.style.transform = 'translate3d(' + (-36 * e).toFixed(1) + 'px,' + (-20 * e).toFixed(1) + 'px,0) scale(' + (1 - .035 * e).toFixed(4) + ')';
    copy.style.filter = 'blur(' + (2 * e).toFixed(2) + 'px)';
  }

  const core = cinematicSection?.querySelector('.blast-core');
  if (core) {
    core.style.opacity = blast.toFixed(3);
    core.style.transform = 'translate(-50%,-50%) scale(' + (.55 + blast * 1.65).toFixed(3) + ')';
    core.style.filter = 'blur(' + (2 + blast * 9).toFixed(2) + 'px)';
  }
  const ringA = cinematicSection?.querySelector('.ring-a');
  const ringB = cinematicSection?.querySelector('.ring-b');
  if (ringA) ringA.style.transform = 'translate(-50%,-50%) scale(' + (.55 + blast * 3).toFixed(3) + ')';
  if (ringB) ringB.style.transform = 'translate(-50%,-50%) scale(' + (.3 + blast * 4.2).toFixed(3) + ')';

  [
    ['.spark-a', 18, 180],
    ['.spark-b', 144, 150],
    ['.spark-c', 272, 200]
  ].forEach(([selector, angle, distance]) => {
    const spark = cinematicSection?.querySelector(selector);
    if (spark) spark.style.transform = 'rotate(' + angle + 'deg) translateX(' + (blast * distance).toFixed(1) + 'px)';
  });

  const exitFade = 1 - cinSmooth((progress - .88) / .10);

  cinematicSection?.querySelectorAll('.component-callout').forEach((callout) => {
    callout.style.opacity = (labels * exitFade).toFixed(3);
    callout.style.transform = 'translateY(' + ((1 - labels) * 18 - (1 - exitFade) * 8).toFixed(1) + 'px)';
  });
  cinematicSection?.querySelectorAll('.part-tag').forEach((tag) => {
    tag.style.opacity = ((.12 + labels * .88) * exitFade).toFixed(3);
  });

  const ribs = cinematicSection?.querySelector('.shell-ribs');
  if (ribs) ribs.style.opacity = (.05 + e * .35).toFixed(3);

  const meta = cinematicSection?.querySelector('.stage-meta');
  if (meta) meta.style.opacity = ((.62 + labels * .38) * exitFade).toFixed(3);

  const phase = cinematicSection?.querySelector('.cinematic-phase');
  if (phase) {
    phase.style.opacity = exitFade.toFixed(3);
    phase.style.transform = 'translateY(' + (-(1 - exitFade) * 12).toFixed(1) + 'px)';
    phase.style.visibility = exitFade < .015 ? 'hidden' : 'visible';
  }

  const rail = cinematicSection?.querySelector('.phase-rail');
  if (rail) rail.style.opacity = exitFade.toFixed(3);

  const cue = cinematicSection?.querySelector('.cinematic-scroll-cue');
  if (cue) cue.style.opacity = Math.max(0, 1 - progress * 2).toFixed(3);
};

const updateCinematicHero = () => {
  cinematicFramePending = false;
  if (!cinematicSection) return;

  if (reducedMotion) {
    cinematicSection.style.setProperty('--cin-p', '1');
    cinematicSection.style.setProperty('--explode', '.72');
    cinematicSection.style.setProperty('--blast', '0');
    cinematicSection.style.setProperty('--labels', '1');
    cinematicSection.style.setProperty('--copy-opacity', '1');
    if (cinematicPercent) cinematicPercent.textContent = '100%';
    if (railProgress) railProgress.style.transform = 'scaleY(1)';
    renderCinematicGeometry(.72, 0, 1, 1, 0);
    setCinematicPhase('mapped');
    return;
  }

  const rect = cinematicSection.getBoundingClientRect();
  const travel = Math.max(1, cinematicSection.offsetHeight - window.innerHeight);
  const progress = cinClamp((-rect.top) / travel);

  // Leave the first frame completely assembled, then accelerate the separation.
  const explode = cinSmooth((progress - .14) / .46);

  // A short flash/ring impulse during the initial shell break-away.
  const blastWindow = cinClamp((progress - .105) / .27);
  const blast = Math.pow(Math.sin(blastWindow * Math.PI), 2) * (progress < .43 ? 1 : 0);

  // Labels arrive after the movement has mostly resolved.
  const labels = cinSmooth((progress - .50) / .24);

  // Product story leaves the stage while the physical object becomes the focus.
  const copyFade = cinSmooth((progress - .055) / .27);
  const copyOpacity = Math.max(0, 1 - copyFade);

  cinematicSection.style.setProperty('--cin-p', progress.toFixed(4));
  cinematicSection.style.setProperty('--explode', explode.toFixed(4));
  cinematicSection.style.setProperty('--blast', blast.toFixed(4));
  cinematicSection.style.setProperty('--labels', labels.toFixed(4));
  cinematicSection.style.setProperty('--copy-opacity', copyOpacity.toFixed(4));
  if (cinematicPercent) cinematicPercent.textContent = String(Math.round(progress * 100)).padStart(2, '0') + '%';
  if (railProgress) railProgress.style.transform = 'scaleY(' + progress.toFixed(4) + ')';
  renderCinematicGeometry(explode, blast, labels, progress, copyOpacity);

  if (progress < .12) setCinematicPhase('sealed');
  else if (progress < .34) setCinematicPhase('ignition');
  else if (progress < .66) setCinematicPhase('exploded');
  else setCinematicPhase('mapped');
};

const requestCinematicUpdate = () => {
  if (cinematicFramePending) return;
  cinematicFramePending = true;
  requestAnimationFrame(updateCinematicHero);
};

if (cinematicSection) {
  window.addEventListener('scroll', requestCinematicUpdate, { passive: true });
  window.addEventListener('resize', requestCinematicUpdate);
  updateCinematicHero();
}


if (cinematicSection && !reducedMotion && window.matchMedia('(pointer:fine)').matches) {
  const stage = cinematicSection.querySelector('[data-explode-stage]');
  const scene = cinematicSection.querySelector('[data-explode-scene]');
  stage?.addEventListener('pointermove', (event) => {
    const rect = stage.getBoundingClientRect();
    const progress = cinClamp((-cinematicSection.getBoundingClientRect().top) / Math.max(1, cinematicSection.offsetHeight - window.innerHeight));
    if (progress < .58 || !scene) return;
    const x = ((event.clientX - rect.left) / rect.width - .5);
    const y = ((event.clientY - rect.top) / rect.height - .5);
    scene.style.setProperty('--cin-hover-x', x.toFixed(3));
    scene.style.setProperty('--cin-hover-y', y.toFixed(3));
    scene.style.filter = 'drop-shadow(' + (-x * 10).toFixed(1) + 'px ' + (30 - y * 8).toFixed(1) + 'px 46px rgba(0,0,0,.13))';
  });
  stage?.addEventListener('pointerleave', () => {
    if (!scene) return;
    scene.style.filter = '';
  });
}


/* =========================================================
   LOWER-PAGE SCROLL CINEMATICS
   ========================================================= */
const lowerCineSections = [...document.querySelectorAll('[data-cine-section]')];
const chapterSlates = [...document.querySelectorAll('[data-chapter-slate]')];

const sectionProgress = (element) => {
  const rect = element.getBoundingClientRect();
  const vh = window.innerHeight || 1;
  return cinClamp((vh * .88 - rect.top) / (vh * .88 + Math.max(rect.height, vh * .55) * .52));
};

const localProgress = (progress, start, length) =>
  cinSmooth((progress - start) / length);

const updateLowerCinematics = () => {
  if (reducedMotion) return;

  for (const section of lowerCineSections) {
    const p = sectionProgress(section);
    const eased = cinSmooth(p);
    section.style.setProperty('--cine-y', ((1 - eased) * 28).toFixed(1) + 'px');
    section.style.setProperty('--cine-scale', (.985 + eased * .015).toFixed(4));
    section.style.setProperty('--cine-opacity', (.18 + eased * .82).toFixed(3));
    section.style.setProperty('--cine-line', (.15 + eased * .85).toFixed(3));
  }

  for (const slate of chapterSlates) {
    const p = sectionProgress(slate);
    const reveal = localProgress(p, .03, .56);
    slate.style.setProperty('--chapter-o', reveal.toFixed(3));
    slate.style.setProperty('--chapter-y', ((1 - reveal) * 70).toFixed(1) + 'px');
    slate.style.setProperty('--chapter-scale', (.95 + reveal * .05).toFixed(4));
    slate.style.setProperty('--chapter-clip', ((1 - reveal) * 100).toFixed(1) + '%');
    slate.style.setProperty('--trace-x', (-110 + p * 235).toFixed(1) + '%');
    slate.style.setProperty('--orbit-rot', (p * 130).toFixed(1) + 'deg');

    const fabLayers = [...slate.querySelectorAll('.fab-layers i')];
    fabLayers.forEach((layer, index) => {
      const spread = localProgress(p, .10 + index * .025, .42);
      const y = (index - 2) * 16 * (1 - spread);
      const z = (index - 2) * 20 * spread;
      layer.style.setProperty('--fab-y', y.toFixed(1) + 'px');
      layer.style.setProperty('--fab-z', z.toFixed(1) + 'px');
      layer.style.opacity = (.16 + spread * .84).toFixed(3);
    });

    const pulse = slate.querySelector('.validation-pulse');
    if (pulse) {
      const pulseP = localProgress(p, .10, .58);
      pulse.style.setProperty('--pulse-scale', (.42 + pulseP * .75).toFixed(3));
      pulse.style.setProperty('--pulse-o', (.08 + pulseP * .72).toFixed(3));
    }
  }

  const pipelineSection = document.querySelector('[data-cine-section="pipeline"]');
  if (pipelineSection && pipeSteps.length) {
    const p = sectionProgress(pipelineSection);
    const active = Math.min(pipeSteps.length - 1, Math.max(0, Math.floor(p * pipeSteps.length)));
    pipeSteps.forEach((step, index) => {
      step.classList.toggle('signal-active', index === active && p > .05 && p < .98);
      step.classList.toggle('signal-past', index < active);
    });
  }

  const architectureSection = document.querySelector('[data-cine-section="architecture"]');
  if (architectureSection) {
    const systems = [...architectureSection.querySelectorAll('.sys')];
    const p = sectionProgress(architectureSection);
    const active = Math.min(systems.length - 1, Math.max(0, Math.floor(p * systems.length)));
    systems.forEach((system, index) => {
      system.classList.toggle('system-focus', index === active && p > .05 && p < .98);
      system.classList.toggle('system-past', index < active);
    });
    const map = architectureSection.querySelector('.system-map');
    if (map) {
      map.style.setProperty('--map-rx', ((1 - cinSmooth(p)) * 3.2).toFixed(2) + 'deg');
      map.style.setProperty('--map-scale', (.975 + cinSmooth(p) * .025).toFixed(4));
    }
  }

  const proofSection = document.querySelector('[data-cine-section="proof"]');
  if (proofSection) {
    const p = sectionProgress(proofSection);
    [...proofSection.querySelectorAll('.proof-grid article')].forEach((tile, index) => {
      const lp = localProgress(p, index * .07, .48);
      tile.style.opacity = (.22 + lp * .78).toFixed(3);
      tile.style.transform = 'translateY(' + ((1 - lp) * 48).toFixed(1) + 'px) scale(' + (.975 + lp * .025).toFixed(4) + ')';
    });
  }

  const stackSection = document.querySelector('[data-cine-section="stack"]');
  if (stackSection) {
    const p = sectionProgress(stackSection);
    [...stackSection.querySelectorAll('.bom-grid>div')].forEach((part, index) => {
      const lp = localProgress(p, index * .045, .44);
      const side = index % 2 === 0 ? -1 : 1;
      part.style.setProperty('--part-o', (.18 + lp * .82).toFixed(3));
      part.style.setProperty('--part-x', ((1 - lp) * side * 26).toFixed(1) + 'px');
      part.style.setProperty('--part-y', ((1 - lp) * 64).toFixed(1) + 'px');
      part.style.setProperty('--part-rx', ((1 - lp) * 6).toFixed(2) + 'deg');
      part.style.setProperty('--part-scale', (.965 + lp * .035).toFixed(4));
    });
  }

  const interfaceSection = document.querySelector('[data-cine-section="interface"]');
  if (interfaceSection) {
    const p = sectionProgress(interfaceSection);
    const lp = localProgress(p, .02, .62);
    interfaceSection.style.setProperty('--interface-o', (.20 + lp * .80).toFixed(3));
    interfaceSection.style.setProperty('--interface-left', ((1 - lp) * -26).toFixed(1) + 'px');
    interfaceSection.style.setProperty('--interface-right', ((1 - lp) * 26).toFixed(1) + 'px');
    interfaceSection.style.setProperty('--interface-ry', ((1 - lp) * -2.25).toFixed(2) + 'deg');
    interfaceSection.style.setProperty('--interface-scale', (.985 + lp * .015).toFixed(4));
  }

  const buildSection = document.querySelector('[data-cine-section="build"]');
  if (buildSection) {
    const rows = [...buildSection.querySelectorAll('.build-row')];
    const p = sectionProgress(buildSection);
    const active = Math.min(rows.length - 1, Math.max(0, Math.floor(p * rows.length)));
    rows.forEach((row, index) => {
      row.classList.toggle('build-current', index === active && p > .04 && p < .99);
      row.classList.toggle('build-past', index < active);
    });
  }

  const experimentSection = document.querySelector('[data-cine-section="experiment"]');
  if (experimentSection) {
    const p = sectionProgress(experimentSection);
    const lp = localProgress(p, .04, .62);
    experimentSection.style.setProperty('--exp-y', ((1 - lp) * 42).toFixed(1) + 'px');
    experimentSection.style.setProperty('--exp-o', (.20 + lp * .80).toFixed(3));
    experimentSection.style.setProperty('--boundary-scale', (.08 + lp * .92).toFixed(3));
  }

  const finaleSection = document.querySelector('[data-cine-section="finale"]');
  if (finaleSection) {
    const p = sectionProgress(finaleSection);
    const lp = localProgress(p, .02, .66);
    finaleSection.style.setProperty('--final-clip', ((1 - lp) * 100).toFixed(1) + '%');
    finaleSection.style.setProperty('--final-y', ((1 - lp) * 60).toFixed(1) + 'px');
    finaleSection.style.setProperty('--final-small-y', ((1 - lp) * 28).toFixed(1) + 'px');
    finaleSection.style.setProperty('--final-o', (.10 + lp * .90).toFixed(3));
    finaleSection.style.setProperty('--final-glow', (.55 + lp * .65).toFixed(3));
  }
};

let lowerCinePending = false;
const requestLowerCinematics = () => {
  if (lowerCinePending) return;
  lowerCinePending = true;
  requestAnimationFrame(() => {
    lowerCinePending = false;
    updateLowerCinematics();
  });
};

window.addEventListener('scroll', requestLowerCinematics, { passive: true });
window.addEventListener('resize', requestLowerCinematics);
updateLowerCinematics();
