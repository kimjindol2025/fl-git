#!/usr/bin/env node
// island-runner.js — Island headless 실행기 (npm 0개, 순수 Node.js)
// AI가 브라우저 없이 Island 동작 검증:
//   node island-runner.js <file.flx> [islandName] [--click=selector] [--dispatch=key:val]
//
// 출력: JSON { ok, html, store, events, error }

'use strict';
const fs   = require('fs');
const path = require('path');
const { compileFileIslands, extractIslands } = require('./island-compiler');

// ── VirtualDOM (순수 JS, npm 0개) ─────────────────────────────
class VNode {
  constructor(tag, attrs={}) {
    this.tag      = tag;
    this.attrs    = attrs;
    this.children = [];
    this.checked  = false;
    this.value    = attrs.value ?? '';
    this._handlers = {};
  }
  setAttribute(k, v) { this.attrs[k] = String(v ?? ''); }
  getAttribute(k)    { return this.attrs[k] ?? null; }
  addEventListener(ev, fn) {
    if (!this._handlers[ev]) this._handlers[ev] = [];
    this._handlers[ev].push(fn);
  }
  appendChild(child) {
    if (child instanceof VNode) this.children.push(child);
    else this.children.push(String(child ?? ''));
    return child;
  }
  removeChild(child) { this.children = this.children.filter(c => c !== child); }
  get innerHTML()    { return this._serialize(); }
  set innerHTML(v)   { this.children = [v]; }
  get firstChild()   { return this.children[0] ?? null; }
  _serialize() {
    const attrs = Object.entries(this.attrs)
      .filter(([k,v]) => v !== null && v !== undefined && v !== '' && !k.startsWith('on'))
      .map(([k,v]) => ` ${k}="${String(v).replace(/"/g,'&quot;')}"`)
      .join('');
    const VOID = new Set(['input','br','hr','img','meta','link']);
    if (VOID.has(this.tag)) return `<${this.tag}${attrs}>`;
    const inner = this.children.map(c => c instanceof VNode ? c._serialize() : String(c ?? '')).join('');
    return `<${this.tag}${attrs}>${inner}</${this.tag}>`;
  }
  // 이벤트 트리거 (헤드리스 테스트용)
  trigger(ev, eventObj={}) {
    const fns = this._handlers[ev] || [];
    fns.forEach(fn => fn(eventObj));
    return fns.length;
  }
  // _fl_str 호환 — VNode를 문자열로 쓸 때 HTML 반환
  toString() { return this._serialize(); }
  // 선택자 탐색 (간단한 id/tag/class)
  querySelector(sel) {
    if (sel.startsWith('#')) {
      const id = sel.slice(1);
      if (this.attrs.id === id) return this;
      for (const c of this.children) { if (c instanceof VNode) { const r = c.querySelector(sel); if (r) return r; } }
    } else {
      if (this.tag === sel) return this;
      for (const c of this.children) { if (c instanceof VNode) { const r = c.querySelector(sel); if (r) return r; } }
    }
    return null;
  }
  querySelectorAll(sel) {
    const results = [];
    const walk = (node) => {
      if (!(node instanceof VNode)) return;
      if (sel.startsWith('.') ? (node.attrs.class||'').split(' ').includes(sel.slice(1)) : node.tag === sel) results.push(node);
      node.children.forEach(walk);
    };
    walk(this);
    return results;
  }
}

function makeVDocument() {
  const root = new VNode('div');
  return {
    createElement: (tag) => new VNode(tag),
    createTextNode: (t) => String(t),
    getElementById: (id) => root.querySelector('#'+id),
    body: root,
    _root: root,
  };
}

