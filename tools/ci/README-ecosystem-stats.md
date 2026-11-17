# Ecosystem statistics

The GitLab job discovers public ESP Board Manager board definitions and dependent projects, applies maintained review decisions, archives private daily changes in the existing GitLab project's Package Registry, and publishes aggregate statistics and README banners to the GitHub `ecosystem-stats` branch. Source rules, review records, detailed results, and change history stay outside the public repository.

## Private configuration

Two files are maintained in the existing GitLab project's **Settings > CI/CD > Secure files**:

| File | Purpose |
| --- | --- |
| `ecosystem_sources.json` | Seed component roots, discovery lineage rules, source-match annotations, and evidence exclusions |
| `ecosystem_board_review.json` | Reviewed outside identities, inclusion decisions, and aliases |

Secure Files are stored outside Git, support CI downloads, and are accessible to project members with Developer or higher access. They have no Git version history. See [GitLab Secure Files](https://docs.gitlab.com/ci/secure_files/).

Local copies belong in the ignored `.ecosystem-private/config/` directory. Do not add the actual configuration or review inventory to a public branch. Tests use synthetic data and do not need these private files.

The source file's `board_review_ledger` must be exactly `ecosystem_board_review.json`. The downloader resolves both exact filenames through the [Secure Files API](https://docs.gitlab.com/api/secure_files/) automatically; no file-ID variables are needed. It uses `CI_JOB_TOKEN`, verifies SHA256 checksums and JSON structure, and creates the configuration directory only after both files pass validation. Existing directories are not replaced. Missing or ambiguous filenames, unavailable permissions, unsupported redirects, or checksum errors stop the run.

## Counting and review

- **Boards:** Configured board packs are discovery seeds. GitHub code search finds additional public repositories. Every candidate must contain regular `board_info.yaml`, `board_devices.yaml`, and `board_peripherals.yaml`, with a nonempty chip in its information file. Candidate repositories are read at one pinned default-branch revision.
- Identical complete board-directory Git tree SHAs merge across sources. Explicit lineage rules also group verified upstream identities and chips while preserving origin evidence. These rules are maintained in the private source configuration; GitHub's `fork=false` flag alone does not establish an independent board model.
- The public board total is `boards.approved_count`: current known-component definitions plus approved additional models. The mechanically deduplicated count and all source evidence remain in the private scan report.
- **Projects:** Count public, non-fork, non-template repositories once by GitHub repository ID when their parsed `idf_component.yml` or `dependencies.lock` declares ESP Board Manager in its dependency map. Lock files can contain transitive dependencies. Multiple matching files in one repository still count once. Configured cache, vendor, and embedded upstream paths are excluded.
- Both counters describe observed public default-branch sources. Private repositories, unindexed code, deployments, and hardware validation are outside this measurement.

The private review ledger stores permanent identities using repository + complete board path + chip, or lineage family + complete relative vendor/model path + chip. Commit hashes and evidence URLs do not define identity. All retained origins participate in matching, so metadata or representative-source changes do not by themselves lose an existing decision.

- `include` approves one additional model. Add verified copies as exact selectors on that same record to count the model once.
- `alias` points to an existing known-component source with the same chip and adds no model.
- Unrecognized identities remain pending and do not increase the badge. Missing alias targets, chip changes, or new possible overlaps also require review.
- `distinct_from` can exempt explicitly reviewed false-positive known-source matches. New or unresolved hints still return to review.
- Disappeared identities become inactive instead of contributing phantom counts. Conflicting decisions fail collection.

The collector reads the ledger without modifying it. After reviewing pending candidates in a restricted report, update the local ledger and replace its Secure File while keeping the exact filename. The next run finds its new ID automatically. Keep local dated backups when changing either file, since Secure Files does not preserve Git history. The project counter does not need board-review approval.

## Local collection

Use Python 3.9 or newer, Git, and PyYAML:

```sh
python3 -m pip install 'PyYAML>=6,<7'
```

Place both private files in `.ecosystem-private/config/`, then provide a GitHub search token through `ECOSYSTEM_GITHUB_TOKEN`, `GITHUB_TOKEN`, or `GH_TOKEN`, in that order. Do not put tokens in source files or command arguments.

```sh
python3 tools/ci/collect_ecosystem_stats.py --output .ecosystem-private/report
```

To select another private configuration path, use `--sources /path/to/ecosystem_sources.json`. A complete run requires its review ledger. To download from Secure Files instead, set the CI environment variables described below and run the downloader with a new output directory:

```sh
python3 tools/ci/fetch_ecosystem_config.py --output .ecosystem-private/config
```

Preview options are `--boards-only` (GitHub board discovery without project collection) and `--boards-only --listed-boards` (configured roots only, without authenticated search). Previews must not be published.

The collector writes a **private** `stats.json`, two SVGs, and a detailed `README.md`. The JSON contains source identities, pinned revisions, hashes, evidence, filters, and review records. Use an ignored directory for all local outputs.

Code-search discovery splits broad queries into disjoint file-size ranges and verifies each final range a second time using repository, path, and blob SHA. Exact-size ranges that still exceed one page use strict pagination; an exact-size range above the 1,000-result API cap stops collection. Each query has at most three consistency attempts. Changing totals, duplicate or missing results, `incomplete_results=true`, or changed range contents discard the failed attempt and restart the affected query. Diagnostics identify the query and inconsistency without logging tokens or result inventories.

Each GET has at most three attempts in total, shared by transport failures, confirmed rate limits, and transient HTTP `500`, `502`, `503`, or `504` responses. Transport and server errors use 2- and 4-second backoff; server errors also honor a longer `Retry-After`, expressed as seconds or an HTTP date. Rate limits follow the server's timing headers or their own longer backoff. Rate-limit and server-error waits share a cumulative 300-second budget for the run. Every search request, including retries, observes the 6.1-second minimum interval and counts toward the 120-request budget. A recovered request retries the same page, then undergoes the normal consistency checks. Authentication/permission errors, other non-retryable HTTP errors, and malformed responses stop immediately. Exhausted attempts or budgets, collection errors, and conflicting review decisions stop the run and preserve the previous published result.

The first successful attempt supplies that query's result set; results from failed attempts are never mixed or selected by their largest `total_count`. The published board/project counts are calculated from the fetched, validated, deduplicated, and reviewed records, not copied from a search total. These checks validate the retrieved pages, but cannot establish exhaustive coverage of GitHub's search index. See [search scope and limits](https://docs.github.com/en/rest/search/search#about-search).

## GitLab setup

No additional repository is needed. In the existing GitLab project:

1. Open **Settings > CI/CD > Secure files** and upload the two local JSON files.
2. Keep their exact filenames as shown above. File IDs are resolved automatically.
3. In **Settings > CI/CD > Variables**, add or confirm the following variables:

| Variable | Type | Visibility | Value |
| --- | --- | --- | --- |
| `ECOSYSTEM_GITHUB_TOKEN` | Variable | Masked | GitHub token for authenticated public code search |
| `GH_PUSH_USER` | Variable | Visible | Existing GitHub publication username |
| `GH_PUSH_TOKEN` | Variable | Masked | Existing GitHub publication token with access to the target repository |

Use **All (default)** for the environment scope and disable variable-reference expansion. Protected variables are available only when the selected branch is protected; this also applies to manual runs before merging. The **File** variable type is not used for these settings; JSON files belong in **Secure files**.

Advanced setups can optionally pin both `ECOSYSTEM_SOURCES_FILE_ID` and `ECOSYSTEM_REVIEW_FILE_ID` as ordinary **Variable** values containing positive numeric IDs. Set both or neither. Explicit IDs still require the matching filenames and checksum verification; update the IDs after replacing files when using this override.

`CI_API_V4_URL`, `CI_PROJECT_ID`, and `CI_JOB_TOKEN` are supplied by GitLab. The job accesses Secure Files in its own project; the user running it must have Developer or higher access and permission to run on the selected branch. The schedule owner needs these permissions on `main`. JSON contents are larger than GitLab's usual CI variable value limit, so upload files instead of pasting their contents into variables.

The job keeps detailed scan artifacts for 30 days with `artifacts:access: developer`. Also set the project's **Settings > General > Visibility, project features, permissions > CI/CD** visibility to **Only project members**, and restrict inbound job-token access to trusted projects. Artifact access settings alone do not restrict all job-token access paths. See [artifact access](https://docs.gitlab.com/ci/yaml/#artifactsaccess).

To verify before merging, first commit and push these CI changes to a branch in this GitLab project. Open **Build > Pipelines > Run pipeline** (or **New pipeline**), choose that branch, and set `ECOSYSTEM_STATS=1`. After merging, the same manual steps work on `main`.

The manual job runs the complete pipeline: tests, configuration download, private snapshot collection, private history archival, and publication of only the public projection to `ecosystem-stats`. A successful run makes the root README badges available even before merging into `main`. This changes the public statistics branch; the source branch and `main` are not updated. Ensure the GitHub publication credentials are available to the selected branch. There is no remote publication before this setup and a successful job run. See [manual pipelines](https://docs.gitlab.com/ci/pipelines/#run-a-pipeline-manually).

For a daily schedule, use **Build > Pipeline schedules > New schedule**:

| Setting | Value |
| --- | --- |
| Target branch | `main` |
| Cron pattern | `0 8 * * *` |
| Timezone | `Asia/Shanghai` |
| Variable | `ECOSYSTEM_STATS=1` |

Manual (`web`) pipelines on any branch and scheduled pipelines on `main` enable the job only with `ECOSYSTEM_STATS=1`. Pushes, merge-request events, tag runs, and schedules on other branches do not enable statistics publication. Other CI jobs are skipped for this statistics pipeline. See [GitLab scheduled pipelines](https://docs.gitlab.com/ci/pipelines/schedules/).

## Public publication and README banners

The publisher validates the complete private snapshot before generating a separate public projection. It never copies the private JSON, detailed README, or original SVG contents to GitHub. The collector still produces the same four private input files; the publisher generates exactly eight public files on `ecosystem-stats`:

| Public file | Contents |
| --- | --- |
| `stats.json` | `schema_version`, `updated_at`, `supported_boards`, `public_dependent_projects` only |
| `supported-boards.svg` | Reviewed board count |
| `dependent-projects.svg` | Public dependent-project count |
| `README.md` | Counts, update time, and a short bilingual explanation of the counting rules |
| `ecosystem-banner-en-light.svg` | English logo and both counts, with two light statistics cards |
| `ecosystem-banner-en-dark.svg` | English logo and both counts, with two dark statistics cards |
| `ecosystem-banner-zh-light.svg` | Chinese labels and both counts, with two light statistics cards |
| `ecosystem-banner-zh-dark.svg` | Chinese labels and both counts, with two dark statistics cards |

Source lists, origins, review notes, aliases, pending candidates, filters, and evidence remain in the private report. The publisher refuses boards-only previews, inconsistent counts, unexpected remote files, and incomplete snapshots. It updates only the statistics branch without force-pushing or changing `main`.

The root READMEs use the matching language's banner and link to the aggregate statistics page. Each metric has a separate rounded card, with its label and right-aligned count on the same row. A `<picture>` element selects the dark or light statistics cards, with the light banner as the fallback. The logo, layout, and solid white background remain the same in both themes. See [GitHub's theme-aware image markup](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/quickstart-for-writing-on-github#adding-an-image-to-suit-your-visitors).

All four banners are generated from the same validated board and project counts using `templates/ecosystem-banner.svg`. The template contains only static artwork and labels; no reviewed inventory or evidence is embedded. Changing banner style or language does not change collection, deduplication, or review decisions. The original two badge URLs remain available for existing links.

The new banner URLs become available after a successful publication using this version of the code. For a preview before merging, commit and push the source branch, then run its statistics pipeline with `ECOSYSTEM_STATS=1`. Until that succeeds, remote README previews cannot display the new images. Merging the code alone does not create the schedule, upload Secure Files, or configure credentials.

## Private daily history

Open **Deploy > Package Registry** in this existing private GitLab project, select the latest version of **ecosystem-history**, and download **changes.md** to see that scan's update. A copy is also available in the current job's artifacts at `.ecosystem-private/report/history/changes.md`.

New versions contain three private files:

| File | Purpose |
| --- | --- |
| `changes.md` | The selected scan only: a full inventory for the first baseline, then only added/removed names and links, or an explicit no-change status |
| `README.md` | Daily closing counts and the cumulative per-scan change timeline, without appending the latest full inventory |
| `history.json` | Machine-readable comparison state: the latest full inventory and all change events; unchanged schema version 1 |

The readable reports contain:

- A daily table in **Asia/Shanghai (UTC+8)**, with the day's final observed board/project counts and changes from the previous recorded day.
- Every individual scan, including zero-change scans and multiple manual runs on the same day.
- Added and removed counted boards and projects, with their source links. Unchanged net totals can still have additions and removals.
- For histories started with this version, the first baseline's `changes.md` contains the complete counted inventory and its links. Later versions' `changes.md` omit unchanged inventory and show only that run's additions/removals. A zero net change can still show both additions and removals. For existing histories, the original baseline inventory remains in the original version's `README.md`.

Use `changes.md` for "what is new in this run", and `README.md` for daily totals and the full change timeline. `history.json` always retains the current full inventory so the next scan can compare stable identities; its size does not represent the number of new boards/projects. Dates and times in the current-update report use Asia/Shanghai (UTC+8).

"Added" and "removed" describe differences between validated scan results; they do not establish repository creation/deletion dates or hardware release dates. Only known-component boards and approved outside models enter the supported-board history. Aliases and pending candidates remain in the detailed scan report. A review decision can therefore change the counted inventory without changing the source repositories.

The first run creates an explicit **baseline**; it does not call every existing board/project newly added. It cannot reconstruct earlier additions from the public aggregate `stats.json`. Missing calendar days are not invented: the next successful scan compares against the previous archived scan. Past private artifacts would need a separate, ordered import to backfill dates.

Project identity uses GitHub repository ID, so renaming a repository does not make it newly added. Board identity uses permanent review identities, full source paths and chips, and verified lineage identities; commit hashes and evidence URLs are not identities. Metadata and source revisions can refresh without generating false additions. Inconsistent counts, ambiguous identity matches, corrupt history, or out-of-order scans stop archival instead of silently resetting the baseline.

### Storage and pipeline order

`tools/ci/archive_ecosystem_history.py` reads the previous archive, compares the complete scan, and writes `.ecosystem-private/report/history/{history.json,README.md,changes.md}`. It uploads `README.md` and `changes.md` first and the canonical cumulative `history.json` last to a new Generic Package version `1.0.<CI_JOB_ID>`. Each version's machine state contains all changes so far and the latest compact inventory, while `changes.md` contains only that job's update. Completed versions are immutable; a retry with identical contents is safe, and conflicting contents are rejected.

Existing schema-1 archives containing only `history.json` and `README.md` remain valid comparison baselines. The next job continues their history and creates the new three-file version automatically. It does not modify older package versions or add `changes.md` to already completed versions. No Secure File changes or extra variables are needed. To inspect the original complete baseline inventory, open its original package version; a later ledger cannot recover every original name/link after metadata changes.

The job uses the predefined `CI_JOB_TOKEN` and project/job/branch metadata. No new token, variable, repository, branch, or MR is required. Keep this GitLab project **private**, Package Registry enabled, and package access restricted to project members; do not enable anonymous pulls or a cleanup policy that deletes these history packages. Unlike the 30-day diagnostic artifacts, package versions have no expiry configured by this workflow. Project administrators can still delete them, so include them in normal project backups. Package access follows GitLab's package permissions, not the artifacts' Developer-only setting. See [Generic Packages](https://docs.gitlab.com/user/packages/generic_packages/) and [job-token permissions](https://docs.gitlab.com/ci/jobs/ci_job_token/).

The existing `resource_group: ecosystem-stats` serializes archive reads and writes. `main` uses the production package name `ecosystem-history`. Other branches use `ecosystem-history-test-<branch-hash>` so pre-merge manual tests cannot become the production comparison baseline. Choose the corresponding test package to inspect those reports.

Archival happens **after complete collection and validation, before GitHub publication**. An archive failure fails the job and prevents publication. If subsequent GitHub publication fails, the valid scan remains in private history; history therefore tracks validated scans, not a guarantee that every scan was publicly published. Authentication and storage errors do not fall back to an empty history. A package without its final `history.json` is treated as an incomplete upload; invalid canonical JSON fails the run.
