# FE-15 — Island 입력값 모양 검사

최신 판정(2026-11-15): **FE15X=계속**. 고정 32개 사례·예제6개·페이지7개의 native/Node 결과가 일치했고, CHECK·FL_TEST71/71·기존 Native18개 재검증이 통과했다. 아래 FE-15S~V 내용은 당시 증거로 보존하며, 최신 수치와 한계는 맨 아래 FE-15X 절을 따른다.

## 기준과 판정 범위

기준 저장소는 `fl-git`, 시작 커밋은 FE-14의 `d2dc91d5`다. 작업 시작 때 FX GitHub 원격 HEAD는 `1d217f53a8aade8a288c0dcad0a1ba23b610f102`로 조회됐다. 현재 사용 가능한 Native checker 컴파일러는 SHA-256 `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`이며 기존 FE-11R 기록에서 FX `6e21df51a09453d7be7b45d10e7551c90b75b555`로부터 생성된 것으로 연결돼 있다. 이 컴파일러는 확인한 FX 원격 HEAD와 일치하지 않으므로, 이번 Native 실행 결과를 FX `1d217f53` 기준 결과라고 부르지 않는다.

고정 C seed의 시작 SHA-256은 `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`다. 끝에서도 재확인한다.

## 허용 및 금지 모양

| 허용 | 금지 |
|---|---|
| 숫자, 문자열, `true`, `false`, `nil` | 함수·함수 정의·콜백·실행 가능한 이름 참조 |
| 위 값만 포함하는 목록과 맵 | DB·요청·응답·연결·소켓·서버·파일 등의 프로세스 자원 |
| 중첩 깊이 최대 4 | 순환 참조 및 깊이 4 초과 |

`E_ISLAND_PROPS_SHAPE`는 Island 선언 줄의 입력 표현에서 리터럴 scalar와 `[]`/`{}` 구조, 깊이 한도를 검사한다. AST 타입·별칭 추적을 하는 완전한 타입 검사는 아니다. `E_ISLAND_PASSES_COMPLEX`의 알려진 식별자 검사도 함께 유지한다.

## 새 검사 예제

| 예제 | 기대 결과 |
|---|---|
| `tests/fe-check/good/22-island-props-scalars.flx` | 통과: scalar 전체 유형 |
| `tests/fe-check/good/23-island-props-nested.flx` | 통과: 목록·맵 중첩 |
| `tests/fe-check/good/24-island-props-depth-four.flx` | 통과: 최대 깊이 4 |
| `tests/fe-check/errors/21-island-props-function.flx` | `E_ISLAND_PROPS_SHAPE` 거부 |
| `tests/fe-check/errors/22-island-props-server-object.flx` | `E_ISLAND_PROPS_SHAPE` 및 복잡 값 규칙 거부 |
| `tests/fe-check/errors/23-island-props-depth-five.flx` | `E_ISLAND_PROPS_SHAPE` 거부 |

기존 검사 입력 26개에 새 사례 6개를 더해 32개를 비교한다. 결과표에는 Node판과 기존 Native compiler판을 별도로 기록하며, 컴파일러 provenance가 FX 원격 HEAD와 다르면 그 차이를 명시한다.

## 7개 페이지 검사

현재 7개 Island 선언의 두 번째 인자는 모두 빈 벡터다. 이 입력들은 모양 규칙상 통과하지만, 빈 값이라 페이지별 실제 데이터 전달을 검증하지 않는다.

| 페이지 | Island | 현재 입력 | 확인된 데이터 흐름 |
|---|---|---|---|
| `web/pages/admin/orders.flx` | `AdminOrders` | `[]` | Island가 주문 API를 직접 조회 |
| `web/pages/admin/products.flx` | `AdminProductForm` | `[]` | Island가 상품 API를 직접 조회 |
| `web/pages/cart.flx` | `CartPage` | `[]` | Island가 장바구니 API를 직접 조회 |
| `web/pages/checkout.flx` | `OrderForm` | `[]` | Island가 장바구니·주문 API를 직접 조회 |
| `web/pages/index.flx` | `FlGitApp` | `[]` | Island가 API를 직접 조회; `render`보다 앞에 선언됨 |
| `web/pages/order/track.flx` | `OrderTrack` | `[]` | Island가 주문 조회 API를 직접 호출 |
| `web/pages/products/[id].flx` | `CartButton` | `[]` | SSR은 `data-product-id`를 DOM에 기록하고 Island가 DOM에서 읽음 |

