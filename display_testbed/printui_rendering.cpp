#include "printui_utility.hpp"
#include "printui_render_definitions.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <algorithm>
#include <charconv>
#include <Windowsx.h>
#include <Shlobj.h>
#include <dxgi1_2.h>
#include <d3d11_3.h>
#include <DirectXMath.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <array>

namespace printui {
	namespace render {
		icon::~icon() {
			safe_release(rendered_layer);
		}
	}

	screen_space_rect intersection(screen_space_rect a, screen_space_rect b) {
		auto max_left = std::max(a.x, b.x);
		auto max_top = std::max(a.y, b.y);
		auto min_right = std::min(a.x + a.width, b.x + b.width);
		auto min_bottom = std::min(a.y + a.height, b.y + b.height);
		if(max_left < min_right && max_top < min_bottom) {
			return screen_space_rect{ max_left, max_top, min_right - max_left, min_bottom - max_top };
		} else {
			return screen_space_rect{ 0,0,0,0 };
		}
	}

	screen_space_point screen_point_from_layout(layout_orientation o,
		int32_t x_pos, int32_t y_pos, screen_space_rect const& in_rect) {

		switch(o) {
			case layout_orientation::horizontal_left_to_right:
				return screen_space_point{ x_pos + in_rect.x, y_pos + in_rect.y };
			case layout_orientation::horizontal_right_to_left:
				return screen_space_point{ (in_rect.x + in_rect.width) - x_pos, y_pos + in_rect.y };
			case layout_orientation::vertical_left_to_right:
				return screen_space_point{ y_pos + in_rect.x, x_pos + in_rect.y };
			case layout_orientation::vertical_right_to_left:
				return screen_space_point{ (in_rect.x + in_rect.width) - y_pos, x_pos + in_rect.y };
		}
		return screen_space_point{ 0, 0 };
	}


	screen_space_point screen_topleft_from_layout_in_ui(window_data const& win, int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height, screen_space_rect const& rect) {
		int32_t top_in = page_position * win.layout_size;
		int32_t left_in = line_position * win.layout_size;
		int32_t right_in = (line_position + line_width) * win.layout_size;
		int32_t bottom_in = (page_position + page_height) * win.layout_size;

		auto point_a = screen_point_from_layout(win.orientation, left_in, top_in, rect);
		auto point_b = screen_point_from_layout(win.orientation, right_in, bottom_in, rect);

		screen_space_point retvalue;
		retvalue.x = std::min(point_a.x, point_b.x);
		retvalue.y = std::min(point_a.y, point_b.y);

		return retvalue;
	}

