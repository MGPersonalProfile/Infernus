#include "raylib.h"
#include <cstdlib>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "core/Game.h"

#ifdef __EMSCRIPTEN__
static Game *g_game = nullptr;

void emscripten_loop() {
  g_game->update(GetFrameTime());
  g_game->render();
}
#endif

int main(int argc, char** argv) {
  Game game;

  // Env-var driven test/headless setup. Recognized:
  //   INFERNUS_TEST=1            -> emit telemetry.jsonl, auto-start in PLAYING
  //   INFERNUS_HEADLESS=1        -> skip InitWindow + render path entirely.
  //                                 Lets Claude run the game from a sandbox
  //                                 with no display server.
  //   INFERNUS_TEST_DURATION=N   -> auto-quit after N seconds (0 = no limit).
  //                                 Useful for reproducible smoke runs.
  // CLI flag mirrors are also accepted (--test, --headless, --duration N) so
  // the launcher doesn't depend on the env environment.
  bool wantTest = std::getenv("INFERNUS_TEST") != nullptr;
  bool wantHeadless = std::getenv("INFERNUS_HEADLESS") != nullptr;
  float wantDuration = 0.0f;
  if (const char *d = std::getenv("INFERNUS_TEST_DURATION")) {
    wantDuration = (float)std::atof(d);
  }
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--test") == 0) wantTest = true;
    else if (std::strcmp(argv[i], "--headless") == 0) wantHeadless = true;
    else if (std::strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
      wantDuration = (float)std::atof(argv[++i]);
    }
  }

  if (wantTest) {
    game.enableTelemetry = true;
    game.testMode = true;
  }
  if (wantHeadless) {
    game.headlessMode = true;
    // Headless implies test (telemetry on, auto-start in PLAYING) — there's
    // no human at the controls.
    game.enableTelemetry = true;
    game.testMode = true;
  }
  if (wantDuration > 0.0f) {
    game.autoQuitAfterSeconds = wantDuration;
  }

#ifdef __EMSCRIPTEN__
  g_game = &game;
  game.init();
  emscripten_set_main_loop(emscripten_loop, 0, 1);
  game.shutdown();
#else
  game.run();
#endif

  return 0;
}
