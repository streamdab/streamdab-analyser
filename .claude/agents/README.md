# Custom Agents for StreamDAB Analyser

## Available Agents

### pm-agent.md
**Project Manager Agent** - Coordinates multi-agent development work

**Usage:**
```
User: "pm-agent: start Phase 0 now"
User: "pm-agent: deploy parallel agents for Day 1"
User: "pm-agent: status update"
```

**Key Features:**
- Deploys multiple cpp-qt-developer agents in parallel
- Coordinates dependencies between agents
- Tracks progress with todo lists
- Ensures code-reviewer runs before phase completion
- Manages project timeline and quality gates

**Primary Agents Used:**
- **cpp-qt-developer** (4-5 concurrent) - C++ Qt6 implementation
- **code-reviewer** (1, mandatory) - Code quality and security review
- **test-automator** (3-4 concurrent) - Test suite creation
- **devops-engineer** (2-3 concurrent) - Build and CI/CD
- **python-pro** (3-4 concurrent) - Scripts and automation
- **Explore** (5-6 concurrent) - Codebase analysis

## How to Add More Agents

Create a new `.md` file in this directory:

```markdown
# agent-name

Description of agent purpose...

## Capabilities
- List of what this agent can do

## Usage
When to use this agent...
```

## Current Project Phase

**Phase 0:** Critical Fixes (3-4 days)
- 7 critical issues to fix
- Parallel cpp-qt-developer deployment
- Mandatory code-reviewer at end
