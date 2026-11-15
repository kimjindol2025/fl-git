# FE-17 — 서버 화면과 브라우저 Island 분리

FE17=계속. 지정한 다섯 화면의 standalone C 생성·컴파일·링크·실행이 성공했다. 기존 26개 입력을 전부 실행해 NATIVE_OK=23/26을 확인했다. 남은 세 건은 이번 범위에서 제외한 TUI·cart·index의 기존 실패다.

## 시작점과 실행 조건

```text
START_HEAD=254b6fdf1cf381fc21c71c8ae0396af3cc444022
BRANCH=feat/fe-17-split
START_WORKTREE=CLEAN
FX_HEAD=c99b6f7833e7c8e73cfd18946867d927e4228146
FX_EXECUTABLE_SHA=aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
FX_WORKTREE=CLEAN
```

작업 대상 저장소가 fl-git임을 확인했다. FX 소스·씨앗·tui.fl을 수정하지 않았다. 지정된 정식 빌드 실행 파일과 해당 빌드의 런타임 오브젝트를 사용했다. 무거운 작업 전에 ps를 보존하고 공통 flock 아래에서 순차 실행했다. 물리 메모리 2147483648바이트, swap 0, 가상 주소 공간 2097152KiB, core 0을 유지했다. 잠금 경로는 승인된 공통 경로 <가림>, TMPDIR·임시 산출물·진단 로그는 작업 전용 영역 <가림>이다.

기존 standalone 실행 제한 10초와 CHECK 런처의 15초를 유지했다. 기존 시험·기대값·제한시간·러너·PRD 변경은 0이다. 금지된 서버와 다른 제품에는 접근하지 않았다. 서버를 띄우지 않았으며 종료 시 지정된 두 포트의 LISTEN 수는 0이었다.

재시작 뒤 남은 빈 산출물은 완료 증거로 사용하지 않았다. 백업과 디스크 여유를 확인한 뒤 별도 결과 영역에서 검증했다.

## DOM 사용 전체 목록

원본 줄 번호는 START_HEAD와 작업 시작 시 생성되어 있던 화면을 기준으로 한다. `document` 16곳과 `window` 1곳, 총 17곳을 그대로 브라우저 원본으로 옮겼다. 아래 파일명은 저장소 안의 상대 이름이다.

| 원래 서버 파일 | 원래 줄 | 옮긴 브라우저 파일·줄 | 하는 일 |
|---|---:|---|---|
| web/pages/admin/orders.flx | 23 | web/islands/admin/orders.flx:16 | 관리자 입력 읽기 |
| web/pages/admin/orders.flx | 34 | web/islands/admin/orders.flx:27 | 인증 실패 시 폼 초기화 |
| web/pages/admin/products.flx | 27 | web/islands/admin/products.flx:20 | 관리자 입력 읽기 |
| web/pages/admin/products.flx | 38 | web/islands/admin/products.flx:31 | 인증 실패 시 폼 초기화 |
| web/pages/admin/products.flx | 50 | web/islands/admin/products.flx:43 | 상품 이름 읽기 |
| web/pages/admin/products.flx | 51 | web/islands/admin/products.flx:44 | 가격 읽기 |
| web/pages/admin/products.flx | 52 | web/islands/admin/products.flx:45 | 설명 읽기 |
| web/pages/admin/products.flx | 53 | web/islands/admin/products.flx:46 | 사진 값 읽기 |
| web/pages/admin/products.flx | 54 | web/islands/admin/products.flx:47 | 재고 읽기 |
| web/pages/checkout.flx | 22 | web/islands/checkout.flx:16 | 받는 사람 읽기 |
| web/pages/checkout.flx | 23 | web/islands/checkout.flx:17 | 연락처 읽기 |
| web/pages/checkout.flx | 24 | web/islands/checkout.flx:18 | 배송지 읽기 |
| web/pages/checkout.flx | 25 | web/islands/checkout.flx:19 | 요청사항 읽기 |
| web/pages/checkout.flx | 38 | web/islands/checkout.flx:32 | 주문 성공 후 완료 화면으로 이동 |
| web/pages/order/track.flx | 12 | web/islands/order/track.flx:5 | 주문번호 읽기 |
| web/pages/order/track.flx | 13 | web/islands/order/track.flx:6 | 연락처 뒤 네 자리 읽기 |
| web/pages/products/[id].flx | 11 | web/islands/products/[id].flx:4 | HTML에 넣은 상품 식별자 읽기 |

## 재사용할 분리 규칙

