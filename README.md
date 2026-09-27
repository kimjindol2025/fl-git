# fl-git

FreeLang 기반 서버·터미널용 Git 관리 도구.

`fl-git`는 VS Code나 웹 화면을 복제하는 프로젝트가 아니다. 서버 터미널에서
실제 Git 저장소를 안전하게 확인하고, 선택한 작업을 실행하며, GitHub와 AI에
현재 프로젝트 문맥을 전달하는 CLI/TUI를 목표로 한다.

## 현재 상태

- CLI, TUI, GitHub 연결, AI 문맥 출력까지 구현된 실행 가능한 MVP
- 기존 `fl-split-term`의 TTY/PTY 계약을 참고하되 소스는 독립 유지
- Git 명령 실행, TUI, GitHub 연동을 AFJ로 구현 완료
- 본체는 Git 프로세스 권한이 필요한 AFJ(`.fl`)로 구현한다.
- `scripts/tui-runner.mjs`는 TTY capability만 여는 얇은 호스트 경계다.

## 실행 예정

```bash
node /home/kim/kim/platform/freelang-afj/bootstrap.js check src/main.fl
node /home/kim/kim/platform/freelang-afj/bootstrap.js run src/main.fl --help
```

## 목표 명령

```bash
fl-git status
fl-git add
fl-git commit
fl-git pull
fl-git push
fl-git branch
fl-git diff
fl-git context
fl-git context-json
fl-git unstage [파일...]
fl-git fetch
fl-git branch-create <이름>
fl-git branch-delete <이름> --yes
```

## 안전 원칙

- 현재 저장소·브랜치·원격을 먼저 표시한다.
- `push`, `pull`, `reset`, `clean`, 브랜치 삭제는 실행 전 확인한다.
- `reset --hard`, `clean -fd`는 기본 차단한다.
- GitHub 인증 토큰을 파일이나 원격 URL에 저장하지 않고 기존 `gh auth`를 사용한다.
- 실행 결과와 실패 원인을 숨기지 않는다.

자세한 범위는 [`docs/PROJECT.md`](docs/PROJECT.md), 단계별 계획은
[`docs/PLAN.md`](docs/PLAN.md), TTY 경계는 [`docs/TTY-CONTRACT.md`](docs/TTY-CONTRACT.md)를 참조한다.
