# Patches to the ReXGlue SDK

Apply to ReXGlue v0.10.0 (`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`), from the SDK checkout:

```
git apply ../../patches/rexglue-v0.10.0-mcla.patch
git apply ../../patches/rexglue-minmax-blend-factor.patch
```

On Linux, these go on top, from the same folder:

```
git apply ../../patches/rexglue-linux-procmap-query.patch
git apply ../../patches/rexglue-linux-wait-sleep.patch
git apply ../../patches/rexglue-present-max-height.patch
```

They make page-protection lookups and multi-handle waits cheap on Linux.

The present-height patch lets the Vulkan swapchain be smaller than the window where the compositor scales it to the surface; it backs the DISPLAY → OUTPUT RESOLUTION row (`present_max_height`, 0 = window size), so a 4K window can present a true 1080p image. It is Linux/Vulkan on Wayland only. Mesa's X11/XWayland WSI reports the window size as both `minImageExtent` and `maxImageExtent`, so the cap cannot be applied there; the presenter logs a warning once and the settings menu omits the row.

The first was several files at first; they are combined now because several touch the same build files and would not apply independently. The second goes on top of the first (both change `src/graphics/vulkan/command_processor.cpp`).

## rexglue-v0.10.0-mcla.patch

| Change | Files | Affects |
|---|---|---|
| Vblank timer resync: the guest tick count can step backwards, which made an unsigned difference wrap and fire vblank interrupts without end | `src/graphics/graphics_system.cpp` | Linux |
| Per-frame counters kept in Release builds, and their CSV path taken from `REX_PERF_LOG_CSV` (the GPU plugin has its own copy of the logger state, so a path set by the host never reached it) | `CMakeLists.txt`, `src/core/CMakeLists.txt`, `src/system/CMakeLists.txt`, `src/core/perf/counter.cpp` | Linux |
| Debug-only register lookup removed from every GPU register write in Release builds | `src/graphics/command_processor.cpp` | Linux |

## rexglue-minmax-blend-factor.patch

The car reflection fix: cars came out white with a glow around them, worst at night. The Xbox 360's GPU applies the source blend factor in the MIN and MAX blend equations; Vulkan ignores blend factors there. The game draws the light glows in the cars' environment map with `max(source x SrcAlpha, destination)`, so every glow became a solid square, and the paint mirrored those. With the patch the translated pixel shader multiplies by its own alpha in that case.

| Change | Files | Affects |
|---|---|---|
| System flags per render target, set when its colour or alpha blend is MIN or MAX with source factor SrcAlpha and destination factor One | `src/graphics/vulkan/command_processor.cpp` | Vulkan (Linux) |
| The multiply by the saturated alpha before the colour is written | `spirv_translator.h`, `spirv_translator_rb.cpp` (under `include/rex/graphics/pipeline/shader` and `src/graphics/pipeline/shader`) | Vulkan (Linux) |

Host render target path only (the default on Linux).

## rexglue-present-max-height.patch

Lets the Vulkan presenter create a swapchain smaller than the window where the presentation engine stretches it to the surface. It backs the DISPLAY → OUTPUT RESOLUTION row and the `present_max_height` cvar (0 = match the window). With a 4K window and `present_max_height = 1080`, the final present pass runs at 1920×1080 and the compositor scales it to 4K. Linux/Vulkan on Wayland only; on X11 the surface capabilities force the window size, so the presenter logs a warning and uses the window size.

| Change | Files | Affects |
|---|---|---|
| `present_max_height` cvar, and capping the swapchain extent to it, with a warning when the surface cannot honour the cap | `src/ui/vulkan/vulkan_presenter.cpp` | Vulkan (Linux, Wayland) |
