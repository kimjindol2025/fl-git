# fl-git Provider 로드맵

**채택 방향 (2026-09-30):**

> GitHub를 붙이는 게 아니라, Provider라는 추상화를 먼저 만들고  
> GitHub와 Forgejo를 그 위에 꽂는다.

```text
목표: fl-git = Git Provider 공통 작업공간

Git Provider
├─ GitHub
├─ Forgejo
└─ GitLab (adapter·UI·write·gate 2026-10-01)
```

기준선 스냅샷: [`PROVIDER-BASELINE.md`](PROVIDER-BASELINE.md)

---

## 1단계 — 기준선 정리

상태: **완료 (문서만, 코드/push 없음)**

분리 기록:

- 현재 HEAD
- 원격 HEAD
- 미커밋 파일
- 테스트 기준

---

## 2단계 — Provider 모델

상태: **구현 완료 (스키마·파서·CLI/Web 정렬·fixture)**  
정본: [`PROVIDER-MODEL.md`](PROVIDER-MODEL.md) · `src/provider.fl`

모든 원격을 공통 구조로 표현한다.

Forgejo 예:

```json
{
  "provider": "forgejo",
  "host": "fg.dclub.kr",
  "owner": "kim",
  "repository": "freelang-aia",
  "remote": "forgejo-dclub:kim/freelang-aia.git",
  "branch": "master",
  "upstream": "forgejo/master",
  "auth": "configured"
}
```

GitHub 예:

```json
{
  "provider": "github",
  "host": "github.com",
  "owner": "kimjindol2025",
  "repository": "freelang-tools",
  "remote": "origin",
  "branch": "main"
}
```

규칙:

- `provider` 필드 필수
- 알 수 없으면 `"provider": "unknown"`
- 인증 상태는 `configured` / `missing` / `blocked` 등으로만 표시
- 토큰 원문은 모델·로그·AI Context에 넣지 않음

---

## 3단계 — Provider Adapter (읽기 전용)

상태: **auth 프로브 + provider-repos 완료**  
정본: `src/provider-adapter.fl`  
범위: 쓰기(`push` / repo create / connect) 없음.

이번 단계에서 제공하는 읽기 기능:

```text
provider status   — auth 프로브
provider repos    — 레포 목록 읽기
```

| Provider | 도구 |
|----------|------|
| GitHub | `gh` (`gh api user`, `gh repo list`) |
| Forgejo | `tea` (`tea login list`, `tea repos list`) |
| 그 외 | `UNKNOWN` |
| 인증 없음 | `AUTH_BLOCKED` |

규칙:

- 토큰 원문은 화면·로그·AI Context에 노출하지 않는다 (`provider-redact`)
- 레포 목록 전에 auth 프로브를 먼저 실행한다
- CLI와 Web API가 같은 판정 코드를 쓴다

```bash
fl-git provider status
fl-git provider status github
fl-git provider status forgejo
fl-git provider repos github
fl-git provider repos forgejo
```

Web:

```http
POST /api {"action":"provider-status","provider":"github"}
POST /api {"action":"provider-repos","provider":"forgejo"}
```

`provider-commits` / `provider-branches` / `provider-detect` CLI는 후속 보강이다.

---

## 4단계 — AI Context 확장

상태: **구현 완료**  
정본: `src/provider-context.fl` · CLI `context-json` · Web `ai-context`

기존 AI Context에 원격 정보를 **provider 분리**로 포함한다.

```json
{
  "remote_info": {
    "provider": "forgejo",
    "host": "fg.dclub.kr",
    "owner": "kim",
    "repository": "freelang-aia",
    "branch": "master",
    "upstream": "forgejo/master"
  },
  "sync": {
    "local_head": "...",
    "remote_head": "...",
    "match": true
  },
  "recent_commits": []
}
```

AI가 GitHub와 Forgejo를 같은 서비스로 오해하지 않게 `provider`를 반드시 분리한다.  
CLI `context` / `context-json`과 Web `ai-context`가 같은 스키마를 공유한다.

---

## 5단계 — UI 분리

상태: **구현 완료**  
정본: `web/pages/index.flx` (Island) · `web/pages/api.flx` `connect-remote` 교차 거부

```text
Remote & Commits
├─ GitHub
├─ Forgejo (fg)
└─ 기타 Provider
```

중요:

- GitHub 목록에서 Forgejo remote를 연결하지 않음
- Forgejo 목록에서 GitHub branch를 섞지 않음

Provider별로 레포 목록 · branch · remote URL · 최근 커밋 · 동기화 상태를 따로 보여준다.

구현 요지:

