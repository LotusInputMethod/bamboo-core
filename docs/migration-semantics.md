# C++ engine migration semantics

This document freezes the intended semantic contract between the legacy Go engine and the migrated C++ engine.

## Scope

- **Reference implementation:** the existing Go engine in the repository root.
- **Target implementation:** the C++ engine behind `include/bamboo/IEngine.h`.
- **Primary goal:** match Go typing behavior for end users before performing deeper performance refactors.

## Go behavior

The Go engine exposes a broad flag- and mode-based API:

- Per-call modes include Vietnamese, English, tone-less, mark-less, lowercase, full-text, punctuation mode, and reverse-order typing.
- Engine flags include free tone marking, standard tone style, and autocorrect.
- `ProcessKey` receives the mode for each call instead of storing mode inside the engine.
- `RemoveLastChar(refreshLastTone)` optionally recomputes the latest tone target after deletion.
- `RestoreLastWord(false)` breaks the latest transformed word back into raw key strokes.
- `RestoreLastWord(true)` rebuilds the latest word into Vietnamese transformations.

## C++ target behavior

The C++ API intentionally narrows the public surface:

- Public mode is stateful and only exposes `Vietnamese` and `English`.
- `processKey(char32_t)` and `processString(std::string_view)` operate against the current mode set via `setMode`.
- `removeLastChar(bool refreshLastToneTarget)` must preserve the Go deletion semantics.
- `restoreLastWord(bool toVietnamese)` must preserve the Go restore semantics:
  - `false`: break the latest transformed word back to raw key strokes and switch to English mode.
  - `true`: rebuild the latest word as Vietnamese composition and switch to Vietnamese mode.
- Backspace (`'\b'` and `0x7f`) in Vietnamese mode is treated as deletion, not as ordinary input.

## Required parity areas

The following behaviors are treated as **must-match** relative to Go:

1. Input-method transformations for Telex, VNI, and VIQR.
2. Tone placement and tone retargeting after edits.
3. `removeLastChar(...)` behavior, including optional tone refresh.
4. `restoreLastWord(...)` behavior in both directions.
5. Backspace handling while in Vietnamese mode.

## Intentional deviations

The following differences are currently intentional and must be documented rather than treated as regressions:

1. The C++ public API does **not** expose Go's per-call mode bitmask directly.
2. Go flags are currently treated as internal policy rather than stable public API in C++.
3. Punctuation/full-text/reverse-order/tone-less/mark-less output modes are not yet part of `IEngine`.

When one of these items needs to be supported in `fcitx5-lotus`, it should first be added to this document before changing the public contract.

## Migration rule

Before optimizing the hot path further, every semantic change must satisfy one of these conditions:

- it is covered by a Go-vs-C++ differential test, or
- it is listed in **Intentional deviations** above.

If neither condition is true, the change is considered unsafe.
