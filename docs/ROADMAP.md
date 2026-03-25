# Native Conferencing App — Development Roadmap

This document is the execution plan for the native conferencing app described in [`OVERVIEW.md`](./OVERVIEW.md).

It is deliberately broken into small tasks that are realistic to complete in one sitting, with or without AI assistance.

## How to use this roadmap

- Prefer **small coherent changes**.
- Do not spread one task across many unrelated subsystems unless the task explicitly requires that.
- When a task exposes a high-impact design choice, pause and write a short decision note before hard-coding it.
  - If you are an AI agent, always ask the user what to do.
- If a task finishes with uncertainty, record the uncertainty explicitly rather than hiding it.
- UI copy note: roadmap wording is often technical. User-facing UI labels should be user-intent phrased and non-technical; use tooltips for optional power-user detail.

## Definition of done for roadmap tasks

Unless a task says otherwise, a task is “done” only if:
- the code builds
- the behaviour can be demonstrated or observed
- obvious failure paths do not crash the app
- any notable decision is written down
- QML UI tests (where applicable) exercise the changed UI and emit no Qt/QML warnings

## Fault tolerance

This application should be able to behave even when life gets messy. Code should be written whilst thinking about what could go wrong. For example:

- somebody’s camera vanishing and reappearing
- a participant reconnecting mid-session
- a screen share being stopped and restarted
- audio drift after half an hour
- one machine being slower than expected
- jitter spikes or packet loss
- two things happening in the wrong order
- a source being changed while OBS is already consuming it
- Windows and Linux behaving slightly differently
- the host disappears

Never write for the happy path only.

---

## Phase 0 — Foundation and project skeleton

### Task 0.1 — Create repository structure and project skeleton
**Goal:** establish the initial Qt 6 C++ project layout.

**Expected output:**
- repository structure
- build files
- source folders
- placeholder application entry point

**Completion criteria:**
- project builds successfully on at least one development machine
- app launches and opens a basic window
- directory structure is documented

### Task 0.2 — Define coding conventions and architecture notes
**Goal:** create a lightweight house style for the project.

**Expected output:**
- coding conventions document
- architecture notes document
- naming guidance for classes/modules

**Completion criteria:**
- documents exist in repo
- key conventions are clear enough for human and AI contributors
- module names are stable enough to start work

### Task 0.3 — Define initial module boundaries
**Goal:** document the initial major classes/services.

**Expected output:**
- short design note listing proposed modules, for example:
  - `AppSupervisor`
  - `SharedTypes`
  - `RoomSession`
  - `RelayServer`
  - `SourceCatalogue`
  - `MediaCapture`
  - `MusicPlaylistSource`
  - `AudioGraph`
  - `VideoGraph`
  - `VideoRenderer`
  - `TextChat`
  - `HttpMediaServer`
  - `Diagnostics`
  - `RendezvousClient` (placeholder only, optional extension)

**Completion criteria:**
- each module has a one-paragraph responsibility summary
- obvious overlaps/conflicts are identified
- at least one proposed dependency direction is recorded

### Task 0.4 — Decide terminology for “source” vs “track”
**Goal:** avoid confusion in docs and code.

**Completion criteria:**
- a short decision note exists
- the code-facing term is chosen intentionally
- the OVERVIEW explains the mapping clearly

---

## Phase 1 — Core application shell and role enablement

### Task 1.1 — Build the main application shell
**Goal:** create the main window and app lifecycle structure.

**Completion criteria:**
- app launches cleanly
- main window exists
- shutdown is clean
- placeholder areas for participant list, chat, diagnostics, role controls, and source lists exist

### Task 1.2 — Add role enablement UI
**Goal:** support:
- join room
- host room
- enable Browser Export for selected sources

**Completion criteria:**
- user can toggle/select these modes in UI
- current local role state is visible
- UI state changes do not crash the app

### Task 1.3 — Add settings persistence skeleton
**Goal:** persist basic local configuration.

**Completion criteria:**
- app remembers at least one simple setting across restarts
- settings storage location is documented
- design leaves room for future device/network/security settings

### Task 1.4 — Add Preferences UI (settings + local device monitor)
**Goal:** provide a home for user preferences and a local "what does my machine see?" monitor for debugging.

