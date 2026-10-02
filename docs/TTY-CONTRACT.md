# TTY/PTY 계약

## 원칙

`fl-git`의 Git 업무 규칙은 FreeLang에 둔다. 터미널 입력·화면 제어·자식
프로세스 PTY처럼 런타임 경계가 필요한 부분만 호스트에 둔다.

기존 [`fl-split-term`](../../fl-split-term/host/PTY_CONTRACT.md)의 계약을
참고하지만, `fl-git`의 저장소 상태와 Git 정책은 독립적으로 관리한다.

## 단계별 경계

### 초기 CLI

일반 출력과 인자를 사용한다. TTY 없이도 다음 명령이 동작해야 한다.

```text
fl-git status
fl-git diff
fl-git context
```

### TUI

AFJ의 `tty-*`를 사용해 raw 입력과 ANSI 화면을 연결한다.
자식 셸이나 장기 프로세스가 필요할 때만 `--allow-pty`를 사용한다.

### 마우스 선택 (SGR)

AFJ에는 마우스 전용 빌트인이 없다. TUI는 기존 `tty-read-byte`로 CSI를 읽고,
터미널 마우스 보고를 ANSI로 켠다.

| 항목 | 계약 |
|---|---|
| enable | `\x1b[?1000h\x1b[?1006h` (normal tracking + SGR) |
| disable | `\x1b[?1000l\x1b[?1006l` — `finally`에서 raw leave와 함께 복구 |
| SGR 클릭 | `\x1b[<Pb;Px;PyM` (press) / `m` (release), 좌표는 1-based |
| X10 예비 | `\x1b[M` + 3바이트 (Cb/Cx/Cy, 각 +32) |
| 동작 | 왼쪽 버튼 press가 **파일 상태 행**에 떨어지면 해당 경로 stage/unstage 토글 |
| 파싱 | FreeLang `src/tui-mouse.fl` — 호스트 JS에 선택 로직을 두지 않음 |
| 권한 | `scripts/tui-runner.mjs`의 `__flTerminalCapabilities.tty=true`만 사용 (pty 불필요) |

화면 행 매핑: 제목·구분선 다음 첫 상태 줄이 row 3. `##` 줄은 branch, 그 외
porcelain short 줄은 파일이다. 키보드 단축키는 기존과 동일하다.

### 명령 실행

Git 명령은 FreeLang AFJ의 process 경계에서 실행한다. 파일명·branch명·커밋
메시지는 `git-shell-quote`를 거치며, 사용자가 임의 셸 명령 자체를 입력하는
인터페이스는 제공하지 않는다.

## 부족한 기능 기록

- FreeLangScript의 process 권한 경계에서는 `shell-exec-result`가 차단된다.
  Git 프로세스 본체는 AFJ `.fl`에서 실행한다.
- AFJ의 `shell-exec-result`는 결과와 exit를 보존하지만, 사용자 입력은 반드시
  shell quote를 거쳐 명령 문자열에 넣는다.
- TTY/PTY 부족을 숨기거나 JS 앱 전체로 우회하지 않는다.
- AFJ에 `tty-read-key`(구조화 CSI)와 마우스 빌트인은 없다. fl-git은
  `tty-read-byte` + SGR 파싱으로 클릭만 처리한다. 드래그 다중 선택·휠
  스크롤 맵핑은 이 계약 범위 밖이다.
