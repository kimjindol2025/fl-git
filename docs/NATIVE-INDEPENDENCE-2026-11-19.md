# Native 독립성 네 작업 진행 결과 — 2026-11-19

전체 판정: **REVISE / STOP3**. 프로세스 식별 계열 검증 실패가 3회 누적되어
추가 Native·Guard·Git UI·DB 실행을 중단했다. 네 번째 실행은 승인되지 않았다.
기존 실패 기록과 성공 증거, 미커밋 변경을 모두 보존했다.

## 네 작업 판정

| 작업 | 확인된 결과 | 남은 범위 |
|---|---|---|
| 자원 감시기 | C 단위 15/15, 정상·분리 자손·PID 보호·예산 차단·기존 증거 거부 통과. 소스와 도구를 고정한 재빌드 ELF가 기존 ELF와 바이트 동일 | 단명 자식 식별 부하에서 실패. 전체 감시기 REVISE / 3회 차단 |
| 검증본 백업 | storefront c10a3d7a는 GitHub·FG, FX 7b74fc3b는 GitHub 원격 HEAD 일치 확인 | FX의 정확한 FG 저장소 주소 미확보. 새 미검증 변경은 별도 보존 묶음 |
| Native Git / TTY | x86_64 ELF 6개 링크, CLI 실제 상태·Diff·Stage·Unstage·Commit·오류·인용 파일명과 TTY 프로브 등 완료 6 case PASS | 전체 터미널 receipt는 FAIL. TUI ESC 출력 수정 재빌드 후 재시험은 HOLD. Git 화면 실제 실행·캡처는 NOT_RUN |
| DB95 정상 검증 | 원본 B06 20/20. writer 정상 종료 뒤 별도 reader의 행·revision·WAL 해시 일치 | 외부 클라이언트 ACK, LSN·Durable Tip, Checkpoint·장애·전원 내구성은 미검증 |

## Native 빌드와 실제 수정

FX 기준은 c99b6f7833e7c8e73cfd18946867d927e4228146 및 격리 미커밋 diff다.
Git HEAD만으로 현재 실행 ELF를 식별했다고 주장하지 않는다.

- compiler source self/cgc-main.fl SHA256:
  551878ce34def2c94eb91ec24d26a5180e94c2f22a55929976e2a4e8b31bd01a.
- 고정 입력·재생성 **C** SHA256:
  f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81.
- 현재 검증에 사용한 compiler2 ELF:
  b37548c2a3e51ec483f0e49c77b5ec0559551212cdd8d63954a64b129d28ae68.
- runtime2 archive:
  abf8b8b8a533ea8d592a1a8cf11472000b4e1eeb7175314b50b41c88eadf94a2.

새 전역 json_keys 등록이 기존 컴파일러 사용자 함수 alias를 가려 생성 C를
7바이트 바꾼 실패를 실제 확인했다. 전역 등록을 제거하고 Native Git 프로필에
실제 keys 기반 지역 호환 함수를 두어 f38f3738 재일치를 확인했다.
실패 생성 C e851fdec…와 원시 diff, 실패 누적 기록을 보존했다.

TUI의 \x1b 리터럴이 Native에서 문자 x1b로 출력되어 첫 전체 TUI 시험이 멈췄다.
할당된 PTY의 해당 실패 기록에는 termios 미복원·cleanup_incomplete도 남아 있다.
기존 bytes/finally/atexit 프로브 3개의 정확한 termios 복원 PASS와 구분한다.
제어문자를 char-from-code 27로 구성하는 7줄 수정은 재빌드만 통과했고 재시험하지 않았다.

Git Island에는 인자 4개인 if가 한 곳 있었다. 기존 JS 생성기는 마지막 인자를 버렸다.
카드들을 같은 overview 분기 str에 묶는 괄호 1개 이동을 적용했다.
정적 if 177개 중 잘못된 arity는 0개다. Native 파서·기존 시험·기대값을 완화하지 않았다.
새 화면 빌드 2회째는 감시기가 컴파일러 호출 전에 차단하여 수정본 화면은 미검증이다.

## 감시기 실패와 중단

기본 Guardian source8dfdb7e… → ELF640d2efe…의 고정 입력 검증은 통과했다.
프로세스 계보·pidfd·샘플 RSS 범위이며 물리 cgroup 상한·독립 관리자 권한은 아니다.

1. Git 화면 빌드에서 CHILD_IDENTITY_UNAVAILABLE. 정확한 errno는 기록이 없어 UNVERIFIED.
2. 새 감시기를 구 감시기 아래 넣은 합성 검증에서 구 감시기가 먼저 종료시켜 검증이 오염됨.
3. 같은 수정본을 최상위 감시기로 둔 최종 합성 검증도 BLOCKED.

