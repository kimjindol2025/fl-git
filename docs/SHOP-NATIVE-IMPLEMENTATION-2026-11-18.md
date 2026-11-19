# 쇼핑몰 Native 구현·검증 — 2026-11-18

## 판정과 범위

NATIVE_COMPILER=PASS (새 격리 빌드), NATIVE_COMMERCE_HTTP=PASS,
NATIVE_CHECKER=32/32, NATIVE_FLP_OUTPUT=8/8, INDEPENDENT_QA=PASS (아래 범위).
FULL_FRONTEND_NATIVE=NOT_RUN, DB95_RECOVERY=NOT_RUN,
TRUST_BOUNDARY=BLOCKED, CRASH_TEST=HOLD, RELEASE=BLOCKED.

이번 산출물은 실제 쇼핑몰 공통 함수·장바구니·주문·관리자 API와 관리자 HTML 라우트를 포함한
759줄의 합성 시험용 Native 프로필이다. 전체 Git UI·브라우저 번들·Island 데이터 전송·화면의
Node 독립성을 완료했다고 판정하지 않는다. 과거 26개 대상의 NATIVE_OK는 이번에 재측정하지 않았다.
JSON 파일 저장 검증이며 DB95 WAL/LSN/revision 내구성 검증과 합산하지 않는다.

## 출처와 빌드

- FX 기준 commit: c99b6f7833e7c8e73cfd18946867d927e4228146.
- self/cgc-main.fl SHA256: 551878ce34def2c94eb91ec24d26a5180e94c2f22a55929976e2a4e8b31bd01a.
- 보존된 기존 Stage2/3/4 C와 새 기준·수정 런타임의 자기 컴파일 C:
  f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81 (모두 동일).
- 새 Native 컴파일러 ELF: 857e56f1638fedb6e48b780f0c714db45474146861291b0fcecfdcc07ed76bad.
- 최종 commerce ELF: c6ed169b5632c5ffda2431e48541d0cb59d24297b162c03e1b65333a02b748d8.
- 비교 대상 ELF 10개 모두 ELF64/x86_64(machine62). ELF SHA/Build ID는 생성 C 고정점과 별도 기록.
- 기존 aa... ELF를 찾았다고 주장하지 않는다. 역사적 ELF 출처는 여전히 부분 미확정이며,
  새 C→ELF 명령·소스·라이브러리·도구 해시를 별도로 보존했다.
- 기존 Stage2 C는 읽기 전용 입력으로 복사했다. Stage0·seed 실행/생성/교체는 하지 않았다.
  역사적 Stage0 파일이 현재 캐시에 없어 이번 직접 seed SHA 재대조를 주장하지 않는다.
- gcc -O2 -g0, C/OpenSSL/libcurl 등 기존 네이티브 라이브러리 사용. Node 빌드 단계 없음.
  기존 MD5 코드의 OpenSSL deprecated 경고와 readelf note-gap 경고는 숨기지 않는다.
  성공 판정은 실제 링크·실행·입력/출력/저장 결과와 함께 했다.

## 구현

| 경계 | 실제 수정 |
|---|---|
| 인증 | 실제 RAND_bytes, HMAC-SHA256, CRYPTO_memcmp 고정 SHA256 digest 비교; UTF-8; UTC 밀리초 시각 |
| 프로세스 | stdout/stderr 별도 수집, 실제 exit, EINTR,8MiB 상한, posix_spawn FD 격리 |
| IP | getpeername 실제 소켓 주소 사용; Forwarded/X-Forwarded-For 무시 |
| 파일 | exclusive0600 생성,0700 소유 디렉터리, 소스 inode/부모 검증,fsync→renameat→부모fsync |
| 쇼핑몰 | 기존 AFJ 쓰기 경로 유지; 명시적 Native 경로만 새 게시 API 사용 |
| 조회 | regex-replace 구형 pattern-first C 호출 대신 text-first/global regex_replace public API 사용 |
| 도구 | Native fl-check 런처(누락/불완전 결과 차단), 기존 flp 소스+실제 상품 헬퍼로 Native 도구 빌드 |

새 규칙·기존 시험·기대값·기존 Node 런처는 변경하지 않았다. 이전 검증본과 기존 FE17 dirty 파일도
그대로 보존했다. Native 변경은 별도 FX/쇼핑몰 작업본에만 있으며 커밋·푸시·운영 배포하지 않았다.

