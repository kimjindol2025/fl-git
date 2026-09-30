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

## 검증 상태

| 항목 | 상태 | 근거 |
|---|---|---|
| 디렉터리·문서 | PASS | 파일 구조 확인 |
| JSON manifest | PASS | JSON 파싱 확인 필요 |
| AFJ check/run | PASS | `scripts/check.sh` |
| Git CLI | PASS | 임시 저장소에서 status/add/unstage/commit/branch/context 검증 |
| 원격 안전장치 | PASS | `pull/push/branch-delete`에 `--yes` 요구 |
| GitHub 연동 | 구현 PASS / 외부 인증 BLOCKED | `gh` 명령 연결 완료, 현재 로컬 토큰 invalid |
| TUI | PASS | PTY에서 상태·커밋 입력·종료 키 검증 |
| 브라우저 UI | PASS | FL-Front 빌드 errors=0/warnings=0, 실제 HTTP 200 |
| 화면 중심 흐름 | 진행 중 | Stage/commit/pull/push 구현, 폴더 선택·GitHub 연결 화면 예정 |
| AI 문맥 | PASS | Markdown `context`, JSON `context-json` |

## 발견된 경계

초기 정지 원인은 Script 런너의 `diagnoseAndNormalize`가 검증기 stdin을
파이프로 넘긴 뒤, 검증기 내부 `bootstrap.js` 자식이 같은 stdin을 기다리는
교착이었다. 런타임은 검증 소스를 임시 파일로 전달하도록 수정했고, 이후
실제 check/run이 통과했다.

샌드박스 내부에서는 자식 프로세스 생성에 `EPERM`이 발생할 수 있어, 최종
런타임 검증은 제한된 권한 밖에서 실행했다. 일반 서버 터미널의 실행 경계와
구분해 기록한다.

검증 스크립트는 무한 대기를 막기 위해 15초 제한을 둔다.
