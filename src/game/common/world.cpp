struct PlayerInput {
	f32 move_up;
	f32 move_down;
};

struct World {
	f32 countdown_to_start;

	f32 ball_position[2];
	f32 ball_velocity[2];

	f32 paddle_positions[2];
	f32 paddle_velocities[2];

	PlayerInput player_inputs[2];
};

void world_init(World* world)
{
	world->countdown_to_start = START_COUNTDOWN_SECONDS;

	world->ball_position[0] = 0.0f;
	world->ball_position[1] = 0.0f;
	world->ball_velocity[0] = BALL_SPEED_INIT_X;
	world->ball_velocity[1] = BALL_SPEED_INIT_Y;

	for(u8 i = 0; i < 2; i++)
	{
		world->paddle_positions[i] = 0.0f;
		world->paddle_velocities[i] = 0.0f;

		world->player_inputs[i].move_up = 0.0f;
		world->player_inputs[i].move_down = 0.0f;
	}
}

void world_simulate(World* world, f32 dt)
{
	if(world->countdown_to_start > 0.0f) {
		world->countdown_to_start -= dt;
		return;
	}

	for(u32 i = 0; i < 2; i++) {
		PlayerInput* input = &world->player_inputs[i];

		if(input->move_up > 0.0f) {
			world->paddle_velocities[i] += input->move_up * PADDLE_ACCELERATION * dt;
			if(world->paddle_velocities[i] > PADDLE_MAX_SPEED) {
				world->paddle_velocities[i] = PADDLE_MAX_SPEED;
			}
		}
		if(input->move_down > 0.0f) {
			world->paddle_velocities[i] -= input->move_down * PADDLE_ACCELERATION * dt;
			if(world->paddle_velocities[i] < -PADDLE_MAX_SPEED) {
				world->paddle_velocities[i] = -PADDLE_MAX_SPEED;
			}
		}

		if(world->paddle_velocities[i] > 0.0f) {
			world->paddle_velocities[i] -= PADDLE_FRICTION * dt;
			if(world->paddle_velocities[i] < 0.0f) {
				world->paddle_velocities[i] = 0.0f;
			}
		} else if(world->paddle_velocities[i] < 0.0f) {
			world->paddle_velocities[i] += PADDLE_FRICTION * dt;
			if(world->paddle_velocities[i] > 0.0f) {
				world->paddle_velocities[i] = 0.0f;
			}
		}

		world->paddle_positions[i] += world->paddle_velocities[i] * dt;
		world->ball_position[i] += world->ball_velocity[i] * dt;
	}

	f32 ball_left = world->ball_position[0] - BALL_WIDTH;
	f32 ball_right = world->ball_position[0] + BALL_WIDTH;
	f32 ball_bottom = world->ball_position[1] - BALL_WIDTH;
	f32 ball_top = world->ball_position[1] + BALL_WIDTH;

	i8 paddle_index = 0;
	if(world->ball_velocity[0] > 0.0f) {
		paddle_index = 1;
	}

	f32 paddle_x = -PADDLE_X + paddle_index * PADDLE_X * 2.0f;
	f32 paddle_left = paddle_x - PADDLE_HALF_WIDTH;
	f32 paddle_right = paddle_x + PADDLE_HALF_WIDTH;
	f32 paddle_bottom = world->paddle_positions[paddle_index] - PADDLE_HEIGHT;
	f32 paddle_top = world->paddle_positions[paddle_index] + PADDLE_HEIGHT;

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
}
