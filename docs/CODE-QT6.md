# Qt 6 / C++17 Coding Conventions

This document is the authoritative set of coding conventions for a native Qt 6 / C++17 application in this codebase.
It targets deployment on both Windows and Linux.

It is written to be:
- easy to follow as a human,
- hard to misread as an AI agent,
- explicit about WHY rules exist,
- explicit about exceptions and how to justify them.

If a rule here conflicts with any other rule, note, or document in this repository, follow `CODE-QT6.md`.

Prose conventions:

- Use British English spelling in documentation and comments.
- In computing prose, prefer "program" (not "programme").
- Exception: keep original spelling when quoting fixed names (Qt APIs, standard library entities, platform APIs, identifiers, file names, protocol fields, etc.).

## Core priorities

Core priorities are listed below in order (most important first). When they conflict, prefer the earlier one and document the trade-off. Priorities guide judgement calls; they do not override MUST rules.

- Clarity for the reader.
  - Code is primarily communication. If something is hard to read, it is hard to review, test, and change safely.
- Correctness and portability.
  - Prefer defined behaviour and explicit contracts. Avoid accidental platform/toolchain assumptions.
- Performance, but only when it is justified.
  - Do not make the code harder to understand "for speed" unless there is a clear need.
  - Justification usually means measurement (profiling/benchmarks) or an architectural constraint documented in the module DD file (e.g. hot path, allocator constraints, real-time budget).

AI Agents must also aim to preserve human understanding of the codebase.
  - AI assistance MUST NOT allow important design knowledge to become lost and overlooked inside generated patches.
  - Workflow should produce evidence that the human reviewer understands a non-trivial change well enough to describe it accurately and modify it later.

## C++ subsidiarity (house policy)

This codebase uses "C++ subsidiarity": we use the most appropriate C++ facilities when they genuinely provide clarity, correctness, or a material capability, but we bias heavily toward explicit, C-like code otherwise.

MUST:

- Prefer the simplest construct that meets the need.
- Prefer explicit control flow and explicit data flow.
- If a C++ feature materially changes the reader's reasoning model (templates, operator overloading, exceptions, implicit conversions, complex RAII, type erasure), its use MUST be local, obvious at the call site, and justified.

WHY: many C++ features can hide control flow, hide types, or create non-local behaviour. That undermines the core priorities.

## Terms

- Module: a cohesive unit with a public interface and optional implementation files.
- Module boundary: any function/type declared in the module's public header(s), plus any entry point callable by framework-managed code (Qt boundaries) or externally owned threads.
- Public: visible outside the module/translation unit.
- Private: internal helpers and data.
- DD file: the module's design document file, stored as `*-dd.txt`.
- Qt boundary: any boundary where control enters Qt-managed code (event loop callbacks, slots, QObject virtuals, QThread entry points, QtConcurrent continuations, message handlers, etc.).
- Third-party boundary: any call into code not owned by this repository.
- Owning pointer: a pointer whose owner is responsible for deleting/freeing the pointed-to object.
- Borrowed pointer/view: a non-owning reference to memory owned elsewhere; the borrower must not outlive the owner.
- UI thread: the thread that owns the GUI event loop.
- Adaptor: a small boundary wrapper whose job is to translate between policies (e.g. exceptions to status returns, std types to Qt types, platform APIs to Qt/core abstractions).
- Trivial change: a change is trivial when it is limited to:
  - formatting/whitespace-only changes,
  - spelling/grammar fixes that do not change meaning,
  - commit message edits.
  - And nothing else.
- Non-trivial change: any change that is not trivial.
  - Examples include (non-exhaustive): behaviour changes, bug fixes, public contract changes, refactors, performance changes, unit test changes, ownership/lifetime/thread-safety changes, portability/build/dependency changes, adding/removing/renaming files, or changing which files will be committed.
- Load-bearing design element: a public interface, protocol/message format, state machine, ownership/lifetime rule, concurrency rule, error-handling rule, or other design choice that materially affects how later code must be structured.
- Patch walkthrough: an explanation of work done, covering the changed files, the purpose of each change, key invariants, risky areas or edge cases, and consequences future maintainers need to know.

## 1) Architecture and modules

### 1.1 Module categories (MUST)

Each module MUST belong to a category with exactly two parts written as `domain/topic` (e.g. `crypto/random`, `net/http`).

- The first part SHOULD be a general domain (broad).
- The second part SHOULD be the cohesive topic (narrow).
- Category names MUST describe the public concept/service, not implementation details or policy.
- Avoid junk-drawer categories (`misc`, `common`, `util`, `helpers`); if you need one, the module boundary/category is probably wrong.

WHY: the category becomes part of navigation, include paths, and test layout; a fixed two-level taxonomy keeps navigation predictable and prevents category sprawl.

### 1.1.1 Module kinds (MUST)

A module MUST be either:

- a pure-function module (no externally visible mutable state), or
- an object module (a type with invariants and a defined lifetime), or
- a Qt object module (a `QObject`-derived type with signal/slot behaviour).

Hidden global state MUST be avoided.

WHY: hidden global state makes testing and reasoning harder; making module shape explicit improves composition.

### 1.2 Module layout (SHOULD)

This repository prefers a predictable layout. For a module `foo` in category `bar/baz`:

- `modules/bar/baz/foo.h` SHOULD exist and provide the public interface.
- `modules/bar/baz/foo.cpp` MAY exist and provide platform-agnostic implementation.
- `modules/bar/baz/foo-win32.cpp` MAY exist and provide Windows-specific implementation.
- `modules/bar/baz/foo-posix.cpp` MAY exist and provide POSIX-specific implementation.
- `modules/bar/baz/foo-dd.txt` MUST exist and document the module.
- `apps/unittests/bar/baz/tst_foo.cpp` SHOULD exist when `foo.h` exposes a public API that can be unit tested.

WHY: predictable layout makes navigation fast, keeps portability boundaries sharp, and makes it easy for tooling (including AI agents) to find the right place to change.

### 1.3 Visibility and linkage (MUST)

- Any helper with file-local meaning SHOULD be in an unnamed namespace in `.cpp` files.
- Do not expose internal symbols from headers.
- In headers, avoid `using namespace ...`.

- Global mutable state MUST NOT be introduced without a DD file justification.
- File-scope constants SHOULD be `constexpr` and `static` (or in an unnamed namespace).

WHY: this prevents accidental symbol export/collision and keeps APIs small and reviewable.

### 1.4 Platform-specific code (MUST)

- Platform-specific behaviour MUST be isolated behind a platform-agnostic interface.
- Avoid scattering `#if defined(Q_OS_...)` throughout core logic.
- Prefer separate platform implementation files where practical.

- Platform-specific implementation files MUST be compilation-guarded with a real platform check, even if there is currently only one platform implementation.
- For a given module, it MUST be true that exactly one platform implementation is compiled for a given build.

Example guard:

```cpp
#if defined(Q_OS_WIN)
// Windows implementation
#endif
```

WHY: scattered conditional compilation creates unreadable code paths and increases test surface. A small boundary makes it easier to reason about correctness and add new platforms.

### 1.5 Baseline requirements (MUST)

- Language baseline: C++17.
- Qt baseline: Qt 6.
- Platforms: Windows and Linux.
- Exceptions: enabled in the toolchain.
- `QT_NO_EXCEPTIONS` MUST NOT be defined for this project.
- House policy: project code MUST NOT use `throw` (see Section 10.6).

Any deviation (new platform, newer standard mode, compiler extension reliance, mandatory third-party dependencies) MUST be declared in the module DD file Requirements section and justified.

WHY: explicit requirements prevent build drift and "it worked on my machine" failures.

### 1.5.1 Qt strictness macros (SHOULD)

To reduce dangerous implicit conversions and error-prone legacy Qt facilities, the build SHOULD enable Qt strictness macros at the project level.

