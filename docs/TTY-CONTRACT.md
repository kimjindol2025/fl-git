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
