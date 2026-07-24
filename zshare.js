/* zshare.js — the share and install layer for the Zistgah properties.
   © 1993–2026 Abhishek Choudhary. All rights reserved.  SPDX: GPL-3.0-or-later

   No dependencies, no build step, no backend. Everything runs on a static
   GitHub Pages origin. Config comes from zshare.config.json so a third party
   can retune every string and control without touching this file.

   Seam census (gated by smoke_pwa.js):
     ZSHARE-STATE 6 · ZSHARE-LINK 4 · ZSHARE-CARD 3 · ZSHARE-INSTALL 6 · ZSHARE-STAMP 2
*/
(function (global) {
  'use strict';

  var DEFAULTS = {
    title: (global.document && global.document.title) || 'Zistgah',
    short: 'Zistgah',
    text: '',
    bg: '#050a18',
    fg: '#ffe9b0',
    hashKey: 's',
    whatsapp: true,
    card: true,
    stamp: true,
    mount: null,          // CSS selector; null = floating control
    installHint: true
  };

  var cfg = Object.assign({}, DEFAULTS);
  var deferredPrompt = null;

  /* ------------------------------------------------------ state in the URL --
     The share atom. Any view worth talking about must be addressable, or there
     is nothing to send. Encoded in the hash so GitHub Pages never sees it and
     no server is required. */

  function b64urlEncode(s) {                                   /* ZSHARE-STATE */
    return btoa(unescape(encodeURIComponent(s)))
      .replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
  }

  function b64urlDecode(s) {                                   /* ZSHARE-STATE */
    s = s.replace(/-/g, '+').replace(/_/g, '/');
    while (s.length % 4) s += '=';
    return decodeURIComponent(escape(atob(s)));
  }

  function readState() {                                       /* ZSHARE-STATE */
    var m = new RegExp('[#&]' + cfg.hashKey + '=([^&]+)').exec(location.hash || '');
    if (!m) return null;
    try { return JSON.parse(b64urlDecode(m[1])); } catch (e) { return null; }
  }

  function writeState(obj, replace) {                          /* ZSHARE-STATE */
    var enc = b64urlEncode(JSON.stringify(obj));
    var rest = (location.hash || '').replace(/^#/, '')
      .split('&').filter(function (p) { return p && p.indexOf(cfg.hashKey + '=') !== 0; });
    rest.unshift(cfg.hashKey + '=' + enc);
    var url = location.pathname + location.search + '#' + rest.join('&');
    if (replace && history.replaceState) history.replaceState(null, '', url);
    else if (history.pushState) history.pushState(null, '', url);
    else location.hash = rest.join('&');
    emit('zshare:state', obj);                                 /* ZSHARE-STATE */
    return url;
  }

  function emit(name, detail) {                                /* ZSHARE-STATE */
    try { global.dispatchEvent(new CustomEvent(name, { detail: detail })); } catch (e) {}
  }

  /* ----------------------------------------------------------------- link -- */

  function link(state) {                                            /* ZSHARE-LINK */
    var s = state || readState();
    var base = location.origin + location.pathname + location.search;
    return s ? base + '#' + cfg.hashKey + '=' + b64urlEncode(JSON.stringify(s)) : base;
  }

  function caption(state) {                                         /* ZSHARE-LINK */
    var s = state || readState();
    var label = (s && (s.label || s.name || s.title)) || cfg.text || cfg.title;
    return String(label);
  }

  function share(state) {                                           /* ZSHARE-LINK */
    var url = link(state), text = caption(state);
    if (navigator.share) {
      return navigator.share({ title: cfg.title, text: text, url: url })
        .then(function () { return 'shared'; })
        .catch(function () { return copy(url); });
    }
    return copy(url);
  }

  function copy(url) {                                              /* ZSHARE-LINK */
    if (navigator.clipboard && navigator.clipboard.writeText) {
      return navigator.clipboard.writeText(url).then(function () { return 'copied'; })
        .catch(function () { return 'failed'; });
    }
    var t = document.createElement('textarea');
    t.value = url; t.setAttribute('readonly', '');
    t.style.position = 'fixed'; t.style.opacity = '0';
    document.body.appendChild(t); t.select();
    var ok = false;
    try { ok = document.execCommand('copy'); } catch (e) {}
    document.body.removeChild(t);
    return Promise.resolve(ok ? 'copied' : 'failed');
  }

  function whatsapp(state) {
    var url = 'https://wa.me/?text=' +
      encodeURIComponent(caption(state) + '\n' + link(state));
    global.open(url, '_blank', 'noopener');
    return url;
  }

  /* ----------------------------------------------------------------- card --
     A PNG the recipient can look at before deciding to tap. Drawn from the
     property's own two colours — the same dome mark as the app icon. */

  function card(state, opts) {                                      /* ZSHARE-CARD */
    opts = opts || {};
    var W = opts.width || 1200, H = opts.height || 630;
    var c = document.createElement('canvas');
    c.width = W; c.height = H;
    var x = c.getContext('2d');
    if (!x) return null;
    x.fillStyle = cfg.bg; x.fillRect(0, 0, W, H);
    drawGlyph(x, 150, H / 2, 120, cfg.fg);                          /* ZSHARE-CARD */
    x.fillStyle = cfg.fg;
    x.font = '600 54px Georgia, serif';
    x.fillText(String(cfg.short).slice(0, 30), 320, H / 2 - 22);
    x.font = '400 28px Georgia, serif';
    x.fillText(caption(state).slice(0, 46), 320, H / 2 + 30);
    x.globalAlpha = 0.62;
    x.font = '400 20px Georgia, serif';
    x.fillText('© 1993–2026 Abhishek Choudhary · AyeAI', 320, H - 64);
    x.globalAlpha = 1;
    x.strokeStyle = cfg.fg; x.lineWidth = 2;
    x.beginPath(); x.moveTo(72, H - 40); x.lineTo(W - 72, H - 40); x.stroke();
    return c;
  }

  function drawGlyph(x, cx, cy, r, fg) {                            /* ZSHARE-CARD */
    x.strokeStyle = fg; x.lineWidth = Math.max(2, r * 0.1); x.lineCap = 'round';
    x.beginPath(); x.arc(cx, cy + r * 0.2, r, Math.PI, 2 * Math.PI); x.stroke();
    x.beginPath(); x.moveTo(cx - r * 1.16, cy + r * 0.2); x.lineTo(cx + r * 1.16, cy + r * 0.2); x.stroke();
    x.beginPath(); x.arc(cx, cy - r * 0.8, r * 0.2, 0, 2 * Math.PI); x.stroke();
  }

  function download(state, filename) {
    var c = card(state);
    if (!c) return null;
    var url = c.toDataURL('image/png');
    var a = document.createElement('a');
    a.href = url; a.download = filename || (cfg.short.replace(/\W+/g, '-').toLowerCase() + '.png');
    document.body.appendChild(a); a.click(); a.remove();
    return url;
  }

  /* ---------------------------------------------------------------- stamp --
     Tok DOI atom: the SHA-256 of the exact shared state. Provenance at the
     granularity of a single view, computed in the browser, no key, no server. */

  function stamp(state) {                                           /* ZSHARE-STAMP */
    var payload = JSON.stringify({ url: link(state), state: state || readState(), t: null });
    if (!(global.crypto && global.crypto.subtle)) return Promise.resolve(null);
    var buf = new TextEncoder().encode(payload);
    return global.crypto.subtle.digest('SHA-256', buf).then(function (d) {   /* ZSHARE-STAMP */
      return Array.prototype.map.call(new Uint8Array(d), function (b) {
        return ('0' + b.toString(16)).slice(-2);
      }).join('');
    });
  }

  /* -------------------------------------------------------------- install --
     iOS is the whole difficulty. There is no automatic prompt, Add to Home
     Screen exists only in Safari, and an in-app webview (the WhatsApp or
     Instagram browser, which is exactly where a shared link lands) hides the
     option entirely. So: detect the webview and say plainly what to do. */

  function ua() { return navigator.userAgent || ''; }

  function isIOS() {                                              /* ZSHARE-INSTALL */
    return /iPad|iPhone|iPod/.test(ua()) ||
      (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
  }

  function isStandalone() {                                       /* ZSHARE-INSTALL */
    return (global.matchMedia && global.matchMedia('(display-mode: standalone)').matches) ||
      navigator.standalone === true;
  }

  function isInAppBrowser() {                                     /* ZSHARE-INSTALL */
    var u = ua();
    if (/FBAN|FBAV|Instagram|Line\/|Twitter|WhatsApp|Snapchat|LinkedInApp|MicroMessenger/i.test(u)) return true;
    // iOS webviews report as Safari-like but omit the Safari token
    return isIOS() && /AppleWebKit/.test(u) && !/Safari/.test(u) && !/CriOS|FxiOS/.test(u);
  }

  function installState() {                                       /* ZSHARE-INSTALL */
    if (isStandalone()) return 'installed';
    if (isInAppBrowser()) return 'open-in-browser';
    if (deferredPrompt) return 'promptable';
    if (isIOS()) return 'ios-manual';
    return 'unsupported';
  }

  function install() {                                            /* ZSHARE-INSTALL */
    var st = installState();
    if (st === 'promptable') {
      deferredPrompt.prompt();
      return deferredPrompt.userChoice.then(function (c) {
        deferredPrompt = null;
        return c && c.outcome === 'accepted' ? 'accepted' : 'dismissed';
      });
    }
    return Promise.resolve(st);
  }

  function installHelp() {                                        /* ZSHARE-INSTALL */
    switch (installState()) {
      case 'installed':        return 'Already installed.';
      case 'promptable':       return 'Install this app on your home screen.';
      case 'ios-manual':       return 'Tap Share, then Add to Home Screen.';
      case 'open-in-browser':  return isIOS()
        ? 'Open this page in Safari first — in-app browsers cannot add to the home screen.'
        : 'Open this page in your browser to install it.';
      default:                 return 'Bookmark this page.';
    }
  }

  /* ------------------------------------------------------------------ boot -- */

  function registerSW(path) {
    if (!('serviceWorker' in navigator)) return Promise.resolve(null);
    if (location.protocol !== 'https:' && location.hostname !== 'localhost') return Promise.resolve(null);
    return navigator.serviceWorker.register(path || 'sw.js').catch(function () { return null; });
  }

  function init(options) {
    Object.assign(cfg, options || {});
    global.addEventListener('beforeinstallprompt', function (e) {
      e.preventDefault(); deferredPrompt = e; emit('zshare:installable', {});
    });
    global.addEventListener('appinstalled', function () {
      deferredPrompt = null; emit('zshare:installed', {});
    });
    registerSW(cfg.sw);
    emit('zshare:ready', { state: readState(), install: installState() });
    return API;
  }

  function loadConfig(url) {
    return fetch(url || 'zshare.config.json')
      .then(function (r) { return r.ok ? r.json() : {}; })
      .catch(function () { return {}; })
      .then(function (j) { return init(j); });
  }

  var API = {
    init: init, config: loadConfig, cfg: cfg,
    state: readState, setState: writeState,
    link: link, caption: caption, share: share, copy: copy, whatsapp: whatsapp,
    card: card, download: download, stamp: stamp,
    installState: installState, install: install, installHelp: installHelp,
    isIOS: isIOS, isInAppBrowser: isInAppBrowser, isStandalone: isStandalone,
    _encode: b64urlEncode, _decode: b64urlDecode
  };

  global.ZShare = API;
  if (typeof module !== 'undefined' && module.exports) module.exports = API;
})(typeof window !== 'undefined' ? window : this);
