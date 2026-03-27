# ADR 0002: Trust Model (TOFU + Host Approval + Optional Room Password)

Status: Accepted

Date: 2026-03-26

## Context

VCStream encrypts non-local traffic by default and intends to support NAT traversal / hole punching.

We do not want to depend on a public certificate authority (CA), and we want a trust model that:

- is understandable to regular users
- blocks silent man-in-the-middle attacks when users do a simple out-of-band check
- stays low-maintenance (Qt 6 + C++17; avoid shipping a large transport dependency)

This ADR assumes the transport direction from ADR 0001: UDP secured with DTLS.

## Decision

### Identities

Each device has a long-term identity keypair.

- The device identity is its public key.
- The app shows a short `identity code` that is a fingerprint of that public key.
- The private key stays on the device.

The public key (and the identity code) are not secrets. An attacker can learn them.

What matters is proof of possession:

- During the DTLS handshake, the peer must prove it controls the private key for the public key it presents.
- The app computes the peer’s identity code locally from the authenticated DTLS peer identity material (public key / certificate) used in the handshake.
- The identity code shown in the UI is not a string provided by the network protocol.

The out-of-band check is how the user verifies that the public key they have received over the network really is the host’s public key, and not an attacker’s.

#### Identity code format

The identity code is a human-friendly display form of a cryptographic fingerprint.

- Fingerprint input: the canonical public key bytes for the device identity.
  - Prefer a stable DER encoding of the public key as used by DTLS peer identity (for example an X.509 SubjectPublicKeyInfo / SPKI encoding).
  - The exact public key type and canonical encoding are part of the implementation work, but the key requirement is: the same key must always produce the same bytes.
- Fingerprint function: `SHA-256( "VCStream identity v1" || public_key_bytes )`.
  - The prefix is domain separation so we do not accidentally re-use the hash in some unrelated context.
  - Here `||` means byte concatenation, not a logical operator.
- Stored form: store the full 256-bit fingerprint.
- Display form: show the first 80 bits of the fingerprint encoded with Crockford Base32.
  - Display 16 characters grouped as `XXXX-XXXX-XXXX-XXXX`.
  - The display code is case-insensitive.
  - Crockford Base32 is chosen because it is human-friendly (case-insensitive and avoids common ambiguous characters) and works well when read aloud, which reduces mistakes when comparing codes out-of-band.

### Host identity trust (TOFU + first-time out-of-band check)

We use TOFU (Trust On First Use) with an explicit first-time check.

On first connect to a host device:

- the joiner is shown the host identity code
- the joiner must confirm it matches what the host shared out-of-band
- if confirmed, the host identity is pinned locally

On later connects:

- if the host identity matches the pinned one, proceed normally
- if it does not match, warn clearly and block by default

This prevents silent MITM after a host is pinned, and prevents MITM on first connect when the out-of-band check is actually performed.

### Room access control

Room access control is separate from host identity trust.

#### Host approval

Unknown joiners require host approval.

- When a join request arrives, the host sees the joiner’s display name (informational) and joiner identity code.
- The host can `Allow` or `Deny`.
- The host can set `Always allow this participant on this host`.
  - This adds the joiner identity to an allowlist scoped to the host device identity (not scoped to a room code).

#### Optional room password

The host may set a room password.

- The password is required every time.
- The password is never persisted to disk.
- The password is checked after host approval.
  - This keeps host diagnostics and control (i.e. attempts to join with the wrong password aren't silently ignored), whilst still requiring the shared secret to complete the join.

Note: the room password is not a public-key identity and does not replace host identity verification.

### What we show users

Joiner UI should show:

- `Your identity` code
- `Host identity` code (and require confirmation on first connect)

Host UI should show for join requests:

- joiner display name (informational)
- joiner identity code

Network endpoint details (IP address, port) may be shown for diagnostics but must be clearly labelled as unstable and not an identity.

### Storage

- Device identity private key: stored in the app data directory with restricted permissions.
  - Additionally wrapped with simple obfuscation to avoid leaving the key as obvious plaintext on disk.
  - This is not a substitute for OS keychains and it does not protect against malware running as the user.
  - It must remain portable: copying the app data to a new machine must still work.
    - Derive an obfuscation key from a built-in app constant plus a per-file random salt (stored alongside the blob) using a small KDF.
- Pinned host identities: stored in the app data directory.
- Host allowlist (remembered participants): stored in the app data directory.
- Room password: in-memory only.

## Alternatives considered

### A1: Public CA / WebPKI certificates

Rejected: beyond the project scope, and not appropriate for ad-hoc rooms.

### A2: TOFU with no first-time verification

Rejected: the first connection can be silently MITM'd.

### A3: Password-only security

Rejected: a password alone does not provide reliable MITM protection without a specific, correctly-designed authentication protocol, and it does not provide a stable identity for remembering participants.

### A4: Write our own cryptography and handshake

Rejected: high risk and high long-term maintenance cost.

## Consequences

### User-facing behaviour

- First-time connect requires an explicit host identity confirmation step.
- Returning connects are simpler (pinned identity check is automatic).
- Hosts see join requests and can allow once or remember participants.
- Passwords are not remembered and must be re-entered each time.

### Engineering impact

- The transport layer must expose enough peer identity material to compute stable identity fingerprints.
- The app must implement:
  - identity key generation and storage
  - host identity pinning and mismatch handling
  - allowlist storage and lookup
  - join request queue / UX
  - password handling without persistence

## Follow-ups

- Note / ADR: rendezvous and NAT traversal scope (discovery is not a trust anchor).
- Define the exact identity code encoding (length, alphabet) so intentional collisions are impractical and the code is typo-resistant.