1. `web/pages/<화면>.flx`에는 서버 `render`와 HTML·데이터·요청 처리만 둔다. `document`·`window`와 브라우저 이벤트는 서버 C 입력에 넣지 않는다.
2. 브라우저 동작은 같은 상대 이름의 `web/islands/<화면>.flx`에 `(island 이름 [] ...)`으로 둔다. 이 디렉터리는 페이지 라우트·서버 헬퍼 검색 밖에 있다. 브라우저 파일에는 Island 블록과 주석만 허용하며, 다른 실행문이 있으면 Front 빌드가 오류로 중단된다.
3. 기존 Front 호스트 빌드의 `readPageSource`가 두 원본을 결합한다. 함수·경계·괄호·Island 문법 등 기존 제품 빌드 검사도 결합된 원본을 읽는다. 이후 기존 Island 추출·마운트·스크립트 삽입 경로를 그대로 사용한다. 캐시 해시에 브라우저 원본을 포함하고 개발 서버도 해당 디렉터리를 감시한다.
4. Island 입력은 FE15의 모양 규칙을 따른다. 숫자·문자열·불리언·nil·목록·맵 리터럴, 깊이 4 이하를 사용한다. 함수·서버 객체·연결을 넘기지 않는다. 이번 다섯 화면의 기존 `[]`를 유지했으며 새 props 전달 방식을 만들지 않았다.
5. 기본 검사기는 서버 파일과 브라우저 파일을 각각 대상으로 실행한다. 두 대상 모두 Native·Node 결과를 원문 대조한다. 브라우저 원본을 standalone 서버 성공 건수에 추가하지 않는다.
6. 생성되는 화면은 생성기에서 분리를 유지해야 한다. `tools/flp-compile.fl`은 상품 상세·결제를 서버 파일과 브라우저 파일로 각각 쓴다. cart 생성 내용은 그대로 유지한다. 해당 두 서버 파일은 기존 ignore 정책에 따라 산출물로 남으며, 커밋된 FreeLang 생성기에서 재현된다.

순서는 주문 조회 한 화면의 standalone 성공과 Front 산출물 동일성을 먼저 확인한 뒤 나머지 네 화면으로 확대했다.

결제의 기존 생성 코드는 서버 `render`의 마지막 닫는 괄호를 Island 뒤에 두었다. 따라서 단순히 Island 시작점에서 자르면 서버 정의가 닫히지 않고 브라우저 파일 끝에 서버 괄호가 남았다. 생성기에서 서버 정의를 먼저 닫고 기존 후처리의 뒤쪽 괄호 추가를 없앴다. 실제 Island 블록·이벤트·입력 검증 내용은 그대로다.

## 화면별 결과와 기능 보존

| 화면 | C 생성·컴파일·링크·실행 | 서버 DOM | 서버·브라우저 경계 3규칙 | 유지한 기능 |
|---|---|---:|---|---|
| admin/orders.flx | 통과 | 0 | 모두 통과 | 인증, 주문 목록, 상태 변경, 새로고침, 로그아웃 |
| admin/products.flx | 통과 | 0 | 모두 통과 | 인증, 상품 목록, 추가·수정, 검증 메시지, 로그아웃 |
| checkout.flx | 통과 | 0 | 모두 통과 | 장바구니 조회, 결제 입력·검증, 주문 요청, 완료 이동 |
| order/track.flx | 통과 | 0 | 모두 통과 | 주문 조회, 오류 표시, 결과·금액 표시 |
| products/[id].flx | 통과 | 0 | 모두 통과 | 상품 서버 조회·상세 표시, 장바구니 담기·개수 |

경계 3규칙은 E_SERVER_USES_DOM, E_ISLAND_PASSES_COMPLEX, E_ISLAND_PROPS_SHAPE이다. 기존 오류 정답에서도 각 규칙의 거부 동작을 확인했다.

기능 보존 증거:

- 다섯 브라우저 파일의 실제 Island 블록은 시작 전 원본과 바이트 단위로 같다.
- 다섯 Island가 생성하는 브라우저 스크립트도 바이트 단위로 같다.
- 동일한 임시 빌드 위치에서 생성한 전체 Front 프로그램은 주석·공백을 제외한 토큰이 전부 같다. 문자열 토큰의 내용은 바이트 그대로 대조했다. HTML·브라우저 스크립트·마운트 식별자·요청·이벤트 내용이 보존됐다.
- Native와 Node 양쪽에서 FreeLang 생성기를 실행해 상품 상세·결제의 서버·브라우저 원본이 현재 결과와 같음을 확인했다. cart 산출물도 시작 전과 바이트 단위로 같다.
- EOF 빈 줄만 정리한 세 서버 파일은 생성 C 바이트와 Native·Node 검사 출력이 정리 전과 같았다.

