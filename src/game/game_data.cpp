#define MENU_OPTIONS_LEN 6

enum class GameState {
	Menu,
	Session
};

struct Game {
	Arena persistent_arena;
	Arena session_arena;
	Arena frame_arena;

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
