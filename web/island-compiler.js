#!/usr/bin/env node
// island-compiler.js — (island Name [] body) → vanilla JS emit

// ── S식 파서 ─────────────────────────────────────────────────
function parse(src, baseLineOffset = 0) {
  let i = 0, line = 1 + baseLineOffset;
  function skipWs() {
    while (i < src.length && /[\s,]/.test(src[i])) {
      if (src[i] === '\n') line++;
      i++;
    }
    if (src[i] === ';') { while (i < src.length && src[i] !== '\n') i++; skipWs(); }
  }
  function readAtom() {
    let s = '';
    while (i < src.length && !/[\s,(){}[\]"]/.test(src[i])) s += src[i++];
    if (s === 'nil') return null;
    if (s === 'true') return true;
    if (s === 'false') return false;
    if (!isNaN(s) && s !== '') return Number(s);
    return { sym: s };
  }
  function readStr() {
    i++; // skip "
    let s = '';
    while (i < src.length && src[i] !== '"') {
      if (src[i] === '\n') line++;
      if (src[i] === '\\') { i++; s += src[i++]; } else s += src[i++];
    }
    i++; // skip "
    return s;
  }
  function readList(close) {
    i++; // skip open
    const startLine = line;
    const items = [];
    skipWs();
    while (i < src.length && src[i] !== close) {
      const before = i;
      items.push(read());
      skipWs();
      if (i === before) {
        const ctx = src.slice(Math.max(0, i - 40), i + 5);
        throw new Error(`island 파싱 오류 (line ${line}): '${close}'를 기대했으나 예상치 못한 '${src[i]}' — 괄호 짝을 확인하세요.\n  …${ctx}`);
      }
    }
    i++; // skip close
    const result = items;
    result._line = startLine;
    return result;
  }
  function read() {
    skipWs();
    if (i >= src.length) return null;
    const c = src[i];
    if (c === '(') return readList(')');
    if (c === '[') return readList(']');
    if (c === '{') {
      const pairs = readList('}');
      const obj = {};
      for (let j = 0; j < pairs.length; j += 2) {
        const k = pairs[j];
        obj[typeof k === 'string' ? k : k?.kw ?? k?.sym ?? String(k)] = pairs[j + 1];
      }
      return { map: obj };
    }
    if (c === '"') return readStr();
    if (c === '@') { i++; return [{ sym: 'deref' }, read()]; }
    if (c === ':') { i++; return { kw: readAtom().sym }; }
    return readAtom();
  }
  return read();
}

// ── FL AST → JS 코드 생성 ────────────────────────────────────
function emit(node, ctx = { needsUpdate: false }) {
  if (node === null) return 'null';
  if (node === true) return 'true';
  if (node === false) return 'false';
  if (typeof node === 'number') return String(node);
  if (typeof node === 'string') return JSON.stringify(node);
  if (Array.isArray(node)) return emitList(node, ctx);   // 배열 먼저 — node?.map은 Array.prototype.map과 충돌
  if (node?.kw)  return JSON.stringify(node.kw);
  if (node?.sym) return emitSym(node.sym);
  if (node?.map) return emitMap(node.map, ctx);
  return 'null';
}

const SYM_MAP = {
  'inc':   'v => v + 1',
  'dec':   'v => v - 1',
  'not':   'v => !v',
  'str':   '_fl_str',
  'nil?':  'v => v == null',
  'empty?':'v => !v || v.length === 0',
};
function emitSym(s) {
  if (SYM_MAP[s]) return SYM_MAP[s];
  return s.replace(/-/g, '_').replace(/\?$/, '_q').replace(/!$/, '_bang');
}

function emitMap(obj, ctx) {
  const pairs = Object.entries(obj).map(([k, v]) => {
    const kStr = k.startsWith(':') ? JSON.stringify(k.slice(1)) : JSON.stringify(k);
    return `${kStr}: ${emit(v, ctx)}`;
  });
  return `{${pairs.join(', ')}}`;
}

function emitList(node, ctx) {
  if (!node.length) return '[]';
  const head = node[0];
  const args = node.slice(1);

  // head가 sym이 아니면 벡터 리터럴 [v1 v2 ...]
  if (head?.sym === undefined && !Array.isArray(head)) {
    return `[${node.map(n => emit(n, ctx)).join(', ')}]`;
  }

  if (head?.sym === 'island') return emitIsland(args, ctx);
  if (head?.sym === 'let')    return emitLet(args, ctx);
  if (head?.sym === 'fn')     return emitFn(args, ctx);
  if (head?.sym === 'if')     return emitIf(args, ctx);
  if (head?.sym === 'cond')   return emitCond(args, ctx);
  if (head?.sym === 'do')     return emitDo(args, ctx);
  if (head?.sym === 'atom')   return `_fl.atom(${emit(args[0], ctx)})`;
  if (head?.sym === 'deref')  return `_fl.deref(${emit(args[0], ctx)})`;
  if (head?.sym === 'swap!')  return emitSwap(args, ctx);
  if (head?.sym === 'reset!') return emitReset(args, ctx);
  if (head?.sym === 'h')      return emitH(args, ctx);
  if (head?.sym === 'h!')     return emitHBang(args, ctx);
  if (head?.sym === 'str')    return emitStr(args, ctx);
  if (head?.sym === '+')      return `(${args.map(a=>emit(a,ctx)).join(' + ')})`;
  if (head?.sym === '-')      return `(${args.map(a=>emit(a,ctx)).join(' - ')})`;
  if (head?.sym === '*')      return `(${args.map(a=>emit(a,ctx)).join(' * ')})`;
  if (head?.sym === '/')      return `(${args.map(a=>emit(a,ctx)).join(' / ')})`;
  if (head?.sym === '=')      return `(${emit(args[0],ctx)} === ${emit(args[1],ctx)})`;
  if (head?.sym === 'not=')   return `(${emit(args[0],ctx)} !== ${emit(args[1],ctx)})`;
  if (head?.sym === '>')      return `(${emit(args[0],ctx)} > ${emit(args[1],ctx)})`;
  if (head?.sym === '<')      return `(${emit(args[0],ctx)} < ${emit(args[1],ctx)})`;
  if (head?.sym === '>=')     return `(${emit(args[0],ctx)} >= ${emit(args[1],ctx)})`;
  if (head?.sym === '<=')     return `(${emit(args[0],ctx)} <= ${emit(args[1],ctx)})`;
  if (head?.sym === 'mod')    return `(${emit(args[0],ctx)} % ${emit(args[1],ctx)})`;
  if (head?.sym === '??')     return `(${emit(args[0],ctx)} ?? ${emit(args[1],ctx)})`;  // nil-coalescing
  if (head?.sym === 'not')    return `!(${emit(args[0],ctx)})`;
  // (when test a b…) — body 여러 개면 do 로 묶음 (notes-island 패턴)
  if (head?.sym === 'when') {
    if (args.length <= 2) return `(${emit(args[0],ctx)}) ? (${emit(args[1],ctx)}) : null`;
    return `(${emit(args[0],ctx)}) ? (${emitDo(args.slice(1), ctx)}) : null`;
  }
  if (head?.sym === 'or')     return `(${args.map(a=>`(${emit(a,ctx)})`).join(' || ')})`;
  if (head?.sym === 'and')    return `(${args.map(a=>`(${emit(a,ctx)})`).join(' && ')})`;
  // 배열/맵 조작 — _fl 런타임 위임
  if (head?.sym === 'push')    return `_fl.push(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'get')     return `_fl.get(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'get-in')  return emitGetIn(args, ctx);
  if (head?.sym === 'assoc')   return `_fl.assoc(${emit(args[0],ctx)}, ${emit(args[1],ctx)}, ${emit(args[2],ctx)})`;
  if (head?.sym === 'dissoc')  return `_fl.dissoc(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'map')     return `_fl.map_arr(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'filter')  return `_fl.filter_arr(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'reduce')  return `_fl.reduce_arr(${emit(args[0],ctx)}, ${emit(args[1],ctx)}, ${emit(args[2],ctx)})`;
  // 반드시 괄호로 감싼다 — ?? 가 === / > 보다 우선순위 낮음
  // 잘못: (x)?.length ?? 0 === 0  →  (x)?.length ?? (0 === 0)
  // 올바: ((x)?.length ?? 0) === 0
  if (head?.sym === 'length')  return `((${emit(args[0],ctx)})?.length ?? 0)`;
  if (head?.sym === 'first')   return `(${emit(args[0],ctx)})?.[0] ?? null`;
  if (head?.sym === 'rest')    return `(${emit(args[0],ctx)})?.slice(1) ?? []`;
  // ── 글로벌 store ──
  if (head?.sym === 'get-store')    return args.length ? `_fl.get_store(${emit(args[0],ctx)})` : `_fl.get_store()`;
  if (head?.sym === 'dispatch')     return `_fl.dispatch(${emit(args[0],ctx)}${args[1]!==undefined?','+emit(args[1],ctx):''})`;
  if (head?.sym === 'watch-store')  return `_fl.watch_store(${emit(args[0],ctx)})`;
  if (head?.sym === 'empty?')  return `(!((${emit(args[0],ctx)}) && (${emit(args[0],ctx)}).length > 0))`;
  // fetch! / fetch-post! — Island 내부 비동기 API 호출
  if (head?.sym === 'fetch!')        return `_fl.fetch(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'fetch-post!')   return `_fl.fetch_post(${emit(args[0],ctx)}, ${emit(args[1],ctx)}, ${emit(args[2],ctx)})`;
  if (head?.sym === 'fetch-delete!') return `_fl.fetch_delete(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  if (head?.sym === 'read-file-dataurl!') return `_fl.read_file_dataurl(${emit(args[0],ctx)}, ${emit(args[1],ctx)})`;
  // Clojure interop: (.-prop obj) → (obj).prop  — DOM 이벤트 값 접근 등
  if (head?.sym && head.sym.startsWith('.-'))
    return `(${emit(args[0], ctx)}).${head.sym.slice(2)}`;
  // Clojure interop: (.method obj a b) → (obj).method(a, b)
  if (head?.sym && head.sym.startsWith('.') && head.sym.length > 1)
    return `(${emit(args[0], ctx)}).${head.sym.slice(1)}(${args.slice(1).map(a => emit(a, ctx)).join(', ')})`;
  // 일반 함수 호출
  const fn = emit(head, ctx);
  return `${fn}(${args.map(a => emit(a, ctx)).join(', ')})`;
}

function emitGetIn(args, ctx) {
  // (get-in obj ["key1" "key2"]) → (obj?.["key1"]?.["key2"] ?? null)
  const obj  = emit(args[0], ctx);
  const keys = args[1]; // array AST node
  if (!Array.isArray(keys) || keys.length === 0) return `(${obj} ?? null)`;
  const chain = keys.reduce((acc, key) => `(${acc})?.[${emit(key, ctx)}]`, obj);
  return `(${chain} ?? null)`;
}

function emitIsland(args, ctx, islandId = null) {
  // (island Name [] body)
  // islandId: HMR soft-reload 시 atom 복원 키
  const body = args.slice(2);
  const hid = islandId ? JSON.stringify(islandId) : 'null';

  // 최상위 let 바인딩에서 atom 선언을 외부 클로저로 끌어올림
  // → update() 호출 시 atom이 재초기화되지 않고 상태 유지
  if (body.length === 1 && Array.isArray(body[0]) && body[0][0]?.sym === 'let') {
    const bindings = body[0][1]; // [name, expr, name2, expr2, ...]
    const letBody  = body[0].slice(2);

    const stateDecls  = []; // atom → 외부 클로저 (1회 생성)
    const renderDecls = []; // 그 외 → 렌더 함수 내부 (매 렌더마다)
    const atomNames   = [];
    for (let i = 0; i < bindings.length; i += 2) {
      const name    = emitSym(bindings[i]?.sym ?? String(bindings[i]));
      const valNode = bindings[i + 1];
      const val     = emit(valNode, ctx);
      const isAtom  = Array.isArray(valNode) && valNode[0]?.sym === 'atom';
      if (isAtom) {
        const initExpr = valNode[1] !== undefined ? emit(valNode[1], ctx) : 'null';
        const key = JSON.stringify(name);
        stateDecls.push(
          `const ${name} = _fl.atom((__saved&&Object.prototype.hasOwnProperty.call(__saved,${key}))?__saved[${key}]:${initExpr})`
        );
        atomNames.push(name);
      } else {
        renderDecls.push(`const ${name} = ${val}`);
      }
    }

    if (stateDecls.length > 0) {
      // 마지막 표현식만 return — 앞 표현식은 side-effect (fetch! 등)
      const bodyJs = letBody.map((n, i) =>
        i === letBody.length - 1 ? `return ${emit(n, ctx)}` : emit(n, ctx)
      ).join(';\n      ');
      const inner  = renderDecls.length > 0
        ? `${renderDecls.join('; ')}; ${bodyJs};`
        : `${bodyJs};`;
      const reg = atomNames.length
        ? `_fl.hmr_register(__hid,{${atomNames.map(n => `${n}:${n}`).join(',')}});`
        : '';
      return `(() => { const __hid=${hid}; const __saved=(_fl._pending_restore&&_fl._pending_restore[__hid])||{}; ${stateDecls.join('; ')}; ${reg}\n  return (update) => { ${inner} };\n})()`;
    }
  }

  const bodyJs = body.map(n => emit(n, ctx)).join(';\n    ');
  return `(update) => {\n    return ${bodyJs};\n  }`;
}

function emitLet(args, ctx) {
  // (let [x expr y expr] body...)
  const bindings = args[0]; // array
  const body = args.slice(1);
  const decls = [];
  for (let i = 0; i < bindings.length; i += 2) {
    const name = emitSym(bindings[i]?.sym ?? String(bindings[i]));
    const val  = emit(bindings[i + 1], ctx);
    decls.push(`const ${name} = ${val}`);
  }
  const bodyJs = body.map(n => emit(n, ctx)).join(';\n  ');
  return `(() => { ${decls.join('; ')}; return ${bodyJs}; })()`;
}

function emitFn(args, ctx) {
  const params = args[0].map(p => emitSym(p?.sym ?? String(p)));
  const body   = args.slice(1);
  const bodyJs = body.map((n, i) =>
    i === body.length - 1 ? `return ${emit(n, ctx)}` : emit(n, ctx)
  ).join('; ');
  return `(${params.join(', ')}) => { ${bodyJs}; }`;
}

function emitIf(args, ctx) {
  return `(${emit(args[0],ctx)}) ? (${emit(args[1],ctx)}) : (${emit(args[2]??null,ctx)})`;
}

// (cond test1 expr1 test2 expr2 … true else) → nested ternary
function emitCond(args, ctx) {
  if (args.length === 0) return 'null';
  let out = 'null';
  for (let i = args.length - 2; i >= 0; i -= 2) {
    const test = args[i];
    const expr = args[i + 1];
    if (!expr) break;
    // 마지막 true / :else 는 else 가지
    if (i === args.length - 2 && (test?.sym === 'true' || test === true || test?.sym === 'else' || test?.kw === 'else')) {
      out = emit(expr, ctx);
      continue;
    }
    out = `(${emit(test, ctx)}) ? (${emit(expr, ctx)}) : (${out})`;
  }
  return out;
}

function emitDo(args, ctx) {
  return `(() => { ${args.map(a=>emit(a,ctx)).join('; ')} })()`;
}

function emitSwap(args, ctx) {
  // (swap! atom fn) → (_fl.swap(atom, fn), update())
  return `(_fl.swap(${emit(args[0],ctx)}, ${emit(args[1],ctx)}), update())`;
}

function emitReset(args, ctx) {
  return `(_fl.reset(${emit(args[0],ctx)}, ${emit(args[1],ctx)}), update())`;
}

function emitH(args, ctx) {
  // (h "tag" {attrs} children...)
  const tag    = emit(args[0], ctx);
  const attrs  = args[1] ? emit(args[1], ctx) : '{}';
  const children = args.slice(2).map(a => emit(a, ctx));
  return `_fl.h(${tag}, ${attrs}${children.length ? ', ' + children.join(', ') : ''})`;
}

function emitHBang(args, ctx) {
  // (h! "input" {attrs}) — void 태그; _fl.h 로 createElement (자식 없음)
  const tag   = emit(args[0], ctx);
  const attrs = args[1] ? emit(args[1], ctx) : '{}';
  return `_fl.h(${tag}, ${attrs})`;
}

function emitStr(args, ctx) {
  return `_fl_str(${args.map(a => emit(a, ctx)).join(', ')})`;
}

// ── Island 런타임 (~900B) ────────────────────────────────────
const FL_ISLAND_RT = `<script>
const _fl={
  // ── 로컬 atom ──
  atom:(v)=>({v}),
  deref:(a)=>a.v,
  swap:(a,f)=>{a.v=f(a.v);return a.v;},
  reset:(a,v)=>{a.v=v;return v;},
  // ── 글로벌 store (Island 간 상태 공유) ──
  _store:{v:{}},
  _watchers:[],
  // HMR soft-reload: island atom/store 스냅샷
  _pending_restore:null,
  _hmr_islands:{},
  hmr_register:(id,atoms)=>{if(id)_fl._hmr_islands[id]=atoms;},
  hmr_snapshot:()=>{
    const islands={};
    for(const[id,atoms] of Object.entries(_fl._hmr_islands||{})){
      const row={};
      for(const[k,a] of Object.entries(atoms||{})) row[k]=a&&typeof a==='object'&&'v' in a?a.v:a;
      islands[id]=row;
    }
    return {store:{...(_fl._store.v||{})}, islands};
  },
  get_store:(key)=>key?(_fl._store.v[key]??null):_fl._store.v,
  dispatch:(key,val)=>{
    const prev=_fl._store.v;
    _fl._store.v=typeof key==='object'
      ? {...prev,...key}
      : {...prev,[key]:val};
    _fl._watchers.forEach(fn=>fn(prev,_fl._store.v,key));
  },
  watch_store:(fn)=>{_fl._watchers.push(fn);return ()=>{_fl._watchers=_fl._watchers.filter(w=>w!==fn);};},
  // ── 배열/맵 조작 ──
  push:(arr,item)=>[...(arr||[]),item],
  get:(obj,key)=>obj==null?null:obj[key]??null,
  assoc:(obj,key,val)=>({...(obj||{}),[key]:val}),
  dissoc:(obj,key)=>{const r={...(obj||{})};delete r[key];return r;},
  map_arr:(fn,arr)=>(arr||[]).map(fn),
  filter_arr:(fn,arr)=>(arr||[]).filter(fn),
  reduce_arr:(fn,init,arr)=>(arr||[]).reduce(fn,init),
  // ── DOM 빌더 ──
  h:(tag,attrs,...ch)=>{
    const el=document.createElement(tag);
    for(const[k,v]of Object.entries(attrs||{})){
      if(k==='checked'){if(v&&v!=='')el.checked=true;}
      else if(k.startsWith('on-'))el.addEventListener(k.slice(3),v);
      else if(v!==null&&v!==undefined&&v!=='')el.setAttribute(k,String(v));
    }
    function app(c){
      if(c==null)return;
      if(Array.isArray(c)){c.forEach(app);return;}
      if(c instanceof Node)el.appendChild(c);
      else{const s=String(c);if(s)el.appendChild(document.createTextNode(s));}
    }
    ch.forEach(app);
    return el;
  },
  // ── fetch! — Island 내부 API 호출 (GET, JSON 자동 파싱) ──
  // 사용: (fetch! "/api/users" (fn [data] (reset! items data)))
  fetch:(url,cb)=>{
    fetch(url,{headers:{'Accept':'application/json'}})
      .then(r=>r.ok?r.json():Promise.reject(r.status))
      .then(data=>{ try{ cb(data); }catch(e){ _fl._err('fetch!',e,'callback'); } })
      .catch(err=>{ _fl._err('fetch!',new Error('GET '+url+' → '+err),'request'); });
  },
  // fetch-post! — POST JSON → Island 콜백
  fetch_post:(url,body,cb)=>{
    fetch(url,{method:'POST',headers:{'Content-Type':'application/json','Accept':'application/json'},body:JSON.stringify(body)})
      .then(r=>r.ok?r.json():Promise.reject(r.status))
      .then(data=>{ try{ cb(data); }catch(e){ _fl._err('fetch-post!',e,'callback'); } })
      .catch(err=>{ _fl._err('fetch-post!',new Error('POST '+url+' → '+err),'request'); });
  },
  read_file_dataurl:(file,cb)=>{
    if(!file){cb(null);return;}
    const r=new FileReader();
    r.onload=()=>{try{cb(r.result);}catch(e){_fl._err('read-file',e,'callback');}};
    r.onerror=()=>{ _fl._err('read-file',new Error('FileReader failed'),'request'); cb(null); };
    r.readAsDataURL(file);
  },
  // fetch-delete! — DELETE → Island 콜백
  fetch_delete:(url,cb)=>{
    fetch(url,{method:'DELETE',headers:{'Accept':'application/json'}})
      .then(r=>r.ok?r.json():Promise.reject(r.status))
      .then(data=>{ try{ cb(data); }catch(e){ _fl._err('fetch-delete!',e,'callback'); } })
      .catch(err=>{ _fl._err('fetch-delete!',new Error('DELETE '+url+' → '+err),'request'); });
  },
  // ── Island 에러 리포터 (콘솔 + Next.js 스타일 오버레이)
  _err:(islandId,err,phase)=>{
    const msg=err&&err.message?err.message:String(err);
    const stack=err&&err.stack?err.stack:'';
    console.error('[Island:'+islandId+']['+phase+'] '+msg+String.fromCharCode(10)+stack);
    try{
      if(window.__FL_EO&&window.__FL_EO.report){
        window.__FL_EO.report({
          type:'island',
          message:'['+islandId+']['+phase+'] '+msg,
          source:'island-'+islandId+'.flx',
          line:0,
          stack:stack
        });
      }
    }catch(e){}
  },
  // ── Island 마운트 (store watch 통합) ──
  mount:(id,renderFn)=>{
    const el=document.getElementById(id);
    if(!el)return;
    const update=()=>{
      try{
        while(el.firstChild)el.removeChild(el.firstChild);
        const r=renderFn(update);
        if(r)el.appendChild(r);
      }catch(e){
        _fl._err(id,e,'render');
        el.innerHTML='<div style="color:#ef4444;font-family:monospace;font-size:.8rem;padding:8px;background:#1a1a1a;border-radius:4px">[Island Error] '+String(e.message)+'</div>';
      }
    };
    try{ update(); }catch(e){ _fl._err(id,e,'mount'); }
    return update;
  },
  // ── store 구독 + 자동 재렌더 ──
  mount_reactive:(id,renderFn,keys)=>{
    const update=_fl.mount(id,renderFn);
    if(!update)return;
    _fl.watch_store((prev,next,changed)=>{
      if(!keys||keys.includes(changed))update();
    });
  },
};
// str: Node 포함 시 children 컨텍스트용 배열 반환, 순수 문자열이면 join
// 주의: 속성값에 Node를 섞어 쓰는 패턴은 지원하지 않음 (String([node]) → 깨짐)
function _fl_str(...xs){
  const flat=[];
  function col(v){if(v==null)return;if(v instanceof Node){flat.push(v);return;}if(Array.isArray(v)){v.forEach(col);return;}flat.push(String(v));}
  xs.forEach(col);
  return flat.some(x=>x instanceof Node)?flat:flat.join('');
}
const floor=Math.floor,ceil=Math.ceil,round=Math.round,abs=Math.abs;
window._fl=_fl;
// HMR soft-reload 복원 — island mount 전에 store/atom pending 주입
(function(){
  try{
    const raw=sessionStorage.getItem('__fl_hmr');
    if(!raw)return;
    sessionStorage.removeItem('__fl_hmr');
    const snap=JSON.parse(raw);
    if(snap&&snap.store&&typeof snap.store==='object') _fl._store.v={..._fl._store.v,...snap.store};
    if(snap&&snap.islands&&typeof snap.islands==='object') _fl._pending_restore=snap.islands;
  }catch(e){}
})();
// ── Island 전역 헬퍼 (arr-join, concat, str-* 등) ──
const arr_join=(arr,sep)=>{const a=arr||[];return a.some(x=>x instanceof Node)?a:a.join(sep??'');};
const concat=(a,b)=>[...(a||[]),...(b||[])];
const str_contains=(s,sub)=>String(s??'').includes(String(sub??''));
const str_blank_q=(s)=>!s||String(s).trim()==='';
const str_slice=(s,start,end)=>String(s??'').slice(start,end);
const str_trim=(s)=>String(s??'').trim();
const str_split=(s,sep)=>String(s??'').split(sep);
const js_prompt=(msg,def)=>window.prompt(msg,def??'');
const js_confirm=(msg)=>window.confirm(msg);
</script>`;

// ── .flx 파일에서 (island ...) 블록 추출 ────────────────────
function extractIslands(content) {
  const islands = [];
  let i = 0;
  while (i < content.length) {
    const idx = content.indexOf('(island ', i);
    if (idx === -1) break;
    const startLine = content.slice(0, idx).split('\n').length;
    let depth = 0, j = idx;
    while (j < content.length) {
      if (content[j] === '(') depth++;
      else if (content[j] === ')') { depth--; if (depth === 0) { j++; break; } }
      j++;
    }
    const block = content.slice(idx, j);
    islands.push({ block, start: idx, end: j, startLine });
    i = j;
  }
  return islands;
}

// ── 파일 내 island 컴파일 ────────────────────────────────────
// 반환: { html: 수정된 HTML 스니펫, scripts: [JS 코드], hasIslands }
function compileFileIslands(flxContent) {
  const found = extractIslands(flxContent);
  if (!found.length) return { flxContent, scripts: [], hasIslands: false };

  const scripts = [];
  let modified  = flxContent;
  let offset    = 0;

  found.forEach(({ block, start, end, startLine = 1 }, idx) => {
    const ast  = parse(block, startLine - 1);
    const name = ast[1]?.sym ?? `Island${idx}`;
    const id   = `island-${name.toLowerCase()}-${idx}`;
    // 두 번째 인자가 배열이면 옵션 맵으로 해석 ([:watch "key1" ...])
    const opts = Array.isArray(ast[2]) ? ast[2] : null;
    const watchKeys = opts
      ? opts.reduce((acc, v, i) => {
          if (v?.kw === 'watch') acc.push(...opts.slice(i+1).filter(x => typeof x === 'string'));
          return acc;
        }, [])
      : [];
    const renderFn = emitIsland(ast.slice(1), {}, id);
    const mountCall = watchKeys.length
      ? `_fl.mount_reactive(${JSON.stringify(id)}, ${renderFn}, ${JSON.stringify(watchKeys)});`
      : `_fl.mount(${JSON.stringify(id)}, ${renderFn});`;
    const js = `${mountCall}\n//# sourceURL=island-${name.toLowerCase()}.flx`;
    scripts.push(js);
    // island 블록 → <div id="..."></div> 플레이스홀더로 교체
    const placeholder = `"<div id='${id}'></div>"`;
    modified = modified.slice(0, start + offset) + placeholder + modified.slice(end + offset);
    offset += placeholder.length - (end - start);
  });

  return { flxContent: modified, scripts, hasIslands: true };
}

module.exports = { compileFileIslands, FL_ISLAND_RT, extractIslands };

// CLI 테스트
if (require.main === module) {
  const testFlx = `
(island Counter []
  (let [n (atom 0)]
    (h "button" {:on-click (fn [] (swap! n inc))} (str @n))))
`;
  const { flxContent, scripts } = compileFileIslands(testFlx);
  const rt = FL_ISLAND_RT.replace(/<\/?script>/g, '');
  const total = Buffer.byteLength(rt + scripts.join('\n'), 'utf8');
  console.log('placeholder:', flxContent.trim());
  console.log('JS:');
  console.log(scripts.join('\n'));
  console.log(`총 크기: ${total}B (${total < 2048 ? '✅ < 2KB' : '❌ >= 2KB'})`);
}
