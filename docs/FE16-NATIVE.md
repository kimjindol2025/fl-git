# FE-16 — standalone Native 8개 원인 분류

FE16=보류. 기존 standalone C 생성·컴파일·링크·실행 기준을 유지했다.26개를 실제로 확인한 결과18개 성공·8개 실패이며 새 이식은0개다. 소스 검사 통과를 실행 성공으로 세지 않았다. CHECK32/32·예제6/6·페이지7/7·FL_TEST71/71과21번1초 이하 조건은 유지했다.

## 시작점과 실행 범위

fl-git의 CLEAN HEAD a8bb6fc783bca37727b1450586601f353e26f252를 확인한 뒤 feat/fe-16-native를 만들었다. ERP 복제본은 사용하지 않았다. 사용자에게 NATIVE_OK의 뜻을 확인했고, 기존 standalone 실행 기준을 유지한다는 답에 따라 진행했다. 실제 경로·소유자·주소는 <가림>이다.

FX는 지정된 정식 빌드 실행 파일만 사용했다. 파일 SHA를 시작과 끝에 다시 확인했다. FX 저장소는 읽기만 했고 소스·설정·seed·실행 파일을 수정하지 않았다.

```text
FX_HEAD=c99b6f7833e7c8e73cfd18946867d927e4228146
FX_EXECUTABLE_SHA=aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
FX_WORKTREE=CLEAN
```

매 무거운 작업 전에 ps를 보존하고 공통 flock 아래에서 순차 실행했다. 잠금을 강제로 해제하지 않았다. 물리 메모리2147483648바이트·swap0·가상 주소 공간2097152KiB·core0을 유지했다. 산출물·TMPDIR·AFJ 오류 로그는 작업 전용 영역 <가림>에만 두었다. 기존 standalone 실행의10초와 원래 CHECK 런처의15초 조건은 바꾸지 않았다.

## FE15X에서 미실행이던 8개

각 입력을 수정 전 기준으로1회씩 실행했다. C 생성은 모두 성공했다. 아래7개 페이지는 이어진 GCC C 컴파일에서 실패했고, TUI는 컴파일·링크 후 실행에서 실패했다. 측정 스크립트의 LINK 표시는 gcc compile/link 명령 단계 이름이며, 아래7건은 링커가 아니라 C 컴파일 오류다. 실패 원문을 보존했으며 같은 입력의 실패 재시도는0회다.

| 입력 | 실제 첫 실패·관측 | 묶음 | 현재 필요한 것 |
|---|---|---|---|
| src/tui.fl | stdout: Function not found: tty-raw-enter! / 최종 stderr: Function not found: tty-raw-leave! (at line144) | TTY1개 | FX의 실제 raw-mode 진입·복원 API |
| web/pages/cart.flx | error: ‘CartPage’ undeclared (first use in this function) | 컴포넌트 바인딩2개 | Island 선언을 실제 Native 서버 컴포넌트 바인딩으로 내리는 경로 |
| web/pages/index.flx | error: ‘FlGitApp’ undeclared (first use in this function) | 컴포넌트 바인딩2개 | 위와 같은 Native frontend lowering |
| web/pages/admin/orders.flx | error: ‘document’ undeclared (first use in this function) | 브라우저 경계5개 | SSR와 client Island의 대상 분리 |
| web/pages/admin/products.flx | error: ‘document’ undeclared (first use in this function) | 브라우저 경계5개 | 위와 같은 대상 분리 |
| web/pages/checkout.flx | error: ‘window’ undeclared (first use in this function) | 브라우저 경계5개 | 위와 같은 대상 분리 |
| web/pages/order/track.flx | error: ‘document’ undeclared (first use in this function) | 브라우저 경계5개 | 위와 같은 대상 분리 |
| web/pages/products/[id].flx | error: ‘document’ undeclared (first use in this function) | 브라우저 경계5개 | 위와 같은 대상 분리 |

TUI의 원문 두 곳은 다음과 같다. finally에서 leave가 실패하기 전에 enter 실패도 stdout에 드러났다.

```text
fl-git TUI 오류: Function not found: tty-raw-enter!
Uncaught error (line 2344): [tty-raw-leave!] Function not found: tty-raw-leave! (at line 144, col 0)
```

## 쉬운 묶음부터 대조한 결과

컴포넌트 바인딩2개부터 확인했다. CartPage·FlGitApp 이름만 선언해도 해결되는 문제로 판단하지 않았다. 생성 C에는 Island가 일반 함수 호출로 내려가며, 지정 FX의 native 경로에는 이를 처리하는 frontend 선언·실행 경로가 없다. 실제 생성 C 발췌는 다음과 같다.

```c
fl_v11_context_call(fl_str_val("island"), fl_named_fn(fl_str_val("island")), ...)
```

기존 web/island-compiler.js의 compileFileIslands는 Island 블록에서 서버용 flxContent와 별도 browser scripts를 함께 만든다. 두 산출물과 컴포넌트 참조가 함께 유지돼야 실제 UI가 보존된다. 현재 고정 standalone 경로는 원본 .flx를 지정 FX compiler에 직접 전달하므로 이 frontend 단계를 거치지 않는다. 임시 컴포넌트 값이나 빈 함수를 추가해 첫 C 오류만 없애는 수정은 하지 않았다.

브라우저 경계5개도 같은 frontend 문제다. document·window는 client Island의 브라우저 객체이므로 C의 가짜 전역 객체로 바꾸지 않았다. Island 본문을 삭제하거나 실행되지 않는 데이터로만 옮겨 점수만 올리는 수정도 하지 않았다. 올바른 Native 경로는 SSR·브라우저 산출물과 런타임 연결을 실제로 보존해야 한다.

