# Conveyor Upgrade Wrench: behavior and implementation

Describes the retained payload **0.3.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.3.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.3.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Buy the $125 Conveyor Engineering unlock in the equipment shop. U with the normal hammer enters or leaves Upgrade mode; E upgrades the aimed belt and F10 retains the legacy mode toggle. Mk2 costs $25 and adds 1.1 m/s rider movement credit; Mk3 costs $75, requires inventoryTier >= 2, and adds 3.3 m/s. Native spacing, endpoint clamping and handoff still own capsule movement. Instance materials display blue/orange tier colors and are reapplied after rebuild.

## State and lifecycle

Engineering ownership and per-belt tiers in this implementation are session-local. Host/world/tool and focused-input gates guard payment and mutation. World replacement clears the feature records. The nearest actual route point determines upgrade reach. The normal hammer remains native equipment; this is not an additional assigned handheld.

## Integration and configuration

Runtime dependencies: [shop-framework](https://github.com/swooping-mods/shop-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/conveyor-upgrade-wrench/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `shop-framework.shop` | 1 | at least 56 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

Packaged defaults in [settings.ini](../source/mods/conveyor-upgrade-wrench/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `[settings]` | `UnlockKey` | `121` |
| `[settings]` | `UpgradeKey` | `69` |
| `[settings]` | `HammerModeKey` | `85` |
| `[settings]` | `UnlockPrice` | `125` |
| `[settings]` | `Tier2Price` | `25` |
| `[settings]` | `Tier3Price` | `75` |

## Implementation sources

- [mod.c](../source/mods/conveyor-upgrade-wrench/mod.c)
- [building.h](../source/mods/conveyor-upgrade-wrench/building.h)
- [config.h](../source/mods/conveyor-upgrade-wrench/config.h)

## Verification boundary

No injected native Upgrade button, persistent tier format or guest replication protocol. Tread visuals, rider behavior, exact throughput and combined conveyor use require gameplay qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