	screen_space_rect reverse_screen_space_orientation(window_data const& win, screen_space_rect source) {
		screen_space_rect retval = source;

		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				break;
			case layout_orientation::horizontal_right_to_left:
				retval.x = win.ui_width - source.width;
				break;
			case layout_orientation::vertical_left_to_right:
				std::swap(retval.width, retval.height);
				std::swap(retval.x, retval.y);
				break;
			case layout_orientation::vertical_right_to_left:
				retval.x = win.ui_width - source.width;
				std::swap(retval.width, retval.height);
				std::swap(retval.x, retval.y);
				break;
		}
		return retval;
	}

	screen_space_rect screen_rectangle_from_layout_in_ui(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height,
		screen_space_rect const& rect) {

		screen_space_rect retval = screen_space_rect{ 0,0,0,0 };

		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				retval.x = rect.x + line_position * win.layout_size;
				retval.y = rect.y + page_position * win.layout_size;
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;

				
				break;
			case layout_orientation::horizontal_right_to_left:
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;
				retval.y = rect.y + page_position * win.layout_size;
				retval.x = rect.x + rect.width - (line_position * win.layout_size + retval.width);

				
				break;
			case layout_orientation::vertical_left_to_right:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = rect.y + line_position * win.layout_size;
				retval.x = rect.x + page_position * win.layout_size;

				
				break;
			case layout_orientation::vertical_right_to_left:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = rect.y + line_position * win.layout_size;
				retval.x = rect.x + rect.width - (page_position * win.layout_size + retval.width);

				break;
		}
		return retval;
	}

	screen_space_rect screen_rectangle_from_layout(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height) {

		screen_space_rect retval = screen_space_rect{ 0,0,0,0 };
		//int32_t bottom_line = win.layout_size * ((win.ui_height - win.window_border * 2) / win.layout_size) + win.window_border;
		//int32_t edge_line = (win.orientation == layout_orientation::horizontal_right_to_left || win.orientation == layout_orientation::vertical_right_to_left)
		//	? win.ui_width - (win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border)
		//	: win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border;

		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				retval.x = win.window_border + line_position * win.layout_size;
				retval.y = win.window_border + page_position * win.layout_size;
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;

				//if(retval.x + retval.width == edge_line) {
				//	retval.width = (win.ui_width - (retval.x + win.window_border));
				//}
				break;
			case layout_orientation::horizontal_right_to_left:
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;
				retval.y = win.window_border + page_position * win.layout_size;
				retval.x = win.ui_width - (win.window_border + line_position * win.layout_size + retval.width);

				//if(retval.x == edge_line) {
				//	retval.x = win.window_border;
				//	retval.width += (edge_line - win.window_border);
				//}
				break;
			case layout_orientation::vertical_left_to_right:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = win.window_border + line_position * win.layout_size;
				retval.x = win.window_border + page_position * win.layout_size;

				//if(retval.x + retval.width == edge_line) {
				//	retval.width = (win.ui_width - (retval.x + win.window_border));
				//}
				break;
			case layout_orientation::vertical_right_to_left:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = win.window_border + line_position * win.layout_size;
				retval.x = win.ui_width - (win.window_border + page_position * win.layout_size + retval.width);

				//if(retval.x == edge_line) {
				//	retval.x = win.window_border;
				//	retval.width += (edge_line - win.window_border);
				//}
				break;
		}
		//if(retval.y + retval.height == bottom_line) {
		//	retval.height = (win.ui_height - (retval.y + win.window_border));
		//}
		return retval;
	}

	void render_interface::render_foreground(ui_rectangle const&, window_data&) {

	}
	void render_interface::render_composite(ui_rectangle const& r, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(r.x_position), float(r.y_position),
			float(r.x_position + r.width), float(r.y_position + r.height) };

		win.rendering_interface->background_rectangle(r, r.display_flags, r.background_index, under_mouse, win);
		win.rendering_interface->fill_from_foreground(r, r.foreground_index, false);
	}

	namespace render {

		void update_preserved_rects(ui_rectangle const& r, direct2d_rendering& d2dri, ID2D1RectangleGeometry*& screen_rect_geom, ID2D1GeometryGroup*& with_preserved_rects, ID2D1Geometry*& old_rects, int32_t ui_width, int32_t ui_height) {

			if(!screen_rect_geom) {
				d2dri.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ 0.0f,0.0f, float(ui_width), float(ui_height) }, &screen_rect_geom);
			} else {
				d2dri.d2d_device_context->PopLayer();
			}
			if(old_rects) {
				ID2D1RectangleGeometry* new_rect = nullptr;
				d2dri.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ float(r.x_position), float(r.y_position),
					float(r.x_position + r.width), float(r.y_position + r.height) }, &new_rect);

				ID2D1PathGeometry1* new_rect_geom = nullptr;
				d2dri.d2d_factory->CreatePathGeometry(&new_rect_geom);
				ID2D1GeometrySink* geom_sink = nullptr;
				new_rect_geom->Open(&geom_sink);

				old_rects->CombineWithGeometry(new_rect, D2D1_COMBINE_MODE_UNION, nullptr, geom_sink);
				geom_sink->Close();
				safe_release(geom_sink);

				ID2D1Geometry* ppGeometries[] = { screen_rect_geom, new_rect_geom };

				safe_release(with_preserved_rects);
				d2dri.d2d_factory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE,
					ppGeometries, 2, &with_preserved_rects
				);

				safe_release(old_rects);
				old_rects = new_rect_geom;
				safe_release(new_rect);
			} else {
				ID2D1RectangleGeometry* new_rect = nullptr;
				d2dri.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ float(r.x_position), float(r.y_position),
					float(r.x_position + r.width), float(r.y_position + r.height) }, &new_rect);
				old_rects = new_rect;
				ID2D1Geometry* ppGeometries[] = { screen_rect_geom, new_rect };

				safe_release(with_preserved_rects);
				d2dri.d2d_factory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE,
					ppGeometries, 2, &with_preserved_rects
				);
			}
			d2dri.d2d_device_context->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), with_preserved_rects, D2D1_ANTIALIAS_MODE_ALIASED), nullptr);

		}

		D2D_RECT_F extend_rect_to_edges(D2D_RECT_F content_rect, window_data const& win) {
			int32_t bottom_line = win.layout_size * ((win.ui_height - win.window_border * 2) / win.layout_size) + win.window_border;
			if(content_rect.bottom == bottom_line) {
				content_rect.bottom = float(win.ui_height - win.window_border);
			}

			if(win.orientation == layout_orientation::horizontal_right_to_left || win.orientation == layout_orientation::vertical_right_to_left) {

				int32_t edge_line = win.ui_width - (win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border);
				if(content_rect.left == edge_line)
					content_rect.left = float(win.window_border);
			} else {
				int32_t edge_line = win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border;
				if(content_rect.right == edge_line)
					content_rect.right = float(win.ui_width - win.window_border);
			}
			return content_rect;
		}
		screen_space_rect extend_rect_to_edges(screen_space_rect content_rect, window_data const& win) {
			int32_t bottom_line = win.layout_size * ((win.ui_height - win.window_border * 2) / win.layout_size) + win.window_border;
			if(content_rect.y + content_rect.height == bottom_line) {
				content_rect.height = (win.ui_height - win.window_border) - content_rect.y;
			}

			if(win.orientation == layout_orientation::horizontal_right_to_left || win.orientation == layout_orientation::vertical_right_to_left) {

				int32_t edge_line = win.ui_width - (win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border);
				if(content_rect.x == edge_line) {
					content_rect.width = content_rect.width + content_rect.x - win.window_border;
					content_rect.x = win.window_border;
				}
			} else {
				int32_t edge_line = win.layout_size * ((win.ui_width - win.window_border * 2) / win.layout_size) + win.window_border;
				if(content_rect.x + content_rect.width == edge_line) {
					content_rect.width = (win.ui_width - win.window_border) - content_rect.x;
				}
			}
			return content_rect;
		}

		
	}

	

	void window_data::flag_for_update_from_interface(render_interface const* i) {
		if(i->l_id != layout_reference_none) {
			auto& n = get_node(i->l_id);
			if(n.visible_rect < get_ui_rects().size()) {
				get_ui_rects()[n.visible_rect].display_flags |= ui_rectangle::flag_needs_update;
				window_interface->invalidate_window();
			}
		}
	}


	void render::direct2d_rendering::composite_animation(window_data& win, uint32_t ui_width, uint32_t ui_height) {
		//composite regions outside of animation
		if(animation_status.description.animated_region.y > 0) {
			d2d_device_context->DrawBitmap(animation_foreground,
				D2D1_RECT_F{0.0f, 0.0f, float(ui_width), float(animation_status.description.animated_region.y) },
				1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
				D2D1_RECT_F{ 0.0f, 0.0f, float(ui_width), float(animation_status.description.animated_region.y) });
		}
		if(animation_status.description.animated_region.y + animation_status.description.animated_region.height < int32_t(ui_height)) {
			d2d_device_context->DrawBitmap(animation_foreground,
				D2D1_RECT_F{ 0.0f, float(animation_status.description.animated_region.y + animation_status.description.animated_region.height), float(ui_width), float(ui_height) },
				1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
				D2D1_RECT_F{ 0.0f, float(animation_status.description.animated_region.y + animation_status.description.animated_region.height), float(ui_width), float(ui_height) });
		}
		if(animation_status.description.animated_region.x > 0) {
			d2d_device_context->DrawBitmap(animation_foreground,
				D2D1_RECT_F{ 0.0f, float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
				1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
				D2D1_RECT_F{ 0.0f, float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
		}
		if(animation_status.description.animated_region.x + animation_status.description.animated_region.width <  int32_t(ui_width)) {
			d2d_device_context->DrawBitmap(animation_foreground,
				D2D1_RECT_F{ float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y), float(ui_width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
				1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
				D2D1_RECT_F{ float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y), float(ui_width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
		}
		auto duration = std::chrono::steady_clock::now() - animation_status.start_time;
		auto in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

		float percent_complete = std::min(float(in_ms.count()) / (1000.0f * animation_status.description.duration_seconds), 1.0f);

		switch(animation_status.description.type) {
			case animation_type::fade:
				if(animation_status.description.animate_in) {
					d2d_device_context->DrawBitmap(animation_background,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
						1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
					d2d_device_context->DrawBitmap(animation_foreground,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
						percent_complete, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
				} else {
					d2d_device_context->DrawBitmap(animation_foreground,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
						1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
					d2d_device_context->DrawBitmap(animation_background,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
						1.0f - percent_complete, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
						D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
				}
				break;
			case animation_type::slide:
				switch(animation_status.description.direction) {
					case animation_direction::bottom:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y) + (1.0f - percent_complete) * animation_status.description.animated_region.height,
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y) + percent_complete * animation_status.description.animated_region.height });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y) + (percent_complete) * animation_status.description.animated_region.height,
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y) + (1.0f - percent_complete) * animation_status.description.animated_region.height });
						}
						break;
					case animation_direction::top:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y) + percent_complete * animation_status.description.animated_region.height },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y) + (1.0f - percent_complete) * animation_status.description.animated_region.height,
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y) + (1.0f - percent_complete) * animation_status.description.animated_region.height },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y) + (percent_complete) * animation_status.description.animated_region.height,
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
					case animation_direction::right:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x) + (1.0f - percent_complete) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + (percent_complete) * animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x) + (percent_complete) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + (1.0f - percent_complete) * animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
					case animation_direction::left:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x) + (percent_complete) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x + (1.0f - percent_complete) * animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x) + (1.0f - percent_complete) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x + (percent_complete)*animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
				}
				break;
			case animation_type::flip:
			{
				float extent = (animation_status.description.animate_in) ? cos((1.0f - percent_complete) * 3.14159f / 2.0f) : cos((percent_complete) * 3.14159f / 2.0f);
				switch(animation_status.description.direction) {
					case animation_direction::bottom:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y) + (1.0f - extent) * animation_status.description.animated_region.height,
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y) + (1.0f - extent) * animation_status.description.animated_region.height,
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
					case animation_direction::top:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y) + extent * animation_status.description.animated_region.height },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y) + extent * animation_status.description.animated_region.height },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
					case animation_direction::left:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x) + extent * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x),
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x) + extent * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
					case animation_direction::right:
						if(animation_status.description.animate_in) {
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x) + (1.0f - extent) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						} else {
							d2d_device_context->DrawBitmap(animation_foreground,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
								1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
								D2D1_RECT_F{ float(animation_status.description.animated_region.x), float(animation_status.description.animated_region.y), float(animation_status.description.animated_region.x + animation_status.description.animated_region.width), float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
							d2d_device_context->DrawBitmap(animation_background,
								D2D1_RECT_F{
									float(animation_status.description.animated_region.x) + (1.0f - extent) * animation_status.description.animated_region.width,
									float(animation_status.description.animated_region.y),
									float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
									float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) },
									1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
									D2D1_RECT_F{
										float(animation_status.description.animated_region.x),
										float(animation_status.description.animated_region.y),
										float(animation_status.description.animated_region.x + animation_status.description.animated_region.width),
										float(animation_status.description.animated_region.y + animation_status.description.animated_region.height) });
						}
						break;
				}
				break;
			}
		}

		if(percent_complete == 1.0f) {
			animation_status.is_running = false;
		} else {
			win.window_interface->invalidate_window();
		}
	}

	namespace render {
		void direct2d_rendering::interactable_or_icon(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical, uint8_t icon_id) {

			interactable(win, location, state, fg_brush, vertical);
			if(win.keyboard_target || (!state.holds_key() && !state.holds_group())) {
				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));
				d2d_device_context->FillOpacityMask(
					icons[icon_id].rendered_layer,
					palette[fg_brush],
					D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
		void direct2d_rendering::interactable_or_foreground(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical) {

			interactable(win, location, state, fg_brush, vertical);
			if(win.keyboard_target || (!state.holds_key() && !state.holds_group())) {
				D2D1_RECT_F content_rect{
					float(location.x),
					float(location.y),
					float(location.x + win.layout_size),
					float(location.y + win.layout_size) };

				d2d_device_context->FillOpacityMask(foreground, palette[fg_brush], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
			}
		}
		void direct2d_rendering::interactable(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical) {

			if(win.keyboard_target)
				return;

			if(win.prompts != prompt_mode::controller) {
				auto is_light = win.dynamic_settings.brushes[fg_brush].is_light_color;
				auto resolved_brush = is_light ?
					(win.dynamic_settings.light_interactable_brush >= 0 ? win.dynamic_settings.light_interactable_brush : fg_brush) :
					(win.dynamic_settings.dark_interactable_brush >= 0 ? win.dynamic_settings.dark_interactable_brush : fg_brush);

				if(state.holds_key()) {
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));
					d2d_device_context->FillOpacityMask(
						vertical ? vertical_interactable[state.get_key()] : horizontal_interactable[state.get_key()],
						palette[resolved_brush],
						D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				} else if(state.holds_group()) {
					// TODO
				}
			} else {
				auto is_light = win.dynamic_settings.brushes[fg_brush].is_light_color;
				auto resolved_brush = is_light ?
					(win.dynamic_settings.light_interactable_brush >= 0 ? win.dynamic_settings.light_interactable_brush : fg_brush) :
					(win.dynamic_settings.dark_interactable_brush >= 0 ? win.dynamic_settings.dark_interactable_brush : fg_brush);

				if(state.holds_key()) {
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));

					int32_t resolved_icon = 0;
					if((win.controller_buttons.val & (win.controller_buttons.button_lb | win.controller_buttons.button_rb)) == 0) {
						if(state.get_key() < 4) {
							resolved_icon = state.get_key();
						} else if(state.get_key() < 8) {
							resolved_icon = 4;
						} else {
							resolved_icon = 5;
						}
					} else if((win.controller_buttons.val & win.controller_buttons.button_lb ) != 0) {
						if(state.get_key() < 4) {
							resolved_icon = 6;
						} else if(state.get_key() < 8) {
							resolved_icon = state.get_key() - 4;
						} else {
							resolved_icon = 5;
						}
					} else {
						if(state.get_key() < 4) {
							resolved_icon = 6;
						} else if(state.get_key() < 8) {
							resolved_icon = 4;
						} else {
							resolved_icon = state.get_key() - 8;
						}
					}
					d2d_device_context->FillOpacityMask(
						vertical ? vertical_controller_interactable[resolved_icon] : horizontal_controller_interactable[resolved_icon],
						palette[resolved_brush],
						D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				} else if(state.holds_group()) {
					// TODO
				}
			}
		}

	void direct2d_rendering::stop_ui_animations(window_data const&) {
		animation_status.is_running = false;
	}
	void direct2d_rendering::prepare_ui_animation(window_data& win) {
		if(win.dynamic_settings.uianimations == false)
			return;

		stop_ui_animations(win);
		refresh_foregound(win);

		d2d_device_context->SetTarget(animation_background);
		d2d_device_context->BeginDraw();

		d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
		d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		to_display(win.get_layout(), win);

		d2d_device_context->EndDraw();
	}

	void direct2d_rendering::start_ui_animation(animation_description description, window_data const& win) {
		if(win.dynamic_settings.uianimations == false)
			return;

		animation_status.description = description;
		animation_status.description.duration_seconds /= win.dynamic_settings.animation_speed_multiplier;
		animation_status.description.animated_region = extend_rect_to_edges(animation_status.description.animated_region, win);

		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				break;
			case layout_orientation::horizontal_right_to_left:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::right;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::left;
				break;
			case layout_orientation::vertical_left_to_right:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::top;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::bottom;
				else if(description.direction == animation_direction::top)
					animation_status.description.direction = animation_direction::left;
				else if(description.direction == animation_direction::bottom)
					animation_status.description.direction = animation_direction::right;
				break;
			case layout_orientation::vertical_right_to_left:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::top;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::bottom;
				else if(description.direction == animation_direction::top)
					animation_status.description.direction = animation_direction::right;
				else if(description.direction == animation_direction::bottom)
					animation_status.description.direction = animation_direction::left;
				break;
		}

		animation_status.is_running = true;
		animation_status.start_time = std::chrono::steady_clock::now();
		win.window_interface->invalidate_window();
	}

	void direct2d_rendering::register_in_place_animation() {
		if(!running_in_place_animation && !previous_frame_in_place_animation) {
			in_place_animation_start = std::chrono::steady_clock::now();
		}
		running_in_place_animation = true;
	}

	int64_t direct2d_rendering::in_place_animation_running_ms() const {
		if(running_in_place_animation || previous_frame_in_place_animation) {
			auto duration = std::chrono::steady_clock::now() - in_place_animation_start;
			auto in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			return in_ms.count();
		} else {
			return 0;
		}
	}

	
		direct2d_rendering::direct2d_rendering() {
			D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
			CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&wic_factory));


			{
				D2D1_STROKE_STYLE_PROPERTIES style_prop;
				style_prop.startCap = D2D1_CAP_STYLE_FLAT;
				style_prop.endCap = D2D1_CAP_STYLE_FLAT;
				style_prop.dashCap = D2D1_CAP_STYLE_FLAT;
				style_prop.lineJoin = D2D1_LINE_JOIN_MITER;
				style_prop.miterLimit = 10.0f;
				style_prop.dashStyle = D2D1_DASH_STYLE_SOLID;
				style_prop.dashOffset = 0.0f;
				d2d_factory->CreateStrokeStyle(style_prop, nullptr, 0, &plain_strokes);
			}

			horizontal_interactable_bg.file_name = L"left_select_i.svg";
			horizontal_interactable_bg.edge_padding = 0.0f;
			vertical_interactable_bg.file_name = L"top_select_i.svg";
			vertical_interactable_bg.edge_padding = 0.0f;

			icons[standard_icons::header_minimize].file_name = L"min_i.svg";
			icons[standard_icons::header_minimize].xsize = 1;
			icons[standard_icons::header_minimize].ysize = 1;
			icons[standard_icons::header_minimize].edge_padding = 0.25f;

			icons[standard_icons::header_close].file_name = L"close_i.svg";
			icons[standard_icons::header_close].xsize = 1;
			icons[standard_icons::header_close].ysize = 1;
			icons[standard_icons::header_close].edge_padding = 0.25f;

			icons[standard_icons::control_button].file_name = L"control_button.svg";
			icons[standard_icons::control_button].xsize = 1;
			icons[standard_icons::control_button].ysize = 1;
			icons[standard_icons::control_button].edge_padding = 0.2f;

			icons[standard_icons::control_menu].file_name = L"control_menu.svg";
			icons[standard_icons::control_menu].xsize = 1;
			icons[standard_icons::control_menu].ysize = 1;
			icons[standard_icons::control_menu].edge_padding = 0.0f;

			icons[standard_icons::control_pages].file_name = L"control_pages.svg";
			icons[standard_icons::control_pages].xsize = 1;
			icons[standard_icons::control_pages].ysize = 1;
			icons[standard_icons::control_pages].edge_padding = 0.0f;

			icons[standard_icons::control_list].file_name = L"control_list.svg";
			icons[standard_icons::control_list].xsize = 1;
			icons[standard_icons::control_list].ysize = 1;
			icons[standard_icons::control_list].edge_padding = 0.0f;

			icons[standard_icons::control_next].file_name = L"control_next.svg";
			icons[standard_icons::control_next].xsize = 1;
			icons[standard_icons::control_next].ysize = 1;
			icons[standard_icons::control_next].edge_padding = 0.0f;

			icons[standard_icons::control_next_next].file_name = L"control_next_next.svg";
			icons[standard_icons::control_next_next].xsize = 1;
			icons[standard_icons::control_next_next].ysize = 1;
			icons[standard_icons::control_next_next].edge_padding = 0.0f;

			icons[standard_icons::control_prev].file_name = L"control_prev.svg";
			icons[standard_icons::control_prev].xsize = 1;
			icons[standard_icons::control_prev].ysize = 1;
			icons[standard_icons::control_prev].edge_padding = 0.0f;

			icons[standard_icons::control_prev_prev].file_name = L"control_prev_prev.svg";
			icons[standard_icons::control_prev_prev].xsize = 1;
			icons[standard_icons::control_prev_prev].ysize = 1;
			icons[standard_icons::control_prev_prev].edge_padding = 0.0f;

			icons[standard_icons::control_toggle].file_name = L"control_toggle.svg";
			icons[standard_icons::control_toggle].xsize = 1;
			icons[standard_icons::control_toggle].ysize = 1;
			icons[standard_icons::control_toggle].edge_padding = 0.0f;

			icons[standard_icons::control_text].file_name = L"control_text.svg";
			icons[standard_icons::control_text].xsize = 1;
			icons[standard_icons::control_text].ysize = 1;
			icons[standard_icons::control_text].edge_padding = 0.0f;

			icons[standard_icons::window_settings].file_name = L"settings_i.svg";
			icons[standard_icons::window_settings].xsize = 2;
			icons[standard_icons::window_settings].ysize = 2;
			icons[standard_icons::window_settings].edge_padding = 0.5f;

			icons[standard_icons::window_max].file_name = L"max_i.svg";
			icons[standard_icons::window_max].xsize = 2;
			icons[standard_icons::window_max].ysize = 2;
			icons[standard_icons::window_max].edge_padding = 0.5f;

			icons[standard_icons::window_restore].file_name = L"restore_i.svg";
			icons[standard_icons::window_restore].xsize = 2;
			icons[standard_icons::window_restore].ysize = 2;
			icons[standard_icons::window_restore].edge_padding = 0.5f;

			icons[standard_icons::window_min].file_name = L"min_i.svg";
			icons[standard_icons::window_min].xsize = 2;
			icons[standard_icons::window_min].ysize = 2;
			icons[standard_icons::window_min].edge_padding = 0.5f;

			icons[standard_icons::window_info].file_name = L"alt_i.svg";
			icons[standard_icons::window_info].xsize = 2;
			icons[standard_icons::window_info].ysize = 2;
			icons[standard_icons::window_info].edge_padding = 0.5f;

			icons[standard_icons::window_close].file_name = L"close_i.svg";
			icons[standard_icons::window_close].xsize = 2;
			icons[standard_icons::window_close].ysize = 2;
			icons[standard_icons::window_close].edge_padding = 0.5f;
		}

		void direct2d_rendering::fill_from_foreground(screen_space_rect location, uint8_t fg_brush, bool optimize_for_text) {

			D2D1_RECT_F content_rect{
				float(location.x), float(location.y),
				float(location.x + location.width), float(location.y + location.height) };

			d2d_device_context->FillOpacityMask(foreground, palette[fg_brush], optimize_for_text ? D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL : D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);

		}

		void direct2d_rendering::background_rectangle(screen_space_rect rect, uint8_t display_flags, uint8_t brush, bool under_mouse, window_data const& win) {

			D2D1_RECT_F content_rect{
				float(rect.x), float(rect.y),
				float(rect.x + rect.width), float(rect.y + rect.height) };
			content_rect = extend_rect_to_edges(content_rect, win);

			if((display_flags & ui_rectangle::flag_skip_bg) == 0) {
				d2d_device_context->FillRectangle(content_rect, palette[brush]);
			}

			if((display_flags & ui_rectangle::flag_line_highlight) != 0) {
				if(under_mouse && (display_flags & ui_rectangle::flag_interactable) != 0) {
					d2d_device_context->FillRectangle(content_rect,
						win.dynamic_settings.brushes[brush].is_light_color ? dark_selected_line : light_selected_line);
				} else {
					d2d_device_context->FillRectangle(content_rect,
						win.dynamic_settings.brushes[brush].is_light_color ? dark_line : light_line);
				}
			} else if(under_mouse && (display_flags & ui_rectangle::flag_interactable) != 0) {
				d2d_device_context->FillRectangle(content_rect,
					win.dynamic_settings.brushes[brush].is_light_color ? dark_selected : light_selected);
			}
		}

		void direct2d_rendering::to_display(std::vector<ui_rectangle> const& uirects, window_data& win) {
			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			if(win.prompts != prompt_mode::hidden)
				win.repopulate_interactable_statuses();

			d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			
			auto real_ptr = (IDWriteFactory6*)(win.text_interface->get_dwrite_factory());
			auto hwnd = (HWND)(win.window_interface->get_hwnd());
			if(hwnd && real_ptr) {
				IDWriteRenderingParams* rparams = nullptr;
				auto monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
				real_ptr->CreateMonitorRenderingParams(monitor_handle, &rparams);
				if(rparams) {
					d2d_device_context->SetTextRenderingParams(rparams);
					safe_release(rparams);
				}
			}

			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			for(ui_reference i = 0; i < uirects.size(); ++i) {
				auto& r = uirects[i];
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					d2d_device_context->PopAxisAlignedClip();
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, *this, screen_rect_geom, with_preserved_rects, old_rects, win.ui_width, win.ui_height);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0) {
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
					if(!(r.parent_object.get_render_interface())) {
						background_rectangle(r, r.display_flags, r.background_index, win.last_under_cursor == i, win);
					} else {
						r.parent_object->render_composite(r, win, win.last_under_cursor == i);
					}
				}

				if((r.display_flags & ui_rectangle::flag_frame) != 0) {
					auto ex_rect = extend_rect_to_edges(screen_space_rect{ r.x_position, r.y_position, r.width, r.height }, win);
					if(r.left_border > 0) {
						d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y),
								float(ex_rect.x + r.left_border),
								float(ex_rect.y + ex_rect.height) },
								palette[r.foreground_index]);
					}
					if(r.right_border > 0) {
						d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x + ex_rect.width - r.right_border),
								float(ex_rect.y),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + ex_rect.height) },
								palette[r.foreground_index]);
					}
					if(r.top_border > 0) {
						d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + r.top_border) },
								palette[r.foreground_index]);
					}
					if(r.bottom_border > 0) {
						d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y + ex_rect.height - r.bottom_border),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + ex_rect.height) },
								palette[r.foreground_index]);
					}
				}

			}

			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			if(screen_rect_geom) {
				d2d_device_context->PopLayer();
			}

			if(win.prompts != prompt_mode::hidden)
				win.remove_interactable_statuses();

			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);
		}

		void direct2d_rendering::update_foregrounds(std::vector<ui_rectangle>& uirects, window_data& win) {


			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

			auto real_ptr = (IDWriteFactory6*)(win.text_interface->get_dwrite_factory());
			auto hwnd = (HWND)(win.window_interface->get_hwnd());
			if(hwnd && real_ptr) {
				IDWriteRenderingParams* rparams = nullptr;
				auto monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
				real_ptr->CreateMonitorRenderingParams(monitor_handle, &rparams);
				if(rparams) {
					d2d_device_context->SetTextRenderingParams(rparams);
					safe_release(rparams);
				}
			}

			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			for(auto& r : uirects) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {

				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, *this, screen_rect_geom, with_preserved_rects, old_rects, win.ui_width, win.ui_height);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0 && (r.display_flags & ui_rectangle::flag_needs_update) != 0) {
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

					d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					if(r.parent_object.get_render_interface())
						r.parent_object->render_foreground(r, win);

					d2d_device_context->PopAxisAlignedClip();
					r.display_flags &= ~ui_rectangle::flag_needs_update;
				}
			}
			if(screen_rect_geom) {
				d2d_device_context->PopLayer();
			}
			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);

		}

		void direct2d_rendering::foregrounds(std::vector<ui_rectangle>& uirects, window_data& win) {

			d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			
			auto real_ptr = (IDWriteFactory6*)(win.text_interface->get_dwrite_factory());
			auto hwnd = (HWND)(win.window_interface->get_hwnd());
			if(hwnd && real_ptr) {
				IDWriteRenderingParams* rparams = nullptr;
				auto monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
				real_ptr->CreateMonitorRenderingParams(monitor_handle, &rparams);
				if(rparams) {
					d2d_device_context->SetTextRenderingParams(rparams);
					safe_release(rparams);
				}
			}

			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			for(auto& r : uirects) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					d2d_device_context->PopAxisAlignedClip();
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, *this, screen_rect_geom, with_preserved_rects, old_rects, win.ui_width, win.ui_height);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0) {
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

					if(r.parent_object.get_render_interface()) {
						d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
						r.parent_object->render_foreground(r, win);
						d2d_device_context->PopAxisAlignedClip();
					}

					r.display_flags &= ~ui_rectangle::flag_needs_update;
				}
			}
			if(screen_rect_geom) {
				d2d_device_context->PopLayer();
			}
			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);
		}

		void direct2d_rendering::setup_icon_transform(window_data const& win, float edge_padding, int32_t xsize, int32_t ysize) {

			switch(win.orientation) {
				case layout_orientation::horizontal_left_to_right:
					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(
						win.layout_size * edge_padding / 2.0f,
						win.layout_size * edge_padding / 2.0f));
					break;
				case layout_orientation::horizontal_right_to_left:
					d2d_device_context->SetTransform(
						D2D1::Matrix3x2F::Translation(
							win.layout_size * edge_padding / 2.0f,
							win.layout_size * edge_padding / 2.0f) *
						D2D1::Matrix3x2F::Scale(-1.0f, 1.0f,
							D2D_POINT_2F{
								float(win.layout_size * xsize) / 2.0f,
								float(win.layout_size * ysize) / 2.0f }
					));
					break;
				case layout_orientation::vertical_right_to_left:
					d2d_device_context->SetTransform(
						D2D1::Matrix3x2F::Translation(
							win.layout_size * edge_padding / 2.0f,
							win.layout_size * edge_padding / 2.0f) *
						D2D1::Matrix3x2F::Rotation(90.0f,
							D2D_POINT_2F{
								float(win.layout_size * xsize) / 2.0f,
								float(win.layout_size * ysize) / 2.0f }
								)
					);
					break;
				case layout_orientation::vertical_left_to_right:
					d2d_device_context->SetTransform(
						D2D1::Matrix3x2F::Translation(
							win.layout_size * edge_padding / 2.0f,
							win.layout_size * edge_padding / 2.0f) *
						D2D1::Matrix3x2F::Rotation(-90.0f,
							D2D_POINT_2F{
								float(win.layout_size * xsize) / 2.0f,
								float(win.layout_size * ysize) / 2.0f }
								) *
						D2D1::Matrix3x2F::Scale(1.0f, -1.0f,
							D2D_POINT_2F{
								float(win.layout_size * xsize) / 2.0f,
								float(win.layout_size * ysize) / 2.0f }
								)
					);
					break;
			}

		}

		void direct2d_rendering::create_interactiable_tags(window_data& win) {

			auto label_format = win.text_interface->create_text_format(L"Arial", (win.layout_size * 15) / 32);


			std::array<ID2D1Bitmap1*, 12> text_bitmaps;

			for(uint32_t i = 0; i < 12; ++i) {
				WCHAR keyname[16];
				auto length = GetKeyNameTextW((uint32_t(win.sc_values[i]) << 16) | (1ui32 << 25) | 1ui32, keyname, 16);

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&text_bitmaps[i]);

				d2d_device_context->SetTarget(text_bitmaps[i]);
				d2d_device_context->BeginDraw();
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

				if(auto dwf = win.text_interface->to_dwrite_format(label_format); dwf)
					d2d_device_context->DrawTextW(keyname, length, (IDWriteTextFormat3*)dwf,
						D2D1_RECT_F{ 0.0f, (float(win.layout_size) * 47.0f) / 64.0f - label_format.baseline, float(win.layout_size), float(win.layout_size) }, dummy_brush);

				d2d_device_context->EndDraw();
			}

			std::array<ID2D1Bitmap1*, 12> button_text_bitmaps;
			wchar_t button_names[] = { L'Y', L'X', L'B', L'A', L'L', L'R', L'-' };

			for(uint32_t i = 0; i < 7; ++i) {

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&button_text_bitmaps[i]);

				d2d_device_context->SetTarget(button_text_bitmaps[i]);
				d2d_device_context->BeginDraw();
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

				if(auto dwf = win.text_interface->to_dwrite_format(label_format); dwf)
					d2d_device_context->DrawTextW(button_names + i, 1, (IDWriteTextFormat3*)dwf,
						D2D1_RECT_F{ 0.0f, (float(win.layout_size) * 5.0f) / 6.0f - label_format.baseline, float(win.layout_size), float(win.layout_size) }, dummy_brush);

				d2d_device_context->EndDraw();
			}

			ID2D1Effect* arithmeticCompositeEffect;
			d2d_device_context->CreateEffect(CLSID_D2D1ArithmeticComposite, &arithmeticCompositeEffect);
			arithmeticCompositeEffect->SetValue(D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS, D2D1::Vector4F(0.0f, 1.0f, -1.0f, 0.0f));

			arithmeticCompositeEffect->SetInput(0, horizontal_interactable_bg.rendered_layer);

			for(uint32_t i = 0; i < 12; ++i) {
				arithmeticCompositeEffect->SetInput(1, text_bitmaps[i]);

				safe_release(horizontal_interactable[i]);

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&(horizontal_interactable[i]));
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(horizontal_interactable[i]);
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

				d2d_device_context->DrawImage(arithmeticCompositeEffect);
				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
			}

			for(uint32_t i = 0; i < 7; ++i) {
				arithmeticCompositeEffect->SetInput(1, button_text_bitmaps[i]);

				safe_release(horizontal_controller_interactable[i]);

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&(horizontal_controller_interactable[i]));
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(horizontal_controller_interactable[i]);
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

				d2d_device_context->DrawImage(arithmeticCompositeEffect);
				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
			}

			arithmeticCompositeEffect->SetInput(0, vertical_interactable_bg.rendered_layer);

			for(uint32_t i = 0; i < 12; ++i) {
				arithmeticCompositeEffect->SetInput(1, text_bitmaps[i]);

				safe_release(vertical_interactable[i]);

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&(vertical_interactable[i]));
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(vertical_interactable[i]);
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });


				d2d_device_context->DrawImage(arithmeticCompositeEffect);
				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
			}

			for(uint32_t i = 0; i < 7; ++i) {
				arithmeticCompositeEffect->SetInput(1, button_text_bitmaps[i]);

				safe_release(vertical_controller_interactable[i]);

				d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size), uint32_t(win.layout_size) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&(vertical_controller_interactable[i]));
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(vertical_controller_interactable[i]);
				d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });


				d2d_device_context->DrawImage(arithmeticCompositeEffect);
				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
			}

			safe_release(arithmeticCompositeEffect);

			for(uint32_t i = 0; i < 12; ++i) {
				safe_release(text_bitmaps[i]);
			}
			for(uint32_t i = 0; i < 7; ++i) {
				safe_release(button_text_bitmaps[i]);
			}

			win.text_interface->release_text_format(label_format);
		}


		void direct2d_rendering::redraw_icons(window_data& win) {
			horizontal_interactable_bg.redraw_image(win, *this);
			vertical_interactable_bg.redraw_image(win, *this);

			for(uint32_t i = 0; i < standard_icons::final_icon; ++i) {
				icons[i].redraw_image(win, *this);
			}

			create_interactiable_tags(win);
		}

		void direct2d_rendering::recreate_dpi_dependent_resource(window_data& win) {
			redraw_icons(win);
		}

		void direct2d_rendering::create_highlight_brushes() {
			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.11f),
				&light_selected
			);
			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.06f),
				&light_line
			);
			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f),
				&light_selected_line
			);

			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.11f),
				&dark_selected
			);
			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.06f),
				&dark_line
			);
			d2d_device_context->CreateSolidColorBrush(
				D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.15f),
				&dark_selected_line
			);
		}

		void direct2d_rendering::create_palette(window_data const& win) {
			create_highlight_brushes();
			HRESULT hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &dummy_brush);

			palette_bitmaps.resize(win.dynamic_settings.brushes.size(), nullptr);
			palette.resize(win.dynamic_settings.brushes.size(), nullptr);

			int32_t i = 0;
			for(auto& b : win.dynamic_settings.brushes) {
				if(SUCCEEDED(hr)) {
					if(b.texture.length() > 0) {
						auto pth = win.file_system->resolve_file_path(b.texture, win.dynamic_settings.texture_directory);

						IWICBitmapDecoder* pDecoder = nullptr;
						IWICBitmapFrameDecode* pSource = nullptr;
						IWICStream* pStream = nullptr;
						IWICFormatConverter* pConverter = nullptr;
						IWICBitmapScaler* pScaler = nullptr;
						ID2D1BitmapBrush* t = nullptr;

						if(pth.has_value()) {
							hr = wic_factory->CreateDecoderFromFilename(pth->c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
						} else {
							hr = E_FAIL;
						}
						if(SUCCEEDED(hr)) {
							hr = pDecoder->GetFrame(0, &pSource);
						}
						if(SUCCEEDED(hr)) {
							hr = wic_factory->CreateFormatConverter(&pConverter);
						}
						if(SUCCEEDED(hr)) {
							hr = pConverter->Initialize(
								pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
								nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
						}
						if(SUCCEEDED(hr)) {
							hr = d2d_device_context->CreateBitmapFromWicBitmap(pConverter, nullptr, &palette_bitmaps[i]);
						}
						if(SUCCEEDED(hr)) {
							hr = d2d_device_context->CreateBitmapBrush(palette_bitmaps[i], &t);
						}
						if(SUCCEEDED(hr)) {
							t->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
							t->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
							palette[i] = t;
						}

						safe_release(pDecoder);
						safe_release(pSource);
						safe_release(pStream);
						safe_release(pConverter);
						safe_release(pScaler);

						if(FAILED(hr)) {
							ID2D1SolidColorBrush* u;
							hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(b.rgb.r, b.rgb.g, b.rgb.b, 1.0f), &u);
							palette[i] = u;
						};
					} else {
						ID2D1SolidColorBrush* t;
						hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(b.rgb.r, b.rgb.g, b.rgb.b, 1.0f), &t);
						palette[i] = t;
					}
				}
				++i;
			}
		}

		void direct2d_rendering::release_palette() {
			for(int32_t i = 0; i < palette.size(); ++i) {
				safe_release(palette[i]);
				safe_release(palette_bitmaps[i]);
			};
			safe_release(dummy_brush);
		}

		void direct2d_rendering::mark_for_complete_redraw() {
			redraw_completely_pending = true;
		}

		void direct2d_rendering::refresh_foregound(window_data& win) {
			win.get_layout();
			if(!redraw_completely_pending) {
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(foreground);
				d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				update_foregrounds(win.get_layout(), win);
				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
			} else {
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(foreground);
				d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

				foregrounds(win.get_layout(), win);

				d2d_device_context->SetTarget(nullptr);
				d2d_device_context->EndDraw();
				redraw_completely_pending = false;
			}
		}

		void direct2d_rendering::create_device_resources(window_data& win) {
			HRESULT hr = S_OK;

			if(!d3d_device) {
				IDXGIAdapter* pAdapter = nullptr;
				IDXGIFactory2* pDXGIFactory = nullptr;

				auto loc = win.window_interface->get_window_location();

				auto nWidth = loc.width;
				auto nHeight = loc.height;

				D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

				safe_release(d3d_device_context);

				hr = D3D11CreateDevice(
					nullptr,
					D3D_DRIVER_TYPE_HARDWARE,
					nullptr,
					D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
					levels, 4,
					D3D11_SDK_VERSION,
					&d3d_device,
					nullptr,
					&d3d_device_context
				);

				if(FAILED(hr)) {
					hr = D3D11CreateDevice(nullptr,
						D3D_DRIVER_TYPE_WARP,
						nullptr,
						D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
						levels, 4,
						D3D11_SDK_VERSION,
						&d3d_device,
						nullptr,
						&d3d_device_context
					);
				}

				if(SUCCEEDED(hr)) {
					safe_release(dxgi_device);
					hr = d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
				}
				if(SUCCEEDED(hr)) {
					safe_release(d2d_device);
					hr = d2d_factory->CreateDevice(dxgi_device, &d2d_device);
				}
				if(SUCCEEDED(hr)) {
					safe_release(d2d_device_context);
					hr = d2d_device->CreateDeviceContext(
						D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
						&d2d_device_context
					);
				}
				if(SUCCEEDED(hr)) {
					hr = dxgi_device->GetAdapter(&pAdapter);
				}
				if(SUCCEEDED(hr)) {
					hr = pAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
				}
				if(SUCCEEDED(hr)) {
					DXGI_SWAP_CHAIN_DESC1  swapDesc;
					::ZeroMemory(&swapDesc, sizeof(swapDesc));

					swapDesc.Width = nWidth;
					swapDesc.Height = nHeight;
					swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					swapDesc.Stereo = FALSE;
					swapDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };
					swapDesc.SampleDesc.Count = 1;
					swapDesc.SampleDesc.Quality = 0;
					swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
					swapDesc.BufferCount = 2;
					swapDesc.Scaling = DXGI_SCALING_NONE;
					swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
					swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
					swapDesc.Flags = 0;

					safe_release(swap_chain);
					hr = pDXGIFactory->CreateSwapChainForHwnd(d3d_device, (HWND)(win.window_interface->get_hwnd()), &swapDesc, nullptr, nullptr, &swap_chain);
				}
				if(SUCCEEDED(hr)) {
					hr = dxgi_device->SetMaximumFrameLatency(1);
				}

				create_palette(win);

				if(!SUCCEEDED(hr)) {
					win.window_interface->display_fatal_error_message(L"Could not create direct X devices, exiting");
					std::terminate();
				}

				safe_release(pAdapter);
				safe_release(pDXGIFactory);
			}
		}

		void direct2d_rendering::create_window_size_resources(window_data& win) {
			HRESULT hr = S_OK;
			IDXGISurface* back_buffer = nullptr;

			create_device_resources(win);

			if(!swap_chain)
				return;

			safe_release(back_buffer_target);
			safe_release(foreground);

			
			safe_release(animation_foreground);
			safe_release(animation_background);

			// Resize render target buffers
			d2d_device_context->SetTarget(nullptr);
			hr = swap_chain->ResizeBuffers(0, win.ui_width, win.ui_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

			if(SUCCEEDED(hr)) {
				hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
			}
			if(SUCCEEDED(hr)) {
				D2D1_BITMAP_PROPERTIES1 bitmapProperties =
					D2D1::BitmapProperties1(
						D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
						D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
						win.dpi, win.dpi
					);
				hr = d2d_device_context->CreateBitmapFromDxgiSurface(
					back_buffer,
					&bitmapProperties,
					&back_buffer_target
				);
			}

			safe_release(back_buffer);

			if(!back_buffer_target)
				std::abort();

			if(SUCCEEDED(hr)) {
				d2d_device_context->CreateBitmap(D2D1_SIZE_U{ win.ui_width, win.ui_height }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(
							DXGI_FORMAT_A8_UNORM,
							D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi,
						D2D1_BITMAP_OPTIONS_TARGET , nullptr },
					&foreground);

				redraw_completely_pending = true;

				d2d_device_context->CreateBitmap(D2D1_SIZE_U{ win.ui_width, win.ui_height }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(
							DXGI_FORMAT_B8G8R8A8_UNORM ,
							D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi,
						D2D1_BITMAP_OPTIONS_TARGET , nullptr },
					&animation_foreground);
				d2d_device_context->CreateBitmap(D2D1_SIZE_U{ win.ui_width, win.ui_height }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(
							DXGI_FORMAT_B8G8R8A8_UNORM ,
							D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi,
						D2D1_BITMAP_OPTIONS_TARGET , nullptr },
					&animation_background);
			}
		}

		void direct2d_rendering::release_device_resources() {
			safe_release(d3d_device);
			safe_release(d3d_device_context);
			safe_release(d2d_device);
			safe_release(d2d_device_context);
			safe_release(dxgi_device);
			safe_release(swap_chain);
			release_palette();
		}

		direct2d_rendering::~direct2d_rendering() {
			safe_release(d2d_factory);
			safe_release(wic_factory);

			safe_release(light_selected);
			safe_release(light_line);
			safe_release(light_selected_line);
			safe_release(dark_selected);
			safe_release(dark_line);
			safe_release(dark_selected_line);
			safe_release(dummy_brush);
			safe_release(plain_strokes);

			safe_release(foreground);

			safe_release(animation_foreground);
			safe_release(animation_background);


			for(uint32_t i = 0; i < 12; ++i) {
				safe_release(horizontal_interactable[i]);
				safe_release(vertical_interactable[i]);
			}
			for(auto& i : palette) {
				safe_release(i);
			}
			for(auto& i : palette_bitmaps) {
				safe_release(i);
			}

			safe_release(d3d_device);
			safe_release(dxgi_device);
			safe_release(d2d_device);

			safe_release(d3d_device_context);
			safe_release(d2d_device_context);
			safe_release(swap_chain);

			safe_release(back_buffer_target);
		}

		void direct2d_rendering::render(window_data& win) {
			HRESULT hr;

			//static int count = 0;

			create_device_resources(win);

			if(!back_buffer_target)
				return;

			if(!is_suspended) {

				refresh_foregound(win);

				if(win.window_interface->is_mouse_cursor_visible()) {
					win.last_under_cursor = reference_under_point(win.get_ui_rects(), win.last_cursor_x_position, win.last_cursor_y_position);
				}

				win.update_window_focus();

				previous_frame_in_place_animation = running_in_place_animation;
				running_in_place_animation = false;

				if(animation_status.is_running == false) {
					d2d_device_context->BeginDraw();
					d2d_device_context->SetTarget(back_buffer_target);
					d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));

					to_display(win.get_layout(), win);

					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
					if(win.window_border != 0) {
						d2d_device_context->DrawRectangle(D2D1_RECT_F{ win.window_border / 2.0f, win.window_border / 2.0f, float(win.ui_width) - win.window_border / 2.0f, float(win.ui_height) - win.window_border / 2.0f }, palette[1], float(win.window_border), plain_strokes);
					}
					d2d_device_context->SetTarget(nullptr);
					hr = d2d_device_context->EndDraw();
				} else {
					d2d_device_context->BeginDraw();
					d2d_device_context->SetTarget(animation_foreground);
					d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));

					to_display(win.get_layout(), win);

					d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
					if(win.window_border != 0) {
						d2d_device_context->DrawRectangle(D2D1_RECT_F{ win.window_border / 2.0f, win.window_border / 2.0f, float(win.ui_width) - win.window_border / 2.0f, float(win.ui_height) - win.window_border / 2.0f }, palette[1], float(win.window_border), plain_strokes);
					}

					d2d_device_context->SetTarget(back_buffer_target);
					d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));


					composite_animation(win, win.ui_width, win.ui_height);

					d2d_device_context->SetTarget(nullptr);
					hr = d2d_device_context->EndDraw();
				}

				if(running_in_place_animation)
					win.window_interface->invalidate_window();

				DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
				hr = swap_chain->Present1(1, 0, &params);
			} else {
				DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
				hr = swap_chain->Present1(1, DXGI_PRESENT_TEST, &params);
				if(hr == S_OK) {
					is_suspended = false;
					win.window_interface->invalidate_window();
				}
			}

			if(hr == DXGI_STATUS_OCCLUDED) {
				is_suspended = true;
				hr = S_OK;
			} else if(hr == DXGI_ERROR_DEVICE_RESET ||
				hr == DXGI_ERROR_DEVICE_REMOVED ||
				hr == D2DERR_RECREATE_TARGET) {
				hr = S_OK;
				release_device_resources();
			}
		}

		void direct2d_rendering::text(window_data const& win, ::printui::text::arranged_text* formatted_text, int32_t x, int32_t y) {
			auto dwl = win.text_interface->to_dwrite_layout(formatted_text);
			if(dwl)
				d2d_device_context->DrawTextLayout(D2D1_POINT_2F{ float(x), float(y) }, (IDWriteTextLayout*)dwl, dummy_brush, 0 /*D2D1_DRAW_TEXT_OPTIONS_CLIP*/);
		}

		void direct2d_rendering::set_brush_opacity(uint8_t b, float o) {
			palette[b]->SetOpacity(o);
		}
		void direct2d_rendering::fill_rectangle(screen_space_rect location, uint8_t b) {
			D2D_RECT_F rect{float(location.x), float(location.y), float(location.x + location.width), float(location.y + location.height) };
			d2d_device_context->FillRectangle(rect, palette[b]);
		}

		void direct2d_rendering::draw_icon(int32_t x, int32_t y, uint8_t ico, uint8_t br) {
			icons[ico].present_image(float(x), float(y), *this, palette[br]);
		}
		void direct2d_rendering::draw_icon_to_foreground(int32_t x, int32_t y, uint8_t ico) {
			icons[ico].present_image(float(x), float(y), *this, dummy_brush);
		}

		layout_position direct2d_rendering::get_icon_size(uint8_t ico) {
			return layout_position{ icons[ico].xsize, icons[ico].ysize };
		}

		void icon::redraw_image(window_data const& win, direct2d_rendering& ri) {
			safe_release(rendered_layer);

			auto resolved_path = win.file_system->resolve_file_path(file_name, win.dynamic_settings.icon_directory);

			if(resolved_path.has_value()) {
				IStream* fstream = nullptr;
				ID2D1SvgDocument* doc = nullptr;

				HRESULT hr = SHCreateStreamOnFileW(resolved_path->c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, &fstream);

				if(SUCCEEDED(hr)) {
					hr = ri.d2d_device_context->CreateSvgDocument(fstream, D2D1_SIZE_F{ win.layout_size * (1.0f - edge_padding) + win.layout_size * (xsize - 1), win.layout_size * (1.0f - edge_padding) + win.layout_size * (ysize - 1) }, &doc);

				}
				if(SUCCEEDED(hr)) {
					hr = ri.d2d_device_context->CreateBitmap(
						D2D1_SIZE_U{ uint32_t(win.layout_size * xsize), uint32_t(win.layout_size * ysize) }, nullptr, 0,
						D2D1_BITMAP_PROPERTIES1{
							D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
							win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
							&rendered_layer);
				}
				if(SUCCEEDED(hr)) {
					ri.d2d_device_context->BeginDraw();
					ri.d2d_device_context->SetTarget(rendered_layer);
					ri.d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });
					ri.setup_icon_transform(win, edge_padding, xsize, ysize);
					ri.d2d_device_context->DrawSvgDocument(doc);
					ri.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
					ri.d2d_device_context->SetTarget(nullptr);
					ri.d2d_device_context->EndDraw();
				}

				safe_release(fstream);
				safe_release(doc);
			}
		}

		void icon::present_image(float x, float y, direct2d_rendering& ri, ID2D1Brush* br) {
			if(rendered_layer) {
				ri.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
				ri.d2d_device_context->FillOpacityMask(
					rendered_layer,
					br,
					D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
				ri.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
	}
}