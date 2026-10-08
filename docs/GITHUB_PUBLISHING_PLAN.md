# GitHub Publishing Plan — Open-Source Release from a Private Repo

**Date:** 2026-10-07 · **Status:** plan (nothing published yet)
**Goal:** publish StreamDAB-Analyser as an **open-source project** on GitHub (with GitHub Actions building/releasing), while keeping **internal documentation and AI/IDE tooling out of the public repo**.

---

## 1. Decision summary (recommended path)

| Question | Recommendation |
|---|---|
| How to exclude `docs/`, agent/AI/IDE folders? | **Rewrite a *clone* with `git filter-repo`** (removes the paths from *every* commit) → push to the public repo. Safer than a live-repo filter (the private repo is untouched) and keeps development history. |
| If history is not wanted / needs maximum safety | Alternative: **single-commit export** (`git archive` of the allowed paths → fresh repo → one commit). No private content can exist in any public commit; history is lost. |
| Where do the private things live? | The **private remote stays the origin** with everything (`docs/`, `.claude`, `.opencode`, `.vscode`, `ref/` PDFs, **all of `eti/`**). The public remote receives only the curated tree. |
| Day-to-day flow | Develop & push to **private origin** as today; publish with `scripts/publish-public.sh` (new) which clones → filters → pushes to the **public** remote. Run it per release (or per milestone). |

> **2026 decision (revised) — ALL `eti/` captures are private.** They are
> World DAB licensed test material and must never be published — including the
> 30 MB Bangkok fixture. The test suite is therefore split: PUBLIC tests run
> in CI, PRIVATE (capture-dependent) tests run only where the files exist
> locally (`docs/TEST_SUITE_PUBLIC_PRIVATE_SPLIT.md`).

## 2. Path classification

### Publish (public)
`src/`, `tests/`, `include/`, `cmake/` (**dead-config decision resolved**: the stale `*Config.cmake` files were deleted, `packaging.cmake` rewritten + wired), `docker/`, `packaging/` (rebranded + wired; AppImage/macOS/Windows scripts kept),
`resources/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/` (CI workflows), `.gitignore`, `.gitattributes`,
`README.md`, `LICENSE`, `INSTALL.md`, `scripts/` (only the build/CI helpers that stay after the cleanup).

### Do NOT publish
| Path | Why |
|---|---|
| `docs/` | Internal notes, plans, audits, session logs, task lists, archived reports (per user decision) |
| `.claude/`, `.opencode/`, `.vscode/`, `.agents-config.md`, `CLAUDE.md`, `AGENTS.md` | Agent/AI/IDE configuration |
| `ref/` | **Third-party PDFs** (ETSI EN 300 401, DAB-XPlorer manuals) — copyright; must not be redistributed |
| `eti/` | **All captures — World DAB licensed test material, never published** (tracked ones like `bkk_…eti` serve the PRIVATE suite only) |
| `build*/`, `Testing/`, `tests/tests/`, `*.o`, `output.yaml`, `*.backup*` | Build/test artifacts |
| `.ropeproject/`, `.playwright-mcp/` | Editor/tool caches |

> **Note on `AGENTS.md`**: it is agent tooling config (→ *do not publish*), but the local agent workflow requires it at the repo root. It stays at the root of the **private** repo and is excluded from the public tree. If a public "contributor guide" is wanted, write a separate `CONTRIBUTING.md` (new, public) instead of publishing `AGENTS.md`.

## 3. Publication procedure (filter-repo route)

```bash
# One-time: install the tool
pipx install git-filter-repo            # or pip install git-filter-repo

# 1) Work on a CLONE — never filter the live repo
git clone --no-local /path/to/streamdab-analyser ../streamdab-public
cd ../streamdab-public

# 2) Remove everything that must not be public (from ALL commits)
git filter-repo --force \
  --invert-paths \
  --path docs \
  --path ref \
  --path .claude \
  --path .opencode \
  --path .vscode \
  --path .playwright-mcp \
  --path .ropeproject \
  --path .agents-config.md \
  --path CLAUDE.md \
  --path AGENTS.md \
  --path eti \
  --path build \
  --path Testing \
  --path tests/tests \
  --path output.yaml

# 3) Add the public remote and push
git remote add public git@github.com:sekz/streamdab-analyser.git
git push public main --tags
```