- 레일·개요·Remote 스위치에 GitHub / Forgejo / 기타 탭 분리
- 패널은 해당 provider remote만 나열. sync는 upstream이 그 provider일 때만 표시
- 기타는 읽기 전용 요약 (목록 adapter 없음)
- 클라이언트 URL 형식 검사 + 서버 `expected != detected` 거부로 교차 연결 차단
- Front 빌드 errors=0, pm2 `fl-git:40850`, Playwright로 패널·토스트 교차 거부 확인

---

## 6단계 — 연결과 쓰기 작업

상태: **구현 완료**  
정본: `src/provider-write.fl` · CLI `fl-git provider fetch|pull|push|connect` · Web `provider-write` / `fetch|pull|push|connect-remote` · UI 패널 쓰기 섹션

읽기 기능이 안정화된 뒤에만:

```text
connect-remote
fetch
pull
push
```

를 provider 공통 인터페이스로 연결한다.

쓰기 작업 순서 (필수):

1. 대상 provider 표시
2. 대상 repository 표시
3. branch 표시
4. 사용자 confirm
5. 실행 결과

구현 요지:

- 공통 게이트: `provider-write-gate` — 대상 표시 → confirm/`--yes` 없으면 `REMOTE_WRITE_CONFIRM_REQUIRED`, 교차 provider는 `REMOTE_WRITE_MISMATCH`
- CLI: `fl-git provider fetch|pull|push [github|forgejo] [--yes]`, `fl-git provider connect <url> [provider] [--yes]`
- Web: `action=provider-write` 및 기존 `fetch`/`pull`/`push`/`connect-remote`가 동일 게이트 사용 (fetch도 confirm 필수)
- UI: provider 패널에 WRITE PROVIDER/REPO/BRANCH 표시 + active일 때만 Fetch/Pull/Push; confirm 문구에 대상 trio 포함
- fixture: `tests/provider-write.test.fl` 9/9 PASS; Front errors=0; Playwright confirm·Forgejo inactive 게이트 확인

---

## 7단계 — 검증

상태: **구현 완료**  
정본: `src/provider-gate.fl` · `tests/provider-gate.test.fl` · `scripts/provider-gate` · CLI `fl-git provider gate`

최소 fixture:

- GitHub remote fixture
- Forgejo remote fixture
- provider 없음
- 잘못된 provider
- 인증 없음
- local/remote HEAD 불일치

PASS 기준:

```text
PROVIDER_DETECT=PASS
GITHUB_CONTEXT=PASS
FORGEJO_CONTEXT=PASS
INVALID_PROVIDER=DENY
HEAD_SYNC=PASS
TOKEN_NOT_EXPOSED=PASS
REMOTE_WRITE_CONFIRM=PASS
```

구현 요지:

- 네트워크 없이 순수 fixture로 위 코드를 산출·검증
- CLI: `fl-git provider gate` → 코드 출력 후 `PROVIDER_GATE=PASS|FAIL`
- 스크립트: `./scripts/provider-gate` (테스트 + CLI 라인 재현). `scripts/check.sh`에 포함
- `INVALID_PROVIDER=DENY`는 잘못된 provider가 거부되는 것이 정상 결과

재현:

```bash
./scripts/provider-gate
# 또는
./scripts/fl-git provider gate
```

---

## 최종 사용 흐름

```text
fl-tools check          # check.sh + provider-gate
fl-git context
fl-git provider status
fl-git provider repos forgejo
fl-git provider repos github
fl-git provider gate
# web: http://127.0.0.1:40850/  (pm2 fl-git)
```

2026-10-01 스모크: `PROVIDER_GATE=PASS`, `PROVIDER_STATUS=PASS`, github/forgejo `PROVIDER_REPOS=PASS`, web HTTP 200, `ai-context` ok.

Forgejo(`fg.dclub.kr`)는 단순 백업이 아니라, 프로젝트·문서·커밋·AI 계승 맥락의 핵심 저장소로 취급한다.

---

## 구현 순서 요약

```text
1 기준선 문서              ✅
2 Provider 모델 스키마      ✅
3 읽기 adapter (CLI+Web)    ✅
4 AI Context 스키마 통일    ✅
5 UI provider 분리          ✅
6 쓰기(connect/fetch/pull/push) + confirm ✅
7 fixture 검증 게이트     ✅
```

기존 `docs/PLAN.md`의 “GitHub 앱” Phase는 이 로드맵으로 대체한다.  
tracked `PLAN.md` 갱신·코드 구현은 2단계 착수 때 수행한다.


## GitLab provider 확장 (2026-10-01)

상태: **구현** — detect/model 기존 + adapter(`glab`)/UI 탭/write confirm/gate `GITLAB_CONTEXT`.
호스트에 glab 없으면 AUTH_BLOCKED가 정상 경로다. 토큰 원문 비노출(`glpat-` redact).
