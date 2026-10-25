# MCLA

Static recompilation of *Midnight Club: Los Angeles Complete Edition* (Xbox 360, title 545407F8) to native Linux code.

**This repository contains no game code, assets or keys.** You supply your own legally obtained copy; a script extracts what is needed on your machine, and the recompiled code is generated locally and never committed. What you build is for you, not for sharing.

## Status

| Target | State |
|---|---|
| Linux, Vulkan | Builds and plays natively, with audio and a controller. Free roam, races, garage and upgrades work; 30 FPS by default, with 60/120 FPS targets and quality presets. |

Car reflections are fixed (cars no longer come out white with a glow at night):

![The same spot before and after the fix, at night and at noon](docs/images/reflection-fix-before-after.webp)

## Which release of the game

Only **Midnight Club: Los Angeles Complete Edition, USA/Europe, for Xbox 360** (title id `545407F8`), as a disc image (`.iso`). The fixes in `config/` are addresses in that release's executable; another region or revision would need them checked again.

## How it works

The game's PowerPC code is translated to C++ ahead of time by the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) (BSD-3, derived from Xenia), which also provides the Xbox 360 kernel, GPU and audio reimplementation. This repository holds the project manifest, function hints, a small host application, patches to the SDK and the Linux build scripts.

## Linux

Tested on Ubuntu 26.04 and Fedora 44 (KDE Plasma, Wayland). You need Clang 20 and LLD (`clang-20`, `clang++-20`, `lld-20`), CMake 3.25+, Ninja, Python 3, Git, a Vulkan 1.3 driver, and these packages (Debian/Ubuntu names): `libgtk-3-dev libx11-xcb-dev libxss-dev libvulkan-dev libwayland-dev libwayland-bin wayland-protocols libxkbcommon-dev libdecor-0-dev libasound2-dev libpulse-dev libpipewire-0.3-dev`. Linux 6.11 or newer is recommended.

```bash
git clone https://github.com/viniciussralves/mcla-recomp.git
cd mcla-recomp
git clone --recursive --branch v0.10.0 https://github.com/rexglue/rexglue-sdk.git third_party/rexglue-sdk
cd third_party/rexglue-sdk
git apply ../../patches/rexglue-v0.10.0-mcla.patch
git apply ../../patches/rexglue-minmax-blend-factor.patch
git apply ../../patches/rexglue-linux-procmap-query.patch
git apply ../../patches/rexglue-linux-wait-sleep.patch
git apply ../../patches/rexglue-present-max-height.patch
CC=clang-20 CXX=clang++-20 cmake --preset linux-amd64 -DREXGLUE_USE_VULKAN=ON -DREXGLUE_ENABLE_TRACY=OFF
cmake --build out/build/linux-amd64 --config Release
cmake --install out/build/linux-amd64 --config Release
cd ../..
python3 scripts/extract_game.py /path/to/your.iso
SDK=$PWD/third_party/rexglue-sdk/out/install/linux-amd64
"$SDK/bin/rexglue" codegen mcla_manifest.toml
CC=clang-20 CXX=clang++-20 cmake --preset linux-amd64-release -DCMAKE_PREFIX_PATH="$SDK"
cmake --build --preset linux-amd64-release
./scripts/play_linux.sh
```

**Play:** `./scripts/play_linux.sh` starts the build and passes any options through. Keep the repository where it was built; the game data is read from `game/`. An Xbox or PlayStation controller works.

Options go after the program name, for example `./scripts/play_linux.sh --fullscreen=true --mcla_fps=60` (30, 60, 120, or 0 for uncapped). `--resolution_scale=3` renders the game internally at 4K, 2 at 1440p and 1 at 720p (whole multiples of the game's 1280×720 framebuffer; 1080p is not an integer render-scale step). Native 4K is GPU-bound on most cards. The pause menu has three extra entries: DISPLAY (render scale, output resolution, frame-rate target, vsync, fullscreen, motion blur, depth of field, quality preset), PERFORMANCE (detail options) and CONTROLS (button prompts, intro speed); changes are saved to `mcla.toml` next to the executable. `F8` toggles a performance overlay, off at start: FPS and frame time, plus CPU and RAM on Linux (GPU and VRAM where the amdgpu driver exposes them); the same switch is the PERFORMANCE OVERLAY row in the pause menu.

On Wayland, put `vulkan_allow_present_mode_immediate = false` and `vulkan_allow_present_mode_mailbox = false` in `out/build/linux-amd64-release/mcla.toml`; without them the game can present far more often than it renders and keep the GPU busy.

**1080p output.** On Wayland, DISPLAY → OUTPUT RESOLUTION (or `--present_max_height=1080`) caps the swapchain to that height and the compositor scales it to the window, so a 4K window can present a true 1920×1080 image. This is a presentation setting, not a render setting; RENDER SCALE still controls the game's internal resolution. With the performance overlay visible, a capped output is also cheaper to composite; without an overlay, the compositor can often present the native output directly, so measure both on your system. Mesa's X11/XWayland WSI only accepts a swapchain the size of the window, so the row is hidden on X11 and `--present_max_height` has no effect there (the log says so once).

**Quality presets.** DISPLAY → QUALITY PRESET groups the options:

- **PERFORMANCE** — 720p render, 1080p output on Wayland, MSAA/motion blur/DOF/fullscreen blur/shadows off, lower detail.
- **BALANCED** — 720p render, 1080p output, effects and shadows on.
- **QUALITY** — 1440p render downscaled to 1080p; the shadow maps also scale up with the render target.
- **NATIVE 4K** — 4K render and output.

Render scale, MSAA, fullscreen blur and output resolution take effect at the next start; the shadow and density options also apply to the running game where the hooks allow it. MCLA's engine framebuffer is still 1280×720 internally; QUALITY is a 1440p render downsampled to 1080p, not a native 1080p framebuffer.

**Update:** `git pull`, rebuild the SDK if its patches changed, then `cmake --build --preset linux-amd64-release`.

| If | Then |
|---|---|
| `codegen failed` | `game/default.xex` is missing or not the supported release; see the end of `out/codegen.log`. |
| `librexruntime.so` or `librexgpu-xenos.so` cannot be loaded | Build and install the SDK first; `scripts/play_linux.sh` looks for the libraries in the SDK's `lib64` or `lib` directory. |
| The game presents about 1,000 times a second | Set `vulkan_allow_present_mode_immediate = false` and `vulkan_allow_present_mode_mailbox = false` in `mcla.toml` (Wayland). |
| `present_max_height` does nothing | It is Wayland-only; Mesa's X11/XWayland WSI requires a swapchain the size of the window. The log warns once. |
| The game closes by itself | Start it with `--log_file=run.log --log_level=info` and open an issue with the end of that file. |

## Missed functions

If the game stops with `Call to invalid or unregistered function at guest address ...`, the recompiler missed a function that is only reached through a pointer. Please open an issue with that line; the fix is one entry in `config/runtime_discovered.toml`.

## Credits

- ReXGlue SDK by Tom Clay and contributors; Xenia by Ben Vanik and contributors.
- [LARecomp](https://github.com/mzzvxm/LARecomp) by mzzvxm and contributors, whose fixes several parts of this project are ported from, with permission.
- XenonRecomp by hedge-dev, on which ReXGlue's analysis builds.
- The Linux build by [@MoonFuji](https://github.com/MoonFuji).

## Licence

GPL-3.0-or-later; see `LICENSE`. *Midnight Club* is a trademark of Take-Two Interactive. This project is not affiliated with or endorsed by Rockstar Games or Take-Two.
