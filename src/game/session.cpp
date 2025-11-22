void session_render_rect(Render::Context* renderer, Rect rect, Windowing::Context* window)
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

void session_render_visual_lerp(f32* visual, f32 real, f32 dt)
{
	*visual = lerp(*visual, real, VISUAL_SMOOTHING_SPEED * dt);
	if(abs(*visual - real) < VISUAL_SMOOTHING_EPSILON) {
		*visual = real;
	}
}

void session_render_requesting_connection_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Attempting to connect...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_SMALL);
}

void session_render_waiting_to_start_state(Game* game, Render::Context* renderer, Windowing::Context* window) 
{
	Render::text_line(
		renderer, 
		"Waiting to start...", 
		64.0f, window->window_height - 64.0f, 
		0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, sin((float)game->frames_since_init * 0.05f),
		FONT_FACE_SMALL);
}

void session_render_active_state(Game* game, Render::Context* renderer, Windowing::Context* window)
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
				session_render_visual_lerp(&game->visual_paddle_positions[i], world->paddle_positions[i], BASE_FRAME_LENGTH);
			} else {
				game->visual_paddle_positions[i] = world->paddle_positions[i];
			}
		}
	} else {
		Client* client = game->client;
		world = &client_state_from_frame(client, client->frame - 1)->world;

		i8 other_id = client_get_other_id(client);
		session_render_visual_lerp(&game->visual_paddle_positions[other_id], world->paddle_positions[other_id], client->frame_length);
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
		session_render_rect(renderer, paddle, window);
	}
	Rect ball;
	ball.x = world->ball_position[0];
	ball.y = world->ball_position[1];
	ball.w = BALL_WIDTH;
	ball.h = BALL_WIDTH;
	session_render_rect(renderer, ball, window);
}

void session_init(Game* game, i32 config_setting) 
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

void session_update(Game* game, Windowing::Context* window, Render::Context* renderer)
{
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

	// NOW: This is a rat's nest.
	// 1. Make it as clean as possible inline.
	// 2. Decide whether anything currently inline should not be that.
	if(game->local_server) {
		Server* server = game->server;

		for(u8 i = 0; i < 2; i++) {
			if(server->slots[i].type == SERVER_PLAYER_LOCAL) {
				ServerEvent event = {};
				event.type = SERVER_EVENT_CLIENT_INPUT;
				event.client_id = i;

				event.client_input.frame = server->frame;
				if(Windowing::button_down(window, game->move_up_buttons[i])) {
					event.client_input.input.move_up = 1.0f;
				}
				if(Windowing::button_down(window, game->move_down_buttons[i])) {
					event.client_input.input.move_down = 1.0f;
				}

				server_push_event(server, event);
			}
		}

		server_update(server, BASE_FRAME_LENGTH, &game->frame_arena);

		if(server_is_active(server)) {
			session_render_active_state(game, renderer, window);
		} else {
			session_render_waiting_to_start_state(game, renderer, window);
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
		client_update(client, &game->frame_arena);

		switch(client->connection_state) {
			case CLIENT_STATE_REQUESTING_CONNECTION:
				session_render_requesting_connection_state(game, renderer, window);
				break;
			case CLIENT_STATE_WAITING_TO_START:
				session_render_waiting_to_start_state(game, renderer, window);
				break;
			case CLIENT_STATE_ACTIVE:
				session_render_active_state(game, renderer, window);
				break;
			default: break;
		}
	}
}
