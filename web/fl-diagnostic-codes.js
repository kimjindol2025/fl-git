#!/usr/bin/env node
/**
 * fl-diagnostic-codes.js — FreeLang Front 진단 코드 카탈로그 (단일 소스)
 * AI-Interface Phase 2 / P5
 *
 * schema 필드 계약 (Klar 정렬):
 *   code, title, severity, fixable, applicability, hint
 *
 * applicability:
 *   machine-applicable — 기계가 안전하게 고칠 수 있음 (P6 후보)
 *   has-suggestions    — 힌트/제안 가능, 자동적용은 위험
 *   manual             — 사람·에이전트 판단 필요
 *
 * 사용:
 *   const { DIAGNOSTIC_CODES, byCode, hints } = require('./fl-diagnostic-codes');
 *   node fl-manifest.js   → diagnosticCodes 포함
 */

'use strict';

/** @type {Array<{
 *   code: string,
 *   title: string,
 *   severity: 'error'|'warning',
 *   fixable: boolean,
 *   applicability: 'machine-applicable'|'has-suggestions'|'manual',
 *   hint: string
 * }>} */
const DIAGNOSTIC_CODES = [
  // ── errors ──────────────────────────────────────────────
  {
    code: 'reserved-word',
    title: '예약어를 식별자로 사용',
    severity: 'error',
    fixable: true,
    applicability: 'machine-applicable',
    hint: '예약어 대신 다른 식별자 사용 (예: act → action). let 바인딩·defn·파라미터 모두 검사',
  },
  {
    code: 'island-boundary',
    title: 'Island 안에서 서버 전용 함수 호출',
    severity: 'error',
    fixable: false,
    applicability: 'manual',
    hint: '서버 전용 호출을 island 밖으로 이동',
  },
  {
    code: 'island-test',
    title: 'Island 단위 테스트 실패',
    severity: 'error',
    fixable: false,
    applicability: 'manual',
    hint: 'tests/*.test.flx 기대값 확인',
  },

  // ── Trap / DX (2026-07 추가) ────────────────────────────
  {
    code: 'json-parse-hyphen',
    title: 'json-parse 하이픈 형태',
    severity: 'warning',
    fixable: true,
    applicability: 'machine-applicable',
    hint: '(json_parse …) 로 교체',
  },
  {
    code: 'upper-hyphen',
    title: '대문자+다중하이픈 식별자',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: '소문자 snake_case 로 rename (예: afl_db_base) — 일괄 rename은 회귀 위험',
  },
  {
    code: 'let-shadow',
    title: 'let 이 전역 헬퍼를 가림',
    severity: 'warning',
    fixable: true,
    applicability: 'machine-applicable',
    hint: 'let 바인딩을 긴 이름으로 (예: sess, stats-body)',
  },
  {
    code: 'fl-http-allow',
    title: 'private HTTP에 FL_HTTP_ALLOW 미설정',
    severity: 'warning',
    fixable: false,
    applicability: 'manual',
    hint: 'FL_HTTP_ALLOW=127.0.0.1,localhost 설정 (ecosystem/env)',
  },
  {
    code: 'helper-load-order',
    title: '헬퍼 파일명이 부모보다 먼저 정렬',
    severity: 'warning',
    fixable: true,
    applicability: 'machine-applicable',
    hint: '_afldb-coll.flx → _afldb_coll.flx (하이픈을 underscore로)',
  },
  {
    code: 'req-key-collision',
    title: 'req body 예약 키 text 사용',
    severity: 'warning',
    fixable: true,
    applicability: 'machine-applicable',
    hint: '(get body "text") / :name "text" → content 등 (raw body 충돌)',
  },

  // ── 기존 체커 ───────────────────────────────────────────
  {
    code: 'undefined-fn',
    title: '정의되지 않은 함수 호출',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: '함수 정의 또는 stdlib/헬퍼 이름 확인',
  },
  {
    code: 'store-key',
    title: '미선언 store 키',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: '(defstore :key init) 추가',
  },
  {
    code: 'arity',
    title: '페이지/헬퍼 함수 인자 수 불일치',
    severity: 'warning',
    fixable: false,
    applicability: 'manual',
    hint: '호출 인자 수를 정의와 맞추기 — (defn f [a b])면 (f x y). 가변인자 없음. 자동 rename 위험 → 사람 확인',
  },
  {
    code: 'builtin-arity',
    title: 'stdlib 내장 함수 인자 수 불일치',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: 'stdlib 시그니처 확인 — 인자 수·순서 고정. --codes / hint 참고',
  },
  {
    code: 'comment-defn',
    title: '주석 속 (defn …)',
    severity: 'warning',
    fixable: false,
    applicability: 'manual',
    hint: '주석 속 defn 제거 또는 실제 코드로 이동',
  },
  {
    code: 'global-shadow',
    title: '파라미터가 전역 헬퍼를 가림',
    severity: 'warning',
    fixable: true,
    applicability: 'has-suggestions',
    hint: '파라미터를 긴 이름으로 (1글자 변수명 금지)',
  },
  {
    code: 'route-shadow',
    title: '동적 라우트가 정적을 가로챌 수 있음',
    severity: 'warning',
    fixable: false,
    applicability: 'manual',
    hint: '정적 라우트가 동적보다 먼저 등록되는지 확인',
  },
  {
    code: 'db-row-mutation',
    title: 'db-query 행에 맵 변형 시도',
    severity: 'warning',
    fixable: false,
    applicability: 'manual',
    hint: 'get 만 사용하거나 명시적 맵으로 재구성',
  },
  {
    code: 'nullable-consume',
    title: 'null 가능 값을 가드 없이 소비',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: '(or x 기본값) / req-query-default 로 가드',
  },
  {
    code: 'str-loop-concat',
    title: '루프 내 (str …) 누적',
    severity: 'warning',
    fixable: false,
    applicability: 'has-suggestions',
    hint: '배열 누적 후 str-join 권장 (O(n²) GC)',
  },
];

function byCode(code) {
  return DIAGNOSTIC_CODES.find(c => c.code === code) || null;
}

function hints() {
  const m = {};
  for (const c of DIAGNOSTIC_CODES) m[c.code] = c.hint;
  return m;
}

function machineApplicable() {
  return DIAGNOSTIC_CODES.filter(c => c.applicability === 'machine-applicable');
}

module.exports = {
  schema: 'fl-diagnostic-codes/1',
  DIAGNOSTIC_CODES,
  byCode,
  hints,
  machineApplicable,
};

// CLI: node fl-diagnostic-codes.js
if (require.main === module) {
  process.stdout.write(JSON.stringify({
    schema: 'fl-diagnostic-codes/1',
    tool: 'freelang-front',
    count: DIAGNOSTIC_CODES.length,
    codes: DIAGNOSTIC_CODES,
    machineApplicable: machineApplicable().map(c => c.code),
  }, null, 2) + '\n');
}
