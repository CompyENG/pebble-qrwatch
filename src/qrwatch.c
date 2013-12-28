#include <pebble.h>
#include "QR_Encode.h"

static Window *window;
static Layer *qr_layer;
static char qr_str[15];
static int qr_width;

void qr_draw(Layer *layer, GContext* ctx) {
	int byte = 0;
	int bit = 7;

	GRect layer_size = layer_get_bounds(layer);
	int block_size = layer_size.size.w / qr_width;
	int padding_x = (layer_size.size.w - (block_size * qr_width)) / 2;
	int padding_y = (layer_size.size.h - (block_size * qr_width)) / 2;

	void *data = layer_get_data(layer);
	for(int y=0;y<qr_width;++y) {
		for(int x=0;x<qr_width;++x) {
			int value = (*((unsigned char*)data + byte)) & (1 << bit);
			if(--bit < 0) {
				bit = 7;
				++byte;
			}

			if(value)
				graphics_fill_rect(ctx, (GRect){.origin={padding_x+x*block_size, padding_y+y*block_size}, .size={block_size, block_size}}, 0, GCornerNone);
		}
	}
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  qr_layer = layer_create_with_data((GRect) { .origin = {0, 0}, .size = {bounds.size.w, bounds.size.h} }, MAX_BITDATA);
  layer_set_update_proc(qr_layer, qr_draw);
  layer_add_child(window_layer, qr_layer);
}

static void window_unload(Window *window) {
	layer_destroy(qr_layer);
}

static void update_time(struct tm *tick_time, TimeUnits units_changed) {
	unsigned char* data = layer_get_data(qr_layer);
	char time_str[6];
	clock_copy_time_string(time_str, 6);
	char date_str[10];
	strftime(date_str, 10, "\n%D", tick_time);

	strcpy(qr_str, time_str);
	strcat(qr_str, date_str);

	qr_width = EncodeData(QR_LEVEL_L, 0, qr_str, 0, data);
	layer_mark_dirty(qr_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, update_time);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
