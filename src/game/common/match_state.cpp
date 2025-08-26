#define MATCH_STATE_BUFFER_LEN 1024

struct MatchState {
	float player_positions[2];
	float ball_position[2];
};

struct MatchStateBuffer {
	MatchState states[MATCH_STATE_BUFFER_LEN];
	u32 states_len;
	u32 oldest_tick_count;
};