**Single-commit alternative (maximum safety):**
```bash
# Build a clean tree from the current commit, then publish one commit
git archive HEAD -- src tests include cmake docker packaging resources \
  CMakeLists.txt CMakePresets.json .github .gitignore .gitattributes \
  README.md LICENSE INSTALL.md scripts \
  | tar -x -C ../streamdab-public-tree
cd ../streamdab-public-tree && git init && git add -A \
  && git commit -m "StreamDAB-Analyser: first public release"
```

## 4. Safeguards before the first push

1. **Secret scan**: `grep -rInE "(api[_-]?key|token|password|secret|BEGIN .*PRIVATE KEY)" --exclude-dir=.git .` over the *filtered* tree; add a `gitleaks`/`trufflehog` run in CI.
2. **Licence check**: `LICENSE` is the project licence (verify: GPL-3.0) and **third-party compliance** — Qt-ADS (LGPL-2.1), Qt (LGPL/GPL), libfaad (GPL-2.0 — verify redistribution implications for a **public** binary!), libzmq (MPL-2.0), FFTW (GPL-2.0). `docs/LICENSE_COMPLIANCE_REPORT.md` (private) has the audit; the **public** repo must ship a correct `LICENSE` + `THIRD_PARTY_NOTICES.md`.
3. **`.gitattributes`** with `export-ignore` for every non-public path (protects `git archive` users from accidentally exporting them):
   ```
   docs/ export-ignore
   ref/ export-ignore
   .claude/ export-ignore
   .opencode/ export-ignore
   .vscode/ export-ignore
   AGENTS.md export-ignore
   CLAUDE.md export-ignore
   .agents-config.md export-ignore
   ```
4. **`.gitignore`** for the public repo: keep the build/artifact ignores (`build*/`, `Testing/`, `tests/tests/`, `*.o`, `output.yaml`, `*.backup*`, `.ropeproject/`, `.playwright-mcp/`) and `eti/` in full.
5. **Fixtures — private**: every `eti/` capture is World DAB licensed material and stays out of the public tree (`--path eti` in the filter; `eti/ export-ignore`). The PRIVATE test suite runs on machines that have the files (cloning the private origin provides the tracked `bkk_…eti` + description files; the larger sets are obtained separately). GitHub CI runs only `ctest -L PUBLIC`.

## 5. CI / release (GitHub Actions — to build next)

Planned workflow set (see the cross-platform plan for the build matrix):
1. **`ci.yml`** — matrix `ubuntu-24.04`, `windows-2022`, `macos-14`: install deps per platform (apt / vcpkg / brew), configure with the OS preset, build with **bounded** parallelism, run the offscreen suites (fixture-dependent suites skipped without the asset). This directly answers "does it build on Windows/macOS?" for every push.
2. **`release.yml`** — on tag `v*`: build Release on all three OSes, produce artifacts (**CPack now exists**: Linux `.tar.gz`/`.deb` via CPack + AppImage via `packaging/linux/create_appimage.sh`, Windows `.zip`/NSIS `.exe` installer via CPack, macOS `.app`/`.dmg` via `scripts/bundle-macos.sh`), attach them to the GitHub Release, and generate release notes from the changelog. Remaining: macOS codesigning/notarization decision for a signed `.app`.
3. **`codeql.yml`** (optional) — C++ static analysis on the public repo.
4. **Secret scanning** — enable GitHub's push protection; optionally `gitleaks` job.

## 6. Sequencing

1. **Cleanup pass (done)**: delete junk + move scripts → `scripts/`; **dead `cmake/*Config.cmake` deleted, `cmake/packaging.cmake` rewritten + wired with `install()` rules (B8 resolved)**.
2. **Docs reorg**: root keeps `README.md`, `LICENSE`, `INSTALL.md` (+ `AGENTS.md` private-only, excluded above); `CLAUDE.md` → `docs/archive/`.
3. **`.gitattributes`** (export-ignore list) + `.gitignore` updates.
4. **`scripts/publish-public.sh`** — implements §3 (filter-repo on a temp clone) with a `--single-commit` alternative flag, a pre-flight secret/licence check, and a confirmation prompt.
5. **CI matrix** (`ci.yml`) + **release pipeline** (`release.yml`) — after the cross-platform build work lands (macOS/presets/audio backend).
6. First publication = the filtered clone pushed to the public remote; thereafter re-run §3 per release.