# Munsters Pro — Custom Spot Multiball

Project notes and build handoff for the custom Spot mode on **Stern The Munsters Pro 1.28**. This README is intended to give a fresh development session enough context to rebuild and continue the work without the original chat history.

The repository is deliberately a **flat source archive**, not a ready-to-open PAD project. Restore the working layout described below before using the existing launcher and scripts. Do not flatten the live PAD project directory.

## Current behavior and boundaries

The custom C handler replaces stock Spot rule **17**, retaining the native qualification, awards, toy sequences, displays, Super Jackpot system, and other rules unless explicitly changed.

- No Spot mode timer or countdown display.
- Spot level 1 starts a two-ball multiball. Native levels 2, 3, 4 and 5 request three, four, five and six balls respectively. Native level progression after Munster Madness determines the target; it is not a count of how many times Spot has started. The SDK caps requests at six, and the native engine clips to the installed ball count.
- No add-a-ball while custom Spot is active.
- The mode ends through native STOP when the native ball count reaches one after multiball has been established. Initial serving and an unknown count do not prematurely end it.
- Grandpa's optional scoop-eject ball save is suppressed during Spot, without changing the saved adjustment or blocking scoop kickout. It works normally outside Spot.
- Mystery cannot collect during Spot. An earned Mystery remains pending for the same player and is collectable after Spot ends.
- **Secret Mania is intentionally unchanged.** It was audited, but the owner observed that it can coexist with normal multiball and explicitly decided not to block it.
- The custom badge is currently hidden by `"screen": false` in `assets.json`. This hides the optional screen; it does not disable the C mode.
- **Spot's Revenge is not implemented.** Its proposed shot sequence, forced drain, and same-ball relaunch are future design ideas, not current requirements to implement silently.

The comment at the top of the C file summarizes the initial rules and is not a complete specification of every native stacking interaction. Preserve native behavior and the owner's decisions above.

### Current experimental addition: Quick Shot

Quick Shot is enabled in the current source. Each time Spot returns to the native up/ready state, including the opening raise:

1. Open a **5,000 ms** opportunity.
2. Flash the left-ramp RGB rectangle yellow (`LEFT RAMP JACKPOT`, 200 ms blink period).
3. A qualifying Spot opto hit before expiry earns **1,000,000 extra points**, with no shot/boost or playfield multiplier applied to that extra amount.
4. Keep normal Spot scoring and multipliers unchanged. Include the bonus in the hit's displayed award and the native mode total.
5. Release the lamp on collection, expiry, lowering, STOP, player change, or leaving play. Do not rearm merely because Spot remains up after expiry or a hit; wait for a new not-ready-to-ready transition.

The settings are near the top of `spot_baseline.c`:

```c
#define QUICK_SHOT_ENABLED 1
#define QUICK_SHOT_WINDOW_MS 5000ul
#define QUICK_SHOT_BONUS 1000000ull
#define QUICK_SHOT_LAMP "LEFT RAMP JACKPOT"
#define QUICK_SHOT_FLASH_MS 200u
```

Set `QUICK_SHOT_ENABLED` to `0` and rebuild to compare with the preceding stable gameplay. Do not confuse this with `assets.json`'s screen setting.

## Repository files

These seven files preserve the custom production source and its generator:

| Flat repository file | Purpose |
| --- | --- |
| `spot_baseline.c` | Custom Spot shot logic, multiball lifecycle and Quick Shot state machine. |
| `assets.json` | PAD code-mode metadata and optional screen configuration. |
| `pad_munsters.h` | Project-local bridge API, including Quick Shot helpers. |
| `munsters_bridge.inc` | Build-specific native adapters and the Grandpa/Mystery guards. Included inside the runtime, not compiled separately. |
| `pad_mode_runtime.c` | Modified PAD runtime with Munsters' 32-bit shot ABI/mask support and bridge inclusion. An unmodified upstream runtime is not an interchangeable replacement. |
| `munsters_pro-1.28.port` | Checked native function sites, object offsets, lamps, shots and other data for this exact game build. Use the local SDK copy as the canonical file. |
| `extend_sdk.py` | Recreates the runtime adaptations and checked port from an upstream SDK baseline and the original extracted game ELF. It is not needed for an ordinary build when the checked-in runtime and port already exist. |

