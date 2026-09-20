# DragonwildsSpellBar

A UE4SS mod for RuneScape: Dragonwilds. A spell bar with up to eight key-bound slots above the item bar, a spell picker built from the game's own spellbook slots, cooldown shading, and larger watering cans and compost buckets. The release zip includes UE4SS.

## Install

1. Download `DragonwildsSpellBar.zip` from the Releases page.
2. In Steam, right click RuneScape: Dragonwilds, choose Manage, then Browse local files. The folder that opens is the game folder. It holds `RSDragonwilds.exe` and a folder named `RSDragonwilds`.
3. Extract the zip into the game folder. The zip's `RSDragonwilds` folder merges into the existing one; accept the merge. The mod lands in `RSDragonwilds\Binaries\Win64\ue4ss\Mods\DragonwildsSpellBar`.
4. Start the game. Open the inventory, right click a slot on the bar, and left click a spell. Keys are `F5` to `F8` until you change `mods.ini`.

To remove the mod and UE4SS, delete `RSDragonwilds\Binaries\Win64\dwmapi.dll` and the folder `RSDragonwilds\Binaries\Win64\ue4ss` while the game is closed.

## Settings

`RSDragonwilds\Binaries\Win64\ue4ss\Mods\DragonwildsSpellBar\mods.ini`. Edits apply while the game runs.

```ini
[Debug]
Enabled = 0

[SpellBar]
Enabled = 1
Scale = 1
Slot1 = F5, USD_HomeTeleport
Slot2 = Ctrl+Q, USD_Surge
Slot3 = F7,
Slot4 = F8,

[Containers]
Enabled = 1
CompostBucket = 5000, 50
WoodWateringCan = 500, 50
BronzeWateringCan = 1000, 50
SteelWateringCan = 2000, 50
AdamantWateringCan = 3000, 50
RuneWateringCan = 5000, 50
```

| Key | Values |
| --- | --- |
| `[Debug] Enabled` | `1` writes diagnostic lines to `RSDragonwilds\Binaries\Win64\ue4ss\UE4SS.log`. |
| `[SpellBar] Enabled` | `0` removes the bar. |
| `Scale` | `0.5` to `2`. |
| `SlotN` | `key, spell`. One to eight slots. Right click a slot in game to pick the spell. |
| `[Containers] Enabled` | `0` leaves the game's container values. |
| Container lines | `name = capacity, amount per use`. |

Keys: `A` to `Z`, `Zero` to `Nine`, `F1` to `F24`, `Num_Zero` to `Num_Nine`, `Ins`, `Del`, `Home`, `End`, `Page_Up`, `Page_Down`, with optional `Ctrl+`, `Shift+`, `Alt+`. A press casts only in gameplay and only when the spell is on a spell wheel page.

Spells, by skill:

| Skill | Spells |
| --- | --- |
| Agility | `USD_PhaseDash`, `USD_Recall` |
| Artisan | `USD_FerociousFurnace`, `USD_MagicalMending`, `USD_SpeedupStation`, `USD_Treequipment` |
| Attack | `USD_EnchantWeapon_Air`, `USD_EnchantWeapon_Fire`, `USD_EnchantWeapon_Water`, `USD_TempestShield` |
| Construction | `USD_AccessPersonalChest`, `USD_EyeOfOculus`, `USD_SummonShelter` |
| Cooking | `USD_BonesToPeaches`, `USD_InternalAlchemy` |
| Farming | `USD_Farming_Compost`, `USD_Farming_RapidGrowth`, `USD_Harvest`, `USD_Humidify` |
| Fishing | `USD_FishCyclone`, `USD_FishingFrenzy`, `USD_InfernalRod` |
| Magic | `USD_Confuse`, `USD_Enfeeble`, `USD_Surge`, `USD_Vengeance` |
| Mining | `USD_DetectOre`, `USD_DivineRock`, `USD_Rocksplosion` |
| Ranged | `USD_CorruptionArrows`, `USD_SnareTrap` |
| Runecrafting | `USD_DetectAnimaVents`, `USD_FireSpirit`, `USD_HomeTeleport`, `USD_RunesToRuneEssence`, `USD_Windstep` |
| Woodcutting | `USD_AxtralProjection`, `USD_PileEmUp`, `USD_Splinter` |

## Build

Windows, with Visual Studio, CMake, and LLVM installed:

```bat
cmake --workflow --preset windows
```

`build\windows\DragonwildsSpellBar.zip` is the mod. Tests, on Linux:

```bash
cmake --workflow --preset tests
```
