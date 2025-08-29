struct PlayerInput {
	bool move_up;
	bool move_down;
};

struct World {
	float paddle_positions[2];
	float paddle_velocities[2];
	PlayerInput player_inputs[2];
};

void world_init(World* world)
{
	for(u8 i = 0; i < 2; i++)
	{
		world->paddle_positions[i] = 0.0f;
		world->paddle_velocities[i] = 0.0f;
		world->player_inputs[i].move_up = false;
		world->player_inputs[i].move_down = false;
	}
}

void world_simulate(World* world, float dt)
{
	float speed = 1.0f * dt;

	for(u32 i = 0; i < 2; i++) {
		PlayerInput* input = &world->player_inputs[i];

		if(input->move_up) {
			world->paddle_positions[i] += speed;
		}
		if(input->move_down) {
			world->paddle_positions[i] -= speed;
		}
	}
}
