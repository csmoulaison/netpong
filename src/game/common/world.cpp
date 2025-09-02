struct PlayerInput {
	bool move_up;
	bool move_down;
};

struct World {
	float paddle_positions[2];
	float paddle_velocities[2];

	// TODO - Separate inputs from world state. Makes a lot more sense, e.g. how
	// inputs are selectively overriden by the client when resolving state updates.
	PlayerInput player_inputs[2];
};

void world_init(World* world)
{
	for(u8 i = 0; i < 2; i++)
	{
		world->paddle_positions[i] = 0.5f;
		world->paddle_velocities[i] = 0.0f;
		world->player_inputs[i].move_up = false;
		world->player_inputs[i].move_down = false;
	}
}

// NOW - we are going to simplify the simulation. a discrete button press, not a hold, will
// determine whether a discrete movement occurs. The client will mark that and the frame it
// occurs on. Then we will trace it all the way through the system.
void world_simulate(World* world, float dt)
{
	float speed = PADDLE_SPEED * dt;

	for(u32 i = 0; i < 2; i++) {
		PlayerInput* input = &world->player_inputs[i];

		//if(input->move_up) {
			//world->paddle_positions[i] = -world->paddle_positions[i];
		//}

		if(input->move_up) {
			world->paddle_positions[i] += speed;
		}
		if(input->move_down) {
			world->paddle_positions[i] -= speed;
		}
	}
}