## 런타임 props 전송 제한

현재 `web/island-compiler.js`는 Island AST의 두 번째 인자를 옵션으로 다루며 `[:watch "key"]` 같은 설정을 사용한다. 프론트 빌더는 Island body를 별도 script로 내보내지만 서버의 동적 값을 Island에 전달하는 직렬화·hydration 계약은 확인되지 않았다. 따라서 이 단계에서는 두 번째 인자에 실제 페이지 데이터를 넣지 않는다. 이를 props라고 가정해 값을 채우면 기존 옵션 해석이나 화면 동작이 바뀔 수 있다. 일곱 페이지의 실데이터 props 검증은 FX/프론트 런타임 계약이 확인될 때까지 보류한다.

## B-18 뒤 바로 할 일

1. FX 기준 컴파일러를 실제 원격 기준 커밋에서 재현하고 생성 C·ELF 출처를 고정한다.
2. Island 옵션과 서버→Island props를 구분하는 데이터 계약 및 직렬화·hydration 경로를 확인한다.
3. 승인된 계약에 맞춰 각 페이지에 필요한 최소 실제 값을 전달하고 7/7 검사한다.
4. Node판과 기준 FX Native판에서 32개 checker 사례의 진단 JSON·종료 코드를 대조한다.
5. 화면 출력이 달라진 경우에만 FE 회귀를 수행하고, `fl-test` 결과 및 seed SHA를 기록한다.

## 실행 결과

| 항목 | 결과 |
|---|---|
| FX remote HEAD | `1d217f53a8aade8a288c0dcad0a1ba23b610f102` |
| 사용 가능한 Native compiler | `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`; FX `6e21df51` provenance, remote HEAD와 불일치 |
| 오탐 원인 | 추적 로그에서 한 줄 검증 입력은 40자였지만 재귀 스캐너는 바깥 파일 문자열 128자를 읽고, 이후 줄의 `)`에서 잘못 거부했다. 스캐너 매개변수 `source`를 `props-source`로 분리해 바깥 바인딩 충돌을 없앴다. 검사 입력은 기존 문자열·주석 제거 결과를 사용한다. |
| Node 새 규칙 예제 | 6/6 기대대로 통과·거부. 깊이 초과 예제는 거부 기대를 바꾸지 않고 실제 깊이 5가 되도록 중첩 한 단계 보정 |
| 기존 26개 비교 | 기존 `.flp` 22개와 `20-server-island-split.flx` 1개는 기준 출력과 동일. `21-island-json-props.flx` 비교가 종료되지 않아 중단했고, 나머지 기존 `.flx` 2개는 미실행. 기존 입력 parity는 23/26 |
| 실제 페이지 shape | 빈 입력 `[]` 7/7: Node 검사기 종료 표식 0, `E_ISLAND_PROPS_SHAPE` 진단 0 |
| `fl-test` / FE regressions | 미실행; 화면 파일은 수정하지 않음 |
| `NATIVE_OK` | 기준 18/26; 전체 컴파일 미실행 |
| New JS / tracked JS lines | 새 JS 파일 0; Git HEAD 객체에서 실제 집계 49,617 |
| Seed | 시작 SHA = 종료 SHA = `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c` |

## 상태

FE-15S는 보류다. 매개변수 이름 충돌을 바로잡은 뒤 새 예제 6/6과 빈 props 페이지 7/7은 Node판에서 통과했다. 기존 26개 비교는 23개만 확인했으며, 기존 JSON props 예제가 종료되지 않아 해당 비교를 중단했다. 따라서 전체 기존 CHECK parity와 커밋·푸시는 보류한다. Native 비교와 `fl-test`는 이번 범위에서 실행하지 않았다. 서버→Island 실제 props 전달 계약도 현재 빌더에서 확인되지 않았다. JS 새 파일은 0개이며 tracked `.js`는 Git 객체 기준 49,617줄이다.

