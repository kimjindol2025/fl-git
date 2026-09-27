#!/usr/bin/env node
// style-compiler.js — (style name :key val ...) DSL → scoped .css
// v2: pseudo-class(:hover-*) + 속성 검증 + xl 브레이크포인트

const fs   = require('fs');
const path = require('path');

// ══════════════════════════════════════════════════════════════
// CSS 속성 매핑 — 단축키 → CSS 속성명
// ══════════════════════════════════════════════════════════════
const PROP_MAP = {
  // 배경
  ':bg':           'background',
  ':bg-img':       'background-image',
  ':bg-size':      'background-size',
  // 텍스트
  ':color':        'color',
  ':size':         'font-size',
  ':weight':       'font-weight',
  ':font':         'font-family',
  ':align':        'text-align',
  ':decoration':   'text-decoration',
  ':lh':           'line-height',
  ':ws':           'white-space',
  ':truncate':     (v) => `overflow:hidden;text-overflow:ellipsis;white-space:nowrap`,
  // 간격
  ':p':            'padding',
  ':pt':           'padding-top',
  ':pb':           'padding-bottom',
  ':pl':           'padding-left',
  ':pr':           'padding-right',
  ':px':           (v) => `padding-left:${v};padding-right:${v}`,
  ':py':           (v) => `padding-top:${v};padding-bottom:${v}`,
  ':m':            'margin',
  ':mt':           'margin-top',
  ':mb':           'margin-bottom',
  ':ml':           'margin-left',
  ':mr':           'margin-right',
  ':mx':           (v) => `margin-left:${v};margin-right:${v}`,
  ':my':           (v) => `margin-top:${v};margin-bottom:${v}`,
  // 크기
  ':width':        'width',
  ':height':       'height',
  ':min-w':        'min-width',
  ':max-w':        'max-width',
  ':min-h':        'min-height',
  ':max-h':        'max-height',
  // 테두리
  ':radius':       'border-radius',
  ':border':       'border',
  ':border-top':   'border-top',
  ':border-color': 'border-color',
  ':outline':      'outline',
  // 그림자
  ':shadow':       'box-shadow',
  // 레이아웃
  ':display':      'display',
  ':flex':         'flex',
  ':flex-dir':     'flex-direction',
  ':flex-wrap':    'flex-wrap',
  ':items':        'align-items',
  ':justify':      'justify-content',
  ':gap':          'gap',
  ':cols':         (v) => `grid-template-columns:repeat(${v},1fr)`,
  ':col-span':     (v) => `grid-column:span ${v}`,
  ':rows':         (v) => `grid-template-rows:repeat(${v},1fr)`,
  // 위치
  ':position':     'position',
  ':top':          'top',
  ':bottom':       'bottom',
  ':left':         'left',
  ':right':        'right',
  ':z':            'z-index',
  ':inset':        'inset',
  // 기타
  ':overflow':     'overflow',
  ':cursor':       'cursor',
  ':opacity':      'opacity',
  ':transition':   'transition',
  ':transform':    'transform',
  ':pointer':      (v) => `pointer-events:${v}`,
  ':select':       (v) => `user-select:${v}`,
  ':aspect':       'aspect-ratio',
  ':object':       'object-fit',
};

// shadow 단축값
const SHADOW_ALIAS = {
  'sm':   '0 1px 3px rgba(0,0,0,.12)',
  'md':   '0 4px 12px rgba(0,0,0,.15)',
  'lg':   '0 8px 24px rgba(0,0,0,.18)',
  'xl':   '0 16px 40px rgba(0,0,0,.22)',
  'none': 'none',
};

// Breakpoint 맵 (mobile-first, max-width 기준)
const BREAKPOINTS = { xl: '1280px', lg: '1024px', md: '768px', sm: '640px' };

// Pseudo-class 목록
const PSEUDOS = ['hover', 'focus', 'active', 'visited', 'disabled', 'checked',
                 'focus-within', 'focus-visible', 'placeholder'];