Also retain these files if the existing launcher/build workflow is to be recoverable directly:

- `Launch Spot PAD.pyw`
- `Launch Spot PAD bootstrap.pyw`
- `Launch Spot PAD.vbs`
- `build_baseline.ps1`

The upstream SDK files below are dependencies but were unchanged from the tested PAD installation. Obtain the matching versions from that installation, or archive them privately with the toolchain:

```text
pad_mode.h
pad_stock.h
pad_mode_assets.h
mode_file.c
build_mode.sh
```

Do not publish the raw card image, extracted `game.stock`, stock audio/video/images, emulator roots, generated shared objects, or logs as part of this source archive. Test scripts and concise test-result records can be versioned separately if desired.

## Supported environment and identity

The tested installation is:

- Windows with PowerShell and WSL 2.
- WSL distribution: `Ubuntu-24.04`.
- ARM hard-float cross-compiler: `arm-linux-gnueabihf-gcc` and its binutils.
- **Pinball Asset Decryptor 1.277.2**, installed at `D:\Program Files\Pinball Asset Decryptor`.
- PAD's bundled Python: `D:\Program Files\Pinball Asset Decryptor\python\python.exe`.
- Existing extracted project: `E:\Stern_Fun\Munsters\Munsters Pro`.
- Original card file: `E:\Stern_Fun\Munsters\munsters_pro-1_28_0.Release.8G.sdcard.raw`.

Upstream project: <https://github.com/davidvanderburgh/pinball-asset-decryptor>

This is a version-specific integration. Do not silently replace its SDK with a newer PAD SDK or auto-update PAD as part of building the mode. If using a different version, compare APIs and validate the launcher/compiler integration explicitly.

The checked port is for **Munsters Pro 1.28**, not Premium/LE or a different firmware revision. PAD may identify the card as `1.28.0`; the local port normalizes to `munsters_pro-1.28.port`.

Identity of the original extracted `/munsters_pro/game` ELF, **not the hash of the whole SD image**:

```text
SHA-256: ee03eba63d0da90439cc878fbfeb7199c92a309a84e990c3680e78020d2a9219
Build ID: b8caff2cf23fc8cce1d5a8ca8aa232b29f204758
```

Use an independently supplied matching game/card. The game ELF is not a source dependency for routine compilation, but it is required for regenerating checked native sites or running original-ARM differential tests.

## Restore the working layout

The existing scripts expect this layout under the extracted PAD project:

```text
Munsters Pro/
  Launch Spot PAD.pyw
  Launch Spot PAD bootstrap.pyw
  Launch Spot PAD.vbs
  modes/
    spot_baseline/
      spot_baseline.c
      assets.json
  analysis/
    stock_spot/
      build_baseline.ps1
      extend_sdk.py                    # optional for routine compilation
      sdk/
        pad_mode_runtime.c             # this repository's modified runtime
        munsters_bridge.inc
        pad_munsters.h
        pad_mode.h                     # matching upstream dependency
        pad_stock.h                    # matching upstream dependency
        pad_mode_assets.h              # matching upstream dependency
        mode_file.c                    # matching upstream dependency
        build_mode.sh                  # matching upstream dependency
        ports/
          munsters_pro-1.28.port        # this repository's custom port
```

Keep only the intended custom Spot mode active under `modes/`. Historical prototypes and backups belong outside that directory; do not register two competing replacements for rule 17.

Example restoration from a flat clone, using the existing extracted project:

```powershell
$repoRoot = 'C:\path\to\your-flat-repo'
$projectRoot = 'E:\Stern_Fun\Munsters\Munsters Pro'
$padRoot = 'D:\Program Files\Pinball Asset Decryptor'
$sdkRoot = Join-Path $projectRoot 'analysis\stock_spot\sdk'
$modeRoot = Join-Path $projectRoot 'modes\spot_baseline'
$upstreamSdk = Join-Path $padRoot 'tools\spike2_emu\modes\sdk'

New-Item -ItemType Directory -Force -Path $modeRoot, $sdkRoot, (Join-Path $sdkRoot 'ports') | Out-Null

foreach ($name in 'pad_mode.h', 'pad_stock.h', 'pad_mode_assets.h', 'mode_file.c', 'build_mode.sh') {
    Copy-Item -LiteralPath (Join-Path $upstreamSdk $name) -Destination $sdkRoot
}
foreach ($name in 'pad_mode_runtime.c', 'munsters_bridge.inc', 'pad_munsters.h') {
    Copy-Item -LiteralPath (Join-Path $repoRoot $name) -Destination $sdkRoot
}
Copy-Item -LiteralPath (Join-Path $repoRoot 'munsters_pro-1.28.port') -Destination (Join-Path $sdkRoot 'ports\munsters_pro-1.28.port')
foreach ($name in 'spot_baseline.c', 'assets.json') {
    Copy-Item -LiteralPath (Join-Path $repoRoot $name) -Destination $modeRoot
}
```

If present in the flat repo, copy the three launcher files to `$projectRoot`; copy `build_baseline.ps1` and `extend_sdk.py` to `$projectRoot\analysis\stock_spot`. Back up existing custom files before restoring over a live project.

The launcher `.pyw` and `.vbs` contain the installed PAD path. Adjust both if PAD lives elsewhere. The working project is located relative to the launcher, so put it at the project root. `extend_sdk.py` also has hardcoded project/installation paths; inspect those before using it.

## Build the code without writing a card

First verify the compiler:

```powershell
wsl -d Ubuntu-24.04 -e arm-linux-gnueabihf-gcc --version
```

From the working project root, if `build_baseline.ps1` is restored:

```powershell
& '.\analysis\stock_spot\build_baseline.ps1'
```

It compiles the current mode plus PAD's mode-file interpreter using the local SDK and writes:

```text
analysis/stock_spot/spot_baseline.so
```

The filename is historical: it contains the current custom multiball/Quick Shot code, not automatically the original stock-rule baseline.

Equivalent explicit WSL command, independent of the optional PowerShell build helper:

```powershell
wsl -d Ubuntu-24.04 -e bash `
  '/mnt/e/Stern_Fun/Munsters/Munsters Pro/analysis/stock_spot/sdk/build_mode.sh' `
  -o '/mnt/e/Stern_Fun/Munsters/Munsters Pro/analysis/stock_spot/spot_baseline.so' `
  '/mnt/e/Stern_Fun/Munsters/Munsters Pro/modes/spot_baseline/spot_baseline.c' `
  '/mnt/e/Stern_Fun/Munsters/Munsters Pro/analysis/stock_spot/sdk/mode_file.c'
```

The build script supplies the runtime itself, includes `munsters_bridge.inc` through that runtime, uses ARM hard-float/PIC/shared-library flags with no linked libc, and checks for unresolved `pm_*` calls and forbidden dependencies. Do not compile the `.inc` as another translation unit or add the runtime twice.

A successful compile does not prove the correct port will be loaded. The port is checked at runtime; it is not an argument to this compiler command.

## Verify and run through PAD

Launch PAD using **`Launch Spot PAD.vbs` from the project root**, not the ordinary installed PAD shortcut. The VBS calls the bootstrap, which runs `Launch Spot PAD.pyw`.

The Python launcher is essential to the existing workflow because it:

- Selects the project-local SDK and its custom port directory.
- Redirects the GUI's separate code-only compiler command to the local `build_mode.sh`.
- Leaves rig/tool scripts in the installed PAD tools directory.
- Sets `PAD_CODE_CACHE=0`.
- Adds `[SPOT SDK]` to the window title and a local-SDK startup log entry.

Restart through this launcher after SDK or port changes; an already-running app may retain old imports/port state.

Two non-GUI checks are available when the launcher is restored:

```powershell
Set-Location 'E:\Stern_Fun\Munsters\Munsters Pro'
& 'D:\Program Files\Pinball Asset Decryptor\python\python.exe' '.\Launch Spot PAD.pyw' --check-build
& 'D:\Program Files\Pinball Asset Decryptor\python\python.exe' '.\Launch Spot PAD.pyw' --check-gui-build
```

The second command exercises the actual code-only GUI compiler method. Both commands compile without modifying a raw SD image. They produce `launcher_check.so` and `gui_compiler_check.so` under `analysis/stock_spot`.

In PAD, open the extracted project, select the matching original card for Emulate, enable the Modes preview feature if necessary, and use **Try it** on the code mode. Look for the native hook being armed and `Quick Shot enabled: 5000ms, flat 1000000` in `mode.log`. Do not infer that the mode is off merely because its optional custom badge is hidden.

With `screen: false` and no other custom assets, this is a **code-only mode**. The old assets-only `build_multiball_set.py` helper can report "There are no modes in this project to try" in that situation; that is not evidence that the C mode is disabled. Use the GUI code-only route or the compiler checks above.

To create a machine image, use PAD's normal image-write/build workflow from this locally configured app, verify its log says it built the C mode, and write/burn a separate output image. Compilation alone is not an image build. Preserve the working stable card/image for comparison.

## Native integration details for future changes

The original Spot shot handler is `0xd3ca8`, rule 17. Munsters uses a **32-bit shot argument in r1** and four-byte per-player masks. The upstream generic stock-rule implementation assumed a different 64-bit ABI/layout; the local runtime's `stock_shot_bits=32` adaptations are necessary.

Relevant local port settings:

```text
stock_slot_shot   25
stock_slot_start  33
stock_slot_stop   36
stock_slot_active 37
stock_field      0x20
spot_phase_at    0x80
spot_hits_at     0x8c
```

`pad_mode_runtime.c` includes `munsters_bridge.inc` at its end, after its internal native/port helpers and stock-rule types are defined. The bridge is intentionally part of that translation unit.

The native mode object is still a timed-mode object even though our lifecycle is multiball. Therefore native "is a multiball running?" checks do not automatically recognize custom Spot. The explicit Grandpa-save and Mystery guards address confirmed behavior. Do not globally reclassify Spot or add more restrictions based solely on finding another such check.

The bridge controls the native timer's pause flag while keeping its process active, so qualification and active-state queries work. Only Spot START's checked clock-registration call is skipped. Other modes keep their timers.

The native shot rules remain important: the Spot opto is `0x800000`; right ramp `0x8` or left orbit `0x1` can raise Spot after the Super stage. Left ramp `0x4` is not an interchangeable raise shot. Preserve the exact opto/composite-mask handling, award buckets, boosts, hit counters, and native Super Jackpot handling.

Quick Shot uses the checked toy getter `0x414bc` for device 6 and verifies its vtable. Native phase 1 plus toy state `+8 == 2` defines readiness. This is commanded state, not an independent physical position sensor. No custom coil commands are introduced.

The Quick Shot flat-score helper follows the SDK's existing score-table/zero-point-refresh approach used by `pm_score_sub`: increment the current player's score and Spot's native earned total directly, then invoke native scoring with zero points for its normal score-change refresh. It does not temporarily change multipliers or increase the base award. Keep this distinction when refactoring scoring.

Mode START/STOP callbacks run inside native dispatch. Initial multiball serving is queued to a subsequent tick. The existing `spot_mb_guard` stays active through Spot and is cleared by STOP/player/game cleanup. Keep these lifecycle boundaries intact.

## Regenerating the SDK/port is a separate operation

Normally use the versioned modified runtime and port directly. Do **not** run `extend_sdk.py` as an automatic build step.

It requires:

- The matching installed upstream SDK baseline.
- An original extracted game ELF at `analysis/stock_spot/game.stock` matching the identity above.
- Correct hardcoded paths in the script.
- The appropriate `sdk_original` backups when rebuilding an existing workspace.

It rebuilds `sdk/pad_mode_runtime.c` from the original backup plus asserted text replacements and writes the checked port to `sdk/ports/munsters_pro-1.28.port`. It also writes an analysis-directory port copy and `stock_identity.json`. It does not recreate `munsters_bridge.inc` or `pad_munsters.h`; retain their versioned copies. Regeneration can overwrite manual runtime/port edits, so compare results before adopting them.