**Discussion guidance:**
This should cover:
- theme/style selection
- accessibility options (e.g. UI scale)
- default values (e.g. display name, preferred webcam)
- a local monitor view that can enumerate and refresh:
  - cameras, microphones
  - screens and windows
  - audio output devices (where supported)
  - any other locally available inputs/outputs the app can use as sinks/sources

**Victorian mode follow-ups (styling):**
- Completed: bundle Victorian fonts and wire them into a typeface preset:
  - body: Libre Caslon Text
  - headings: Libre Caslon Display
- Deferred: optional subtle paper-grain overlay (no faux ageing; keep it clean and low-contrast).
  - Reconsider when the app has more real content-dense screens and we can validate readability/contrast across common DPI/scaling.

**Accessibility follow-ups (motion):**
- Deferred: reduced-motion preference.
  - Reconsider once we introduce non-trivial animations/transitions (beyond simple hover state changes), so the preference has immediate effect.
  - Regardless of this deferral, any future authored motion must degrade to instant state changes when reduced-motion is enabled.

**Completion criteria:**
- preferences entry point exists in the app shell UI
- at least one preference is persisted and restored (beyond window placement)
- device monitor can refresh without restart and does not require "starting capture" to be useful
- obvious failure paths (device disappears, permission denied) are surfaced without crashing

### Task 1.5 — Add landing-page popups (Scheduling + Support)
**Goal:** add two small optional buttons on the landing screen, each opening its own popup:
- scheduling: placeholder copy such as "Need help scheduling?" linking to the schedule coordination tool website
- support: placeholder copy such as "Support the creator" linking to Twitch and (optionally) a tip jar

**Completion criteria:**
- landing screen has two buttons, each opening a separate overlay popup (no layout reflow/oscillation)
- links open externally (platform-default browser)
- popups are clearly optional and only appear on the landing screen
- QML UI smoke tests exercise open/close and emit no warnings

---

## Phase 2 — Early architecture decisions that affect everything later

### Task 2.1 — Write a transport and protocol ADR
**Goal:** make the first major networking decision before too much code is written.

**Discussion guidance:**
This decision should explicitly consider:
- separate control and media channels or not
- reliable channel choice
- encrypted transport approach
- whether future NAT traversal is a serious goal
- whether that pushes the design towards UDP/QUIC or a similar transport family

**Completion criteria:**
- a short ADR is written
- chosen direction is explicit
- known risks are recorded
- the effect on a future rendezvous/NAT-traversal extension is stated clearly

### Task 2.2 — Write an initial trust-model ADR
**Goal:** decide how clients trust the relay and any future rendezvous service.

**Completion criteria:**
- ADR written
- trust model is explicit
- likely user-facing consequences are recorded

### Task 2.3 — Write a rendezvous/NAT-traversal scope note
**Goal:** keep discovery-only and full NAT-traversal work clearly separated.

**Completion criteria:**
- note explains the difference between room discovery and actual reachability
- note states whether NAT traversal is a real medium-term goal or merely a possible future idea
- note records what early design choices this affects

---

## Phase 3 — Local devices and local preview

### Task 3.1 — Enumerate local media devices
**Goal:** list available cameras, microphones, screens, and windows as applicable.

**Completion criteria:**
- app can show available local sources in UI
- no selected source is required yet
- refresh/re-enumeration works without restart

**Decision note:** this is where Qt capture APIs should be explored concretely.

### Task 3.2 — Start and stop local camera preview
**Goal:** support selecting a local camera and previewing it.

**Completion criteria:**
- selected camera preview appears in-app
- user can stop/start preview
- switching cameras works or is at least handled gracefully

### Task 3.3 — Start and stop microphone capture state
**Goal:** establish microphone acquisition pipeline basics.

**Completion criteria:**
- microphone can be selected
- app can indicate active microphone input state
- mute/unmute state is represented in UI

### Task 3.4 — Start and stop screen/window capture preview
**Goal:** support selecting a screen or window and previewing it locally.

**Completion criteria:**
- user can choose a screen or window source
- preview appears in-app
- source switching is possible or gracefully denied with clear messaging

### Task 3.5 — Define local source model in code
**Goal:** represent local published sources explicitly.

**Completion criteria:**
- local sources have IDs, types, and state
- webcam, microphone, screen/window, and future music are representable independently
- code uses explicit source objects instead of ad hoc flags

