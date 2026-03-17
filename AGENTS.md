# Claude Code Instructions

## Additional Context

- `docs/BUILD.md` provides build and run instructions.
- `docs/CODE-QT6.md` provides all coding conventions and a great deal of the AI-operation policies for working with this codebase.  This is **MANDATORY** reading.
- `docs/HISTORY.md` provides a description of what work has been completed so far.
- `docs/MODULES.md` defines the initial module boundaries and dependency direction.
- `docs/OVERVIEW.md` provides onboarding information to understand this project and its goals.
- `docs/ROADMAP.md` provides a detailed account of what is to be done (broken down into manageable tasks).

## Tooling

This repository uses CMake.

Canonical build and run instructions live in `docs/BUILD.md`.

Common commands (from the repository root):

```sh
cmake -S . -B build
cmake --build build
./build/bin/vcstream
```

### Git Policy

See `docs/CODE-QT6.md`.

Whenever a version number is incremented:

- Create an annotated git tag in the form `vX.Y.Z` that points at the version-bumped commit.
  - Example: `git tag -a v0.0.1 -m "v0.0.1"`.

Commit message note:

- `git commit -m` may be provided multiple times; each `-m` adds a new paragraph, and git will insert a blank line between paragraphs.
- Do not introduce unnecessary blank lines in commit messages (notably in lists).
- Prefer `git commit` (editor) or provide the full message via stdin/file (e.g. `git commit -F -`).

## Task Workflow

**Finding work:**

Check `docs/ROADMAP.md` against `docs/HISTORY.md`.  Never use TodoWrite for tracking work.

**Rules:**

- One task at a time
- Wait for user to tell you which task to work on
- Never start tasks proactively or switch without user direction

**Scoping work:**

When requirements are unclear or there are design decisions to make, clarify **before** implementing:

- Use `AskUserQuestion` with concrete options — never list numbered questions as plain text
- Ask early (before writing code), not after discovering ambiguity mid-implementation
- Keep it focused: 1–4 targeted questions with meaningful choices
- Use `multiSelect` when choices aren't mutually exclusive

Once scope is understood, capture it in `HISTORY.md`.  **Write thorough descriptions.**  Descriptions should include:

- **What** needs to be done (specific requirements, not vague goals)
- **Why** it's needed (business context, user impact, motivation)
- **Acceptance criteria** (concrete conditions for "done")
- **Decisions made** during scoping (options considered, rationale for the chosen approach)
- **Technical notes** (relevant files, APIs, edge cases, dependencies on other tasks)
- Enough context that another session — or another developer — can pick up the work cold

**Completing work:**

Follow the instructions in `docs/CODE-QT6.md`.  Then update `docs/HISTORY.md` with a thorough description of work done.  Include enough context that another session — or another developer — can pick up the work cold from just checking `docs/ROADMAP.md` against `docs/HISTORY.md`.

## Versioning

When completing a task, increment the project version in `CMakeLists.txt` using the following semver:

- **Patch** (x.y.Z): Bug fixes, small changes
- **Minor** (x.Y.0): New features, backward-compatible changes
- **Major** (X.0.0): Breaking changes

The version is project-wide; all executables in this repository share the same version number unless explicitly changed by a documented decision.

Versioning should be prospective:  if a a given task begins the process of implementing a new feature, that warrants a minor number increment, even if taken on its own the patch barely does anything.

Increment the version **once per task**, not once per individual change.  If the version has already been bumped for the current task, do not bump it again for subsequent changes within the same task.
