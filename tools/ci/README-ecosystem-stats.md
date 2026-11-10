# Ecosystem statistics

The GitLab job discovers public ESP Board Manager board definitions and dependent projects, applies maintained review decisions, and publishes aggregate statistics and README banners to the GitHub `ecosystem-stats` branch. Source rules, review records, and detailed results stay outside the public repository.

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

Each code-search query has at most three attempts. Changing totals, duplicate results, missing results, or `incomplete_results=true` discard the entire attempt and restart that query from page 1, with the existing request rate limit still applied. Diagnostics identify the query, attempt, page, and inconsistent counts without logging tokens or result inventories. Authentication/HTTP errors, malformed responses, and the 1,000-result search cap stop immediately. If no attempt passes all checks, collection errors occur, or review decisions conflict, the run stops and preserves the previous published result.

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

The manual job runs the complete pipeline: tests, configuration download, private snapshot collection, and publication of only the public projection to `ecosystem-stats`. A successful run makes the root README badges available even before merging into `main`. This changes the public statistics branch; the source branch and `main` are not updated. Ensure the GitHub publication credentials are available to the selected branch. There is no remote publication before this setup and a successful job run. See [manual pipelines](https://docs.gitlab.com/ci/pipelines/#run-a-pipeline-manually).

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
