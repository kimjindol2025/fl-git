# fl-git 프로젝트 정의

## 목적

서버 개발자가 VS Code 없이 실제 터미널에서 Git 저장소를 읽고 관리할 수 있게
한다. 단순 명령 별칭이 아니라 현재 상태를 설명하고, 위험을 표시하고, AI에게
프로젝트 문맥을 내보내는 FreeLang 도구를 만든다.

## 사용자 문제

- 여러 프로젝트의 변경 상태를 매번 Git 명령으로 조합해야 한다.
- `add → commit → push` 순서를 놓치기 쉽다.
- 현재 브랜치와 원격 상태를 한눈에 보기 어렵다.
- AI에게 현재 저장소 상태와 다음 작업을 반복 설명해야 한다.
- 서버에서는 IDE의 Source Control 버튼을 사용할 수 없다.

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
- 키보드 TUI와 이후 마우스 이벤트 지원

### 제외

- 터미널 에뮬레이터 자체 제작
- IDE 전체 복제
- 인증 토큰 직접 보관
- 사용자 승인 없는 원격 push나 파괴적 명령
- 여러 프로젝트를 임의로 동시에 조작하는 전역 관리자

## 성공 기준

1. 임시 Git 저장소에서 현재 상태를 정확히 보여준다.
2. 사용자가 선택한 파일만 stage할 수 있다.
3. 커밋 전에 실제 diff와 대상 브랜치를 보여준다.
4. push/pull은 확인 후 실제 결과를 표시한다.
5. `context` 결과를 Codex·Grok·OpenCode에 바로 전달할 수 있다.
6. 실패한 Git 명령은 종료 코드·stderr·복구 방법을 함께 보여준다.

## 구현 경계

Git 실행과 정책은 FreeLang AFJ `.fl` 본체에 있으며, `scripts/tui-runner.mjs`는
AFJ의 deny-by-default TTY capability를 `fl-git tui` 실행에만 여는 호스트
브리지다. 인증은 `gh` CLI 세션을 그대로 사용한다.
