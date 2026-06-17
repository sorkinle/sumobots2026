# Copilot Instructions — sumobots2026

## What this repository is

This is a **specification-first** repository for the **ESP32 Sumobot Firmware**. The bulk of it is the SRS
(what to build) and a design companion (how to build it); firmware implementation is just beginning. A first
**connection-test sketch** lives in `firmware/sumobot_connection_test/` (Controller → ESP32 → driver → motor).
Most of the spec is still unimplemented — treat `docs/design/implementation-notes.md` as the authoritative
blueprint for anything you add.

The product (per the spec): an Xbox-controller-driven, 4-motor tank-drive, **push-only** sumobot. Fixed
hardware: ESP-WROOM-32, two BTS7960 H-bridge drivers (one per side, two motors in parallel each), 4× Pololu
37D gearmotors, 4S LiPo + LM2596 buck. Input is an Xbox Series controller over **Bluetooth LE** via Bluepad32.

## Build, test, and TDD

Firmware uses the **Arduino IDE** (ESP32 core + the Bluepad32 board/library), **not** PlatformIO. Build and
flash by opening a sketch folder (e.g. `firmware/sumobot_connection_test/`) in the Arduino IDE and uploading
to an ESP32 Dev Module. The `docs/` tree is plain Markdown — nothing to build or lint.

**Test-driven development is the rule for firmware logic.** Write or update a host test first, watch it fail,
then implement until it passes. This works because all decision logic (mixing, the ARMED gate) lives in
**Arduino-free** `.h`/`.cpp` files that the `.ino` `#include`s; hardware glue (LEDC/GPIO, Bluepad32) stays in
thin `.ino`/wrapper files that are not unit-tested. Host tests live in `firmware/test/` as plain C++ (no
framework) and run on any machine:

    cd firmware/test
    g++ -std=c++17 -I ../sumobot_connection_test test_drive_logic.cpp \
        ../sumobot_connection_test/drive_logic.cpp -o test_drive_logic && ./test_drive_logic

(`clang++` or `python -m ziglang c++` are drop-in replacements for `g++` if it isn't installed.)

## Documentation architecture (the big picture)

The docs encode a deliberate **WHAT vs HOW** split — read across both to understand any feature:

- `docs/srs/` — the **WHAT**, written with the **Problem-Based SRS method (Gorski & Stadzisz)**. Behavior only,
  **no code snippets** (FR files even carry `<!-- ⚠️ NO CODE SNIPPETS -->` markers). Structured as a strict
  trace chain: **Customer Problem (CP) → Customer Need (CN) → Functional/Non-Functional Requirement (FR/NFR)**.
  - `00-context.md` … `04-software-vision.md` — context, problems, needs, glance, vision (read in order to onboard).
  - `functional-requirements/FR-0NN.md`, `non-functional-requirements/NFR-0NN.md` — one requirement per file.
  - `_index.md` summary tables in each requirements folder, plus a bidirectional `traceability-matrix.md`.
- `docs/design/implementation-notes.md` — the **HOW**: pin map, LEDC/PWM config, source-tree layout, main-loop
  pseudocode, per-subsystem C++ sketches, bring-up procedure, and a design-section → FR/NFR trace index.
  Every design section cites the FR/NFR it implements.

The full firmware will map **1-to-1 to the 8 SRS components** (BLE input, mixer, motor output, state machine,
failsafe, config/NVS, status LED, serial/telemetry) — keep that correspondence as you add code. (The current
connection-test sketch is a thin vertical slice of just the input → mixer → motor-output path.)

## The one safety invariant that drives the design

Operator input may reach the motors **only** through the State Machine's single `ARMED` gate, and there must be
**exactly one PWM write site** (`MotorPair::apply`) guarded by one `!armed` early-return. This is FR-010 / NFR-003
and is the most important constraint in the whole spec. Any firmware you write must preserve it (one guard,
grep-verifiable, covered by a state-injection unit test). Anything other than `ARMED` ⇒ both sides PWM = 0.

## Conventions to follow when editing docs

- **Preserve the trace chain.** A new FR/NFR must cite its CN (and through it a CP). When you add or change a
  requirement, update the folder `_index.md` table **and** `traceability-matrix.md` so the zigzag stays orphan-free
  (every CP→CN, every CN→FR, every FR/NFR→CN).
- **Keep WHAT and HOW separate.** Never put code, GPIO numbers, libraries, or timings into `docs/srs/` — those
  belong in `docs/design/`. SRS statements stay solution-free.
- **FR/NFR file shape** is fixed: `# FR-0NN: Title`, then `## Requirement` (Statement, Priority, Status),
  `## Traceability` (table to CN/CP), `## Acceptance Criteria` (`- [ ]` checkboxes), `## Notes`.
- **IDs are zero-padded** (`FR-001`, `NFR-001`) and CP/CN use dotted form (`CP.1`, `CN.15`).
- **Priority** uses MoSCoW (Must/Should/Could/Won't); **Status** starts at `Draft`.
- **Every doc ends with a metadata footer**: a `---` rule then `*Created: YYYY-MM-DD*` / `*Last Updated: YYYY-MM-DD*`
  (FR/NFR files also add `*Author:*`). Update `Last Updated` when you change a file.

## Scope guardrails (from the spec)

Out of scope by design — don't add them without an explicit spec change: autonomy / opponent detection, edge/line
detection, weapon control (the bot is push-only), OTA updates, mobile app, and cloud features. Current sensing
(BTS7960 IS pins) and non-USB telemetry are explicitly deferred to v2.
