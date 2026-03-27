# ADR 0004: Reliable Control Layer (UDP + DTLS)

Status: Accepted

Date: 2026-03-26

## Context

ADR 0001 chooses UDP secured with DTLS.

UDP is fast and NAT-friendly, but it does not guarantee delivery. For control messages (join / leave, approvals, publish / unpublish, subscribe / unsubscribe, chat, etc.) we need predictable behaviour when packets are lost.

This ADR defines a small application-level reliability layer for control traffic.

## Goals

- Important control messages either arrive, or we time out with a clear per-peer error.
- Avoid head-of-line blocking: one missing packet should not stall all later control messages.
- Keep the design simple enough to implement and test.

## Non-goals

- Replace DTLS security. DTLS already provides confidentiality and integrity.
- Make media reliable. Missing audio / video frames are expected on UDP.
- Disconnect everyone because one peer had a blip.

## Decision

### Separate concerns: state, media, and reliability

This ADR is about control reliability.

- “Control state” means room / relay state (who is present, what sources exist, subscriptions, permissions, etc.).
- “Media” means audio / video frames.

Missing media frames do not imply a control state gap.

### Terms

- Control datagram: one UDP datagram that carries exactly one CBOR control message (ADR 0001).
- Reliable message: a control message that is retried until it is acknowledged (or until it times out).
- Unreliable message: a control message that may be dropped and is not retried.
- Peer: the remote endpoint we are exchanging DTLS-protected traffic with.

For this ADR, a “session” means one established DTLS association between two peers.

### Session boundary (restarts and reconnects)

The DTLS handshake defines the session boundary.

- Reliable counters and ack state are scoped to one DTLS session.
- If a device restarts and reconnects, it will establish a new DTLS session.

If the host sees the same device identity connect again (a new DTLS session completes), the newest session should supersede the old one.

### Message identity (reliable control)

Each sender maintains a monotonically increasing `msg_id` counter for reliable messages.

- `msg_id` starts at 1 for a new session.
- `msg_id` is scoped to one DTLS session.

The receiver deduplicates reliable messages by `msg_id` within a DTLS session and delivers each reliable message to the application at most once.

More explicitly: the deduplication key is `(dtls_session, msg_id)`.

### Acknowledgements (cumulative + bitmap)

Every control datagram may piggyback acknowledgement information:

- `ack_base`: the highest contiguous `msg_id` received from the peer.
- `ack_bits`: a bitmap for messages immediately above `ack_base`.

Bitmap rules (window size 64):

- Bit 0 corresponds to `ack_base + 1`.
- Bit 1 corresponds to `ack_base + 2`.
- …
- Bit 63 corresponds to `ack_base + 64`.

If a bit is set, that `msg_id` has been received.

This supports selective acknowledgements without requiring per-message ACK packets.

### Retries, limits, and giving up

For each outbound reliable message, the sender keeps a copy until it is acknowledged.

Retry behaviour:

- Resend unacknowledged reliable messages after a short delay.
- Use bounded backoff to avoid flooding.

Limits (required):

- cap the number of in-flight reliable messages
- cap the total buffered bytes for retries

Window saturation handling (required):

- Do not send new reliable messages once the in-flight window reaches the ack bitmap size (64 messages beyond `ack_base`).
- If `ack_base` does not advance and `ack_bits` shows later messages being received, prioritise retransmitting the missing `ack_base + 1` message.

Timeout handling:

- If a reliable message cannot be delivered within an overall timeout budget, surface a clear error for that peer.
- This must not disconnect unrelated peers.
- By default, the peer may remain connected but be marked `Unresponsive` (see Liveness and UI states).

### Ordering

The reliable layer does not guarantee in-order delivery.

- If a message arrives and has not been seen before, deliver it immediately.
- If ordering matters for a message type, that message type must include its own ordering / revision field.

This avoids head-of-line blocking under loss.

### What is reliable

Treat these as reliable by default:

- join / leave
- host approval / denial
- publish / unpublish
- subscribe / unsubscribe
- chat messages
- explicit error responses

Treat these as unreliable by default:

- periodic full state snapshots
- state deltas

Snapshots are the primary recovery path for control state:

- A snapshot should be sufficient to reconstruct the current state even if many deltas were missed.
- A client may send a snapshot request when it suspects a gap, but the request is best-effort.
  - If ignored or delayed, the client waits for the next periodic snapshot.
  - Snapshot request failure is not a reason to drop the session.

### Liveness and UI states

We distinguish two user-visible problems:

- `Stalled (media)`: we have no new media to play.
- `Unresponsive`: we have not heard from the peer at all.

Stalled (media):

- Applies when we expect media from a peer (or from a published source).
- If playback catches up to real time and there is nothing left to play, mark the source (and possibly the peer) as stalled.

Unresponsive:

- Applies to any peer.
- If we receive no valid DTLS-protected traffic (control, media, or keepalive) from a peer for a threshold, mark them unresponsive.
- For peers who are not publishing, use a conservative default threshold (for example 30 seconds).

Keepalive:

- If a peer is otherwise idle, the implementation should send a small keepalive control message so unresponsive detection works.

Default behaviour:

- Do not auto-disconnect a peer just because they are unresponsive.
- Make the unresponsive status obvious in the UI.
- Allow the host to kick an unresponsive peer.

### Missed chat detection

We treat chat history as best-effort across stalls / reconnects.

- The host maintains a room-wide monotonic `chat_seq`.
- Each host-broadcast chat message includes `chat_seq`.
- Periodic room snapshots include `chat_seq_latest`.

Each client tracks `last_chat_seq_seen` in memory.

- If a client becomes unresponsive and later receives a snapshot where `chat_seq_latest > last_chat_seq_seen`, the client inserts a system line in chat:
  - “System: You may have missed chat messages while you were offline.”

Chat ordering:

- Chat is ordered by `chat_seq`.
- A sender should show a local “pending” message immediately.
- The sender includes a `client_msg_id` in the outgoing chat.
- When the host echoes the message back with the assigned `chat_seq` and the same `client_msg_id`, the sender updates the pending message and places it in the correct order.

### Corruption and truncation

DTLS provides integrity.

- If a packet is truncated or any bit is flipped, DTLS authentication fails and the packet is dropped.
- The reliable layer handles loss and duplication. It is not an integrity layer.

## Alternatives considered

### A1: Per-message ACK packets

Rejected: easy to implement, but creates more packets and more wakeups.

### A2: In-order reliable stream

Rejected: simple for consumers, but a single lost packet can stall all later control messages.

### A3: No reliability (snapshots only)

Rejected: join / approval / publish / subscribe need stronger guarantees than “wait for the next snapshot”.

## Consequences

- Control message types should be replay-safe (idempotent) or should include enough identity fields that duplicates can be ignored safely.
- Implementations must treat `msg_id` and ack windows carefully to avoid unbounded memory growth.
- Liveness and reliability are separate. A reliable timeout is a per-peer error, not a global disconnect.

## Follow-ups

- Define the exact CBOR envelope fields for `msg_id`, `ack_base`, and `ack_bits`.
- Choose initial retry / backoff / timeout values and in-flight limits.
- Decide the exact keepalive behaviour.
