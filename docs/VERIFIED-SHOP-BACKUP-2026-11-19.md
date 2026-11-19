# 검증된 쇼핑몰 Native 소스 보관 — 2026-11-19

이 체크포인트는 11월 18일 실제 검증과 독립 QA가 끝난 소스만 보관한다.
11월 19일 진행 중인 자원 감시기·Git UI·TTY·DB95 변경은 포함하지 않는다.
운영 배포나 main 병합은 하지 않았다. 원본 작업 트리와 미커밋 작업을 보존했다.

## 직접 실행된 범위

- Native commerce API: 60 assertions / 42 distinct labels.
- 재고 1 / 동시 주문 10: 성공 1, 잠금 충돌 503 9, 저장 주문 1, 재고 0.
- Native 화면: 7 page/Island pairs, 정상 Chrome 검증 26 assertions / 24 distinct labels.
- Native checker: 32/32. Native flp 산출물: 보존된 Node 비교본과 8/8 바이트 동일.
- Node 계열 실행: 해당 프로필의 exec 추적에서 0. 전체 앱/Provider/TTY의 결과가 아니다.
- Native UI compiler ELF SHA256:
  f200c3bb66a77e8da2680b481231a959f8254c569f24e78890c45b17a7a3dee8.
- 기존 Stage2/3/4 및 Native 자기 컴파일 **생성 C** SHA256:
  f38f373890d0898552cbe89768b992f934e9c4a567bf27cdf66248a3c1b5cb81.
  ELF SHA/Build ID는 생성 C 고정점과 별도다. Stage0/seed 실행·재생성은 없다.

## 판정 한계와 원시 자료

UI 실행 원장은 3회 사용 후 닫혔다. 첫 2회는 시험 자원 초과,
3회째 정상 기능 검증이 통과했다. 이번 보관 때문에 4회째 실행하지 않는다.
기존 SID 기반 감시기는 proc 읽기 실패 누락과 detached 자손 측정 한계가 있어
수정 중이다. 이전 관측 peak RSS 1,970,274,304 bytes는 **그 측정 범위의 관측값**이며,
cgroup 전체 메모리 상한이나 독립 실행 권한의 증거로 확대하지 않는다.

원시 로그·명령·종료 코드·입력/출력 해시는 별도 로컬 증거 보관소에 있다.
인증값, 합성 주문 저장 파일, 실제 경로, 실행 ELF는 Git에 올리지 않는다.
이번에는 원시 실행을 반복하지 않고 기존 입력·산출물 SHA를 대조했다.
독립 UI QA 문서 SHA256:
3cd43a50217565b0b58acf8575a27477b6758d30d3cd045cf9908a41242cdd90.

`NATIVE_SHOP_NORMAL_PROFILE=PASS` (위 직접 검증 범위)
`RESOURCE_MONITOR_REMEDIATION=IN_PROGRESS`
`FULL_GIT_UI_NATIVE=NOT_VERIFIED`
`DB95_DURABILITY=UNVERIFIED`
`TRUST_BOUNDARY=BLOCKED`
`CRASH_TEST=HOLD`
`RELEASE=BLOCKED`
