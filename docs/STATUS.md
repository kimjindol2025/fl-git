# fl-git 현재 상태

## 준비 결과

- 프로젝트 폴더: `projects/fl-git`
- 언어 프로필: FreeLang AFJ `.fl` (Git 프로세스 권한 필요)
- manifest: `freelang.json`
- 초기 진입점: `src/main.fl`
- 제품 범위·계획·명령·TTY 계약 문서 작성 완료
- 기존 `fl-split-term`과 소스 분리 완료
- 브라우저 UI·CLI·TUI·GitHub·AI 문맥 기능 구현 완료
- Provider 모델 2단계: `src/provider.fl`, `fl-git provider model`, `remote_info` 스키마 정렬, fixture 15/15 PASS
- Provider adapter 3단계: `src/provider-adapter.fl`, `provider status` / `provider repos` 읽기 전용, AUTH_BLOCKED/UNKNOWN 판정
- AI Context 4단계: `sync.local_head/remote_head/match`, `recent_commits`, `remotes` provider 분리, CLI/Web 스키마 정렬
- UI 분리 5단계: GitHub / Forgejo / 기타 패널 분리, 목록·remote·sync 혼입 방지, 클라이언트·서버 교차 연결 거부
- 쓰기 6단계: `provider-write` 공통 게이트, CLI/Web confirm, 패널 Fetch/Pull/Push 대상 표시
- 검증 7단계: `provider-gate` fixture PASS/DENY 코드, `fl-git provider gate`, `scripts/provider-gate`
- Provider 로드맵 1–7 완료. 커밋 `527d4c8` (`origin/main`)

## 검증 상태

| 항목 | 상태 | 근거 |
|---|---|---|
| 디렉터리·문서 | PASS | 파일 구조 확인 |
| JSON manifest | PASS | JSON 파싱 확인 필요 |
| AFJ check/run | PASS | `scripts/check.sh` |
| Git CLI | PASS | 임시 저장소에서 status/add/unstage/commit/branch/context 검증 |
| 원격 안전장치 | PASS | `pull/push/branch-delete`에 `--yes` 요구 |
| Provider 쓰기 confirm | PASS | `provider-write` preview/mismatch, CLI `--yes`, Web `confirm=true` |
| Provider fixture 게이트 | PASS | `PROVIDER_DETECT`…`REMOTE_WRITE_CONFIRM` 로드맵 코드 재현 |
| GitHub 연동 | PASS | `provider status` AUTH_OK, `provider repos github` PROVIDER_REPOS=PASS |
| Forgejo 연동 | PASS | `provider status` AUTH_OK, `provider repos forgejo` PROVIDER_REPOS=PASS |
| TUI | PASS | PTY에서 상태·커밋 입력·종료 키 검증 |
| 브라우저 UI | PASS | FL-Front 빌드 errors=0, HTTP 200, Playwright 패널·교차 거부 |
| Provider UI 분리 | PASS | GitHub/Forgejo/기타 탭, sync 게이트, URL 형식 토스트 |
| Provider 쓰기 UI | PASS | 패널 WRITE trio·active 게이트, confirm에 provider/repo/branch |
| 화면 중심 흐름 | PASS | Stage1–7 Provider 로드맵 완료 |
| AI 문맥 | PASS | Markdown `context`, JSON `context-json` |
| 최종 사용 흐름 스모크 | PASS | gate · context · provider status/repos · web `:40850` · ai-context |

## 다음 후보

- `docs/PLAN.md` 잔여: TTY 마우스 선택

## 발견된 경계

초기 정지 원인은 Script 런너의 `diagnoseAndNormalize`가 검증기 stdin을
파이프로 넘긴 뒤, 검증기 내부 `bootstrap.js` 자식이 같은 stdin을 기다리는
교착이었다. 런타임은 검증 소스를 임시 파일로 전달하도록 수정했고, 이후
실제 check/run이 통과했다.

샌드박스 내부에서는 자식 프로세스 생성에 `EPERM`이 발생할 수 있어, 최종
런타임 검증은 제한된 권한 밖에서 실행했다. 일반 서버 터미널의 실행 경계와
구분해 기록한다.

검증 스크립트는 무한 대기를 막기 위해 15초 제한을 둔다.


## GitLab provider 확장 · 2026-10-01

- adapter: `provider status|repos gitlab` (glab 없으면 `AUTH_BLOCKED`)
- UI: GitLab 탭·디렉터리 카드·패널 (기타와 분리)
- write: confirm/mismatch에 gitlab 포함
- gate: `GITLAB_CONTEXT=PASS` · `PROVIDER_GATE=PASS`
- Front 빌드: `web/fl-front-build.js` diagnostics errors=0
- 재현: `./scripts/provider-gate` · `./scripts/fl-git provider status gitlab` · POST `/api` provider-status/repos


## 참조 시스템 카드 연결 · 2026-10-02

- Web `ai-context` 카드에 `spec:*`(SPEC.airc) · `provider:remotes` 추가 (총 19장)
- CLI `context` / `context-json`에 continuity·spec·cards 패리티
- `ai-search` continuity 범위에 SPEC.airc 포함
- 재현: `fl-git context-json` · `curl ... -d '{"action":"ai-context"}'` · `{"action":"context","keys":["spec:hot"]}`