Recommended (when supported by the chosen Qt baseline):

- `QT_ENABLE_STRICT_MODE_UP_TO=QT_VERSION_CHECK(<minQtMajor>, <minQtMinor>, 0)`
- `QT_NO_FOREACH`

For libraries/public headers (SHOULD):

- define `QT_NO_KEYWORDS` and use `Q_SIGNALS` / `Q_SLOTS` / `Q_EMIT`.

WHY: strict-mode disables APIs that tend to create non-obvious behaviour (implicit conversions, contextless connects, etc.). This aligns with the core priorities.

### 1.5.2 Compiler warnings and analysis (SHOULD)

- Warnings SHOULD be enabled at a high level on all supported toolchains.
- Warnings that indicate likely bugs (shadowing, narrowing, implicit conversions, missing `override`, sign conversions) SHOULD be treated as errors.
- Sanitiser builds (ASan/UBSan) SHOULD exist in CI for supported compilers.

WHY: these tools catch whole classes of portability and correctness bugs early.

### 1.6 Choosing among Qt vs standard C++ vs platform APIs (MUST)

Prefer solutions in this order:

1) Qt cross-platform facilities (especially for text, paths, files, and threading around the Qt event loop)
2) standard C++17 facilities
3) platform APIs (Win32/POSIX)
4) compiler extensions

The further down the list you go, the smaller the surface area MUST be and the stronger the justification MUST be.

WHY: Qt is already a required dependency and provides consistent cross-platform behaviour; platform APIs and extensions increase portability/testing burden.

## 2) Documentation: DD files

### 2.1 DD structure (MUST)

Each `*-dd.txt` contains:

- Description: what it is, what it does, what it does NOT do (Smalltalk class comment-like).
- Public interface: how external code uses the module (manpage-like).
- Implementation details: internal design decisions, algorithms, invariants, tricky edge cases, and any load-bearing design elements that future changes will need to preserve or revisit.
- Requirements: build/runtime/environment assumptions. This section MAY be omitted if all requirements are baseline and there is nothing more to say.

WHY: API users and maintainers need different information; separating it reduces noise while preserving rigour.

- All public-facing functions, methods, and data structures MUST be described in the Public interface section.
- The Implementation details section MAY omit trivial/private details when there are no hidden invariants or tricky edge cases.

WHY: the public contract must be testable and reviewable without reading the implementation.

### 2.1.1 Class-comment style (SHOULD)

The DD file Description section SHOULD read like a Smalltalk-style class comment:

- what the module/type is for,
- what it explicitly does not do (non-goals),
- what state model callers should assume (ownership, lifetime, thread affinity, re-entrancy).

The tone should be human-readable and usage-oriented.

WHY: it frames the reader's mental model, which is more valuable than listing every function.

### 2.1.2 Formal reasoning in DD files (SHOULD)

When an implementation depends on non-obvious properties (tricky arithmetic, indexing formulas, overflow/UB hazards, thread-affinity invariants, lock ordering, subtle portability constraints), the Implementation details section SHOULD include formal reasoning such as:

- invariants (what must always be true, and what maintains it),
- bounds (max/min sizes, indices, capacities),
- overflow/UB arguments (why operations are defined and safe),
- time/space complexity and why it matters for this module,
- derivations for formulas (enough that a maintainer can re-derive or verify them).

The reasoning SHOULD be written so a reader can check it:

- define symbols and assumptions,
- state what is being proven and why the proof matters,
- where practical, encode key assumptions as checks:
  - runtime `Q_ASSERT` / `assert` for developer-time invariants,
  - `static_assert` for compile-time invariants.

WHY: the hardest-to-maintain code is where correctness lives in unstated math or unstated invariants. A proof in the DD file turns "trust me" code into reviewable code.

### 2.1.3 Where comments belong (MUST)

- Anything fully described in the DD file SHOULD NOT be duplicated in headers/implementation files.
- Otherwise, comment liberally but succinctly. If in doubt, comment.

WHY: duplication rots; one canonical explanation reduces contradictory docs.

### 2.2 DD files and unit tests (MUST)

- The Public interface section is the canonical contract for externally observable behaviour.
- Unit tests MUST be derived from the Public interface section.
- If a unit test asserts behaviour not described in the Public interface section, you MUST either:
  - update the Public interface section (if the behaviour is real and important), or
  - change the test (if the behaviour is accidental or undesirable).

WHY: a written contract prevents misunderstandings, and an executable contract (tests) prevents silent drift.

### 2.3 Requirements section contents (MUST)

The `Requirements` section MUST include (when applicable):

- C++ standard mode (baseline is C++17).
- Qt version range (baseline is Qt 6).
- Platforms/ABIs (Windows, Linux).
- Threading/event-loop assumptions.
- Exception policy notes (e.g. "no throw", and where catch/translation occurs).
- Third-party libraries (and minimum versions).

WHY: requirements are part of the contract between the module and the build/deployment environment.

### 2.4 DD template (SHOULD)

New modules SHOULD start from a template.

```text
= <ModuleName>

== Description
<What it is, what it does, what it does NOT do>

== Public interface
<Usage rules, contracts, ownership, lifetime, thread safety, Qt boundary notes>

== Implementation details
<Algorithms, invariants, proofs/derivations, data layout, complexity>

== Requirements
- Language: C++17.
- Qt: 6.x.

Optional (include if applicable):
- Platform-specific implementations: Windows and Linux.
  - Name the platform-facing API boundary and list the platform implementation files.
  - Example:
    - Platform-facing boundary: `FileWatcherBackend` (`modules/os/fs/fileWatcher_backend.h`).
    - Implementations: `modules/os/fs/fileWatcher_backend-win32.cpp`, `modules/os/fs/fileWatcher_backend-posix.cpp`.
- Build flags: <warnings/sanitisers/etc>.
- Extensions: <compiler extensions relied upon>.
- Third-party libraries: <names and minimum versions>.
- Runtime assumptions: <threading model, event loop assumptions, etc>.
```

WHY: a consistent structure makes DD files skimmable, comparable across modules, and easy for tools to consume.

## 3) API shape and contracts

### 3.1 Contracts must be explicit (MUST)

Public APIs MUST state:

- valid argument rules (including nullability),
- ownership (who owns what, who deletes/frees),
- lifetime rules for returned pointers/views,
- thread-safety expectations (re-entrant, thread-compatible, thread-safe),
- exception behaviour (this project is "no throw" by policy; if a boundary catches, say so).

WHY: most bugs are contract bugs (ownership, lifetime, invalid arguments, thread affinity). Making the contract explicit makes correct usage possible.

#### 3.1.1 Contract wording templates (copy/paste)

These templates are intentionally formulaic so humans and AI agents can recognise them.

```text
Arguments:
- `foo` must not be null.
- `bar` may be null only when `barLen` is 0.

Ownership:
- Returns an owning pointer; caller must delete it.
  or
- Returns a `QObject*` whose parent is `<thing>`; caller must not delete it.

Lifetime:
- Returns a borrowed view valid until the next mutating call / until `close()` / until `<owner>` is destroyed.

Thread safety:
- Thread-safe: Yes/No.
- Re-entrant: Yes/No.
- Notes: <what must be synchronised, what is immutable, what is per-instance>

Errors:
- Returns true on success, false on failure.
- On failure, logs an error with context.
```

WHY: consistent wording prevents implied contracts and makes review and tooling more reliable.

### 3.2 Nullability and parameter style (MUST)

- Use pointers for nullable parameters.
- Use references only when null is not a valid value and reference semantics materially improves clarity.
- For output values, prefer out-parameters (`Foo *outFoo`) or references (`Foo &outFoo`) plus a simple success/failure return.

WHY: pointer-vs-reference communicates nullability; explicit out-parameters make multi-value returns obvious.

### 3.3 Qt types: text vs bytes (MUST)

