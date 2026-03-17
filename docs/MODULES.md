# Initial Module Boundaries

This document defines the initial module boundaries for this repository.

It is intentionally an early, lightweight decomposition: it is meant to stabilise vocabulary, reduce overlap, and prevent accidental dependency cycles before the networking/media ADRs are written.

Non-goals:

- This is not yet a commitment to an exact `modules/<domain>/<topic>/...` directory taxonomy.
- This is not a final architecture; later ADRs (transport, trust model, NAT traversal scope) may require adjustments.

## Modules

Each module description is written in a Smalltalk class-comment style:

- what it is for
- what it collaborates with
- what it explicitly does not do (non-goals)
- any early ownership/lifetime/threading assumptions worth making explicit

### AppSupervisor

I am the top-level application supervisor.
I own startup/shutdown, settings load/save, role enablement, and the lifetime wiring of the major services.
I present a coherent "one instance can be participant/relay/HTTP media server" control surface to the UI.

I collaborate with the service modules to do real work; I do not implement capture, rendering, networking, or protocol details myself.

State model (initial): I am UI-thread-affine; I own long-lived service objects and start/stop them cleanly.

I do not:

- own authoritative room/session state (that is `RoomSession` / relay-side state)
- implement capture/render/networking directly (delegates to the relevant services)

### SharedTypes

I am the shared vocabulary.
I define the core data model types and identifiers (for example: `Participant`, `Source`, `Room`, id types, enums, and small value objects) so every subsystem speaks the same language.

I collaborate with all other modules by providing types; I do not perform IO, emit signals, spawn threads, or own resources.

State model (initial): I should be mostly immutable value types; if anything here becomes stateful, that is a design smell and should be justified.

I do not:

- contain business logic or side effects
- perform IO, networking, capture, rendering, or persistence

### RoomSession

I am the room session coordinator.
I own connection state, join/leave/reconnect flows, and the client-side view of room membership and participant/source metadata.
I turn user intent ("join", "leave", "subscribe") into coordination actions, and I turn network/relay events into state transitions.

I collaborate with `SourceCatalogue` to keep source identity/state consistent, and with `TextChat` for room text chat.
I do not forward media (that is `RelayServer`) and I do not duplicate the canonical source table (that is `SourceCatalogue`).

State model (initial): I am treated as single-threaded from the UI perspective; if background IO threads are introduced later, updates are delivered to me via explicit Qt-thread-safe mechanisms.

I do not:

- implement relay forwarding/routing (that is `RelayServer`)
- own the canonical source table (that is `SourceCatalogue`)

### RelayServer

I am the relay host server.
When the instance is acting as a relay, I listen for inbound connections, accept clients, and perform subscription-aware forwarding/routing of media and coordination traffic.

I collaborate with `SourceCatalogue` for source identity/state and with `RoomSession`-owned policy about who is subscribed to what.
I do not implement UI, HTTP export, or local capture.

State model (initial): my public control surface is single-threaded; internal concurrency is an implementation detail that must not leak through my interface.

I do not:

- implement UI, HTTP export, or local media capture
- define the user-facing room/session UX (that remains in `AppSupervisor` / UI)

### SourceCatalogue

I am the source catalogue.
I track locally published sources and remotely announced sources, maintain stable source identity, and publish source state changes to interested consumers.

I collaborate with capture, audio/video graphs, relay, and HTTP export as the shared source-of-truth for source metadata/state.
I do not capture devices or decode/render media.
I do not implement subscription policy; I store the current state and expose it in a usable form.

State model (initial): I own the canonical in-memory source table and issue change notifications; callers must not maintain shadow copies without a deliberate design decision.

I do not:

- capture devices or decode/render media (that is `MediaCapture` / `VideoGraph` / `VideoRenderer`)
- implement subscription policy itself (it stores state; policy lives in higher-level services)

### MediaCapture

I am the local capture controller.
I enumerate local capture devices (cameras, microphones, screens/windows), start/stop capture, and report local source availability/state changes.

