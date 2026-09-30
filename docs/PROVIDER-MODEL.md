# Provider 모델 (2단계)

`fl-git`의 모든 원격은 GitHub 전용 구조가 아니라 **공통 Provider 모델**로 표현한다.

## 스키마

| 필드 | 타입 | 설명 |
|------|------|------|
| `provider` | string | `github` · `forgejo` · `gitlab` · `unknown` · `none` |
| `host` | string | 예: `github.com`, `fg.dclub.kr`, `gitlab.com` |
| `owner` | string | owner / org |
| `repository` | string | 저장소 **이름만** (`owner/name` 결합 금지) |
| `remote` | string | remote URL 또는 SSH alias |
| `branch` | string | 현재 로컬 branch (없으면 `""`) |
| `upstream` | string | 예: `origin/main`, `forgejo/master` |
| `auth` | string | `configured` · `missing` · `blocked` · `unknown` |
| `name` | string | (선택) git remote 이름 — `origin`, `forgejo`, `github` |

## 예시

Forgejo:

```json
{
  "provider": "forgejo",
  "host": "fg.dclub.kr",
  "owner": "kim",
  "repository": "freelang-aia",
  "remote": "forgejo-dclub:kim/freelang-aia.git",
  "branch": "master",
  "upstream": "forgejo/master",
  "auth": "configured",
  "name": "forgejo"
}
```

GitHub:

```json
{
  "provider": "github",
  "host": "github.com",
  "owner": "kimjindol2025",
  "repository": "freelang-tools",
  "remote": "https://github.com/kimjindol2025/freelang-tools.git",
  "branch": "main",
  "upstream": "origin/main",
  "auth": "configured",
  "name": "origin"
}
```

## 규칙

1. AI Context·CLI·Web API는 같은 스키마를 쓴다.
2. `provider`로 GitHub와 Forgejo를 분리한다. 같은 서비스로 합치지 않는다.
3. 인증 토큰 원문은 모델·로그·화면·AI Context에 넣지 않는다. `auth` 상태만 기록한다.
4. remote URL이 비면 `provider=none`, `auth=missing`.
5. URL은 파싱했지만 인증을 아직 검사하지 않으면 `auth=unknown` (2단계 기본).
6. 인식 불가 URL은 `provider=unknown`.
7. 표시용 full name이 필요하면 `owner` + `/` + `repository`로 조합한다.

## 인식 URL

| Provider | 패턴 |
|----------|------|
| github | `https://github.com/…`, `git@github.com:…` |
| forgejo | `https://fg.dclub.kr/…`, `ssh://git@fg.dclub.kr/…`, `forgejo-dclub:…` (구표기 `fg.dclub.jp`도 인식) |
| gitlab | `https://gitlab.com/…`, `git@gitlab.com:…` |

## 구현

- AFJ 정본: `src/provider.fl`
- CLI: `fl-git provider model` · `context-json`의 `remote_info` / `remotes`
- Web: `web/pages/api.flx`의 `api-remote-info` / `api-remote-provider`
- 테스트: `tests/provider-model.test.fl`

2단계는 **모델 표현·파싱**까지다.  
3단계는 `provider status`(auth 프로브)와 `provider repos`(목록 읽기)다. 쓰기는 포함하지 않는다.