- Use `QString` for human text.
- Use `QByteArray` for binary data.
- Do not treat `QByteArray` as a string implicitly.
- When converting, be explicit about encoding (prefer UTF-8 for interoperability).

WHY: text/binary confusion is a major source of bugs, especially on Windows where path and console encodings differ.

#### 3.3.1 Qt-first types (MUST)

- By default, prefer Qt value types in core code (`QString`, `QByteArray`, `QVector`, `QHash`, `QDateTime`, etc.).
- Do not sprinkle conversions between Qt and standard types throughout the code.
- If an interface boundary requires standard types, do the conversion in a small adaptor module and document the boundary.

WHY: consistent types reduce friction and conversion bugs; localised adaptors make boundaries explicit and reviewable.

### 3.4 Buffer + length pairs (SHOULD)

- If an API accepts a raw buffer, it SHOULD also accept its length (pointer + length).
- Prefer Qt/standard value types that carry length (`QByteArray`, `QString`) when ownership is clear.

WHY: explicit lengths prevent overreads/writes and avoid accidental quadratic behaviour.

### 3.5 Validate at the boundary (SHOULD)

- Public functions SHOULD validate inputs at the boundary and return a defined error state.
- Internal helpers MAY assume preconditions when enforced by callers (document assumptions).

WHY: boundary checks keep undefined behaviour and contract violations out of public call sites while still allowing performance-sensitive internals.

### 3.6 Const-correctness (MUST)

- If a function's contract forbids modifying an input, it MUST take it as `const`.
- If a function returns a pointer/view to data that callers must not modify, it SHOULD return a `const` pointer/view.
- If you must call an API that lacks `const` correctness, localise the cast at the boundary and document why it is safe.

WHY: `const` is part of the contract; it prevents accidental writes and makes APIs harder to misuse.

### 3.7 Do not ignore important return values (SHOULD)

- Functions that return status SHOULD be marked `[[nodiscard]]`.
- Call sites SHOULD handle the status immediately.

WHY: silently ignoring a failure is a common source of latent bugs.

### 3.8 Views and borrowed ranges (SHOULD be rare)

- Prefer owning Qt value types (`QString`, `QByteArray`) in public APIs unless profiling shows a need.
- Non-owning views (`QStringView`, `QByteArrayView`, raw pointer+length) MAY be used when they materially reduce copying, but their lifetime rules MUST be documented.
- Do not store borrowed views in long-lived objects unless the owner/lifetime is part of the explicit contract.

WHY: borrowed views are easy to dangle; avoiding them by default keeps lifetime reasoning simple.

## 4) C++ language use (subsidiarity rules)

### 4.1 Forbidden-by-default facilities (MUST NOT)

The following MUST NOT be used in project code. If a module DD file justifies it, some of these MAY be tolerated in tightly localised code (see the relevant section). Explicit `throw` is forbidden (Section 10.6).

- iostreams (`std::cout`, `std::cerr`, `std::ostringstream`, streaming operator-based logging).
- template metaprogramming (SFINAE tricks, type-level computation, expression templates).
- operator overloading that is not obviously numeric/structural.
- macros that hide control flow or create DSL-like syntax.

WHY: these features often hide types/control flow and make correctness harder to reason about.

### 4.2 Templates (MAY; MUST be justified)

Templates MAY be used when all of these are true:

- the generic behaviour is simple,
- the template can be understood without reading generated code,
- the instantiation set is small and predictable,
- compilation time and error messages remain reasonable.

Templates MUST NOT be used to simulate a different language.

WHY: templates are powerful but can turn straightforward code into hard-to-debug machinery.

### 4.3 Operator overloading (MAY; MUST be justified)

Operator overloading MAY be used when the type genuinely represents a numeric or algebraic domain where operators match the reader's existing mathematical model (e.g. vectors, matrices, quaternions).

This also includes deliberate "structural" operator use such as bitwise operations on flag sets.

MUST:

- Overloads MUST preserve conventional meaning (`+` adds, `*` multiplies/combines, `==` tests equality).
- Overloads MUST be cheap to understand at the call site.
- Avoid clever proxy types and expression-template tricks.

WHY: operator overloading can improve clarity for true numeric types, but it can also create unreadable code and surprising performance.

#### 4.3.1 Do not use `operator<<` chaining (MUST)

- Do not use overloaded `operator<<` for "streaming" or chaining in project-authored code.
  - This includes (but is not limited to):
    - Qt message streaming (for example: `qWarning() << ...`, `qDebug() << ...`).
    - `QTextStream` output chains.
    - `std::ostream` output chains (`std::cout << ...`).
    - Serialisation chains such as `QDataStream << ...`.
    - Qt container append idioms such as `QStringList() << ...` or `args << ...`.

- The bitshift operators (`<<` / `>>`) are allowed when used as bitshifts.
- Overloads of `<<` / `>>` are only acceptable when the meaning is genuinely "shift-like" (for example: fixed-point numeric shift).

Exception:
- If a third-party API is fundamentally designed around `operator<<` (for example, QtTest data rows), and avoiding it would do more harm than good, it may be used.
  - In that case, keep the usage local and add a short inline comment explaining why this is the least harmful option.

WHY: This project prioritises readability above all else. Streaming/chaining syntax encourages long, hard-to-scan expressions, hides formatting and encoding decisions, and makes it harder to reason about allocations and side effects. Explicit formatting and explicit append/insert calls are more reviewable and less error-prone.

### 4.4 Implicit conversions (MUST be minimised)

- Single-argument constructors SHOULD be `explicit` unless implicit conversion is part of the deliberate, documented API.
- Avoid user-defined conversion operators.
- Prefer named conversion functions.

WHY: implicit conversions create non-local behaviour and surprising overload resolution.

### 4.5 `auto` (SHOULD be rare)

- Prefer explicit types.

`auto` MAY be used only when at least one of these is true:

- the type is unnamed (e.g. a lambda/closure type),
- the explicit type is long and adds no clarity (e.g. iterator types),
- the type's spelling is not stable/portable (implementation-defined or unspecified library types), and the variable is local and short-lived.

`auto` MUST NOT be used when it hides important information:

- numeric type/signedness/size type,
- value vs reference (copy vs alias),
- ownership (owning vs borrowed),
- Qt detaching/copy behaviour.

`auto` MUST NOT be used for raw pointer declarations. Write the explicit pointer type.

Examples:

```cpp
// Unnamed type (lambda): auto is the clearest.
auto onDone = [this]( const QByteArray &bytes ) {
    handleBytes( bytes );
};

// Long type adds no clarity (iterator).
auto it = m_items.constFind( key );
if ( it != m_items.constEnd() ) {
    useItem( it.value() );
}

// Prefer explicit pointer types.
QDialog *dialog = new QDialog( parent );
dialog->setModal( true );
```

WHY: hidden types slow down review and make it harder to reason about copies, detaches, lifetimes, and performance.

### 4.6 C++ basics (MUST)

- Use `nullptr`, not `NULL` or `0`.
- Prefer `enum class` for new enumerations.
- Virtual overrides MUST use `override`.
- Types not designed for inheritance SHOULD be marked `final`.
- Disable unwanted copy/move explicitly with `= delete`.

WHY: these rules make intent explicit and prevent common C++ hazards.

### 4.7 Casts (MUST)

- Avoid casts.
- If a cast is necessary, prefer C++ casts (`static_cast`, `const_cast`, `reinterpret_cast`) so intent is explicit.
- C-style casts MUST NOT be used.
- `reinterpret_cast` MUST be rare and justified in the DD file.

WHY: casts are often a sign of a boundary violation or a type design problem; making them explicit helps review.

### 4.8 Inheritance and virtual dispatch (SHOULD be minimised)

