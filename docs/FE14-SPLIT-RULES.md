# FE-14 — 서버와 화면 경계 규칙

## 목적과 적용 범위

페이지 소스에서 서버 렌더링과 브라우저 동작을 혼합하지 않도록, 기존 FreeLang 폼을 기계가 판별할 수 있는 경계로 사용한다. 새 사람용 표기법이나 새 페이지 문법은 추가하지 않았다. 현재 `render`/`render_post` 함수와 `(island ...)` 폼을 그대로 사용한다.

이 규칙은 화면 파일 전체를 Native로 컴파일할 수 있게 만들지는 않는다. 검수 시 Native 직접 실행이 확인된 기준은 FE-13의 18/26이며, 이번 변경에서는 7개 실패 페이지 파일이나 생성기를 수정하지 않았다. 이 문서는 규칙과 정적 검출을 고정하고, 물리적 파일 분리는 Native 빌더의 안전한 산출물 계약이 준비될 때까지 보류한다.

## 규칙

### 서버 영역

- 서버 영역은 파일 시작부터 `(island Name [props] ...)` 전까지, 그리고 `(defn render ...)` 또는 `(defn render_post ...)` 내부다.
- 서버는 요청·응답, 파일·DB 접근, SSR HTML 생성 등 서버 기능을 담당한다.
- 서버 영역에서 `document`, `window`, 브라우저 `fetch!` 계열, DOM 조회·이벤트 등록, `:on-click`·`:on-input`·`:on-change`·`:on-submit` 등 화면 전용 기능을 직접 사용하지 않는다.
- 브라우저 조작은 기존 `(island ...)` 폼 안에 둔다.

### 화면 / Island 영역

- `(island Name [props] ...)`가 기계가 읽는 Island 시작 표식이다. 새 주석 태그나 별도 문법은 두지 않는다.
- Island는 DOM, 브라우저 이벤트, 화면 상태 및 API 요청을 담당한다.
- Island에서 서버의 내부 요청 객체, DB 연결, 소켓, 파일 핸들 등 서버 전용 리소스에 직접 접근하지 않는다. 서버 자료는 API 응답 또는 SSR 값으로 전달한다.
- 서버와 Island 사이에는 JSON으로 직렬화할 수 있는 값만 둔다: `nil`, 불리언, 숫자, 문자열, 위 값만 포함하는 배열과 맵.
- 함수/클로저, atom, 콜백, 요청·응답, DB/소켓/서버 연결, 스트림, 파일 핸들 및 그 밖의 프로세스 자원은 전달하지 않는다.
- 기존 placeholder, route/API 경로, JSON 키, DOM id는 명시적인 데이터 계약이다. 값 전달이 필요한 페이지는 이 계약을 사용하고, Island props에는 직렬화 가능한 값만 적는다.

## `fl-check` 규칙과 범위

### `E_SERVER_USES_DOM`

서버 영역의 코드에서 브라우저 전용 토큰을 찾으면 오류를 낸다. 현재 검사 토큰은 `document`, `window`, `fetch!`, `fetch-post!`, `fetch-result!`, `.getElementById`, `.querySelector`, `.addEventListener`, `:on-click`, `:on-input`, `:on-change`, `:on-submit`이다. 문자열과 `;` 뒤 주석은 토큰 검사 전에 제외한다. 위반은 파일과 줄 번호를 진단한다.

### `E_ISLAND_PASSES_COMPLEX`

Island 선언 행의 props 표현에서 함수·콜백·atom·연결·요청·응답·스트림·파일 핸들 등 복잡 값의 흔한 식별자를 찾으면 오류를 낸다. 검사에 걸리는 이름에는 `fn`, `defn`, `callback`, `handler`, `connection`, `db-`, `socket`, `request`, `response`, `stream`, `file` 등이 포함된다.

이 규칙은 현재 정적 이름 기반 보호 장치다. 전체 FreeLang AST 타입 검사나 별칭 추적은 하지 않으므로, 이름을 바꾼 값의 실제 타입까지 증명하지는 않는다. 함수와 연결 값을 임의 이름으로 우회해 전달하지 말아야 하며, 완전한 타입 검증은 별도 FX 기능으로 남긴다. props는 현재 Island 시작 행에서 검사한다.

두 규칙 모두 기존 Node 실행기와 Native 컴파일한 검사기에서 같은 진단 JSON과 종료 표식을 내는지 26개 입력으로 대조했다.

## 실패 7개 파일 분석

표의 줄 번호는 이번 FE-14 작업 트리에서 확인한 값이다. 각 파일은 서버 SSR 및 화면 동작이 모두 있지만, 두 영역은 이미 `render`와 `island` 폼으로 나뉘어 있다. 브라우저 전용 기능은 Island 쪽에만 있었다. 두 경계 사이의 현재 props는 모두 빈 `[]`다.

