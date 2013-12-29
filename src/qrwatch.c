#include <pebble.h>
#include "QR_Encode.h"

static Window *window;
static Layer *qr_layer;
static TextLayer *time_layer = NULL;
static char time_str[6];
static char qr_str[15];
static int qr_width;

// TODO: Have this caluclated from font somehow, instead of hardcoding width and height?
#define TIME_SIZE_BLOCKS_W 6
#define TIME_SIZE_BLOCKS_H 3

void qr_draw(Layer *layer, GContext* ctx) {
	if(qr_width == 0) return;
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

	if(time_layer == NULL) {
		// time_layer is created here so we know the bounds of the QR code.  This way, we don't have to rely
		//  on the QR code always being the same size

		// The time will take up the last 6x3 blocks in the lower-right corner of the QR code.
		GRect time_position = {
				.origin = {
						layer_size.size.w - padding_x - (block_size * TIME_SIZE_BLOCKS_W),
						layer_size.size.h - padding_y - (block_size * TIME_SIZE_BLOCKS_H)
				},
				.size = {
						block_size * TIME_SIZE_BLOCKS_W,
						block_size * TIME_SIZE_BLOCKS_H
				}
		};
		time_layer = text_layer_create(time_position);
		text_layer_set_text_alignment(time_layer, GTextAlignmentRight);
		text_layer_set_overflow_mode(time_layer, GTextOverflowModeFill);
		text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
		layer_add_child(layer, text_layer_get_layer(time_layer));
	}

	// TODO: Optionally hide time, showing only QR code?
	text_layer_set_text(time_layer, time_str);
	layer_mark_dirty(text_layer_get_layer(time_layer));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  qr_layer = layer_create_with_data((GRect) { .origin = {0, 0}, .size = {bounds.size.w, bounds.size.h} }, MAX_BITDATA);
  layer_set_update_proc(qr_layer, qr_draw);
  layer_add_child(window_layer, qr_layer);

//  time_layer = text_layer_create((GRect) { .origin = {bounds.size.w/2 - 20, bounds.size.h/2 - 20}, .size = {30, 20}});
//  layer_add_child(window_layer, text_layer_get_layer(time_layer));
}

static void window_unload(Window *window) {
	layer_destroy(qr_layer);
	text_layer_destroy(time_layer);
}

static void update_time(struct tm *tick_time, TimeUnits units_changed) {
	unsigned char* data = layer_get_data(qr_layer);
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
