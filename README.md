# Conveyor Upgrade Wrench

Buy Conveyor Engineering in Modded Shop (default $125).

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [conveyor-upgrade-wrench-0.3.0.msmod](content/conveyor-upgrade-wrench-0.3.0.msmod), import it with **Install mod package**, then select the mod and its required frameworks.

Required mods: [shop-framework](https://github.com/swooping-mods/shop-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

## Controls

- Buy Conveyor Engineering in the equipment shop (default $125).
- Hammer U enters Upgrade mode; E upgrades an aimed belt; F10 toggles the legacy wrench mode.
- Native purchase/equipment/hints integration implemented; gameplay checks deferred.

## Compatibility

Version 0.3.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
