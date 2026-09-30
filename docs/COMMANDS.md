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
| `branch-delete <이름> --yes` | 로컬 branch 삭제 | 확인 필요 |
| `github repo` | 현재 GitHub 저장소 정보 | 읽기 |
| `github pr` | 열려 있는 PR 목록 | 읽기 |
| `github pr-create <제목>` | 현재 branch PR 생성 | 원격 변경 |

모든 명령은 현재 저장소 루트, branch, remote를 먼저 보여주는 것을 기본으로
한다.

`pull`과 `push`는 실수로 실행되지 않도록 `--yes`를 명시해야 한다. TUI에서는
사용자가 해당 키를 누른 것이 확인으로 취급된다.

GitHub 명령은 `gh` CLI의 기존 인증 세션을 사용하며 토큰을 읽거나 저장하지
않는다.