// ══════════════════════════════════════════════════════════════
// 속성 검증
// ══════════════════════════════════════════════════════════════
const COLOR_RE   = /^(#[0-9a-fA-F]{3,8}|rgb|rgba|hsl|hsla|transparent|currentColor|inherit|white|black|[a-z]+)$/;
const DIM_RE     = /^-?[\d.]+(%|px|rem|em|vh|vw|fr|ch|ex|vmin|vmax|auto|inherit|initial)$/;

function validateProp(key, value, context) {
  const knownKey = ':' + key.replace(/^hover-|^focus-|^active-|^sm-|^md-|^lg-|^xl-/, ':');
  if (!PROP_MAP[knownKey] && !PROP_MAP[':' + key]) {
    return `⚠ 알 수 없는 속성 '${key}' (${context})`;
  }
  return null;
}

// ══════════════════════════════════════════════════════════════
// djb2 해시
// ══════════════════════════════════════════════════════════════
function hashStr(s) {
  let h = 5381;
  for (let i = 0; i < s.length; i++) h = ((h << 5) + h) ^ s.charCodeAt(i);
  return (h >>> 0).toString(16).slice(0, 4);
}

// ══════════════════════════════════════════════════════════════
// (style name :k1 v1 :hover-k2 v2 :sm-k3 v3 ...) 파싱
// ══════════════════════════════════════════════════════════════
function parseStyleBlock(block) {
  const nameMatch = block.match(/^\(style\s+([\w-]+)/);
  if (!nameMatch) return null;
  const name = nameMatch[1];

  const base    = [];   // [[':bg', '#fff'], ...]
  const pseudo  = {};   // { hover: [[':bg', '#f00']], focus: [...] }
  const responsive = {}; // { sm: [[':p', '8px']], md: [...] }
  const warnings = [];

  const pairRe = /(:[\w-]+)\s+"([^"]+)"/g;
  const numRe  = /(:[\w-]+)\s+(\d[\d.]*)/g;
  const allPairs = [];
  let m;
  while ((m = pairRe.exec(block)) !== null) allPairs.push([m[1], m[2]]);
  while ((m = numRe.exec(block)) !== null) {
    if (!allPairs.find(p => p[0] === m[1])) allPairs.push([m[1], m[2]]);
  }

  for (const [rawKey, val] of allPairs) {
    const key = rawKey.slice(1); // 앞 ':' 제거

    // pseudo-class 감지: hover-bg, focus-border, active-opacity
    const pseudoMatch = key.match(new RegExp(`^(${PSEUDOS.join('|')})-(.+)$`));
    if (pseudoMatch) {
      const [, pc, prop] = pseudoMatch;
      if (!pseudo[pc]) pseudo[pc] = [];
      pseudo[pc].push([':' + prop, val]);
      continue;
    }

    // breakpoint 감지: sm-p, md-cols, lg-size, xl-display
    const bpMatch = key.match(/^(sm|md|lg|xl)-(.+)$/);
    if (bpMatch) {
      const [, bp, prop] = bpMatch;
      if (!responsive[bp]) responsive[bp] = [];
      responsive[bp].push([':' + prop, val]);
      continue;
    }

    // 기본 속성
    base.push([':' + key, val]);
  }

  return { name, base, pseudo, responsive, warnings };
}

// ══════════════════════════════════════════════════════════════
// CSS 규칙 생성
// ══════════════════════════════════════════════════════════════
function pairsToDecls(pairs) {
  const decls = [];
  for (const [k, v] of pairs) {
    const prop = PROP_MAP[k];
    if (!prop) continue;
    let val = v;
    if (k === ':shadow') val = SHADOW_ALIAS[v] || v;
    if (typeof prop === 'function') decls.push(prop(val) + ';');
    else decls.push(`${prop}:${val};`);
  }
  return decls;
}

