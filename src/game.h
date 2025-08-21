#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "base.h"
#include "renderer.h"
#include "platform.h"

struct Game;

Game* game_init(Platform* platform, Arena* arena);
void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena);
bool game_close_requested(Game* game);

#endif
