# wrocpp-unit-in-integer-name

## Mistake

A quantity with a unit is stored in a plain arithmetic type, with the unit only in its name:
`timeoutMs`, `ttl_sec`, `bufferBytes`. Nothing stops a value in seconds from being assigned to a
variable in milliseconds.

## Evidence

The proprietary service: a root-cause defect where a TTL in seconds was assigned to a timeout in
milliseconds, followed by a campaign of 8 or more refactor commits; 99 integer declarations with a
unit suffix remained outside tests. cloudevents-sdk-cpp: its CLAUDE.md requires every time value to
be a `std::chrono` type (2a05d72, cce2e5e).

## Must flag

Variables, fields and parameters of integer or floating type whose name ends in a configured unit
suffix (default: `Ms`, `_ms`, `Millis`, `Us`, `_us`, `Ns`, `_ns`, `Sec`, `Secs`, `Seconds`,
`_s`, `_sec`, `Bytes`, `_bytes`, `Kb`, `Mb`).

## Must not flag

- The same names with a `std::chrono::duration` or a units-library type.
- Files matching the configured boundary paths (wire formats, database rows, config readers),
  where a raw integer is the external representation.

## Form

Query check. Suffixes and exempt paths are set by editing the matcher's `matchesName` regex.
