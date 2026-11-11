# AI 통합 개발도구 단계 게이트

## 게이트 규칙

- 단계는 순서대로 진행한다.
- 이전 단계가 PASS가 아니면 다음 단계 구현을 시작하지 않는다.
- PASS는 실행 가능한 검증 결과와 숫자를 함께 기록해야 한다.
- 기존 회귀 테스트, 신규 기능 테스트, 오류 경로, 보안 경계를 모두 확인한다.

## 1단계 — AI API 연결 + 채팅창

상태: **PASS**

구현 범위:

- OpenAI 호환 API 연결
- 'ai-status' 설정 상태 API
- 'ai-chat' 읽기 전용 채팅 API
- 기존 저장소 AI Context 자동 주입
- 웹 'AI Chat' 화면
- API 키 비노출·메시지 크기·provider 오류 처리

검증 결과:

| 검증 | 결과 |
|---|---:|
| 기존 'scripts/check.sh' 회귀 assertion | 71 PASS |
| Front 괄호/Island/경계 검사 | 3 PASS |
| Front build errors | 0 |
| AI 상태 API — 키 미설정 | 1 PASS |
| AI 채팅 — 키 미설정 안전 차단 | 1 PASS ('503', 'AI_NOT_CONFIGURED') |
| AI 채팅 — 빈 메시지 차단 | 1 PASS ('400', 'AI_MESSAGE_REQUIRED') |
| AI 상태 API — mock provider 설정 | 1 PASS |
| AI 채팅 — mock provider 응답 | 1 PASS ('200') |
| AI Context 자동 주입 확인 | 1 PASS ('MOCK_CONTEXT_OK') |
| UI 'AI Chat' 표시 확인 | 2 occurrences |
| API 응답의 키 원문 노출 | 0건 |

검증 명령 및 환경:

- './scripts/check.sh' → 종료 코드 '0'
- 'POST /api {"action":"ai-status"}'
- 'POST /api {"action":"ai-chat","message":"..."}'
- 로컬 mock provider '127.0.0.1:40998'와 별도 web port '40861' 사용
- 실제 비밀키는 사용하지 않음

다음 단계: **2단계 파일/코드 검색을 시작할 수 있음**.

## 2단계 — 파일/코드 검색을 AI 도구로 제공

상태: **PASS**

구현 범위:

- OpenAI 호환 `search_files` function tool 정의
- `files`·`history`·`continuity`·`all` scope 전달
- 기존 `api-ai-search` 읽기 전용 검색 실행
- tool call 결과를 AI 후속 요청에 전달
- 허용되지 않은 도구는 실행하지 않고 오류 결과 반환

검증 결과:

| 검증 | 결과 |
|---|---:|
| `npm test` 회귀 assertion | 71 PASS |
| Front build errors | 0 |
| AI tool call 왕복 | 2 provider requests |
| `search_files` tool call 생성 | 1 PASS |
| tool 결과 후속 AI 응답 | 1 PASS (`200`, `MOCK_TOOL_FOLLOWUP_OK`) |
| 읽기 전용 검색 경로 | 1 PASS |
| 파일 변경·Git 쓰기·셸 실행 | 0건 |

검증 환경:

- 로컬 mock provider `127.0.0.1:40997`
- 별도 web port `40862`
- 실제 비밀키 미사용

다음 단계: **3단계 diff 기반 코드 수정 및 승인으로 진행 가능**.

## 3단계 — diff 기반 코드 수정 및 승인

상태: **PASS**

구현 범위:

- `ai-patch`: 안전한 대상 파일 원문을 AI에 전달하고 unified diff 제안 생성
- `ai-apply-patch`: `confirm=true`와 base signature 일치 후에만 적용
- `git apply --check --unidiff-zero` 사전 검증
- 작업 트리 변경 시 `AI_PATCH_STALE` 차단
- 웹 AI Chat의 수정안 생성·Diff 확인 후 적용 버튼

검증 결과:

| 검증 | 결과 |
|---|---:|
| `npm test` 회귀 assertion | 71 PASS |
| Front 괄호/Island/경계 검사 | 3 PASS |
| Front build errors | 0 |
| AI patch proposal | 1 PASS (`200`, proposal=true) |
| 승인 전 적용 차단 | 1 PASS (`400`, `AI_PATCH_APPROVAL_REQUIRED`) |
| 작업 트리 stale diff 차단 | 1 PASS (`AI_PATCH_STALE`) |
| 승인 후 diff 적용 | 1 PASS (`200`, `AI_PATCH_APPLIED`) |
| 적용 전 `git apply --check` | 1 PASS |
| 웹 수정안 UI source marker | 2 PASS |
| 사용자 승인 없는 파일 변경 | 0건 |

