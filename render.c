#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>

#include "pool-buffer.h"
#include "render.h"
#include "slurp.h"

static void set_source_u32(cairo_t *cairo, uint32_t color) {
	cairo_set_source_rgba(cairo, (color >> (3 * 8) & 0xFF) / 255.0,
		(color >> (2 * 8) & 0xFF) / 255.0,
		(color >> (1 * 8) & 0xFF) / 255.0,
		(color >> (0 * 8) & 0xFF) / 255.0);
}

static void draw_rect(cairo_t *cairo, struct slurp_box *box, uint32_t color, int32_t scale) {
	set_source_u32(cairo, color);
	cairo_rectangle(cairo, box->x * scale, box->y * scale,
			box->width * scale, box->height * scale);
}

static void box_layout_to_output(struct slurp_box *box, struct slurp_output *output) {
	box->x -= output->logical_geometry.x;
	box->y -= output->logical_geometry.y;
}

void render(struct slurp_output *output) {
	struct slurp_state *state = output->state;
	struct pool_buffer *buffer = output->current_buffer;
	cairo_t *cairo = buffer->cairo;
	int32_t scale = output->scale;

	// Clear
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	set_source_u32(cairo, state->colors.background);
	cairo_paint(cairo);

	// Draw option boxes from input
	struct slurp_box *choice_box;
	wl_list_for_each(choice_box, &state->boxes, link) {
		if (box_intersect(&output->logical_geometry,
					choice_box)) {
			struct slurp_box b = *choice_box;
			box_layout_to_output(&b, output);
			draw_rect(cairo, &b, state->colors.choice, scale);
			cairo_fill(cairo);
		}
	}

	struct slurp_seat *seat;
	wl_list_for_each(seat, &state->seats, link) {
		struct slurp_selection *current_selection =
			seat->touch_selection.has_selection ?
				&seat->touch_selection :
				&seat->pointer_selection;

		if (!current_selection->has_selection) {
			continue;
		}

		if (!box_intersect(&output->logical_geometry,
			&current_selection->selection)) {
			continue;
		}
		struct slurp_box b = current_selection->selection;
		box_layout_to_output(&b, output);

		draw_rect(cairo, &b, state->colors.selection, scale);
		cairo_fill(cairo);

		// Draw border
		cairo_set_line_width(cairo, state->border_weight * scale);
		draw_rect(cairo, &b, state->colors.border, scale);
		cairo_stroke(cairo);

		if (state->display_dimensions) {

      int32_t textX = 0;
      int32_t textY = 0;
      int32_t borderX = 0;
      int32_t borderY = 0;
      if(b.x < current_selection->anchor_x) {
        borderX = (b.x) * scale;
        textX = borderX + 8;
      } else {
        borderX = (b.x + b.width - ((12 * 5) + 10)) * scale;
        textX = borderX +8;
      }
      if(b.y < current_selection->anchor_y) {
        borderY = (b.y - 30) * scale;
        textY = borderY + 28;
      } else {
        borderY = (b.y + b.height + 10) * scale;
        textY = borderY + 28;
      }



      set_source_u32(cairo, 0x4f4f4fFF);
      cairo_rectangle(cairo, borderX, borderY,
                      ((12 * 5) + 6) * scale, (20 + 4) * scale);

      cairo_fill(cairo);

			cairo_select_font_face(cairo, "Sans",
					       CAIRO_FONT_SLANT_NORMAL,
					       CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size(cairo, 13 * scale);
			// buffer of 12 can hold selections up to 99999x99999
			char dimensions[12];
			snprintf(dimensions, sizeof(dimensions), "%ix%i",
				 b.width, b.height);
      set_source_u32(cairo, 0xFFFFFFFF);
			cairo_move_to(cairo, textX, textY);
			cairo_show_text(cairo, dimensions);
		}
	}
}
