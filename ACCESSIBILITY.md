# Digimon World 2 accessibility

This development fork adds native narration and navigation for **Digimon World 2
USA (SLUS_011.93)**. It reads game state in the emulator core and submits speech
to RetroArch's accessibility backend. It includes menu and dialogue reading,
battle information, named NPCs, domain objects, floor announcements, direct
portal guidance, and story objectives.

The accessibility source is on the `dw2-accessibility` branch. It starts from
upstream commit `ee042b73f8fe2aa9c8c73408b5bf200a3ce1a67b`, the base used for the
tested development build. Integrating newer emulator changes is separate work.

## Matching frontend

Use the `accessibility/core-speech` branch of
[buu420/RetroArch](https://github.com/buu420/RetroArch/tree/accessibility/core-speech)
with accessibility enabled. This source uses the proposed experimental
`RETRO_ENVIRONMENT_ACCESSIBILITY_SPEAK` command, currently numbered
`95 | RETRO_ENVIRONMENT_EXPERIMENTAL`. The allocation is provisional pending
upstream review. Core and frontend headers must agree.

Earlier private prototypes used command 82. That identifier is now used by an
unrelated upstream command; those prototypes must stay paired with their matching
frontend. The source on this branch uses 95. Ordinary frontend message fallbacks
do not guarantee screen-reader speech.

## Building

Use Python 3.10 or newer and the normal Beetle PSX compiler/build dependencies.
Generate the fallback dialogue table from your own unmodified USA game image:

```sh
python tools/generate_dw2_messages.py --bin "/path/to/Digimon World 2 (USA).bin"
make -j4
```

The generator accepts a MODE2/2352 BIN image. It verifies the boot executable and
the resulting message table against the tested profile before writing
`accessibility_dw2_messages.inc`. This generated file contains game dialogue and
is excluded from Git. No game image, extracted dialogue table, or proprietary
BIOS is distributed here. The live text decoder remains in the core; the generated
table preserves the existing fallback behavior.

For the hardware renderer, run `make clean` and then `make HAVE_HW=1 -j4`.
On Windows with MSYS2/MinGW-w64, use `platform=windows_x64` in the make command.
Follow the upstream README for general dependencies and other platforms.

## Navigation controls

- Home / End: change the navigation category.
- Page Up / Page Down: change the selected target.
- Delete: repeat the current target or guidance direction.
- Numpad Enter: start or stop guidance.

Game Focus may be needed so RetroArch passes keyboard input to the core. Story
guidance uses direct floor portals and mapped mission encounters on final floors.
NPCs, enemies, and available objects are listed for the current area or floor.

## Development status

This is an experimental source publication. The Windows development build has
been used for gameplay and tested with automated DW2 checks; complete playthrough
coverage and native speech on other platforms remain unverified. This branch's
publication checks also verify that the generated fallback table matches all
14,543 entries in the tested prototype.

Diagnostic traces are written to an `accessibility-traces` directory under the
save directory. Review logs before sharing them; they can contain game text and
local paths. Report the game version, location/menu, input, expected speech, and
actual speech when filing a bug.

## License

The emulator and accessibility changes are distributed under the existing GPL
terms in `COPYING`. Upstream dependency notices remain in their source files.
Game content and generated game data are not relicensed by this project.