- Prefer composition over inheritance.
- Inheritance for non-Qt types SHOULD be rare and justified.
- Multiple inheritance MUST be avoided except for the Qt pattern of `QObject` plus a pure abstract interface (and that use MUST be documented).

WHY: inheritance creates non-local behaviour and makes invariants harder to reason about.

### 4.9 RTTI and downcasts (SHOULD be avoided)

- Prefer designing explicit interfaces over relying on downcasts.
- For `QObject` hierarchies, prefer `qobject_cast` over `dynamic_cast`.
- `typeid` and RTTI-based control flow SHOULD be avoided.

WHY: downcasts are fragile and often indicate a missing interface boundary.

### 4.10 Lambdas and captures (MUST)

- Lambda captures MUST be explicit; avoid `[&]` and `[=]`.
- Do not capture borrowed references into work that can outlive the current scope.
- For Qt async callbacks (timers, queued connections, futures), do not capture raw `this` unless lifetime is guaranteed by a context object; prefer a context object and/or a `QPointer` for guarded access.

WHY: capture lists are a frequent source of use-after-free and unintended sharing.

### 4.11 Type erasure (SHOULD be avoided)

- Avoid `std::function` when a template or function pointer is clearer and cheaper.
- Avoid `std::any`.
- Avoid pervasive `QVariant`/dynamic property usage in core logic.

Exception (MAY): type erasure is allowed at deliberate boundaries (plugin systems, UI glue, property systems) when documented.

Note:

- `std::optional` MAY be used for local optional values when it improves clarity.
- Public APIs SHOULD still prefer explicit status + out-parameters unless `std::optional` is clearly the better contract.

WHY: type erasure hides types and often hides allocations; it makes reasoning and performance analysis harder.

### 4.12 `noexcept` (MUST be used deliberately)

- Destructors MUST be `noexcept` (implicitly or explicitly).
- Move constructors/assignments SHOULD be `noexcept` when they cannot fail.
- Boundary adaptors that catch/translate third-party exceptions SHOULD be marked `noexcept`.

WHY: `noexcept` is part of the contract. Used well, it prevents exceptions escaping forbidden boundaries.

### 4.13 Integer arithmetic and undefined behaviour (MUST)

- Code MUST NOT rely on undefined behaviour for correctness.
- Signed integer overflow is undefined behaviour; code MUST NOT rely on it.
  - If overflow is possible, prevent it (range checks / wider type) or use unsigned arithmetic and document modulo assumptions.

- Bit operations SHOULD use unsigned types.
  - Shift counts MUST be in range (0..width-1).
  - Avoid right-shifting negative signed values.

- Be deliberate about size types:
  - Qt container sizes use `qsizetype`.
  - Raw buffer sizes from C/OS APIs are often `size_t`.
  - Conversions between signed and unsigned sizes MUST be range-checked.

- Pointer arithmetic MUST stay within the same array object (except one-past-end as allowed by the standard).

- Code MUST obey aliasing and alignment rules.
  - Do not type-pun via pointer casts.
  - Use `std::memcpy` for reinterpretation and `unsigned char *` for raw byte access.

WHY: UB often works until optimisation level, compiler, or architecture changes; then it fails in ways that are hard to debug.

## 5) Naming (Qt style)

### 5.1 General rules (MUST)

- Names MUST be descriptive; do not avoid long names when they reduce ambiguity.
- Name things by service/interface, not implementation.

WHY: names are the primary API.

### 5.2 Identifiers (MUST)

- Types: `CamelCase` (e.g. `PacketParser`).
- Functions/methods: `lowerCamelCase` (e.g. `parseHeader`).
- Members: `m_` prefix (e.g. `m_buffer`).
- Constants: prefer `constexpr` with `k` prefix (e.g. `kMaxRetries`).
- Enums: prefer `enum class` with `CamelCase` enumerators.

- Local variables: `lowerCamelCase`.
- Booleans: predicate names (`isReady`, `hasData`, `canWrite`).

WHY: Qt style matches Qt documentation and common tooling expectations.

### 5.2.1 File names (SHOULD)

- Prefer lower-case file names.
- Platform-specific suffixes SHOULD be `-win32` and `-posix` (matching Section 1.2).
- Avoid names that differ only by case.

WHY: case-only differences break on case-insensitive file systems and create portability problems.

### 5.3 Reserved identifiers (MUST NOT)

- Do not define identifiers with leading underscores at global scope.
- Do not define identifiers containing `__`.
- Do not define identifiers that begin with `_` followed by an uppercase letter.

WHY: reserved identifiers are a portability hazard across libc/compiler implementations.

## 6) Headers and includes

### 6.1 Include ordering (MUST)

Includes MUST be grouped and ordered:

1) standard library headers
2) platform headers
3) Qt headers
4) third-party headers
5) project headers

Each group is separated by a blank line; within each group, headers are alphabetical.

WHY: stable ordering reduces diff noise and makes dependencies visible.

Windows header hygiene note:

- Avoid including `windows.h` from public headers.
- Prefer `#include <qt_windows.h>` over `#include <windows.h>`.
- If `windows.h` is required, centralise it behind a single project header that sets `WIN32_LEAN_AND_MEAN` and `NOMINMAX` before including it.

WHY: `windows.h` is macro-heavy and can break code in non-obvious ways (e.g. `min`/`max` macros). Centralising and isolating it reduces portability surprises.

#### 6.1.1 Include syntax (MUST)

- Use `#include <...>` for standard library headers and externally-provided headers (Qt, platform, third-party).
- Use `#include "..."` for project headers.
- Project headers SHOULD be included via their canonical path (e.g. `"modules/bar/baz/foo.h"`), not via relative `../` paths.

WHY: it keeps dependencies explicit and prevents include-path accidents.

### 6.2 Include hygiene (MUST)

- All C++ files (e.g. `.h`, `.cpp`) MUST include what they use; do not rely on transitive includes or include order.
- Every project header MUST have an include guard.
- `#pragma once` MAY be used in addition to an include guard, but MUST NOT replace it.

WHY: brittle include order causes non-reproducible builds and platform-specific breakage.

#### 6.2.1 Include guard naming (MUST)

- Include guards MUST use a unique macro derived from the header path and MUST NOT use reserved identifier forms.
- Suggested pattern: uppercase path with underscores, ending in `_H`.

Example for `modules/bar/baz/foo.h`:

```cpp
#ifndef MODULES_BAR_BAZ_FOO_H
#define MODULES_BAR_BAZ_FOO_H

// ...

#endif
```

WHY: consistent guard naming prevents collisions and makes headers easy to recognise in diagnostics.

#### 6.2.2 Forward declarations (SHOULD)

- Prefer forward declarations in headers when they reduce dependencies.
- Do not forward declare types that must be complete in the header (value members, base classes, templates that require the full definition).

WHY: reducing includes speeds up builds, but incomplete-type mistakes produce brittle code.

### 6.3 Qt keywords in public headers (SHOULD)

- In library/public headers, prefer `Q_SIGNALS`, `Q_SLOTS`, and `Q_EMIT` over bare `signals`, `slots`, and `emit`.

WHY: `signals`/`slots`/`emit` are macros; using the `Q_` forms keeps intent explicit and reduces macro pollution.

### 6.4 Macros (SHOULD be minimised)

- Prefer language facilities over macros:
  - `constexpr` constants over `#define` constants.
  - inline functions over function-like macros.

If you must use a function-like macro (MUST):

- The macro MUST evaluate each argument exactly once.
- Macro expansions MUST parenthesise parameters and the whole expansion where expression context requires it.

WHY: macros have no types or scope; most macro bugs are double-evaluation and precedence surprises.

## 7) Formatting

### 7.1 Expression spacing (MUST)

- Binary operators MUST have whitespace on both sides.
- Prefix unary operators MUST NOT have whitespace between the operator and its operand.
- Postfix operators and member access MUST NOT introduce whitespace.

Example:

