# Security Policy

RailDeck Pro is a demonstrator for safety-adjacent railway HMI software. It
controls no real train, but the project treats findings that would matter in
a real cab — supervision bypasses, vigilance-logic defects, state corruption
in the core — as security/safety-relevant and prioritizes them accordingly.

## Reporting a vulnerability

Please use GitHub's **private vulnerability reporting** (Security → Report a
vulnerability) instead of a public issue. Include reproduction steps and the
affected component (core / ui-qt / ui-lvgl / tooling). You should receive a
first response within a week.

## Scope notes

- **Safety-logic defects**: anything that lets the simulation bypass the
  vigilance escalation (REQ-F-004/005), the door traction interlock
  (REQ-F-007) or the overspeed intervention (REQ-F-003) — report it.
- **Supply chain**: the LVGL dependency is pinned (v9.3.0, FetchContent);
  report vulnerable pinned versions of Qt, LVGL or the pipx tools.
  `./setup.sh update` is the standard remedy; CI runs SBOM + grype + trivy.
- **Secrets**: the repository is expected to contain none; the trivy secret
  scan gates CI.
