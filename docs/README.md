# SM_TD documentation

This directory contains the user and contributor guides for SM_TD 0.6.5-SNAPSHOT.

## Recommended reading order

1. [Overview](000_overview.md): understand the tap-hold timing model.
2. [Installation](010_manual_installation_guide.md): install manually or as a QMK community module.
3. [Customization](050_customization.md): define actions for your keys.
4. [Examples](060_customization_examples.md): copy practical home-row mod, layer-tap, and multi-tap patterns.
5. [Timeouts](070_customization_timeouts.md): tune tap, sequence, and release timing.
6. [Feature flags](080_customization_features.md): enable behavior globally or per key.
7. [Debugging](040_debugging.md): inspect decisions when a layout behaves unexpectedly.

## Guide index

| Guide | Purpose |
|---|---|
| [Overview](000_overview.md) | Concepts, event sequences, and the decision model |
| [Manual installation](010_manual_installation_guide.md) | Files, QMK settings, and integration steps |
| [Releases](015_releases.md) | Version history and notable changes |
| [Upgrade instructions](020_upgrade_instructions.md) | Required changes when moving between versions |
| [Known problems](030_known_problems.md) | Current limitations and workarounds |
| [Debugging](040_debugging.md) | Debug output and troubleshooting workflow |
| [Customization](050_customization.md) | Action hooks and configuration API |
| [Customization examples](060_customization_examples.md) | Complete patterns for common layouts |
| [Timeouts](070_customization_timeouts.md) | Global and per-key timing controls |
| [Feature flags](080_customization_features.md) | Optional behavior and feature selection |
| [Adding tests](090_test_template.md) | Unit and integration test authoring guide |

## Quick validation checklist

After changing a keymap configuration:

- Confirm `DEFERRED_EXEC_ENABLE = yes` and sufficient deferred executors.
- Keep `process_smtd()` first in `process_record_user()` for manual installations.
- Verify every custom action returns the intended `smtd_resolution`.
- Test a tap, a lone hold, a rolling pair, and the corresponding release order.
- Enable debug logging only while troubleshooting.

For repository development, read [CONTRIBUTING.md](../CONTRIBUTING.md). Internal architecture and historical investigations are in [AGENTS.md](../AGENTS.md).
