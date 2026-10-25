// Controller button prompts: Xbox or PlayStation glyphs.
//
// Both glyph sets ship in the game's own UI files; this only sets the
// `platform` value the UI scripts read. See config/ui.toml. Mechanism from
// LARecomp.

#include "generated/default/mcla_init.h"

#include "mcla_app.h"

#include <atomic>
#include <cstdint>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>

REXCVAR_DEFINE_STRING(mcla_button_prompts, "auto", "MCLA",
                      "Button glyphs: auto (match the controller), xbox or playstation");

namespace {

constexpr uint32_t kImageBegin = 0x82000000;
constexpr uint32_t kSwfSetVarInt = 0x825EE0E0;   // (context, name, int)
constexpr uint32_t kStringPlatform = 0x8201ABDC;  // the game's own "platform" string

std::atomic<uint32_t> g_platform_var{0};
std::atomic<int> g_applied{-1};

// 1 = PlayStation glyphs, 0 = Xbox glyphs. Auto keeps the Xbox glyphs on
// Linux; the setting can still force either set.
int WantedPlatform() {
  std::string mode = REXCVAR_GET(mcla_button_prompts);
  if (mode == "playstation") {
    return 1;
  }
  return 0;
}

void WritePlatformVar(uint32_t entry, int value) {
  uint8_t* base = rex::Runtime::instance()->virtual_membase();
  // +0x58: the integer value. +0x5C: its "%d" text.
  base[entry + 0x58] = 0;
  base[entry + 0x59] = 0;
  base[entry + 0x5A] = 0;
  base[entry + 0x5B] = static_cast<uint8_t>(value);
  base[entry + 0x5C] = static_cast<uint8_t>('0' + value);
  base[entry + 0x5D] = 0;
}

struct MovieContext {
  uint32_t context = 0;
  int value = -1;
};
constexpr int kMaxMovies = 16;
MovieContext g_movies[kMaxMovies];
int g_movie_count = 0;

}  // namespace

// Called once per frame from the frame-timing hook.
void MclaTickButtonPrompts() {
  const uint32_t entry = g_platform_var.load(std::memory_order_relaxed);
  if (!entry) {
    return;
  }
  const int want = WantedPlatform();
  if (want == g_applied.load(std::memory_order_relaxed)) {
    return;
  }
  WritePlatformVar(entry, want);
  g_applied.store(want, std::memory_order_relaxed);
  REXLOG_INFO("button prompts: {}", want ? "PlayStation" : "Xbox");
}

void mcla_platform_var_init(PPCRegister& r27) {
  const uint32_t entry = r27.u32;
  if (entry < 0x10000) {
    return;
  }
  const int want = WantedPlatform();
  g_platform_var.store(entry, std::memory_order_relaxed);
  WritePlatformVar(entry, want);
  g_applied.store(want, std::memory_order_relaxed);
  REXLOG_INFO("button prompts: {} (platform variable at 0x{:08X})", want ? "PlayStation" : "Xbox",
              entry);
}

bool mcla_platform_push(PPCRegister& r5) {
  r5.u64 = static_cast<uint64_t>(WantedPlatform());
  return true;
}

void mcla_swf_context_enter(PPCRegister& r3) {
  const uint32_t context = r3.u32;
  if (!context || !g_platform_var.load(std::memory_order_relaxed)) {
    return;
  }
  const int want = WantedPlatform();
  int slot = -1;
  for (int i = 0; i < g_movie_count; ++i) {
    if (g_movies[i].context != context) {
      continue;
    }
    if (g_movies[i].value == want) {
      return;
    }
    slot = i;
    break;
  }
  if (slot < 0) {
    slot = g_movie_count < kMaxMovies ? g_movie_count++ : 0;
    g_movies[slot].context = context;
  }
  // Write only into the context being run right now; nothing is kept to be
  // written later, so a destroyed movie can never be touched.
  if (PPCFunc* set_var = rex::Runtime::instance()->function_dispatcher()->GetFunction(kSwfSetVarInt)) {
    rex::ppc::GuestToHostFunction<uint32_t>(set_var, context, kStringPlatform, uint32_t(want));
  }
  g_movies[slot].value = want;
}