| 파일 | 서버 전용 부분 | 화면 전용 부분 | 공유 / 양쪽 경계 | 화면 전용 토큰 및 판단 |
|---|---|---|---|---|
| `web/pages/admin/orders.flx` | 2–6행 `render`: layout, 제목/설명, 주문 placeholder와 Island script 슬롯의 SSR HTML | 8–89행 `AdminOrders`: 인증 상태, 주문 목록 조회·상태 변경, 로그인/로그아웃·새로고침·상태 버튼 | `admin-orders-root`, `/api/admin/orders`, `/api/admin/order/:id`, 상태 필드; props `[]` | `fetch!` 14·23·47행, DOM 23·34행, `:on-click` 69·73·74·86행; 모두 Island 안. 서버 위반 없음. |
| `web/pages/admin/products.flx` | 2–6행 `render`: 상품 관리 안내 및 Island placeholder SSR | 8–135행 `AdminProductForm`: 인증, 입력값 수집, 상품 추가/수정, 화면 목록과 폼 갱신 | `admin-products-root`, 상품 API·입력 필드 계약; props `[]` | DOM 27·38·50–54행, `fetch!` 28·70행, `:on-click` 105·110·111·122·134·135행; 모두 Island 안. 서버 위반 없음. |
| `web/pages/cart.flx` | 2–3행 `render`: 제목, 설명, placeholder, script 슬롯 | 4–23행 `CartPage`: 장바구니 조회·합계와 사용자 조작 | `/api/cart`, placeholder, props `[]` | `fetch!` 11행과 Island 내 이벤트 렌더링; 서버 위반 없음. |
| `web/pages/checkout.flx` | 2–6행 `render`: 장바구니 합계 검사, 빈 장바구니 redirect, SSR 주문서 | 7–56행 `OrderForm`: 수령인 입력, 장바구니/API 호출, 주문 전송과 완료 화면 이동 | `/api/cart`, `/api/order`, `recipient`·`phone`·`address` 등의 요청 키; props `[]` | `fetch!` 15행, `document` 22–25행, `fetch-result!` 32행, `window` 38행, `:on-click` 56행; 모두 Island 안. 서버 위반 없음. |
| `web/pages/index.flx` | 1011–1048행 `render`: SSR HTML, 메타/스타일 및 shell layout | 3–1009행 `FlGitApp`: 탭·저장소·파일·AI·변경 UI와 사용자 동작 | Island placeholder와 `/api` 요청/응답 계약; props `[]`. 이 파일은 Island가 render보다 먼저 나오는 순서다. | Island 쪽에서 `fetch!`, `fetch-post!`, DOM 기반 입력 및 다수 `:on-click`·`:on-change`를 사용. 1011행에서 서버 영역으로 전환. |
| `web/pages/order/track.flx` | 2–6행 `render`: 조회 설명, 입력 영역 placeholder, SSR style | 8–43행 `OrderTrack`: 주문번호/전화 뒤 4자리 입력, 요청, 결과/오류 표시 | `/api/order/track`, `order_id`·`phone_last4`, placeholder; props `[]` | `document`·`.getElementById` 12행, `fetch!` 14행, `:on-click` 31행; 모두 Island 안. 서버 위반 없음. |
| `web/pages/products/[id].flx` | 2–7행 `render`: 요청의 상품 id, 상품 조회, 404 또는 상품 SSR, 사진·설명·가격 렌더링 | 8–19행 `CartButton`: 장바구니 추가와 버튼 갱신 | `data-product-id`, `cart-island-slot`, `/api/cart/add`; props `[]` | `.getElementById` 11행, `.getAttribute` 12행, `fetch-post!` 13행, `:on-click` 19행; 모두 Island 안. 서버 위반 없음. |

화면 전용 API 이름 전체 목록: `document`, `window`, `fetch!`, `fetch-post!`, `fetch-result!`, `.getElementById`, `.querySelector`, `.addEventListener`, `.getAttribute`, `:on-click`, `:on-input`, `:on-change`, `:on-submit`. 검사기는 위 목록 중 서버 오류 규칙에 등록한 토큰을 자동 검사한다. `.getAttribute`는 Island 경계 분석 목록에는 포함했지만 현재 `E_SERVER_USES_DOM` 탐지 목록에는 없다.

### 이번에 물리적으로 파일을 나누지 않은 이유

7개는 소스 내에서 이미 server `render`와 `(island ...)` 형태가 분리돼 있으며 기존 프론트 빌더가 Island를 추출한다. Native 쪽은 아직 한 aggregate server artifact를 받으며, FE-13에서 전체 입력은 2 GiB 제한 아래 C 생성 단계의 메모리 한계로 실패했다. 별도 route artifact의 helper/link 계약을 확인하지 않고 파일을 옮기거나 생성기를 바꾸면 SSR·API·Island 동작 회귀를 판정할 수 없다. 따라서 화면 소스는 손대지 않았고, FE-13의 직접 Native 성공 수 18/26도 그대로다. 이는 FE-14의 목표 20/26에 못 미치므로 FE-14 전체는 보류다.

## 규칙 예제

추가된 네 예제는 기존 22개 `.flp` 검사 예제 수를 바꾸지 않고 경계 규칙 사례 네 개를 보탠다.

