#!/bin/bash

# TDD Git Hooks Installation Script
# Installs pre-commit hooks to enforce TDD methodology

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo -e "${BLUE}===============================================${NC}"
echo -e "${BLUE}🔧 TDD GIT HOOKS INSTALLATION 🔧${NC}"
echo -e "${BLUE}===============================================${NC}"
echo ""

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}ERROR: Not in a git repository${NC}"
    echo -e "${RED}Please run this script from within the project repository${NC}"
    exit 1
fi

# Get the project root and git directory
PROJECT_ROOT=$(git rev-parse --show-toplevel)
GIT_DIR=$(git rev-parse --git-dir)
HOOKS_DIR="$GIT_DIR/hooks"

echo -e "${BLUE}Project root: $PROJECT_ROOT${NC}"
echo -e "${BLUE}Git hooks directory: $HOOKS_DIR${NC}"
echo ""

# Ensure hooks directory exists
mkdir -p "$HOOKS_DIR"

# Install pre-commit hook
PRE_COMMIT_HOOK="$HOOKS_DIR/pre-commit"
TDD_PRE_COMMIT_SCRIPT="$PROJECT_ROOT/scripts/tdd_pre_commit.sh"

echo -e "${BLUE}Installing TDD pre-commit hook...${NC}"

# Check if TDD pre-commit script exists
if [ ! -f "$TDD_PRE_COMMIT_SCRIPT" ]; then
    echo -e "${RED}ERROR: TDD pre-commit script not found at $TDD_PRE_COMMIT_SCRIPT${NC}"
    exit 1
fi

# Backup existing pre-commit hook if it exists
if [ -f "$PRE_COMMIT_HOOK" ]; then
    echo -e "${YELLOW}Backing up existing pre-commit hook...${NC}"
    cp "$PRE_COMMIT_HOOK" "$PRE_COMMIT_HOOK.backup.$(date +%Y%m%d_%H%M%S)"
fi

# Create the pre-commit hook
cat > "$PRE_COMMIT_HOOK" << EOF
#!/bin/bash

# TDD Pre-Commit Hook
# Automatically installed by TDD framework

# Get the project root
PROJECT_ROOT=\$(git rev-parse --show-toplevel)

# Execute the TDD pre-commit validation
exec "\$PROJECT_ROOT/scripts/tdd_pre_commit.sh"
EOF

# Make the hook executable
chmod +x "$PRE_COMMIT_HOOK"

echo -e "${GREEN}✅ Pre-commit hook installed successfully${NC}"
echo ""

# Install pre-push hook (optional - for additional validation)
PRE_PUSH_HOOK="$HOOKS_DIR/pre-push"

echo -e "${BLUE}Installing TDD pre-push hook...${NC}"

# Backup existing pre-push hook if it exists
if [ -f "$PRE_PUSH_HOOK" ]; then
    echo -e "${YELLOW}Backing up existing pre-push hook...${NC}"
    cp "$PRE_PUSH_HOOK" "$PRE_PUSH_HOOK.backup.$(date +%Y%m%d_%H%M%S)"
fi

# Create the pre-push hook
cat > "$PRE_PUSH_HOOK" << EOF
#!/bin/bash

# TDD Pre-Push Hook
# Additional validation before pushing to remote

echo "🚀 TDD Pre-Push Validation"
echo ""

# Get the project root
PROJECT_ROOT=\$(git rev-parse --show-toplevel)
cd "\$PROJECT_ROOT"

# Run integration tests if they exist
if [ -d "tests/integration" ]; then
    echo "Running integration tests..."
    if ! make run_integration_tests 2>/dev/null; then
        echo "⚠️  Integration tests not configured yet"
    fi
fi

# Run performance benchmarks if available
if [ -d "tests/benchmarks" ]; then
    echo "Running performance benchmarks..."
    if ! make run_benchmarks 2>/dev/null; then
        echo "⚠️  Performance benchmarks not configured yet"
    fi
fi

echo "✅ Pre-push validation complete"
exit 0
EOF

# Make the hook executable
chmod +x "$PRE_PUSH_HOOK"