---

## Phase 4 — Local rendering and independent windows

### Task 4.1 — Implement one local render window for a test video source
**Goal:** create a native render window abstraction.

**Completion criteria:**
- a test/local preview source can appear in a separate top-level window
- the window can be opened and closed safely
- the abstraction is reusable

### Task 4.2 — Generalise window binding for arbitrary video sources
**Goal:** make render windows source-driven.

**Completion criteria:**
- any chosen video source can be bound to a separate window
- multiple windows can exist simultaneously
- window lifecycle does not leak or crash

### Task 4.3 — Document window/render binding design
**Goal:** record how render windows relate to source objects.

**Completion criteria:**
- short design note exists
- binding model is clear enough for future remote-source work

---

## Phase 5 — Minimal encrypted control path

### Task 5.1 — Implement minimal encrypted control connection
**Goal:** create a basic client-to-relay control connection.

**Completion criteria:**
- relay can listen
- client can connect by IP/port
- connection is encrypted
- a simple hello/ack exchange works

### Task 5.2 — Add participant join/leave signalling
**Goal:** establish room membership signalling.

**Completion criteria:**
- relay tracks connected participants
- clients receive basic join/leave events
- UI reflects membership changes

### Task 5.3 — Add structured message types for the control area
**Goal:** avoid ad hoc strings/protocol drift early.

**Completion criteria:**
- message envelope format is defined
- at least a few message types exist:
  - `hello`
  - `join`
  - `leave`
  - `error`
  - `chat_text`
  - `publish_source`
  - `subscribe_source`
- serialisation/deserialisation is implemented and documented

### Task 5.4 — Add heartbeats/timeouts
**Goal:** detect silent failure and dead peers early.

**Completion criteria:**
- relay can detect dead clients
- clients can detect lost relay connection
- timeout behaviour is visible in diagnostics and does not crash the app

---

## Phase 6 — Relay and room state

### Task 6.1 — Implement minimal relay host mode
**Goal:** enable one app instance to host a room.

**Completion criteria:**
- host can start a relay listener
- remote client can connect
- host UI shows itself as relay-capable

### Task 6.2 — Implement participant registry on relay
**Goal:** track participants authoritatively.

**Completion criteria:**
- relay maintains participant records
- records are created and cleaned up correctly
- disconnect cleanup works

### Task 6.3 — Implement source registry on relay
**Goal:** relay should know which sources are published.

**Completion criteria:**
- clients can announce sources
- relay stores metadata for those sources
- source add/remove events propagate to clients

### Task 6.4 — Implement subscription model
**Goal:** define which clients receive which sources.

**Completion criteria:**
- clients can subscribe/unsubscribe to sources
- relay stores subscription mappings
- mappings are visible in diagnostics/logs

**Decision note:** keep policy simple first; correctness matters more than cleverness.

---

## Phase 7 — Media transport

### Task 7.1 — Implement outbound test media transport for one video source
**Goal:** send one simple video source from client to relay.

**Completion criteria:**
- local client can publish one video source
- relay receives identifiable media units
- diagnostics show media traffic

### Task 7.2 — Implement relay forwarding for one video source
**Goal:** forward one published source to one subscriber.

**Completion criteria:**
- subscriber receives forwarded media from relay
- forwarding uses subscription state
- disconnect/reconnect edge cases are at least visible in logs

### Task 7.3 — Implement remote decode/reconstruction path for one video source
**Goal:** turn forwarded media back into something renderable.

**Completion criteria:**
- one remote source can be displayed on another client
- end-to-end path works:
  - capture
  - encode/package
  - relay
  - receive
  - decode
  - render

### Task 7.4 — Extend media path to one microphone source
**Goal:** prove audio is viable in the architecture.

**Completion criteria:**
- one microphone source can be published and received
- basic mute/unmute works
- audio path is documented

### Task 7.5 — Add jitter/ordering instrumentation
**Goal:** make media timing faults visible early.

**Completion criteria:**
- basic timing/jitter stats are logged or displayed
- out-of-order or delayed data is visible in diagnostics
- known rough edges are documented honestly

---

## Phase 8 — Multi-source support

### Task 8.1 — Support multiple local published sources per participant
**Goal:** one participant can publish more than one source.