검증 환경:

- 로컬 mock provider `127.0.0.1:40996`
- 별도 web port `40864`
- 실제 비밀키 미사용
- 승인 테스트 후 `docs/PROJECT.md` 원상 복구 확인

다음 단계: **4단계 테스트 실행과 오류 재수정으로 진행 가능**.

## 4단계 — 테스트 실행과 오류 재수정

상태: **PASS**

구현 범위:

- 허용 명령 allowlist: `npm test`, `./scripts/check.sh`, `npm run test`
- `confirm=true` 전용 테스트 실행 게이트
- 120초 timeout
- stdout·stderr·종료 코드 반환
- 민감정보 redaction
- 실패 결과를 AI patch 요청으로 전달하는 웹 흐름

검증 결과:

| 검증 | 결과 |
|---|---:|
| Front 괄호/Island/경계 검사 | 3 PASS |
| Front build errors | 0 |
| 승인 없는 테스트 실행 차단 | 1 PASS (`AI_TEST_APPROVAL_REQUIRED`) |
| 비허용 명령 차단 | 1 PASS (`AI_TEST_COMMAND_DENIED`) |
| `npm test` 승인 실행 | 1 PASS (`HTTP 200`, exit `0`) |
| 테스트 assertion | 71 PASS |
| 테스트 stdout/stderr 반환 | 1 PASS |
| 승인 없는 자동 수정 | 0건 |

다음 단계: **5단계 코드 에디터·터미널 통합으로 진행 가능**.

## 5단계 — 코드 에디터·터미널 통합

상태: **PASS**

구현 범위:

- 웹 Editor 탭에서 저장소 파일 읽기·편집·저장
- 파일 저장 전 confirm 및 worktree signature stale 차단
- 민감 파일·경로 차단
- Limited Terminal 패널
- allowlist 기반 `git status`, `git diff`, `git log`, `npm test`, `./scripts/check.sh`, `pwd`
- 모든 터미널 실행 전 사용자 승인

검증 결과:

| 검증 | 결과 |
|---|---:|
| Front 괄호/Island/경계 검사 | 3 PASS |
| Front build errors | 0 |
| Editor 파일 읽기 | 1 PASS (`README.md`, 2,341 bytes) |
| Editor 저장 승인 차단 | 1 PASS (`EDITOR_APPROVAL_REQUIRED`) |
| Editor 저장 승인 실행 | 1 PASS (`EDITOR_SAVED`) |
| Editor 저장 후 원본 diff | 0건 |
| Terminal 승인 차단 | 1 PASS (`TERMINAL_APPROVAL_REQUIRED`) |
| Terminal `git status` 실행 | 1 PASS (exit `0`) |
| 임의 명령 실행 | 0건 |

5단계는 서버형 제한 터미널까지 포함한다. 완전한 PTY/인터랙티브 셸은 안전한
권한·세션 모델을 설계한 뒤 후속 보강 대상으로 둔다.

다음 단계: **6단계 멀티 프로젝트·권한·작업 기록으로 진행 가능**.

## 6단계 — 멀티 프로젝트·권한·작업 기록

상태: **PASS**

구현 범위:

- 기존 `FL_GIT_ROOTS` 기반 멀티 프로젝트 목록·전환
- `FL_GIT_ROLE`: `viewer`·`developer`·`operator`
- `access-status` 권한 상태 API
- editor·test·terminal·local-write·remote-write 권한 분리
- developer의 remote write 차단
- AI patch·editor save·test·terminal 실행 감사 기록
- 웹 Audit 탭과 `audit-log` API

검증 결과:

| 검증 | 결과 |
|---|---:|
| Front 괄호/Island/경계 검사 | 3 PASS |
| Front build errors | 0 |
| 멀티 프로젝트 root 목록 | 1 PASS (repository 10개 발견) |
| access-status 응답 | 1 PASS (권한 7종) |
| developer remote-write 차단 | 1 PASS (`403`, `PERMISSION_DENIED`) |
| 승인된 terminal 감사 기록 | 1 PASS |
| audit-log 필드 검증 | 1 PASS (5개 필드) |
| API 키·토큰 감사 기록 노출 | 0건 |

현재 감사 기록은 서버 세션 메모리에 보관된다. 영속 DB 저장과 사용자 인증은
운영 배포 전 보강 대상으로 남겨두며, 현재 단계의 권한 경계와 작업 추적
계약은 검증 완료했다.

전체 PRD 1~6단계 게이트: **PASS**.