// ── Island 실행 컨텍스트 ──────────────────────────────────────
function makeContext(vdoc) {
  const events = [];

  const _fl = {
    atom:   (v) => ({ v }),
    deref:  (a) => a.v,
    swap:   (a, f) => { a.v = f(a.v); return a.v; },
    reset:  (a, v) => { a.v = v; return v; },
    _store: { v: {} },
    _watchers: [],
    _pending_restore: null,
    _hmr_islands: {},
    hmr_register: (id, atoms) => { if (id) _fl._hmr_islands[id] = atoms; },
    hmr_snapshot: () => ({ store: { ..._fl._store.v }, islands: {} }),
    get_store: (key) => key ? (_fl._store.v[key] ?? null) : _fl._store.v,
    dispatch: (key, val) => {
      const prev = _fl._store.v;
      _fl._store.v = typeof key === 'object' ? {...prev,...key} : {...prev,[key]:val};
      events.push({ type:'dispatch', key, val, store: {..._fl._store.v} });
      _fl._watchers.forEach(fn => fn(prev, _fl._store.v, key));
    },
    watch_store: (fn) => { _fl._watchers.push(fn); },
    push:       (arr, item) => [...(arr||[]), item],
    get:        (obj, key) => obj == null ? null : (obj[key] ?? null),
    assoc:      (obj, key, val) => ({...(obj||{}), [key]:val}),
    dissoc:     (obj, key) => { const r={...(obj||{})}; delete r[key]; return r; },
    map_arr:    (fn, arr) => (arr||[]).map(fn),
    filter_arr: (fn, arr) => (arr||[]).filter(fn),
    reduce_arr: (fn, init, arr) => (arr||[]).reduce(fn, init),
    h: (tag, attrs, ...ch) => {
      const el = vdoc.createElement(tag);
      for (const [k,v] of Object.entries(attrs||{})) {
        if (k === 'checked') { if (v && v !== '') el.checked = true; }
        else if (k.startsWith('on-')) el.addEventListener(k.slice(3), v);
        else if (v !== null && v !== undefined && v !== '') el.setAttribute(k, String(v));
      }
      ch.flat().forEach(c => {
        if (c == null) return;
        if (c instanceof VNode) el.appendChild(c);
        else el.appendChild(String(c));
      });
      return el;
    },
    fetch: (url, cb) => {
      events.push({ type:'fetch', url });
      // headless에선 mock 응답
      setTimeout(() => cb({ ok:true, _mock:true, url }), 0);
    },
    fetch_post: (url, body, cb) => {
      events.push({ type:'fetch-post', url, body });
      setTimeout(() => cb({ ok:true, _mock:true }), 0);
    },
    fetch_delete: (url, cb) => {
      events.push({ type:'fetch-delete', url });
      setTimeout(() => cb({ ok:true }), 0);
    },
    _err: (id, e, phase) => {
      events.push({ type:'error', island:id, phase, message:String(e?.message||e) });
    },
    mount: (id, renderFn) => {
      const el = vdoc.createElement('div');
      el.setAttribute('id', id);
      vdoc.body.appendChild(el);
      const update = () => {
        while (el.children[0]) el.removeChild(el.children[0]);
        el.children = [];
        try {
          const r = renderFn(update);
          if (r) el.appendChild(r);
          events.push({ type:'render', island:id });
        } catch(e) {
          events.push({ type:'error', island:id, phase:'render', message:String(e?.message||e) });
        }
      };
      update();
      return update;
    },
    mount_reactive: (id, renderFn, keys) => {
      const update = _fl.mount(id, renderFn);
      if (!update) return;
      _fl.watch_store((prev, next, changed) => {
        if (!keys || keys.includes(changed)) update();
      });
    },
  };

  const _fl_str = (...xs) => xs.map(String).join('');

  // island 런타임 전역 math/util (FL_ISLAND_RT 에 없고 _app.fl.out.js const 로 주입되는 것들)
  const round    = (n) => Math.round(n);
  const floor    = (n) => Math.floor(n);
  const ceil     = (n) => Math.ceil(n);
  const abs      = (n) => Math.abs(n);
  const min_val  = (a, b) => Math.min(a, b);
  const max_val  = (a, b) => Math.max(a, b);
  const math_sqrt = (n) => Math.sqrt(n);
  const math_pi   = Math.PI;
  const not     = (v) => !v;
  const nil_q   = (v) => v == null;
  const or      = (...xs) => xs.find(x => x != null && x !== false) ?? false;

  return { _fl, _fl_str, document: vdoc, window: { _fl }, events,
    round, floor, ceil, abs, min_val, max_val, math_sqrt, math_pi, not, nil_q, or,
    location:{ hostname:'headless' }, fetch:()=>{}, EventSource:()=>{},
    setTimeout: (fn,ms) => { try { fn(); } catch {} }, setInterval:()=>{},
    performance:{ now:()=>0 }, console:{ error:()=>{}, log:()=>{} } };
}