**Completion criteria:**
- at least camera + microphone + one extra video source can coexist logically
- source registry and UI support this without confusion

### Task 8.2 — Support remote subscription to multiple sources
**Goal:** subscriber can receive multiple independent sources.

**Completion criteria:**
- one subscriber can receive at least two video sources from the same remote participant
- sources remain distinct in UI and rendering

### Task 8.3 — Bind each remote video source to its own native window
**Goal:** satisfy a key production requirement.

**Completion criteria:**
- multiple remote video sources can each appear in separate windows
- windows are stable enough for local capture/testing
- source/window association is visible somewhere in UI or diagnostics

---

## Phase 9 — Text chat and backstage communication

### Task 9.1 — Implement room text chat messages on control channel
**Goal:** add the basic fallback/backstage chat.

**Completion criteria:**
- one participant can send a text message
- relay forwards it to others
- chat appears in UI

### Task 9.2 — Add system/status messages to chat or adjacent panel
**Goal:** improve operational usefulness.

**Completion criteria:**
- system notices such as join/leave or device errors can be surfaced
- distinction between user chat and system messages is clear

### Task 9.3 — Optional small in-memory history buffer on relay
**Goal:** support recent-context recovery without committing to persistence.

**Completion criteria:**
- history buffer size is bounded
- newly connected participants can receive recent messages if enabled
- history can be disabled

---

## Phase 10 — Browser Export (localhost first)

### Task 10.1 — Implement localhost HTTP server skeleton
**Goal:** create the local Browser Export service.

**Completion criteria:**
- service binds to localhost only
- a test page is reachable
- service can be enabled/disabled in the app

### Task 10.2 — Implement one `/solo/<source-id>` endpoint
**Goal:** expose one locally available source through the bridge.

**Completion criteria:**
- a known local/remote source can be selected for export
- HTTP endpoint serves a page representing only that source
- endpoint works from the local machine

### Task 10.3 — Verify OBS Browser Source can use the localhost endpoint
**Goal:** prove the bridge is useful in reality.

**Completion criteria:**
- OBS can load the endpoint successfully
- exported source is visible in OBS
- documentation records any constraints or quirks

### Task 10.4 — Add UI for managing OBS exports
**Goal:** make the feature usable.

**Completion criteria:**
- user can see which sources are exportable
- user can enable/disable export per source
- visible localhost URL/path is shown

---

## Phase 11 — Music source (v1 shape)

### Task 11.1 — Add local music playback service skeleton
**Goal:** create an in-app music player rather than relying on external app-audio capture.

**Completion criteria:**
- a local audio file can be loaded
- play/pause/stop works
- current state is visible in UI

### Task 11.2 — Add looping and simple queue support
**Goal:** support the immediate use case that triggered this feature.

**Completion criteria:**
- a single item can loop
- at least a simple queue or playlist exists
- behaviour is documented

### Task 11.3 — Publish music as a separate audio source
**Goal:** make music part of the conference architecture cleanly.

**Completion criteria:**
- music can be published without being the relay host
- music is distinct from microphone audio
- remote clients can receive it as a separate source

### Task 11.4 — Add per-client local mute/volume control for music
**Goal:** let participants opt out individually.

**Completion criteria:**
- each client can mute or adjust music locally
- muting music does not mute speech
- UI reflects current music-audio state clearly

### Task 11.5 — Verify streamer/producer workflow with music + OBS
**Goal:** ensure music can reach both participants and OBS.

**Completion criteria:**
- one client can publish music
- another client can subscribe to it and expose it to OBS
- relay host does not need to be the DJ or the streamer

---

## Phase 12 — Security hardening and trust UX

### Task 12.1 — Implement certificate/fingerprint or equivalent trust UX
**Goal:** make encrypted connections usable by real users.

**Completion criteria:**
- client can verify trust material
- user-facing trust prompt or trust storage exists
- trust failures are handled clearly

### Task 12.2 — Review Browser Export exposure safety
**Goal:** ensure Browser Export cannot accidentally become an internet-exposed service.

**Completion criteria:**
- localhost-only binding is enforced or clearly justified otherwise
- logs/UI make the binding scope visible
- document the security boundary

