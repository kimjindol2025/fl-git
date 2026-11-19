# 쇼핑몰 보안 경로 Native 호환성 — 2026-11-18

## 판정

`NATIVE_SHOP_EXECUTION=BLOCKED`
`NATIVE_COMPILE=NOT_RUN`
`NATIVE_HTTP=NOT_RUN`
`RELEASE=BLOCKED`

기존 AFJ/Chrome 검증과 독립 정적 QA 결과는 보존한다. 이번 조사에서는 제품 소스·기존 FX 작업 트리·씨앗을 변경하지 않았다. Stage0 사용/생성/빌드, 컴파일러 대체 실행, 장애 주입, 커밋·푸시·배포는 하지 않았다.

## 기준과 출처

- fl-git 작업: `fix/shop-session-orders`, 기반 `b371d5abb747c41a1bccc60bfa96e30954399aca`.
- 보안 코드/config 18개 검증 manifest: `56a6c01a33d6aae0ed09497a25cdb183fc2822dbb8df0d09d84204bf7f56ca4c`.
- FE-18 기록의 FX 기준: `c99b6f7833e7c8e73cfd18946867d927e4228146`.
- 기록된 컴파일러 ELF: `aac71e832916db715b680760211f55053e3890141e446033eb69a35541fdbafe`.
- 조사한 임시 ELF 1,654개 및 fl-git 기존 임시 영역에서 위 SHA와 같은 파일을 찾지 못했다. 이 결과는 조사 범위의 부재이며 모든 장비에서의 유실 확정은 아니다. 기존 운영 패키지와 임시 컴파일러는 출처/해시가 달라 대신 실행하지 않았다.
- 공식 GitHub 저장소와 위 기준 커밋의 `self/cgc-main.fl`을 연결된 GitHub 도구로 확인했다. 저장소 원본 아카이브의 self/runtime/scripts/docs만 별도 임시 분석 영역에 추출했다. bootstrap/Stage0 및 bin은 추출·사용하지 않았다. 아카이브의 대상 Native ELF도 발견하지 못했으며 GitHub release 목록은 비어 있었다.
- 원본 아카이브 SHA-256: `3a910495c8287772d4b0930623e784e4a9ac75920f29991badcdd1495914eb39`.
- `self/cgc-main.fl` Git blob: `54e174fd9977b265b5bd143bac9c9676b8f7e94f`; GitHub 반환값과 추출 파일의 Git blob 해시가 일치했다. 파일 SHA-256: `551878ce34def2c94eb91ec24d26a5180e94c2f22a55929976e2a4e8b31bd01a`.

