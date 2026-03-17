# Native Conferencing App

## Purpose

This project is a **single native desktop application** for small-group conferencing and live-production support.

It is intended for a private group that needs something more production-friendly than Discord-like meeting UIs, especially for use with OBS.

The application targets:
- **Windows and Linux**
- **C++ with Qt 6**
- **one application** that can act as participant, relay host, and/or OBS bridge
- **direct connection by IP/port** for the core version
- an **optional rendezvous/NAT-traversal extension** for easier peer discovery and reduced reliance on port forwarding

This document is the **front-door overview** for human developers and AI agents. It explains the product vision, current architectural direction, major constraints, and the key decisions that have already been made.

The task-by-task execution plan lives in [`ROADMAP.md`](./ROADMAP.md).

---

## Product summary

The application supports three core roles, which may be combined in one running instance:

- **Participant**
  - connect to a relay
  - publish local media
  - receive remote media
  - render remote media locally
  - optionally expose locally available media to OBS

- **Relay host**
  - listen for inbound client connections
  - maintain room/session state
  - relay media between participants
  - may run over LAN, VPN, or the public internet
  - in the simplest deployment, the host handles port forwarding if needed

- **OBS bridge**
  - expose selected **locally available** media over HTTP
  - intended for OBS Browser Source and/or local production tooling
  - can be enabled on **any** client, not just the relay host
  - should support **localhost first**, with **trusted-LAN output** as an optional extension

Valid combinations include:
- participant only
- participant + OBS bridge
- participant + relay
- participant + relay + OBS bridge

The relay host, the DJ/music publisher, and the person running OBS do **not** need to be the same person.

---

## Main user needs

The system should support:
- multi-person audio/video calls
- webcam and screen/window sharing
- OBS Virtual Camera as an input source
- each remote video source handled independently
- optional OBS integration without capturing fragile UI windows
- basic private text chat as a fallback/backstage channel
- optional music playback published by any participant
- optional LAN-visible OBS output so one PC can host the conference while another PC streams to Twitch

---

## Core goals

- One application, not separate “server” and “client” products.
- Native desktop UI using Qt 6.
- The main conferencing path should be native, not browser-dependent.
- A participant may publish more than one media source.
- Any client should be able to export locally received media to OBS.
- Provide a simple room-wide text chat for fallback/private coordination.
- Encrypt all non-local network traffic by default.
- Keep the architecture flexible enough to support an **optional rendezvous/NAT-traversal extension** later.
- Avoid unnecessary ongoing costs.
- Keep the design suitable for a small private group rather than an internet-scale service.

---

## Non-goals for the first serious version

Do **not** accidentally turn this into “build Discord/Zoom from scratch”.

Out of scope unless explicitly revisited:
- federation
- internet-scale public rooms
- mandatory user accounts
- polished file transfer
- stickers/reactions/emojis
- mobile apps
- browser-first clients
- public internet exposure of the OBS bridge
- complicated moderation/admin systems
- fully general public-service media relaying
- end-to-end encryption across independently relayed media hops, unless deliberately planned

---

## Key assumptions and constraints

### Networking
- The relay host may be on LAN, VPN, or the public internet.
- The simplest deployment assumes the host is reachable directly, possibly via port forwarding.
- A later optional extension may add **rendezvous and NAT traversal** so rooms can be discovered and joined without manual port forwarding.
- A rendezvous service by itself solves **discovery**, not necessarily **reachability**.
- If NAT traversal is a genuine goal, it affects the protocol and transport choice early.
- This is a small-group system, not a massive conferencing platform.

### Technical
- Language: **C++**
- Framework: **Qt 6**
- Platforms: **Windows 10/11** and **modern Linux**
- Build/deployment should support ordinary end users on standard Windows machines.
- HTTP is acceptable for OBS integration on localhost and, optionally, a trusted LAN.
- Non-local traffic should be encrypted.
- If NAT traversal is pursued, a **UDP-friendly transport design** is likely to matter.