### Task 12.3 — Add basic auth/token protection hooks for later LAN bridge work
**Goal:** avoid painting future trusted-LAN output into a corner.

**Completion criteria:**
- there is a documented hook point for per-endpoint tokens or similar access control
- no unnecessary complexity is forced into localhost mode

---

## Phase 13 — Trusted-LAN Browser Export extension

### Task 13.1 — Write a LAN bridge scope note
**Goal:** define what “trusted LAN” means for this project.

**Completion criteria:**
- note defines allowed bind modes
- note states default remains localhost only
- note records the main security risks

### Task 13.2 — Allow HTTP bridge to bind to a chosen LAN interface
**Goal:** support two-PC streaming setups on a trusted network.

**Completion criteria:**
- user can opt into LAN binding explicitly
- app does not default to all interfaces silently
- current bind mode is visible in UI/diagnostics

### Task 13.3 — Verify second-PC OBS workflow over LAN
**Goal:** prove the feature solves the intended use case.

**Completion criteria:**
- conference PC can expose a source over LAN
- second PC can load it in OBS Browser Source
- constraints and risks are documented

---

## Phase 14 — Rendezvous service (discovery-only first)

### Task 14.1 — Write rendezvous service scope note
**Goal:** define the tiny public service honestly.

**Completion criteria:**
- note states clearly that discovery is not the same as reachability
- note defines the initial non-relaying role of the service
- note records resource assumptions for a cheap headless Linux VPS

### Task 14.2 — Define room-code and password model
**Goal:** formalise the proposed UX.

**Discussion guidance:**
Consider a registration-plate-style room code (letters + digits) with a fixed per-position character class so the app can canonicalise lookalike characters on input.
- input: case-insensitive; ignore separators (spaces/hyphens)
- canonicalisation: convert lookalikes where the position type is known (e.g. `O` -> `0` in digit positions)
- alphabet: optionally forbid overly ambiguous glyphs to reduce errors across typefaces
- display: show the canonical form with grouping for memorability

**Completion criteria:**
- room code format is specified
- password handling is specified
- expiry/cleanup rules are specified
- brute-force/rate-limit concerns are noted

### Task 14.3 — Implement bare-bones rendezvous server prototype
**Goal:** support publishing and joining ephemeral password-protected rooms.

**Completion criteria:**
- host can publish a room record
- server assigns a room code in the chosen format
- joining client can look up the room by code and password
- no media is relayed by this service

### Task 14.4 — Add rendezvous client support in the app
**Goal:** allow room discovery without manual IP/port entry.

**Completion criteria:**
- host can publish or withdraw a room
- joining client can enter room code and password
- app can obtain the connection hint needed for the next step

---

## Phase 15 — NAT-traversal investigation and optional implementation

### Task 15.1 — Write a NAT-traversal ADR
**Goal:** decide whether this feature is worth doing beyond theory.

**Completion criteria:**
- ADR written
- expected benefits are stated
- expected complexity is stated
- the relationship to the already-chosen transport is recorded

### Task 15.2 — Build a small reachability experiment harness
**Goal:** test real behaviour rather than theorising forever.

**Completion criteria:**
- harness can collect useful endpoint/reachability data
- logs are good enough to understand what happened
- results are written down

### Task 15.3 — Decide go/no-go on NAT traversal
**Goal:** avoid dragging the project indefinitely into half-built networking complexity.

**Completion criteria:**
- decision is written down
- roadmap is updated accordingly
- if “no”, the rendezvous service remains discovery-only
- if “yes”, the next concrete implementation tasks are listed

### Task 15.4 — Optional first NAT-traversal prototype
**Goal:** attempt the smallest viable implementation if the previous task says “yes”.

**Completion criteria:**
- prototype exists
- behaviour is measured, not guessed
- failure modes are documented honestly

---

## Phase 16 — Reliability and diagnostics

### Task 16.1 — Add structured logging
**Goal:** improve debuggability for humans and AI agents.

**Completion criteria:**
- logs are timestamped and categorised
- network/media/UI subsystems can log meaningfully
- logs are accessible in a predictable location or panel

### Task 16.2 — Add connection and media stats panel
**Goal:** make faults diagnosable without reading raw logs.

**Completion criteria:**
- app can show at least basic stats:
  - connected/disconnected
  - participant count
  - source count
  - sent/received bytes or packets
  - jitter or timing counters where available