## 직접 검증

| 검증 | 결과 및 원시 증거 |
|---|---|
| 암호/시각 |20/20, crypto-execute.* |
| 프로세스 |11개 비교, exit7/양쪽 출력/UTF-8/320KB drain/8MiB 거부, process-execute.* |
| 파일 |14/14, 경로 탈출·symlink·동일 경로·다른 부모·재사용 거부, file-publish-execute.* |
| 접속 IP |2/2, 위조 전달 헤더 무시, peer-http-corrected-execute.* |
| 실제 commerce HTTP |60 assertion(42 distinct labels), commerce-final-traced-execute.* |
| 재고1/동시주문10 |200=1,503=9; 저장 주문1·최종 재고0, 기존 기대식 그대로 |
| 정상 재시작 |주문 재시도 동일 영수증/파일 유지, 조회·관리자 잠금 지속 |
| 관리자 |암호 없이401, 실패5회 뒤6번째429, 실제 로그인/권한/로그아웃/설정누락401 |
| 조회 |올바른 정보200·개인정보 제외, 오류404·한도429·다른IP/재시작 한도 유지 |
| Native 검사기 |32/32, 기존 Node 원시 증거와 JSON 진단 차이0, native-checker-corpus* |
| Native 페이지 생성 |8/8 바이트 해시 동일, flp-native-output-comparison.json |
| Node 실행 추적 |execve 추적상 Node/nodejs/npm/npx/bun/deno 호출0, native-exec-summary.json |
| 실제 저장 순서 |fsync(file)=0→renameat=0→fsync(parent)=0, file-publication-order.json/syscalls.log |
| 입력 보존 |최종 실행 시작/종료 125개 입력 SHA 일치; 독립 검수 |

Native 검사기/페이지 도구의 Node 비교는 이번 Node 재실행이 아니라 **보존된 기존 증거와 대조**다.
원래 보안 작업본은 이전 HTTP·브라우저·71+9 테스트가 통과한 그대로18/18 SHA를 유지한다.
이번 범위에서 기존 FL_TEST71개를 새 Native PASS로 계산하지 않는다.

첫 commerce 실행의 조회 실패와 최초 IP 하네스의 응답키 오타 실패도 별도 영수증으로 보존했다.
기존 실패를 덮어쓰지 않고 원인을 대조한 뒤 다른 실행명으로 검증했다.

## 자원·제한

무거운 작업 전 ps 확인, 공용 flock 잠금 대기, 프로세스별 가상 메모리2GiB 상한을 적용했다.
전체 cgroup 합산 제한을 구성했다고 주장하지 않는다. 기준 컴파일 최대RSS484284KiB,
수정 컴파일러 링크484480KiB, 자기 컴파일543964KiB, commerce 생성144744KiB,
직접 commerce 검증27940KiB(추적 이전 기록)였다. 시험은 loopback40850/40851에서만 수행하고 닫았다.

동일 UID의 악의적인 파일 변경·상위 디렉터리 이동까지 방어하는 독립 관리자 경계는 아니다.
rename 뒤 fsync 실패는 결과 불명확 상태이며 성공 ACK를 반환하면 안 된다. 실패/전원 장애 주입은
하지 않았다. 일반 임의 shell 명령의 실행시간 상한은 없고, 이번 앱은 고정 mkdir/rmdir만 사용한다.
프로세스 API는 glibc2.34 이상이 필요하다. Unicode 본문은 검증했지만 모든 비ASCII 저장 경로
호환성을 검증한 것은 아니다. now_iso 시각만으로 저장 순서를 입증하지 않는다.

NEW_JS=0, 추적 JS=49617줄. 제품 전체 Node/JS 의존성 제거는 아직 미완료다.

## 보존·다음 작업

원시 증거·빌드 명령·종료 코드·C/ELF/소스 해시는 evidence/ 아래에 있다. 실제 경로/소유자와
임시 인증값은 보고에 출력하지 않는다. 독립 QA는 evidence/independent-qa.md.
다음 분리 과제는 전체 프론트 Native 빌드/HTTP·SSR·Island 흐름 검증과 별도 권한 경계다.
장애·DB95 내구성·운영 배포는 이 결과로 승인되지 않는다.
