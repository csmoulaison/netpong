// NOW: < THIS: We are on the path to allowing locally hosted servers and fully
// local play. Here's the steps:
// - Remove rendering logic from the client. Maybe some other stuff too, we'll
// figure it out.
// - Allow for the game to allocate a local server instead of client. The client
// won't be used at all in the case of a local server.
// - To start with, try the fully local case, with the inputs being passed into
// the server directly as an event, the server authoritatively simulating the
// world state, and the game layer rendering the results in both the local and
// remote case.
// - Make whatever changes are needed to support the local server and 1 remote
// client case.
// - Make a dead simple bot which sends input messages to the server like a
// client.
// - Assess whether the way events are being handled is sensical or if anything
// needs to be more systemetized.
//
// Afterwards, the following are on the agenda:
// - Restructure the platform layer, turning it into a series of namespaced
// engine subsystems which forward declare whatever platform specific stuff they
// need. The engine part will be the same in every build, and this way we can
// go all in on a maximally thin platform layer. As we add more platforms, the
// goal of thinning out the platform layer while allowing for platform specific
// optimizations should make things good and clear.
//      (even those that other subsystems use if needed)
// - Move connection acceptance/request pipeline over to the platform side of
// things, including timeouts.
// - Add some text rendering functionality so we can have some menus and debug
// stuff up.
// - Bitpacking of packets: might open up some ideas about streamlining the
// packet stuff, who knows?
// - Cleanup. I think it makes sense to save major cleanups for after a lot of
// this potentially very disruptive work.
//
// Whether we want the following before moving to the mech stuff is debatable:
// - Most majorly, a task system with a thread pool, dependency graph, all that.
// I think it makes the most sense to do this in another project first before
// including it with a networked project.
// - An audio subsystem. This will certainly be different for pong than for
// whatever stuff we get up to in the mech project, so it's not particularly
// important. But do we want fun beeps? Maybe we want fun beeps.
// - Advanced compression stuff for packets beyond just bitpacking.

struct Game {
	bool close_requested;
	u32 frames_since_init;

	ButtonHandle button_move_up;
	ButtonHandle button_move_down;
	ButtonHandle button_quit;

	bool local_server;
	Client* client;
	Server* server;
};

Game* game_init(Platform* platform, Arena* arena, char* ip_string) 
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->close_requested = false;
	game->frames_since_init = 0;

	game->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	game->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	game->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	game->local_server = false;
	game->client = client_init(platform, arena, ip_string);
	game->server = nullptr;

	return game;
}

void render_rect(RenderState* render_state, Rect rect, Platform* platform)
{
	f32 x_scale = (f32)platform->window_height / platform->window_width;

	Rect* rect_to_render = &render_state->rects[render_state->rects_len];
	render_state->rects_len += 1;

	rect_to_render->x = rect.x * x_scale;
	rect_to_render->y = rect.y;
	rect_to_render->w = rect.w;
	rect_to_render->h = rect.h;
}

void render_visual_lerp(f32* visual, f32 real, f32 dt)
{
	*visual = lerp(*visual, real, VISUAL_SMOOTHING_SPEED * dt);
	if(abs(*visual - real) < VISUAL_SMOOTHING_EPSILON) {
		*visual = real;
	}
}

void render_requesting_connection_state(Game* game, RenderState* render_state, Platform* platform) 
{
	// Render blinking indicator.
	for(u8 i = 0; i < 3; i++) {
		f32 xoff = -1000.0f;
		if((game->frames_since_init / 60) % 3 == 0) {
			xoff = 0.0f;
		}
	
		Rect rect;
		rect.x = -0.75f + xoff;
		rect.y = 0.75f;
		rect.w = 0.025f;
		rect.h = 0.025f;
		render_rect(render_state, rect, platform);
	}
}

void render_waiting_to_start_state(Game* game, RenderState* render_state, Platform* platform) 
{
	// Render blinking indicator.
	for(u8 i = 0; i < 3; i++) {
		f32 xoff = 0.0f;
		xoff += ((game->frames_since_init / 30) % 3) * 0.025;

		Rect rect;
		rect.x = -0.75f + xoff;
		rect.y = 0.75f;
		rect.w = 0.025f;
		rect.h = 0.025f;
		render_rect(render_state, rect, platform);
	}
}

// NOW: Remove reference to client and figure out how we want it factored out.
void render_state_from_world(Game* game, RenderState* render_state, Platform* platform)
{
	Client* client = game->client;

	World* world = &client_state_from_frame(client, client->frame - 1)->world;
	render_visual_lerp(&client->visual_ball_position[0], world->ball_position[0], client->frame_length * 4.0f);
	render_visual_lerp(&client->visual_ball_position[1], world->ball_position[1], client->frame_length * 4.0f);

	i8 other_id = client_get_other_id(client);
	render_visual_lerp(&client->visual_paddle_positions[other_id], world->paddle_positions[other_id], client->frame_length);
	client->visual_paddle_positions[client->id] = world->paddle_positions[client->id];

	// Render
	for(u8 i = 0; i < 2; i++) {
		Rect paddle;
		paddle.x = -PADDLE_X + i * PADDLE_X * 2.0f;
		paddle.y = client->visual_paddle_positions[i];
		paddle.w = PADDLE_WIDTH;
		paddle.h = PADDLE_HEIGHT;
		render_rect(render_state, paddle, platform);
	}
	Rect ball;
	ball.x = client->visual_ball_position[0];
	ball.y = client->visual_ball_position[1];
	ball.w = BALL_WIDTH;
	ball.h = BALL_WIDTH;
	render_rect(render_state, ball, platform);
}

void game_update(Game* game, Platform* platform, RenderState* render_state)
{
	if(game->local_server) {
		// NOW: Local server logic.
		Server* server = game->server;
	} else {
		Client* client = game->client;
		if(platform_button_down(platform, game->button_move_up)) {
			client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_UP;
			client->events_len++;
		}
		if(platform_button_down(platform, game->button_move_down)) {
			client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_DOWN;
			client->events_len++;
		}
		client_update(client, platform, render_state);

		switch(client->connection_state) {
			case CLIENT_STATE_REQUESTING_CONNECTION:
				render_requesting_connection_state(game, render_state, platform);
				break;
			case CLIENT_STATE_WAITING_TO_START:
				render_waiting_to_start_state(game, render_state, platform);
				break;
			case CLIENT_STATE_ACTIVE:
				render_state_from_world(game, render_state, platform);
				break;
			default: break;
		}
	}

	game->close_requested = platform_button_down(platform, game->button_quit);
	game->frames_since_init++;
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