## FE-15T 기존 세 입력 원인 분류

FE-14 checker (`d2dc91d5`)를 별도 파일로 꺼내 기존 세 입력을 각각 120초 제한으로 실행했다. 세 입력 모두 제한 내 종료했다: `good/21-island-json-props.flx`는 `FL_CHECK_EXIT=0`, `errors/19-server-uses-dom.flx`와 `errors/20-island-passes-complex.flx`는 기존 기대대로 `FL_CHECK_EXIT=1`이었다.

현재 checker는 `good/21-island-json-props.flx`에서 CPU를 계속 사용하며 종료하지 않았다. 이 동작은 수정 전 checker에서는 나타나지 않아 FE-15의 shape 검사 변경으로 분류한다. 재시도 제한에 따라 같은 입력을 세 번째 진단 시도 뒤 중단했다. 일시적 trace는 제거했다. 현재 scanner의 재귀 종료 지점은 아직 특정하지 못했으며, 다른 두 기존 `.flx` 입력은 현재 checker로 재실행하지 않았다.

`FE15T=보류`: 새 규칙 테스트와 페이지 검사는 유지되지만 기존 26개 parity가 미완료이므로 커밋·푸시는 보류한다. 다음 작업은 `island-props-shape-scan`의 반복 위치를 정적으로 좁힌 뒤, 원인이 확정되기 전까지 해당 입력을 반복 실행하지 않는 것이다.

## FE-15U 재진단

시작 때 보존 패치 SHA는 `e6efc45b005b455f57921f33323920434a8f00c68fb149c12a50720b06d2e824`로 일치했다. 기준 checker는 HEAD `d2dc91d5`에서 별도 `<가림>` 파일로 꺼냈다.

수정 전 세 입력은 모두 120초 안에 끝났다: `21-island-json-props.flx` 통과, `19-server-uses-dom.flx`와 `20-island-passes-complex.flx` 기존 오류로 거부. 현재 checker의 21번 입력은 30초 제한 진단에서 끝나지 않았다. 세 번째 진단 시도 후 중단 규칙을 적용했다. 임시 기록은 `RAW_START line=4 length=82`까지 쓰였고 `CODE_READY`는 없었다. 즉 정지 관측 지점은 `source-code-line(raw)` 호출 중이며, `island-props-shape-scan`에는 진입하지 않았다. 이 관찰만으로 해당 공통 함수 자체가 결함이라고 단정할 수는 없다. 수정 전 checker가 같은 줄을 처리했으므로, 변경과의 연결 원인은 아직 해소되지 않았다.

임시 trace 함수와 출력은 제거했다. 수정 후 19·20은 미실행이고, 21은 세 번 모두 30초 진단에서 비종료했다. 새 예제 6/6 및 페이지 7/7의 이전 Node 증거는 유지되지만, 전체 26 parity는 완료되지 않았다. `FE15U=보류`; 커밋·푸시하지 않는다.

## FE-15V

shape 검사 입력은 정제된 `code` 대신 원문 `raw`를 사용하도록 바꿨고, scanner 인자 `props-source`, 규칙, 예제는 유지했다. 기존 `good/21-island-json-props.flx`는 Node checker에서 120초 제한에 걸려 종료 코드 124를 반환했다. 앞선 동일 원인 진단 3회가 있어 추가 실행을 중단했으며, 나머지 필수 비교는 미실행이다. 시작 seed SHA는 `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`; 이번 실행 뒤에는 seed 파일을 다시 해시하지 않아 종료 SHA는 미확인이다. 커밋·푸시는 하지 않았다.

다음 할 일: 문자열·주석 제거가 따옴표에서 멈추는 문제

## FE-15X — 새 FX 정식 빌드와 무제한 시간 측정