### UX
- The UI should remain simple enough for non-technical group members to use.
- Manual IP/port entry is acceptable for the core version.
- A later room-code rendezvous flow is desirable.
- The app should support live-production use without requiring users to understand networking jargon.

---

## Terminology

In these documents, prefer **source** in high-level prose.

A **source** means one independently publishable media source, such as:
- microphone audio
- webcam video
- screen share video
- window share video
- avatar/output video
- music audio

Inside the code and protocol, it is acceptable to use **track** as the technical/internal term if that proves convenient. If so, document the mapping clearly:

> source (product/architecture term) ≈ track (internal implementation term)

Do not assume readers will automatically recognise “track” in the live-conferencing sense.

---

## Architectural overview

### High-level model

The application is a **single executable** with **role-based composition**.

Every running instance is fundamentally a **participant node**.

Optional capabilities:
- relay hosting
- OBS bridge
- later, perhaps rendezvous publication/lookup support

This avoids duplicated code and matches real usage better than rigid “server vs client” binaries.

### Main concern areas

The system should be understood as three main concern areas.

#### 1. Control area
Handles:
- connect/join/leave
- participant list
- source registry and metadata
- subscriptions
- mute/unmute state
- text chat
- diagnostics/status messages
- authentication/admission decisions
- OBS export bindings
- music source control metadata

#### 2. Media area
Handles:
- device capture
- media normalisation
- encode/decode
- packetisation
- transport
- receive/reorder/buffer
- render/playback
- mixing or routing of distinct audio sources

#### 3. OBS presentation area
Handles:
- exposing selected locally available media via HTTP
- simple endpoints such as `/solo/<source-id>`
- localhost-first presentation for OBS Browser Source
- optional trusted-LAN presentation later

The OBS presentation area should **not** be part of the relay architecture. It is a local adapter.

---

## Core architectural principle: source-first design

The most important modelling decision is:

> Treat each audio/video source as an independent first-class object.

This matters because one participant may publish:
- microphone
- webcam
- screen share
- avatar output
- music

These must be independently:
- published
- relayed
- subscribed
- decoded
- rendered
- muted/adjusted
- exported to OBS

Do not model the system as “one participant = one media blob”.

---

## Structured-analysis view of major transformations

### T1. Source acquisition
Transforms devices, windows, screens, or local player output into raw media.

### T2. Media preparation
Transforms raw media into a normalised internal representation with timestamps and stable format assumptions.

### T3. Encoding and framing
Transforms prepared media into transmittable packets/units.

### T4. Relay distribution
Transforms one published source into zero or more forwarded deliveries according to the subscription map.

### T5. Reconstruction
Transforms inbound packet streams into playable/renderable media again.

### T6. Presentation
Transforms reconstructed media into:
- native windows/audio output
- localhost HTTP presentation for OBS
- optionally trusted-LAN HTTP presentation for OBS

### T7. Coordination
Transforms user actions and system events into structured control messages:
- join
- leave
- subscribe
- mute
- text chat
- music control
- diagnostics
- rendezvous publication/lookup if that extension is enabled

---

## Suggested subsystem decomposition

### AppSupervisor
Responsible for:
- startup
- settings loading/saving
- role enablement
- major subsystem lifecycle

### RoomSession
Responsible for:
- connection state
- room membership
- participant/source metadata
- join/leave/reconnect flow

### RelayServer
Responsible for:
- listening for inbound connections
- accepting clients
- source forwarding
- subscription-aware routing

### SourceCatalogue
Responsible for:
- tracking locally published sources
- tracking remotely announced sources
- maintaining source identity and state

### MediaCapture
Responsible for:
- enumerating cameras/microphones/screens/windows
- starting/stopping capture
- reporting source state changes

### MusicPlaylistSource
Responsible for:
- queue/playlist control
- playback controls (play/pause/seek/loop)
- publishing music as a distinct audio source

### AudioGraph
Responsible for:
- keeping voice and music as separate logical sources
- local volume/mute control
- routing local playback and outbound publication appropriately
- optional later mixing where truly needed

### VideoGraph
Responsible for:
- decode and transform pipeline
- optional filter chains
- routing video to one or more sinks

