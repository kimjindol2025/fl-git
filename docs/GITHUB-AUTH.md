# GitHub 인증 환경변수

`fl-git`은 토큰을 코드나 저장소에 저장하지 않고, 실행 시 GitHub CLI에 인증 환경변수를 전달한다.

## 공용 파일 설정

공용 인증 파일은 프로젝트 폴더가 아닌 다음 위치에 둔다.

```text
/home/kim/kim/.config/fl-git/github.env
```

파일 권한은 `600`이어야 한다. 파일은 이미 만들어져 있으며, 아래 내용에서
토큰 부분만 직접 입력한다.

```env
GH_TOKEN=여기에_실제_토큰만_입력
GITHUB_REPOSITORY=kimjindol2025/fl-git
```

토큰을 입력한 뒤 파일 권한을 확인한다.

```bash
chmod 600 /home/kim/kim/.config/fl-git/github.env
```

`GH_TOKEN`을 권장하며, 대신 `GITHUB_TOKEN`을 사용할 수도 있다. 두 변수를 동시에
설정하지 않는다. 실제 토큰은 이 문서나 저장소에 기록하지 않는다.

## 실행

현재 셸에만 환경변수를 주입한다.

```bash
set -a
. /home/kim/kim/.config/fl-git/github.env
set +a

gh auth status -h github.com
gh auth setup-git
./scripts/fl-git push
```

작업이 끝나면 현재 셸에서 제거한다.

```bash
unset GH_TOKEN GITHUB_TOKEN
```

## 토큰 권한

Fine-grained token은 `kimjindol2025/fl-git` 저장소만 선택하고 다음 권한을 사용한다.

- `Contents`: Read and write
- `Metadata`: Read-only

Pull Request 생성까지 사용할 경우에만 Pull requests 권한을 추가한다. 필요하지 않은
조직·저장소 권한은 부여하지 않는다.

## 주의

- `/home/kim/kim/.config/fl-git/github.env`는 Git 저장소에 추가하지 않는다.
- 토큰을 커밋 메시지, URL, 로그, 브라우저 저장소에 넣지 않는다.
- 토큰이 노출되면 GitHub에서 즉시 revoke하고 새 토큰을 발급한다.
- `gh auth status`가 invalid이면 기존 인증을 정리한 뒤 환경변수 방식으로 재인증한다.