```cpp
answer = ( scale * x ) / y;

if ( !ptr )
    answer = 0;

tmp = *p;
count = a + b;
ok = ( x == y ) && ( i < n );
```

WHY: it makes scan-reading and AI edits safer.

### 7.2 `switch` indentation (MUST)

Cases in a `switch` MUST align with the enclosing braces (no extra indentation level).

WHY: prevents "double indentation" and keeps case labels visually distinct.

### 7.3 Control blocks spacing (MUST)

- Add a blank line before and after a control block (`if`, `for`, `while`, `switch`, etc.), unless it is part of the same logical multi-block.
- A multi-block (`if`/`else if`/`else`) MUST have no blank lines between arms, but SHOULD have a blank line before the chain and after the chain.
- Exception: no leading blank line is needed if it is the first statement in the scope; no trailing blank line if it is the last statement in the scope.

WHY: consistent whitespace makes structure obvious, especially during reviews and diffs.

### 7.4 Equality style (SHOULD)

- Prefer constants/literals on the left in comparisons:
  - `0 == n`, `'x' == kind`, `nullptr == ptr`.

WHY: it prevents accidental assignment in conditionals on toolchains with weaker warnings, and it makes comparisons visually uniform.

## 8) Control flow (house style: MUST)

These rules are strict because they prevent subtle bugs and make changes safer.

### 8.1 Single exit (MUST)

- Non-`void` function-like objects (functions, methods, lambdas, etc.) MUST have a single `return` at the end.
- `void` function-like objects MUST NOT use early `return`.
- A trailing `return;` at the end of a `void` function-like object is redundant and SHOULD be omitted.

WHY: a single exit makes cleanup and invariants easier to audit.

### 8.2 No `break`/`continue` in loops (MUST)

- Loops MUST NOT use `break` or `continue`.
- Exception: `break` is allowed inside `switch`.

WHY: hidden control transfers make it harder to reason about loop invariants and cleanup.

### 8.3 Braces policy (MAY omit only for single statements)

- A control structure MAY omit braces only if the immediate body contains exactly one statement.
- Nested single-statement bodies MUST still be braced.
- For `if/else if/else`, if any arm uses braces then all arms MUST use braces.

WHY: this preserves concision for trivial cases while preventing the classic "one-line edit introduces a bug" problem.

### 8.4 No `goto` (MUST NOT)

- Code MUST NOT use `goto`.

WHY: in C++ (especially with RAII), `goto` is rarely needed and often obscures control flow.

### 8.4.1 No `setjmp`/`longjmp` (MUST NOT)

- Code MUST NOT use `setjmp`/`longjmp`.

WHY: non-local jumps bypass normal control flow and can skip destructors/cleanup, breaking invariants and causing leaks.

### 8.4.2 `switch` correctness (MUST)

- Fallthrough MUST be explicit.
  - Prefer `[[fallthrough]];`.

- For `enum class` switches, prefer to list all enumerators and omit `default` so toolchains can warn on missing cases.
- If `default` is required, it MUST handle safely (and SHOULD log/assert).

WHY: explicit fallthrough prevents accidental bugs, and exhaustive enum handling prevents regressions when new values are added.

### 8.5 Prefer affirmative conditionals (SHOULD)

- Prefer `if ( ptr ) ... else ...` over `if ( !ptr ) ... else ...`.
- Exception: if one branch is very long and the other is short, putting the short branch first MAY improve readability.

WHY: readers parse the normal path faster when it is written positively.

### 8.6 Single-exit error handling example (pattern)

Example shape (no early returns, no exceptions escaping):

```cpp
[[nodiscard]] bool ConfigLoader::loadFile( const QString &path, Config *outConfig )
{
    bool ok;
    QFile file( path );

    ok = false;

    Q_ASSERT( outConfig );

    if ( file.open( QIODevice::ReadOnly ) ) {
        const QByteArray bytes = file.readAll();
        if ( !bytes.isEmpty() ) {
            if ( parseConfigBytes( bytes, outConfig ) ) {
                ok = true;
            }
        }
    }

    return ok;
}
```

Notes:

- RAII (`QFile`) still provides cleanup.
- The result is accumulated in `ok` and returned once.

WHY: this preserves the house control-flow rules while still using C++/Qt resource management.

## 9) Function size

- Once a function reaches ~24 lines (including signature, braces, and body), you MUST consider factoring.

Good candidates to factor out:

- code that does one thing well and can be described in only a few words,
- code that does not need much state (few parameters),
- code whose state is mostly self-contained (parameters + locals that do not interact with the rest of the function),
- code that looks reusable even if it is not currently reused.

Bad candidates are the reverse:

- Do NOT shrink functions by deleting useful comments or formatting.
- Large dispatch functions (big `switch`/`if-else`) are allowed if each case is simple.

WHY: 24 lines is a cognitive breakpoint (traditional terminal height). Factoring improves testability and reuse when it reduces coupling. Factoring is useful when it reduces coupling, not when it merely moves lines around.

Example of a justifiable long function:

- a large `switch` dispatch that maps input kinds to a single call or assignment per case.

## 10) Error handling and reporting

### 10.1 Always have an error reporting facility (MUST)

- If the program can produce an error or warning, it MUST have a central error reporting facility.
- The interface MUST at minimum support Warning and Error levels and printf-style formatting.
- iostream-style logging and streaming operator chains MUST NOT be used.  See Section 4.3.1.

This includes Qt streaming loggers such as `qDebug() << ...`.

Allowed forms (SHOULD be consistent within the project):

- a project logger API (e.g. `logWarning( "..." )`, `logError( "..." )`),
- Qt printf-style logging functions/macros (e.g. `qWarning( "%s", ... )`) provided they do not use streaming operators.

If log categorisation exists, messages SHOULD include a stable component/category.

WHY: centralised reporting makes behaviour consistent; printf-style formatting is explicit and readable.

#### 10.1.1 Formatting and encodings (MUST)

- Prefer format strings over stream operators.
- Be explicit when formatting Qt text:
  - use UTF-8 at boundaries by converting with `toUtf8()`.

- Do not take `constData()` from a temporary `QByteArray`.
  - Bind the `QByteArray` to a named local first.

Example:

```cpp
const QByteArray pathUtf8 = path.toUtf8();
logError( "Failed to open: %s", pathUtf8.constData() );
```

WHY: explicit formatting is easier to read and review; explicit UTF-8 avoids platform-dependent "local 8-bit" surprises.

### 10.2 Avoid silent failures (MUST)

- Errors and warnings MUST be reported as close to where they occur as practical.
- Prefer propagating error states, not error strings; log with local context.

WHY: upstream callers usually lose the detailed context needed to explain what happened.

### 10.3 Capture OS error codes immediately (MUST)

- When reporting a failure from a library/OS call that exposes a last-error code (e.g. `errno`, `GetLastError()`), capture it immediately after the failing call.

WHY: last-error values are fragile; logging them incorrectly produces misleading diagnostics.

### 10.4 Assertions are not error handling (MUST)

- Use assertions (`Q_ASSERT` / `assert`) to document internal assumptions and catch programmer mistakes during development.
- Assertions MUST NOT be used as user-facing error handling.
- Assertions SHOULD be enabled/disabled via build configuration (e.g. `QT_NO_DEBUG`, `NDEBUG`), not by editing source.

WHY: assertions provide "debug teeth"; production failures need defined error paths.

### 10.5 Error propagation shape (SHOULD)

- Prefer simple success/failure returns (`bool` or an enum error code) plus out-parameters.
- Out-parameters MUST have defined behaviour on failure (document whether they are unchanged or set to safe defaults).

WHY: consistent error conventions prevent ad-hoc error handling.

### 10.5.1 Centralised cleanup pattern (SHOULD)