### VideoRenderer
Responsible for:
- local previews
- remote video presentation
- one window per remote video source where required

### TextChat
Responsible for:
- sending and receiving room text chat
- optional small bounded in-memory history
- displaying system/status messages

### HttpMediaServer
Responsible for:
- enabling/disabling HTTP export
- binding source IDs to export endpoints
- serving localhost pages/media for browser-source-style consumers
- later, optionally serving trusted-LAN pages/media

### Diagnostics
Responsible for:
- structured logging
- state inspection
- counters/stats
- aiding debugging for humans and AI agents

### RendezvousClient (optional extension)
Responsible for:
- publishing room availability to a public rendezvous service
- joining rooms by short code
- exchanging metadata needed for direct connection or NAT-traversal attempts

---

## Data model

The following entities should exist explicitly.

### InstanceRole
Possible flags:
- participant
- relay
- http_export

### Participant
Fields:
- participant_id
- display_name
- connection_state
- published_source_ids
- subscribed_source_ids
- host_flag

### Source
Fields:
- source_id
- owner_participant_id
- media_kind (`audio` / `video`)
- source_type (`microphone`, `camera`, `screen`, `window`, `avatar`, `music`, etc.)
- codec/format metadata
- state (`live`, `muted`, `paused`, `lost`, `stopping`, etc.)

### Room
Fields:
- room_id
- relay_instance_id
- participants
- source registry
- subscription map

### LocalSourceBinding
Represents a source that is actually available on the local machine.
Fields:
- source_id
- receive_state
- decoder/render state
- bound_window_id (optional)
- obs_export_state

### ChatMessage
Fields:
- message_id
- sender_participant_id
- timestamp
- delivery_scope
- text

For the first version, `delivery_scope` can just be `room`.

### ObsEndpoint
Fields:
- endpoint_path
- source_id
- enabled_flag
- bind_scope (`localhost`, later maybe `trusted_lan`)

### RendezvousRoomRecord (optional extension)
Fields:
- room_code
- password_hash
- relay_endpoint_hint
- created_at
- expires_at
- last_seen_at
- protocol_version
- capability_flags

---

## Security direction

### Strong recommendation
Encrypt all **non-local** traffic by default.

### Practical split
- **Control area**: encrypted reliable channel
- **Media area**: encrypted transport
- **OBS bridge**: plain HTTP is acceptable if bound strictly to `127.0.0.1`; trusted-LAN mode needs deliberate opt-in and careful scoping
- **Rendezvous service**: encrypted control traffic

### Guidance
Treat encryption as a default architectural requirement, not a late optional hardening step.

Do **not** defer the overall security model indefinitely. Decide the basic transport trust model early enough that the protocol is not built around plaintext assumptions.

### Early decisions that must be made
At a minimum, decide:
- whether the control channel is TLS-based
- whether the media channel uses the same connection family or a separate encrypted transport
- how clients trust the relay host
- whether future NAT traversal is a serious goal
- whether that future goal implies a UDP/QUIC-friendly design from day one

---

## Text chat guidance

Text chat should exist as a **simple IM-style feature**, not a terminal emulator.

### Intended uses
- fallback if microphones fail
- backstage/private coordination during a live stream
- quick status messages
- system notices

### First-version scope
- room-wide text chat only
- no formatting
- no files
- no reactions
- no mandatory persistence
- optional small in-memory buffer on relay

Text chat is part of the **control area**, not the media area.

---

## Music guidance

Music should be treated as a **published audio source**, not as a special hidden global mix.

### First-version scope
- any participant may act as DJ
- the DJ does **not** need to be the relay host
- music playback should happen **inside the application**
- loop support and a simple playlist/queue are desirable
- participants should be able to mute music individually
- the streaming machine should also be able to subscribe to the music source for OBS

### Guidance
Do **not** make v1 depend on capturing arbitrary external application audio on Windows. That can be a later enhancement.

---

## OBS bridge guidance

The OBS bridge should be locally enableable on **any client**.

