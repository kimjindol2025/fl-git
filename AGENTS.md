# 서버에서 VS Code 소스제어에 해당하는 화면. 폴더 선택부터 push까지.

`fl-git` 작업 규칙

## 제품 방향

`fl-git`는 서버에서 VS Code Source Control에 해당하는 화면을 제공하는
FreeLang 기반 **Git Provider 공통 작업공간**이다. 웹 UI가 본체이며, CLI/TUI는
같은 동작을 터미널에서 실행하기 위한 보조 경로다.

목표는 GitHub 전용 앱이 아니라 Provider 추상화 위에 GitHub·Forgejo·(확장) GitLab을
꽂는 것이다. 스키마 정본: `docs/PROVIDER-MODEL.md`.

핵심 흐름은 다음 순서를 따른다.

```text
폴더 선택 → 이 폴더 Git 상태 → 파일 선택/Stage → Commit
→ Provider 확인(GitHub/Forgejo 분리) → Pull/Push
```

AI context는 부가 기능이다. 제품의 중심은 저장소를 화면으로 선택하고
변경을 안전하게 처리하는 것이다. 직원용 업무 앱과 데이터·메뉴·인증을
공유하지 않는 독립 제품이다.

## 구현 경계

- Git 프로세스·장기 실행 업무 규칙: FreeLang AFJ `.fl`
- 짧은 검증 자동화: FreeLangScript `.fls`
- 브라우저 UI: `web/pages/index.flx`, `web/pages/api.flx`, FL-Front Island
- TTY/PTY 호스트 경계: 기존 `fl-split-term/host` 계약을 참고한다.
- JavaScript는 Front 빌드·TTY capability 같은 호스트 경계에만 사용한다.

## 안전

- 사용자의 현재 저장소 밖에서 Git 명령을 실행하지 않는다.
- Git 인자는 FreeLang에서 shell quote하고, 임의 셸 명령 자체를 입력받지 않도록
  제공된 Git 동작만 조합한다.
- 파괴적 Git 작업은 기본 거부 또는 명시적 확인을 요구한다.
- 기존 프로젝트와 전역 PM2·서비스를 건드리지 않는다.

## 검증

```bash
node /home/kim/kim/platform/freelang-afj/bootstrap.js check src/main.fl
node /home/kim/kim/platform/freelang-afj/bootstrap.js run src/main.fl --help
```

실제 Git 실행 기능을 추가할 때는 임시 테스트 저장소에서 먼저 검증한다.
