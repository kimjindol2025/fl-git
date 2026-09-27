# fl-git 구현 계획

## Phase 0 — 프로젝트 준비

- [x] 독립 폴더 생성
- [x] FreeLang manifest 생성
- [x] 제품 범위와 안전 규칙 문서화
- [x] TTY/PTY 경계 문서화
- [x] 실행 가능한 CLI·TUI 진입점 추가

## Phase 1 — 읽기 전용 Git 정보

- [x] 현재 작업 디렉터리와 저장소 루트 확인
- [x] 현재 branch와 remote 표시
- [x] `git status --short --branch` 표시
- [x] staged/unstaged/untracked/conflict 원문 보존 표시
- [x] 임시 저장소 테스트 추가

## Phase 2 — 안전한 변경 작업

- [x] 파일 단위 stage/unstage
- [x] diff 미리보기
- [x] commit 메시지 입력
- [x] TUI commit 입력 화면
- [x] `reset --hard`, `clean -fd` 미제공으로 기본 차단

## Phase 3 — 원격과 브랜치

- [x] fetch/pull/push
- [x] push/pull/branch-delete 명시 확인
- [x] branch 생성·전환·삭제
- [x] Git 출력 기반 conflict 오류 보존
- [x] stash 조회

## Phase 4 — 터미널 UI

- [x] 키보드 기반 메뉴
- [x] Git 상태·작업 결과 화면
- [x] TTY 크기 조회 계약
- [x] TTY capability 호스트 경계
- [ ] 마우스 선택은 TTY 계약 확인 후 추가

## Phase 5 — GitHub와 AI 문맥

- [x] `gh auth status` 연동
- [x] 저장소와 PR 목록
- [x] 현재 branch에서 PR 생성
- [x] `fl-git context` Markdown 출력
- [x] JSON context 출력
- [ ] 기존 참조 시스템 카드와 연결

## 완료 판정

각 Phase는 문서만 작성해서 완료하지 않는다. 실제 실행 결과와 임시 저장소
검증 로그가 있어야 완료로 표시한다.
