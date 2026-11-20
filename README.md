# fl-git — 화면으로 쓰는 서버 Git 작업공간

서버에서 VS Code Source Control처럼 Git을 화면으로 관리하는 FreeLang 제품.

`fl-git`의 본체는 브라우저 UI다. 폴더를 선택하고, 변경을 확인하고, 파일을
Stage하고, 커밋한 뒤 GitHub 연결·branch를 확인하고 Pull/Push한다. CLI와 TUI는
같은 동작을 터미널에서 수행하는 보조 경로이며, AI context는 부가 기능이다.

## 현재 상태

- 브라우저 UI, CLI, TUI, GitHub 연결, AI 문맥 출력까지 구현된 실행 가능한 MVP
- 기존 `fl-split-term`의 TTY/PTY 계약을 참고하되 소스는 독립 유지
- Git 명령 실행, TUI, GitHub 연동을 AFJ로 구현 완료
- 본체는 Git 프로세스 권한이 필요한 AFJ(`.fl`)로 구현한다.
- 기본 CLI·TUI는 Native 실행 파일과 런타임의 TTY 호스트를 사용한다.

## 실행

검증된 Native SDK의 CLI와 TUI 실행 파일을 지정한다. 기본 명령은 Node 없이
실행하며, 도구가 없으면 명확히 실패한다.

```bash
export FREELANG_NATIVE_FLGIT=<verified-native-cli-executable>
export FREELANG_NATIVE_FLGIT_TUI=<matching-native-tui-executable>
./scripts/fl-git --help
./scripts/fl-git status
./scripts/fl-git tui
```

SDK 환경과 검증 범위는 [Native 기본 CLI](docs/NATIVE-DEFAULT-CLI.md)를 따른다.

## 테스트

FreeLang 테스트는 AFJ 공용 `deftest`/`is`/`is=`/`run-tests` 모델을 사용한다.
`tests/*.test.fl` 파일을 자동 발견해 문법 검사와 실행을 함께 수행한다.

검증된 compiler·runtime SDK 변수는 [Native 시험 실행](docs/NATIVE-TEST-RUNTIME.md)을
따른다. 기본 시험 명령은 Node 없이 실행한다.

```bash
./scripts/fl-test
```

`scripts/fl-test-native`도 같은 실행기로 사용할 수 있다. [기본 시험 명령의 검증](docs/NATIVE-DEFAULT-TEST.md)을
따른다. `npm test`는 npm 자체를 요구하며 공용 도구의 Node 경로 전환도 남아 있다.

기존 전체 회귀 검증은 다음 명령으로 실행한다.

```bash
./scripts/check.sh
```

## 페이지 검사

검증된 Native 페이지 검사기를 지정한다. 기본 명령은 Node 없이 실행한다.

```bash
export FREELANG_NATIVE_FLCHECK=<verified-native-page-checker>
./scripts/fl-check
./scripts/fl-check web/pages/products.flp
```

[기본 페이지 검사 명령](docs/NATIVE-DEFAULT-CHECK.md)에 설정과 검증 범위를 기록했다.

FLP 계약 컴파일도 Native 도구를 지정해 실행한다. 기존 두 페이지 검사 후 서버·island
파일을 생성한다. [Native FLP 명령](docs/NATIVE-DEFAULT-FLP.md)에 출력 보존과 검증 범위를 기록했다.

```bash
export FREELANG_NATIVE_FLP_TOOL=<matching-native-flp-generator>
./scripts/flp-compile
```

## FreeLang Tools

자주 쓰는 개발 명령은 공용 CLI로도 실행할 수 있다.

```bash
fl-tools test
fl-tools check
fl-tools dev
fl-tools server
fl-tools git status
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

## 브라우저 UI

```bash
./scripts/fl-dev
```

개발 서버는 `.flx`/호스트 경계 파일을 감시하고 변경 시 Front 빌드와 런타임을
재시작한다. 생성되는 `public/app.css`는 감시하지 않아 자기 재빌드 루프를
만들지 않는다. 포트는 `PORT=40860 ./scripts/fl-dev`처럼 바꿀 수 있다.

개발 서버는 검증된 Native SDK의 도구를 사용하는 Native supervisor다.
기본 진입점과 검증 범위는 [Native 개발 명령](docs/NATIVE-DEFAULT-DEV.md)을 따른다.
빌드 후 바로 실행하는 명령은 다음과 같다.

```bash
./scripts/fl-git-web
```

필수 SDK 변수와 검증 범위는 [Native 기본 웹 명령](docs/NATIVE-DEFAULT-WEB.md)을
따른다. 도구가 없으면 명확히 실패하며 Node로 전환하지 않는다. 산출물은
소스 밖의 `FREELANG_NATIVE_WEB_OUTPUT` 또는 `TMPDIR` 아래에 보존한다.
운영용 코드와 서비스 설정은 별도로 준비한다.

```bash
./scripts/fl-release-prepare-native ./web --artifacts <기존-산출물-폴더>
./scripts/fl-release-run-native <검증된-code-폴더>
./.freelang/rollback.sh <이전-code-폴더> --artifacts <기존-산출물-폴더>
```

`.freelang/deploy.sh`는 Native 릴리스·서비스 설정 준비 명령에 연결된다.
성공 상태는 `PREPARED`, 서비스 적용은 `ACTIVATION=NOT_RUN`이다.
롤백도 명시한 이전 코드의 설정만 준비하며 서비스를 재시작하지 않는다.
SDK 설정, 실행 옵션과 미검증 범위는 [Native 릴리스 계약](docs/NATIVE-RELEASE.md)을
확인한다. 실제 운영 적용과 상용 승인은 아직 완료되지 않았다.

브라우저에서 [http://127.0.0.1:40850](http://127.0.0.1:40850)을 열면 저장소
대시보드가 표시된다. 화면에서 허용된 폴더를 탐색해 선택하거나 경로를 입력할 수
있고, 현재 폴더에 새 로컬 Git 저장소를 만들 수 있다.

## 안전 원칙

- 현재 저장소·브랜치·원격을 먼저 표시한다.
- `push`, `pull`, `reset`, `clean`, 브랜치 삭제는 실행 전 확인한다.
- `reset --hard`, `clean -fd`는 기본 차단한다.
- GitHub 인증 토큰을 파일이나 원격 URL에 저장하지 않고 기존 `gh auth`를 사용한다.
- 실행 결과와 실패 원인을 숨기지 않는다.

자세한 범위는 [`docs/PROJECT.md`](docs/PROJECT.md), 단계별 계획은
[`docs/PLAN.md`](docs/PLAN.md), TTY 경계는 [`docs/TTY-CONTRACT.md`](docs/TTY-CONTRACT.md)를 참조한다.
내부용과 외부용 보안 경계는 [`docs/SECURITY-MODEL.md`](docs/SECURITY-MODEL.md)에 기록한다.
