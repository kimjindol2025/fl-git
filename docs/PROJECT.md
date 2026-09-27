# fl-git 프로젝트 정의

## 목적

서버에서 VS Code Source Control처럼 Git 저장소를 화면으로 읽고 관리하게 한다.
웹 UI를 본체로 하고 CLI/TUI는 보조 경로로 유지한다. 단순 명령 별칭이나 AI
문맥 출력기가 아니라, 폴더 선택부터 GitHub push까지의 작업 흐름을 화면으로
제공한다.

## 사용자 문제

- 여러 프로젝트의 변경 상태를 매번 Git 명령으로 조합해야 한다.
- `add → commit → push` 순서를 놓치기 쉽다.
- 현재 브랜치와 원격 상태를 한눈에 보기 어렵다.
- AI에게 현재 저장소 상태와 다음 작업을 반복 설명해야 한다.
- 서버에서는 IDE를 설치하지 않고도 Source Control 흐름이 필요하다.

## 범위

### 포함

- 저장소 경계와 현재 브랜치 확인
- 변경 파일·staged 파일·충돌 파일 요약
- 선택 파일 stage/unstage
- 커밋 메시지 입력과 커밋
- pull/push 전 확인과 결과 표시
- branch, diff, log, stash 기본 작업
- GitHub CLI(`gh`) 기반 저장소·PR 연결
- AI용 Markdown/JSON context 출력
- 브라우저 기반 저장소 선택·상태·변경 파일 화면
- 마우스 기반 파일 선택·Stage·Commit·GitHub 작업
- 키보드 TUI와 CLI 보조 경로

### 제외

- 터미널 에뮬레이터 자체 제작
- IDE 전체 기능 복제(편집기·디버거·확장 생태계)
- 인증 토큰 직접 보관
- 사용자 승인 없는 원격 push나 파괴적 명령
- 여러 프로젝트를 임의로 동시에 조작하는 전역 관리자

## 성공 기준

1. 사용자가 폴더를 선택하면 해당 Git 저장소 상태를 정확히 보여준다.
2. 사용자가 선택한 파일만 stage할 수 있다.
3. 커밋 전에 실제 diff와 대상 브랜치를 보여준다.
4. GitHub 연결·branch를 확인한 뒤 push/pull 결과를 표시한다.
5. CLI/TUI에서도 같은 작업을 수행할 수 있다.
6. `context`는 필요할 때 Codex·Grok·OpenCode에 전달할 수 있다.
7. 실패한 Git 명령은 종료 코드·stderr·복구 방법을 함께 보여준다.

## 구현 경계

Git 실행과 정책은 FreeLang AFJ `.fl` 본체에 있으며, `scripts/tui-runner.mjs`는
AFJ의 deny-by-default TTY capability를 `fl-git tui` 실행에만 여는 호스트
브리지다. 인증은 `gh` CLI 세션을 그대로 사용한다.