I collaborate with `SourceCatalogue` to publish local source state.
I do not decide which remote sources are subscribed to, and I do not implement UI presentation or HTTP export.

State model (initial): I own capture lifetimes; if device threads/callbacks exist, I translate them into safe state updates that do not leak exceptions across Qt boundaries.

I do not:

- decide which remote sources are subscribed to
- implement HTTP export or UI presentation

### MusicPlaylistSource

I am the local music playlist source.
I manage a queue/playlist and playback controls (play/pause, seek, looping), and I produce a stream of music samples as a distinct audio source.

I collaborate with `AudioGraph` to ensure music remains a first-class source and is routed intentionally.
I do not own the system audio output device; I am a source, not a sink.

State model (initial): I own playlist state (queue position, play/pause, loop mode) and publish state changes in a way other modules can observe.

I do not:

- define global mixing/output semantics (that is `AudioGraph` policy)
- implement HTTP export

### AudioGraph

I am the audio graph.
I build and own the local audio topology: which audio sources exist, how they are routed, and which sinks they feed.
I own per-source audio policy (mute/volume/monitor/publish) and, where needed, mixing/downmixing for local listening.

I collaborate with `MediaCapture` (voice/mic sources), `MusicPlaylistSource` (music), `SourceCatalogue` (source identity/state), `RoomSession` (session policy), and `Diagnostics`.
I do not perform device enumeration (that is `MediaCapture`) and I do not host the relay (that is `RelayServer`).

State model (initial): I am configured from the UI/session policy but I own the runtime audio graph and fail safely when sources disappear or reappear.

I do not:

- collapse all audio into a single global mix by default
- become the authoritative source catalogue (that is `SourceCatalogue`)

### VideoGraph

I am the video graph.
I build and own the video processing topology: decode, transform, optional filter chains, and routing to one or more sinks.

I collaborate with `SourceCatalogue` (source identity/state), `RoomSession` (session policy), `VideoRenderer` (local display sink), `HttpMediaServer` (HTTP sink), and `Diagnostics`.
I do not own window management or drawing surfaces; I feed sinks such as `VideoRenderer`.

State model (initial): I own video pipeline lifetimes and per-source filter configuration; I must fail safely when a source disappears or changes format.

I do not:

- duplicate source identity/state (that is `SourceCatalogue`)
- define UI layout or window policy (that remains UI-driven; `VideoRenderer` is the sink)

### VideoRenderer

I am a local video display sink.
I present video streams (previews and remote video) in windows or viewports as required.

I collaborate with `VideoGraph` to receive video frames and with `Diagnostics` for visibility into failures/performance.
I do not decide which filters apply or which sinks should exist; that is `VideoGraph` and UI/session policy.

State model (initial): I own presentation resources; I can be restarted or reconfigured without affecting source identity/state.

I do not:

- own source identity/state (that is `SourceCatalogue`)
- implement HTTP export (that is `HttpMediaServer`)

### TextChat

I am the room text chat service.
I send and receive room text chat, provide system/status messages suitable for user display, and (optionally) maintain a small bounded in-memory history.

I collaborate with `RoomSession` for membership context.
I do not own room membership and I do not decide transport framing; those are set by the transport/protocol ADR.

State model (initial): I own chat history state (if enabled) and expose a safe, bounded view for UI consumption.

I do not:

- own room membership state (that is `RoomSession`)
- implement transport-level message framing (that is decided by the transport/protocol ADR)

### HttpMediaServer

I am the local HTTP media server.
I provide a local HTTP interface intended for browser-source-style consumers, mapping source IDs to export endpoints and serving the required pages/media.

I collaborate with `SourceCatalogue` (source identity/binding), `AudioGraph` and `VideoGraph` (media sinks), and `Diagnostics`.
I do not invent a second subscription model; I present locally available media as selected by the user/session policy.

State model (initial): I own the HTTP server lifetime and endpoint bindings; I must fail safely when a source disappears or changes state.

I do not:

- depend on relay networking (I remain a local adaptor)
- replace the session/room model (that remains in `RoomSession`)

### Diagnostics

I am the diagnostics sink.
I provide structured logging, state inspection hooks, and counters/stats to aid debugging.

I collaborate with every module by receiving diagnostics and making them visible.
I do not become a controller of application state and I should not depend on other feature modules.

State model (initial): I aggregate events and counters; I should remain safe to call from different contexts, but any thread-safety guarantees must be explicit when implemented.

I do not:

- become a dependency hub for feature logic (it should be a sink, not a controller)

### RendezvousClient (optional extension)

I am the optional rendezvous client.
When enabled, I publish room availability to a rendezvous service and allow joining rooms via short codes.

I collaborate with `RoomSession` to translate rendezvous outcomes into normal join flows.
I do not replace direct IP/port connectivity in the core version and I do not force NAT traversal requirements into the core architecture.

State model (initial): I am optional and should be removable without affecting core behaviour; any persistent identity/credentials must be explicit and documented.

I do not:

- replace direct IP/port connectivity in the core version
- force NAT traversal requirements into the core architecture

## Dependency direction

This section records an initial dependency direction to prevent accidental cycles.

High-level rules:

- UI (QML + thin C++ adaptors) depends on `AppSupervisor` and `SharedTypes`; it must not reach directly into networking/capture/render/HTTP services.
- `AppSupervisor` orchestrates; it owns lifetimes and wires services together.
- Service modules depend on `SharedTypes` and on each other only through narrow, intentional interfaces.
- `Diagnostics` is a sink: everyone may report to it, but it should not depend on other feature modules.

Initial allowed dependencies (first pass):

- All modules -> `SharedTypes` (shared types and identifiers)
- `AppSupervisor` -> `RoomSession`, `RelayServer`, `SourceCatalogue`, `MediaCapture`, `MusicPlaylistSource`, `AudioGraph`, `VideoGraph`, `VideoRenderer`, `TextChat`, `HttpMediaServer`, `Diagnostics`
- `RoomSession` -> `SourceCatalogue`, `TextChat`, `Diagnostics`
- `RelayServer` -> `SourceCatalogue`, `Diagnostics`
- `MediaCapture` -> `SourceCatalogue`, `Diagnostics`
- `MusicPlaylistSource` -> `AudioGraph`, `Diagnostics`
- `AudioGraph` -> `MediaCapture`, `MusicPlaylistSource`, `SourceCatalogue`, `RoomSession`, `Diagnostics`
- `VideoGraph` -> `SourceCatalogue`, `RoomSession`, `VideoRenderer`, `HttpMediaServer`, `Diagnostics`
- `VideoRenderer` -> `Diagnostics`
- `HttpMediaServer` -> `SourceCatalogue`, `AudioGraph`, `VideoGraph`, `Diagnostics`
- `RendezvousClient` -> `RoomSession`, `Diagnostics`

Explicitly forbidden dependencies (to keep boundaries real):

- UI -> `RelayServer` / `MediaCapture` / `AudioGraph` / `VideoGraph` / `VideoRenderer` / `HttpMediaServer` (go through `AppSupervisor`)
- `RelayServer` -> UI / `HttpMediaServer`
- `HttpMediaServer` -> `RelayServer`

## Overlaps and conflict risks (call-outs)

- `RoomSession` vs `RelayServer`: room/session state ownership must be clear (authoritative relay state vs client view).
- `SourceCatalogue`: should be the single place for source identity/state; avoid parallel source tables in other modules.
- `VideoGraph` vs `HttpMediaServer`: both touch "what is served"; do not create divergent selection/subscription semantics.
- Music handling: keep music as a first-class source; avoid implicitly mixing it into voice without an explicit design decision.

## Future layout notes

Once modules become concrete C++ modules, they should follow `docs/CODE-QT6.md`:

- a `*-dd.txt` design document per module
- a stable include path under `modules/<domain>/<topic>/...`
- unit tests derived from the DD public interface where applicable