- panel updates live

### Task 16.3 — Handle disconnects gracefully
**Goal:** avoid catastrophic state corruption.

**Completion criteria:**
- disconnecting a client cleans up sources/windows/subscriptions
- reconnect failures are surfaced clearly
- app does not crash on common disconnect paths

### Task 16.4 — Exercise messy-path scenarios deliberately
**Goal:** make fault tolerance real rather than aspirational.

**Completion criteria:**
- at least several messy-path scenarios are tested deliberately
- observed failures are written down
- roadmap or issue list is updated from evidence

---

## Phase 17 — Packaging and real-world use

### Task 17.1 — Decide Windows build and packaging strategy
**Goal:** formalise the shipping path.

**Completion criteria:**
- decision recorded:
  - compiler/toolchain
  - deployment strategy
  - installer or portable bundle
- list of runtime dependencies is written down

### Task 17.2 — Produce a Windows test build for non-developer users
**Goal:** validate real-user install/run experience.

**Completion criteria:**
- non-developer can install or unpack and run the app
- missing dependency issues are identified
- setup instructions are written

### Task 17.3 — Produce a Linux test build plan
**Goal:** define how Linux users will run it.

**Completion criteria:**
- initial packaging/distribution plan is documented
- known platform-specific caveats are listed

### Task 17.4 — Produce a minimal rendezvous-server deployment note
**Goal:** ensure the optional public mini-server is realistic to host.

**Completion criteria:**
- note covers expected OS, runtime, and basic service supervision
- note states that no media relay is involved
- note records any firewall/port requirements

---

## Phase 18 — Stabilisation and MVP review

### Task 18.1 — Define MVP checklist
**Goal:** explicitly decide when the application is “MVP complete”.

**Suggested MVP scope:**
- host can run relay
- clients can connect by IP/port
- webcam and microphone work
- at least one extra video source works
- basic room chat works
- each remote video source can be shown separately
- Browser Export can expose selected sources
- encryption is enabled for non-local traffic
- music can be published as a separate audio source

**Completion criteria:**
- checklist exists in repo
- checklist is agreed or intentionally revised

### Task 18.2 — Run end-to-end group test
**Goal:** validate the whole flow with real users.

**Completion criteria:**
- at least a small multi-person session completes
- issues are logged
- blockers are prioritised

### Task 18.3 — Write post-test architecture review
**Goal:** update the design based on reality.

**Completion criteria:**
- document records what worked
- document records what failed
- roadmap is updated based on evidence

---

## Guidance for AI agents working on this project

When working on this project, the AI agent should:

1. **Prefer small coherent changes.**
   - Avoid sprawling multi-subsystem rewrites in one pass.

2. **Keep the source model central.**
   - If a proposed change hides or muddles source identity, reconsider it.

3. **Preserve role decoupling.**
   - Relay hosting and Browser Export are optional capabilities, not separate products.

4. **Record decisions.**
   - If a meaningful choice is made, write it down.

5. **Do not overdesign future features too early.**
   - Design for extension, but do not build everything now.

6. **Surface uncertainty honestly.**
   - If a transport, security, or media choice is unclear, flag it explicitly.

7. **Discuss high-impact choices before hard-coding them.**
   - Especially:
     - transport model
     - NAT-traversal implications
     - encryption model
     - trust model
     - media pipeline strategy
     - packaging approach

8. **Bias towards debuggability.**
   - A less clever system with strong diagnostics is better than an elegant black box.

9. **Never write for the happy path only.**
   - Fault tolerance is part of the job, not a later luxury.

---

## Open questions that remain unresolved

These are deliberate future decision points.

- Exact media transport architecture
- Exact codec choices
- Exact encryption/trust implementation details
- Exact serialisation format for control messages
- Whether the internal primary term should be `Source` or `Track`
- Whether Browser Export pages are rendered from decoded local state or another local media surface
- Windows installer technology
- Linux packaging format
- Whether relay can optionally be headless later
- Whether direct/private chat is needed
- Whether file transfer is worth supporting later
- Whether NAT traversal is worth the complexity for this group
- Whether the rendezvous service should remain discovery-only or later attempt NAT-traversal assistance

These should be decided when they become relevant enough to affect implementation.
