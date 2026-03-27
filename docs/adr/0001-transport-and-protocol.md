# ADR 0001: Transport and Protocol Direction (UDP + DTLS + CBOR)

Status: Accepted

Date: 2026-03-26

## Context

VCStream needs a transport + protocol direction early because it affects:

- the shape of control messages (hello / join / leave / chat / publish / subscribe)
- how media is carried
- what encryption is assumed
- whether NAT traversal / hole punching is still possible later without a rewrite

The project’s stated direction is:

- Encrypt all non-local traffic by default.
- Keep a practical split between a control area and a media area.
- Treat NAT traversal / hole punching as a serious medium-term goal.

This ADR focuses on transport and message framing. It does not fully specify the trust UX (that is a separate ADR), but it does state the basic security building blocks we depend on.

## Decision

### Transport family

Use UDP for both control and media.

Use DTLS to encrypt and authenticate the UDP traffic.

Why:

- UDP keeps NAT traversal / hole punching on the table.
- Qt 6 provides DTLS support (via `QDtls`), so we can build this using Qt + C++17 without pulling in a separate QUIC library.
- UDP avoids TCP head-of-line blocking.

Notes:

- DTLS provides encryption, integrity, and endpoint authentication of the handshake. It does not magically solve “who do I trust?” by itself; that is handled by the trust model (TOFU + optional out-of-band verification).

### Channels (over UDP)

Within one UDP+DTLS session, treat control and media as separate channels:

- Control: mostly reliable messages (join / leave, publish / unpublish, subscribe / unsubscribe, errors)
- Media: time-sensitive messages where loss is acceptable (late is worse than lost)

Notes:

- UDP is message-oriented. We will carry control and media as discrete datagrams and distinguish them by an explicit message `type`.
- Because UDP is unreliable, we need a small reliability layer for the control channel (acks / retries for messages that must arrive).

### Control message format

Encode control messages as CBOR.

- Each message is a CBOR map with required fields `type` and `protocol_version`.
- For UDP, one datagram carries one CBOR message (no stream framing needed).

Datagram payload:

- CBOR bytes for exactly one message.

### Protocol versioning

- Define a single integer `protocol_version` for the whole connection.
- The first control message on a new connection is `hello`. It must include `protocol_version` and capability flags.

### Identity and first-connection safety

We will not rely on a public certificate authority.

- Each relay host has a stable long-term identity key.
- The app shows a short “host identity” code derived from that public key.
- On first connection, the joiner should confirm the host identity matches what the host shares out-of-band.
- After first trust, the app pins the host identity (TOFU) so future connections can warn / refuse on unexpected identity changes.

This is part of the trust model, but it affects what the transport must expose (peer identity material that can be fingerprinted and pinned).

## Guidance (not wire format)

This section is advice. It does not define the wire format, but it captures constraints we should keep in mind.

### Real-time media: prefer drop over delay

- For real-time media, delayed delivery is usually worse than loss.
- Design media so receivers can drop late units and recover cleanly without waiting for retransmits.

### Datagram sizing: avoid fragmentation

- Packetise media to a conservative datagram payload size to reduce the risk of IP fragmentation.
- Treat this as a real constraint for codec packetisation and framing overhead.

### Replicated state: plan for loss

For state that must converge (room membership, published sources, subscriptions, etc.), do not assume every delta arrives. Instead:

- Carry an explicit monotonic `state_revision` (or similar) on relay-authored state.
- Send periodic snapshots (full state) plus deltas (changes) keyed by a `baseline_revision`.
- When a receiver detects gaps (missing baseline, revision jump), request a fresh snapshot (reliable control) or wait for the next scheduled snapshot.

This keeps the protocol robust under loss while still being efficient in steady state.

## Alternatives considered

### A1: TLS over TCP for control + media (or separate TCP connections)

Pros:

- simplest to implement with Qt’s built-in networking primitives

Cons:

- makes NAT traversal / hole punching materially harder (and pushes the design towards TCP-only assumptions)
- poor fit for real-time media (head-of-line blocking, late delivery)

Rejected because NAT traversal / hole punching is considered a serious medium-term goal.

### A2: QUIC for everything

Pros:

- a modern UDP-based transport with built-in encryption, multiplexing, and congestion control

Cons:

- Qt does not provide a general-purpose QUIC stack. We would need to pick, ship, and maintain a QUIC library dependency.
- Packaging and long-term maintenance burden is higher, especially on Windows.

Rejected because this project prioritises a Qt-only implementation path for v1.

### A3: TLS / TCP control + UDP / DTLS media

Pros:

- UDP-friendly media path

Cons:

- If we need NAT traversal / hole punching, a TCP control connection becomes the weak link. If the host / relay is not publicly reachable, the session can fail even if UDP media could be established.
- Requires building and testing two transport families (TCP+TLS and UDP+DTLS), which increases complexity early.

Rejected because it keeps TCP as a required dependency for the session and increases complexity by splitting transport across two stacks.

### A4: Custom UDP security (write our own crypto)

Pros:

- maximum control

Cons:

- high risk: effectively re-implementing DTLS-class security (handshake, key updates, replay protection) and the surrounding transport behaviours needed for Internet use

Rejected as not realistic for this project.

## Consequences

### Implementation impact

- We will need to implement a small reliability layer for control messages (at minimum: message ids, acknowledgements, and retries).
- DTLS support comes from Qt (`QDtls`) and its underlying platform TLS backend.

### Engineering risks

- Reliable control over UDP must be designed carefully to avoid edge-case state corruption.
- NAT traversal / hole punching is not guaranteed to work on every network; we need clear diagnostics and fallbacks.
- Debuggability needs deliberate investment (wire logs, connection stats, message tracing) to avoid a black box.

### Design constraints for later work

- Control message schemas should be designed for forward / backward compatibility (explicit fields, conservative defaults, ignore unknown fields).
- Media units should be self-describing enough to be carried over UDP datagrams without relying on ordered delivery.

## Follow-ups

- ADR: trust model and how clients trust a relay host (fingerprints, TOFU, or a PKI-like approach).
- ADR / note: rendezvous service scope and how it assists NAT traversal (STUN-like discovery, hole punching coordination; TURN-like relaying is out of scope unless explicitly added).
- Specify the control reliability layer (what is reliable vs not, retry rules, id ranges, and how to recover after gaps).
