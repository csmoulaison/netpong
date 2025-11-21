// NOW: < LIST:
// - Cleanup. I think it makes sense to save major cleanups for after a lot of
// this potentially very disruptive work.
//   - Primarily, make sure everything is in the proper place with regards to
//   different configurations. Look for repeated work, etc.
//   - Only finish once we have combed through every file in the project.
//   
// WHETHER we want the following before moving to the mech stuff is debatable:
// - Most majorly, a task system with a thread pool, dependency graph, all that.
// I think it makes the most sense to do this in another project first before
// including it with a networked project.
// - An audio subsystem. This will certainly be different for pong than for
// whatever stuff we get up to in the mech project, so it's not particularly
// important. But do we want fun beeps? Maybe we want fun beeps.
// - Advanced compression stuff for packets beyond just bitpacking.

#define MENU_OPTIONS_LEN 6

enum class GameState {
	Menu,
	Session
};

struct Game {
	Arena persistent_arena;
	Arena session_arena;
	Arena transient_arena;

	GameState state;
	bool close_requested;
	u32 frames_since_init;

	Windowing::ButtonHandle move_up_buttons[2];
	Windowing::ButtonHandle move_down_buttons[2];
	Windowing::ButtonHandle quit_button;
	Windowing::ButtonHandle select_button;

	char ip_string[16];

	i32 menu_selection;
	float menu_activations[MENU_OPTIONS_LEN];

	f32 visual_paddle_positions[2];

	bool local_server;
	Client* client;
	Server* server;
};

Game* game_init(Windowing::Context* window, char* ip_string, Arena* program_arena) 
{
	Game* game = (Game*)arena_alloc(program_arena, sizeof(Game));

	arena_init(&game->persistent_arena, MEGABYTE);
	arena_init(&game->session_arena, MEGABYTE);
	arena_init(&game->transient_arena, MEGABYTE);

	game->state = GameState::Menu;
	game->close_requested = false;
	game->frames_since_init = 0;

	game->move_up_buttons[0] = Windowing::register_key(window, Windowing::Keycode::W);
	game->move_down_buttons[0] = Windowing::register_key(window, Windowing::Keycode::S);
	game->move_up_buttons[1] = Windowing::register_key(window, Windowing::Keycode::Up);
	game->move_down_buttons[1] = Windowing::register_key(window, Windowing::Keycode::Down);
	game->quit_button = Windowing::register_key(window, Windowing::Keycode::Escape);
	game->select_button = Windowing::register_key(window, Windowing::Keycode::Space);

	if(ip_string != nullptr) {
		memcpy(game->ip_string, ip_string, 16);
	} else {
		game->ip_string[0] = '\0';
	}

	game->menu_selection = 0;
	for(i32 i = 0; i < MENU_OPTIONS_LEN; i++) {
		game->menu_activations[i] = 0.0f;
	}

	game->client = nullptr;
	game->server = nullptr;

	return game;
}

void game_init_session(Game* game, i32 config_setting) 
{
	switch(config_setting) {
		case CONFIG_REMOTE:
			game->local_server = false;
			game->client = client_init(&game->session_arena, game->ip_string);
			break;
		case CONFIG_FULL_LOCAL:
			game->local_server = true;
			game->server = server_init(&game->session_arena, false);
			server_add_local_player(game->server);
			server_add_local_player(game->server);
			break;
		case CONFIG_HALF_LOCAL:
			game->local_server = true;
			game->server = server_init(&game->session_arena, true);
			server_add_local_player(game->server);
			break;
		case CONFIG_FULL_BOT:
			game->local_server = true;
			game->server = server_init(&game->session_arena, true);
			server_add_bot(game->server);
			server_add_bot(game->server);
			break;
		case CONFIG_HALF_BOT:
			game->local_server = true;
			game->server = server_init(&game->session_arena, true);
			server_add_local_player(game->server);
			server_add_bot(game->server);
			break;
		default: break;
	}
}

void render_rect(Render::Context* renderer, Rect rect, Windowing::Context* window)
{
	Render::State* render_state = &renderer->current_state;
	f32 x_scale = (f32)window->window_height / window->window_width;

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

void render_menu_state(Game* game, Render::Context* renderer, Windowing::Context* window)
{
	Render::text_line(
		renderer, 
		"Netpong", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		FONT_FACE_LARGE);

	const char* strings[MENU_OPTIONS_LEN] = { "Local", "Host", "Join", "Half bot", "Full bot", "Quit" };
	for(u32 i = 0; i < MENU_OPTIONS_LEN; i++) {
		float* activation = &game->menu_activations[i];
		float activator_speed = 10.0f;

		// Animate text based on selection.
		if(i == game->menu_selection && *activation < 1.0f) {
			*activation += BASE_FRAME_LENGTH * activator_speed;
			if(*activation > 1.0f) {
				*activation = 1.0f;
			}
		} else if(i != game->menu_selection && *activation > 0.0f) {
			*activation -= BASE_FRAME_LENGTH * activator_speed;
			if(*activation < 0.0f) {
				*activation = 0.0f;
			}
		}

		Render::text_line(
			renderer, 
			strings[i], 
			64.0f + (48.0f * *activation), window->window_height - 200.0f - (96.0f * i), 
			0.0f, 1.0f,
			1.0f, 1.0f, 1.0f - *activation, 1.0f,
			FONT_FACE_SMALL);
	}
}

void render_requesting_connection_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Attempting to connect...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_SMALL);
}