- Functions that acquire multiple resources SHOULD be structured so cleanup is centralised and the single-exit rule is preserved.
- When a resource is not RAII-managed, initialise it to a safe sentinel (often `nullptr`), and release it at the end only if it was acquired.
- Prefer a "commit at the end" pattern for mutations:
  - build/validate new state in locals,
  - assign to member state only when the operation is fully successful.

WHY: most error-handling bugs are cleanup and partial-state bugs. A predictable structure prevents leaks, double-frees, and half-written state.

### 10.6 Exceptions policy (MUST)

#### 10.6.1 Toolchain (MUST)

- Exceptions are enabled.

#### 10.6.2 House policy (MUST)

- Project code MUST NOT use explicit `throw`.
- No exception is allowed to escape a module boundary or a Qt boundary.
- Any call into potentially-throwing third-party code MUST catch exceptions at the lowest practical boundary and translate to the module's normal error/result convention.

#### 10.6.3 Identify potentially-throwing boundaries (MUST)

- Treat these as potentially-throwing boundaries and guard them accordingly:
  - third-party libraries
  - QtConcurrent/QFuture retrieval points (Qt can transfer and rethrow exceptions in some facilities)

- Treat any operation that may allocate (directly or transitively) as potentially throwing, unless it is explicitly non-throwing and documented.

Common potentially-throwing operations include:

- `new` (and any code path that may allocate)
- Qt containers/strings and conversions that may allocate
- standard library containers/strings/algorithms that may allocate

Explicitly non-throwing allocation patterns (MAY; MUST be used deliberately):

- `new ( std::nothrow )` with explicit null handling
- `malloc`/`calloc`/`realloc` with explicit null handling
  - Do not mix allocation families: `malloc`/`free` memory MUST NOT be managed with `delete`, and `new` memory MUST NOT be released with `free`.
  - These APIs SHOULD be used only in low-level boundary/adaptor code (C interop, raw byte buffers, allocator/pool implementations), not as a general allocation style.
- project/third-party allocators that return status/null instead of throwing

#### 10.6.4 Defensive top-of-stack guards (MUST)

- Every program entry point (`main`) MUST have a last-resort catch-all guard that logs and attempts to exit gracefully.
- Every explicitly created thread entry point MUST have a last-resort catch-all guard.
  - The thread creation site MUST choose an explicit policy for what happens on an unhandled exception:
    - stop only the affected thread, or
    - request whole-application shutdown.

- A `std::terminate` handler SHOULD be installed early so that truly unhandled failures still produce a useful log.

Notes:

- This guard is defensive programming: it is not a normal control-flow mechanism.
- The guard path MUST be best-effort and low-allocation. Prefer logging with minimal formatting and then exiting.

#### 10.6.5 Catch/translate requirements (MUST)

- Catch specific exception types first when useful, then `catch ( ... )`.
- Log/report with local context.
- Return a defined failure result.

#### 10.6.6 Canonical translation wrapper (SHOULD)

Provide a single helper in each boundary layer to keep catch/translate code uniform.

```cpp
template <typename Fn>
[[nodiscard]] bool runNoThrow( Fn &&fn, const char *context ) noexcept
{
    bool ok;

    ok = false;

    try {
        ok = fn();
    } catch ( const std::bad_alloc & ) {
        logError( "Out of memory: %s", context );
    } catch ( const std::exception &e ) {
        logError( "Exception: %s: %s", context, e.what() );
    } catch ( ... ) {
        logError( "Unknown exception: %s", context );
    }

    return ok;
}
```

Notes:

- Keep the translation path low-allocation, especially for `std::bad_alloc`.
- Do not let exceptions propagate through Qt callbacks/slots/event handlers.

`std::bad_alloc` policy (SHOULD):

- Treat allocation failure like `malloc` returning `nullptr`.
- Recovery MAY be attempted only when it is bounded and documented (e.g. retry with a smaller allocation, disable a cache, reduce a limit).
- Most allocation failures are not recoverable; default behaviour SHOULD be to fail the operation safely and report.

WHY: exceptions are a non-local control flow mechanism. This policy keeps code readable and predictable while still allowing safe integration with third-party code that may throw.

## 11) Memory management and ownership

### 11.1 Prefer RAII, but keep it obvious (SHOULD)

- Prefer stack lifetimes for scope-bound resources.
- Prefer explicit ownership.

WHY: RAII prevents leaks, but overly clever RAII can hide important control flow.

### 11.1.1 Avoid hidden allocations (SHOULD)

- Avoid APIs/patterns that allocate implicitly in surprising places (type erasure, streaming formatters, repeated concatenation in loops).
- When building strings in loops, prefer `QStringBuilder`-style patterns only if they remain readable; otherwise preallocate and append.

WHY: hidden allocations complicate performance reasoning and increase OOM surface area.

### 11.1.2 Two-phase initialisation and finalisation (SHOULD)

In a no-`throw` codebase, constructors should not perform work that can fail unless failure is impossible or the failure can be handled internally.

For types that must acquire resources and can fail, prefer a two-phase pattern:

- The constructor leaves the object in a valid baseline state.
- `[[nodiscard]] bool init( ... )` performs the fallible work and returns success/failure.
- On failure, `init()` MUST leave the object in the baseline state (often by calling `finalise()` before returning).
- `void finalise()` MUST be safe to call on partially initialised objects, and SHOULD be idempotent.
- The destructor SHOULD call `finalise()` to return the object to the baseline state.

If initialisation has multiple steps, track progress explicitly (e.g. `m_initLevel` or named boolean flags) so teardown order is auditable.

WHY: this makes failure handling explicit, prevents half-initialised objects from escaping, and keeps cleanup predictable without relying on exceptions.

### 11.2 QObject ownership (MUST)

- `QObject`-derived objects MUST have a clear ownership strategy:
  - parent/child ownership, or
  - explicit owner (e.g. via `std::unique_ptr` / `QScopedPointer`) when no parent is appropriate.
