#include "raylib.h"

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

int main() {
  Game game;

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