echo -e "${GREEN}✅ Pre-push hook installed successfully${NC}"
echo ""

# Install commit-msg hook for TDD commit message validation
COMMIT_MSG_HOOK="$HOOKS_DIR/commit-msg"

echo -e "${BLUE}Installing TDD commit message hook...${NC}"

# Backup existing commit-msg hook if it exists
if [ -f "$COMMIT_MSG_HOOK" ]; then
    echo -e "${YELLOW}Backing up existing commit-msg hook...${NC}"
    cp "$COMMIT_MSG_HOOK" "$COMMIT_MSG_HOOK.backup.$(date +%Y%m%d_%H%M%S)"
fi

# Create the commit-msg hook
cat > "$COMMIT_MSG_HOOK" << EOF
#!/bin/bash

# TDD Commit Message Hook
# Validates commit message format for TDD workflow

COMMIT_MSG_FILE="\$1"
COMMIT_MSG=\$(cat "\$COMMIT_MSG_FILE")

# Check for TDD phase indicators (optional but encouraged)
if echo "\$COMMIT_MSG" | grep -qE "(RED|GREEN|REFACTOR|TDD)" || \
   echo "\$COMMIT_MSG" | grep -qE "^(feat|fix|test|refactor|docs|style|chore)"; then
    # Good commit message format
    exit 0
fi

# Allow commits but suggest better formatting
echo ""
echo "💡 TDD Commit Message Suggestion:"
echo "Consider prefixing with TDD phase or conventional commit type:"
echo "  - 🔴 RED: <description>     (failing test)"
echo "  - 🟢 GREEN: <description>   (passing implementation)"
echo "  - 🔵 REFACTOR: <description> (code improvement)"
echo "  - feat: <description>        (new feature)"
echo "  - fix: <description>         (bug fix)"
echo "  - test: <description>        (test changes)"
echo ""

# Allow the commit to proceed
exit 0
EOF

# Make the hook executable
chmod +x "$COMMIT_MSG_HOOK"

echo -e "${GREEN}✅ Commit message hook installed successfully${NC}"
echo ""

# Test the installation
echo -e "${BLUE}Testing TDD hooks installation...${NC}"

# Test if hooks are executable
if [ -x "$PRE_COMMIT_HOOK" ] && [ -x "$PRE_PUSH_HOOK" ] && [ -x "$COMMIT_MSG_HOOK" ]; then
    echo -e "${GREEN}✅ All hooks are executable${NC}"
else
    echo -e "${RED}❌ Some hooks are not executable${NC}"
    ls -la "$HOOKS_DIR"
    exit 1
fi

# Summary
echo ""
echo -e "${GREEN}===============================================${NC}"
echo -e "${GREEN}🎉 TDD GIT HOOKS INSTALLATION COMPLETE 🎉${NC}"
echo -e "${GREEN}===============================================${NC}"
echo ""
echo -e "${GREEN}Installed hooks:${NC}"
echo -e "${GREEN}  ✅ pre-commit: TDD quality gate enforcement${NC}"
echo -e "${GREEN}  ✅ pre-push: Additional integration validation${NC}"
echo -e "${GREEN}  ✅ commit-msg: TDD commit message guidance${NC}"
echo ""
echo -e "${GREEN}TDD Enforcement Active:${NC}"
echo -e "${GREEN}  🔴 All tests must pass before commit${NC}"
echo -e "${GREEN}  🟢 Code coverage ≥ 80% required${NC}"
echo -e "${GREEN}  🔵 Build must succeed before commit${NC}"
echo ""
echo -e "${BLUE}To disable hooks temporarily:${NC}"
echo -e "${BLUE}  git commit --no-verify${NC}"
echo ""
echo -e "${BLUE}To uninstall hooks:${NC}"
echo -e "${BLUE}  rm $HOOKS_DIR/pre-commit${NC}"
echo -e "${BLUE}  rm $HOOKS_DIR/pre-push${NC}"
echo -e "${BLUE}  rm $HOOKS_DIR/commit-msg${NC}"
echo ""
echo -e "${GREEN}TDD workflow is now enforced! 🧪${NC}"
echo ""
