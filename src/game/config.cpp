// $TODO: Determines the type of client server setup we have. Temporary until we
// allow the user to switch between them at runtime.
// UPDATE: Are we still using these, or?
#define CONFIG_FULL_LOCAL 0
#define CONFIG_HALF_LOCAL 1
#define CONFIG_REMOTE 2
#define CONFIG_HALF_BOT 3
#define CONFIG_FULL_BOT 4
#define CONFIG_SETTING CONFIG_HALF_LOCAL

#define BASE_FRAME_LENGTH 0.01f

#define START_COUNTDOWN_SECONDS 1.0f

#define INPUT_WINDOW_FRAMES 10
#define READY_TIMEOUT_LENGTH 0.5

#define INPUT_ATTENUATION_SPEED 2.0f
#define VISUAL_SMOOTHING_SPEED 10.0f
#define VISUAL_SMOOTHING_EPSILON 0.001f

#define PADDLE_ACCELERATION 10.0f
#define PADDLE_MAX_SPEED 1.5f
#define PADDLE_FRICTION 5.0f
#define BALL_SPEED_INIT_X 1.0f
#define BALL_SPEED_INIT_Y BALL_SPEED_INIT_X * 0.66f

#define PADDLE_X 0.9f
#define PADDLE_WIDTH 0.025f
#define PADDLE_HEIGHT 0.1f
#define BALL_WIDTH 0.025f

#define PADDLE_HALF_WIDTH PADDLE_WIDTH / 2.0f
#define PADDLE_HALF_HEIGHT PADDLE_HEIGHT / 2.0f
#define BALL_HALF_WIDTH BALL_WIDTH / 2.0f
