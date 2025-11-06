// NOW: < LIST:
// - Move connection acceptance/request pipeline over to the platform side of
// things, including timeouts.
// - Bitpacking of packets: might open up some ideas about streamlining the
// packet stuff, who knows?
// - Cleanup. I think it makes sense to save major cleanups for after a lot of
// this potentially very disruptive work.
//
// WHETHER we want the following before moving to the mech stuff is debatable:
// - Most majorly, a task system with a thread pool, dependency graph, all that.
// I think it makes the most sense to do this in another project first before
// including it with a networked project.
// - An audio subsystem. This will certainly be different for pong than for
// whatever stuff we get up to in the mech project, so it's not particularly
// important. But do we want fun beeps? Maybe we want fun beeps.
// - Advanced compression stuff for packets beyond just bitpacking.

#define MENU_OPTIONS_LEN 5

enum class GameState {
	Menu,
	Session
};

struct Game {
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

	f32 visual_ball_position[2];
	f32 visual_paddle_positions[2];

	bool local_server;
	Client* client;
	Server* server;
};

Game* game_init(Windowing::Context* window, Arena* arena, char* ip_string) 
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

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

void game_init_session(Game* game, i32 config_setting, Arena* arena) 
{
	switch(config_setting) {
		case CONFIG_REMOTE:
			game->local_server = false;
			game->client = client_init(arena, game->ip_string);
			break;
		case CONFIG_FULL_LOCAL:
			game->local_server = true;
			game->server = server_init(arena, false);
			server_add_local_player(game->server);
			server_add_local_player(game->server);
			break;
		case CONFIG_HALF_LOCAL:
			game->local_server = true;
			game->server = server_init(arena, true);
			server_add_local_player(game->server);
			break;
		case CONFIG_FULL_BOT:
			game->local_server = true;
			game->server = server_init(arena, true);
			server_add_bot(game->server);
			server_add_bot(game->server);
			break;
		case CONFIG_HALF_BOT:
			game->local_server = true;
			game->server = server_init(arena, true);
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

void render_requesting_connection_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Attempting to connect...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_LARGE);
}

void render_waiting_to_start_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Waiting to start...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_LARGE);
}

void render_active_state(Game* game, Render::Context* renderer, Windowing::Context* window)
{
	if(game->local_server) {
		Server* server = game->server;
		World* world = &server->world;

		game->visual_ball_position[0] = world->ball_position[0];
		game->visual_ball_position[1] = world->ball_position[1];

		for(u8 i = 0; i < 2; i++) {
			ServerSlot* slot = &server->slots[i];
			if(slot->type == SERVER_PLAYER_REMOTE) {
				render_visual_lerp(&game->visual_paddle_positions[i], world->paddle_positions[i], BASE_FRAME_LENGTH);
			} else {
				game->visual_paddle_positions[i] = world->paddle_positions[i];
			}
		}
	} else { // Server is remote
		Client* client = game->client;
		World* world = &client_state_from_frame(client, client->frame - 1)->world;

		//render_visual_lerp(&game->visual_ball_position[0], world->ball_position[0], client->frame_length * 4.0f);
		//render_visual_lerp(&game->visual_ball_position[1], world->ball_position[1], client->frame_length * 4.0f);
		game->visual_ball_position[0] = world->ball_position[0];
		game->visual_ball_position[1] = world->ball_position[1];

		i8 other_id = client_get_other_id(client);
		render_visual_lerp(&game->visual_paddle_positions[other_id], world->paddle_positions[other_id], client->frame_length);
		game->visual_paddle_positions[client->id] = world->paddle_positions[client->id];
	}

	// Render world
	for(u8 i = 0; i < 2; i++) {
		Rect paddle;
		paddle.x = -PADDLE_X + i * PADDLE_X * 2.0f;
		paddle.y = game->visual_paddle_positions[i];
		paddle.w = PADDLE_WIDTH;
		paddle.h = PADDLE_HEIGHT;
		render_rect(renderer, paddle, window);
	}
	Rect ball;
	ball.x = game->visual_ball_position[0];
	ball.y = game->visual_ball_position[1];
	ball.w = BALL_WIDTH;
	ball.h = BALL_WIDTH;
	render_rect(renderer, ball, window);
}

void game_update(Game* game, Windowing::Context* window, Render::Context* renderer, Arena* arena)
{
	if(game->state == GameState::Menu) {
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
		
		const char* strings[MENU_OPTIONS_LEN] = {
			"Local",
			"Host",
			"Join",
			"Half bot",
			"Full bot"
		};

		for(u32 i = 0; i < MENU_OPTIONS_LEN; i++) {
			float activator_speed = 10.0f;
			if(i == game->menu_selection) {
				if(game->menu_activations[i] < 1.0f) {
					game->menu_activations[i] += BASE_FRAME_LENGTH * activator_speed;
					if(game->menu_activations[i] > 1.0f) {
						game->menu_activations[i] = 1.0f;
					}
				}
			} else {
				if(game->menu_activations[i] > 0.0f) {
					game->menu_activations[i] -= BASE_FRAME_LENGTH * activator_speed;
					if(game->menu_activations[i] < 0.0f) {
						game->menu_activations[i] = 0.0f;
					}
				}
			}
			Render::text_line(
				renderer, 
				strings[i], 
				64.0f + (64.0f * game->menu_activations[i]), window->window_height - 64.0f - (96.0f * i), 
				0.0f, 1.0f,
				1.0f, 1.0f, 1.0f - game->menu_activations[i], 1.0f,
				FONT_FACE_LARGE);
		}

		if(Windowing::button_pressed(window, game->select_button)) {
			game->state = GameState::Session;
			game_init_session(game, game->menu_selection, arena);
		}
	} else {
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
			server_update(server, BASE_FRAME_LENGTH);

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
			client_update(client);

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

	game->close_requested = Windowing::button_down(window, game->quit_button);
	game->frames_since_init++;
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
