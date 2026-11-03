# Changelog

## 0.8.4

### Bug Fixes

- Avoid Windows CRT re-execution crashes after first-use dependency bootstrap; wait for the restarted `idf.py` and propagate its exit status.
- Correct the package homepage and local source-install paths for the ESP Board Manager repository.

## 0.8.3

### Features

- Add preflight hints for temporarily bypassing assist checks with `ESP_BMGR_ASSIST_PREFLIGHT=0`.

### Bug Fixes

- Keep the placeholder `idf.py bmgr` / `idf.py gen-bmgr-config` options aligned with `-a/--amend` and `--skip-sdkconfig-check`.
