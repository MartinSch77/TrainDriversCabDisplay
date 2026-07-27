## What & why

<!-- What does this change, and why? Reference REQ-… ids where applicable. -->

## Quality checklist

- [ ] `tools/run_tests.sh build` — all tests pass
- [ ] `python3 tools/trace_report.py` — no hard traceability gaps
      (new tests tagged `@tstid` / `@design` / `@relation(REQ-…, scope=function)`)
- [ ] `tools/static_analysis.sh build` — no new findings
- [ ] New/changed requirements edited in `requirements/requirements.sdoc`
      (not in the generated `docs/requirements.md`)
- [ ] Behaviour changes implemented in the core AND reflected in BOTH
      frontends (Qt Quick + LVGL); colors/metrics only via `design/theme.json`
- [ ] UI changes verified with `--screenshot` on both frontends