| 분류 | 예제 | 예상 |
|---|---|---|
| 나쁜 예 | `tests/fe-check/errors/19-server-uses-dom.flx` — `render`에서 `document` 접근 | `E_SERVER_USES_DOM` |
| 좋은 예 | `tests/fe-check/good/20-server-island-split.flx` — SSR render와 DOM을 쓰는 Island 분리 | 오류 없음 |
| 나쁜 예 | `tests/fe-check/errors/20-island-passes-complex.flx` — props에 `db-connection`/callback 참조 | `E_ISLAND_PASSES_COMPLEX` |
| 좋은 예 | `tests/fe-check/good/21-island-json-props.flx` — 문자열·숫자·불리언·배열·맵 | 오류 없음 |

검사 결과: 기존 22개 `.flp`와 경계 예제 4개를 합쳐 Node판 26/26, Native판 26/26. 각 입력의 진단과 종료 표식은 byte-for-byte 같았다. 실제 `web/pages` 검사도 양쪽 종료 0, 출력 차이 0이었다.

## FE-13 기준 및 FX 후속 기능 우선순위

FE-13 기준 Native 직접 실행은 18/26이다. 이번 변경으로 7개 페이지를 Native 성공으로 추가 계산하지 않았다. FX 7f6e8304의 체크아웃을 찾지 못해 이 커밋의 소스를 이번 작업에서 직접 검사하지 못했다. 아래는 FE-13에서 남긴 관측을 기반으로 한 필요 기능 목록이며, 해당 FX 커밋에 대한 새 소스 검증은 UNVERIFIED다.

1. **Native route 단위 산출물 경계:** 서버 `render`와 Island bundle을 별도 산출·연결하고 공용 서버 helper의 심볼을 안전하게 연결하는 기능. 6개의 브라우저/교차 페이지 바인딩 실패에 필요.
2. **메모리 제한 내 incremental/chunked C 생성:** FE-13 전체 앱 입력 315,480 bytes가 제한 내 생성되지 않았고 관측 peak RSS가 1,945,364 KiB였다. route별 빌드 또는 증분 생성이 가능하도록 해야 한다. 전체 앱 컴파일은 이번에 재시도하지 않았다.
3. **Island props 구조/직렬화 검증:** 타입·별칭을 따라 JSON 값인지 검사하고 빌드 manifest에 props shape를 남기는 FX/컴파일러 기능. 이번 fl-check의 이름 기반 `E_ISLAND_PASSES_COMPLEX` 한계를 보완한다.

`src/tui.fl`의 `tty-raw-leave!`는 FE-13에 별도로 기록된 Native API 공백으로, 이번 페이지 경계 분석/7개 실패 집합 밖이다.

## 실행 증거와 상태

| 항목 | 결과 |
|---|---|
| 검사기 변경 | `tools/fl-check.fl`에 두 규칙; UI `.flx` 7개는 변경하지 않음 |
| 경계 예제 | 나쁜 예 2, 좋은 예 2; Node 4/4, Native 4/4 |
| 전체 checker 사례 | `.flp` 22 + `.flx` 4 = 26; Node 26/26, Native 26/26, raw 출력 차이 0 |
| 현재 `web/pages` checker | Node 및 Native exit 0, 출력 차이 0 |
| `fl-test` | 7 묶음 합계 71/71 |
| `flp-compile` | FE-13 0차이 증거 유지; FE-14에서는 다시 실행하지 않음 |
| FE 화면 회귀/375·1280 캡처 | 미실행; 화면 파일은 변경하지 않음 |
| `NATIVE_OK` | 18/26, FE-13 기준 유지; 20/26 목표 미달 |
| 새 JavaScript / JS 줄수 | 0 / FE-13 기준 49,617 유지 |
| Seed | FE-11R 기록 SHA와 이번 재확인 SHA 모두 `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c` |
| FX 변경 / Stage0 재생성 / 전체 앱 컴파일 | 0 / 0 / 미실행 |

이번 checker용 Native 실행 파일은 이전에 검증된 FE-11R Stage3 compiler SHA-256 `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`와 해당 runtime object를 사용했다. 입력 `tools/fl-check.fl` SHA-256은 `edaf9b5918b0027fcdbac8c4404d21a29bf08ee14a8f046230f6a1c8252062e3`, 생성 C는 `e81515e0ce75527b19838b61cceef9a913c96e0d4ecd9cbd0be15491c397c7d1`, 실행 ELF는 `ef26dda7838767609fd90d421e281e17beac9fb6da88616f31279685a7a87d91`이다. 링크에는 기존 `libcurl.so.4` 방식만 사용했다. FX 7f6e8304 checkout은 로컬 후보에서 확인되지 않아, 이 Native checker 검증을 그 커밋으로 빌드한 결과라고 주장하지 않는다.

### 판정

`FE14=보류`. 규칙 문서와 양쪽 검사기 검증은 완료했지만 Native 직접 페이지 실행 수가 18/26으로 목표보다 2개 적고, FE 화면 회귀 캡처도 실행하지 않았다. 안전한 후속 단계는 Native compiler의 route/island artifact 설계 및 증분 생성 기능을 먼저 확정한 뒤, 승인된 격리 빌드에서 화면 동작을 재검증하는 것이다.
