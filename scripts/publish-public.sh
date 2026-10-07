#!/usr/bin/env bash
# ============================================================================
# publish-public.sh — publish a curated tree of this repo to the public remote
#
# The private repo keeps everything (docs/, ref/, agent/IDE folders). This
# script produces a public-safe copy by removing the not-for-public paths from
# a CLONE (never from the live repo) and pushing it to the public remote.
#
# See docs/GITHUB_PUBLISHING_PLAN.md for the strategy and safeguards.
#
# Usage:
#   scripts/publish-public.sh --remote git@github.com:<org>/<repo>.git [options]
#
# Options:
#   --remote URL|NAME   required: the public remote URL (or an existing remote name)
#   --branch NAME       branch to publish (default: current branch)
#   --single-commit     publish a squash of the current tree instead of the
#                       filtered history (maximum safety, no history)
#   --dry-run           do everything except the final push; prints the tree diff
#   --yes               skip the confirmation prompt
#   -h|--help           this help
#
# Requirements: git; for the default route: `git-filter-repo`
# (`pipx install git-filter-repo`, `pip install --user git-filter-repo`, or
# drop the single script into ~/.local/bin).
# ============================================================================

set -euo pipefail

REMOTE=""
BRANCH=""
MODE="filter"
DRY_RUN=0
ASSUME_YES=0

usage() { sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
    case "$1" in
        --remote) REMOTE="${2:-}"; shift 2 ;;
        --branch) BRANCH="${2:-}"; shift 2 ;;
        --single-commit) MODE="single"; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        --yes) ASSUME_YES=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

[ -n "$REMOTE" ] || { echo "error: --remote is required" >&2; exit 2; }

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
[ -z "$BRANCH" ] && BRANCH="$(git rev-parse --abbrev-ref HEAD)"

# Paths excluded from the public tree (keep in sync with .gitattributes).
# NOTE: eti/bkk_20062022_141637.eti is PUBLIC (the CI parity gate + smoke
# depend on it); the other eti/ subdirectories stay private (gitignored).
EXCLUDES=(
    docs
    ref
    build
    Testing
    tests/tests
    .claude
    .opencode
    .vscode
    .playwright-mcp
    .ropeproject
    AGENTS.md
    CLAUDE.md
    .agents-config.md
    output.yaml
)
# Paths that stay in the public tree (informational).
KEEPS=(src tests include cmake docker packaging resources scripts \
       CMakeLists.txt CMakePresets.json .github .gitignore .gitattributes \
       README.md LICENSE INSTALL.md)

echo "== StreamDAB-Analyser public publish =="
echo "  source repo : $ROOT_DIR (branch $BRANCH)"
echo "  public remote: $REMOTE"
echo "  mode        : $MODE"
echo "  exclude     : ${EXCLUDES[*]}"
echo "  keep        : ${KEEPS[*]}"
echo

if [ "$ASSUME_YES" -ne 1 ] && [ "$DRY_RUN" -ne 1 ]; then
    read -r -p "Publish '$BRANCH' to $REMOTE ? [y/N] " reply
    case "$reply" in [yY]*) ;; *) echo "aborted"; exit 1 ;; esac
fi

WORK_DIR="$(mktemp -d /tmp/opencode/publish-public.XXXXXX)"
trap 'rm -rf "$WORK_DIR"' EXIT
echo "-> working copy: $WORK_DIR"

# 1) Clone the current repo (all history) into a scratch dir.
git clone --quiet --no-local --branch "$BRANCH" "$ROOT_DIR" "$WORK_DIR/repo"
cd "$WORK_DIR/repo"

if [ "$MODE" = "filter" ]; then
    if ! command -v git-filter-repo >/dev/null 2>&1; then
        echo "error: git-filter-repo not found (pipx install git-filter-repo); use --single-commit otherwise" >&2
        exit 1
    fi
    args=(--force --invert-paths)
    for p in "${EXCLUDES[@]}"; do args+=(--path "$p"); done
    git filter-repo "${args[@]}"
else
    # single-commit: build a clean tree from HEAD, fresh history.
    git checkout --quiet --orphan public
    git add -A
    for p in "${EXCLUDES[@]}"; do git rm -r --quiet --ignore-unmatch --cached "$p" >/dev/null 2>&1 || true; done
    git commit --quiet -m "StreamDAB-Analyser: public release snapshot"
fi

# 2) Pre-flight: nothing that must stay private may remain tracked.
echo "-> verifying the filtered tree"
leaks=0
for p in "${EXCLUDES[@]}"; do
    if git ls-files --error-unmatch "$p" >/dev/null 2>&1 || [ -e "$p" ]; then
        # allow files that legitimately remain tracked under a partially kept dir
        if [ -e "$p" ]; then echo "   !! still present: $p"; leaks=1; fi
    fi
done
[ "$leaks" -eq 0 ] || { echo "error: excluded paths are still present" >&2; exit 1; }

# 3) Secret-ish scan over the tracked files (advisory, fails the run on hits).
if git grep -nIE '(api[_-]?key|secret|password|BEGIN [A-Z ]*PRIVATE KEY)' -- . 2>/dev/null | head -20; then
    echo "error: potential secrets found (above) — review before publishing" >&2
    exit 1
fi

echo "-> files to publish: $(git ls-files | wc -l)"

if [ "$DRY_RUN" -eq 1 ]; then
    echo "dry-run: skipping push. Public tree summary:"
    git ls-files | sed -n '1,40p'
    echo "..."
    exit 0
fi

# 4) Push.
git remote add public "$REMOTE" 2>/dev/null || git remote set-url public "$REMOTE"
git push public "HEAD:${BRANCH}"
echo "== done: published to $REMOTE ($BRANCH) =="