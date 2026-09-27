# fl-git 작업 규칙

## 제품 방향

`fl-git`는 실제 터미널에서 Git과 GitHub 작업을 안전하게 돕는 FreeLang 기반
도구다. 웹 UI나 VS Code 확장이 본체가 아니다.

## 구현 경계

- Git 프로세스·장기 실행 업무 규칙: FreeLang AFJ `.fl`
- 짧은 검증 자동화: FreeLangScript `.fls`
- TTY/PTY 호스트 경계: 기존 `fl-split-term/host` 계약을 참고한다.
- JavaScript는 호스트 브리지나 개발 도구가 필요한 경우에만 사용한다.

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