페이지 7/7은 소스 구조 검사다. standalone 성공은 기존 FE15X·FE16 기준인 C 생성·컴파일·링크 후 프로그램 시작·종료 0을 뜻한다. 이번에는 실제 브라우저 주문·관리자 업무 E2E를 실행하지 않았다. Front 결합 단계는 기존 호스트 빌드를 사용한다.

## 전체 검사 원문과 남은 세 건

```text
CHECK_CASES=32/32
EXAMPLES=6/6
NODE_NATIVE_DIFF=0
PAGES=7/7
BROWSER_BOUNDARIES=5/5
CHECK_DEFAULT=PASS
ORIGINAL_CHECK_PIPELINE=PASS
FL_TEST=PASS
NATIVE_OK=23/26
NATIVE_FAILED=3
NATIVE_EXECUTED=26/26
NATIVE_UNEXECUTED=0
ORIGINAL_FIXTURES_RUNNERS_UNCHANGED=PASS
```

FL_TEST의 원래 7개 시험 파일 결과를 합산하면 71/71이다. 고정 정답 32개, 예제 6개, 페이지 7개, 분리한 브라우저 원본 5개의 Native·Node 원문 차이는 0이다. 기본 CHECK 런처와 원래 전체 회귀도 성공했다. 고정 정답·러너·AFJ 실행기 해시의 전후 일치를 확인했다.

| 남은 입력 | 이번 실제 결과 | 원문 |
|---|---|---|
| src/tui.fl | 실행 종료 1 | Function not found: tty-raw-enter! / Function not found: tty-raw-leave! |
| web/pages/cart.flx | GCC C 컴파일 실패 | error: ‘CartPage’ undeclared (first use in this function) |
| web/pages/index.flx | GCC C 컴파일 실패 | error: ‘FlGitApp’ undeclared (first use in this function) |

세 파일은 수정하지 않았다. cart·index는 동일한 서버/Island 원본 분리의 다음 후보로 기록한다. 실제 분리와 섬 연결 검증은 미실행이다. TUI는 지정 FX의 실제 TTY API가 필요한 별도 작업이다.

중간 기록에는 생성 시각 주석 차이로 인한 초기 추가 산출물 비교 실패와 결제 파일의 서버 괄호 경계 오류가 남아 있다. 준비 스크립트 문법 오류는 제품 실행 전에 고쳤다. 기존 시험의 기대값·제한시간·러너를 바꾸지 않았으며 수정 후 같은 관문을 세 번 실패한 경우는 없다. 원문 결과는 작업 전용 영역 <가림>에 보존했다.

## 변경·원격 확인

변경한 정식 소스는 FreeLang 생성기 1개, 기존 JS 호스트 파일 2개, 기존 서버 원본 3개, 이동한 브라우저 원본 5개다. 상품 상세·결제의 서버 산출물 2개도 재생성·검증했다. 새 JS 파일은 0개이며 추적 JS 총 줄 수는 49617 → 49617이다. 시험 실행기인 island-runner와 프로젝트 scripts는 수정하지 않았다.

언어 성과: 서버 HTML·데이터와 브라우저 Island를 같은 상대 이름의 FreeLang 원본으로 분리하는 규칙을 확정했고, 지정 FX로 standalone 실행 가능한 입력이 18개에서 23개로 늘었다.

```text
FE17=계속
시작점검=fl-git / 지정 HEAD 일치 / CLEAN / 공통 잠금 / 2GiB 상한
FX_실행파일_SHA=aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
DOM사용_목록=위 17곳, 서버 부분 0
나누는_규칙=pages 서버 원본 + 같은 상대 이름의 islands 브라우저 원본
화면별_결과(5개)=5/5 standalone 성공, 경계 규칙 통과
NATIVE_OK=23/26
기본검사기_Node_차이=0
CHECK=32/32  예제=6/6  페이지=7/7
FL_TEST=71/71
섬연결_다음후보=cart·index (이번 미수정·분리 미실행)
NEW_JS=0  JS_줄수=49617/49617
HEAD=9e3530826b3b03a65d769cd8c22738942f923107
GITHUB_CODE_HEAD_MATCH=9e3530826b3b03a65d769cd8c22738942f923107
CODE_WORKTREE=CLEAN
FORGEJO=보류
```

위 HEAD는 검증한 코드 커밋이며 GitHub 원격 일치를 확인했다. 이 보고서를 추가한 최종 커밋과 원격 HEAD 일치·CLEAN 결과는 최종 답변에 기록한다. 커밋 전 민감정보 diff 검사는 통과했다.