기준 원본: [FX 소스](https://github.com/kimjindol2025/freelang-v11-fx/blob/c99b6f7833e7c8e73cfd18946867d927e4228146/self/cgc-main.fl).

## 확인한 호환성 차이

애플리케이션이 사용하는 대표 API 25개를 카탈로그와 대조했다. 19개는 이름이 등록되어 있으나, 이는 실행 PASS가 아니다. 아래 6개는 기준 소스의 Native 카탈로그에 등록되지 않았다. 정규화는 `v11-compat.h:720-735`의 규칙(특정 file_/dir_/regex_ 외에는 underscore→hyphen)을 적용했다. 애플리케이션에서 해당 이름의 대체 정의도 제공하지 않는다.

| 현재 호출 | 확인 결과 | 필요한 계약/수정 쪽 |
|---|---|---|
| random_bytes | Native 이름 미등록. `auth-random-token`은 `v11-auth.h:113-120`에서 OpenSSL RAND_bytes로 같은 형태의 hex 난수를 생성 | 지원되는 함수로의 애플리케이션 어댑터 후보. 크기·오류·난수 구현 및 실제 호출을 검증한 뒤 변경 |
| hash_eq | Native 이름 미등록. 현재 앱은 고정 길이 SHA-256/HMAC hex를 비교함 | Native의 실제 상수 시간 비교 필요. 일반 문자열 = 또는 상수 true로 대체 금지 |
| hmac_sha256 | Native 이름 미등록. 현재 세션을 암호와 토큰의 HMAC으로 묶음 | UTF-8 key/message, SHA-256, 64자리 hex, 오류 전파의 실제 Native 구현 필요 |
| shell-exec-result | Native 이름 미등록. `process.c:28-40`의 _fl_process_run / shell_run은 stdout 문자열만 돌려주고 pclose 상태를 버림 | stdout/stderr/exit를 보존하는 실행 계약 또는 셸을 제거한 실제 Native 잠금·게시 API 필요. 이름만 shell-run으로 바꾸면 잠금 성공 여부를 판정할 수 없음 |
| server_req_client_ip | Native 이름 미등록 | 실제 연결 peer 주소를 요청에 넣는 계약 필요. 임의 Forwarded 헤더나 고정 unknown으로 대체해 PASS 처리 금지 |
| now_iso | Native 이름 미등록. now-ms·format-date-utc 등 다른 API는 별개로 존재 | UTC ISO-8601 밀리초 형식의 검증된 어댑터 또는 Native API 필요. 임의 문자열/0 시각 금지 |

추가로 **디렉터리 동기화 호출은 이름만 맞춰도 동작하지 않는다.** 앱의 `fl-shop-sync(root)`는 fd_open(root,"r") 뒤 fsync를 한다. 기준 `runtime/v11-fd.h:47`은 모든 fd-open 대상이 일반 파일인지 검사해 디렉터리를 거부한다. 이 보호를 느슨하게 만들지 않는다. 기존 `fd-open-with-parent`/`fd-create`가 보유한 parent descriptor와 `fd-sync-parent`를 사용하는 별도 Native 게시 경로가 후보이며, rename 전후·오류·close 순서를 실제로 검증해야 한다. AFJ와 같은 이름의 함수가 있다는 이유로 이 차이를 무시할 수 없다.

그 외 `sha256`도 등록 여부와 실행 계약을 나눠 확인해야 한다. `aliases.c:1575`에 실제 OpenSSL 경로가 있지만 `crypto-stub.c:5`의 no-crypto 빌드는 `(sha256-unavailable)` 문자열을 반환한다. 이 경로는 인증·세션에 사용할 수 없다. 실제 링크 옵션·객체와 알려진 UTF-8 SHA/HMAC 벡터를 확인해야 한다.

원본 근거: [FD 계약](https://github.com/kimjindol2025/freelang-v11-fx/blob/c99b6f7833e7c8e73cfd18946867d927e4228146/runtime/v11-fd.h), [프로세스 실행](https://github.com/kimjindol2025/freelang-v11-fx/blob/c99b6f7833e7c8e73cfd18946867d927e4228146/runtime/process.c), [Native API 목록](https://github.com/kimjindol2025/freelang-v11-fx/blob/c99b6f7833e7c8e73cfd18946867d927e4228146/runtime/native-symbols.h).

## 최소 재개 순서

1. 검증된 x86_64 컴파일러와 정확히 대응하는 runtime header/object, 입력 소스·생성 C·빌드 명령·SHA를 확보한다. 새로운 빌드를 승인 범위에서 수행한다면 Stage0·씨앗 교체 없이 그 출처부터 검증한다. 현재 발견한 다른 ELF를 임의로 기준으로 삼지 않는다.
2. FX 담당 구현 범위: 상수 시간 digest 비교·HMAC, 실제 peer IP, 종료 상태를 포함한 실행 또는 Native 잠금/게시 API. 이번 fl-git 작업에서는 FX를 수정하지 않았다.
3. fl-git 담당 범위: 검증된 난수/UTC 시각 이름 어댑터, parent descriptor를 보존하는 파일 게시 어댑터. 기존 AFJ 경로와 API 의미가 같아야 한다. 런타임 실패를 무시하거나 보안 검사를 생략하는 분기는 추가하지 않는다.
4. 아래의 작은 Native 검증이 통과한 뒤에만 보안 모듈 → 합성 파일 주문 → HTTP → 기존 브라우저 흐름 순서로 확장한다. 현재 이 검증들은 모두 NOT_RUN이다.

| 작은 검증 | 실제로 확인할 값 |
|---|---|
| 출처/아키텍처 | x86_64 ELF, SHA, 대응 runtime 객체·라이브러리·컴파일 옵션. crypto-stub 사용0 |
| 난수 | 요청한 바이트 수의 hex, 연속 호출 독립성·실패 전파. 난수 품질은 구현/OS 근거와 구분 |
| SHA/HMAC/비교 | 알려진 ASCII·한글/UTF-8 벡터, 올바른 값/다른 값. 실제 상수 시간 primitive 연결 |
| 프로세스 결과 | 고정된 무해 명령의 stdout/stderr/정상 종료·비정상 종료 코드. 임의 PID 종료 없음 |
| 파일/부모 동기화 | 전용 새 합성 파일 create/write/fsync/publish/parent-sync/close, UTF-8 바이트·권한. 장애 주입 없음 |
| 실제 peer/시각 | loopback 127.0.0.1/127.0.0.2 주소 구별, 요청 헤더 위조와 peer 분리, UTC 시각 형식 |
| 정상 주문 | 카트 분리, 재고1/동시10건 성공1, 같은 요청 재시도 영수증 동일, 실제 ACK/파일 대조 |

매 무거운 실행은 ps 확인, 공용 flock, 2GiB 제한을 유지한다. 기존 동일 원인 실패 이력은 초기화하지 않고, 새 시험도 같은 원인 3회 실패 시 차단한다. 이번 조사에서 컴파일/Native 실행 횟수는0이다. 기존 실패 로그를 새 실행으로 대체하지 않았다.

## 증거와 한계

`provenance.json`, `source-files.sha256`, `native-api-matrix.json`, `archive-elf-inspection.json`은 별도 임시 분석 영역에 보관했다. 위치 메타데이터는 작업 증거 폴더 `.tmp/shop-review/20261118/native-review-location.json`에 있다. 외부 FX 소스에 포함된 JS가 fl-git 검사기의 JS 기준선에 섞이지 않도록 분석 원본은 제품 트리 밖에 두었다.

기존 FE-18의 Native25/26 기록은 standalone 입력의 컴파일·프로그램 시작/종료를 확인한 기록이다. 이번에 바뀐 인증·저장·실제 HTTP 호출의 Native 실행 증거로 확대하지 않는다. 현재 소스 카탈로그 분석은 실제 Native 동작 검증과 구분한다. 기본 AFJ/Chrome 성공을 Native 성공으로 계산하지 않는다.

## 독립 정적 QA

별도 검증자가 추출 소스와 manifest를 읽기 전용으로 대조했다. 6개 API 미등록, 이름 정규화, 디렉터리 FD 거부, stdout 전용 프로세스 실행, crypto-stub의 위험, 19/25를 실행 성공과 구별한 표현이 근거와 일치한다고 확인했다. 아카이브·self 소스·각 소스 파일 해시 대조도 일치했다.

이 판정은 호환성 분석의 정적 QA다. 검증자는 ELF 후보 전체 검색이나 원격 조회를 재실행하지 않았으며 컴파일·런타임·시험·Stage0를 실행하지 않았다. Native 기능 PASS로 확대하지 않는다.
