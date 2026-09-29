# Changelog

## v0.1.0 — FreeLang Dev Server Workflow

2026-09-30

이번 운영 기준선은 FreeLang 기반 `fl-git` 개발 서버 실행 경로를 정리한
릴리즈 노트다.

- `./scripts/fl-dev` 전용 개발 서버 명령 추가
- `npm run dev`가 전용 명령을 사용하도록 연결
- Front 생성물 `public/app.css`를 감시 대상에서 제외해 자기 재빌드 루프 제거
- 개발 런타임이 생성된 `web/_app.fl`을 실행하도록 정정
- README에 개발 서버와 포트 변경 방법 기록

검증:

- `npm run dev` 실제 기동
- `GET /` HTTP 200
- FreeLang AFJ `check src/main.fl` 통과
- `fl-git --help` 통과
- PM2 `fl-git` 서비스 online 유지

태그는 이번 작업에서 생성하지 않았다. 이 문서는 저장소에 푸시되는 개발 서버
운영 기준선 기록이다.
