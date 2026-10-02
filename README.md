# N64 Utilities

A small, native Nintendo 64 utility ROM inspired by the joystick display in
[sanni/controllertest](https://github.com/sanni/controllertest). This is a fresh
implementation; it does not reuse that project's source or artwork.

The compact main menu contains **Controller test**, **Rumble Pak test**,
**Audio test**, and **Controller Pak**. Navigate with D-pad up/down; A or Start opens the selected test.

- Large raw joystick plot with an octagonal reference, continuous movement trail,
  signed X/Y readings, and observed minimum/maximum values.
- All 14 digital inputs: A, B, Start, Z, L, R, four C buttons, and four D-pad
  directions. Original procedural graphics use the controller's familiar shapes
  and colours. Released buttons are dim greyscale; pressed buttons brighten.
- All four controller ports, with independent histories and disconnect detection.
- 320 × 240, triple-buffered output with no Expansion Pak requirement.

## Controls

Use the D-pad to choose a utility. Press **A** or **Start** on any connected N64 controller to open it for
that port. P1–P4 at the top show connection status; the active port is highlighted
and underlined, and disconnected ports are dimmed.

- Hold **Start on the active controller for 1.2 seconds** to return to the menu.
- Hold **Start on another controller for 1.2 seconds** to switch to it. A bar
  labelled `Switch to P2` (or the relevant port) fills while holding.
- Release early or unplug the requesting controller to cancel the gesture.
- After opening, switching, or exiting, release any held Start buttons before
  another hold can trigger an action. This prevents accidental exits or bouncing
  between controllers.

The first hold owns the progress bar until it completes or is cancelled.
Simultaneous holds prefer the active port, then the lowest port number. Each
connected controller continues collecting its own trail and extrema while
another port is displayed. Switching preserves that history. Re-entering from
the menu resets every history; unplugging a controller clears only its data.
If the active controller disconnects, its screen stays selected and another
controller can take over with the same Start hold.

The stick is displayed without a deadzone, smoothing, or software calibration.
Positive Y points up, and the full signed 8-bit range fits on the plot. The gate
is illustrative (85 at cardinal points, 60 per diagonal axis), **not a pass/fail
boundary**. The cardinal value follows libdragon’s approximate healthy OEM
[stick range](https://github.com/DragonMinded/libdragon/blob/e356bf3f56f7afbf7e5246329562f145965cfdfc/include/joypad.h);
the diagonal shape is illustrative. Controller models and wear differ. The trail holds the most recent
128 samples joined in time order, with older segments dimmed; extrema cover the current test session. Start is labelled `S` on its
red round button. The first version accepts N64-style controllers only.

## Rumble Pak test

Insert a Rumble Pak into the selected controller. The screen shows detection
status and the requested motor state. **Hold A** for manual rumble, **press B**
for three pulses (150 ms on / 150 ms off). Release the
menu-opening A press before operating the motor. Start stops the motor while
you hold to exit. Switching ports cancels the pulse sequence and stops the old
port; the new port requires a fresh button press. A missing Pak cannot activate
rumble. Detection and the indicator do not confirm that the physical motor or
its batteries are working: this is a tactile test.

## Audio test

**A** starts or stops playback; its on-screen label changes between Play and Stop. D-pad **left/right** selects left, both,
or right channels. D-pad **up/down** selects 220 Hz, 440 Hz, or 1 kHz. Output is
a sine wave at 20% digital full-scale peak, with a 5 ms fade when starting,
stopping, or changing channels. Playback starts muted. The audio callback uses
the hardware's actual sample rate. Switching controllers, disconnecting the
active controller, or holding Start stops playback. Queued audio may take a
short moment to drain. The L/R indicators identify the requested output routing;
listen to confirm your console/cable/speakers reproduce it correctly.

## Controller Pak inspector

A browser for standard Controller Pak save directories. Browsing and rescanning
are read-only; explicit write operations have a separate confirmation screen. Use D-pad
**up/down** to select a note, **B** to toggle details, and **A** to rescan.
Hold Start to exit or hold Start on another controller to switch ports, as in
the other utilities. The list pages automatically after six notes.

The inspector shows used/free capacity out of 123 usable 256-byte blocks,
occupied directory slots out of 16, save names and per-save block counts.
Details include the directory slot, byte size, and raw hexadecimal game,
vendor, and region identifiers from libdragon. These are stored metadata, not
names resolved against a game database.

Scans use one bounded read operation per frame. Header and directory signatures
are checked again before publishing a scan, then periodically while browsing.
A changed Pak triggers a fresh scan. Removing a controller/Pak or encountering a
read error clears cached results. Read errors and invalid/unformatted filesystems
are distinct states; press A to retry after addressing them. Other accessories
are identified as not being Controller Paks. Each scan is numbered, and pressing
A highlights a rescan acknowledgement for 750 ms even when validation fails
immediately. Validation failures show header-copy checks, both allocation-table
checks, and all-zero/all-FF header detection from read-only sector reads. These
are diagnostics against the pinned libdragon validation rules, not proof that
stored saves are unrecoverable. For manually bank-switched cards, only the
currently selected bank is visible; the ROM cannot operate physical switches.

Nonempty slots rejected by libdragon remain visible as invalid entries. A
warning is shown if entries are invalid or their total blocks disagree with
allocated capacity. These checks do not establish that a game's save payload is intact.

### Write operations

- **C-up: Repair** redundant header/allocation-table copies. The plan uses a
  checksum-valid source; differing valid copies, missing sources, and malformed
  allocation pointers are refused. It does not reconstruct lost save data or
  fix arbitrary block-chain corruption.
- **C-down: Delete** the selected valid save note permanently.
- **C-right: Format** the currently selected bank, erasing all its saves.

Each operation first reads the bank metadata and shows a review screen. Release
A, then **hold A for two seconds** to confirm, or **B to cancel**. Holding Start
also cancels. Keep the Pak inserted and its bank switches unchanged during the
operation. The reviewed metadata is reread before writing; a mismatch cancels
without writing. Writes are verified afterwards. A write or verification error
may mean a partial operation; there is no automatic rollback. B returns to the
inspector and rescans. No restore or export operation is implemented yet.

Write/repair operations have only been tested against simulated cards so far.
Use a spare bank with no needed saves for initial hardware validation.

## Build

The recommended project setup is the official libdragon toolchain container,
with **both the container digest and libdragon commit pinned** in
[`tools/Dockerfile`](tools/Dockerfile). It uses libdragon's stable `trunk` API,
RDPQ for hardware rendering, the joypad subsystem for input, and the normal
`n64.mk` ROM pipeline and open-source boot code. No proprietary Nintendo SDK,
JavaScript build tooling, external art, or preview-branch features are needed.

Install Docker, then run:

```sh
./tools/build-rom.sh -j4
```

The first build downloads the compiler and compiles libdragon. Subsequent builds
reuse the Docker cache. Output: **`n64-util.z64`**. Build files are owned by your
user. To clean: `./tools/build-rom.sh clean`.

With a native libdragon installation instead:

```sh
export N64_INST=/path/to/your/n64/toolchain
make -j4
```

See the [official installation guide](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon)
and [libdragon documentation](https://libdragon.dev/ref/) for the underlying SDK.
To update dependencies, deliberately update the Dockerfile revision/digest and
rebuild; the normal build does not silently follow the newest library version.

## Deploy to SummerCart64

With `just` and `sc64deployer` installed, leave the SummerCart64 connected over
USB and power off the N64 so it releases the SD-card lock. The SD card must
already have a `/CUSTOM` directory. Run:

```sh
just deploy
```

This builds the ROM with the pinned toolchain, then copies it directly to
`/CUSTOM/n64-util.z64` on the connected cartridge, replacing the previous copy.
The copy command is:

```sh
sc64deployer sd upload n64-util.z64 /CUSTOM/n64-util.z64
```

After the command finishes, power on the N64 and select the ROM in `/CUSTOM`.
`just build`, `just check`, and `just clean` are also available.

## Run and verify

Load `n64-util.z64` on an N64 flash cartridge, or open it in
[ares](https://ares-emu.net/) with Homebrew Mode enabled. Libdragon recommends
accurate emulators; older HLE-only configurations may fail to render the ROM.

Host checks require a C compiler and libm:

```sh
./tools/check.sh
```

This runs input-state regression tests and creates SVG layout previews in
`build/preview-*.svg` using the **same UI drawing code**. Preview text uses a host
font; the ROM uses libdragon's built-in font. Previews do not emulate N64 RDP
rendering or timing.

Validation completed during initial implementation: ROM cross-compilation with
`-Wall -Wextra -Werror`, host input-state tests, and visual layout inspection.
Basic rumble and audio output have been confirmed on hardware by the user.
The revised controls and Controller Pak reads still need hardware testing. Host tests cover gesture
cancellation, simultaneous holds, disconnection, per-port history, menu entry,
rumble pulse sequencing, output shutdown, and generated PCM channel isolation,
frequency, amplitude bounds, and the fade to silence. Inspector tests use a
read-only fake card backend to cover full/empty directories, note details,
malformed metadata, inconsistent totals, read failures, hot-swaps, mid-scan
changes, port switching, selection, and rescan controls.

Before releasing a hardware-tested version:

1. Boot with and without a controller; unplug and reconnect during the test.
2. Check all buttons individually and in combinations, including opposite D-pad
   directions where the controller permits them.
3. Rotate the stick through the gate, check all quadrants, release it to inspect
   centre drift, and compare readings against a known controller.
4. Open from each port, switch among all four, cancel a partial hold, unplug
   the active/requesting controller, and try simultaneous Start holds. Confirm
   switching preserves per-port history and re-entering resets all histories.
5. Verify manual rumble, all three pulses, missing/reinserted Paks, and
   that switching/exiting stops the old motor.
6. Verify left/right/both audio routing, all frequencies, play/stop, and silence
   after switching/exiting/disconnecting.
7. Inspect an empty and a populated Controller Pak, scroll through every note,
   compare capacity against another Pak manager, and check rescan/removal/swap
   behavior. Check an unreadable Pak only if one is already available.
8. Check NTSC/PAL timing, CRT overscan, and sustained frame rate on real hardware.

## Source map

- `src/main.c`: libdragon startup, controller adapter, and RDPQ drawing backend.
- `src/app.c`: menu, input, hold gesture, and stick history.
- `src/ui.c`: shared layout and procedural button graphics.
- `tools/preview.c`: host SVG drawing backend for layout review.
- `src/tone.c`: portable stereo sine generation and gain ramps.
- `tests/app_test.c`: portable input-state regression tests.
- `tests/utilities_test.c`: utility controls, rumble sequencing, and PCM tests.

- `src/pak.c`: portable incremental inspector and cache invalidation.
- `src/pak_n64.c`: libdragon Controller Pak read/write adapter.
- `tests/pak_test.c`: inspector backend and navigation regression tests.

- `src/pak_write.c`: repair planning, metadata snapshot checks, and verified writes.
- `tests/pak_write_test.c`: confirmation, ambiguous repairs, card changes and write errors.
