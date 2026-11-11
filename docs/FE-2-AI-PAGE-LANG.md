# FE-2 AI 전용 페이지 언어 설계 초안

상태: 설계 초안

목표: AI가 페이지·API·컴포넌트·폼·주소의 관계를 추측하지 않고 작성하며, 컴파일러가 실행 전에 구조 오류를 차단하는 FreeLang Front 언어를 만든다.

## 1. 설계 원칙

1. 사람에게 예쁜 문법보다 구조를 우선한다. 화면의 데이터, 이벤트, 주소, 응답 형식이 명시되어야 한다.
2. 같은 화면을 만드는 방법은 하나만 둔다. HTML 문자열, 임의 JavaScript, 다른 DSL을 섞지 않는다.
3. 컴파일 때 엄격하게 막는다. 실행 중에 발견되는 오류를 정상 응답이나 빈 화면으로 바꾸지 않는다.
4. 오류는 AI가 바로 수정할 수 있는 고정 형식으로 출력한다.
5. 파일과 주소를 일대일로 연결한다. 페이지 파일을 읽으면 주소와 입력값을 알 수 있어야 한다.
6. 화면은 데이터 계약을 소비한다. API 응답 형식과 화면에서 읽는 값이 일치해야 한다.

## 2. 기본 파일과 주소

```text
pages/index.flp          → /
pages/products.flp       → /products
pages/products/[id].flp  → /products/:id
pages/api/products.flp   → /api/products
```

`[id]`는 동적 주소 변수다. 페이지는 `params.id`를 선언한 뒤에만 사용할 수 있다.

주소는 페이지 파일에서 자동 생성하지만, 링크와 API 호출은 컴파일러가 등록 목록과 대조한다.

## 3. 블록

### PAGE

페이지의 주소, 입력, 화면 진입점을 선언한다.

```text
PAGE ProductList {
  path = "/products"
  render = ProductListScreen
  data = Products.list
}
```

### COMPONENT

재사용 가능한 화면 구조다. 모든 입력값과 반환 화면을 선언한다.

```text
COMPONENT ProductCard {
  props = { id: ID, name: String, price: Money }
  emits = { select: ID }
  render = ProductCardView
}
```

### FORM

입력값, 검증, 제출 API를 한 계약으로 묶는다.

```text
FORM ProductSearchForm {
  fields = { q: String? }
  submit = Products.search
  result = ProductSearchResult
}
```

FORM의 입력 규칙은 필드마다 한 줄로 선언한다. 이 줄이 브라우저 오류 표시와 서버 400 검증의 공통 계약이다.

```text
FORM OrderForm {
  fields = [recipient phone address note]
  submit = Order.create
  rule recipient = required min-length=2 max-length=50
  rule phone = required phone
  rule address = required min-length=5 max-length=200
  rule note = max-length=200
}
```

지원 규칙은 `required`, `min-length`, `max-length`, `phone`, `numeric`, `email`이다. 규칙 이름과 필드 목록은 컴파일러가 브라우저·서버 산출물 양쪽에 전달하며, 애플리케이션은 같은 규칙을 두 번 선언하지 않는다.

FORM의 `fields`는 `submit`이 가리키는 API의 `input`과 반드시 같아야 한다. 필드가 빠지거나 더 있으면 `E_FORM_INPUT_MISMATCH`로 컴파일을 중단한다. 지원 목록 밖의 규칙 이름도 `E_UNKNOWN_INPUT_RULE`로 중단한다.

### API

메서드, 주소, 입력, 응답, 오류를 명시한다.

```text
API Products.list {
  method = GET
  path = "/api/products"
  response = ProductListResult
  errors = [Unauthorized, ServerError]
}
```

### ROUTE

주소에서 페이지나 API로 연결되는 공개 계약이다. PAGE와 API가 자동 등록되더라도 최종 주소 충돌과 링크 대상을 검사할 수 있도록 컴파일 결과에 포함한다.

```text
ROUTE ProductDetail {
  path = "/products/:id"
  params = { id: ID }
  page = ProductDetailPage
}
```

### DESIGN / LAYOUT

디자인 값은 `DESIGN` 한 곳에만 선언하고, 공통 틀은 `LAYOUT`에서 그 이름을 가져온다. 페이지는 `layout`, `title`, `description`을 반드시 선언한다. LAYOUT은 가게 이름과 상품·장바구니 링크, 장바구니 개수, 운영자 정보 자리, 반응형 공통 CSS를 한 번만 제공한다.

```text
DESIGN StoreTheme {
  color.primary = "#3157d5"
  color.surface = "#ffffff"
  font.body = "system-ui"
  space.page = "16px"
}

LAYOUT StoreLayout {
  design = StoreTheme
  header = "store-name products cart-count"
  footer = "operator-placeholder"
}

PAGE ProductListPage {
  path = "/products"
  layout = StoreLayout
  title = "상품 목록"
  description = "프리랭 상점 상품 목록"
}
```

없는 LAYOUT 이름은 `E_LAYOUT_NOT_FOUND`, 선언하지 않은 DESIGN 이름은 `E_DESIGN_TOKEN_NOT_FOUND`로 막는다. title/description 누락은 `W_META_MISSING` 경고로 보고하되 다른 계약 오류처럼 통과를 위장하지 않는다.

### ACTION / BUTTON

브라우저 버튼과 서버 API 연결은 BUTTON 블록 하나로 선언한다. `action`은 등록된 API 이름이어야 하고, `sends`는 API의 `input` 필드와 같아야 한다. `success`는 성공 응답으로 갱신할 화면 값이다.

