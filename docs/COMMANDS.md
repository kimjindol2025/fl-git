# 명령 계약

사용자 명령 계약이다. 출력·종료 코드·실패 형태를 이 문서와 테스트에 함께
고정한다.

| 명령 | 역할 | 기본 위험도 |
|---|---|---|
| `status` | 저장소·branch·변경 요약 | 읽기 |
| `diff` | 변경 내용 확인 | 읽기 |
| `log` | 최근 커밋 확인 | 읽기 |
| `add [파일...]` | 선택 파일 stage | 변경 |
| `unstage [파일...]` | stage 취소 | 변경 |
| `commit` | 로컬 커밋 | 변경 |
| `fetch` | 원격 참조 갱신 | 원격 읽기 |
| `pull --yes` | 원격 변경 통합 | 확인 필요 |
| `push --yes` | 원격 업로드 | 확인 필요 |
| `branch` | branch 조회 | 읽기 |
| `branch-create <이름>` | 새 branch 생성·전환 | 변경 |
| `context` | AI 전달 문맥 출력 | 읽기 |
| `context-json` | AI/API 전달용 JSON 문맥 (`remote_info`·`remotes`·`sync`·`recent_commits`) | 읽기 |
| `provider model` | Provider 공통 모델 JSON 출력 | 읽기 |
| `provider status [github\|forgejo]` | Provider 인증 프로브 (`AUTH_OK` / `AUTH_BLOCKED` / `UNKNOWN`) | 읽기 |
| `provider repos <github\|forgejo>` | Provider별 레포 목록 (인증 필요, 쓰기 없음) | 읽기 |
| `provider gate` | fixture 검증 게이트 (`PROVIDER_DETECT`…`REMOTE_WRITE_CONFIRM`) | 읽기 |
| `provider fetch [provider] --yes` | 대상 provider/repo/branch 표시 후 fetch | 확인 필요 |
| `provider pull [provider] --yes` | 대상 표시 후 pull | 확인 필요 |
| `provider push [provider] --yes` | 대상 표시 후 push | 확인 필요 |
| `provider connect <url> [provider] --yes` | 대상 표시 후 origin 연결 (Push 없음) | 확인 필요 |
| `branch-delete <이름> --yes` | 로컬 branch 삭제 | 확인 필요 |
| `github repo` | 현재 GitHub 저장소 정보 | 읽기 |
| `github pr` | 열려 있는 PR 목록 | 읽기 |
| `github pr-create <제목>` | 현재 branch PR 생성 | 원격 변경 |

모든 명령은 현재 저장소 루트, branch, remote를 먼저 보여주는 것을 기본으로
한다.

`pull`과 `push`는 실수로 실행되지 않도록 `--yes`를 명시해야 한다. TUI에서는
사용자가 해당 키를 누른 것이 확인으로 취급된다.

`fl-git tui`는 raw TTY에서 키보드 단축키와 SGR 마우스 클릭을 받는다. 상태
화면의 파일 행을 왼쪽 클릭하면 해당 경로를 stage/unstage 토글한다. 계약은
`docs/TTY-CONTRACT.md`다.

웹 UI 저장소 전환: POST `/api` `action=open-path` (Git 저장소면 전환, 일반
폴더면 하위 목록), `list-folders`, `list-parent`(허용 루트 안 상위),
`list-repos`, `select-repo`. 경로는 `FL_GIT_ROOTS` 물리 경로 안으로 제한한다.

`provider fetch|pull|push|connect`는 실행 전에 provider → repository → branch를
표시하고, `--yes`가 없으면 `REMOTE_WRITE_CONFIRM_REQUIRED`로 거부한다.
요청 provider와 대상 provider가 다르면 `REMOTE_WRITE_MISMATCH`다.

GitHub 명령은 `gh` CLI의 기존 인증 세션을 사용하며 토큰을 읽거나 저장하지
않는다.
