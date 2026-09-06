# Atomic Persistence And Schema Migration

## The durable replacement pattern

For a small complete record in one directory, a typical update is:

1. Create a uniquely named temporary file with restrictive permissions.
2. Write the complete new representation and check every return value.
3. Flush the file with `fsync()` or the required stronger primitive.
4. Rename it over the old name within the same filesystem.
5. Open and synchronize the containing directory when directory durability matters.
6. Reopen and validate the record after restart.

`rename()` gives readers an atomic name transition, but not necessarily durable storage by itself. Cross-filesystem renames fail, and a successful `close()` is not a substitute for the required synchronization. For larger data, use a journal, append-only generations, or a database whose durability contract is understood.

## Record design

Include a magic value, schema version, length, generation, and integrity check. Reject impossible lengths before allocation. A checksum detects accidental corruption; authentication or a signature is needed when an attacker can modify the medium. Generation numbers help reject stale copies and make recovery decisions deterministic.

Keep the old record until the new one is known to be durable. On load, validate candidates completely, choose the newest valid generation, and explicitly report whether recovery used a fallback. Never “repair” by overwriting the only surviving copy before the replacement has been verified.

## Schema migration

Migrations should be bounded, idempotent, and transactional from the perspective of the application. Prefer read-old/write-new conversion into a new generation. If a migration can fail, preserve the old version and make the service able to retry or roll back. Test upgrades across every supported version, interrupted migration points, missing fields, unknown fields, and oversized values.
