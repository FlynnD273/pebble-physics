#include <pebble.h>

#define NUM_BALLS 20

static Window *s_main_window;
static Layer *board;
static int width;
static int height;

#define FPS 60
#define SCALE 128

static uint64_t f_count = 0;
static AccelData *accel = NULL;

typedef struct ball_struct {
  int x;
  int y;
  int vx;
  int vy;
  char restitution_percent;
  char friction_percent;
  int radius;
  int mass;
  GColor color;
} Ball;

static Ball balls[NUM_BALLS];

uint32_t isqrt(uint32_t n) {
  if (n == 0) {
    return 0;
  }
  uint32_t root = 0;
  uint32_t bit = 1 << 30;

  while (bit > n) {
    bit >>= 2;
  }

  while (bit != 0) {
    if (n >= root + bit) {
      n -= root + bit;
      root += 2 * bit;
    }
    root >>= 1;
    bit >>= 2;
  }
  return root;
}

static int dist_sqr(uint x1, uint y1, uint x2, uint y2) {
  int dx = x1 - x2;
  int dy = y1 - y2;
  return dx * dx + dy * dy;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  for (size_t i = 0; i < NUM_BALLS; i++) {
    Ball *ball = &balls[i];
    graphics_context_set_fill_color(ctx, ball->color);
    graphics_fill_circle(ctx, GPoint(ball->x / SCALE, ball->y / SCALE),
                         ball->radius / SCALE);
  }
}

static int min(int a, int b) { return a < b ? a : b; }
#define DOT(x1, y1, x2, y2) (x1 * x2 + y1 * y2)

static void resolve_collision(Ball *a, Ball *b) {
  int sidelen = a->radius + b->radius;
  int sidelen_sqr = sidelen * sidelen;
  int d_sqr = dist_sqr(a->x, a->y, b->x, b->y);
  if (d_sqr < 0) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "DISTANCE OVERFLOW");
  }
  if (d_sqr > sidelen_sqr) {
    return;
  }
  int dvx = b->vx - a->vx;
  int dvy = b->vy - a->vy;
  int dx = b->x - a->x;
  int dy = b->y - a->y;
  int d = isqrt(d_sqr);
  int vel_along_normal = DOT(dvx, dvy, dx / d, dy / d);
  if (vel_along_normal > 0) {
    return;
  }
  int restitution_percent = min(a->restitution_percent, b->restitution_percent);

  int impulse_scalar =
      -(vel_along_normal * 100 + vel_along_normal * restitution_percent) *
      a->mass * b->mass;
  int ix = dx * impulse_scalar;
  int iy = dy * impulse_scalar;
  int mass_sum = b->mass + a->mass;
  a->vx -= ix / a->mass / d / mass_sum / 100;
  a->vy -= iy / a->mass / d / mass_sum / 100;
  b->vx += ix / b->mass / d / mass_sum / 100;
  b->vy += iy / b->mass / d / mass_sum / 100;
  a->x += a->vx;
  a->y += a->vy;
  b->x += b->vx;
  b->y += b->vy;
}

/**
 * From this article:
 * https://code.tutsplus.com/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331t
 */
static void physics_frame() {
  for (size_t i = 0; i < NUM_BALLS; i++) {
    Ball *ball = &balls[i];
    ball->vx += accel->x;
    ball->vy -= accel->y;
    ball->vx = ball->vx * ball->friction_percent / 100;
    ball->vy = ball->vy * ball->friction_percent / 100;
    ball->x += ball->vx;
    ball->y += ball->vy;
#ifdef PBL_RECT
    if (ball->y + ball->radius > height) {
      ball->y = height - ball->radius;
      ball->vy = -ball->vy * ball->restitution_percent / 100;
    }
    if (ball->y - ball->radius < 0) {
      ball->y = ball->radius;
      ball->vy = -ball->vy * ball->restitution_percent / 100;
    }
    if (ball->x + ball->radius > width) {
      ball->x = width - ball->radius;
      ball->vx = -ball->vx * ball->restitution_percent / 100;
    }
    if (ball->x - ball->radius < 0) {
      ball->x = ball->radius;
      ball->vx = -ball->vx * ball->restitution_percent / 100;
    }
#else
    int d_sqr = dist_sqr(ball->x, ball->y, width / 2, height / 2);
    int max_dist = (width / 2) - ball->radius;
    if (d_sqr > max_dist * max_dist) {
      int d = isqrt(d_sqr);
      int newx = (ball->x - width / 2) * max_dist / d + width / 2;
      int newy = (ball->y - width / 2) * max_dist / d + width / 2;
      ball->vx += (newx - ball->x) * ball->restitution_percent / 100;
      ball->vy += (newy - ball->y) * ball->restitution_percent / 100;
      ball->x = newx;
      ball->y = newy;
    }
#endif
    for (size_t j = 0; j < NUM_BALLS; j++) {
      if (j == i) {
        continue;
      }
      resolve_collision(ball, &balls[j]);
    }
  }
}

static void new_frame(void *data) {
  f_count++;
  app_timer_register(1000 / FPS, new_frame, NULL);
  physics_frame();
  layer_mark_dirty(board);
}

static void accel_data_handler(AccelData *data, uint32_t count) {
  accel->x = data->x;
  accel->y = data->y;
  accel->z = data->z;
}

static void main_window_load(Window *window) {
#ifdef PBL_COLOR
  GColor colors[] = {
      GColorVividViolet,    GColorCadetBlue, GColorGreen,
      GColorFashionMagenta, GColorBlueMoon,  GColorWhite,
  };
#else
  GColor colors[] = {
      GColorLightGray,
      GColorWhite,
  };
#endif
  accel = (AccelData *)malloc(sizeof(AccelData));
  accel_service_peek(accel);
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(window_layer);
  board = layer_create(frame);
  width = frame.size.w * SCALE;
  height = frame.size.h * SCALE;
  size_t num_colors = sizeof(colors) / sizeof(GColor);
  for (size_t i = 0; i < NUM_BALLS; i++) {
    Ball *ball = &balls[i];
    ball->radius = 75 * SCALE / 10;
    ball->x = (rand() % (width - ball->radius * 2)) + ball->radius;
    ball->y = (rand() % (height - ball->radius * 2)) + ball->radius;
    ball->vx = (rand() % (10 * SCALE)) - 5 * SCALE;
    ball->vy = (rand() % (10 * SCALE)) - 5 * SCALE;
    ball->restitution_percent = 80;
    ball->friction_percent = 90;
    ball->mass = 1;
    ball->color = colors[i % num_colors];
  }
  layer_add_child(window_layer, board);
  layer_set_update_proc(board, frame_redraw);
  accel_data_service_subscribe(1, accel_data_handler);
  new_frame(NULL);
}

static void main_window_unload(Window *window) { layer_destroy(board); }

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
  layer_destroy(board);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
