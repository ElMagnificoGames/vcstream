# Agent Instructions

## Additional Context

- `docs/BUILD.md` provides build and run instructions.
- `docs/CODE-QT6.md` provides all coding conventions and a great deal of the AI-operation policies for working with this codebase.  This is **MANDATORY** reading.
- `docs/DD-TEMPLATE.txt` provides the starting template for module DD files (`*-dd.txt`).
- `docs/HISTORY.md` provides a description of what work has been completed so far.
- `docs/MODULES.md` defines the initial module boundaries and dependency direction.
- `docs/OVERVIEW.md` provides onboarding information to understand this project and its goals.
- `docs/ROADMAP.md` provides a detailed account of what is to be done (broken down into manageable tasks).
- `docs/STYLE.md` defines the app-owned visual design language and theme-system expectations for authored UI.
- `docs/SETTINGS.md` documents settings persistence and storage location.

## Tooling

This repository uses CMake.

Unit tests use QtTest and live under `apps/unittests/`. Shared unit test helper utilities live under `apps/unittests/helpers/`.

QML UI smoke tests live under `apps/unittests/qml/` and MUST fail on any Qt/QML warning.

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

## Documentation

### DD files

When adding or editing any module DD file (`*-dd.txt`), follow this checklist.

- Description MUST use Smalltalk class-comment voice:
  - start with "I am ..."
  - include "I collaborate with ..."
  - include explicit non-goals as "I do not ..."
  - include a "State model" section (thread affinity, lifetime/ownership expectations, and re-entrancy where relevant)
- Use the DD template in `docs/DD-TEMPLATE.txt` as the starting point.
- Keep the Public interface section contract-like (arguments, ownership, lifetime, thread safety, errors).

## UX and UI

### Copy

User-facing UI labels and control names should describe user intent in simple, non-technical language.

- Prefer human wording over implementation terms (for example: "Join room" rather than "Connect to relay").
- Use hover/tooltip text to provide optional power-user detail, still written in plain language.
- If the roadmap uses technical wording for a capability, treat it as implementation vocabulary and re-evaluate the user-facing copy when implementing the UI.
- This isn't limited to QML, user-facing copy can include things like logged error messages in C++, etc.

### Colours and themes

This application now has an app-owned styling direction documented in `docs/STYLE.md`. Future versions may still allow the user to choose a Qt theme/style, but authored UI should not rely on the desktop style to provide the application's visual identity.

- Do not hardcode light/dark UI colours for surfaces, text, or borders outside the shared token system (for example: `#ffffff`, `#000000`).
- Prefer shared app theme tokens and wrapper components for authored controls; use the active palette/system palette as an input/reference layer rather than the sole styling mechanism.
- End-user customisation must remain token-driven and coherent (for example: mode, accent, density, motion, and future style variants), not ad-hoc per-widget colour tweaking.
- Do not use `opacity` on containers that contain text/controls (it dims children and reduces readability); prefer alpha in the background `color` instead.

### Interaction stability (avoid oscillation)

Be careful with UI elements that appear/disappear on hover or focus.

- Hover help (tooltips, help text, popups) MUST NOT participate in layout sizing of the hovered control or its parents.
- Render hover help in an overlay layer so showing/hiding it cannot move surrounding elements.
- Watch for hover oscillation loops (cursor hovers -> UI appears -> geometry shifts -> hover ends -> geometry shifts back -> hover starts).
- If you add or change hover behaviour, extend `apps/unittests/qml/tst_qml_ui.cpp` to reproduce it (hover/move/click-outside) and ensure it emits no warnings.

### QML warning hygiene

QML warnings are treated as test failures.

- If you change QML behaviour, you MUST update `apps/unittests/qml/tst_qml_ui.cpp` to navigate to the new/changed UI state and interact with it (click/hover/type as appropriate).
- Add stable `objectName` values in QML for any controls that tests need to locate.
- Prefer testing real user flows (start screen -> join/host -> disconnect) rather than directly poking internal state.
- Avoid deprecated implicit signal parameter injection in QML handlers. Prefer explicit function parameters (for example: `onPressed: function( mouse ) { ... }`).
- Ensure the warning gate actually intercepts warnings: capture both `QQmlApplicationEngine::warnings` and Qt message output, and install message handlers within the QtTest lifecycle (for example: `initTestCase` / `cleanupTestCase`).

Smoke test policy (project-specific):
- `apps/unittests/qml/tst_qml_ui.cpp` performs an automatic interaction sweep.
- Any interactive control authored under `qrc:/qml/*` MUST have a stable `objectName` so the sweep can report and reproduce failures.
- The sweep hovers all interactive controls and clicks most controls.
- For controls that are unsafe or non-deterministic to auto-click in CI (engine reload, external links, process exit, etc.), set `property bool testSkipActivate: true` on the control; the sweep will still hover it.

## British spelling

- Use British spelling, for example: "centre", "colour", "behaviour", "organisation", "optimise".  This includes, but is not limited to, in source code, documentation, and user-facing UI copy.
- Do not change established external names/identifiers (Qt types/APIs, class names, file names, protocol fields, third-party terminology) or quotations.
