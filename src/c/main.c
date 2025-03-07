#include <pebble.h>

static Window *s_main_window;
static Layer *board;

#define FPS 60

static uint64_t frame = 0;
static GFont font;

static void board_redraw(Layer *layer, GContext *ctx) {
  frame++;
  GRect frame = layer_get_frame(layer);
  // User inputs
  // Draw board
}

static void new_frame(void *data) {
  app_timer_register(1000 / FPS, new_frame, NULL);
  layer_mark_dirty(board);
}

static void main_window_load(Window *window) {
  font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(window_layer);
  board = layer_create(frame);
  layer_set_update_proc(board, board_redraw);
  layer_add_child(window_layer, board);
  app_timer_register(1000 / FPS, new_frame, NULL);
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

static void deinit() { window_destroy(s_main_window); }

int main(void) {
  init();
  app_event_loop();
  deinit();
}
