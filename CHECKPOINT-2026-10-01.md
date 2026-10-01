# Checkpoint · 2026-10-01

## AIRC
- spec: SPEC.airc
- hot: task-gitlab-provider-확장
- 완료: Provider roadmap 1-7
- 다음: GitLab provider 확장

## 다음 Grok에게
너는 Provider roadmap 1-7까지 끝난 상태다. GitLab provider 확장을 하면 된다.
1) `python3 ~/.grok/skills/checkpoint/scripts/print-hot.py SPEC.airc` 만 실행 (본문 10줄 이후 금지)
2) topics/work-bridge.md 읽기
3) `docs/PROVIDER-ROADMAP.md` · `docs/STATUS.md` · `./scripts/provider-gate` 참고

## 증거
- commit: `527d4c8` test: add offline provider fixture verification gate (origin/main)
- prior: `3220e64` feat: separate provider UI and gate remote writes with confirm
- gate: `./scripts/provider-gate` → PROVIDER_GATE=PASS
- smoke: provider status/repos github+forgejo PASS, web :40850, context/ai-context ok
- path: `/home/kim/kim/projects/fl-git`
