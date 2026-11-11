# AI Provider 설정

1단계 AI Chat은 OpenAI 호환 Chat Completions API를 사용한다. API 키는 서버
환경변수에서만 읽으며 브라우저에 전달하지 않는다.

~~~
export FL_AI_API_KEY='발급받은_API_KEY'
export FL_AI_MODEL='gpt-4o-mini'
export FL_AI_BASE_URL='https://api.openai.com/v1/chat/completions'
./scripts/fl-dev
~~~

OpenAI 호환 서버를 사용할 때는 'FL_AI_BASE_URL'만 해당 endpoint로 바꾼다.
'FL_AI_API_KEY'가 없으면 웹의 AI Chat은 'AI_NOT_CONFIGURED' 상태를 표시하며
provider 호출을 시도하지 않는다.

## 확인

~~~
curl -X POST http://127.0.0.1:40850/api \
  -H 'Content-Type: application/json' \
  -d '{"action":"ai-status"}'
~~~

'configured'는 키의 존재 여부만 반환하며 키 원문은 반환하지 않는다.

~~~
curl -X POST http://127.0.0.1:40850/api \
  -H 'Content-Type: application/json' \
  -d '{"action":"ai-chat","message":"현재 저장소 상태를 요약해줘"}'
~~~

1단계에서는 AI가 파일·Git·셸을 변경하지 않는다. 파일 검색 도구는 2단계,
diff 생성과 적용은 3단계에서 추가한다.