TTY1개는 읽기 전용 FX의 runtime·심볼과 실제 예외를 대조했다. 현재 Native에서 tty-raw-enter!·tty-raw-leave!를 사용할 수 없다. shell 명령이나 무동작 함수로 대신하지 않았다. 따라서 현재의 FX 수정 금지·실행기 변경 금지 범위에서 채택할 이식 수정은 없었고 제품 소스 변경은0이다.

## FX 인계 사항

1. **FX 필요 — TTY raw-mode 쌍**: tty-raw-enter!·tty-raw-leave!의 실제 Native 구현, 원래 터미널 상태 복원·오류·finally 동작 계약이 필요하다. 빈 함수나 외부 명령 대체는 제외한다.
2. **프론트·FX 공동 지원 계약 — Native .flx lowering**: Island 선언의 컴포넌트 바인딩, SSR 입력과 client DOM 코드의 대상 분리, 기존 Node의 flxContent+scripts 산출물 및 실제 mount 동작을 보존하는 Native 진입 경로가 필요하다. C에 document/window를 흉내 내는 방식은 요청하지 않는다.
3. **실행 계약**: 위 지원을 지정 Native compiler에 넣거나 Native frontend 진입 단계를 별도 승인해야 한다. 이번에는 FX와 고정 실행기를 바꾸지 않았다.7개 페이지를 다른 입력으로 바꿔 통과시키지도 않았다.

이 목록을 보고서로 인계한다. 외부 AI나 메일로 전달하지 않았다. FX의 핵심 TTY 부재와 프론트 Native 변환 경로 부재를 구분하며,7개 페이지를 모두 단순한 FX 함수 이름 누락이라고 부르지 않는다.

## 마지막 회귀

변경 없는 제품 소스로 원래 관문을 다시 실행했다.32개 고정 사례와7개 페이지에서 Native·Node 원문 및 검사 표식이 일치했다. 기본 CHECK의 launcher 표식 처리 결과도 동일하다. 원래 scripts/check.sh는 CLI·smoke·FL_TEST·Provider gate까지 통과했다.

| 항목 | 결과 |
|---|---|
| CHECK 사례 | 32/32 |
| 예제 | 6/6 |
| 페이지 소스 검사 | 7/7 |
| 기본 검사기·Node 차이 | 0 |
| 기본 CHECK / 원래 CHECK pipeline | PASS / PASS |
| FL_TEST | 71/71,7개 파일 |
| Native 21번 | 0.01초 / 7144KiB, 종료0 |
| 기존 standalone 통과18개 | 새 C 생성·컴파일·링크·실행 모두 종료0 |

페이지7/7은 기존 빈 입력[]의 구조 검사이며, 위7개 페이지의 standalone 실행 성공을 뜻하지 않는다. 이번에는 새로 이식한 항목이 없으므로8개 standalone 프로그램의 Node 실행 결과 비교는 미실행이다. 소스 검사기의 원문 차이0과 이 미실행 비교를 섞지 않는다.

```text
Test Results: 3/3 passed
Test Results: 16/16 passed
Test Results: 8/8 passed
Test Results: 9/9 passed
Test Results: 15/15 passed
Test Results: 9/9 passed
Test Results: 11/11 passed
FREELANG_TEST=PASS
```

최초8개 기준 실행과 마지막18개 성공 재검증을 합친 실제 standalone 결과는 다음과 같다. 마지막 회귀 단계의18개 부분집합 기록과 전체26개 결과를 구분했다.

```text
NATIVE_EXECUTED=26
NATIVE_PASS=18
NATIVE_FAIL=8
NATIVE_UNEXECUTED=0
MIGRATED=0
```

시험32개·기존 런처·AFJ 러너의 SHA는 시작과 끝에 동일했다. 제품·시험·기대값·제한시간·실행기·PRD·FX 소스 수정은0이다. Git에 남기는 변경은 이 문서1개다. 새 JS0·기존 JS 수정0, tracked Git 객체의 newline 집계는 시작과 끝 모두49617이다. 지정 시험 포트의 리스너는0이며 운영 배포·서버 발행은 하지 않았다. 커밋 전 민감정보 diff를 확인하고 GitHub만 푸시한다. Forgejo는 보류한다. 최종 보고서 커밋 HEAD·원격 일치·CLEAN은 완료 응답에 실제 값으로 기록한다.

언어 성과: Node 없이 실행한 소스 검사32/32·페이지 구조 검사7/7과 standalone18/26을 유지했다. standalone 신규 이식은0개다. 남은8개의 실제 첫 오류와 FX·frontend 지원 요청을 분리해 남겼다.

```text
FE16=보류
시작점검=fl-git / CLEAN a8bb6fc783bca37727b1450586601f353e26f252 / feat/fe-16-native
FX_실행파일_SHA=aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
미실행8개_목록과원인=이번에8개 모두1회 재현; TTY1 / 컴포넌트 바인딩2 / 브라우저 경계5, 위 표 참조
NATIVE_OK=18/26(26개 실행,18성공·8실패)
기본검사기_Node_차이=0(소스 검사);8개 standalone Node 비교는 미실행
CHECK=32/32  예제=6/6  페이지=7/7(구조 검사)
FL_TEST=71/71
21번_시간=0.01초 / 7144KiB
FX_필요_목록=tty-raw-enter!·tty-raw-leave!, Native SSR/Island lowering·컴포넌트 연결 계약(프론트 공동)
NEW_JS=0  JS_줄수=49617(변경0)
HEAD=a8bb6fc783bca37727b1450586601f353e26f252(검증 제품 소스; 이후 보고서만 커밋)
```
