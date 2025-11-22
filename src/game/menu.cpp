void menu_update(Game* game, Windowing::Context* window, Render::Context* renderer)
{
	// CONTROL
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
			session_init(game, game->menu_selection);
		}
	}

	// RENDERING
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