최종 실제 기록은 errno3(ESRCH) pin 실패 29건, 같은 FD의 PID 부재 확인 26건이다.
첫 차단 유발 1건과 차단 뒤 2건의 확인 실패 세부 조건은 UNVERIFIED다.
시험 driver는 정상 종료 0, 잔여 자식 0이지만 감시기 PASS 조건을 충족하지 않았다.
미관측 단명 프로세스 RSS peak는 0으로 계산하지 않으며 UNVERIFIED로 유지한다.
실패 횟수 3을 영속 기록했고 차단을 해제하거나 다시 실행하지 않았다.

## DB95 직접 증거

- 소스 HEAD 05ff75608399a54a58c35174e69a73f9862de772, 현재 도달 모듈 6개.
- Native C 생성 5/5, x86_64 ELF 링크 6/6, 컴파일·링크 경고 0.
  readelf note-gap 메타데이터 경고 12개는 보존했다.
- B06 PID1845555 exit0 / 기존 20개 assertion.
- writer PID1845556 exit0 → reader PID1845557 exit0. 서로 다른 프로세스다.
- 합성 행 일치, quantity2, revision3. WAL 1404바이트, prepare/commit 각3.
- WAL SHA256 0755418d631b79a1a8ab985f629a84015dd716a3910c750ef86b0df27024057c 불변.
- 해당 완료 job의 Guardian PASS / driver0 / 잔여0 / 관측 peak32,153,600바이트.
- 추적된 Node 계열 실행 0. 원본 준비25/25·모듈6/6 해시 보존.

이 결과는 정상 in-process COMMIT 반환과 정상 종료·재개 검증이다.
외부 ACK 수집 C 도구는 링크만 했고 네트워크 실행은 0이다.
현재 ACK의 TxID·LSN·Tip·revision 계약과 역사적 1704파일 스냅샷은 미확인이다.

## 보존과 독립 QA

기존 Native 원본 입력125/125와 UI 산출물124/124를 읽기 전용 재대조했다.
완료된 독립 QA는 실제 로그·종료 코드·저장 데이터와 미완 항목을 구분했다.
전체 QA 문서 SHA256: a995cc5e74455fb6ed4e68ead5a65d98bf777e2ad3a97fa95c3ae2407d8eb927.

새 격리 변경은 완성본으로 커밋하지 않고 private 소스·패치 묶음으로 보존했다.

- 소스 보존: 60파일, SHA256 a60500f0c5ac9556ea06343ff3ccf4a2b7b6a67f8223c7a9dd93c132edf3858c.
- 실행 증거 보존: 643파일, SHA256 ff9ade90cc875204c8acba5bea35eb668848dbe9b3996e74ab51eb72017832b8.
- 원시 명령·로그·C·ELF·파일 SHA·실제 위치는 별도 비공개 보관소와 locator에 있다.
  인증값·실제 고객 정보·실제 경로·ELF를 이 문서 커밋에 포함하지 않는다.

추적 JS는 실제 49,617줄 / 9파일, NEW_JS=0.
제품 전체 Node·JS·TS 및 필수 시험 의존성 제거는 HOLD다.
기존 JS 실행기와 시험을 삭제·SKIP하지 않았다. 기존 NATIVE_OK 18/26은 재측정하지 않았다.
Stage0·seed 실행/교체/재생성, 운영 서비스·계정·권한 변경, 장애 주입·배포는 0건이다.
고정 C 입력은 시작·종료 f38f3738 일치. 역사적 Stage0 파일의 직접 SHA 재확인은 주장하지 않는다.

## 다음 별도 승인 범위

원래 지시의 "동일 원인 3회 실패 시 중단" 때문에 현재 실행 차단을 유지한다.
진단 소스 cf3bb7db5f19051c9f73b2cad59966ca4306d5974b3f42fef5e726f85883a92a는
실패한 확인 단계의 고정 reason/phase와 실제 errno 또는 poll revents만 추가했다.
수용·거부 조건은 그대로이며 정적 QA만 PASS, 컴파일·실행은 0이다.

재개 승인안은 이 소스의 격리 컴파일과 **데이터 없는 단명 자식 진단 1회**에 한정한다.
최대30초, AS2GiB, 관측RSS2GiB, 자식 최대동시16개/총256개,10ms샘플링,
정상 종료 우선·필요 시 소유 pidfd TERM만 허용한다. SIGKILL·DB·브라우저·Node 실행은 포함하지 않는다.
원래 실패 횟수는 초기화하지 않는다. 재실패·증거 누락·소유권 불일치·잔여 자식은 차단하고
원본·실패 자료·새 자료를 보존한다. 새 소스·ELF를 원본에 덮어쓰지 않으므로 롤백은
새 범위를 중단하고 기존 파일을 유지하는 것이다. 실행은 별도 사용자 승인 전 HOLD다.

`RESOURCE_GUARD=REVISE`
`FAIL_FAST=STOP3`
`DB95_NORMAL_BATCH=20/20`
`DB95_NORMAL_REOPEN=PASS`
`EXTERNAL_COMMIT_ACK=NOT_RUN`
`TRUST_BOUNDARY=BLOCKED`
`CRASH_TEST=HOLD`
`RELEASE=BLOCKED`