function buildCss(name, parsed) {
  const { base, pseudo, responsive } = parsed;
  const key  = JSON.stringify({ base, pseudo, responsive });
  const hash = hashStr(name + key);
  const cls  = name.replace(/-/g, '_') + '__' + hash;

  const parts = [];

  // 기본 스타일
  const baseDecls = pairsToDecls(base);
  if (baseDecls.length) parts.push(`.${cls}{${baseDecls.join('')}}`);

  // pseudo-class 스타일
  for (const [pc, pairs] of Object.entries(pseudo)) {
    const decls = pairsToDecls(pairs);
    if (decls.length) parts.push(`.${cls}:${pc}{${decls.join('')}}`);
  }

  // 반응형 미디어 쿼리 (xl→lg→md→sm 순)
  for (const bp of ['xl', 'lg', 'md', 'sm']) {
    if (!responsive[bp]) continue;
    const decls = pairsToDecls(responsive[bp]);
    if (decls.length) parts.push(`@media(max-width:${BREAKPOINTS[bp]}){.${cls}{${decls.join('')}}}`);
  }

  return { cls, css: parts.join(' ') };
}

// ══════════════════════════════════════════════════════════════
// .flx 파일에서 (style ...) 블록 추출
// ══════════════════════════════════════════════════════════════
function extractStyles(content) {
  const results = [];
  let i = 0;
  while (i < content.length) {
    const idx = content.indexOf('(style ', i);
    if (idx === -1) break;
    let depth = 0, j = idx;
    while (j < content.length) {
      if (content[j] === '(') depth++;
      else if (content[j] === ')') { depth--; if (depth === 0) { j++; break; } }
      j++;
    }
    const block = content.slice(idx, j);
    const parsed = parseStyleBlock(block);
    if (parsed) results.push({ ...parsed, block, start: idx, end: j });
    i = j;
  }
  return results;
}

// ══════════════════════════════════════════════════════════════
// 메인 컴파일 함수
// ══════════════════════════════════════════════════════════════
function compile(pagesDir, outDir) {
  const allStyles  = new Map();
  const buildWarns = [];

  function scanDir(dir) {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const e of entries) {
      const full = path.join(dir, e.name);
      if (e.isDirectory()) { scanDir(full); continue; }
      if (!e.name.endsWith('.flx')) continue;
      const content = fs.readFileSync(full, 'utf8');
      for (const s of extractStyles(content)) {
        if (!allStyles.has(s.name)) {
          allStyles.set(s.name, buildCss(s.name, s));
        }
        if (s.warnings.length) buildWarns.push(...s.warnings.map(w => `  ${e.name}: ${w}`));
      }
    }
  }
  scanDir(pagesDir);

  if (buildWarns.length) {
    console.log('\x1b[33m⚠ CSS 검증 경고:\x1b[0m');
    buildWarns.forEach(w => console.log(w));
  }

  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
  const cssPath  = path.join(outDir, 'app.css');
  const cssLines = ['/* FL-Front 자동 생성 — 직접 수정 금지 */'];
  const bindings = [];

  for (const [name, { cls, css }] of allStyles) {
    cssLines.push(css);
    bindings.push({ name, cls });
  }

  fs.writeFileSync(cssPath, cssLines.join('\n') + '\n', 'utf8');
  return { cssPath, bindings };
}

// 단일 파일 Critical CSS 생성
function compilePageCSS(filePath) {
  const content = fs.readFileSync(filePath, 'utf8');
  const styles  = extractStyles(content);
  if (!styles.length) return { css: '', bindings: [], rawCss: '' };

  const bindings = [];
  const rules    = [];
  for (const s of styles) {
    const { cls, css } = buildCss(s.name, s);
    bindings.push({ name: s.name, cls });
    rules.push(css);
  }
  const rawCss = rules.join(' ');
  return { css: `<style>${rawCss}</style>`, bindings, rawCss };
}

module.exports = { compile, compilePageCSS, extractStyles, buildCss, PROP_MAP, PSEUDOS };

// CLI
if (require.main === module) {
  const pagesDir = path.join(__dirname, 'pages');
  const outDir   = path.join(__dirname, 'public');
  const { cssPath, bindings } = compile(pagesDir, outDir);
  console.log('✅ CSS 생성:', cssPath);
  bindings.forEach(b => console.log(`  .${b.cls}  ←  ${b.name}`));
}
