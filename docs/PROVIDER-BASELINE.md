# fl-git Provider 기준선 (1단계)

**기록 시각:** 2026-09-30  
**원칙:** 현재 HEAD · 원격 HEAD · 미커밋 파일 · 테스트 기준을 **분리** 기록한다.  
**이 단계 금지:** 코드 수정, commit, push.

---

## A. 현재 HEAD (커밋된 정본)

| 항목 | 값 |
|------|-----|
| 저장소 | `/home/kim/kim/projects/fl-git` |
| 브랜치 | `main` |
| Upstream | `origin/main` |
| Local HEAD | `dd5f1456a09a7814b0868d16ef4a3abfe4d9d969` |
| 커밋 시각 | 2026-09-30 03:23:53 +0900 |
| 커밋 메시지 | `test: add shared FreeLang test runner` |

최근 커밋 (HEAD 기준):

```text
dd5f145 test: add shared FreeLang test runner
c893aa0 fix: support global fl-git server commands
6684353 feat: show project AIRC documents in main UI
9d4ca4e ops: establish FreeLang dev server release workflow
d9ab851 fix: make left navigation tabs interactive
```

---

## B. 원격 HEAD

| 항목 | 값 |
|------|-----|
| remote 이름 | `origin` |
| remote URL | `https://github.com/kimjindol2025/fl-git.git` |
| provider (추정) | `github` |
| host | `github.com` |
| owner | `kimjindol2025` |
| repository | `fl-git` |
| Remote HEAD (`refs/heads/main`) | `dd5f1456a09a7814b0868d16ef4a3abfe4d9d969` |
| local/remote match | **일치** (`match=true`) |

Forgejo remote는 이 저장소에 **없다**.  
참고 fixture 후보: `/home/kim/kim/platform/freelang-aia` 는 `forgejo` + `github` 이중 remote를 가진다.

```text
# freelang-aia (참고만, fl-git 작업 대상 아님)
forgejo  forgejo-dclub:kim/freelang-aia.git
github   https://github.com/kimjindol2025/freelang-aia.git
branch   master → forgejo/master
```

---

## C. 미커밋 파일 (HEAD와 섞지 말 것)

`git status --porcelain` 스냅샷:

| 상태 | 경로 | 성격 |
|------|------|------|
| ` M` | `README.md` | tracked 수정 — FreeLang Tools / deploy 안내 추가 |
| `??` | `.freelang/deploy.sh` | untracked — fl-tools deploy 계약 |
| `??` | `.freelang/rollback.sh` | untracked |
| `??` | `.freelang/smoke.sh` | untracked |
| `??` | `docs/AI-CONTEXT.md` | untracked — Provider/AI Context 계약 초안 |
| `??` | `scripts/fl-tools` | untracked — 로컬 fl-tools 래퍼 |

**해석:** dirty worktree는 “다음 Provider 작업의 재료”일 수 있으나, **현재 HEAD 기능 범위가 아니다.**  
1단계에서 이 파일들을 commit/push하지 않는다.

---

## D. 테스트 기준선

재현 명령과 결과 (2026-09-30, 코드 변경 전):

```bash
cd /home/kim/kim/projects/fl-git
./scripts/fl-test
# FREELANG_TEST=PASS — 3/3 (산술·컬렉션·문자열 — Provider fixture 없음)

./scripts/check.sh
# PASS fl-git project smoke — AFJ check + CLI --help + fl-test
```

| 항목 | 결과 | 한계 |
|------|------|------|
| `tests/fl-git.test.fl` | PASS 3/3 | Provider/Git fixture가 아닌 플레이스홀더 |
| `tests/smoke.fl` | check PASS | 문법만 |
| Provider fixture | **없음** | GitHub/Forgejo/UNKNOWN/AUTH_BLOCKED/HEAD mismatch 미구축 |
| CLI `context-json` | 동작 | `root/branch/remote/status/recent_log`만. `remote_info`·`sync` 스키마 없음 |
| CLI Provider 명령 | **없음** | `github` 하위명령만 존재 |
| Web `provider-repos` | 코드 존재 | GitHub=`gh`, Forgejo=`tea` 읽기 경로 |
| Web `remote_info` | 코드 존재 | AI context API에 provider/host/repo 포함 |
| Web `connect-remote` | 코드 존재 | confirm 후 `origin` add, push 없음 |
| 인증 | GitHub `gh` keyring OK / Forgejo `tea` login `dclub` 존재 | 토큰 값은 이 문서에 기록하지 않음 |

---

## E. 현재 제품 위치 (기준선 해석)

목표:

```text
fl-git = Git Provider 공통 작업공간
         ├─ GitHub
         ├─ Forgejo
         └─ GitLab 확장 가능
```

HEAD(`dd5f145`) 시점 실제 구조:

```text
CLI/TUI  : Git 로컬 작업 + GitHub(`gh`) 중심
Web API  : provider-repos / remote_info / connect-remote 초안 존재
문서     : PLAN·PROJECT는 여전히 “GitHub 앱” 서사
미커밋   : AI-CONTEXT·fl-tools 연동 초안 (HEAD 밖)
```

**갭:** Provider 추상화가 Web에 부분 존재하고, CLI·테스트·제품 정의는 아직 GitHub 단일 모델에 가깝다.

---

## F. 다음 단계 진입 조건

1단계 완료 조건:

- [x] Local HEAD 기록
- [x] Remote HEAD 기록 및 match 여부
- [x] 미커밋 목록을 HEAD와 분리
- [x] 테스트 기준선 재현 결과 기록
- [x] 코드 수정·push 없음

2단계로 넘어갈 때: Provider 공통 JSON 모델을 문서·스키마로 고정한 뒤, 읽기 전용 adapter부터 구현한다.  
상세: [`PROVIDER-ROADMAP.md`](PROVIDER-ROADMAP.md)