This is important because:
- the relay host and the stream producer may be different people
- a production machine can subscribe to the sources it needs
- the production machine can then expose those sources to OBS locally

### Design guidance
The OBS bridge should work only from **locally available sources**.

It should not bypass normal subscription/routing logic or create a special hidden second client model.

### First-version goal
Support endpoints that map one locally received video source to one localhost URL/page.

### Later optional extension
Allow the bridge to bind to a chosen **trusted LAN** interface so a second PC on the LAN can consume the source in OBS.

---

## Rendezvous and NAT-traversal extension

A later optional extension may add a very small public service for room discovery.

### Intended shape
- assign a short **6-digit room code**
- store a password hash
- keep a small ephemeral room record
- allow users to join by room code + password
- implement this public service as a standalone CLI application, **NOT** part of the main application
- use a tiny cheap headless Linux server with a fixed IP

### Important caution
A room directory by itself solves **discovery**, not necessarily **reachability**.

If the long-term goal is “join without port forwarding and without media relay”, then:
- NAT traversal becomes relevant
- hole punching becomes relevant
- a UDP/QUIC-friendly design may be important early

So this extension should be planned honestly:
- **simple rendezvous** is small
- **rendezvous plus NAT traversal** is materially bigger

---

## Deployment guidance

### Windows
Expected approach:
- build Windows x64 release
- bundle Qt runtime dependencies
- use a normal installer for regular users
- likely include/install the MSVC runtime if using MSVC
- users should not need to install development tools

### Linux
Expected approach:
- native package or portable bundle to be decided later
- avoid assuming unusual system libraries beyond those needed by the app
- packaging strategy should be revisited once the feature set is stable enough

### Public mini-server (optional extension)
If the rendezvous service is built, it should be able to run cheaply on a small headless Linux VPS with:
- fixed IP
- no domain name required
- no media relay duties
- small persistent state or even mostly in-memory state with expiry

---

## Decision framework

Not all design choices should be made at once.

### Decide early when the decision affects:
- core protocol shape
- data model
- transport family
- NAT-traversal viability
- security model
- packaging/dependency strategy
- major subsystem boundaries

### Delay a decision when it affects only:
- non-essential UI polish
- naming
- visual styling
- secondary convenience features
- optional advanced configuration

### When discussing a decision with an AI agent
The AI agent should:
1. identify the decision explicitly
2. state why it matters now
3. present 2–3 viable options
4. recommend one option with reasoning
5. note what future work the decision makes easier or harder

---

## Open questions still intentionally unresolved

These are deliberate future decision points.

- Exact media transport architecture
- Exact codec choices
- Exact encryption/trust implementation details
- Exact serialisation format for control messages
- Whether the internal code/protocol should use `Source` or `Track` as the primary type name
- Whether OBS bridge pages are rendered from decoded local state or another local media surface
- Windows installer technology
- Linux packaging format
- Whether relay can optionally be headless later
- Whether direct/private chat is needed
- Whether file transfer is worth supporting later
- Whether NAT traversal is a serious enough goal to justify transport choices that are more complex up front
- Whether the rendezvous service should remain discovery-only or later attempt NAT-traversal assistance

These should be decided when they become relevant enough to affect implementation.

---

## Immediate next steps

If starting from scratch, the recommended first tasks are:
1. create repository and Qt project skeleton
2. define coding conventions and architecture notes
3. define initial module boundaries
4. build the main application shell
5. add role enablement UI
6. enumerate local media devices
7. define the local source model in code
8. make the first transport/security/NAT-traversal ADR

See [`ROADMAP.md`](./ROADMAP.md) for the full phased task list with completion criteria.

---

## Short plain-English summary

This project is a **single Qt 6 native desktop application** for small-group conferencing and live-production support. Each instance acts as a participant and may optionally also host the relay and/or expose selected locally available media for OBS. The architecture is source-first, with separate control, media, and OBS-presentation concern areas. Text chat, a distinct music source, and LAN OBS output are planned as sensible extensions. A later optional rendezvous service may improve room discovery and, if pursued far enough, may also drive early transport choices because NAT traversal is not free.