- Do not mix QObject parent ownership with shared ownership.
- Cross-thread deletion MUST be done safely (often via `deleteLater()` on the owning thread's event loop).

- Storing a raw `QObject *` as a non-owning handle is allowed when lifetime is guaranteed by the owner (often the parent).
  - The owner/lifetime assumptions MUST be clear from the surrounding code or documented in the DD file.

WHY: QObject lifetime and thread affinity are core correctness concerns in Qt programs.

### 11.3 Shared ownership (SHOULD be avoided)

- Shared ownership (`QSharedPointer`, `std::shared_ptr`) SHOULD be avoided.
- If shared ownership is necessary, document the ownership graph and shutdown behaviour in the DD file.

WHY: shared ownership obscures lifetimes and creates shutdown-order bugs.

### 11.4 Minimise steady-state allocations (SHOULD)

- Prefer designs that do not need to allocate during steady-state operation.
- If a module needs working buffers, allocate them during initialisation and reuse them.
- Allocation in a documented hot path MUST be justified in the DD file.
- If a module may be used by multiple threads, shared buffers MUST be designed to avoid data races and SHOULD avoid high contention.
  - Prefer per-instance or per-thread buffers.

For example:
- When building `QString`/`QByteArray` values in loops, prefer a single allocation (`resize` + indexed fill) over incremental growth (e.g. repeated `append()`).

WHY: allocations can fail and can introduce synchronisation, fragmentation, and cache/page-fault costs.

### 11.4.1 When many small allocations are a good choice (MAY; MUST be justified)

Many small allocations MAY be the best design when at least one of these is true:

- object lifetimes are highly irregular and a pool/arena would waste large amounts of memory,
- the allocation rate is low in typical workloads,
- the code is simpler and more correct with per-object ownership,
- measurements show allocator cost is not a bottleneck.

Justification MUST be written down (usually in the module DD file).

WHY: allocation strategy is architectural; sometimes the "obvious" optimisation makes the overall system worse.

### 11.5 "Small stack buffer first" pattern (SHOULD)

- If a function usually needs a small buffer but occasionally needs more, use a small stack buffer first and allocate only on overflow.

WHY: it avoids many unnecessary allocations while still supporting worst cases.

### 11.6 Pools/arenas (MAY; MUST be justified)

- Pool/arena allocators MAY be used when they materially simplify correctness or meet performance constraints.

- If you need many same-sized allocations, a pool allocator MAY be used.
  - Fixed-size pools are often particularly effective: they can use very simple O(1) allocation/free (e.g. a free list) and can reduce fragmentation.

- If many allocations share the same lifetime, an arena/mark-and-release allocator MAY be used.

- These choices MUST be justified and documented:
  - lifetime model,
  - thread-safety,
  - failure behaviour,
  - memory-waste analysis.

- Avoid wasting memory: a few live objects sitting inside many large preallocated buffers is often worse than using the system allocator.

WHY: specialised allocators can help, but they impose lifecycle constraints and can waste memory.

### 11.7 Locality over big-O folklore (SHOULD)

- Prefer contiguous storage for iteration-heavy workloads.
- Avoid linked lists in performance-sensitive iteration unless there is a strong reason.

WHY: pointer chasing is often far slower than contiguous access; constants matter.

### 11.7.1 Avoid per-element wrapper allocations (SHOULD)

- Where practical, collections SHOULD avoid allocating a separate wrapper object per element (node-based containers, heap-allocated list nodes, etc.).
- Prefer contiguous storage (`QVector`, `QByteArray`, `QString`) for iteration-heavy workloads.
- If stable addresses are required, prefer explicit indirection (indices/handles) or a documented pool rather than implicit node allocation.

WHY: fewer allocations improves robustness and often improves cache locality.

## 12) Qt-specific correctness rules

### 12.1 Signals/slots connections (MUST)

- Prefer the function-pointer `connect` syntax.
- Provide a context object for lambda connections so they auto-disconnect on destruction.
- For cross-thread connections, explicitly specify `Qt::QueuedConnection`.

WHY: explicit connection style is type-checked; context objects prevent use-after-free; explicit queued connections make thread hops obvious.

### 12.2 Thread affinity (MUST)

- Do not access a `QObject` from a thread other than the one it lives in unless the access is explicitly documented as thread-safe.
- UI objects MUST only be accessed from the UI thread.

WHY: Qt's object model is thread-affine; violating it causes hard-to-debug races and crashes.

### 12.3 Do not block the UI thread (MUST)

- The UI thread MUST NOT perform long-running or blocking work (file I/O, network waits, heavy computation).
- Long work MUST be moved to a worker thread or made asynchronous.

WHY: blocking the UI event loop creates freezes and re-entrancy hazards.

### 12.4 QObject lifetime safety (MUST)

- Do not store raw owning pointers to `QObject`.
- Use parent/child ownership or a single explicit owner.
- For non-owning references that must become null on deletion, prefer `QPointer`.

- Non-owning raw `QObject *` members are allowed when ownership is handled elsewhere (typically via parent/child).
  - Do not manually `delete` such members.

WHY: Qt object graphs are dynamic; guard pointers prevent use-after-free.

### 12.4.1 QObject class design (MUST)

- Only derive from `QObject` when you need Qt's object model (signals/slots, properties, event handling).
- `QObject`-derived types MUST NOT be copyable or movable.
- Constructors for `QObject`-derived types SHOULD accept an optional `QObject *parent` and pass it to the base.

WHY: `QObject` is an identity-bearing, thread-affine type; copying/moving it is nonsensical and leads to bugs.

### 12.5 Avoid contextless connects (MUST)

- The functor/lambda overloads of `QObject::connect` that omit a context object MUST NOT be used.
- For lambda slots, always use the overload that takes a context object (often the receiver, such as `this`).
- Receiver/slot overloads that specify a receiver object are allowed; the receiver provides the lifetime context for the connection.

WHY: contextless connections make lifetime management implicit and are a frequent source of use-after-free.

### 12.6 Worker-thread pattern (SHOULD)

- Prefer the "worker object moved to a thread" pattern (`QObject` worker + `moveToThread`) over subclassing `QThread`.
- Thread entry points MUST catch and translate any third-party exceptions (Section 10.6).
- Thread entry points MUST also have a last-resort catch-all guard (Section 10.6.4).

WHY: the worker-object pattern composes better with signals/slots and keeps thread-affinity explicit.

### 12.7 Paths and files (MUST)

- Prefer Qt file and path facilities (`QFile`, `QSaveFile`, `QDir`, `QFileInfo`) over platform APIs.
- Be explicit about text encoding when reading/writing text files.

- When writing persistent/user-visible files, prefer `QSaveFile` and commit atomically.
- Normalise and join paths using Qt utilities:
  - prefer `QDir::cleanPath` and `QDir::filePath` over manual separator munging.

WHY: Qt handles Unicode paths correctly across Windows/Linux and provides consistent behaviour.

### 12.8 Exceptions must not cross Qt boundaries (MUST)

- Qt callbacks/slots/event handlers MUST NOT allow exceptions to propagate through Qt.
- If you call potentially-throwing code from a slot/callback, catch and translate inside the slot/callback.

WHY: exceptions unwinding through Qt is not a supported control-flow model and commonly ends in termination or corruption.

### 12.9 Qt Quick (QML) / C++ boundary guideline (SHOULD)

For applications that use Qt Quick (QML) with C++ back-end code, prefer a strict boundary:

- QML owns presentation: layout, control ordering, styling, animations, and user-visible copy.
- C++ owns state and services: networking, capture, persistence, and other fallible/long-running work.
- QML should not perform IO or embed business logic; it should bind to C++ properties/models and invoke explicit commands.
- C++ should not encode UI layout/styling policy; instead expose the smallest useful surface (properties, signals, invokable commands, and list models).

WHY: this keeps UI iteration fast (QML-only edits for layout/styling) whilst keeping stateful and fallible work testable and reviewable in C++.

## 13) Unit tests (QtTest)

### 13.1 Scope (MUST)

- All public module interfaces MUST be thoroughly unit tested.
- Tests MUST attempt to break the module while respecting the public interface.
- Tests MUST treat the DD file Public interface section as the contract.

### 13.2 QtTest usage (SHOULD)

- Use QtTest for unit tests.
- Prefer one test target per module.
- Test functions SHOULD be small and named descriptively.

- Tests SHOULD use the QtTest runner interface.
- Do not implement additional custom CLI parsing/flags in unit test binaries.
  - If you need to list, filter, select, or repeat tests, use QtTest's built-in runner options.
  - If a project-wide wrapper is needed (CI integration, reporting), implement it once at the outer runner/script level, not per-test.

WHY: QtTest integrates with Qt types and provides a stable runner.

Exception (MAY): test code may use QtTest macros that short-circuit a test function (e.g. `QVERIFY`) even though production code forbids early returns.

### 13.3 Test-driven development (MUST)

For any change that adds or changes externally observable behaviour, you MUST do the work in this order:

- Update the module DD file (Public interface section) to describe the expected behaviour.
- Write the unit test(s) that enforce that behaviour.
- Implement the minimal change needed to make the new tests pass.
- Refactor only while all tests remain green.

Bug fixes (MUST):

- A bug fix MUST include a regression test that fails before the fix and passes after.

AI + human workflow (MUST):

- Before writing implementation code, propose the new/changed test cases (names + concrete inputs/expected outputs) to the user/reviewer and incorporate their corrections.
- If the task exposes a load-bearing design element, do not proceed straight into implementation as though the design were already settled.
  - Pause and ask the user/reviewer to choose among the concrete design options first.
  - Record the decision in the DD file before implementation.
- Do not let implementation silently hard-code a load-bearing design element that has not been explicitly approved.

If you learn something new while implementing (MUST):

- If implementation work reveals a load-bearing design element that was not previously surfaced or a missing or wrong contract detail, loop back.
  - Stop and ask before continuing.
  - Once a decision has been made update the DD file first, then update tests, then continue implementation.

WHY: writing the contract and tests first makes "done" concrete and catches misunderstandings early.

Exceptions (MAY; MUST be justified):

- Purely mechanical changes with no behaviour change (formatting, renames, comment/doc fixes).
- Work that only enables testing (build harness, runner plumbing); write the tests as soon as the harness can express them.

### 13.4 Randomised/generative tests (MAY; MUST be reproducible)

- Randomised tests MUST be reproducible.
- Use a deterministic PRNG with a fixed default seed.
- The seed SHOULD be overrideable (environment variable or test argument).
- On failure, tests SHOULD log the seed and case index so the failure can be replayed.

WHY: random exploration finds edge cases, but only reproducibility makes failures actionable.

### 13.5 Testing mindset (SHOULD)

- Write tests with unyielding adversarial intent. Adopt a relentless resolve to break the module (like a hostile agent of a foreign state) while staying within the documented contract.
- Look for what the implementer did not think of: boundary values, empty and singleton cases, very large cases, repeated operations, unusual sequences of calls, and combinations of edge cases.
- Where applicable, use randomised or generative tests to explore the input space (Section 13.4).

WHY: most defects live at boundaries and in sequences; a hostile mindset finds them early.

### 13.6 What does not count as breaking the module (MUST)

- If the API requires a valid object pointer, passing `nullptr` and crashing is not a "successful break".
- Calling functions with arguments the contract forbids and observing undefined behaviour is not a "successful break".
- Violating documented thread-affinity rules (e.g. touching a `QObject` from the wrong thread) and observing a crash is not a "successful break".

WHY: tests are about catching design bugs and edge cases within the contract, not about inducing undefined behaviour outside it.

## 14) AI-operation policies

This section is written for AI agents and human reviewers. It defines the expected working process for making changes safely and reviewably.

### 14.1 When starting work (MUST)

- One task at a time. Do not start unrelated work or switch tasks without explicit user direction.
- If requirements are unclear or a question could change behaviour/API/architecture, ask before implementing.
- Ask early (before writing code), not after discovering ambiguity mid-implementation.
- Keep questions focused: 1-4 targeted questions with meaningful choices.
- For design/architecture questions, AI agents MUST use a structured question format with concrete options (if an interactive question tool exists, use it).
- Treat the module DD file Public interface section as the canonical contract.
- For behaviour changes and bug fixes, update the DD file first, then write tests, then write code.
- If a task exposes a load-bearing design element, pause before implementation and ask the user/reviewer what to do.
  - Do not silently choose a protocol shape, state machine shape, ownership model, public API shape, or concurrency model when that choice would materially constrain later work.
- If implementation reveals a load-bearing design element that was not visible at the start, stop and ask before continuing.

WHY: this keeps changes reviewable, prevents accidental contract drift, and prevents AI-generated code from hard-coding architectural decisions that the human has not actually chosen.

### 14.2 If you are adding a new module (MUST)

- Follow Section 14.1.
- Create the DD file early and keep it current.
- Before substantial implementation begins, make the module boundary explicit:
  - purpose,
  - non-goals,
  - public interface,
  - ownership/lifetime expectations,
  - thread-affinity/thread-safety expectations,
  - platform boundary if any.
- If the module introduces a load-bearing design element, get explicit user/reviewer approval before filling in the implementation.
- Ensure the DD file includes a Requirements section.
- Add unit tests for the public interface.

WHY: new modules set long-term patterns; getting contracts, boundaries, approval, and tests right up front prevents future debt.

### 14.3 If you are modifying an existing module (MUST)

- Follow Section 14.1.
- Keep the module Requirements accurate; update them if you change dependencies, platform behaviour, or exception-handling boundaries.
- Update/add unit tests for any public-interface change.
- If the change affects a load-bearing design element, obtain explicit user/reviewer approval first and update the DD file before treating implementation as complete.
- Do not treat an implementation-discovered design change as "just part of the patch"; surface it explicitly.

WHY: most regressions come from unrecorded contract changes, and most long-term comprehension loss comes from design changes being smuggled in as implementation detail.

### 14.4 Review and commit procedure (MUST)

When you believe the task is complete and ready to commit, follow this procedure:

1. Pre-approval verification (MUST). Check that:
   - Public API is documented in `*-dd.txt`.
   - Requirements section is present and accurate.
   - DD file and unit tests agree on externally observable behaviour.
   - Ownership/lifetime/thread-safety rules are stated where they matter.
   - No reserved identifiers were introduced.
   - No platform-specific code leaked through public headers.
   - No exceptions can escape module/Qt boundaries.
   - No forbidden facilities were introduced (`throw`, iostream-style logging, template machinery).
   - Unit tests cover both success and failure paths.
   - Run the relevant tests (and any build/lint steps that exist).

2. Request review start (MUST, no commit yet).
   - Provide:
     - the changed file list,
     - how to test,
     - and a one-sentence statement of the patch purpose.
   - Do not dump full diffs by default.
   - Ask whether the reviewer agrees to start review and proceed to the patch walkthrough.
   - Review start means the reviewer agrees the change set is ready to be walked through in detail next.
   - It does NOT authorise a commit.
   - From time to time, the user may also request review start. Please don't be too particular about language in this case, phrases like 'we are ready for review' should be taken as a prompt to start reviewing.

3. Change disclosure rule (MUST).
   - If any changes are made after review start (even trivial ones), call them out explicitly before the patch walkthrough.

4. Patch walkthrough (MUST for non-trivial changes).
   - After review start, present a patch walkthrough.
   - The walkthrough MUST cover:
     - the changed files,
     - the purpose of each changed file,
     - key invariants,
     - risky areas or edge cases,
     - and consequences future maintainers need to know.

5. Post-walkthrough invalidation rule (MUST).
   - After the patch walkthrough, if any non-trivial change is made, review is no longer valid; go back to Step 1.
   - Only trivial changes (as defined in Terms) may be made without restarting review.
   - If a patch contains any non-trivial change, treat the entire patch as non-trivial.
   - If any changes are made after the walkthrough (even trivial ones), call them out explicitly before the human-written commit message.
   - If unsure, treat the change as non-trivial.

6. Human-written commit message (MUST for non-trivial changes).
   - After the patch walkthrough, the human reviewer MUST write the commit message by hand.
   - The human-written commit message SHOULD state:
     - what changed,
     - why it changed,
     - any consequences future maintainers need to know, and
     - a checklist of manual tests where applicable.

7. AI review of the human-written commit message (MUST for non-trivial changes).
   - The AI MUST review the human-written commit message for:
     - accuracy,
     - completeness, and
     - appropriate emphasis.
   - If the commit message is misleading, materially incomplete, or there is any mismatch between the commit message and the actual patch, reject it with the reason stated. Go back to Step 6.

8. Prepare the commit (MUST).
   - Decide exactly which files belong in the commit.
   - Do not add new/untracked files without explicit approval.
   - Present the final commit message and final file list.

9. Final confirmation to commit (MUST).
   - Ask for explicit final confirmation to create the commit.

10. Commit (MUST).
   - Only after confirmation: stage the agreed files, create the commit, and verify success.

WHY: this checklist reduces contract drift, scope drift, and the risk that code is approved without being understood.

### 14.5 Trivial-change exceptions (MAY)

For changes that are trivial:

- the patch walkthrough MAY be omitted,
- the human-written commit message MAY be brief,
- and the AI review of the commit message MAY be correspondingly brief.

This exception MUST NOT be used for any change that alters behaviour, load-bearing design elements, public contracts, tests in a meaningful way, ownership/lifetime rules, portability/build assumptions, or file structure.

WHY: the workflow should be strict where understanding matters and lightweight where it does not.
