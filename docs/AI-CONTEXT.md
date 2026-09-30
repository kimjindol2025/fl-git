# fl-git AI Context 계약

`fl-git`은 현재 저장소를 AI가 추측하지 않고 읽을 수 있도록 읽기 전용 POST API를 제공한다.
GitHub 검색 결과만으로는 알 수 없는 로컬 변경, 현재 branch, 충돌, AIRC 결정, 다음 작업을 함께 전달하는 것이 목적이다.

## 전체 프로젝트 패킷

```http
POST /api
Content-Type: application/json

{"action":"ai-context"}
```

응답에는 다음이 포함된다.

- `continuity.identity`: 프로젝트 목적과 존재 이유
- `continuity.hot`: 현재 우선 작업
- `continuity.hot_node`: 작업 내용과 검증 명령
- `continuity.locks`: 계승하면 안 되는 금지·보호 규칙
- `continuity.handoff`: 다음 세션 인수인계 절차
- `remote_info`: Provider 공통 모델 (`provider`, `host`, `owner`, `repository`, `remote`, `branch`, `upstream`, `auth`) — 정본 [`PROVIDER-MODEL.md`](PROVIDER-MODEL.md)
- `remotes`: 로컬 remote를 provider별로 분리한 배열 (GitHub와 Forgejo를 섞지 않음)
- `sync`: `local_head` · `remote_head` · `match` · `upstream` · `ahead` · `behind`
- `recent_commits`: `{hash,date,author,message}` 배열
- `change_risk`: 변경 파일·민감 파일·충돌 기반 위험도
- `cards`: 저장소 상태와 AIRC를 분리한 참고 카드
- `markdown`: AI 프롬프트에 바로 붙일 수 있는 읽기용 문맥
- `ai_instruction`: 읽기 전용 및 승인 경계

CLI도 같은 핵심 필드를 제공한다.

```bash
fl-git context-json
```

```json
{
  "remote_info": {"provider": "github", "host": "github.com", "owner": "...", "repository": "..."},
  "remotes": [{"provider": "github", "...": "..."}],
  "sync": {"local_head": "...", "remote_head": "...", "match": true},
  "recent_commits": [{"hash": "...", "message": "..."}]
}
```

## 저장소 검색

```http
POST /api
Content-Type: application/json

{"action":"ai-search","query":"FreeLang","scope":"files"}
```

`scope`는 다음 값만 허용한다.

- `files`: 현재 Git 작업 트리의 파일 내용 검색
- `history`: 모든 branch의 커밋 메시지 검색
- `continuity`: `PROJECT-CONTINUITY.airc` 검색
- `all` 또는 생략: 위 세 영역 검색

검색 API는 `git grep`, `git log`, 고정된 AIRC 파일 검색만 수행한다. 임의 셸 명령,
commit, stage, pull, push, branch 변경은 실행하지 않는다.

## Provider별 레포 화면

GitHub와 Forgejo는 현재 로컬 `origin`을 공유하는 단순 탭이 아니다. 각 화면은
`POST /api`에 `{"action":"provider-repos","provider":"github"}` 또는
`{"action":"provider-repos","provider":"forgejo"}`를 보내 provider별 레포
목록을 별도로 읽는다. GitHub는 `gh repo list`, Forgejo는 `tea repos list`를
사용하며 모두 읽기 전용이다. 목록에서 레포를 선택하면 해당 provider의 Remote
입력값만 채워지고, 실제 origin 연결은 사용자가 별도로 확인해야 한다.

## Remote 연결

기존 저장소 연결 화면은 `POST /api`에 다음 요청을 보낸다.

```json
{"action":"connect-remote","remote_url":"forgejo-dclub:owner/repo.git","confirm":true}
```

GitHub, Forgejo(`fg.dclub.kr`), GitLab의 HTTPS/SSH 형식을 허용한다. 이미
`origin`이 있으면 덮어쓰지 않으며, 확인 후 `git remote add origin`만 실행한다.
저장소 생성, commit, pull, push는 이 흐름에서 실행하지 않는다.

## AI 사용 규칙

1. 먼저 `ai-context`를 호출해 프로젝트 정체성과 현재 `hot`을 확인한다.
2. 질문의 단어가 결정·다음 작업·계승 규칙과 관련되면 `ai-search`의 `continuity`를 사용한다.
3. 코드 위치나 구현 사례가 필요하면 `files`를 사용한다.
4. 과거 선택 이유나 회귀 사례가 필요하면 `history`를 사용한다.
5. `change_risk.level`이 `high`이면 변경 제안 전에 충돌·민감 파일을 먼저 사용자에게 알린다.
6. 이 API의 응답은 참고 문맥이다. 실제 변경은 사용자 승인과 기존 UI의 확인 절차 뒤에만 실행한다.

원격 Provider는 GitHub에 한정되지 않는다. `remote_info.provider`가 `forgejo`이면
`fg.dclub.kr`의 저장소가 현재 Remote 정본이며, 인증 토큰은 응답에 포함되지 않는다.

웹의 `Remote & Commits` 화면도 provider별로 분리되어 있다. `GitHub` 탭은
`github.com` 연결 예시와 상태를, `Forgejo (fg)` 탭은 `fg.dclub.kr` 및
`forgejo-dclub:` SSH alias 예시와 상태를 보여준다. 두 탭은 공통 읽기 API를
사용하지만 화면의 연결 의도와 provider 문맥은 섞지 않는다.

## 검증

```bash
python3 -m json.tool PROJECT-CONTINUITY.airc
curl -X POST http://127.0.0.1:40850/api \
  -H 'Content-Type: application/json' \
  -d '{"action":"ai-context"}'
curl -X POST http://127.0.0.1:40850/api \
  -H 'Content-Type: application/json' \
  -d '{"action":"ai-search","query":"FreeLang","scope":"files"}'
```