void render_waiting_to_start_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Waiting to start...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_SMALL);
}

void render_active_state(Game* game, Render::Context* renderer, Windowing::Context* window)
{
	// Rendering player paddles differs depending on the client/server configuration.
	// Essentially, the following code differentiates between remote and local
	// players, and linearly interpolates the position of the remote ones to smooth
	// out mispredictions.
	World* world;
	if(game->local_server) {
		Server* server = game->server;
		world = &server->world;

		for(u8 i = 0; i < 2; i++) {
			ServerSlot* slot = &server->slots[i];
			if(slot->type == SERVER_PLAYER_REMOTE) {
				render_visual_lerp(&game->visual_paddle_positions[i], world->paddle_positions[i], BASE_FRAME_LENGTH);
			} else {
				game->visual_paddle_positions[i] = world->paddle_positions[i];
			}
		}
	} else {
		Client* client = game->client;
		world = &client_state_from_frame(client, client->frame - 1)->world;

		i8 other_id = client_get_other_id(client);
		render_visual_lerp(&game->visual_paddle_positions[other_id], world->paddle_positions[other_id], client->frame_length);
		game->visual_paddle_positions[client->id] = world->paddle_positions[client->id];
	}

	// We use calculated paddle positions for rendering, and render the ball
	// position directly without any interpolation.
	for(u8 i = 0; i < 2; i++) {
		Rect paddle;
		paddle.x = -PADDLE_X + i * PADDLE_X * 2.0f;
		paddle.y = game->visual_paddle_positions[i];
		paddle.w = PADDLE_WIDTH;
		paddle.h = PADDLE_HEIGHT;
		render_rect(renderer, paddle, window);
	}
	Rect ball;
	ball.x = world->ball_position[0];
	ball.y = world->ball_position[1];
	ball.w = BALL_WIDTH;
	ball.h = BALL_WIDTH;
	render_rect(renderer, ball, window);
}

void game_update(Game* game, Windowing::Context* window, Render::Context* renderer)
{
	if(game->state == GameState::Menu) {
		if(Windowing::button_pressed(window, game->quit_button)) {
			game->close_requested = true;
			return;
		}

		if(Windowing::button_pressed(window, game->move_up_buttons[0])) {
			game->menu_selection--;
			if(game->menu_selection < 0) {
				game->menu_selection = MENU_OPTIONS_LEN - 1;
			}
		}
		if(Windowing::button_pressed(window, game->move_down_buttons[0])) {
			game->menu_selection++;
			if(game->menu_selection > MENU_OPTIONS_LEN - 1) {
				game->menu_selection = 0;
			}
		}

		if(Windowing::button_pressed(window, game->select_button)) {
			if(game->menu_selection == MENU_OPTIONS_LEN - 1) {
				game->close_requested = true;
			} else {
				game->state = GameState::Session;
				game_init_session(game, game->menu_selection);
			}
		}

		render_menu_state(game, renderer, window);
	} else {
		// Pressing the quit button from here brings us back to the menu, and cleanup
		// of the per session resources is done here.
		if(Windowing::button_pressed(window, game->quit_button)) {
			if(game->client != nullptr) {
				Network::close_socket(game->client->socket);
				game->client->socket = nullptr;
				game->client = nullptr;
			}
			if(game->server != nullptr) {
				if(game->server->socket != nullptr) {
					Network::close_socket(game->server->socket);
					game->server->socket = nullptr;
				}
				game->server = nullptr;
			}
			arena_clear(&game->session_arena);
			game->state = GameState::Menu;
			return;
		}

		// NOW: This is a rat's nest. What should be here vs elsewhere and where
		// should we introduce a nested function?
		if(game->local_server) {
			Server* server = game->server;

			for(u8 i = 0; i < 2; i++) {
				if(server->slots[i].type == SERVER_PLAYER_LOCAL) {
					ClientInput event_input = {};
					event_input.frame = server->frame;

					if(Windowing::button_down(window, game->move_up_buttons[i])) {
						event_input.input.move_up = 1.0f;
					}
					if(Windowing::button_down(window, game->move_down_buttons[i])) {
						event_input.input.move_down = 1.0f;
					}

					server_push_event(server, (ServerEvent){ 
						.type = SERVER_EVENT_CLIENT_INPUT, 
						.client_id = i,
						.client_input = event_input
					});
				}
			}
			server_update(server, BASE_FRAME_LENGTH, &game->transient_arena);

			if(server_is_active(server)) {
				render_active_state(game, renderer, window);
			} else {
				render_waiting_to_start_state(game, renderer, window);
			}
		} else { // Server is remote.
			Client* client = game->client;
			if(Windowing::button_down(window, game->move_up_buttons[0])) {
				client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_UP;
				client->events_len++;
			}
			if(Windowing::button_down(window, game->move_down_buttons[0])) {
				client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_DOWN;
				client->events_len++;
			}
			client_update(client, &game->transient_arena);

			switch(client->connection_state) {
				case CLIENT_STATE_REQUESTING_CONNECTION:
					render_requesting_connection_state(game, renderer, window);
					break;
				case CLIENT_STATE_WAITING_TO_START:
					render_waiting_to_start_state(game, renderer, window);
					break;
				case CLIENT_STATE_ACTIVE:
					render_active_state(game, renderer, window);
					break;
				default: break;
			}
		}

	}

	game->frames_since_init++;
	arena_clear(&game->transient_arena);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