```text
BUTTON AddToCartButton {
  action = Cart.add
  sends = [id quantity]
  success = [cart-count cart-total]
}
```

컴파일러는 존재하지 않는 API를 부르는 버튼과 `sends`·API `input`이 다른 버튼을 오류로 막는다. 브라우저에서는 성공 응답을 선언된 값에 반영하며 페이지를 다시 불러오지 않는다.

## 4. 상품 목록 예시

파일: `pages/products.flp`

```text
API Products.list {
  method = GET
  path = "/api/products"
  response = ProductListResult
}

COMPONENT ProductCard {
  props = { id: ID, name: String, price: Money }
  render = ProductCardView
}

COMPONENT ProductListScreen {
  data = { products: Product[] }
  uses = [ProductCard]
  links = [ProductDetailPage]
}

PAGE ProductListPage {
  path = "/products"
  load = Products.list
  render = ProductListScreen
}
```

이 페이지는 `/api/products`의 `ProductListResult.products`만 읽는다. `ProductCard`는 선언된 `id`, `name`, `price`만 받는다.

## 5. 상품 상세 예시

파일: `pages/products/[id].flp`

```text
API Products.detail {
  method = GET
  path = "/api/products/:id"
  params = { id: ID }
  response = ProductDetailResult
}

FORM ProductBuyForm {
  fields = { quantity: PositiveInt }
  submit = Products.buy
  result = PurchaseResult
}

PAGE ProductDetailPage {
  path = "/products/[id]"
  params = { id: ID }
  load = Products.detail
  render = ProductDetailScreen
  form = ProductBuyForm
}
```

`id`를 선언하지 않은 상태에서 `params.id`를 읽거나, 상세 API에 없는 응답 필드를 화면에서 사용하면 컴파일 오류다.

## 6. 컴파일 때 반드시 막을 것

최소한 다음 항목은 경고가 아니라 오류로 처리한다.

1. 없는 주소로 가는 링크: `links`에 등록되지 않은 PAGE 또는 ROUTE를 참조
2. 없는 API 호출: 등록되지 않은 API 이름·메서드·주소를 호출
3. 비어 있을 수 있는 값의 무검사 사용: `String?`를 `String`처럼 출력하거나 하위 필드 접근
4. API 응답 형식 불일치: 응답에 없는 필드, 틀린 타입, 없는 배열 원소 필드 사용
5. 같은 주소 중복: 두 PAGE·API·ROUTE가 같은 메서드와 주소를 등록
6. 쓰지 않는 컴포넌트: 선언됐지만 어떤 PAGE의 `uses`에도 도달하지 않는 COMPONENT
7. 동적 주소 변수 불일치: `[id]`, `:id`, `params.id` 이름이나 타입이 다름
8. 폼 제출 계약 불일치: FORM 필드와 API 입력 또는 결과 타입이 다름
9. 버튼 API 미등록: BUTTON이 존재하지 않는 API를 호출
10. 버튼 입력 불일치: BUTTON의 `sends`와 API의 `input` 필드가 다름

## 7. AI용 오류 형식

사람용 문장을 별도로 파싱하지 않도록 한 줄 JSON을 정본으로 한다.

```json
{
  "file": "pages/products/[id].flp",
  "line": 14,
  "code": "E_NULLABLE_FIELD",
  "what": "ProductDetailResult.name은 비어 있을 수 있는데 String 필드로 사용했습니다.",
  "fix": "name을 required로 바꾸거나 fallback/default를 선언하세요.",
  "related": ["ProductDetailResult.name", "ProductDetailPage"]
}
```

오류 코드 예:

```text
E_LINK_NOT_FOUND
E_API_NOT_FOUND
E_NULLABLE_FIELD
E_RESPONSE_MISMATCH
E_DUPLICATE_ROUTE
E_UNUSED_COMPONENT
E_PARAM_MISMATCH
E_FORM_CONTRACT
```

컴파일러는 `errors[]`, `warnings[]`, `summary`를 포함한 진단 문서를 함께 내보낸다. 구조 오류는 `errors[]`에만 기록하고 산출물을 만들지 않는다.

## 8. 현재 .flx island에서 이행하는 순서

1. 현재 `pages/index.flx`의 단일 island를 기능 단위로 나눈다. 화면 상태, API 호출, 이벤트 목록을 먼저 추출한다.
2. `/`와 `/api`를 PAGE·API 계약으로 고정하고 응답 타입을 만든다.
3. 반복되는 카드·행·패널을 COMPONENT로 옮기고 props와 emits를 명시한다.
4. `fetch!`, `fetch-post!`, `on-click`을 각각 `load`, FORM, action 계약으로 치환한다.
5. 실제 파일명에서 정적·동적 주소를 생성하고 링크·API 호출을 컴파일러의 등록 목록과 연결한다.
6. nullable 검사, 응답 형식 검사, 중복 주소 검사를 빌드 실패 조건으로 추가한다.
7. 기존 island 결과와 새 언어 결과를 같은 입력·HTTP·브라우저 상호작용으로 비교한다.
8. 모든 화면이 새 계약으로 통과한 뒤에만 기존 island 직접 작성 경로를 폐기한다.

이번 문서는 문법과 검증 계약의 초안이며, 컴파일러 구현과 기존 `.flx` 자동 변환은 다음 지시서에서 진행한다.