FE15X=계속. 출발 저장소·가지는 fl-git / feat/fe-15-props, HEAD는 d2dc91d51f4b479b26f3366f012d0deec36ad235였다. 처음 제공된 현재 폴더는 blog1이었으므로 fl-git을 별도로 확인한 뒤 그 저장소에서만 제품 변경을 했다. ERP 복제본을 사용하지 않았다. 실제 경로·소유자·주소는 <가림>이다.

착수 때 검사기·문서·6개 예제의 기존 WIP가 있었다. 검사기의 임시 `island-props-shape-valid?`가 항상 true를 반환하는 상태임을 확인하고 이를 통과 판정에 사용하지 않았다. 보존 패치의 실제 스캐너를 복원한 뒤 다음 두 함수만 성능 수정했다. 모든 32개 검사 입력과 기존 런처·AFJ 러너의 SHA는 전후 동일하다. 새로 쓴 시험이나 기대값 변경은0이며, 시작 전에 존재하던 6개 WIP 예제를 원문 그대로 버전 관리에 남겼다.

### FX 출처와 자원 제한

GitHub에서 c99b6f7833e7c8e73cfd18946867d927e4228146을 받아 독립된 CLEAN 소스로 만들었다. 고정 C seed부터 Stage1~4를 실제 재빌드하고 runtime 객체도 새로 컴파일했다. 과거 compiler ELF를 사용하지 않았다.

```text
FX_SOURCE=c99b6f7833e7c8e73cfd18946867d927e4228146
FX_COMPILER_ELF_SHA=aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
STAGE2_SHA=f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81
STAGE3_SHA=f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81
STAGE4_SHA=f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81
SEED_SHA_BEFORE=f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c
SEED_SHA_AFTER=f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c
```

매 무거운 작업 직전 ps를 보존하고 공통 flock 아래에서 순차 실행했다. 강제 잠금 해제는 없었다. 물리 메모리 상한2147483648바이트·swap0·가상 주소 공간2097152KiB·core0을 유지했다. 빌드·측정·진단의 일반 산출물과 TMPDIR·AFJ 오류 로그는 모두 작업 전용 영역 <가림>에 두었다. 측정 명령에 timeout을 붙이지 않았다. 원래 회귀 런처의15초 및 기존 standalone Native 실행의10초 조건은 바꾸지 않았다.

### 세 보존 패치와 단계 정의

```text
PATCH_1=e6efc45b005b455f57921f33323920434a8f00c68fb149c12a50720b06d2e824
PATCH_2=0b9822d75dccc98c6106df96607c1135d0b6acd99d8307472800f763b786a704
PATCH_3=958ac20d7b5a530f21e935567ec3886b5ba120f03128baeb73119094908c9acf
```

앞의 두 패치는 검사기 부분이 byte 동일했고 문서 기록이 달랐다. 마지막 패치는 shape 호출에 정제 code 대신 raw를 전달한다. 중복 패치를 누적 적용하지 않았다. 기본은 HEAD 원문, 1단계는 실제 shape 함수 정의만 추가하고 호출은 연결하지 않은 상태, 2단계는 마지막 보존본의 실제 함수·raw 호출까지 연결한 상태다. 1단계의 미호출 함수도 빈 함수로 대체하지 않았다. 이 분리 기준을 실행 전에 알리고 같은 입력·루트로 비교했다.

### 시간 제한 없는 원래 단계 측정

아래는 새 FX로 생성한 native checker의 실제 실행 시간/최대 RSS다. compile/link 시간은 이 실행 시간에 포함하지 않는다. 각 프로세스 종료0과 별도로 검사기의 FL_CHECK_EXIT 표식을 확인했다.19·20의 거부 표식1도 정상 기대 결과다.

| 입력 | 기본 초/KiB | 1단계 초/KiB | 2단계 초/KiB | 표식 |
|---|---|---|---|---|
| good/21-island-json-props.flx | 0.02 / 7184 | 0.01 / 7708 | 0.01 / 7168 | 0 / 0 / 0 |
| errors/19-server-uses-dom.flx | 0.02 / 7048 | 0.02 / 7112 | 0.02 / 7144 | 1 / 1 / 1 |
| errors/20-island-passes-complex.flx | 0.01 / 7256 | 0.01 / 7116 | 0.01 / 7120 | 1 / 1 / 1 |