// ── 메인 실행 ────────────────────────────────────────────────
async function run(flxFile, opts={}) {
  const src = fs.readFileSync(flxFile, 'utf8');
  const islands = extractIslands(src);

  if (!islands.length) {
    return { ok:false, error:'island 블록 없음', file:flxFile };
  }

  const targetName = opts.island ? opts.island.toLowerCase() : null;
  const results = [];

  for (let idx = 0; idx < islands.length; idx++) {
    const { block } = islands[idx];
    const nameMatch = block.match(/\(island\s+([\w-]+)/);
    const name = nameMatch ? nameMatch[1].toLowerCase() : `island${idx}`;
    if (targetName && name !== targetName) continue;

    const { scripts } = compileFileIslands(block);
    if (!scripts.length) continue;

    const vdoc = makeVDocument();
    const ctx  = makeContext(vdoc);

    // store 초기값 주입
    if (opts.store) {
      for (const [k,v] of Object.entries(opts.store)) ctx._fl.dispatch(k,v);
      ctx.events.length = 0; // 초기화 이벤트 제거
    }

    // Island 실행
    try {
      const code = Object.entries(ctx).map(([k,v]) => `const ${k} = __ctx__.${k};`).join('\n')
        + '\n' + scripts.join('\n');
      new Function('__ctx__', code)(ctx);
    } catch(e) {
      results.push({ island:name, ok:false, error:e.message });
      continue;
    }

    // 초기 렌더 HTML
    const html = vdoc.body._serialize();

    // 이벤트 시뮬레이션
    const simEvents = [];
    if (opts.click) {
      const el = vdoc.body.querySelector(opts.click);
      if (el) { el.trigger('click', {}); simEvents.push('click:'+opts.click); }
      else simEvents.push('click:NOT_FOUND:'+opts.click);
    }
    if (opts.dispatch) {
      const [k,...rest] = opts.dispatch.split(':');
      ctx._fl.dispatch(k, rest.join(':'));
    }

    const htmlAfter = simEvents.length ? vdoc.body._serialize() : null;

    results.push({
      island: name,
      ok: !ctx.events.some(e=>e.type==='error'),
      html,
      html_after: htmlAfter,
      store: ctx._fl._store.v,
      events: ctx.events,
      sim: simEvents,
    });
  }

  return { ok: results.every(r=>r.ok), results, file: path.basename(flxFile) };
}

// ── CLI ──────────────────────────────────────────────────────
if (require.main === module) {
  const args = process.argv.slice(2);
  const file = args.find(a => !a.startsWith('--'));
  const islandArg = args.find(a => a.startsWith('--island='))?.split('=')[1];
  const clickArg  = args.find(a => a.startsWith('--click='))?.split('=')[1];
  const dispArg   = args.find(a => a.startsWith('--dispatch='))?.split('=')[1];
  const storeArg  = args.find(a => a.startsWith('--store='))?.split('=')[1];

  if (!file) {
    console.error('사용법: node island-runner.js <file.flx> [--island=Name] [--click=#id] [--dispatch=key:val] [--store={"key":0}]');
    process.exit(1);
  }

  let store = {};
  if (storeArg) try { store = JSON.parse(storeArg); } catch {}

  run(path.resolve(file), { island:islandArg, click:clickArg, dispatch:dispArg, store })
    .then(r => { console.log(JSON.stringify(r, null, 2)); process.exit(r.ok ? 0 : 1); })
    .catch(e => { console.error(JSON.stringify({ok:false,error:e.message})); process.exit(1); });
}

module.exports = { run };