## Validation and remaining machine tests

Validation status as of **October 30, 2026**:

- The preceding custom multiball version, including Grandpa-save and Mystery fixes, was played on the real machine in repeated two-, three- and four-player games. The owner reported correct tracking and no observed issues.
- **Quick Shot is newer than that machine report.** It passed ARM and emulator tests, but its physical insert mapping, yellow appearance, timing feel, and multiplayer gameplay still need machine playtesting. Do not label it hardware-proven based on the earlier report.
- Quick Shot's compiled ARM tests cover 4,999/5,000 ms boundaries, one bonus per raise, all four players, cleanup, repeat raises, timer rollover, and exact flat scoring with native playfield multipliers 1x through 6x.
- With Quick Shot disabled in the regression fixture, 2,220 comparisons against the original ARM handler passed, excluding the previously deliberate timer changes.
- A live emulator run covered opening and repeated native raises, expiry, post-Super right-ramp raises, yellow lamp ownership/release, native totals, and STOP cleanup. Example: at native 3x, a 450,000 stock award plus Quick Shot scored **1,450,000**, not 3,450,000.
- Grandpa and Mystery each had separate native emulator regressions verifying behavior before/during/after Spot. Mystery's randomized lottery body was suppressed only in its test object so random awards could not invalidate deterministic checks.

The larger local analysis workspace contains optional test scripts and results not required for ordinary builds:

```text
test_differential.py
test_multiball_differential.py
test_multiball_lifecycle.py
test_grandpa_guard.py
test_mystery_guard.py
test_quickshot.py
quickshot_probe_runtime.c
run_quickshot.sh
quickshot_unit_result.json
quickshot_emulator_result.json
```

Do not assume these exist in a minimal flat clone. The ARM tests need their harness companions, matching `game.stock`, the compiled `spot_baseline.so`, and Python packages such as Unicorn/Capstone. The tested bundled Windows Python provided those packages; WSL Python is a separate environment.

The emulator scripts use isolated **slot 1** with a rig lease and a test-only object. Do not install emulator probes or enable `spot_test_controls` in a production port. Preserve any user's running emulator, stop only your test slot, and release it after cleanup.

Quiet emulator runs previously lost apparent native in-play counts after a long interval with no playfield switches. Keep simulated playfield activity going when testing long persistence, and distinguish emulator inventory behavior from actual timer expiration.

The local pre-Quick-Shot snapshot is `analysis/stock_spot/stable_before_quickshot_20261030/`. That snapshot may not be in the flat repo; use Git history to preserve and identify the stable version too.

## Troubleshooting

| Symptom | First check |
| --- | --- |
| `pad_munsters.h: No such file or directory` | The build is probably using the installed SDK. Check the local SDK placement, launcher path, and GUI-specific compiler redirection. |
| Custom C mode builds, but Grandpa/Mystery fix is absent | Check that the current local port and modified runtime/bridge were deployed together; restart the launcher and inspect native hook logs. |
| Quick Shot logs "disabled" | Check the updated port's toy getter/vtable entries, lamp capability and `LEFT RAMP JACKPOT` mapping, and `QUICK_SHOT_ENABLED`. |
| No custom label | Expected with `screen: false`; verify hook/readiness logs and actual gameplay instead. |
| No modes in the assets-only build helper | Use the code-only GUI/compiler route when the mode has no custom assets. |
| Launcher seems to do nothing | Read `analysis/stock_spot/launcher_error.log`; verify the bundled Python path in the VBS. |
| New PAD version or different game firmware | Treat compatibility as unverified. Do not bypass native-site checks or substitute addresses from another build. |

## Continuing development

Start by reading this README, the current C file, the bridge and the checked port. Confirm which variant/image the owner is testing. Make one gameplay experiment at a time, retain a stable comparison version, and preserve all unspecified stock rules. Do not implement Revenge or block Secret Mania unless the owner changes those decisions.
