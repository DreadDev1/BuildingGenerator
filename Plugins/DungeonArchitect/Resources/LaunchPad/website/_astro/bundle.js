"use strict";
var App = (() => {
  // dist/_astro/pagination.js
  function fe(e) {
    return e !== null && typeof e == "object" && "constructor" in e && e.constructor === Object;
  }
  function de(e = {}, i2 = {}) {
    const t = ["__proto__", "constructor", "prototype"];
    Object.keys(i2).filter((s) => t.indexOf(s) < 0).forEach((s) => {
      typeof e[s] > "u" ? e[s] = i2[s] : fe(i2[s]) && fe(e[s]) && Object.keys(i2[s]).length > 0 && de(e[s], i2[s]);
    });
  }
  var Se = { body: {}, addEventListener() {
  }, removeEventListener() {
  }, activeElement: { blur() {
  }, nodeName: "" }, querySelector() {
    return null;
  }, querySelectorAll() {
    return [];
  }, getElementById() {
    return null;
  }, createEvent() {
    return { initEvent() {
    } };
  }, createElement() {
    return { children: [], childNodes: [], style: {}, setAttribute() {
    }, getElementsByTagName() {
      return [];
    } };
  }, createElementNS() {
    return {};
  }, importNode() {
    return null;
  }, location: { hash: "", host: "", hostname: "", href: "", origin: "", pathname: "", protocol: "", search: "" } };
  function F() {
    const e = typeof document < "u" ? document : {};
    return de(e, Se), e;
  }
  var Le = { document: Se, navigator: { userAgent: "" }, location: { hash: "", host: "", hostname: "", href: "", origin: "", pathname: "", protocol: "", search: "" }, history: { replaceState() {
  }, pushState() {
  }, go() {
  }, back() {
  } }, CustomEvent: function() {
    return this;
  }, addEventListener() {
  }, removeEventListener() {
  }, getComputedStyle() {
    return { getPropertyValue() {
      return "";
    } };
  }, Image() {
  }, Date() {
  }, screen: {}, setTimeout() {
  }, clearTimeout() {
  }, matchMedia() {
    return {};
  }, requestAnimationFrame(e) {
    return typeof setTimeout > "u" ? (e(), null) : setTimeout(e, 0);
  }, cancelAnimationFrame(e) {
    typeof setTimeout > "u" || clearTimeout(e);
  } };
  function D() {
    const e = typeof window < "u" ? window : {};
    return de(e, Le), e;
  }
  function Ie(e = "") {
    return e.trim().split(" ").filter((i2) => !!i2.trim());
  }
  function ke(e) {
    const i2 = e;
    Object.keys(i2).forEach((t) => {
      try {
        i2[t] = null;
      } catch {
      }
      try {
        delete i2[t];
      } catch {
      }
    });
  }
  function be(e, i2 = 0) {
    return setTimeout(e, i2);
  }
  function X() {
    return Date.now();
  }
  function Oe(e) {
    const i2 = D();
    let t;
    return i2.getComputedStyle && (t = i2.getComputedStyle(e, null)), !t && e.currentStyle && (t = e.currentStyle), t || (t = e.style), t;
  }
  function Ae(e, i2 = "x") {
    const t = D();
    let s, n, r2;
    const l2 = Oe(e);
    return t.WebKitCSSMatrix ? (n = l2.transform || l2.webkitTransform, n.split(",").length > 6 && (n = n.split(", ").map((o) => o.replace(",", ".")).join(", ")), r2 = new t.WebKitCSSMatrix(n === "none" ? "" : n)) : (r2 = l2.MozTransform || l2.OTransform || l2.MsTransform || l2.msTransform || l2.transform || l2.getPropertyValue("transform").replace("translate(", "matrix(1, 0, 0, 1,"), s = r2.toString().split(",")), i2 === "x" && (t.WebKitCSSMatrix ? n = r2.m41 : s.length === 16 ? n = parseFloat(s[12]) : n = parseFloat(s[4])), i2 === "y" && (t.WebKitCSSMatrix ? n = r2.m42 : s.length === 16 ? n = parseFloat(s[13]) : n = parseFloat(s[5])), n || 0;
  }
  function j(e) {
    return typeof e == "object" && e !== null && e.constructor && Object.prototype.toString.call(e).slice(8, -1) === "Object";
  }
  function ze(e) {
    return typeof window < "u" && typeof window.HTMLElement < "u" ? e instanceof HTMLElement : e && (e.nodeType === 1 || e.nodeType === 11);
  }
  function V(...e) {
    const i2 = Object(e[0]), t = ["__proto__", "constructor", "prototype"];
    for (let s = 1; s < e.length; s += 1) {
      const n = e[s];
      if (n != null && !ze(n)) {
        const r2 = Object.keys(Object(n)).filter((l2) => t.indexOf(l2) < 0);
        for (let l2 = 0, o = r2.length; l2 < o; l2 += 1) {
          const a = r2[l2], u2 = Object.getOwnPropertyDescriptor(n, a);
          u2 !== void 0 && u2.enumerable && (j(i2[a]) && j(n[a]) ? n[a].__swiper__ ? i2[a] = n[a] : V(i2[a], n[a]) : !j(i2[a]) && j(n[a]) ? (i2[a] = {}, n[a].__swiper__ ? i2[a] = n[a] : V(i2[a], n[a])) : i2[a] = n[a]);
        }
      }
    }
    return i2;
  }
  function q(e, i2, t) {
    e.style.setProperty(i2, t);
  }
  function ye({ swiper: e, targetPosition: i2, side: t }) {
    const s = D(), n = -e.translate;
    let r2 = null, l2;
    const o = e.params.speed;
    e.wrapperEl.style.scrollSnapType = "none", s.cancelAnimationFrame(e.cssModeFrameID);
    const a = i2 > n ? "next" : "prev", u2 = (f, S2) => a === "next" && f >= S2 || a === "prev" && f <= S2, g2 = () => {
      l2 = (/* @__PURE__ */ new Date()).getTime(), r2 === null && (r2 = l2);
      const f = Math.max(Math.min((l2 - r2) / o, 1), 0), S2 = 0.5 - Math.cos(f * Math.PI) / 2;
      let c = n + S2 * (i2 - n);
      if (u2(c, i2) && (c = i2), e.wrapperEl.scrollTo({ [t]: c }), u2(c, i2)) {
        e.wrapperEl.style.overflow = "hidden", e.wrapperEl.style.scrollSnapType = "", setTimeout(() => {
          e.wrapperEl.style.overflow = "", e.wrapperEl.scrollTo({ [t]: c });
        }), s.cancelAnimationFrame(e.cssModeFrameID);
        return;
      }
      e.cssModeFrameID = s.requestAnimationFrame(g2);
    };
    g2();
  }
  function _(e, i2 = "") {
    const t = D(), s = [...e.children];
    return t.HTMLSlotElement && e instanceof HTMLSlotElement && s.push(...e.assignedElements()), i2 ? s.filter((n) => n.matches(i2)) : s;
  }
  function De(e, i2) {
    const t = [i2];
    for (; t.length > 0; ) {
      const s = t.shift();
      if (e === s) return true;
      t.push(...s.children, ...s.shadowRoot ? s.shadowRoot.children : [], ...s.assignedElements ? s.assignedElements() : []);
    }
  }
  function Ge(e, i2) {
    const t = D();
    let s = i2.contains(e);
    return !s && t.HTMLSlotElement && i2 instanceof HTMLSlotElement && (s = [...i2.assignedElements()].includes(e), s || (s = De(e, i2))), s;
  }
  function U(e) {
    try {
      console.warn(e);
      return;
    } catch {
    }
  }
  function K(e, i2 = []) {
    const t = document.createElement(e);
    return t.classList.add(...Array.isArray(i2) ? i2 : Ie(i2)), t;
  }
  function Be(e) {
    const i2 = D(), t = F(), s = e.getBoundingClientRect(), n = t.body, r2 = e.clientTop || n.clientTop || 0, l2 = e.clientLeft || n.clientLeft || 0, o = e === i2 ? i2.scrollY : e.scrollTop, a = e === i2 ? i2.scrollX : e.scrollLeft;
    return { top: s.top + o - r2, left: s.left + a - l2 };
  }
  function Ve(e, i2) {
    const t = [];
    for (; e.previousElementSibling; ) {
      const s = e.previousElementSibling;
      i2 ? s.matches(i2) && t.push(s) : t.push(s), e = s;
    }
    return t;
  }
  function $e(e, i2) {
    const t = [];
    for (; e.nextElementSibling; ) {
      const s = e.nextElementSibling;
      i2 ? s.matches(i2) && t.push(s) : t.push(s), e = s;
    }
    return t;
  }
  function N(e, i2) {
    return D().getComputedStyle(e, null).getPropertyValue(i2);
  }
  function Q(e) {
    let i2 = e, t;
    if (i2) {
      for (t = 0; (i2 = i2.previousSibling) !== null; ) i2.nodeType === 1 && (t += 1);
      return t;
    }
  }
  function Z(e, i2) {
    const t = [];
    let s = e.parentElement;
    for (; s; ) i2 ? s.matches(i2) && t.push(s) : t.push(s), s = s.parentElement;
    return t;
  }
  function ae(e, i2, t) {
    const s = D();
    return e[i2 === "width" ? "offsetWidth" : "offsetHeight"] + parseFloat(s.getComputedStyle(e, null).getPropertyValue(i2 === "width" ? "margin-right" : "margin-top")) + parseFloat(s.getComputedStyle(e, null).getPropertyValue(i2 === "width" ? "margin-left" : "margin-bottom"));
  }
  function A(e) {
    return (Array.isArray(e) ? e : [e]).filter((i2) => !!i2);
  }
  function le(e, i2 = "") {
    typeof trustedTypes < "u" ? e.innerHTML = trustedTypes.createPolicy("html", { createHTML: (t) => t }).createHTML(i2) : e.innerHTML = i2;
  }
  var J;
  function _e() {
    const e = D(), i2 = F();
    return { smoothScroll: i2.documentElement && i2.documentElement.style && "scrollBehavior" in i2.documentElement.style, touch: !!("ontouchstart" in e || e.DocumentTouch && i2 instanceof e.DocumentTouch) };
  }
  function Te() {
    return J || (J = _e()), J;
  }
  var ee;
  function Fe({ userAgent: e } = {}) {
    const i2 = Te(), t = D(), s = t.navigator.platform, n = e || t.navigator.userAgent, r2 = { ios: false, android: false }, l2 = t.screen.width, o = t.screen.height, a = n.match(/(Android);?[\s\/]+([\d.]+)?/);
    let u2 = n.match(/(iPad)(?!\1).*OS\s([\d_]+)/);
    const g2 = n.match(/(iPod)(.*OS\s([\d_]+))?/), f = !u2 && n.match(/(iPhone\sOS|iOS)\s([\d_]+)/), S2 = s === "Win32";
    let c = s === "MacIntel";
    const m = ["1024x1366", "1366x1024", "834x1194", "1194x834", "834x1112", "1112x834", "768x1024", "1024x768", "820x1180", "1180x820", "810x1080", "1080x810"];
    return !u2 && c && i2.touch && m.indexOf(`${l2}x${o}`) >= 0 && (u2 = n.match(/(Version)\/([\d.]+)/), u2 || (u2 = [0, 1, "13_0_0"]), c = false), a && !S2 && (r2.os = "android", r2.android = true), (u2 || f || g2) && (r2.os = "ios", r2.ios = true), r2;
  }
  function xe(e = {}) {
    return ee || (ee = Fe(e)), ee;
  }
  var te;
  function He() {
    const e = D(), i2 = xe();
    let t = false;
    function s() {
      const o = e.navigator.userAgent.toLowerCase();
      return o.indexOf("safari") >= 0 && o.indexOf("chrome") < 0 && o.indexOf("android") < 0;
    }
    if (s()) {
      const o = String(e.navigator.userAgent);
      if (o.includes("Version/")) {
        const [a, u2] = o.split("Version/")[1].split(" ")[0].split(".").map((g2) => Number(g2));
        t = a < 16 || a === 16 && u2 < 2;
      }
    }
    const n = /(iPhone|iPod|iPad).*AppleWebKit(?!.*Safari)/i.test(e.navigator.userAgent), r2 = s(), l2 = r2 || n && i2.ios;
    return { isSafari: t || r2, needPerspectiveFix: t, need3dFix: l2, isWebView: n };
  }
  function we() {
    return te || (te = He()), te;
  }
  function Ne({ swiper: e, on: i2, emit: t }) {
    const s = D();
    let n = null, r2 = null;
    const l2 = () => {
      !e || e.destroyed || !e.initialized || (t("beforeResize"), t("resize"));
    }, o = () => {
      !e || e.destroyed || !e.initialized || (n = new ResizeObserver((g2) => {
        r2 = s.requestAnimationFrame(() => {
          const { width: f, height: S2 } = e;
          let c = f, m = S2;
          g2.forEach(({ contentBoxSize: v2, contentRect: w2, target: d }) => {
            d && d !== e.el || (c = w2 ? w2.width : (v2[0] || v2).inlineSize, m = w2 ? w2.height : (v2[0] || v2).blockSize);
          }), (c !== f || m !== S2) && l2();
        });
      }), n.observe(e.el));
    }, a = () => {
      r2 && s.cancelAnimationFrame(r2), n && n.unobserve && e.el && (n.unobserve(e.el), n = null);
    }, u2 = () => {
      !e || e.destroyed || !e.initialized || t("orientationchange");
    };
    i2("init", () => {
      if (e.params.resizeObserver && typeof s.ResizeObserver < "u") {
        o();
        return;
      }
      s.addEventListener("resize", l2), s.addEventListener("orientationchange", u2);
    }), i2("destroy", () => {
      a(), s.removeEventListener("resize", l2), s.removeEventListener("orientationchange", u2);
    });
  }
  function Re({ swiper: e, extendParams: i2, on: t, emit: s }) {
    const n = [], r2 = D(), l2 = (u2, g2 = {}) => {
      const f = r2.MutationObserver || r2.WebkitMutationObserver, S2 = new f((c) => {
        if (e.__preventObserver__) return;
        if (c.length === 1) {
          s("observerUpdate", c[0]);
          return;
        }
        const m = function() {
          s("observerUpdate", c[0]);
        };
        r2.requestAnimationFrame ? r2.requestAnimationFrame(m) : r2.setTimeout(m, 0);
      });
      S2.observe(u2, { attributes: typeof g2.attributes > "u" ? true : g2.attributes, childList: e.isElement || (typeof g2.childList > "u" ? true : g2).childList, characterData: typeof g2.characterData > "u" ? true : g2.characterData }), n.push(S2);
    }, o = () => {
      if (e.params.observer) {
        if (e.params.observeParents) {
          const u2 = Z(e.hostEl);
          for (let g2 = 0; g2 < u2.length; g2 += 1) l2(u2[g2]);
        }
        l2(e.hostEl, { childList: e.params.observeSlideChildren }), l2(e.wrapperEl, { attributes: false });
      }
    }, a = () => {
      n.forEach((u2) => {
        u2.disconnect();
      }), n.splice(0, n.length);
    };
    i2({ observer: false, observeParents: false, observeSlideChildren: false }), t("init", o), t("destroy", a);
  }
  var We = { on(e, i2, t) {
    const s = this;
    if (!s.eventsListeners || s.destroyed || typeof i2 != "function") return s;
    const n = t ? "unshift" : "push";
    return e.split(" ").forEach((r2) => {
      s.eventsListeners[r2] || (s.eventsListeners[r2] = []), s.eventsListeners[r2][n](i2);
    }), s;
  }, once(e, i2, t) {
    const s = this;
    if (!s.eventsListeners || s.destroyed || typeof i2 != "function") return s;
    function n(...r2) {
      s.off(e, n), n.__emitterProxy && delete n.__emitterProxy, i2.apply(s, r2);
    }
    return n.__emitterProxy = i2, s.on(e, n, t);
  }, onAny(e, i2) {
    const t = this;
    if (!t.eventsListeners || t.destroyed || typeof e != "function") return t;
    const s = i2 ? "unshift" : "push";
    return t.eventsAnyListeners.indexOf(e) < 0 && t.eventsAnyListeners[s](e), t;
  }, offAny(e) {
    const i2 = this;
    if (!i2.eventsListeners || i2.destroyed || !i2.eventsAnyListeners) return i2;
    const t = i2.eventsAnyListeners.indexOf(e);
    return t >= 0 && i2.eventsAnyListeners.splice(t, 1), i2;
  }, off(e, i2) {
    const t = this;
    return !t.eventsListeners || t.destroyed || !t.eventsListeners || e.split(" ").forEach((s) => {
      typeof i2 > "u" ? t.eventsListeners[s] = [] : t.eventsListeners[s] && t.eventsListeners[s].forEach((n, r2) => {
        (n === i2 || n.__emitterProxy && n.__emitterProxy === i2) && t.eventsListeners[s].splice(r2, 1);
      });
    }), t;
  }, emit(...e) {
    const i2 = this;
    if (!i2.eventsListeners || i2.destroyed || !i2.eventsListeners) return i2;
    let t, s, n;
    return typeof e[0] == "string" || Array.isArray(e[0]) ? (t = e[0], s = e.slice(1, e.length), n = i2) : (t = e[0].events, s = e[0].data, n = e[0].context || i2), s.unshift(n), (Array.isArray(t) ? t : t.split(" ")).forEach((l2) => {
      i2.eventsAnyListeners && i2.eventsAnyListeners.length && i2.eventsAnyListeners.forEach((o) => {
        o.apply(n, [l2, ...s]);
      }), i2.eventsListeners && i2.eventsListeners[l2] && i2.eventsListeners[l2].forEach((o) => {
        o.apply(n, s);
      });
    }), i2;
  } };
  function je() {
    const e = this;
    let i2, t;
    const s = e.el;
    typeof e.params.width < "u" && e.params.width !== null ? i2 = e.params.width : i2 = s.clientWidth, typeof e.params.height < "u" && e.params.height !== null ? t = e.params.height : t = s.clientHeight, !(i2 === 0 && e.isHorizontal() || t === 0 && e.isVertical()) && (i2 = i2 - parseInt(N(s, "padding-left") || 0, 10) - parseInt(N(s, "padding-right") || 0, 10), t = t - parseInt(N(s, "padding-top") || 0, 10) - parseInt(N(s, "padding-bottom") || 0, 10), Number.isNaN(i2) && (i2 = 0), Number.isNaN(t) && (t = 0), Object.assign(e, { width: i2, height: t, size: e.isHorizontal() ? i2 : t }));
  }
  function qe() {
    const e = this;
    function i2(T, E3) {
      return parseFloat(T.getPropertyValue(e.getDirectionLabel(E3)) || 0);
    }
    const t = e.params, { wrapperEl: s, slidesEl: n, rtlTranslate: r2, wrongRTL: l2 } = e, o = e.virtual && t.virtual.enabled, a = o ? e.virtual.slides.length : e.slides.length, u2 = _(n, `.${e.params.slideClass}, swiper-slide`), g2 = o ? e.virtual.slides.length : u2.length;
    let f = [];
    const S2 = [], c = [];
    let m = t.slidesOffsetBefore;
    typeof m == "function" && (m = t.slidesOffsetBefore.call(e));
    let v2 = t.slidesOffsetAfter;
    typeof v2 == "function" && (v2 = t.slidesOffsetAfter.call(e));
    const w2 = e.snapGrid.length, d = e.slidesGrid.length, p = e.size - m - v2;
    let h2 = t.spaceBetween, b = -m, x = 0, P = 0;
    if (typeof p > "u") return;
    typeof h2 == "string" && h2.indexOf("%") >= 0 ? h2 = parseFloat(h2.replace("%", "")) / 100 * p : typeof h2 == "string" && (h2 = parseFloat(h2)), e.virtualSize = -h2 - m - v2, u2.forEach((T) => {
      r2 ? T.style.marginLeft = "" : T.style.marginRight = "", T.style.marginBottom = "", T.style.marginTop = "";
    }), t.centeredSlides && t.cssMode && (q(s, "--swiper-centered-offset-before", ""), q(s, "--swiper-centered-offset-after", ""));
    const M = t.grid && t.grid.rows > 1 && e.grid;
    M ? e.grid.initSlides(u2) : e.grid && e.grid.unsetSlides();
    let y;
    const k2 = t.slidesPerView === "auto" && t.breakpoints && Object.keys(t.breakpoints).filter((T) => typeof t.breakpoints[T].slidesPerView < "u").length > 0;
    for (let T = 0; T < g2; T += 1) {
      y = 0;
      const E3 = u2[T];
      if (!(E3 && (M && e.grid.updateSlide(T, E3, u2), N(E3, "display") === "none"))) {
        if (o && t.slidesPerView === "auto") t.virtual.slidesPerViewAutoSlideSize && (y = t.virtual.slidesPerViewAutoSlideSize), y && E3 && (t.roundLengths && (y = Math.floor(y)), E3.style[e.getDirectionLabel("width")] = `${y}px`);
        else if (t.slidesPerView === "auto") {
          k2 && (E3.style[e.getDirectionLabel("width")] = "");
          const C = getComputedStyle(E3), L = E3.style.transform, O3 = E3.style.webkitTransform;
          if (L && (E3.style.transform = "none"), O3 && (E3.style.webkitTransform = "none"), t.roundLengths) y = e.isHorizontal() ? ae(E3, "width") : ae(E3, "height");
          else {
            const G2 = i2(C, "width"), ce2 = i2(C, "padding-left"), R2 = i2(C, "padding-right"), I2 = i2(C, "margin-left"), z3 = i2(C, "margin-right"), B2 = C.getPropertyValue("box-sizing");
            if (B2 && B2 === "border-box") y = G2 + I2 + z3;
            else {
              const { clientWidth: H2, offsetWidth: Pe2 } = E3;
              y = G2 + ce2 + R2 + I2 + z3 + (Pe2 - H2);
            }
          }
          L && (E3.style.transform = L), O3 && (E3.style.webkitTransform = O3), t.roundLengths && (y = Math.floor(y));
        } else y = (p - (t.slidesPerView - 1) * h2) / t.slidesPerView, t.roundLengths && (y = Math.floor(y)), E3 && (E3.style[e.getDirectionLabel("width")] = `${y}px`);
        E3 && (E3.swiperSlideSize = y), c.push(y), t.centeredSlides ? (b = b + y / 2 + x / 2 + h2, x === 0 && T !== 0 && (b = b - p / 2 - h2), T === 0 && (b = b - p / 2 - h2), Math.abs(b) < 1 / 1e3 && (b = 0), t.roundLengths && (b = Math.floor(b)), P % t.slidesPerGroup === 0 && f.push(b), S2.push(b)) : (t.roundLengths && (b = Math.floor(b)), (P - Math.min(e.params.slidesPerGroupSkip, P)) % e.params.slidesPerGroup === 0 && f.push(b), S2.push(b), b = b + y + h2), e.virtualSize += y + h2, x = y, P += 1;
      }
    }
    if (e.virtualSize = Math.max(e.virtualSize, p) + v2, r2 && l2 && (t.effect === "slide" || t.effect === "coverflow") && (s.style.width = `${e.virtualSize + h2}px`), t.setWrapperSize && (s.style[e.getDirectionLabel("width")] = `${e.virtualSize + h2}px`), M && e.grid.updateWrapperSize(y, f), !t.centeredSlides) {
      const T = [];
      for (let E3 = 0; E3 < f.length; E3 += 1) {
        let C = f[E3];
        t.roundLengths && (C = Math.floor(C)), f[E3] <= e.virtualSize - p && T.push(C);
      }
      f = T, Math.floor(e.virtualSize - p) - Math.floor(f[f.length - 1]) > 1 && f.push(e.virtualSize - p);
    }
    if (o && t.loop) {
      const T = c[0] + h2;
      if (t.slidesPerGroup > 1) {
        const E3 = Math.ceil((e.virtual.slidesBefore + e.virtual.slidesAfter) / t.slidesPerGroup), C = T * t.slidesPerGroup;
        for (let L = 0; L < E3; L += 1) f.push(f[f.length - 1] + C);
      }
      for (let E3 = 0; E3 < e.virtual.slidesBefore + e.virtual.slidesAfter; E3 += 1) t.slidesPerGroup === 1 && f.push(f[f.length - 1] + T), S2.push(S2[S2.length - 1] + T), e.virtualSize += T;
    }
    if (f.length === 0 && (f = [0]), h2 !== 0) {
      const T = e.isHorizontal() && r2 ? "marginLeft" : e.getDirectionLabel("marginRight");
      u2.filter((E3, C) => !t.cssMode || t.loop ? true : C !== u2.length - 1).forEach((E3) => {
        E3.style[T] = `${h2}px`;
      });
    }
    if (t.centeredSlides && t.centeredSlidesBounds) {
      let T = 0;
      c.forEach((C) => {
        T += C + (h2 || 0);
      }), T -= h2;
      const E3 = T > p ? T - p : 0;
      f = f.map((C) => C <= 0 ? -m : C > E3 ? E3 + v2 : C);
    }
    if (t.centerInsufficientSlides) {
      let T = 0;
      c.forEach((C) => {
        T += C + (h2 || 0);
      }), T -= h2;
      const E3 = (m || 0) + (v2 || 0);
      if (T + E3 < p) {
        const C = (p - T - E3) / 2;
        f.forEach((L, O3) => {
          f[O3] = L - C;
        }), S2.forEach((L, O3) => {
          S2[O3] = L + C;
        });
      }
    }
    if (Object.assign(e, { slides: u2, snapGrid: f, slidesGrid: S2, slidesSizesGrid: c }), t.centeredSlides && t.cssMode && !t.centeredSlidesBounds) {
      q(s, "--swiper-centered-offset-before", `${-f[0]}px`), q(s, "--swiper-centered-offset-after", `${e.size / 2 - c[c.length - 1] / 2}px`);
      const T = -e.snapGrid[0], E3 = -e.slidesGrid[0];
      e.snapGrid = e.snapGrid.map((C) => C + T), e.slidesGrid = e.slidesGrid.map((C) => C + E3);
    }
    if (g2 !== a && e.emit("slidesLengthChange"), f.length !== w2 && (e.params.watchOverflow && e.checkOverflow(), e.emit("snapGridLengthChange")), S2.length !== d && e.emit("slidesGridLengthChange"), t.watchSlidesProgress && e.updateSlidesOffset(), e.emit("slidesUpdated"), !o && !t.cssMode && (t.effect === "slide" || t.effect === "fade")) {
      const T = `${t.containerModifierClass}backface-hidden`, E3 = e.el.classList.contains(T);
      g2 <= t.maxBackfaceHiddenSlides ? E3 || e.el.classList.add(T) : E3 && e.el.classList.remove(T);
    }
  }
  function Ye(e) {
    const i2 = this, t = [], s = i2.virtual && i2.params.virtual.enabled;
    let n = 0, r2;
    typeof e == "number" ? i2.setTransition(e) : e === true && i2.setTransition(i2.params.speed);
    const l2 = (o) => s ? i2.slides[i2.getSlideIndexByData(o)] : i2.slides[o];
    if (i2.params.slidesPerView !== "auto" && i2.params.slidesPerView > 1) if (i2.params.centeredSlides) (i2.visibleSlides || []).forEach((o) => {
      t.push(o);
    });
    else for (r2 = 0; r2 < Math.ceil(i2.params.slidesPerView); r2 += 1) {
      const o = i2.activeIndex + r2;
      if (o > i2.slides.length && !s) break;
      t.push(l2(o));
    }
    else t.push(l2(i2.activeIndex));
    for (r2 = 0; r2 < t.length; r2 += 1) if (typeof t[r2] < "u") {
      const o = t[r2].offsetHeight;
      n = o > n ? o : n;
    }
    (n || n === 0) && (i2.wrapperEl.style.height = `${n}px`);
  }
  function Xe() {
    const e = this, i2 = e.slides, t = e.isElement ? e.isHorizontal() ? e.wrapperEl.offsetLeft : e.wrapperEl.offsetTop : 0;
    for (let s = 0; s < i2.length; s += 1) i2[s].swiperSlideOffset = (e.isHorizontal() ? i2[s].offsetLeft : i2[s].offsetTop) - t - e.cssOverflowAdjustment();
  }
  var ue = (e, i2, t) => {
    i2 && !e.classList.contains(t) ? e.classList.add(t) : !i2 && e.classList.contains(t) && e.classList.remove(t);
  };
  function Ue(e = this && this.translate || 0) {
    const i2 = this, t = i2.params, { slides: s, rtlTranslate: n, snapGrid: r2 } = i2;
    if (s.length === 0) return;
    typeof s[0].swiperSlideOffset > "u" && i2.updateSlidesOffset();
    let l2 = -e;
    n && (l2 = e), i2.visibleSlidesIndexes = [], i2.visibleSlides = [];
    let o = t.spaceBetween;
    typeof o == "string" && o.indexOf("%") >= 0 ? o = parseFloat(o.replace("%", "")) / 100 * i2.size : typeof o == "string" && (o = parseFloat(o));
    for (let a = 0; a < s.length; a += 1) {
      const u2 = s[a];
      let g2 = u2.swiperSlideOffset;
      t.cssMode && t.centeredSlides && (g2 -= s[0].swiperSlideOffset);
      const f = (l2 + (t.centeredSlides ? i2.minTranslate() : 0) - g2) / (u2.swiperSlideSize + o), S2 = (l2 - r2[0] + (t.centeredSlides ? i2.minTranslate() : 0) - g2) / (u2.swiperSlideSize + o), c = -(l2 - g2), m = c + i2.slidesSizesGrid[a], v2 = c >= 0 && c <= i2.size - i2.slidesSizesGrid[a], w2 = c >= 0 && c < i2.size - 1 || m > 1 && m <= i2.size || c <= 0 && m >= i2.size;
      w2 && (i2.visibleSlides.push(u2), i2.visibleSlidesIndexes.push(a)), ue(u2, w2, t.slideVisibleClass), ue(u2, v2, t.slideFullyVisibleClass), u2.progress = n ? -f : f, u2.originalProgress = n ? -S2 : S2;
    }
  }
  function Ke(e) {
    const i2 = this;
    if (typeof e > "u") {
      const g2 = i2.rtlTranslate ? -1 : 1;
      e = i2 && i2.translate && i2.translate * g2 || 0;
    }
    const t = i2.params, s = i2.maxTranslate() - i2.minTranslate();
    let { progress: n, isBeginning: r2, isEnd: l2, progressLoop: o } = i2;
    const a = r2, u2 = l2;
    if (s === 0) n = 0, r2 = true, l2 = true;
    else {
      n = (e - i2.minTranslate()) / s;
      const g2 = Math.abs(e - i2.minTranslate()) < 1, f = Math.abs(e - i2.maxTranslate()) < 1;
      r2 = g2 || n <= 0, l2 = f || n >= 1, g2 && (n = 0), f && (n = 1);
    }
    if (t.loop) {
      const g2 = i2.getSlideIndexByData(0), f = i2.getSlideIndexByData(i2.slides.length - 1), S2 = i2.slidesGrid[g2], c = i2.slidesGrid[f], m = i2.slidesGrid[i2.slidesGrid.length - 1], v2 = Math.abs(e);
      v2 >= S2 ? o = (v2 - S2) / m : o = (v2 + m - c) / m, o > 1 && (o -= 1);
    }
    Object.assign(i2, { progress: n, progressLoop: o, isBeginning: r2, isEnd: l2 }), (t.watchSlidesProgress || t.centeredSlides && t.autoHeight) && i2.updateSlidesProgress(e), r2 && !a && i2.emit("reachBeginning toEdge"), l2 && !u2 && i2.emit("reachEnd toEdge"), (a && !r2 || u2 && !l2) && i2.emit("fromEdge"), i2.emit("progress", n);
  }
  var ie = (e, i2, t) => {
    i2 && !e.classList.contains(t) ? e.classList.add(t) : !i2 && e.classList.contains(t) && e.classList.remove(t);
  };
  function Qe() {
    const e = this, { slides: i2, params: t, slidesEl: s, activeIndex: n } = e, r2 = e.virtual && t.virtual.enabled, l2 = e.grid && t.grid && t.grid.rows > 1, o = (f) => _(s, `.${t.slideClass}${f}, swiper-slide${f}`)[0];
    let a, u2, g2;
    if (r2) if (t.loop) {
      let f = n - e.virtual.slidesBefore;
      f < 0 && (f = e.virtual.slides.length + f), f >= e.virtual.slides.length && (f -= e.virtual.slides.length), a = o(`[data-swiper-slide-index="${f}"]`);
    } else a = o(`[data-swiper-slide-index="${n}"]`);
    else l2 ? (a = i2.find((f) => f.column === n), g2 = i2.find((f) => f.column === n + 1), u2 = i2.find((f) => f.column === n - 1)) : a = i2[n];
    a && (l2 || (g2 = $e(a, `.${t.slideClass}, swiper-slide`)[0], t.loop && !g2 && (g2 = i2[0]), u2 = Ve(a, `.${t.slideClass}, swiper-slide`)[0], t.loop && !u2 === 0 && (u2 = i2[i2.length - 1]))), i2.forEach((f) => {
      ie(f, f === a, t.slideActiveClass), ie(f, f === g2, t.slideNextClass), ie(f, f === u2, t.slidePrevClass);
    }), e.emitSlidesClasses();
  }
  var Y = (e, i2) => {
    if (!e || e.destroyed || !e.params) return;
    const t = () => e.isElement ? "swiper-slide" : `.${e.params.slideClass}`, s = i2.closest(t());
    if (s) {
      let n = s.querySelector(`.${e.params.lazyPreloaderClass}`);
      !n && e.isElement && (s.shadowRoot ? n = s.shadowRoot.querySelector(`.${e.params.lazyPreloaderClass}`) : requestAnimationFrame(() => {
        s.shadowRoot && (n = s.shadowRoot.querySelector(`.${e.params.lazyPreloaderClass}`), n && n.remove());
      })), n && n.remove();
    }
  };
  var se = (e, i2) => {
    if (!e.slides[i2]) return;
    const t = e.slides[i2].querySelector('[loading="lazy"]');
    t && t.removeAttribute("loading");
  };
  var oe = (e) => {
    if (!e || e.destroyed || !e.params) return;
    let i2 = e.params.lazyPreloadPrevNext;
    const t = e.slides.length;
    if (!t || !i2 || i2 < 0) return;
    i2 = Math.min(i2, t);
    const s = e.params.slidesPerView === "auto" ? e.slidesPerViewDynamic() : Math.ceil(e.params.slidesPerView), n = e.activeIndex;
    if (e.params.grid && e.params.grid.rows > 1) {
      const l2 = n, o = [l2 - i2];
      o.push(...Array.from({ length: i2 }).map((a, u2) => l2 + s + u2)), e.slides.forEach((a, u2) => {
        o.includes(a.column) && se(e, u2);
      });
      return;
    }
    const r2 = n + s - 1;
    if (e.params.rewind || e.params.loop) for (let l2 = n - i2; l2 <= r2 + i2; l2 += 1) {
      const o = (l2 % t + t) % t;
      (o < n || o > r2) && se(e, o);
    }
    else for (let l2 = Math.max(n - i2, 0); l2 <= Math.min(r2 + i2, t - 1); l2 += 1) l2 !== n && (l2 > r2 || l2 < n) && se(e, l2);
  };
  function Ze(e) {
    const { slidesGrid: i2, params: t } = e, s = e.rtlTranslate ? e.translate : -e.translate;
    let n;
    for (let r2 = 0; r2 < i2.length; r2 += 1) typeof i2[r2 + 1] < "u" ? s >= i2[r2] && s < i2[r2 + 1] - (i2[r2 + 1] - i2[r2]) / 2 ? n = r2 : s >= i2[r2] && s < i2[r2 + 1] && (n = r2 + 1) : s >= i2[r2] && (n = r2);
    return t.normalizeSlideIndex && (n < 0 || typeof n > "u") && (n = 0), n;
  }
  function Je(e) {
    const i2 = this, t = i2.rtlTranslate ? i2.translate : -i2.translate, { snapGrid: s, params: n, activeIndex: r2, realIndex: l2, snapIndex: o } = i2;
    let a = e, u2;
    const g2 = (c) => {
      let m = c - i2.virtual.slidesBefore;
      return m < 0 && (m = i2.virtual.slides.length + m), m >= i2.virtual.slides.length && (m -= i2.virtual.slides.length), m;
    };
    if (typeof a > "u" && (a = Ze(i2)), s.indexOf(t) >= 0) u2 = s.indexOf(t);
    else {
      const c = Math.min(n.slidesPerGroupSkip, a);
      u2 = c + Math.floor((a - c) / n.slidesPerGroup);
    }
    if (u2 >= s.length && (u2 = s.length - 1), a === r2 && !i2.params.loop) {
      u2 !== o && (i2.snapIndex = u2, i2.emit("snapIndexChange"));
      return;
    }
    if (a === r2 && i2.params.loop && i2.virtual && i2.params.virtual.enabled) {
      i2.realIndex = g2(a);
      return;
    }
    const f = i2.grid && n.grid && n.grid.rows > 1;
    let S2;
    if (i2.virtual && n.virtual.enabled && n.loop) S2 = g2(a);
    else if (f) {
      const c = i2.slides.find((v2) => v2.column === a);
      let m = parseInt(c.getAttribute("data-swiper-slide-index"), 10);
      Number.isNaN(m) && (m = Math.max(i2.slides.indexOf(c), 0)), S2 = Math.floor(m / n.grid.rows);
    } else if (i2.slides[a]) {
      const c = i2.slides[a].getAttribute("data-swiper-slide-index");
      c ? S2 = parseInt(c, 10) : S2 = a;
    } else S2 = a;
    Object.assign(i2, { previousSnapIndex: o, snapIndex: u2, previousRealIndex: l2, realIndex: S2, previousIndex: r2, activeIndex: a }), i2.initialized && oe(i2), i2.emit("activeIndexChange"), i2.emit("snapIndexChange"), (i2.initialized || i2.params.runCallbacksOnInit) && (l2 !== S2 && i2.emit("realIndexChange"), i2.emit("slideChange"));
  }
  function et(e, i2) {
    const t = this, s = t.params;
    let n = e.closest(`.${s.slideClass}, swiper-slide`);
    !n && t.isElement && i2 && i2.length > 1 && i2.includes(e) && [...i2.slice(i2.indexOf(e) + 1, i2.length)].forEach((o) => {
      !n && o.matches && o.matches(`.${s.slideClass}, swiper-slide`) && (n = o);
    });
    let r2 = false, l2;
    if (n) {
      for (let o = 0; o < t.slides.length; o += 1) if (t.slides[o] === n) {
        r2 = true, l2 = o;
        break;
      }
    }
    if (n && r2) t.clickedSlide = n, t.virtual && t.params.virtual.enabled ? t.clickedIndex = parseInt(n.getAttribute("data-swiper-slide-index"), 10) : t.clickedIndex = l2;
    else {
      t.clickedSlide = void 0, t.clickedIndex = void 0;
      return;
    }
    s.slideToClickedSlide && t.clickedIndex !== void 0 && t.clickedIndex !== t.activeIndex && t.slideToClickedSlide();
  }
  var tt = { updateSize: je, updateSlides: qe, updateAutoHeight: Ye, updateSlidesOffset: Xe, updateSlidesProgress: Ue, updateProgress: Ke, updateSlidesClasses: Qe, updateActiveIndex: Je, updateClickedSlide: et };
  function it(e = this.isHorizontal() ? "x" : "y") {
    const i2 = this, { params: t, rtlTranslate: s, translate: n, wrapperEl: r2 } = i2;
    if (t.virtualTranslate) return s ? -n : n;
    if (t.cssMode) return n;
    let l2 = Ae(r2, e);
    return l2 += i2.cssOverflowAdjustment(), s && (l2 = -l2), l2 || 0;
  }
  function st(e, i2) {
    const t = this, { rtlTranslate: s, params: n, wrapperEl: r2, progress: l2 } = t;
    let o = 0, a = 0;
    const u2 = 0;
    t.isHorizontal() ? o = s ? -e : e : a = e, n.roundLengths && (o = Math.floor(o), a = Math.floor(a)), t.previousTranslate = t.translate, t.translate = t.isHorizontal() ? o : a, n.cssMode ? r2[t.isHorizontal() ? "scrollLeft" : "scrollTop"] = t.isHorizontal() ? -o : -a : n.virtualTranslate || (t.isHorizontal() ? o -= t.cssOverflowAdjustment() : a -= t.cssOverflowAdjustment(), r2.style.transform = `translate3d(${o}px, ${a}px, ${u2}px)`);
    let g2;
    const f = t.maxTranslate() - t.minTranslate();
    f === 0 ? g2 = 0 : g2 = (e - t.minTranslate()) / f, g2 !== l2 && t.updateProgress(e), t.emit("setTranslate", t.translate, i2);
  }
  function nt() {
    return -this.snapGrid[0];
  }
  function rt() {
    return -this.snapGrid[this.snapGrid.length - 1];
  }
  function at(e = 0, i2 = this.params.speed, t = true, s = true, n) {
    const r2 = this, { params: l2, wrapperEl: o } = r2;
    if (r2.animating && l2.preventInteractionOnTransition) return false;
    const a = r2.minTranslate(), u2 = r2.maxTranslate();
    let g2;
    if (s && e > a ? g2 = a : s && e < u2 ? g2 = u2 : g2 = e, r2.updateProgress(g2), l2.cssMode) {
      const f = r2.isHorizontal();
      if (i2 === 0) o[f ? "scrollLeft" : "scrollTop"] = -g2;
      else {
        if (!r2.support.smoothScroll) return ye({ swiper: r2, targetPosition: -g2, side: f ? "left" : "top" }), true;
        o.scrollTo({ [f ? "left" : "top"]: -g2, behavior: "smooth" });
      }
      return true;
    }
    return i2 === 0 ? (r2.setTransition(0), r2.setTranslate(g2), t && (r2.emit("beforeTransitionStart", i2, n), r2.emit("transitionEnd"))) : (r2.setTransition(i2), r2.setTranslate(g2), t && (r2.emit("beforeTransitionStart", i2, n), r2.emit("transitionStart")), r2.animating || (r2.animating = true, r2.onTranslateToWrapperTransitionEnd || (r2.onTranslateToWrapperTransitionEnd = function(S2) {
      !r2 || r2.destroyed || S2.target === this && (r2.wrapperEl.removeEventListener("transitionend", r2.onTranslateToWrapperTransitionEnd), r2.onTranslateToWrapperTransitionEnd = null, delete r2.onTranslateToWrapperTransitionEnd, r2.animating = false, t && r2.emit("transitionEnd"));
    }), r2.wrapperEl.addEventListener("transitionend", r2.onTranslateToWrapperTransitionEnd))), true;
  }
  var lt = { getTranslate: it, setTranslate: st, minTranslate: nt, maxTranslate: rt, translateTo: at };
  function ot(e, i2) {
    const t = this;
    t.params.cssMode || (t.wrapperEl.style.transitionDuration = `${e}ms`, t.wrapperEl.style.transitionDelay = e === 0 ? "0ms" : ""), t.emit("setTransition", e, i2);
  }
  function Ee({ swiper: e, runCallbacks: i2, direction: t, step: s }) {
    const { activeIndex: n, previousIndex: r2 } = e;
    let l2 = t;
    l2 || (n > r2 ? l2 = "next" : n < r2 ? l2 = "prev" : l2 = "reset"), e.emit(`transition${s}`), i2 && l2 === "reset" ? e.emit(`slideResetTransition${s}`) : i2 && n !== r2 && (e.emit(`slideChangeTransition${s}`), l2 === "next" ? e.emit(`slideNextTransition${s}`) : e.emit(`slidePrevTransition${s}`));
  }
  function dt(e = true, i2) {
    const t = this, { params: s } = t;
    s.cssMode || (s.autoHeight && t.updateAutoHeight(), Ee({ swiper: t, runCallbacks: e, direction: i2, step: "Start" }));
  }
  function ct(e = true, i2) {
    const t = this, { params: s } = t;
    t.animating = false, !s.cssMode && (t.setTransition(0), Ee({ swiper: t, runCallbacks: e, direction: i2, step: "End" }));
  }
  var ft = { setTransition: ot, transitionStart: dt, transitionEnd: ct };
  function ut(e = 0, i2, t = true, s, n) {
    typeof e == "string" && (e = parseInt(e, 10));
    const r2 = this;
    let l2 = e;
    l2 < 0 && (l2 = 0);
    const { params: o, snapGrid: a, slidesGrid: u2, previousIndex: g2, activeIndex: f, rtlTranslate: S2, wrapperEl: c, enabled: m } = r2;
    if (!m && !s && !n || r2.destroyed || r2.animating && o.preventInteractionOnTransition) return false;
    typeof i2 > "u" && (i2 = r2.params.speed);
    const v2 = Math.min(r2.params.slidesPerGroupSkip, l2);
    let w2 = v2 + Math.floor((l2 - v2) / r2.params.slidesPerGroup);
    w2 >= a.length && (w2 = a.length - 1);
    const d = -a[w2];
    if (o.normalizeSlideIndex) for (let M = 0; M < u2.length; M += 1) {
      const y = -Math.floor(d * 100), k2 = Math.floor(u2[M] * 100), T = Math.floor(u2[M + 1] * 100);
      typeof u2[M + 1] < "u" ? y >= k2 && y < T - (T - k2) / 2 ? l2 = M : y >= k2 && y < T && (l2 = M + 1) : y >= k2 && (l2 = M);
    }
    if (r2.initialized && l2 !== f && (!r2.allowSlideNext && (S2 ? d > r2.translate && d > r2.minTranslate() : d < r2.translate && d < r2.minTranslate()) || !r2.allowSlidePrev && d > r2.translate && d > r2.maxTranslate() && (f || 0) !== l2)) return false;
    l2 !== (g2 || 0) && t && r2.emit("beforeSlideChangeStart"), r2.updateProgress(d);
    let p;
    l2 > f ? p = "next" : l2 < f ? p = "prev" : p = "reset";
    const h2 = r2.virtual && r2.params.virtual.enabled;
    if (!(h2 && n) && (S2 && -d === r2.translate || !S2 && d === r2.translate)) return r2.updateActiveIndex(l2), o.autoHeight && r2.updateAutoHeight(), r2.updateSlidesClasses(), o.effect !== "slide" && r2.setTranslate(d), p !== "reset" && (r2.transitionStart(t, p), r2.transitionEnd(t, p)), false;
    if (o.cssMode) {
      const M = r2.isHorizontal(), y = S2 ? d : -d;
      if (i2 === 0) h2 && (r2.wrapperEl.style.scrollSnapType = "none", r2._immediateVirtual = true), h2 && !r2._cssModeVirtualInitialSet && r2.params.initialSlide > 0 ? (r2._cssModeVirtualInitialSet = true, requestAnimationFrame(() => {
        c[M ? "scrollLeft" : "scrollTop"] = y;
      })) : c[M ? "scrollLeft" : "scrollTop"] = y, h2 && requestAnimationFrame(() => {
        r2.wrapperEl.style.scrollSnapType = "", r2._immediateVirtual = false;
      });
      else {
        if (!r2.support.smoothScroll) return ye({ swiper: r2, targetPosition: y, side: M ? "left" : "top" }), true;
        c.scrollTo({ [M ? "left" : "top"]: y, behavior: "smooth" });
      }
      return true;
    }
    const P = we().isSafari;
    return h2 && !n && P && r2.isElement && r2.virtual.update(false, false, l2), r2.setTransition(i2), r2.setTranslate(d), r2.updateActiveIndex(l2), r2.updateSlidesClasses(), r2.emit("beforeTransitionStart", i2, s), r2.transitionStart(t, p), i2 === 0 ? r2.transitionEnd(t, p) : r2.animating || (r2.animating = true, r2.onSlideToWrapperTransitionEnd || (r2.onSlideToWrapperTransitionEnd = function(y) {
      !r2 || r2.destroyed || y.target === this && (r2.wrapperEl.removeEventListener("transitionend", r2.onSlideToWrapperTransitionEnd), r2.onSlideToWrapperTransitionEnd = null, delete r2.onSlideToWrapperTransitionEnd, r2.transitionEnd(t, p));
    }), r2.wrapperEl.addEventListener("transitionend", r2.onSlideToWrapperTransitionEnd)), true;
  }
  function pt(e = 0, i2, t = true, s) {
    typeof e == "string" && (e = parseInt(e, 10));
    const n = this;
    if (n.destroyed) return;
    typeof i2 > "u" && (i2 = n.params.speed);
    const r2 = n.grid && n.params.grid && n.params.grid.rows > 1;
    let l2 = e;
    if (n.params.loop) if (n.virtual && n.params.virtual.enabled) l2 = l2 + n.virtual.slidesBefore;
    else {
      let o;
      if (r2) {
        const v2 = l2 * n.params.grid.rows;
        o = n.slides.find((w2) => w2.getAttribute("data-swiper-slide-index") * 1 === v2).column;
      } else o = n.getSlideIndexByData(l2);
      const a = r2 ? Math.ceil(n.slides.length / n.params.grid.rows) : n.slides.length, { centeredSlides: u2, slidesOffsetBefore: g2, slidesOffsetAfter: f } = n.params, S2 = u2 || !!g2 || !!f;
      let c = n.params.slidesPerView;
      c === "auto" ? c = n.slidesPerViewDynamic() : (c = Math.ceil(parseFloat(n.params.slidesPerView, 10)), S2 && c % 2 === 0 && (c = c + 1));
      let m = a - o < c;
      if (S2 && (m = m || o < Math.ceil(c / 2)), s && S2 && n.params.slidesPerView !== "auto" && !r2 && (m = false), m) {
        const v2 = S2 ? o < n.activeIndex ? "prev" : "next" : o - n.activeIndex - 1 < n.params.slidesPerView ? "next" : "prev";
        n.loopFix({ direction: v2, slideTo: true, activeSlideIndex: v2 === "next" ? o + 1 : o - a + 1, slideRealIndex: v2 === "next" ? n.realIndex : void 0 });
      }
      if (r2) {
        const v2 = l2 * n.params.grid.rows;
        l2 = n.slides.find((w2) => w2.getAttribute("data-swiper-slide-index") * 1 === v2).column;
      } else l2 = n.getSlideIndexByData(l2);
    }
    return requestAnimationFrame(() => {
      n.slideTo(l2, i2, t, s);
    }), n;
  }
  function mt(e, i2 = true, t) {
    const s = this, { enabled: n, params: r2, animating: l2 } = s;
    if (!n || s.destroyed) return s;
    typeof e > "u" && (e = s.params.speed);
    let o = r2.slidesPerGroup;
    r2.slidesPerView === "auto" && r2.slidesPerGroup === 1 && r2.slidesPerGroupAuto && (o = Math.max(s.slidesPerViewDynamic("current", true), 1));
    const a = s.activeIndex < r2.slidesPerGroupSkip ? 1 : o, u2 = s.virtual && r2.virtual.enabled;
    if (r2.loop) {
      if (l2 && !u2 && r2.loopPreventsSliding) return false;
      if (s.loopFix({ direction: "next" }), s._clientLeft = s.wrapperEl.clientLeft, s.activeIndex === s.slides.length - 1 && r2.cssMode) return requestAnimationFrame(() => {
        s.slideTo(s.activeIndex + a, e, i2, t);
      }), true;
    }
    return r2.rewind && s.isEnd ? s.slideTo(0, e, i2, t) : s.slideTo(s.activeIndex + a, e, i2, t);
  }
  function ht(e, i2 = true, t) {
    const s = this, { params: n, snapGrid: r2, slidesGrid: l2, rtlTranslate: o, enabled: a, animating: u2 } = s;
    if (!a || s.destroyed) return s;
    typeof e > "u" && (e = s.params.speed);
    const g2 = s.virtual && n.virtual.enabled;
    if (n.loop) {
      if (u2 && !g2 && n.loopPreventsSliding) return false;
      s.loopFix({ direction: "prev" }), s._clientLeft = s.wrapperEl.clientLeft;
    }
    const f = o ? s.translate : -s.translate;
    function S2(p) {
      return p < 0 ? -Math.floor(Math.abs(p)) : Math.floor(p);
    }
    const c = S2(f), m = r2.map((p) => S2(p)), v2 = n.freeMode && n.freeMode.enabled;
    let w2 = r2[m.indexOf(c) - 1];
    if (typeof w2 > "u" && (n.cssMode || v2)) {
      let p;
      r2.forEach((h2, b) => {
        c >= h2 && (p = b);
      }), typeof p < "u" && (w2 = v2 ? r2[p] : r2[p > 0 ? p - 1 : p]);
    }
    let d = 0;
    if (typeof w2 < "u" && (d = l2.indexOf(w2), d < 0 && (d = s.activeIndex - 1), n.slidesPerView === "auto" && n.slidesPerGroup === 1 && n.slidesPerGroupAuto && (d = d - s.slidesPerViewDynamic("previous", true) + 1, d = Math.max(d, 0))), n.rewind && s.isBeginning) {
      const p = s.params.virtual && s.params.virtual.enabled && s.virtual ? s.virtual.slides.length - 1 : s.slides.length - 1;
      return s.slideTo(p, e, i2, t);
    } else if (n.loop && s.activeIndex === 0 && n.cssMode) return requestAnimationFrame(() => {
      s.slideTo(d, e, i2, t);
    }), true;
    return s.slideTo(d, e, i2, t);
  }
  function gt(e, i2 = true, t) {
    const s = this;
    if (!s.destroyed) return typeof e > "u" && (e = s.params.speed), s.slideTo(s.activeIndex, e, i2, t);
  }
  function vt(e, i2 = true, t, s = 0.5) {
    const n = this;
    if (n.destroyed) return;
    typeof e > "u" && (e = n.params.speed);
    let r2 = n.activeIndex;
    const l2 = Math.min(n.params.slidesPerGroupSkip, r2), o = l2 + Math.floor((r2 - l2) / n.params.slidesPerGroup), a = n.rtlTranslate ? n.translate : -n.translate;
    if (a >= n.snapGrid[o]) {
      const u2 = n.snapGrid[o], g2 = n.snapGrid[o + 1];
      a - u2 > (g2 - u2) * s && (r2 += n.params.slidesPerGroup);
    } else {
      const u2 = n.snapGrid[o - 1], g2 = n.snapGrid[o];
      a - u2 <= (g2 - u2) * s && (r2 -= n.params.slidesPerGroup);
    }
    return r2 = Math.max(r2, 0), r2 = Math.min(r2, n.slidesGrid.length - 1), n.slideTo(r2, e, i2, t);
  }
  function St() {
    const e = this;
    if (e.destroyed) return;
    const { params: i2, slidesEl: t } = e, s = i2.slidesPerView === "auto" ? e.slidesPerViewDynamic() : i2.slidesPerView;
    let n = e.getSlideIndexWhenGrid(e.clickedIndex), r2;
    const l2 = e.isElement ? "swiper-slide" : `.${i2.slideClass}`, o = e.grid && e.params.grid && e.params.grid.rows > 1;
    if (i2.loop) {
      if (e.animating) return;
      r2 = parseInt(e.clickedSlide.getAttribute("data-swiper-slide-index"), 10), i2.centeredSlides ? e.slideToLoop(r2) : n > (o ? (e.slides.length - s) / 2 - (e.params.grid.rows - 1) : e.slides.length - s) ? (e.loopFix(), n = e.getSlideIndex(_(t, `${l2}[data-swiper-slide-index="${r2}"]`)[0]), be(() => {
        e.slideTo(n);
      })) : e.slideTo(n);
    } else e.slideTo(n);
  }
  var bt = { slideTo: ut, slideToLoop: pt, slideNext: mt, slidePrev: ht, slideReset: gt, slideToClosest: vt, slideToClickedSlide: St };
  function yt(e, i2) {
    const t = this, { params: s, slidesEl: n } = t;
    if (!s.loop || t.virtual && t.params.virtual.enabled) return;
    const r2 = () => {
      _(n, `.${s.slideClass}, swiper-slide`).forEach((m, v2) => {
        m.setAttribute("data-swiper-slide-index", v2);
      });
    }, l2 = () => {
      const c = _(n, `.${s.slideBlankClass}`);
      c.forEach((m) => {
        m.remove();
      }), c.length > 0 && (t.recalcSlides(), t.updateSlides());
    }, o = t.grid && s.grid && s.grid.rows > 1;
    s.loopAddBlankSlides && (s.slidesPerGroup > 1 || o) && l2();
    const a = s.slidesPerGroup * (o ? s.grid.rows : 1), u2 = t.slides.length % a !== 0, g2 = o && t.slides.length % s.grid.rows !== 0, f = (c) => {
      for (let m = 0; m < c; m += 1) {
        const v2 = t.isElement ? K("swiper-slide", [s.slideBlankClass]) : K("div", [s.slideClass, s.slideBlankClass]);
        t.slidesEl.append(v2);
      }
    };
    if (u2) {
      if (s.loopAddBlankSlides) {
        const c = a - t.slides.length % a;
        f(c), t.recalcSlides(), t.updateSlides();
      } else U("Swiper Loop Warning: The number of slides is not even to slidesPerGroup, loop mode may not function properly. You need to add more slides (or make duplicates, or empty slides)");
      r2();
    } else if (g2) {
      if (s.loopAddBlankSlides) {
        const c = s.grid.rows - t.slides.length % s.grid.rows;
        f(c), t.recalcSlides(), t.updateSlides();
      } else U("Swiper Loop Warning: The number of slides is not even to grid.rows, loop mode may not function properly. You need to add more slides (or make duplicates, or empty slides)");
      r2();
    } else r2();
    const S2 = s.centeredSlides || !!s.slidesOffsetBefore || !!s.slidesOffsetAfter;
    t.loopFix({ slideRealIndex: e, direction: S2 ? void 0 : "next", initial: i2 });
  }
  function Tt({ slideRealIndex: e, slideTo: i2 = true, direction: t, setTranslate: s, activeSlideIndex: n, initial: r2, byController: l2, byMousewheel: o } = {}) {
    const a = this;
    if (!a.params.loop) return;
    a.emit("beforeLoopFix");
    const { slides: u2, allowSlidePrev: g2, allowSlideNext: f, slidesEl: S2, params: c } = a, { centeredSlides: m, slidesOffsetBefore: v2, slidesOffsetAfter: w2, initialSlide: d } = c, p = m || !!v2 || !!w2;
    if (a.allowSlidePrev = true, a.allowSlideNext = true, a.virtual && c.virtual.enabled) {
      i2 && (!p && a.snapIndex === 0 ? a.slideTo(a.virtual.slides.length, 0, false, true) : p && a.snapIndex < c.slidesPerView ? a.slideTo(a.virtual.slides.length + a.snapIndex, 0, false, true) : a.snapIndex === a.snapGrid.length - 1 && a.slideTo(a.virtual.slidesBefore, 0, false, true)), a.allowSlidePrev = g2, a.allowSlideNext = f, a.emit("loopFix");
      return;
    }
    let h2 = c.slidesPerView;
    h2 === "auto" ? h2 = a.slidesPerViewDynamic() : (h2 = Math.ceil(parseFloat(c.slidesPerView, 10)), p && h2 % 2 === 0 && (h2 = h2 + 1));
    const b = c.slidesPerGroupAuto ? h2 : c.slidesPerGroup;
    let x = p ? Math.max(b, Math.ceil(h2 / 2)) : b;
    x % b !== 0 && (x += b - x % b), x += c.loopAdditionalSlides, a.loopedSlides = x;
    const P = a.grid && c.grid && c.grid.rows > 1;
    u2.length < h2 + x || a.params.effect === "cards" && u2.length < h2 + x * 2 ? U("Swiper Loop Warning: The number of slides is not enough for loop mode, it will be disabled or not function properly. You need to add more slides (or make duplicates) or lower the values of slidesPerView and slidesPerGroup parameters") : P && c.grid.fill === "row" && U("Swiper Loop Warning: Loop mode is not compatible with grid.fill = `row`");
    const M = [], y = [], k2 = P ? Math.ceil(u2.length / c.grid.rows) : u2.length, T = r2 && k2 - d < h2 && !p;
    let E3 = T ? d : a.activeIndex;
    typeof n > "u" ? n = a.getSlideIndex(u2.find((I2) => I2.classList.contains(c.slideActiveClass))) : E3 = n;
    const C = t === "next" || !t, L = t === "prev" || !t;
    let O3 = 0, G2 = 0;
    const R2 = (P ? u2[n].column : n) + (p && typeof s > "u" ? -h2 / 2 + 0.5 : 0);
    if (R2 < x) {
      O3 = Math.max(x - R2, b);
      for (let I2 = 0; I2 < x - R2; I2 += 1) {
        const z3 = I2 - Math.floor(I2 / k2) * k2;
        if (P) {
          const B2 = k2 - z3 - 1;
          for (let H2 = u2.length - 1; H2 >= 0; H2 -= 1) u2[H2].column === B2 && M.push(H2);
        } else M.push(k2 - z3 - 1);
      }
    } else if (R2 + h2 > k2 - x) {
      G2 = Math.max(R2 - (k2 - x * 2), b), T && (G2 = Math.max(G2, h2 - k2 + d + 1));
      for (let I2 = 0; I2 < G2; I2 += 1) {
        const z3 = I2 - Math.floor(I2 / k2) * k2;
        P ? u2.forEach((B2, H2) => {
          B2.column === z3 && y.push(H2);
        }) : y.push(z3);
      }
    }
    if (a.__preventObserver__ = true, requestAnimationFrame(() => {
      a.__preventObserver__ = false;
    }), a.params.effect === "cards" && u2.length < h2 + x * 2 && (y.includes(n) && y.splice(y.indexOf(n), 1), M.includes(n) && M.splice(M.indexOf(n), 1)), L && M.forEach((I2) => {
      u2[I2].swiperLoopMoveDOM = true, S2.prepend(u2[I2]), u2[I2].swiperLoopMoveDOM = false;
    }), C && y.forEach((I2) => {
      u2[I2].swiperLoopMoveDOM = true, S2.append(u2[I2]), u2[I2].swiperLoopMoveDOM = false;
    }), a.recalcSlides(), c.slidesPerView === "auto" ? a.updateSlides() : P && (M.length > 0 && L || y.length > 0 && C) && a.slides.forEach((I2, z3) => {
      a.grid.updateSlide(z3, I2, a.slides);
    }), c.watchSlidesProgress && a.updateSlidesOffset(), i2) {
      if (M.length > 0 && L) {
        if (typeof e > "u") {
          const I2 = a.slidesGrid[E3], B2 = a.slidesGrid[E3 + O3] - I2;
          o ? a.setTranslate(a.translate - B2) : (a.slideTo(E3 + Math.ceil(O3), 0, false, true), s && (a.touchEventsData.startTranslate = a.touchEventsData.startTranslate - B2, a.touchEventsData.currentTranslate = a.touchEventsData.currentTranslate - B2));
        } else if (s) {
          const I2 = P ? M.length / c.grid.rows : M.length;
          a.slideTo(a.activeIndex + I2, 0, false, true), a.touchEventsData.currentTranslate = a.translate;
        }
      } else if (y.length > 0 && C) if (typeof e > "u") {
        const I2 = a.slidesGrid[E3], B2 = a.slidesGrid[E3 - G2] - I2;
        o ? a.setTranslate(a.translate - B2) : (a.slideTo(E3 - G2, 0, false, true), s && (a.touchEventsData.startTranslate = a.touchEventsData.startTranslate - B2, a.touchEventsData.currentTranslate = a.touchEventsData.currentTranslate - B2));
      } else {
        const I2 = P ? y.length / c.grid.rows : y.length;
        a.slideTo(a.activeIndex - I2, 0, false, true);
      }
    }
    if (a.allowSlidePrev = g2, a.allowSlideNext = f, a.controller && a.controller.control && !l2) {
      const I2 = { slideRealIndex: e, direction: t, setTranslate: s, activeSlideIndex: n, byController: true };
      Array.isArray(a.controller.control) ? a.controller.control.forEach((z3) => {
        !z3.destroyed && z3.params.loop && z3.loopFix({ ...I2, slideTo: z3.params.slidesPerView === c.slidesPerView ? i2 : false });
      }) : a.controller.control instanceof a.constructor && a.controller.control.params.loop && a.controller.control.loopFix({ ...I2, slideTo: a.controller.control.params.slidesPerView === c.slidesPerView ? i2 : false });
    }
    a.emit("loopFix");
  }
  function xt() {
    const e = this, { params: i2, slidesEl: t } = e;
    if (!i2.loop || !t || e.virtual && e.params.virtual.enabled) return;
    e.recalcSlides();
    const s = [];
    e.slides.forEach((n) => {
      const r2 = typeof n.swiperSlideIndex > "u" ? n.getAttribute("data-swiper-slide-index") * 1 : n.swiperSlideIndex;
      s[r2] = n;
    }), e.slides.forEach((n) => {
      n.removeAttribute("data-swiper-slide-index");
    }), s.forEach((n) => {
      t.append(n);
    }), e.recalcSlides(), e.slideTo(e.realIndex, 0);
  }
  var wt = { loopCreate: yt, loopFix: Tt, loopDestroy: xt };
  function Et(e) {
    const i2 = this;
    if (!i2.params.simulateTouch || i2.params.watchOverflow && i2.isLocked || i2.params.cssMode) return;
    const t = i2.params.touchEventsTarget === "container" ? i2.el : i2.wrapperEl;
    i2.isElement && (i2.__preventObserver__ = true), t.style.cursor = "move", t.style.cursor = e ? "grabbing" : "grab", i2.isElement && requestAnimationFrame(() => {
      i2.__preventObserver__ = false;
    });
  }
  function Ct() {
    const e = this;
    e.params.watchOverflow && e.isLocked || e.params.cssMode || (e.isElement && (e.__preventObserver__ = true), e[e.params.touchEventsTarget === "container" ? "el" : "wrapperEl"].style.cursor = "", e.isElement && requestAnimationFrame(() => {
      e.__preventObserver__ = false;
    }));
  }
  var Mt = { setGrabCursor: Et, unsetGrabCursor: Ct };
  function Pt(e, i2 = this) {
    function t(s) {
      if (!s || s === F() || s === D()) return null;
      s.assignedSlot && (s = s.assignedSlot);
      const n = s.closest(e);
      return !n && !s.getRootNode ? null : n || t(s.getRootNode().host);
    }
    return t(i2);
  }
  function pe(e, i2, t) {
    const s = D(), { params: n } = e, r2 = n.edgeSwipeDetection, l2 = n.edgeSwipeThreshold;
    return r2 && (t <= l2 || t >= s.innerWidth - l2) ? r2 === "prevent" ? (i2.preventDefault(), true) : false : true;
  }
  function Lt(e) {
    const i2 = this, t = F();
    let s = e;
    s.originalEvent && (s = s.originalEvent);
    const n = i2.touchEventsData;
    if (s.type === "pointerdown") {
      if (n.pointerId !== null && n.pointerId !== s.pointerId) return;
      n.pointerId = s.pointerId;
    } else s.type === "touchstart" && s.targetTouches.length === 1 && (n.touchId = s.targetTouches[0].identifier);
    if (s.type === "touchstart") {
      pe(i2, s, s.targetTouches[0].pageX);
      return;
    }
    const { params: r2, touches: l2, enabled: o } = i2;
    if (!o || !r2.simulateTouch && s.pointerType === "mouse" || i2.animating && r2.preventInteractionOnTransition) return;
    !i2.animating && r2.cssMode && r2.loop && i2.loopFix();
    let a = s.target;
    if (r2.touchEventsTarget === "wrapper" && !Ge(a, i2.wrapperEl) || "which" in s && s.which === 3 || "button" in s && s.button > 0 || n.isTouched && n.isMoved) return;
    const u2 = !!r2.noSwipingClass && r2.noSwipingClass !== "", g2 = s.composedPath ? s.composedPath() : s.path;
    u2 && s.target && s.target.shadowRoot && g2 && (a = g2[0]);
    const f = r2.noSwipingSelector ? r2.noSwipingSelector : `.${r2.noSwipingClass}`, S2 = !!(s.target && s.target.shadowRoot);
    if (r2.noSwiping && (S2 ? Pt(f, a) : a.closest(f))) {
      i2.allowClick = true;
      return;
    }
    if (r2.swipeHandler && !a.closest(r2.swipeHandler)) return;
    l2.currentX = s.pageX, l2.currentY = s.pageY;
    const c = l2.currentX, m = l2.currentY;
    if (!pe(i2, s, c)) return;
    Object.assign(n, { isTouched: true, isMoved: false, allowTouchCallbacks: true, isScrolling: void 0, startMoving: void 0 }), l2.startX = c, l2.startY = m, n.touchStartTime = X(), i2.allowClick = true, i2.updateSize(), i2.swipeDirection = void 0, r2.threshold > 0 && (n.allowThresholdMove = false);
    let v2 = true;
    a.matches(n.focusableElements) && (v2 = false, a.nodeName === "SELECT" && (n.isTouched = false)), t.activeElement && t.activeElement.matches(n.focusableElements) && t.activeElement !== a && (s.pointerType === "mouse" || s.pointerType !== "mouse" && !a.matches(n.focusableElements)) && t.activeElement.blur();
    const w2 = v2 && i2.allowTouchMove && r2.touchStartPreventDefault;
    (r2.touchStartForcePreventDefault || w2) && !a.isContentEditable && s.preventDefault(), r2.freeMode && r2.freeMode.enabled && i2.freeMode && i2.animating && !r2.cssMode && i2.freeMode.onTouchStart(), i2.emit("touchStart", s);
  }
  function It(e) {
    const i2 = F(), t = this, s = t.touchEventsData, { params: n, touches: r2, rtlTranslate: l2, enabled: o } = t;
    if (!o || !n.simulateTouch && e.pointerType === "mouse") return;
    let a = e;
    if (a.originalEvent && (a = a.originalEvent), a.type === "pointermove" && (s.touchId !== null || a.pointerId !== s.pointerId)) return;
    let u2;
    if (a.type === "touchmove") {
      if (u2 = [...a.changedTouches].find((x) => x.identifier === s.touchId), !u2 || u2.identifier !== s.touchId) return;
    } else u2 = a;
    if (!s.isTouched) {
      s.startMoving && s.isScrolling && t.emit("touchMoveOpposite", a);
      return;
    }
    const g2 = u2.pageX, f = u2.pageY;
    if (a.preventedByNestedSwiper) {
      r2.startX = g2, r2.startY = f;
      return;
    }
    if (!t.allowTouchMove) {
      a.target.matches(s.focusableElements) || (t.allowClick = false), s.isTouched && (Object.assign(r2, { startX: g2, startY: f, currentX: g2, currentY: f }), s.touchStartTime = X());
      return;
    }
    if (n.touchReleaseOnEdges && !n.loop) if (t.isVertical()) {
      if (f < r2.startY && t.translate <= t.maxTranslate() || f > r2.startY && t.translate >= t.minTranslate()) {
        s.isTouched = false, s.isMoved = false;
        return;
      }
    } else {
      if (l2 && (g2 > r2.startX && -t.translate <= t.maxTranslate() || g2 < r2.startX && -t.translate >= t.minTranslate())) return;
      if (!l2 && (g2 < r2.startX && t.translate <= t.maxTranslate() || g2 > r2.startX && t.translate >= t.minTranslate())) return;
    }
    if (i2.activeElement && i2.activeElement.matches(s.focusableElements) && i2.activeElement !== a.target && a.pointerType !== "mouse" && i2.activeElement.blur(), i2.activeElement && a.target === i2.activeElement && a.target.matches(s.focusableElements)) {
      s.isMoved = true, t.allowClick = false;
      return;
    }
    s.allowTouchCallbacks && t.emit("touchMove", a), r2.previousX = r2.currentX, r2.previousY = r2.currentY, r2.currentX = g2, r2.currentY = f;
    const S2 = r2.currentX - r2.startX, c = r2.currentY - r2.startY;
    if (t.params.threshold && Math.sqrt(S2 ** 2 + c ** 2) < t.params.threshold) return;
    if (typeof s.isScrolling > "u") {
      let x;
      t.isHorizontal() && r2.currentY === r2.startY || t.isVertical() && r2.currentX === r2.startX ? s.isScrolling = false : S2 * S2 + c * c >= 25 && (x = Math.atan2(Math.abs(c), Math.abs(S2)) * 180 / Math.PI, s.isScrolling = t.isHorizontal() ? x > n.touchAngle : 90 - x > n.touchAngle);
    }
    if (s.isScrolling && t.emit("touchMoveOpposite", a), typeof s.startMoving > "u" && (r2.currentX !== r2.startX || r2.currentY !== r2.startY) && (s.startMoving = true), s.isScrolling || a.type === "touchmove" && s.preventTouchMoveFromPointerMove) {
      s.isTouched = false;
      return;
    }
    if (!s.startMoving) return;
    t.allowClick = false, !n.cssMode && a.cancelable && a.preventDefault(), n.touchMoveStopPropagation && !n.nested && a.stopPropagation();
    let m = t.isHorizontal() ? S2 : c, v2 = t.isHorizontal() ? r2.currentX - r2.previousX : r2.currentY - r2.previousY;
    n.oneWayMovement && (m = Math.abs(m) * (l2 ? 1 : -1), v2 = Math.abs(v2) * (l2 ? 1 : -1)), r2.diff = m, m *= n.touchRatio, l2 && (m = -m, v2 = -v2);
    const w2 = t.touchesDirection;
    t.swipeDirection = m > 0 ? "prev" : "next", t.touchesDirection = v2 > 0 ? "prev" : "next";
    const d = t.params.loop && !n.cssMode, p = t.touchesDirection === "next" && t.allowSlideNext || t.touchesDirection === "prev" && t.allowSlidePrev;
    if (!s.isMoved) {
      if (d && p && t.loopFix({ direction: t.swipeDirection }), s.startTranslate = t.getTranslate(), t.setTransition(0), t.animating) {
        const x = new window.CustomEvent("transitionend", { bubbles: true, cancelable: true, detail: { bySwiperTouchMove: true } });
        t.wrapperEl.dispatchEvent(x);
      }
      s.allowMomentumBounce = false, n.grabCursor && (t.allowSlideNext === true || t.allowSlidePrev === true) && t.setGrabCursor(true), t.emit("sliderFirstMove", a);
    }
    if ((/* @__PURE__ */ new Date()).getTime(), n._loopSwapReset !== false && s.isMoved && s.allowThresholdMove && w2 !== t.touchesDirection && d && p && Math.abs(m) >= 1) {
      Object.assign(r2, { startX: g2, startY: f, currentX: g2, currentY: f, startTranslate: s.currentTranslate }), s.loopSwapReset = true, s.startTranslate = s.currentTranslate;
      return;
    }
    t.emit("sliderMove", a), s.isMoved = true, s.currentTranslate = m + s.startTranslate;
    let h2 = true, b = n.resistanceRatio;
    if (n.touchReleaseOnEdges && (b = 0), m > 0 ? (d && p && s.allowThresholdMove && s.currentTranslate > (n.centeredSlides ? t.minTranslate() - t.slidesSizesGrid[t.activeIndex + 1] - (n.slidesPerView !== "auto" && t.slides.length - n.slidesPerView >= 2 ? t.slidesSizesGrid[t.activeIndex + 1] + t.params.spaceBetween : 0) - t.params.spaceBetween : t.minTranslate()) && t.loopFix({ direction: "prev", setTranslate: true, activeSlideIndex: 0 }), s.currentTranslate > t.minTranslate() && (h2 = false, n.resistance && (s.currentTranslate = t.minTranslate() - 1 + (-t.minTranslate() + s.startTranslate + m) ** b))) : m < 0 && (d && p && s.allowThresholdMove && s.currentTranslate < (n.centeredSlides ? t.maxTranslate() + t.slidesSizesGrid[t.slidesSizesGrid.length - 1] + t.params.spaceBetween + (n.slidesPerView !== "auto" && t.slides.length - n.slidesPerView >= 2 ? t.slidesSizesGrid[t.slidesSizesGrid.length - 1] + t.params.spaceBetween : 0) : t.maxTranslate()) && t.loopFix({ direction: "next", setTranslate: true, activeSlideIndex: t.slides.length - (n.slidesPerView === "auto" ? t.slidesPerViewDynamic() : Math.ceil(parseFloat(n.slidesPerView, 10))) }), s.currentTranslate < t.maxTranslate() && (h2 = false, n.resistance && (s.currentTranslate = t.maxTranslate() + 1 - (t.maxTranslate() - s.startTranslate - m) ** b))), h2 && (a.preventedByNestedSwiper = true), !t.allowSlideNext && t.swipeDirection === "next" && s.currentTranslate < s.startTranslate && (s.currentTranslate = s.startTranslate), !t.allowSlidePrev && t.swipeDirection === "prev" && s.currentTranslate > s.startTranslate && (s.currentTranslate = s.startTranslate), !t.allowSlidePrev && !t.allowSlideNext && (s.currentTranslate = s.startTranslate), n.threshold > 0) if (Math.abs(m) > n.threshold || s.allowThresholdMove) {
      if (!s.allowThresholdMove) {
        s.allowThresholdMove = true, r2.startX = r2.currentX, r2.startY = r2.currentY, s.currentTranslate = s.startTranslate, r2.diff = t.isHorizontal() ? r2.currentX - r2.startX : r2.currentY - r2.startY;
        return;
      }
    } else {
      s.currentTranslate = s.startTranslate;
      return;
    }
    !n.followFinger || n.cssMode || ((n.freeMode && n.freeMode.enabled && t.freeMode || n.watchSlidesProgress) && (t.updateActiveIndex(), t.updateSlidesClasses()), n.freeMode && n.freeMode.enabled && t.freeMode && t.freeMode.onTouchMove(), t.updateProgress(s.currentTranslate), t.setTranslate(s.currentTranslate));
  }
  function kt(e) {
    const i2 = this, t = i2.touchEventsData;
    let s = e;
    s.originalEvent && (s = s.originalEvent);
    let n;
    if (s.type === "touchend" || s.type === "touchcancel") {
      if (n = [...s.changedTouches].find((x) => x.identifier === t.touchId), !n || n.identifier !== t.touchId) return;
    } else {
      if (t.touchId !== null || s.pointerId !== t.pointerId) return;
      n = s;
    }
    if (["pointercancel", "pointerout", "pointerleave", "contextmenu"].includes(s.type) && !(["pointercancel", "contextmenu"].includes(s.type) && (i2.browser.isSafari || i2.browser.isWebView))) return;
    t.pointerId = null, t.touchId = null;
    const { params: l2, touches: o, rtlTranslate: a, slidesGrid: u2, enabled: g2 } = i2;
    if (!g2 || !l2.simulateTouch && s.pointerType === "mouse") return;
    if (t.allowTouchCallbacks && i2.emit("touchEnd", s), t.allowTouchCallbacks = false, !t.isTouched) {
      t.isMoved && l2.grabCursor && i2.setGrabCursor(false), t.isMoved = false, t.startMoving = false;
      return;
    }
    l2.grabCursor && t.isMoved && t.isTouched && (i2.allowSlideNext === true || i2.allowSlidePrev === true) && i2.setGrabCursor(false);
    const f = X(), S2 = f - t.touchStartTime;
    if (i2.allowClick) {
      const x = s.path || s.composedPath && s.composedPath();
      i2.updateClickedSlide(x && x[0] || s.target, x), i2.emit("tap click", s), S2 < 300 && f - t.lastClickTime < 300 && i2.emit("doubleTap doubleClick", s);
    }
    if (t.lastClickTime = X(), be(() => {
      i2.destroyed || (i2.allowClick = true);
    }), !t.isTouched || !t.isMoved || !i2.swipeDirection || o.diff === 0 && !t.loopSwapReset || t.currentTranslate === t.startTranslate && !t.loopSwapReset) {
      t.isTouched = false, t.isMoved = false, t.startMoving = false;
      return;
    }
    t.isTouched = false, t.isMoved = false, t.startMoving = false;
    let c;
    if (l2.followFinger ? c = a ? i2.translate : -i2.translate : c = -t.currentTranslate, l2.cssMode) return;
    if (l2.freeMode && l2.freeMode.enabled) {
      i2.freeMode.onTouchEnd({ currentPos: c });
      return;
    }
    const m = c >= -i2.maxTranslate() && !i2.params.loop;
    let v2 = 0, w2 = i2.slidesSizesGrid[0];
    for (let x = 0; x < u2.length; x += x < l2.slidesPerGroupSkip ? 1 : l2.slidesPerGroup) {
      const P = x < l2.slidesPerGroupSkip - 1 ? 1 : l2.slidesPerGroup;
      typeof u2[x + P] < "u" ? (m || c >= u2[x] && c < u2[x + P]) && (v2 = x, w2 = u2[x + P] - u2[x]) : (m || c >= u2[x]) && (v2 = x, w2 = u2[u2.length - 1] - u2[u2.length - 2]);
    }
    let d = null, p = null;
    l2.rewind && (i2.isBeginning ? p = l2.virtual && l2.virtual.enabled && i2.virtual ? i2.virtual.slides.length - 1 : i2.slides.length - 1 : i2.isEnd && (d = 0));
    const h2 = (c - u2[v2]) / w2, b = v2 < l2.slidesPerGroupSkip - 1 ? 1 : l2.slidesPerGroup;
    if (S2 > l2.longSwipesMs) {
      if (!l2.longSwipes) {
        i2.slideTo(i2.activeIndex);
        return;
      }
      i2.swipeDirection === "next" && (h2 >= l2.longSwipesRatio ? i2.slideTo(l2.rewind && i2.isEnd ? d : v2 + b) : i2.slideTo(v2)), i2.swipeDirection === "prev" && (h2 > 1 - l2.longSwipesRatio ? i2.slideTo(v2 + b) : p !== null && h2 < 0 && Math.abs(h2) > l2.longSwipesRatio ? i2.slideTo(p) : i2.slideTo(v2));
    } else {
      if (!l2.shortSwipes) {
        i2.slideTo(i2.activeIndex);
        return;
      }
      i2.navigation && (s.target === i2.navigation.nextEl || s.target === i2.navigation.prevEl) ? s.target === i2.navigation.nextEl ? i2.slideTo(v2 + b) : i2.slideTo(v2) : (i2.swipeDirection === "next" && i2.slideTo(d !== null ? d : v2 + b), i2.swipeDirection === "prev" && i2.slideTo(p !== null ? p : v2));
    }
  }
  function me() {
    const e = this, { params: i2, el: t } = e;
    if (t && t.offsetWidth === 0) return;
    i2.breakpoints && e.setBreakpoint();
    const { allowSlideNext: s, allowSlidePrev: n, snapGrid: r2 } = e, l2 = e.virtual && e.params.virtual.enabled;
    e.allowSlideNext = true, e.allowSlidePrev = true, e.updateSize(), e.updateSlides(), e.updateSlidesClasses();
    const o = l2 && i2.loop;
    (i2.slidesPerView === "auto" || i2.slidesPerView > 1) && e.isEnd && !e.isBeginning && !e.params.centeredSlides && !o ? e.slideTo(e.slides.length - 1, 0, false, true) : e.params.loop && !l2 ? e.slideToLoop(e.realIndex, 0, false, true) : e.slideTo(e.activeIndex, 0, false, true), e.autoplay && e.autoplay.running && e.autoplay.paused && (clearTimeout(e.autoplay.resizeTimeout), e.autoplay.resizeTimeout = setTimeout(() => {
      e.autoplay && e.autoplay.running && e.autoplay.paused && e.autoplay.resume();
    }, 500)), e.allowSlidePrev = n, e.allowSlideNext = s, e.params.watchOverflow && r2 !== e.snapGrid && e.checkOverflow();
  }
  function Ot(e) {
    const i2 = this;
    i2.enabled && (i2.allowClick || (i2.params.preventClicks && e.preventDefault(), i2.params.preventClicksPropagation && i2.animating && (e.stopPropagation(), e.stopImmediatePropagation())));
  }
  function At() {
    const e = this, { wrapperEl: i2, rtlTranslate: t, enabled: s } = e;
    if (!s) return;
    e.previousTranslate = e.translate, e.isHorizontal() ? e.translate = -i2.scrollLeft : e.translate = -i2.scrollTop, e.translate === 0 && (e.translate = 0), e.updateActiveIndex(), e.updateSlidesClasses();
    let n;
    const r2 = e.maxTranslate() - e.minTranslate();
    r2 === 0 ? n = 0 : n = (e.translate - e.minTranslate()) / r2, n !== e.progress && e.updateProgress(t ? -e.translate : e.translate), e.emit("setTranslate", e.translate, false);
  }
  function zt(e) {
    const i2 = this;
    Y(i2, e.target), !(i2.params.cssMode || i2.params.slidesPerView !== "auto" && !i2.params.autoHeight) && i2.update();
  }
  function Dt() {
    const e = this;
    e.documentTouchHandlerProceeded || (e.documentTouchHandlerProceeded = true, e.params.touchReleaseOnEdges && (e.el.style.touchAction = "auto"));
  }
  var Ce = (e, i2) => {
    const t = F(), { params: s, el: n, wrapperEl: r2, device: l2 } = e, o = !!s.nested, a = i2 === "on" ? "addEventListener" : "removeEventListener", u2 = i2;
    !n || typeof n == "string" || (t[a]("touchstart", e.onDocumentTouchStart, { passive: false, capture: o }), n[a]("touchstart", e.onTouchStart, { passive: false }), n[a]("pointerdown", e.onTouchStart, { passive: false }), t[a]("touchmove", e.onTouchMove, { passive: false, capture: o }), t[a]("pointermove", e.onTouchMove, { passive: false, capture: o }), t[a]("touchend", e.onTouchEnd, { passive: true }), t[a]("pointerup", e.onTouchEnd, { passive: true }), t[a]("pointercancel", e.onTouchEnd, { passive: true }), t[a]("touchcancel", e.onTouchEnd, { passive: true }), t[a]("pointerout", e.onTouchEnd, { passive: true }), t[a]("pointerleave", e.onTouchEnd, { passive: true }), t[a]("contextmenu", e.onTouchEnd, { passive: true }), (s.preventClicks || s.preventClicksPropagation) && n[a]("click", e.onClick, true), s.cssMode && r2[a]("scroll", e.onScroll), s.updateOnWindowResize ? e[u2](l2.ios || l2.android ? "resize orientationchange observerUpdate" : "resize observerUpdate", me, true) : e[u2]("observerUpdate", me, true), n[a]("load", e.onLoad, { capture: true }));
  };
  function Gt() {
    const e = this, { params: i2 } = e;
    e.onTouchStart = Lt.bind(e), e.onTouchMove = It.bind(e), e.onTouchEnd = kt.bind(e), e.onDocumentTouchStart = Dt.bind(e), i2.cssMode && (e.onScroll = At.bind(e)), e.onClick = Ot.bind(e), e.onLoad = zt.bind(e), Ce(e, "on");
  }
  function Bt() {
    Ce(this, "off");
  }
  var Vt = { attachEvents: Gt, detachEvents: Bt };
  var he = (e, i2) => e.grid && i2.grid && i2.grid.rows > 1;
  function $t() {
    const e = this, { realIndex: i2, initialized: t, params: s, el: n } = e, r2 = s.breakpoints;
    if (!r2 || r2 && Object.keys(r2).length === 0) return;
    const l2 = F(), o = s.breakpointsBase === "window" || !s.breakpointsBase ? s.breakpointsBase : "container", a = ["window", "container"].includes(s.breakpointsBase) || !s.breakpointsBase ? e.el : l2.querySelector(s.breakpointsBase), u2 = e.getBreakpoint(r2, o, a);
    if (!u2 || e.currentBreakpoint === u2) return;
    const f = (u2 in r2 ? r2[u2] : void 0) || e.originalParams, S2 = he(e, s), c = he(e, f), m = e.params.grabCursor, v2 = f.grabCursor, w2 = s.enabled;
    S2 && !c ? (n.classList.remove(`${s.containerModifierClass}grid`, `${s.containerModifierClass}grid-column`), e.emitContainerClasses()) : !S2 && c && (n.classList.add(`${s.containerModifierClass}grid`), (f.grid.fill && f.grid.fill === "column" || !f.grid.fill && s.grid.fill === "column") && n.classList.add(`${s.containerModifierClass}grid-column`), e.emitContainerClasses()), m && !v2 ? e.unsetGrabCursor() : !m && v2 && e.setGrabCursor(), ["navigation", "pagination", "scrollbar"].forEach((P) => {
      if (typeof f[P] > "u") return;
      const M = s[P] && s[P].enabled, y = f[P] && f[P].enabled;
      M && !y && e[P].disable(), !M && y && e[P].enable();
    });
    const d = f.direction && f.direction !== s.direction, p = s.loop && (f.slidesPerView !== s.slidesPerView || d), h2 = s.loop;
    d && t && e.changeDirection(), V(e.params, f);
    const b = e.params.enabled, x = e.params.loop;
    Object.assign(e, { allowTouchMove: e.params.allowTouchMove, allowSlideNext: e.params.allowSlideNext, allowSlidePrev: e.params.allowSlidePrev }), w2 && !b ? e.disable() : !w2 && b && e.enable(), e.currentBreakpoint = u2, e.emit("_beforeBreakpoint", f), t && (p ? (e.loopDestroy(), e.loopCreate(i2), e.updateSlides()) : !h2 && x ? (e.loopCreate(i2), e.updateSlides()) : h2 && !x && e.loopDestroy()), e.emit("breakpoint", f);
  }
  function _t(e, i2 = "window", t) {
    if (!e || i2 === "container" && !t) return;
    let s = false;
    const n = D(), r2 = i2 === "window" ? n.innerHeight : t.clientHeight, l2 = Object.keys(e).map((o) => {
      if (typeof o == "string" && o.indexOf("@") === 0) {
        const a = parseFloat(o.substr(1));
        return { value: r2 * a, point: o };
      }
      return { value: o, point: o };
    });
    l2.sort((o, a) => parseInt(o.value, 10) - parseInt(a.value, 10));
    for (let o = 0; o < l2.length; o += 1) {
      const { point: a, value: u2 } = l2[o];
      i2 === "window" ? n.matchMedia(`(min-width: ${u2}px)`).matches && (s = a) : u2 <= t.clientWidth && (s = a);
    }
    return s || "max";
  }
  var Ft = { setBreakpoint: $t, getBreakpoint: _t };
  function Ht(e, i2) {
    const t = [];
    return e.forEach((s) => {
      typeof s == "object" ? Object.keys(s).forEach((n) => {
        s[n] && t.push(i2 + n);
      }) : typeof s == "string" && t.push(i2 + s);
    }), t;
  }
  function Nt() {
    const e = this, { classNames: i2, params: t, rtl: s, el: n, device: r2 } = e, l2 = Ht(["initialized", t.direction, { "free-mode": e.params.freeMode && t.freeMode.enabled }, { autoheight: t.autoHeight }, { rtl: s }, { grid: t.grid && t.grid.rows > 1 }, { "grid-column": t.grid && t.grid.rows > 1 && t.grid.fill === "column" }, { android: r2.android }, { ios: r2.ios }, { "css-mode": t.cssMode }, { centered: t.cssMode && t.centeredSlides }, { "watch-progress": t.watchSlidesProgress }], t.containerModifierClass);
    i2.push(...l2), n.classList.add(...i2), e.emitContainerClasses();
  }
  function Rt() {
    const e = this, { el: i2, classNames: t } = e;
    !i2 || typeof i2 == "string" || (i2.classList.remove(...t), e.emitContainerClasses());
  }
  var Wt = { addClasses: Nt, removeClasses: Rt };
  function jt() {
    const e = this, { isLocked: i2, params: t } = e, { slidesOffsetBefore: s } = t;
    if (s) {
      const n = e.slides.length - 1, r2 = e.slidesGrid[n] + e.slidesSizesGrid[n] + s * 2;
      e.isLocked = e.size > r2;
    } else e.isLocked = e.snapGrid.length === 1;
    t.allowSlideNext === true && (e.allowSlideNext = !e.isLocked), t.allowSlidePrev === true && (e.allowSlidePrev = !e.isLocked), i2 && i2 !== e.isLocked && (e.isEnd = false), i2 !== e.isLocked && e.emit(e.isLocked ? "lock" : "unlock");
  }
  var qt = { checkOverflow: jt };
  var ge = { init: true, direction: "horizontal", oneWayMovement: false, swiperElementNodeName: "SWIPER-CONTAINER", touchEventsTarget: "wrapper", initialSlide: 0, speed: 300, cssMode: false, updateOnWindowResize: true, resizeObserver: true, nested: false, createElements: false, eventsPrefix: "swiper", enabled: true, focusableElements: "input, select, option, textarea, button, video, label", width: null, height: null, preventInteractionOnTransition: false, userAgent: null, url: null, edgeSwipeDetection: false, edgeSwipeThreshold: 20, autoHeight: false, setWrapperSize: false, virtualTranslate: false, effect: "slide", breakpoints: void 0, breakpointsBase: "window", spaceBetween: 0, slidesPerView: 1, slidesPerGroup: 1, slidesPerGroupSkip: 0, slidesPerGroupAuto: false, centeredSlides: false, centeredSlidesBounds: false, slidesOffsetBefore: 0, slidesOffsetAfter: 0, normalizeSlideIndex: true, centerInsufficientSlides: false, watchOverflow: true, roundLengths: false, touchRatio: 1, touchAngle: 45, simulateTouch: true, shortSwipes: true, longSwipes: true, longSwipesRatio: 0.5, longSwipesMs: 300, followFinger: true, allowTouchMove: true, threshold: 5, touchMoveStopPropagation: false, touchStartPreventDefault: true, touchStartForcePreventDefault: false, touchReleaseOnEdges: false, uniqueNavElements: true, resistance: true, resistanceRatio: 0.85, watchSlidesProgress: false, grabCursor: false, preventClicks: true, preventClicksPropagation: true, slideToClickedSlide: false, loop: false, loopAddBlankSlides: true, loopAdditionalSlides: 0, loopPreventsSliding: true, rewind: false, allowSlidePrev: true, allowSlideNext: true, swipeHandler: null, noSwiping: true, noSwipingClass: "swiper-no-swiping", noSwipingSelector: null, passiveListeners: true, maxBackfaceHiddenSlides: 10, containerModifierClass: "swiper-", slideClass: "swiper-slide", slideBlankClass: "swiper-slide-blank", slideActiveClass: "swiper-slide-active", slideVisibleClass: "swiper-slide-visible", slideFullyVisibleClass: "swiper-slide-fully-visible", slideNextClass: "swiper-slide-next", slidePrevClass: "swiper-slide-prev", wrapperClass: "swiper-wrapper", lazyPreloaderClass: "swiper-lazy-preloader", lazyPreloadPrevNext: 0, runCallbacksOnInit: true, _emitClasses: false };
  function Yt(e, i2) {
    return function(s = {}) {
      const n = Object.keys(s)[0], r2 = s[n];
      if (typeof r2 != "object" || r2 === null) {
        V(i2, s);
        return;
      }
      if (e[n] === true && (e[n] = { enabled: true }), n === "navigation" && e[n] && e[n].enabled && !e[n].prevEl && !e[n].nextEl && (e[n].auto = true), ["pagination", "scrollbar"].indexOf(n) >= 0 && e[n] && e[n].enabled && !e[n].el && (e[n].auto = true), !(n in e && "enabled" in r2)) {
        V(i2, s);
        return;
      }
      typeof e[n] == "object" && !("enabled" in e[n]) && (e[n].enabled = true), e[n] || (e[n] = { enabled: false }), V(i2, s);
    };
  }
  var ne = { eventsEmitter: We, update: tt, translate: lt, transition: ft, slide: bt, loop: wt, grabCursor: Mt, events: Vt, breakpoints: Ft, checkOverflow: qt, classes: Wt };
  var re = {};
  var $ = class _$ {
    constructor(...i2) {
      let t, s;
      i2.length === 1 && i2[0].constructor && Object.prototype.toString.call(i2[0]).slice(8, -1) === "Object" ? s = i2[0] : [t, s] = i2, s || (s = {}), s = V({}, s), t && !s.el && (s.el = t);
      const n = F();
      if (s.el && typeof s.el == "string" && n.querySelectorAll(s.el).length > 1) {
        const a = [];
        return n.querySelectorAll(s.el).forEach((u2) => {
          const g2 = V({}, s, { el: u2 });
          a.push(new _$(g2));
        }), a;
      }
      const r2 = this;
      r2.__swiper__ = true, r2.support = Te(), r2.device = xe({ userAgent: s.userAgent }), r2.browser = we(), r2.eventsListeners = {}, r2.eventsAnyListeners = [], r2.modules = [...r2.__modules__], s.modules && Array.isArray(s.modules) && r2.modules.push(...s.modules);
      const l2 = {};
      r2.modules.forEach((a) => {
        a({ params: s, swiper: r2, extendParams: Yt(s, l2), on: r2.on.bind(r2), once: r2.once.bind(r2), off: r2.off.bind(r2), emit: r2.emit.bind(r2) });
      });
      const o = V({}, ge, l2);
      return r2.params = V({}, o, re, s), r2.originalParams = V({}, r2.params), r2.passedParams = V({}, s), r2.params && r2.params.on && Object.keys(r2.params.on).forEach((a) => {
        r2.on(a, r2.params.on[a]);
      }), r2.params && r2.params.onAny && r2.onAny(r2.params.onAny), Object.assign(r2, { enabled: r2.params.enabled, el: t, classNames: [], slides: [], slidesGrid: [], snapGrid: [], slidesSizesGrid: [], isHorizontal() {
        return r2.params.direction === "horizontal";
      }, isVertical() {
        return r2.params.direction === "vertical";
      }, activeIndex: 0, realIndex: 0, isBeginning: true, isEnd: false, translate: 0, previousTranslate: 0, progress: 0, velocity: 0, animating: false, cssOverflowAdjustment() {
        return Math.trunc(this.translate / 2 ** 23) * 2 ** 23;
      }, allowSlideNext: r2.params.allowSlideNext, allowSlidePrev: r2.params.allowSlidePrev, touchEventsData: { isTouched: void 0, isMoved: void 0, allowTouchCallbacks: void 0, touchStartTime: void 0, isScrolling: void 0, currentTranslate: void 0, startTranslate: void 0, allowThresholdMove: void 0, focusableElements: r2.params.focusableElements, lastClickTime: 0, clickTimeout: void 0, velocities: [], allowMomentumBounce: void 0, startMoving: void 0, pointerId: null, touchId: null }, allowClick: true, allowTouchMove: r2.params.allowTouchMove, touches: { startX: 0, startY: 0, currentX: 0, currentY: 0, diff: 0 }, imagesToLoad: [], imagesLoaded: 0 }), r2.emit("_swiper"), r2.params.init && r2.init(), r2;
    }
    getDirectionLabel(i2) {
      return this.isHorizontal() ? i2 : { width: "height", "margin-top": "margin-left", "margin-bottom ": "margin-right", "margin-left": "margin-top", "margin-right": "margin-bottom", "padding-left": "padding-top", "padding-right": "padding-bottom", marginRight: "marginBottom" }[i2];
    }
    getSlideIndex(i2) {
      const { slidesEl: t, params: s } = this, n = _(t, `.${s.slideClass}, swiper-slide`), r2 = Q(n[0]);
      return Q(i2) - r2;
    }
    getSlideIndexByData(i2) {
      return this.getSlideIndex(this.slides.find((t) => t.getAttribute("data-swiper-slide-index") * 1 === i2));
    }
    getSlideIndexWhenGrid(i2) {
      return this.grid && this.params.grid && this.params.grid.rows > 1 && (this.params.grid.fill === "column" ? i2 = Math.floor(i2 / this.params.grid.rows) : this.params.grid.fill === "row" && (i2 = i2 % Math.ceil(this.slides.length / this.params.grid.rows))), i2;
    }
    recalcSlides() {
      const i2 = this, { slidesEl: t, params: s } = i2;
      i2.slides = _(t, `.${s.slideClass}, swiper-slide`);
    }
    enable() {
      const i2 = this;
      i2.enabled || (i2.enabled = true, i2.params.grabCursor && i2.setGrabCursor(), i2.emit("enable"));
    }
    disable() {
      const i2 = this;
      i2.enabled && (i2.enabled = false, i2.params.grabCursor && i2.unsetGrabCursor(), i2.emit("disable"));
    }
    setProgress(i2, t) {
      const s = this;
      i2 = Math.min(Math.max(i2, 0), 1);
      const n = s.minTranslate(), l2 = (s.maxTranslate() - n) * i2 + n;
      s.translateTo(l2, typeof t > "u" ? 0 : t), s.updateActiveIndex(), s.updateSlidesClasses();
    }
    emitContainerClasses() {
      const i2 = this;
      if (!i2.params._emitClasses || !i2.el) return;
      const t = i2.el.className.split(" ").filter((s) => s.indexOf("swiper") === 0 || s.indexOf(i2.params.containerModifierClass) === 0);
      i2.emit("_containerClasses", t.join(" "));
    }
    getSlideClasses(i2) {
      const t = this;
      return t.destroyed ? "" : i2.className.split(" ").filter((s) => s.indexOf("swiper-slide") === 0 || s.indexOf(t.params.slideClass) === 0).join(" ");
    }
    emitSlidesClasses() {
      const i2 = this;
      if (!i2.params._emitClasses || !i2.el) return;
      const t = [];
      i2.slides.forEach((s) => {
        const n = i2.getSlideClasses(s);
        t.push({ slideEl: s, classNames: n }), i2.emit("_slideClass", s, n);
      }), i2.emit("_slideClasses", t);
    }
    slidesPerViewDynamic(i2 = "current", t = false) {
      const s = this, { params: n, slides: r2, slidesGrid: l2, slidesSizesGrid: o, size: a, activeIndex: u2 } = s;
      let g2 = 1;
      if (typeof n.slidesPerView == "number") return n.slidesPerView;
      if (n.centeredSlides) {
        let f = r2[u2] ? Math.ceil(r2[u2].swiperSlideSize) : 0, S2;
        for (let c = u2 + 1; c < r2.length; c += 1) r2[c] && !S2 && (f += Math.ceil(r2[c].swiperSlideSize), g2 += 1, f > a && (S2 = true));
        for (let c = u2 - 1; c >= 0; c -= 1) r2[c] && !S2 && (f += r2[c].swiperSlideSize, g2 += 1, f > a && (S2 = true));
      } else if (i2 === "current") for (let f = u2 + 1; f < r2.length; f += 1) (t ? l2[f] + o[f] - l2[u2] < a : l2[f] - l2[u2] < a) && (g2 += 1);
      else for (let f = u2 - 1; f >= 0; f -= 1) l2[u2] - l2[f] < a && (g2 += 1);
      return g2;
    }
    update() {
      const i2 = this;
      if (!i2 || i2.destroyed) return;
      const { snapGrid: t, params: s } = i2;
      s.breakpoints && i2.setBreakpoint(), [...i2.el.querySelectorAll('[loading="lazy"]')].forEach((l2) => {
        l2.complete && Y(i2, l2);
      }), i2.updateSize(), i2.updateSlides(), i2.updateProgress(), i2.updateSlidesClasses();
      function n() {
        const l2 = i2.rtlTranslate ? i2.translate * -1 : i2.translate, o = Math.min(Math.max(l2, i2.maxTranslate()), i2.minTranslate());
        i2.setTranslate(o), i2.updateActiveIndex(), i2.updateSlidesClasses();
      }
      let r2;
      if (s.freeMode && s.freeMode.enabled && !s.cssMode) n(), s.autoHeight && i2.updateAutoHeight();
      else {
        if ((s.slidesPerView === "auto" || s.slidesPerView > 1) && i2.isEnd && !s.centeredSlides) {
          const l2 = i2.virtual && s.virtual.enabled ? i2.virtual.slides : i2.slides;
          r2 = i2.slideTo(l2.length - 1, 0, false, true);
        } else r2 = i2.slideTo(i2.activeIndex, 0, false, true);
        r2 || n();
      }
      s.watchOverflow && t !== i2.snapGrid && i2.checkOverflow(), i2.emit("update");
    }
    changeDirection(i2, t = true) {
      const s = this, n = s.params.direction;
      return i2 || (i2 = n === "horizontal" ? "vertical" : "horizontal"), i2 === n || i2 !== "horizontal" && i2 !== "vertical" || (s.el.classList.remove(`${s.params.containerModifierClass}${n}`), s.el.classList.add(`${s.params.containerModifierClass}${i2}`), s.emitContainerClasses(), s.params.direction = i2, s.slides.forEach((r2) => {
        i2 === "vertical" ? r2.style.width = "" : r2.style.height = "";
      }), s.emit("changeDirection"), t && s.update()), s;
    }
    changeLanguageDirection(i2) {
      const t = this;
      t.rtl && i2 === "rtl" || !t.rtl && i2 === "ltr" || (t.rtl = i2 === "rtl", t.rtlTranslate = t.params.direction === "horizontal" && t.rtl, t.rtl ? (t.el.classList.add(`${t.params.containerModifierClass}rtl`), t.el.dir = "rtl") : (t.el.classList.remove(`${t.params.containerModifierClass}rtl`), t.el.dir = "ltr"), t.update());
    }
    mount(i2) {
      const t = this;
      if (t.mounted) return true;
      let s = i2 || t.params.el;
      if (typeof s == "string" && (s = document.querySelector(s)), !s) return false;
      s.swiper = t, s.parentNode && s.parentNode.host && s.parentNode.host.nodeName === t.params.swiperElementNodeName.toUpperCase() && (t.isElement = true);
      const n = () => `.${(t.params.wrapperClass || "").trim().split(" ").join(".")}`;
      let l2 = s && s.shadowRoot && s.shadowRoot.querySelector ? s.shadowRoot.querySelector(n()) : _(s, n())[0];
      return !l2 && t.params.createElements && (l2 = K("div", t.params.wrapperClass), s.append(l2), _(s, `.${t.params.slideClass}`).forEach((o) => {
        l2.append(o);
      })), Object.assign(t, { el: s, wrapperEl: l2, slidesEl: t.isElement && !s.parentNode.host.slideSlots ? s.parentNode.host : l2, hostEl: t.isElement ? s.parentNode.host : s, mounted: true, rtl: s.dir.toLowerCase() === "rtl" || N(s, "direction") === "rtl", rtlTranslate: t.params.direction === "horizontal" && (s.dir.toLowerCase() === "rtl" || N(s, "direction") === "rtl"), wrongRTL: N(l2, "display") === "-webkit-box" }), true;
    }
    init(i2) {
      const t = this;
      if (t.initialized || t.mount(i2) === false) return t;
      t.emit("beforeInit"), t.params.breakpoints && t.setBreakpoint(), t.addClasses(), t.updateSize(), t.updateSlides(), t.params.watchOverflow && t.checkOverflow(), t.params.grabCursor && t.enabled && t.setGrabCursor(), t.params.loop && t.virtual && t.params.virtual.enabled ? t.slideTo(t.params.initialSlide + t.virtual.slidesBefore, 0, t.params.runCallbacksOnInit, false, true) : t.slideTo(t.params.initialSlide, 0, t.params.runCallbacksOnInit, false, true), t.params.loop && t.loopCreate(void 0, true), t.attachEvents();
      const n = [...t.el.querySelectorAll('[loading="lazy"]')];
      return t.isElement && n.push(...t.hostEl.querySelectorAll('[loading="lazy"]')), n.forEach((r2) => {
        r2.complete ? Y(t, r2) : r2.addEventListener("load", (l2) => {
          Y(t, l2.target);
        });
      }), oe(t), t.initialized = true, oe(t), t.emit("init"), t.emit("afterInit"), t;
    }
    destroy(i2 = true, t = true) {
      const s = this, { params: n, el: r2, wrapperEl: l2, slides: o } = s;
      return typeof s.params > "u" || s.destroyed || (s.emit("beforeDestroy"), s.initialized = false, s.detachEvents(), n.loop && s.loopDestroy(), t && (s.removeClasses(), r2 && typeof r2 != "string" && r2.removeAttribute("style"), l2 && l2.removeAttribute("style"), o && o.length && o.forEach((a) => {
        a.classList.remove(n.slideVisibleClass, n.slideFullyVisibleClass, n.slideActiveClass, n.slideNextClass, n.slidePrevClass), a.removeAttribute("style"), a.removeAttribute("data-swiper-slide-index");
      })), s.emit("destroy"), Object.keys(s.eventsListeners).forEach((a) => {
        s.off(a);
      }), i2 !== false && (s.el && typeof s.el != "string" && (s.el.swiper = null), ke(s)), s.destroyed = true), null;
    }
    static extendDefaults(i2) {
      V(re, i2);
    }
    static get extendedDefaults() {
      return re;
    }
    static get defaults() {
      return ge;
    }
    static installModule(i2) {
      _$.prototype.__modules__ || (_$.prototype.__modules__ = []);
      const t = _$.prototype.__modules__;
      typeof i2 == "function" && t.indexOf(i2) < 0 && t.push(i2);
    }
    static use(i2) {
      return Array.isArray(i2) ? (i2.forEach((t) => _$.installModule(t)), _$) : (_$.installModule(i2), _$);
    }
  };
  Object.keys(ne).forEach((e) => {
    Object.keys(ne[e]).forEach((i2) => {
      $.prototype[i2] = ne[e][i2];
    });
  });
  $.use([Ne, Re]);
  function Xt({ swiper: e, extendParams: i2, on: t, emit: s }) {
    const n = F(), r2 = D();
    e.keyboard = { enabled: false }, i2({ keyboard: { enabled: false, onlyInViewport: true, pageUpDown: true } });
    function l2(u2) {
      if (!e.enabled) return;
      const { rtlTranslate: g2 } = e;
      let f = u2;
      f.originalEvent && (f = f.originalEvent);
      const S2 = f.keyCode || f.charCode, c = e.params.keyboard.pageUpDown, m = c && S2 === 33, v2 = c && S2 === 34, w2 = S2 === 37, d = S2 === 39, p = S2 === 38, h2 = S2 === 40;
      if (!e.allowSlideNext && (e.isHorizontal() && d || e.isVertical() && h2 || v2) || !e.allowSlidePrev && (e.isHorizontal() && w2 || e.isVertical() && p || m)) return false;
      if (!(f.shiftKey || f.altKey || f.ctrlKey || f.metaKey) && !(n.activeElement && (n.activeElement.isContentEditable || n.activeElement.nodeName && (n.activeElement.nodeName.toLowerCase() === "input" || n.activeElement.nodeName.toLowerCase() === "textarea")))) {
        if (e.params.keyboard.onlyInViewport && (m || v2 || w2 || d || p || h2)) {
          let b = false;
          if (Z(e.el, `.${e.params.slideClass}, swiper-slide`).length > 0 && Z(e.el, `.${e.params.slideActiveClass}`).length === 0) return;
          const x = e.el, P = x.clientWidth, M = x.clientHeight, y = r2.innerWidth, k2 = r2.innerHeight, T = Be(x);
          g2 && (T.left -= x.scrollLeft);
          const E3 = [[T.left, T.top], [T.left + P, T.top], [T.left, T.top + M], [T.left + P, T.top + M]];
          for (let C = 0; C < E3.length; C += 1) {
            const L = E3[C];
            if (L[0] >= 0 && L[0] <= y && L[1] >= 0 && L[1] <= k2) {
              if (L[0] === 0 && L[1] === 0) continue;
              b = true;
            }
          }
          if (!b) return;
        }
        e.isHorizontal() ? ((m || v2 || w2 || d) && (f.preventDefault ? f.preventDefault() : f.returnValue = false), ((v2 || d) && !g2 || (m || w2) && g2) && e.slideNext(), ((m || w2) && !g2 || (v2 || d) && g2) && e.slidePrev()) : ((m || v2 || p || h2) && (f.preventDefault ? f.preventDefault() : f.returnValue = false), (v2 || h2) && e.slideNext(), (m || p) && e.slidePrev()), s("keyPress", S2);
      }
    }
    function o() {
      e.keyboard.enabled || (n.addEventListener("keydown", l2), e.keyboard.enabled = true);
    }
    function a() {
      e.keyboard.enabled && (n.removeEventListener("keydown", l2), e.keyboard.enabled = false);
    }
    t("init", () => {
      e.params.keyboard.enabled && o();
    }), t("destroy", () => {
      e.keyboard.enabled && a();
    }), Object.assign(e.keyboard, { enable: o, disable: a });
  }
  function Me(e, i2, t, s) {
    return e.params.createElements && Object.keys(s).forEach((n) => {
      if (!t[n] && t.auto === true) {
        let r2 = _(e.el, `.${s[n]}`)[0];
        r2 || (r2 = K("div", s[n]), r2.className = s[n], e.el.append(r2)), t[n] = r2, i2[n] = r2;
      }
    }), t;
  }
  var ve = '<svg class="swiper-navigation-icon" width="11" height="20" viewBox="0 0 11 20" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M0.38296 20.0762C0.111788 19.805 0.111788 19.3654 0.38296 19.0942L9.19758 10.2796L0.38296 1.46497C0.111788 1.19379 0.111788 0.754138 0.38296 0.482966C0.654131 0.211794 1.09379 0.211794 1.36496 0.482966L10.4341 9.55214C10.8359 9.9539 10.8359 10.6053 10.4341 11.007L1.36496 20.0762C1.09379 20.3474 0.654131 20.3474 0.38296 20.0762Z" fill="currentColor"/></svg>';
  function Ut({ swiper: e, extendParams: i2, on: t, emit: s }) {
    i2({ navigation: { nextEl: null, prevEl: null, addIcons: true, hideOnClick: false, disabledClass: "swiper-button-disabled", hiddenClass: "swiper-button-hidden", lockClass: "swiper-button-lock", navigationDisabledClass: "swiper-navigation-disabled" } }), e.navigation = { nextEl: null, prevEl: null, arrowSvg: ve };
    function n(c) {
      let m;
      return c && typeof c == "string" && e.isElement && (m = e.el.querySelector(c) || e.hostEl.querySelector(c), m) ? m : (c && (typeof c == "string" && (m = [...document.querySelectorAll(c)]), e.params.uniqueNavElements && typeof c == "string" && m && m.length > 1 && e.el.querySelectorAll(c).length === 1 ? m = e.el.querySelector(c) : m && m.length === 1 && (m = m[0])), c && !m ? c : m);
    }
    function r2(c, m) {
      const v2 = e.params.navigation;
      c = A(c), c.forEach((w2) => {
        w2 && (w2.classList[m ? "add" : "remove"](...v2.disabledClass.split(" ")), w2.tagName === "BUTTON" && (w2.disabled = m), e.params.watchOverflow && e.enabled && w2.classList[e.isLocked ? "add" : "remove"](v2.lockClass));
      });
    }
    function l2() {
      const { nextEl: c, prevEl: m } = e.navigation;
      if (e.params.loop) {
        r2(m, false), r2(c, false);
        return;
      }
      r2(m, e.isBeginning && !e.params.rewind), r2(c, e.isEnd && !e.params.rewind);
    }
    function o(c) {
      c.preventDefault(), !(e.isBeginning && !e.params.loop && !e.params.rewind) && (e.slidePrev(), s("navigationPrev"));
    }
    function a(c) {
      c.preventDefault(), !(e.isEnd && !e.params.loop && !e.params.rewind) && (e.slideNext(), s("navigationNext"));
    }
    function u2() {
      const c = e.params.navigation;
      if (e.params.navigation = Me(e, e.originalParams.navigation, e.params.navigation, { nextEl: "swiper-button-next", prevEl: "swiper-button-prev" }), !(c.nextEl || c.prevEl)) return;
      let m = n(c.nextEl), v2 = n(c.prevEl);
      Object.assign(e.navigation, { nextEl: m, prevEl: v2 }), m = A(m), v2 = A(v2);
      const w2 = (d, p) => {
        if (d) {
          if (c.addIcons && d.matches(".swiper-button-next,.swiper-button-prev") && !d.querySelector("svg")) {
            const h2 = document.createElement("div");
            le(h2, ve), d.appendChild(h2.querySelector("svg")), h2.remove();
          }
          d.addEventListener("click", p === "next" ? a : o);
        }
        !e.enabled && d && d.classList.add(...c.lockClass.split(" "));
      };
      m.forEach((d) => w2(d, "next")), v2.forEach((d) => w2(d, "prev"));
    }
    function g2() {
      let { nextEl: c, prevEl: m } = e.navigation;
      c = A(c), m = A(m);
      const v2 = (w2, d) => {
        w2.removeEventListener("click", d === "next" ? a : o), w2.classList.remove(...e.params.navigation.disabledClass.split(" "));
      };
      c.forEach((w2) => v2(w2, "next")), m.forEach((w2) => v2(w2, "prev"));
    }
    t("init", () => {
      e.params.navigation.enabled === false ? S2() : (u2(), l2());
    }), t("toEdge fromEdge lock unlock", () => {
      l2();
    }), t("destroy", () => {
      g2();
    }), t("enable disable", () => {
      let { nextEl: c, prevEl: m } = e.navigation;
      if (c = A(c), m = A(m), e.enabled) {
        l2();
        return;
      }
      [...c, ...m].filter((v2) => !!v2).forEach((v2) => v2.classList.add(e.params.navigation.lockClass));
    }), t("click", (c, m) => {
      let { nextEl: v2, prevEl: w2 } = e.navigation;
      v2 = A(v2), w2 = A(w2);
      const d = m.target;
      let p = w2.includes(d) || v2.includes(d);
      if (e.isElement && !p) {
        const h2 = m.path || m.composedPath && m.composedPath();
        h2 && (p = h2.find((b) => v2.includes(b) || w2.includes(b)));
      }
      if (e.params.navigation.hideOnClick && !p) {
        if (e.pagination && e.params.pagination && e.params.pagination.clickable && (e.pagination.el === d || e.pagination.el.contains(d))) return;
        let h2;
        v2.length ? h2 = v2[0].classList.contains(e.params.navigation.hiddenClass) : w2.length && (h2 = w2[0].classList.contains(e.params.navigation.hiddenClass)), s(h2 === true ? "navigationShow" : "navigationHide"), [...v2, ...w2].filter((b) => !!b).forEach((b) => b.classList.toggle(e.params.navigation.hiddenClass));
      }
    });
    const f = () => {
      e.el.classList.remove(...e.params.navigation.navigationDisabledClass.split(" ")), u2(), l2();
    }, S2 = () => {
      e.el.classList.add(...e.params.navigation.navigationDisabledClass.split(" ")), g2();
    };
    Object.assign(e.navigation, { enable: f, disable: S2, update: l2, init: u2, destroy: g2 });
  }
  function W(e = "") {
    return `.${e.trim().replace(/([\.:!+\/()[\]])/g, "\\$1").replace(/ /g, ".")}`;
  }
  function Kt({ swiper: e, extendParams: i2, on: t, emit: s }) {
    const n = "swiper-pagination";
    i2({ pagination: { el: null, bulletElement: "span", clickable: false, hideOnClick: false, renderBullet: null, renderProgressbar: null, renderFraction: null, renderCustom: null, progressbarOpposite: false, type: "bullets", dynamicBullets: false, dynamicMainBullets: 1, formatFractionCurrent: (d) => d, formatFractionTotal: (d) => d, bulletClass: `${n}-bullet`, bulletActiveClass: `${n}-bullet-active`, modifierClass: `${n}-`, currentClass: `${n}-current`, totalClass: `${n}-total`, hiddenClass: `${n}-hidden`, progressbarFillClass: `${n}-progressbar-fill`, progressbarOppositeClass: `${n}-progressbar-opposite`, clickableClass: `${n}-clickable`, lockClass: `${n}-lock`, horizontalClass: `${n}-horizontal`, verticalClass: `${n}-vertical`, paginationDisabledClass: `${n}-disabled` } }), e.pagination = { el: null, bullets: [] };
    let r2, l2 = 0;
    function o() {
      return !e.params.pagination.el || !e.pagination.el || Array.isArray(e.pagination.el) && e.pagination.el.length === 0;
    }
    function a(d, p) {
      const { bulletActiveClass: h2 } = e.params.pagination;
      d && (d = d[`${p === "prev" ? "previous" : "next"}ElementSibling`], d && (d.classList.add(`${h2}-${p}`), d = d[`${p === "prev" ? "previous" : "next"}ElementSibling`], d && d.classList.add(`${h2}-${p}-${p}`)));
    }
    function u2(d, p, h2) {
      if (d = d % h2, p = p % h2, p === d + 1) return "next";
      if (p === d - 1) return "previous";
    }
    function g2(d) {
      const p = d.target.closest(W(e.params.pagination.bulletClass));
      if (!p) return;
      d.preventDefault();
      const h2 = Q(p) * e.params.slidesPerGroup;
      if (e.params.loop) {
        if (e.realIndex === h2) return;
        const b = u2(e.realIndex, h2, e.slides.length);
        b === "next" ? e.slideNext() : b === "previous" ? e.slidePrev() : e.slideToLoop(h2);
      } else e.slideTo(h2);
    }
    function f() {
      const d = e.rtl, p = e.params.pagination;
      if (o()) return;
      let h2 = e.pagination.el;
      h2 = A(h2);
      let b, x;
      const P = e.virtual && e.params.virtual.enabled ? e.virtual.slides.length : e.slides.length, M = e.params.loop ? Math.ceil(P / e.params.slidesPerGroup) : e.snapGrid.length;
      if (e.params.loop ? (x = e.previousRealIndex || 0, b = e.params.slidesPerGroup > 1 ? Math.floor(e.realIndex / e.params.slidesPerGroup) : e.realIndex) : typeof e.snapIndex < "u" ? (b = e.snapIndex, x = e.previousSnapIndex) : (x = e.previousIndex || 0, b = e.activeIndex || 0), p.type === "bullets" && e.pagination.bullets && e.pagination.bullets.length > 0) {
        const y = e.pagination.bullets;
        let k2, T, E3;
        if (p.dynamicBullets && (r2 = ae(y[0], e.isHorizontal() ? "width" : "height"), h2.forEach((C) => {
          C.style[e.isHorizontal() ? "width" : "height"] = `${r2 * (p.dynamicMainBullets + 4)}px`;
        }), p.dynamicMainBullets > 1 && x !== void 0 && (l2 += b - (x || 0), l2 > p.dynamicMainBullets - 1 ? l2 = p.dynamicMainBullets - 1 : l2 < 0 && (l2 = 0)), k2 = Math.max(b - l2, 0), T = k2 + (Math.min(y.length, p.dynamicMainBullets) - 1), E3 = (T + k2) / 2), y.forEach((C) => {
          const L = [...["", "-next", "-next-next", "-prev", "-prev-prev", "-main"].map((O3) => `${p.bulletActiveClass}${O3}`)].map((O3) => typeof O3 == "string" && O3.includes(" ") ? O3.split(" ") : O3).flat();
          C.classList.remove(...L);
        }), h2.length > 1) y.forEach((C) => {
          const L = Q(C);
          L === b ? C.classList.add(...p.bulletActiveClass.split(" ")) : e.isElement && C.setAttribute("part", "bullet"), p.dynamicBullets && (L >= k2 && L <= T && C.classList.add(...`${p.bulletActiveClass}-main`.split(" ")), L === k2 && a(C, "prev"), L === T && a(C, "next"));
        });
        else {
          const C = y[b];
          if (C && C.classList.add(...p.bulletActiveClass.split(" ")), e.isElement && y.forEach((L, O3) => {
            L.setAttribute("part", O3 === b ? "bullet-active" : "bullet");
          }), p.dynamicBullets) {
            const L = y[k2], O3 = y[T];
            for (let G2 = k2; G2 <= T; G2 += 1) y[G2] && y[G2].classList.add(...`${p.bulletActiveClass}-main`.split(" "));
            a(L, "prev"), a(O3, "next");
          }
        }
        if (p.dynamicBullets) {
          const C = Math.min(y.length, p.dynamicMainBullets + 4), L = (r2 * C - r2) / 2 - E3 * r2, O3 = d ? "right" : "left";
          y.forEach((G2) => {
            G2.style[e.isHorizontal() ? O3 : "top"] = `${L}px`;
          });
        }
      }
      h2.forEach((y, k2) => {
        if (p.type === "fraction" && (y.querySelectorAll(W(p.currentClass)).forEach((T) => {
          T.textContent = p.formatFractionCurrent(b + 1);
        }), y.querySelectorAll(W(p.totalClass)).forEach((T) => {
          T.textContent = p.formatFractionTotal(M);
        })), p.type === "progressbar") {
          let T;
          p.progressbarOpposite ? T = e.isHorizontal() ? "vertical" : "horizontal" : T = e.isHorizontal() ? "horizontal" : "vertical";
          const E3 = (b + 1) / M;
          let C = 1, L = 1;
          T === "horizontal" ? C = E3 : L = E3, y.querySelectorAll(W(p.progressbarFillClass)).forEach((O3) => {
            O3.style.transform = `translate3d(0,0,0) scaleX(${C}) scaleY(${L})`, O3.style.transitionDuration = `${e.params.speed}ms`;
          });
        }
        p.type === "custom" && p.renderCustom ? (le(y, p.renderCustom(e, b + 1, M)), k2 === 0 && s("paginationRender", y)) : (k2 === 0 && s("paginationRender", y), s("paginationUpdate", y)), e.params.watchOverflow && e.enabled && y.classList[e.isLocked ? "add" : "remove"](p.lockClass);
      });
    }
    function S2() {
      const d = e.params.pagination;
      if (o()) return;
      const p = e.virtual && e.params.virtual.enabled ? e.virtual.slides.length : e.grid && e.params.grid.rows > 1 ? e.slides.length / Math.ceil(e.params.grid.rows) : e.slides.length;
      let h2 = e.pagination.el;
      h2 = A(h2);
      let b = "";
      if (d.type === "bullets") {
        let x = e.params.loop ? Math.ceil(p / e.params.slidesPerGroup) : e.snapGrid.length;
        e.params.freeMode && e.params.freeMode.enabled && x > p && (x = p);
        for (let P = 0; P < x; P += 1) d.renderBullet ? b += d.renderBullet.call(e, P, d.bulletClass) : b += `<${d.bulletElement} ${e.isElement ? 'part="bullet"' : ""} class="${d.bulletClass}"></${d.bulletElement}>`;
      }
      d.type === "fraction" && (d.renderFraction ? b = d.renderFraction.call(e, d.currentClass, d.totalClass) : b = `<span class="${d.currentClass}"></span> / <span class="${d.totalClass}"></span>`), d.type === "progressbar" && (d.renderProgressbar ? b = d.renderProgressbar.call(e, d.progressbarFillClass) : b = `<span class="${d.progressbarFillClass}"></span>`), e.pagination.bullets = [], h2.forEach((x) => {
        d.type !== "custom" && le(x, b || ""), d.type === "bullets" && e.pagination.bullets.push(...x.querySelectorAll(W(d.bulletClass)));
      }), d.type !== "custom" && s("paginationRender", h2[0]);
    }
    function c() {
      e.params.pagination = Me(e, e.originalParams.pagination, e.params.pagination, { el: "swiper-pagination" });
      const d = e.params.pagination;
      if (!d.el) return;
      let p;
      typeof d.el == "string" && e.isElement && (p = e.el.querySelector(d.el)), !p && typeof d.el == "string" && (p = [...document.querySelectorAll(d.el)]), p || (p = d.el), !(!p || p.length === 0) && (e.params.uniqueNavElements && typeof d.el == "string" && Array.isArray(p) && p.length > 1 && (p = [...e.el.querySelectorAll(d.el)], p.length > 1 && (p = p.find((h2) => Z(h2, ".swiper")[0] === e.el))), Array.isArray(p) && p.length === 1 && (p = p[0]), Object.assign(e.pagination, { el: p }), p = A(p), p.forEach((h2) => {
        d.type === "bullets" && d.clickable && h2.classList.add(...(d.clickableClass || "").split(" ")), h2.classList.add(d.modifierClass + d.type), h2.classList.add(e.isHorizontal() ? d.horizontalClass : d.verticalClass), d.type === "bullets" && d.dynamicBullets && (h2.classList.add(`${d.modifierClass}${d.type}-dynamic`), l2 = 0, d.dynamicMainBullets < 1 && (d.dynamicMainBullets = 1)), d.type === "progressbar" && d.progressbarOpposite && h2.classList.add(d.progressbarOppositeClass), d.clickable && h2.addEventListener("click", g2), e.enabled || h2.classList.add(d.lockClass);
      }));
    }
    function m() {
      const d = e.params.pagination;
      if (o()) return;
      let p = e.pagination.el;
      p && (p = A(p), p.forEach((h2) => {
        h2.classList.remove(d.hiddenClass), h2.classList.remove(d.modifierClass + d.type), h2.classList.remove(e.isHorizontal() ? d.horizontalClass : d.verticalClass), d.clickable && (h2.classList.remove(...(d.clickableClass || "").split(" ")), h2.removeEventListener("click", g2));
      })), e.pagination.bullets && e.pagination.bullets.forEach((h2) => h2.classList.remove(...d.bulletActiveClass.split(" ")));
    }
    t("changeDirection", () => {
      if (!e.pagination || !e.pagination.el) return;
      const d = e.params.pagination;
      let { el: p } = e.pagination;
      p = A(p), p.forEach((h2) => {
        h2.classList.remove(d.horizontalClass, d.verticalClass), h2.classList.add(e.isHorizontal() ? d.horizontalClass : d.verticalClass);
      });
    }), t("init", () => {
      e.params.pagination.enabled === false ? w2() : (c(), S2(), f());
    }), t("activeIndexChange", () => {
      typeof e.snapIndex > "u" && f();
    }), t("snapIndexChange", () => {
      f();
    }), t("snapGridLengthChange", () => {
      S2(), f();
    }), t("destroy", () => {
      m();
    }), t("enable disable", () => {
      let { el: d } = e.pagination;
      d && (d = A(d), d.forEach((p) => p.classList[e.enabled ? "remove" : "add"](e.params.pagination.lockClass)));
    }), t("lock unlock", () => {
      f();
    }), t("click", (d, p) => {
      const h2 = p.target, b = A(e.pagination.el);
      if (e.params.pagination.el && e.params.pagination.hideOnClick && b && b.length > 0 && !h2.classList.contains(e.params.pagination.bulletClass)) {
        if (e.navigation && (e.navigation.nextEl && h2 === e.navigation.nextEl || e.navigation.prevEl && h2 === e.navigation.prevEl)) return;
        const x = b[0].classList.contains(e.params.pagination.hiddenClass);
        s(x === true ? "paginationShow" : "paginationHide"), b.forEach((P) => P.classList.toggle(e.params.pagination.hiddenClass));
      }
    });
    const v2 = () => {
      e.el.classList.remove(e.params.pagination.paginationDisabledClass);
      let { el: d } = e.pagination;
      d && (d = A(d), d.forEach((p) => p.classList.remove(e.params.pagination.paginationDisabledClass))), c(), S2(), f();
    }, w2 = () => {
      e.el.classList.add(e.params.pagination.paginationDisabledClass);
      let { el: d } = e.pagination;
      d && (d = A(d), d.forEach((p) => p.classList.add(e.params.pagination.paginationDisabledClass))), m();
    };
    Object.assign(e.pagination, { enable: v2, disable: w2, render: S2, update: f, init: c, destroy: m });
  }

  // dist/_astro/FeaturedCard.astro_astro_type_script_index_0_lang.js
  function z({ swiper: e, extendParams: C, on: r2, emit: a, params: n }) {
    e.autoplay = { running: false, paused: false, timeLeft: 0 }, C({ autoplay: { enabled: false, delay: 3e3, waitForTransition: true, disableOnInteraction: false, stopOnLastSlide: false, reverseDirection: false, pauseOnMouseEnter: false } });
    let u2, c, i2 = n && n.autoplay ? n.autoplay.delay : 3e3, f = n && n.autoplay ? n.autoplay.delay : 3e3, l2, g2 = (/* @__PURE__ */ new Date()).getTime(), b, S2, m, M, L, d, h2;
    function O3(t) {
      !e || e.destroyed || !e.wrapperEl || t.target === e.wrapperEl && (e.wrapperEl.removeEventListener("transitionend", O3), !(h2 || t.detail && t.detail.bySwiperTouchMove) && y());
    }
    const I2 = () => {
      if (e.destroyed || !e.autoplay.running) return;
      e.autoplay.paused ? b = true : b && (f = l2, b = false);
      const t = e.autoplay.paused ? l2 : g2 + f - (/* @__PURE__ */ new Date()).getTime();
      e.autoplay.timeLeft = t, a("autoplayTimeLeft", t, t / i2), c = requestAnimationFrame(() => {
        I2();
      });
    }, k2 = () => {
      let t;
      return e.virtual && e.params.virtual.enabled ? t = e.slides.find((o) => o.classList.contains("swiper-slide-active")) : t = e.slides[e.activeIndex], t ? parseInt(t.getAttribute("data-swiper-autoplay"), 10) : void 0;
    }, T = (t) => {
      if (e.destroyed || !e.autoplay.running) return;
      cancelAnimationFrame(c), I2();
      let s = typeof t > "u" ? e.params.autoplay.delay : t;
      i2 = e.params.autoplay.delay, f = e.params.autoplay.delay;
      const o = k2();
      !Number.isNaN(o) && o > 0 && typeof t > "u" && (s = o, i2 = o, f = o), l2 = s;
      const E3 = e.params.speed, F4 = () => {
        !e || e.destroyed || (e.params.autoplay.reverseDirection ? !e.isBeginning || e.params.loop || e.params.rewind ? (e.slidePrev(E3, true, true), a("autoplay")) : e.params.autoplay.stopOnLastSlide || (e.slideTo(e.slides.length - 1, E3, true, true), a("autoplay")) : !e.isEnd || e.params.loop || e.params.rewind ? (e.slideNext(E3, true, true), a("autoplay")) : e.params.autoplay.stopOnLastSlide || (e.slideTo(0, E3, true, true), a("autoplay")), e.params.cssMode && (g2 = (/* @__PURE__ */ new Date()).getTime(), requestAnimationFrame(() => {
          T();
        })));
      };
      return s > 0 ? (clearTimeout(u2), u2 = setTimeout(() => {
        F4();
      }, s)) : requestAnimationFrame(() => {
        F4();
      }), s;
    }, A2 = () => {
      g2 = (/* @__PURE__ */ new Date()).getTime(), e.autoplay.running = true, T(), a("autoplayStart");
    }, v2 = () => {
      e.autoplay.running = false, clearTimeout(u2), cancelAnimationFrame(c), a("autoplayStop");
    }, p = (t, s) => {
      if (e.destroyed || !e.autoplay.running) return;
      clearTimeout(u2), t || (d = true);
      const o = () => {
        a("autoplayPause"), e.params.autoplay.waitForTransition ? e.wrapperEl.addEventListener("transitionend", O3) : y();
      };
      if (e.autoplay.paused = true, s) {
        L && (l2 = e.params.autoplay.delay), L = false, o();
        return;
      }
      l2 = (l2 || e.params.autoplay.delay) - ((/* @__PURE__ */ new Date()).getTime() - g2), !(e.isEnd && l2 < 0 && !e.params.loop) && (l2 < 0 && (l2 = 0), o());
    }, y = () => {
      e.isEnd && l2 < 0 && !e.params.loop || e.destroyed || !e.autoplay.running || (g2 = (/* @__PURE__ */ new Date()).getTime(), d ? (d = false, T(l2)) : T(), e.autoplay.paused = false, a("autoplayResume"));
    }, q3 = () => {
      if (e.destroyed || !e.autoplay.running) return;
      const t = F();
      t.visibilityState === "hidden" && (d = true, p(true)), t.visibilityState === "visible" && y();
    }, P = (t) => {
      t.pointerType === "mouse" && (d = true, h2 = true, !(e.animating || e.autoplay.paused) && p(true));
    }, N3 = (t) => {
      t.pointerType === "mouse" && (h2 = false, e.autoplay.paused && y());
    }, x = () => {
      e.params.autoplay.pauseOnMouseEnter && (e.el.addEventListener("pointerenter", P), e.el.addEventListener("pointerleave", N3));
    }, B2 = () => {
      e.el && typeof e.el != "string" && (e.el.removeEventListener("pointerenter", P), e.el.removeEventListener("pointerleave", N3));
    }, R2 = () => {
      F().addEventListener("visibilitychange", q3);
    }, _2 = () => {
      F().removeEventListener("visibilitychange", q3);
    };
    r2("init", () => {
      e.params.autoplay.enabled && (x(), R2(), A2());
    }), r2("destroy", () => {
      B2(), _2(), e.autoplay.running && v2();
    }), r2("_freeModeStaticRelease", () => {
      (m || d) && y();
    }), r2("_freeModeNoMomentumRelease", () => {
      e.params.autoplay.disableOnInteraction ? v2() : p(true, true);
    }), r2("beforeTransitionStart", (t, s, o) => {
      e.destroyed || !e.autoplay.running || (o || !e.params.autoplay.disableOnInteraction ? p(true, true) : v2());
    }), r2("sliderFirstMove", () => {
      if (!(e.destroyed || !e.autoplay.running)) {
        if (e.params.autoplay.disableOnInteraction) {
          v2();
          return;
        }
        S2 = true, m = false, d = false, M = setTimeout(() => {
          d = true, m = true, p(true);
        }, 200);
      }
    }), r2("touchEnd", () => {
      if (!(e.destroyed || !e.autoplay.running || !S2)) {
        if (clearTimeout(M), clearTimeout(u2), e.params.autoplay.disableOnInteraction) {
          m = false, S2 = false;
          return;
        }
        m && e.params.cssMode && y(), m = false, S2 = false;
      }
    }), r2("slideChange", () => {
      e.destroyed || !e.autoplay.running || (L = true);
    }), Object.assign(e.autoplay, { start: A2, stop: v2, pause: p, resume: y });
  }
  document.addEventListener("DOMContentLoaded", () => {
    document.querySelectorAll(".featured-carousel").forEach((a) => {
      const n = a;
      if (!n.getAttribute("data-swiper-id")) return;
      const c = n.querySelector(".swiper");
      c && new $(c, { modules: [Ut, Kt, Xt, z], loop: true, autoplay: { delay: 4e3, disableOnInteraction: false, pauseOnMouseEnter: true }, pagination: { el: n.querySelector(".swiper-pagination"), clickable: true }, keyboard: { enabled: true, onlyInViewport: true }, on: { slideChange: function() {
        const i2 = n.querySelector(".swiper-current");
        i2 && (i2.textContent = (this.realIndex + 1).toString());
      } } });
    }), document.querySelectorAll(".featured-carousel .swiper-pagination").forEach((a) => {
      a.addEventListener("click", (n) => {
        n.stopPropagation();
      }, true);
    }), document.querySelectorAll("[data-sample-card]").forEach((a) => {
      a.addEventListener("click", (n) => {
        const u2 = n.target;
        if (u2.closest("[data-clickable-area]"), u2.closest(".swiper-pagination") || u2.closest(".swiper-slide")) return;
        const i2 = a.getAttribute("data-card-data");
        if (i2 && window.openSampleModal) {
          const f = JSON.parse(i2);
          window.openSampleModal(f);
        }
      });
    });
  });

  // dist/_astro/Modal.astro_astro_type_script_index_0_lang.js
  var E = { OpenURL: { label: "Open URL", style: "secondary" }, OpenFolder: { label: "Open Folder", style: "secondary" }, OpenScene: { label: "Open Scene", style: "primary" }, OpenTheme: { label: "Open Theme", style: "secondary" }, OpenSnapFlow: { label: "Open Snap Flow", style: "primary" }, OpenGridFlow: { label: "Open Grid Flow", style: "primary" }, OpenSnapGridFlow: { label: "Open Snap Grid Flow", style: "primary" }, CloneScene: { label: "Clone Scene", style: "primary" }, CloneSceneAndBuild: { label: "Clone & Build", style: "primary" }, CloneTheme: { label: "Clone Theme", style: "secondary" }, CloneSnapFlow: { label: "Clone Snap Flow", style: "secondary" }, CloneGridFlow: { label: "Clone Grid Flow", style: "secondary" }, CloneSnapGridFlow: { label: "Clone Snap Grid Flow", style: "secondary" }, Documentation: { label: "Documentation", style: "secondary" }, Video: { label: "Watch Video", style: "secondary" }, LauncherURL: { label: "Get Asset", style: "tertiary" }, AddStarterContent: { label: "Add Starter Content", style: "secondary" }, News: { label: "Read More", style: "secondary" } };
  function u(e, t) {
    const s = e.path || e.url || t || "", l2 = new URL(`daaction://${e.type}`);
    return s && l2.searchParams.set("path", s), e.title && l2.searchParams.set("title", e.title), l2.toString();
  }
  function v(e) {
    const t = E[e.type];
    return { label: e.title || e.label || t.label || e.type, icon: t.icon, style: t.style || "secondary" };
  }
  function I(e) {
    switch (e) {
      case "primary":
        return "btn btn-primary";
      case "secondary":
        return "btn btn-secondary";
      case "tertiary":
        return "btn btn-tertiary";
      default:
        return "btn btn-secondary";
    }
  }
  var r = document.querySelector("[data-modal-overlay]");
  var h = document.querySelector("[data-modal-close]");
  document.querySelector("[data-modal-content]");
  var i = null;
  function F2(e) {
    const t = document.getElementById("modal-single-image"), s = document.getElementById("modal-carousel"), l2 = document.getElementById("modal-image"), m = document.getElementById("modal-title"), b = document.getElementById("modal-description"), a = document.getElementById("modal-path"), d = document.getElementById("modal-category"), c = document.getElementById("modal-actions");
    if (m && (m.textContent = e.title), b && (b.textContent = e.desc || e.shortDesc), a && (e.path ? (a.textContent = e.path, a.style.display = "block") : a.style.display = "none"), d && (d.textContent = e.category), e.images && Array.isArray(e.images) && e.images.length > 1) t.style.display = "none", s.style.display = "block", O(e.images, e.title);
    else {
      t.style.display = "block", s.style.display = "none";
      const p = e.images && e.images.length > 0 ? e.images[0] : "";
      l2 && p && (l2.src = p, l2.alt = e.title), i && (i.destroy(), i = null);
    }
    if (c) {
      if (c.innerHTML = "", e.path) {
        const n = document.createElement("button");
        n.className = "btn btn-primary", n.innerHTML = '<i class="fas fa-copy"></i><span>Clone</span>', n.onclick = () => {
          const o = e.buildOnClone ? "CloneSceneAndBuild" : "CloneScene", f = u({ type: o, path: e.path }, e.path);
          window.location.href = f;
        }, c.appendChild(n);
      }
      if (e.path) {
        const n = document.createElement("a");
        n.className = "btn btn-secondary";
        const o = u({ type: "FolderView", path: e.path }, e.path);
        n.href = o, n.innerHTML = '<img src="./images/unreal_find.svg" alt="" class="btn-icon-img" /><span>View Folder</span>', c.appendChild(n);
      }
      if (e.videoUrl) {
        const n = document.createElement("a");
        n.className = "btn btn-secondary";
        const o = u({ type: "Video", url: e.videoUrl }, e.videoUrl);
        n.href = o, n.innerHTML = '<i class="fab fa-youtube"></i><span>Watch Video</span>', c.appendChild(n);
      }
      if (e.docsUrl) {
        const n = document.createElement("a");
        n.className = "btn btn-secondary";
        const o = u({ type: "Documentation", url: e.docsUrl }, e.docsUrl);
        n.href = o, n.innerHTML = '<i class="fas fa-book"></i><span>View Docs</span>', c.appendChild(n);
      }
      const p = e.actions || [];
      p.length > 0 && p.forEach((n) => {
        const o = v(n), f = u(n, e.path), y = document.createElement("a");
        y.className = I(o.style), y.href = f, y.textContent = o.label, n.width && (y.style.minWidth = `${n.width}px`), c.appendChild(y);
      });
    }
    r.classList.add("is-active"), document.body.classList.add("modal-open"), requestAnimationFrame(() => {
      r.classList.add("is-visible");
    });
  }
  function O(e, t) {
    const s = document.getElementById("modal-carousel-slides"), l2 = document.getElementById("swiper-current"), m = document.getElementById("swiper-total");
    s && (s.innerHTML = "", e.forEach((b) => {
      const a = document.createElement("div");
      a.className = "swiper-slide";
      const d = document.createElement("img");
      d.src = b, d.alt = t, a.appendChild(d), s.appendChild(a);
    }), m && (m.textContent = e.length.toString()), i && i.destroy(), i = new $(".swiper", { modules: [Ut, Kt, Xt], loop: true, navigation: { nextEl: ".swiper-button-next", prevEl: ".swiper-button-prev" }, pagination: { el: ".swiper-pagination", clickable: true }, keyboard: { enabled: true, onlyInViewport: false }, on: { slideChange: function() {
      l2 && (l2.textContent = (this.realIndex + 1).toString());
    } } }));
  }
  function g() {
    if (r.classList.remove("is-visible"), i) {
      try {
        i.destroy(true, true);
      } catch (e) {
        console.warn("Error destroying swiper:", e);
      }
      i = null;
    }
    document.body.classList.remove("modal-open"), setTimeout(() => {
      r.classList.remove("is-active"), document.body.style.overflow = "", document.body.style.position = "";
    }, 250);
  }
  h && h.addEventListener("click", g);
  r && r.addEventListener("click", (e) => {
    e.target === r && g();
  });
  document.addEventListener("keydown", (e) => {
    e.key === "Escape" && r?.classList.contains("is-active") && g();
  });
  window.openSampleModal = F2;

  // dist/_astro/index.astro_astro_type_script_index_0_lang.js
  function F3(t) {
    return Array.isArray ? Array.isArray(t) : ce(t) === "[object Array]";
  }
  function ge2(t) {
    if (typeof t == "string") return t;
    let e = t + "";
    return e == "0" && 1 / t == -1 / 0 ? "-0" : e;
  }
  function Ae2(t) {
    return t == null ? "" : ge2(t);
  }
  function B(t) {
    return typeof t == "string";
  }
  function re2(t) {
    return typeof t == "number";
  }
  function pe2(t) {
    return t === true || t === false || Ee2(t) && ce(t) == "[object Boolean]";
  }
  function ie2(t) {
    return typeof t == "object";
  }
  function Ee2(t) {
    return ie2(t) && t !== null;
  }
  function E2(t) {
    return t != null;
  }
  function j2(t) {
    return !t.trim().length;
  }
  function ce(t) {
    return t == null ? t === void 0 ? "[object Undefined]" : "[object Null]" : Object.prototype.toString.call(t);
  }
  var Ce2 = "Incorrect 'index' type";
  var me2 = (t) => `Invalid value for key ${t}`;
  var ye2 = (t) => `Pattern length exceeds max of ${t}.`;
  var Be2 = (t) => `Missing ${t} property in key`;
  var Me2 = (t) => `Property 'weight' in key '${t}' must be a positive integer`;
  var q2 = Object.prototype.hasOwnProperty;
  var Fe2 = class {
    constructor(e) {
      this._keys = [], this._keyMap = {};
      let s = 0;
      e.forEach((n) => {
        let r2 = ue2(n);
        this._keys.push(r2), this._keyMap[r2.id] = r2, s += r2.weight;
      }), this._keys.forEach((n) => {
        n.weight /= s;
      });
    }
    get(e) {
      return this._keyMap[e];
    }
    keys() {
      return this._keys;
    }
    toJSON() {
      return JSON.stringify(this._keys);
    }
  };
  function ue2(t) {
    let e = null, s = null, n = null, r2 = 1, i2 = null;
    if (B(t) || F3(t)) n = t, e = ee2(t), s = W2(t);
    else {
      if (!q2.call(t, "name")) throw new Error(Be2("name"));
      const c = t.name;
      if (n = c, q2.call(t, "weight") && (r2 = t.weight, r2 <= 0)) throw new Error(Me2(c));
      e = ee2(c), s = W2(c), i2 = t.getFn;
    }
    return { path: e, id: s, weight: r2, src: n, getFn: i2 };
  }
  function ee2(t) {
    return F3(t) ? t : t.split(".");
  }
  function W2(t) {
    return F3(t) ? t.join(".") : t;
  }
  function De2(t, e) {
    let s = [], n = false;
    const r2 = (i2, c, u2) => {
      if (E2(i2)) if (!c[u2]) s.push(i2);
      else {
        let o = c[u2];
        const a = i2[o];
        if (!E2(a)) return;
        if (u2 === c.length - 1 && (B(a) || re2(a) || pe2(a))) s.push(Ae2(a));
        else if (F3(a)) {
          n = true;
          for (let h2 = 0, f = a.length; h2 < f; h2 += 1) r2(a[h2], c, u2 + 1);
        } else c.length && r2(a, c, u2 + 1);
      }
    };
    return r2(t, B(e) ? e.split(".") : e, 0), n ? s : s[0];
  }
  var xe2 = { includeMatches: false, findAllMatches: false, minMatchCharLength: 1 };
  var Ie2 = { isCaseSensitive: false, ignoreDiacritics: false, includeScore: false, keys: [], shouldSort: true, sortFn: (t, e) => t.score === e.score ? t.idx < e.idx ? -1 : 1 : t.score < e.score ? -1 : 1 };
  var Se2 = { location: 0, threshold: 0.6, distance: 100 };
  var _e2 = { useExtendedSearch: false, getFn: De2, ignoreLocation: false, ignoreFieldNorm: false, fieldNormWeight: 1 };
  var l = { ...Ie2, ...xe2, ...Se2, ..._e2 };
  var we2 = /[^ ]+/g;
  function Le2(t = 1, e = 3) {
    const s = /* @__PURE__ */ new Map(), n = Math.pow(10, e);
    return { get(r2) {
      const i2 = r2.match(we2).length;
      if (s.has(i2)) return s.get(i2);
      const c = 1 / Math.pow(i2, 0.5 * t), u2 = parseFloat(Math.round(c * n) / n);
      return s.set(i2, u2), u2;
    }, clear() {
      s.clear();
    } };
  }
  var U2 = class {
    constructor({ getFn: e = l.getFn, fieldNormWeight: s = l.fieldNormWeight } = {}) {
      this.norm = Le2(s, 3), this.getFn = e, this.isCreated = false, this.setIndexRecords();
    }
    setSources(e = []) {
      this.docs = e;
    }
    setIndexRecords(e = []) {
      this.records = e;
    }
    setKeys(e = []) {
      this.keys = e, this._keysMap = {}, e.forEach((s, n) => {
        this._keysMap[s.id] = n;
      });
    }
    create() {
      this.isCreated || !this.docs.length || (this.isCreated = true, B(this.docs[0]) ? this.docs.forEach((e, s) => {
        this._addString(e, s);
      }) : this.docs.forEach((e, s) => {
        this._addObject(e, s);
      }), this.norm.clear());
    }
    add(e) {
      const s = this.size();
      B(e) ? this._addString(e, s) : this._addObject(e, s);
    }
    removeAt(e) {
      this.records.splice(e, 1);
      for (let s = e, n = this.size(); s < n; s += 1) this.records[s].i -= 1;
    }
    getValueForItemAtKeyId(e, s) {
      return e[this._keysMap[s]];
    }
    size() {
      return this.records.length;
    }
    _addString(e, s) {
      if (!E2(e) || j2(e)) return;
      let n = { v: e, i: s, n: this.norm.get(e) };
      this.records.push(n);
    }
    _addObject(e, s) {
      let n = { i: s, $: {} };
      this.keys.forEach((r2, i2) => {
        let c = r2.getFn ? r2.getFn(e) : this.getFn(e, r2.path);
        if (E2(c)) {
          if (F3(c)) {
            let u2 = [];
            const o = [{ nestedArrIndex: -1, value: c }];
            for (; o.length; ) {
              const { nestedArrIndex: a, value: h2 } = o.pop();
              if (E2(h2)) if (B(h2) && !j2(h2)) {
                let f = { v: h2, i: a, n: this.norm.get(h2) };
                u2.push(f);
              } else F3(h2) && h2.forEach((f, d) => {
                o.push({ nestedArrIndex: d, value: f });
              });
            }
            n.$[i2] = u2;
          } else if (B(c) && !j2(c)) {
            let u2 = { v: c, n: this.norm.get(c) };
            n.$[i2] = u2;
          }
        }
      }), this.records.push(n);
    }
    toJSON() {
      return { keys: this.keys, records: this.records };
    }
  };
  function oe2(t, e, { getFn: s = l.getFn, fieldNormWeight: n = l.fieldNormWeight } = {}) {
    const r2 = new U2({ getFn: s, fieldNormWeight: n });
    return r2.setKeys(t.map(ue2)), r2.setSources(e), r2.create(), r2;
  }
  function be2(t, { getFn: e = l.getFn, fieldNormWeight: s = l.fieldNormWeight } = {}) {
    const { keys: n, records: r2 } = t, i2 = new U2({ getFn: e, fieldNormWeight: s });
    return i2.setKeys(n), i2.setIndexRecords(r2), i2;
  }
  function $2(t, { errors: e = 0, currentLocation: s = 0, expectedLocation: n = 0, distance: r2 = l.distance, ignoreLocation: i2 = l.ignoreLocation } = {}) {
    const c = e / t.length;
    if (i2) return c;
    const u2 = Math.abs(n - s);
    return r2 ? c + u2 / r2 : u2 ? 1 : c;
  }
  function Re2(t = [], e = l.minMatchCharLength) {
    let s = [], n = -1, r2 = -1, i2 = 0;
    for (let c = t.length; i2 < c; i2 += 1) {
      let u2 = t[i2];
      u2 && n === -1 ? n = i2 : !u2 && n !== -1 && (r2 = i2 - 1, r2 - n + 1 >= e && s.push([n, r2]), n = -1);
    }
    return t[i2 - 1] && i2 - n >= e && s.push([n, i2 - 1]), s;
  }
  var w = 32;
  function ke2(t, e, s, { location: n = l.location, distance: r2 = l.distance, threshold: i2 = l.threshold, findAllMatches: c = l.findAllMatches, minMatchCharLength: u2 = l.minMatchCharLength, includeMatches: o = l.includeMatches, ignoreLocation: a = l.ignoreLocation } = {}) {
    if (e.length > w) throw new Error(ye2(w));
    const h2 = e.length, f = t.length, d = Math.max(0, Math.min(n, f));
    let g2 = i2, A2 = d;
    const p = u2 > 1 || o, m = p ? Array(f) : [];
    let D2;
    for (; (D2 = t.indexOf(e, A2)) > -1; ) {
      let C = $2(e, { currentLocation: D2, expectedLocation: d, distance: r2, ignoreLocation: a });
      if (g2 = Math.min(C, g2), A2 = D2 + h2, p) {
        let x = 0;
        for (; x < h2; ) m[D2 + x] = 1, x += 1;
      }
    }
    A2 = -1;
    let M = [], L = 1, _2 = h2 + f;
    const de2 = 1 << h2 - 1;
    for (let C = 0; C < h2; C += 1) {
      let x = 0, I2 = _2;
      for (; x < I2; ) $2(e, { errors: C, currentLocation: d + I2, expectedLocation: d, distance: r2, ignoreLocation: a }) <= g2 ? x = I2 : _2 = I2, I2 = Math.floor((_2 - x) / 2 + x);
      _2 = I2;
      let X2 = Math.max(1, d - I2 + 1), P = c ? f : Math.min(d + I2, f) + h2, b = Array(P + 2);
      b[P + 1] = (1 << C) - 1;
      for (let y = P; y >= X2; y -= 1) {
        let v2 = y - 1, Z2 = s[t.charAt(v2)];
        if (p && (m[v2] = +!!Z2), b[y] = (b[y + 1] << 1 | 1) & Z2, C && (b[y] |= (M[y + 1] | M[y]) << 1 | 1 | M[y + 1]), b[y] & de2 && (L = $2(e, { errors: C, currentLocation: v2, expectedLocation: d, distance: r2, ignoreLocation: a }), L <= g2)) {
          if (g2 = L, A2 = v2, A2 <= d) break;
          X2 = Math.max(1, 2 * d - A2);
        }
      }
      if ($2(e, { errors: C + 1, currentLocation: d, expectedLocation: d, distance: r2, ignoreLocation: a }) > g2) break;
      M = b;
    }
    const T = { isMatch: A2 >= 0, score: Math.max(1e-3, L) };
    if (p) {
      const C = Re2(m, u2);
      C.length ? o && (T.indices = C) : T.isMatch = false;
    }
    return T;
  }
  function ve2(t) {
    let e = {};
    for (let s = 0, n = t.length; s < n; s += 1) {
      const r2 = t.charAt(s);
      e[r2] = (e[r2] || 0) | 1 << n - s - 1;
    }
    return e;
  }
  var O2 = String.prototype.normalize ? ((t) => t.normalize("NFD").replace(/[\u0300-\u036F\u0483-\u0489\u0591-\u05BD\u05BF\u05C1\u05C2\u05C4\u05C5\u05C7\u0610-\u061A\u064B-\u065F\u0670\u06D6-\u06DC\u06DF-\u06E4\u06E7\u06E8\u06EA-\u06ED\u0711\u0730-\u074A\u07A6-\u07B0\u07EB-\u07F3\u07FD\u0816-\u0819\u081B-\u0823\u0825-\u0827\u0829-\u082D\u0859-\u085B\u08D3-\u08E1\u08E3-\u0903\u093A-\u093C\u093E-\u094F\u0951-\u0957\u0962\u0963\u0981-\u0983\u09BC\u09BE-\u09C4\u09C7\u09C8\u09CB-\u09CD\u09D7\u09E2\u09E3\u09FE\u0A01-\u0A03\u0A3C\u0A3E-\u0A42\u0A47\u0A48\u0A4B-\u0A4D\u0A51\u0A70\u0A71\u0A75\u0A81-\u0A83\u0ABC\u0ABE-\u0AC5\u0AC7-\u0AC9\u0ACB-\u0ACD\u0AE2\u0AE3\u0AFA-\u0AFF\u0B01-\u0B03\u0B3C\u0B3E-\u0B44\u0B47\u0B48\u0B4B-\u0B4D\u0B56\u0B57\u0B62\u0B63\u0B82\u0BBE-\u0BC2\u0BC6-\u0BC8\u0BCA-\u0BCD\u0BD7\u0C00-\u0C04\u0C3E-\u0C44\u0C46-\u0C48\u0C4A-\u0C4D\u0C55\u0C56\u0C62\u0C63\u0C81-\u0C83\u0CBC\u0CBE-\u0CC4\u0CC6-\u0CC8\u0CCA-\u0CCD\u0CD5\u0CD6\u0CE2\u0CE3\u0D00-\u0D03\u0D3B\u0D3C\u0D3E-\u0D44\u0D46-\u0D48\u0D4A-\u0D4D\u0D57\u0D62\u0D63\u0D82\u0D83\u0DCA\u0DCF-\u0DD4\u0DD6\u0DD8-\u0DDF\u0DF2\u0DF3\u0E31\u0E34-\u0E3A\u0E47-\u0E4E\u0EB1\u0EB4-\u0EB9\u0EBB\u0EBC\u0EC8-\u0ECD\u0F18\u0F19\u0F35\u0F37\u0F39\u0F3E\u0F3F\u0F71-\u0F84\u0F86\u0F87\u0F8D-\u0F97\u0F99-\u0FBC\u0FC6\u102B-\u103E\u1056-\u1059\u105E-\u1060\u1062-\u1064\u1067-\u106D\u1071-\u1074\u1082-\u108D\u108F\u109A-\u109D\u135D-\u135F\u1712-\u1714\u1732-\u1734\u1752\u1753\u1772\u1773\u17B4-\u17D3\u17DD\u180B-\u180D\u1885\u1886\u18A9\u1920-\u192B\u1930-\u193B\u1A17-\u1A1B\u1A55-\u1A5E\u1A60-\u1A7C\u1A7F\u1AB0-\u1ABE\u1B00-\u1B04\u1B34-\u1B44\u1B6B-\u1B73\u1B80-\u1B82\u1BA1-\u1BAD\u1BE6-\u1BF3\u1C24-\u1C37\u1CD0-\u1CD2\u1CD4-\u1CE8\u1CED\u1CF2-\u1CF4\u1CF7-\u1CF9\u1DC0-\u1DF9\u1DFB-\u1DFF\u20D0-\u20F0\u2CEF-\u2CF1\u2D7F\u2DE0-\u2DFF\u302A-\u302F\u3099\u309A\uA66F-\uA672\uA674-\uA67D\uA69E\uA69F\uA6F0\uA6F1\uA802\uA806\uA80B\uA823-\uA827\uA880\uA881\uA8B4-\uA8C5\uA8E0-\uA8F1\uA8FF\uA926-\uA92D\uA947-\uA953\uA980-\uA983\uA9B3-\uA9C0\uA9E5\uAA29-\uAA36\uAA43\uAA4C\uAA4D\uAA7B-\uAA7D\uAAB0\uAAB2-\uAAB4\uAAB7\uAAB8\uAABE\uAABF\uAAC1\uAAEB-\uAAEF\uAAF5\uAAF6\uABE3-\uABEA\uABEC\uABED\uFB1E\uFE00-\uFE0F\uFE20-\uFE2F]/g, "")) : ((t) => t);
  var ae2 = class {
    constructor(e, { location: s = l.location, threshold: n = l.threshold, distance: r2 = l.distance, includeMatches: i2 = l.includeMatches, findAllMatches: c = l.findAllMatches, minMatchCharLength: u2 = l.minMatchCharLength, isCaseSensitive: o = l.isCaseSensitive, ignoreDiacritics: a = l.ignoreDiacritics, ignoreLocation: h2 = l.ignoreLocation } = {}) {
      if (this.options = { location: s, threshold: n, distance: r2, includeMatches: i2, findAllMatches: c, minMatchCharLength: u2, isCaseSensitive: o, ignoreDiacritics: a, ignoreLocation: h2 }, e = o ? e : e.toLowerCase(), e = a ? O2(e) : e, this.pattern = e, this.chunks = [], !this.pattern.length) return;
      const f = (g2, A2) => {
        this.chunks.push({ pattern: g2, alphabet: ve2(g2), startIndex: A2 });
      }, d = this.pattern.length;
      if (d > w) {
        let g2 = 0;
        const A2 = d % w, p = d - A2;
        for (; g2 < p; ) f(this.pattern.substr(g2, w), g2), g2 += w;
        if (A2) {
          const m = d - w;
          f(this.pattern.substr(m), m);
        }
      } else f(this.pattern, 0);
    }
    searchIn(e) {
      const { isCaseSensitive: s, ignoreDiacritics: n, includeMatches: r2 } = this.options;
      if (e = s ? e : e.toLowerCase(), e = n ? O2(e) : e, this.pattern === e) {
        let p = { isMatch: true, score: 0 };
        return r2 && (p.indices = [[0, e.length - 1]]), p;
      }
      const { location: i2, distance: c, threshold: u2, findAllMatches: o, minMatchCharLength: a, ignoreLocation: h2 } = this.options;
      let f = [], d = 0, g2 = false;
      this.chunks.forEach(({ pattern: p, alphabet: m, startIndex: D2 }) => {
        const { isMatch: M, score: L, indices: _2 } = ke2(e, p, m, { location: i2 + D2, distance: c, threshold: u2, findAllMatches: o, minMatchCharLength: a, includeMatches: r2, ignoreLocation: h2 });
        M && (g2 = true), d += L, M && _2 && (f = [...f, ..._2]);
      });
      let A2 = { isMatch: g2, score: g2 ? d / this.chunks.length : 1 };
      return g2 && r2 && (A2.indices = f), A2;
    }
  };
  var S = class {
    constructor(e) {
      this.pattern = e;
    }
    static isMultiMatch(e) {
      return te2(e, this.multiRegex);
    }
    static isSingleMatch(e) {
      return te2(e, this.singleRegex);
    }
    search() {
    }
  };
  function te2(t, e) {
    const s = t.match(e);
    return s ? s[1] : null;
  }
  var $e2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "exact";
    }
    static get multiRegex() {
      return /^="(.*)"$/;
    }
    static get singleRegex() {
      return /^=(.*)$/;
    }
    search(e) {
      const s = e === this.pattern;
      return { isMatch: s, score: s ? 0 : 1, indices: [0, this.pattern.length - 1] };
    }
  };
  var Oe2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "inverse-exact";
    }
    static get multiRegex() {
      return /^!"(.*)"$/;
    }
    static get singleRegex() {
      return /^!(.*)$/;
    }
    search(e) {
      const n = e.indexOf(this.pattern) === -1;
      return { isMatch: n, score: n ? 0 : 1, indices: [0, e.length - 1] };
    }
  };
  var Ne2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "prefix-exact";
    }
    static get multiRegex() {
      return /^\^"(.*)"$/;
    }
    static get singleRegex() {
      return /^\^(.*)$/;
    }
    search(e) {
      const s = e.startsWith(this.pattern);
      return { isMatch: s, score: s ? 0 : 1, indices: [0, this.pattern.length - 1] };
    }
  };
  var Te2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "inverse-prefix-exact";
    }
    static get multiRegex() {
      return /^!\^"(.*)"$/;
    }
    static get singleRegex() {
      return /^!\^(.*)$/;
    }
    search(e) {
      const s = !e.startsWith(this.pattern);
      return { isMatch: s, score: s ? 0 : 1, indices: [0, e.length - 1] };
    }
  };
  var Pe = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "suffix-exact";
    }
    static get multiRegex() {
      return /^"(.*)"\$$/;
    }
    static get singleRegex() {
      return /^(.*)\$$/;
    }
    search(e) {
      const s = e.endsWith(this.pattern);
      return { isMatch: s, score: s ? 0 : 1, indices: [e.length - this.pattern.length, e.length - 1] };
    }
  };
  var je2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "inverse-suffix-exact";
    }
    static get multiRegex() {
      return /^!"(.*)"\$$/;
    }
    static get singleRegex() {
      return /^!(.*)\$$/;
    }
    search(e) {
      const s = !e.endsWith(this.pattern);
      return { isMatch: s, score: s ? 0 : 1, indices: [0, e.length - 1] };
    }
  };
  var he2 = class extends S {
    constructor(e, { location: s = l.location, threshold: n = l.threshold, distance: r2 = l.distance, includeMatches: i2 = l.includeMatches, findAllMatches: c = l.findAllMatches, minMatchCharLength: u2 = l.minMatchCharLength, isCaseSensitive: o = l.isCaseSensitive, ignoreDiacritics: a = l.ignoreDiacritics, ignoreLocation: h2 = l.ignoreLocation } = {}) {
      super(e), this._bitapSearch = new ae2(e, { location: s, threshold: n, distance: r2, includeMatches: i2, findAllMatches: c, minMatchCharLength: u2, isCaseSensitive: o, ignoreDiacritics: a, ignoreLocation: h2 });
    }
    static get type() {
      return "fuzzy";
    }
    static get multiRegex() {
      return /^"(.*)"$/;
    }
    static get singleRegex() {
      return /^(.*)$/;
    }
    search(e) {
      return this._bitapSearch.searchIn(e);
    }
  };
  var le2 = class extends S {
    constructor(e) {
      super(e);
    }
    static get type() {
      return "include";
    }
    static get multiRegex() {
      return /^'"(.*)"$/;
    }
    static get singleRegex() {
      return /^'(.*)$/;
    }
    search(e) {
      let s = 0, n;
      const r2 = [], i2 = this.pattern.length;
      for (; (n = e.indexOf(this.pattern, s)) > -1; ) s = n + i2, r2.push([n, s - 1]);
      const c = !!r2.length;
      return { isMatch: c, score: c ? 0 : 1, indices: r2 };
    }
  };
  var G = [$e2, le2, Ne2, Te2, je2, Pe, Oe2, he2];
  var se2 = G.length;
  var Ke2 = / +(?=(?:[^\"]*\"[^\"]*\")*[^\"]*$)/;
  var ze2 = "|";
  function We2(t, e = {}) {
    return t.split(ze2).map((s) => {
      let n = s.trim().split(Ke2).filter((i2) => i2 && !!i2.trim()), r2 = [];
      for (let i2 = 0, c = n.length; i2 < c; i2 += 1) {
        const u2 = n[i2];
        let o = false, a = -1;
        for (; !o && ++a < se2; ) {
          const h2 = G[a];
          let f = h2.isMultiMatch(u2);
          f && (r2.push(new h2(f, e)), o = true);
        }
        if (!o) for (a = -1; ++a < se2; ) {
          const h2 = G[a];
          let f = h2.isSingleMatch(u2);
          if (f) {
            r2.push(new h2(f, e));
            break;
          }
        }
      }
      return r2;
    });
  }
  var Ge2 = /* @__PURE__ */ new Set([he2.type, le2.type]);
  var He2 = class {
    constructor(e, { isCaseSensitive: s = l.isCaseSensitive, ignoreDiacritics: n = l.ignoreDiacritics, includeMatches: r2 = l.includeMatches, minMatchCharLength: i2 = l.minMatchCharLength, ignoreLocation: c = l.ignoreLocation, findAllMatches: u2 = l.findAllMatches, location: o = l.location, threshold: a = l.threshold, distance: h2 = l.distance } = {}) {
      this.query = null, this.options = { isCaseSensitive: s, ignoreDiacritics: n, includeMatches: r2, minMatchCharLength: i2, findAllMatches: u2, ignoreLocation: c, location: o, threshold: a, distance: h2 }, e = s ? e : e.toLowerCase(), e = n ? O2(e) : e, this.pattern = e, this.query = We2(this.pattern, this.options);
    }
    static condition(e, s) {
      return s.useExtendedSearch;
    }
    searchIn(e) {
      const s = this.query;
      if (!s) return { isMatch: false, score: 1 };
      const { includeMatches: n, isCaseSensitive: r2, ignoreDiacritics: i2 } = this.options;
      e = r2 ? e : e.toLowerCase(), e = i2 ? O2(e) : e;
      let c = 0, u2 = [], o = 0;
      for (let a = 0, h2 = s.length; a < h2; a += 1) {
        const f = s[a];
        u2.length = 0, c = 0;
        for (let d = 0, g2 = f.length; d < g2; d += 1) {
          const A2 = f[d], { isMatch: p, indices: m, score: D2 } = A2.search(e);
          if (p) {
            if (c += 1, o += D2, n) {
              const M = A2.constructor.type;
              Ge2.has(M) ? u2 = [...u2, ...m] : u2.push(m);
            }
          } else {
            o = 0, c = 0, u2.length = 0;
            break;
          }
        }
        if (c) {
          let d = { isMatch: true, score: o / c };
          return n && (d.indices = u2), d;
        }
      }
      return { isMatch: false, score: 1 };
    }
  };
  var H = [];
  function Ve2(...t) {
    H.push(...t);
  }
  function V2(t, e) {
    for (let s = 0, n = H.length; s < n; s += 1) {
      let r2 = H[s];
      if (r2.condition(t, e)) return new r2(t, e);
    }
    return new ae2(t, e);
  }
  var N2 = { AND: "$and", OR: "$or" };
  var Y2 = { PATH: "$path", PATTERN: "$val" };
  var Q2 = (t) => !!(t[N2.AND] || t[N2.OR]);
  var Ye2 = (t) => !!t[Y2.PATH];
  var Qe2 = (t) => !F3(t) && ie2(t) && !Q2(t);
  var ne2 = (t) => ({ [N2.AND]: Object.keys(t).map((e) => ({ [e]: t[e] })) });
  function fe2(t, e, { auto: s = true } = {}) {
    const n = (r2) => {
      let i2 = Object.keys(r2);
      const c = Ye2(r2);
      if (!c && i2.length > 1 && !Q2(r2)) return n(ne2(r2));
      if (Qe2(r2)) {
        const o = c ? r2[Y2.PATH] : i2[0], a = c ? r2[Y2.PATTERN] : r2[o];
        if (!B(a)) throw new Error(me2(o));
        const h2 = { keyId: W2(o), pattern: a };
        return s && (h2.searcher = V2(a, e)), h2;
      }
      let u2 = { children: [], operator: i2[0] };
      return i2.forEach((o) => {
        const a = r2[o];
        F3(a) && a.forEach((h2) => {
          u2.children.push(n(h2));
        });
      }), u2;
    };
    return Q2(t) || (t = ne2(t)), n(t);
  }
  function Je2(t, { ignoreFieldNorm: e = l.ignoreFieldNorm }) {
    t.forEach((s) => {
      let n = 1;
      s.matches.forEach(({ key: r2, norm: i2, score: c }) => {
        const u2 = r2 ? r2.weight : null;
        n *= Math.pow(c === 0 && u2 ? Number.EPSILON : c, (u2 || 1) * (e ? 1 : i2));
      }), s.score = n;
    });
  }
  function Ue2(t, e) {
    const s = t.matches;
    e.matches = [], E2(s) && s.forEach((n) => {
      if (!E2(n.indices) || !n.indices.length) return;
      const { indices: r2, value: i2 } = n;
      let c = { indices: r2, value: i2 };
      n.key && (c.key = n.key.src), n.idx > -1 && (c.refIndex = n.idx), e.matches.push(c);
    });
  }
  function Xe2(t, e) {
    e.score = t.score;
  }
  function Ze2(t, e, { includeMatches: s = l.includeMatches, includeScore: n = l.includeScore } = {}) {
    const r2 = [];
    return s && r2.push(Ue2), n && r2.push(Xe2), t.map((i2) => {
      const { idx: c } = i2, u2 = { item: e[c], refIndex: c };
      return r2.length && r2.forEach((o) => {
        o(i2, u2);
      }), u2;
    });
  }
  var R = class {
    constructor(e, s = {}, n) {
      this.options = { ...l, ...s }, this.options.useExtendedSearch, this._keyStore = new Fe2(this.options.keys), this.setCollection(e, n);
    }
    setCollection(e, s) {
      if (this._docs = e, s && !(s instanceof U2)) throw new Error(Ce2);
      this._myIndex = s || oe2(this.options.keys, this._docs, { getFn: this.options.getFn, fieldNormWeight: this.options.fieldNormWeight });
    }
    add(e) {
      E2(e) && (this._docs.push(e), this._myIndex.add(e));
    }
    remove(e = () => false) {
      const s = [];
      for (let n = 0, r2 = this._docs.length; n < r2; n += 1) {
        const i2 = this._docs[n];
        e(i2, n) && (this.removeAt(n), n -= 1, r2 -= 1, s.push(i2));
      }
      return s;
    }
    removeAt(e) {
      this._docs.splice(e, 1), this._myIndex.removeAt(e);
    }
    getIndex() {
      return this._myIndex;
    }
    search(e, { limit: s = -1 } = {}) {
      const { includeMatches: n, includeScore: r2, shouldSort: i2, sortFn: c, ignoreFieldNorm: u2 } = this.options;
      let o = B(e) ? B(this._docs[0]) ? this._searchStringList(e) : this._searchObjectList(e) : this._searchLogical(e);
      return Je2(o, { ignoreFieldNorm: u2 }), i2 && o.sort(c), re2(s) && s > -1 && (o = o.slice(0, s)), Ze2(o, this._docs, { includeMatches: n, includeScore: r2 });
    }
    _searchStringList(e) {
      const s = V2(e, this.options), { records: n } = this._myIndex, r2 = [];
      return n.forEach(({ v: i2, i: c, n: u2 }) => {
        if (!E2(i2)) return;
        const { isMatch: o, score: a, indices: h2 } = s.searchIn(i2);
        o && r2.push({ item: i2, idx: c, matches: [{ score: a, value: i2, norm: u2, indices: h2 }] });
      }), r2;
    }
    _searchLogical(e) {
      const s = fe2(e, this.options), n = (u2, o, a) => {
        if (!u2.children) {
          const { keyId: f, searcher: d } = u2, g2 = this._findMatches({ key: this._keyStore.get(f), value: this._myIndex.getValueForItemAtKeyId(o, f), searcher: d });
          return g2 && g2.length ? [{ idx: a, item: o, matches: g2 }] : [];
        }
        const h2 = [];
        for (let f = 0, d = u2.children.length; f < d; f += 1) {
          const g2 = u2.children[f], A2 = n(g2, o, a);
          if (A2.length) h2.push(...A2);
          else if (u2.operator === N2.AND) return [];
        }
        return h2;
      }, r2 = this._myIndex.records, i2 = {}, c = [];
      return r2.forEach(({ $: u2, i: o }) => {
        if (E2(u2)) {
          let a = n(s, u2, o);
          a.length && (i2[o] || (i2[o] = { idx: o, item: u2, matches: [] }, c.push(i2[o])), a.forEach(({ matches: h2 }) => {
            i2[o].matches.push(...h2);
          }));
        }
      }), c;
    }
    _searchObjectList(e) {
      const s = V2(e, this.options), { keys: n, records: r2 } = this._myIndex, i2 = [];
      return r2.forEach(({ $: c, i: u2 }) => {
        if (!E2(c)) return;
        let o = [];
        n.forEach((a, h2) => {
          o.push(...this._findMatches({ key: a, value: c[h2], searcher: s }));
        }), o.length && i2.push({ idx: u2, item: c, matches: o });
      }), i2;
    }
    _findMatches({ key: e, value: s, searcher: n }) {
      if (!E2(s)) return [];
      let r2 = [];
      if (F3(s)) s.forEach(({ v: i2, i: c, n: u2 }) => {
        if (!E2(i2)) return;
        const { isMatch: o, score: a, indices: h2 } = n.searchIn(i2);
        o && r2.push({ score: a, key: e, value: i2, idx: c, norm: u2, indices: h2 });
      });
      else {
        const { v: i2, n: c } = s, { isMatch: u2, score: o, indices: a } = n.searchIn(i2);
        u2 && r2.push({ score: o, key: e, value: i2, norm: c, indices: a });
      }
      return r2;
    }
  };
  R.version = "7.1.0";
  R.createIndex = oe2;
  R.parseIndex = be2;
  R.config = l;
  R.parseQuery = fe2;
  Ve2(He2);
  var J2 = null;
  var k = [];
  var K2 = null;
  function qe2() {
    const t = document.getElementById("search-input"), e = document.getElementById("search-clear"), s = document.getElementById("search-results-count");
    t && (k = et2(), J2 = new R(k, { keys: [{ name: "title", weight: 0.4 }, { name: "description", weight: 0.3 }, { name: "category", weight: 0.2 }, { name: "tags", weight: 0.1 }], threshold: 0.4, distance: 100, minMatchCharLength: 2, includeScore: true }), t.addEventListener("input", (n) => {
      const r2 = n.target.value;
      r2.length > 0 ? e.style.display = "flex" : e.style.display = "none", K2 && clearTimeout(K2), K2 = window.setTimeout(() => {
        z2(r2, s);
      }, 150);
    }), e.addEventListener("click", () => {
      t.value = "", e.style.display = "none", z2("", s), t.focus();
    }), t.addEventListener("keydown", (n) => {
      n.key === "Escape" && (t.value = "", e.style.display = "none", z2("", s));
    }));
  }
  function et2() {
    const t = [];
    return document.querySelectorAll("[data-searchable]").forEach((s) => {
      const n = s, r2 = n.dataset;
      t.push({ id: r2.searchId || "", title: r2.searchTitle || "", description: r2.searchDescription || "", category: r2.searchCategory || "", tags: r2.searchTags ? r2.searchTags.split(",") : [], element: n });
    }), t;
  }
  function z2(t, e) {
    if (!J2) return;
    if (!t || t.trim().length === 0) {
      k.forEach((i2) => {
        i2.element.style.display = "";
      }), e.style.display = "none", tt2();
      return;
    }
    const s = J2.search(t), n = new Set(s.map((i2) => i2.item.id));
    let r2 = 0;
    k.forEach((i2) => {
      n.has(i2.id) ? (i2.element.style.display = "", r2++) : i2.element.style.display = "none";
    }), st2(), nt2(r2, k.length, e);
  }
  function tt2() {
    document.querySelectorAll(".content-section").forEach((e) => {
      e.style.display = "";
    });
  }
  function st2() {
    document.querySelectorAll(".content-section").forEach((e) => {
      const s = e, n = s.querySelectorAll("[data-searchable]");
      Array.from(n).filter((i2) => i2.style.display !== "none").length === 0 ? s.style.display = "none" : s.style.display = "";
    });
  }
  function nt2(t, e, s) {
    s.style.display = "block", t === 0 ? (s.textContent = "No results found", s.classList.add("no-results")) : (s.textContent = `Showing ${t} of ${e} items`, s.classList.remove("no-results"));
  }
  document.addEventListener("DOMContentLoaded", () => {
    qe2();
  });
})();