19는 E_SERVER_USES_DOM,20은 기존 E_ISLAND_PASSES_COMPLEX를 유지했다.2단계에서는20에 E_ISLAND_PROPS_SHAPE도 추가됐다.21은 진단 없이 통과했다.

동일한21을 기존 Node AFJ 실행 경로에서도 제한 없이 기다렸다. 실제로 종료했으므로 과거의30/120초 타임아웃을 무한 루프 확정 증거로 부르지 않는다.

| Node 21 | 초 | 최대 RSS KiB | native 원문과 비교 |
|---|---:|---:|---|
| 기본 | 301.21 | 115828 | 동일 |
| 1단계 | 23.33 | 112660 | 동일 |
| 2단계 | 388.36 | 377264 | 동일 |

2단계 Node 수치는 실행 중 진단용 debugger 접속이 포함된 관측값이다. 샘플 스택에는 ScopeStack.has·eval·evalSExpr·evalCond가 반복 관측됐다. Node19·20의 수정 전 단계별 시간은 미실행이다. 새 FX native의 짧은 시간만으로 과거 Node 제한 초과를 구형 FX compiler의 단독 원인이라고 단정하지 않는다. 이전 compiler ELF 재실행은 지시대로 하지 않았다.

### 느린 함수 두 곳의 수정과 실패 후보 보존

source-code-line-scan과 island-props-shape-scan의 문자별 자기 재귀를 같은 상태의 loop/recur로 바꿨다. 문자·escape·주석 처리, 허용 토큰, 깊이4, 기존 오류 코드·문구는 유지했다. shape는 raw 입력을 사용한다. 효과는 같은 Node 실행 경로에서 아래처럼 확인했다.

첫 후보는 Node21을0.57초로 줄였지만 native19의 DOM 진단을 없애므로 거부했다. 반복 상태 이름을 분리한 두 번째 후보도 같은 native19 원문 차이로 거부했다. 둘 다 임시 영역에서만 실행했으며 실제 도구에 채택하지 않았다. 기대값을 바꾸거나 오류를 숨기지 않았다.

생성 C와 읽기 전용 FX 소스를 대조한 결과, loop의 recur-aware cgc-cond-wr는 평면 cond를 fl_nil()로 생성했다. 해당 두 함수에서 같은 뜻의 중첩 분기 [조건 결과] 표현을 사용한 세 번째 후보는19·20·21의 기존 출력과 모두 일치했다. 동일 점검의 세 번째 실패는 발생하지 않았다. 이 검증 뒤에만 실제 도구의 임시 true 함수를 최종 구현으로 교체했다.

언어 개선 후보: native loop 안의 평면 cond lowering 누락. 이번에는 FX를 수정하지 않았고, 지정 compiler에서 지원하는 동등한 FreeLang 표현을 사용했다.

| 최종 입력 | native 초/KiB | Node 초/KiB | 수정 전2단계 원문과 비교 |
|---|---|---|---|
| 21 | 0.02 / 7136 | 0.52 / 90668 | 양쪽 동일 |
| 19 | 0.02 / 7116 | 0.48 / 89424 | 양쪽 동일 |
| 20 | 0.01 / 7092 | 0.52 / 91328 | 양쪽 동일 |

21은 두 실행 경로 모두120초 미만이다. 제한 시간을 높여 통과시키지 않았다. Node2단계 관측388.36초와 최종0.52초는 같은 검사 입력이며, 전자는 위 진단 접속 조건을 함께 읽어야 한다.

### 최종 관문과 범위

```text
CHECK_CASES=32/32
EXAMPLES=6/6
NODE_NATIVE_DIFF=0
PAGES=7/7
CHECK_DEFAULT=PASS
ORIGINAL_CHECK_PIPELINE=PASS
FL_TEST=PASS
NATIVE_OK=18/26
NATIVE_RECHECKED=18
NATIVE_UNEXECUTED=8
ORIGINAL_FIXTURES_RUNNERS_UNCHANGED=PASS
```

