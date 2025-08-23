#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "base/base.h"
#include "renderer/renderer.h"
#include "platform/platform_media.h"
#include "platform/platform_network.h"

struct Game;

Game* game_init(Platform* platform, Arena* arena);
void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena);
bool game_close_requested(Game* game);

#endif
