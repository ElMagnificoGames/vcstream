# ADR 0003: Rendezvous vs NAT Traversal (Scope and Expectations)

Status: Accepted

Date: 2026-03-26

## Context

VCStream plans an optional small rendezvous service for room discovery and hole punching coordination.

It is easy to blur “I can find the room” with “I can connect to the room”. They are different problems, and mixing them leads to false expectations and the wrong early design choices.

This ADR records what we mean by rendezvous (discovery), what we mean by NAT traversal (reachability), and how serious NAT traversal is as a goal.

## Components and responsibilities

There are two separate programs involved:

- `vcstream` (the desktop application)
  - hosts a room / relay (in host mode)
  - joins a room (in join mode)
  - performs DTLS handshakes and identity verification (ADR 0002)
  - performs NAT traversal / hole punching attempts
  - shows user-facing diagnostics and fallbacks

- `vcstream_rendezvous` (a separate headless service)
  - provides room discovery (room code -> endpoint hints)
  - assists hole punching by acting as a public coordination point (signalling and observed endpoint hints)
  - stores only short-lived records (expiry-driven)
  - is not a trust anchor

## Definitions

### Rendezvous (discovery)

Rendezvous answers:

- “What is the relay endpoint for room code X?”

It is a directory service. It does not make a private host reachable by itself.

It can still materially improve connection success by providing endpoint hints and timing coordination for hole punching (see Decision).

### NAT traversal (reachability)

NAT traversal answers:

- “Can two devices behind NATs / firewalls establish a usable UDP path without manual port forwarding?”

This includes:

- learning external address / port mappings (STUN-like behaviour)
- coordinating hole punching
- coping with NAT timeouts (keepalives)

Some networks will still be unreachable without a relay-like fallback.

## Decision

### Scope

- Rendezvous is in scope as an optional future extension.
- NAT traversal / hole punching is a real medium-term goal.
- We will treat NAT traversal as best-effort. We will not claim “always works”.
- TURN-like media relaying (a third-party server that forwards traffic when hole punching fails) is out of scope unless explicitly added later.

### What rendezvous does (and why it helps)

`vcstream_rendezvous` may assist hole punching by:

- observing and returning the host’s apparent public UDP endpoint (best-effort hint)
- coordinating join timing so both sides send UDP traffic during the same window
- exchanging small connection hints / candidates between joiner and host

This tends to improve success rates, but it still cannot guarantee reachability on all networks.

### Trust

- Rendezvous is not a trust anchor.
- Clients must still verify the relay host identity using the trust model (ADR 0002).

Rendezvous may store the host identity fingerprint as part of the room record, but:

- the client must still compute the host identity from the authenticated DTLS peer identity material
- a mismatch is treated as suspicious (do not silently accept “whatever rendezvous says”)

### Privacy

Storing a host identity fingerprint is useful for early warning and debugging, but it is also a stable identifier.

- Keep records short-lived.
- Keep stored fields minimal.

## Consequences

### User-facing expectations

- A room code can help you find a room, but it cannot guarantee you can reach it.
- Joining may fail on some networks (for example restrictive corporate networks or certain CGNAT setups).
- The app should surface clear diagnostics when reachability fails and suggest practical fallbacks (try a different network, host on a machine with a public IP, or configure port forwarding).

The app may also offer an optional “advanced diagnostics” mode that shows more technical details for troubleshooting.

### Early design choices this affects

Because NAT traversal is a real goal, we have already made these choices:

- Transport direction stays UDP-based (ADR 0001).
- Security is based on DTLS and pinned host identity (ADR 0002).

It also constrains future protocol and implementation work:

- We need explicit connection attempt states and timeouts ("trying", "waiting for approval", "handshake", "connected", "failed").
- We need packet budget discipline (avoid fragmentation) and conservative keepalive behaviour.
- We should design the control protocol so rendezvous can pass minimal connection hints without becoming a complex stateful service.

## Alternatives considered

### A1: Discovery-only forever

Rejected: NAT traversal / hole punching is considered a real medium-term goal.

### A2: Add TURN-like relaying early

Deferred: it materially increases infrastructure and operating cost and is not required to begin development.

## Follow-ups

- Specify the rendezvous record fields and expiry rules (room code, endpoint hints, host identity fingerprint, protocol version, capability flags).
- Define the NAT traversal approach (candidate gathering, coordination, retries, keepalive policy) and how failures are reported.