32개 전체에서 expected 통과/거부 표식과 native/Node 원문을 비교했다. 새6개는 shape 진단도 확인했다.7개 페이지도 양쪽에서 진단 없이 통과했다. 이7개는 기존 빈 입력[]의 구조 검사이며 실데이터 props 전송·직렬화·hydration·실제 화면 동작 검증을 뜻하지 않는다. 해당 런타임 계약의 기존 보류 범위를 그대로 둔다.

원래 scripts/fl-check 기본 실행과 native의 launcher 표식 처리 결과도 일치했다. 원래 scripts/check.sh를 수정 없이 실행해 CLI·smoke·FL_TEST·Provider gate까지 통과했다. FL_TEST의7개 파일 원문 합계는71/71이다.

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

기존 FE13 standalone26 코퍼스에서 과거 통과18개를 새 FX로 각각 다시 C 생성·링크·실행해 모두 종료0을 확인했다.8개의 과거 실패 입력은 이번에 실행하지 않았으며 새 compiler에서도 실패한다고 추정하지 않는다. 미실행은 src/tui.fl 및 기존7개 browser/Island 페이지다. 따라서18/26은 재확인한 하한이며26개 전체 완료를 뜻하지 않는다. 고정 입력32개 및 기존 런처·AFJ runner의 전후 SHA는 모두 동일했다.

```text
CHECKER_SOURCE_SHA=a843a95950488242049c69d425f57481b657e4a525017a64d70aee4bf386cb7b
AFJ_RUNNER_SHA=2827f82534025ea18e06050dd56dfeefbd176c71f5eac887d4edd10d0e900052
IMPLEMENTATION_HEAD=daa36f82a6c9ddea23f98ba495f841f7040e50cb
```

새 JS 파일0·기존 JS 수정0이다. 실제 제품 배포·서버 발행·외부 메일·외부 AI 연결은 하지 않았다. 진단용 로컬 inspector도 종료됐고 지정 시험 포트의 리스너는0이다. 구현 diff의 민감정보 검사는 PASS였으며 보고서 diff도 커밋 전에 확인한다. GitHub만 푸시하고 Forgejo는 보류한다. 최종 보고서 커밋 HEAD·원격 HEAD 일치·CLEAN은 완료 응답에서 실제 값으로 기록한다.

언어 성과: FreeLang으로 구현한 두 문자 스캐너를 반복문으로 바꿔 Node 경로의21번을388.36초 관측에서0.52초로 줄였고, 새 FX native 경로는0.02초로 통과했다. 두 실행 경로의32개 원문 차이는0이며 새 JS는0이다. 순수한 구·신 FX compiler 버전 간 속도비는 옛 ELF를 실행하지 않아 미실행이다.

```text
FE15X=계속
시작점검=fl-git / feat/fe-15-props / d2dc91d5 / 보존 패치3개 SHA 확인
FX_실행파일=c99b6f78 정식 빌드, SHA aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe
21번_시간(기본/1단계/2단계)=native 0.02/0.01/0.01초; 최종 native0.02초/Node0.52초
19번_시간=native 0.02/0.02/0.02초; 최종 native0.02초/Node0.48초
20번_시간=native 0.01/0.01/0.01초; 최종 native0.01초/Node0.52초
원인=검사기 느림(Node AFJ 재귀 문자 스캔 실행 경로); 구형 FX 단독 원인 확정 불가
고친_곳=tools/fl-check.fl의 두 문자 스캐너와 기존 shape 호출 연결; 시작 전6개 예제는 내용 그대로 보존
CHECK=32/32, 기본 CHECK 및 원래 CHECK 파이프라인 PASS
기본검사기_Node_차이=0
예제=6/6  페이지=7/7(빈 입력 구조 검사)
FL_TEST=71/71
NATIVE_OK=18/26(18개 재검증, 나머지8개 미실행)
NEW_JS=0
HEAD=daa36f82a6c9ddea23f98ba495f841f7040e50cb(검증 구현; 이후 보고서만 커밋)
```
