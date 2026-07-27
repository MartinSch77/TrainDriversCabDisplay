---
name: axivion-dashboard
description: Run the Axivion analysis for RailDeck Pro and verify the result on the local dashboard (versions, providers, finding deltas) via the REST API. Use when the user wants the dashboard updated, asks why providers/findings are missing, or wants finding counts per tool.
---

# Axivion run + dashboard verification (RailDeck Pro)

## Running the analysis
- Exactly ONE run at a time: `./axivion/start_analysis.sh` holds a lock
  (`~/.axivion-RailDeckPro.lock`); a second run aborts with "already active".
  Never run `clean_all.sh` or another `build_all.sh` while it analyses —
  concurrent runs delete each other's `build_axivion/` IR before upload.
  The script exits 0 with a message when `~/bauhaus-suite` is not installed.
- Full run ≈ 30–60 min. Start it in the background; while it runs, do not
  create or modify repository files (the fossil shadow phase snapshots the
  tree at the end). The script exports `RAILDECK_UI=qt` so the fetched LVGL
  third-party sources stay out of the MISRA analysis.
- The run imports every external log present in `analysis-results/`:
  providers cppcheck, clang-tidy, clazy, gcc-analyzer, codespell, asan-ubsan,
  tsan, valgrind (configured in `axivion/external_import.py` — a Python
  config layer registered in `axivion_config.json`; matchers CANNOT be
  expressed in the JSON files, the Suite validator requires real
  teecap.Match objects). Run `tools/static_analysis.sh build` and/or
  `tools/sanitize.sh` first if the dashboard should reflect current results.
  Empty logs = clean = the provider shows no open findings (correct, not a bug).

## Verifying on the dashboard (REST, read-only)
Credentials for the local dashboard come from `axivion/start_analysis.sh`.

```bash
# versions (index/date):
curl -s -u admin:password 'http://localhost:9090/axivion/api/projects/RailDeckPro' \
  | python3 -c "import json,sys; print([(v['index'],v['name']) for v in json.load(sys.stdin)['versions']])"
# SV delta between two versions (state added/removed, provider, rule, path):
curl -s -u admin:password \
  'http://localhost:9090/axivion/api/projects/RailDeckPro/issues?kind=SV&start=<N>&end=<M>&state=changed'
```

Notes: the axdashboard MCP server spawns its own empty dashboard instance on a
random port — it cannot see the :9090 uploads; use the REST API for checks.
This configuration was ported from a sibling project — treat the first
completed run as the validation of the port.
