struct PlayerInput {
	bool move_up;
	bool move_down;
};

struct World {
	float ball_position[2];
	float ball_velocity[2];

	float paddle_positions[2];
	float paddle_velocities[2];

	PlayerInput player_inputs[2];
};

void world_init(World* world)
{
	for(u8 i = 0; i < 2; i++)
	{
		world->ball_position[i] = 0.0f;
		world->ball_velocity[i] = 0.75f - (i * 0.35f);

		world->paddle_positions[i] = 0.0f;
		world->paddle_velocities[i] = 0.0f;

		world->player_inputs[i].move_up = false;
		world->player_inputs[i].move_down = false;
	}
}

void handle_collisions(World* world, float dt) {
}

void world_simulate(World* world, float dt)
{
	for(u32 i = 0; i < 2; i++) {
		PlayerInput* input = &world->player_inputs[i];

		if(input->move_up) {
			world->paddle_velocities[i] += PADDLE_ACCELERATION * dt;
		}
		if(input->move_down) {
			world->paddle_velocities[i] -= PADDLE_ACCELERATION * dt;
		}

		if(world->paddle_velocities[i] > 0) {
			world->paddle_velocities[i] -= PADDLE_FRICTION * dt;
		} else if(world->paddle_velocities[i] < 0) {
			world->paddle_velocities[i] += PADDLE_FRICTION * dt;
		}

		if(world->paddle_velocities[i] > PADDLE_MAX_SPEED) {
			world->paddle_velocities[i] = PADDLE_MAX_SPEED;
		}
		if(world->paddle_velocities[i] < -PADDLE_MAX_SPEED) {
			world->paddle_velocities[i] = -PADDLE_MAX_SPEED;
		}

		world->paddle_positions[i] += world->paddle_velocities[i] * dt;
		world->ball_position[i] += world->ball_velocity[i] * dt;
	}

	float ball_left = world->ball_position[0] - BALL_HALF_WIDTH;
	float ball_right = world->ball_position[0] + BALL_HALF_WIDTH;
	float ball_bottom = world->ball_position[1] - BALL_HALF_WIDTH;
	float ball_top = world->ball_position[1] + BALL_HALF_WIDTH;

	i8 paddle_index = 0;
	if(world->ball_velocity[0] > 0.0f) {
		paddle_index = 1;
	}

	float paddle_x = -PADDLE_X + paddle_index * PADDLE_X * 2.0f;
	float paddle_left = paddle_x - PADDLE_HALF_WIDTH;
	float paddle_right = paddle_x + PADDLE_HALF_WIDTH;
	float paddle_bottom = world->paddle_positions[paddle_index] - PADDLE_HEIGHT;
	float paddle_top = world->paddle_positions[paddle_index] + PADDLE_HEIGHT;

	// TODO - Rollover extra velocity into bounce direction.
	if(ball_left < paddle_right && ball_right > paddle_left
	&& ball_top > paddle_bottom && ball_bottom < paddle_top) {
		if(ball_left - world->ball_velocity[0] > paddle_right
		|| ball_right - world->ball_velocity[0] < paddle_left) {
			world->ball_velocity[0] = -world->ball_velocity[0];
			world->ball_position[0] += world->ball_velocity[0] * dt;
		} else {
			world->ball_velocity[1] = -world->ball_velocity[1];
			world->ball_position[1] += world->ball_velocity[1] * dt;
		}
	}

	if(ball_bottom < -1.0f
	|| ball_top > 1.0f) {
		world->ball_velocity[1] = -world->ball_velocity[1];
		world->ball_velocity[1] += world->ball_velocity[1] * dt;
	}

	// TODO - Proper restart.
	if(ball_left < -1.25f
	|| ball_right > 1.25f) {
		world->ball_position[0] = 0.0f;
		world->ball_position[1] = 0.0f;
		world->ball_velocity[0] = 0.75f;
		world->ball_velocity[1] = 0.45f;
	}
}
