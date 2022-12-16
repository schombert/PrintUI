#include "printui_utility.hpp"

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

	icon::~icon() {
		safe_release(rendered_layer);
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
		int32_t x_pos, int32_t y_pos, ui_rectangle const& in_rect) {

		switch(o) {
			case layout_orientation::horizontal_left_to_right:
				return screen_space_point{ x_pos + in_rect.x_position, y_pos + in_rect.y_position };
			case layout_orientation::horizontal_right_to_left:
				return screen_space_point{ (in_rect.x_position + in_rect.width) - x_pos, y_pos + in_rect.y_position };
			case layout_orientation::vertical_left_to_right:
				return screen_space_point{ y_pos + in_rect.x_position, x_pos + in_rect.y_position };
			case layout_orientation::vertical_right_to_left:
				return screen_space_point{ (in_rect.x_position + in_rect.width) - y_pos, x_pos + in_rect.y_position };
		}
		return screen_space_point{ 0, 0 };
	}


	screen_space_point screen_topleft_from_layout_in_ui(window_data const& win, int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height, ui_rectangle const& rect) {
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
		ui_rectangle const& rect) {

		screen_space_rect retval = screen_space_rect{ 0,0,0,0 };

		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				retval.x = rect.x_position + line_position * win.layout_size;
				retval.y = rect.y_position + page_position * win.layout_size;
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;

				
				break;
			case layout_orientation::horizontal_right_to_left:
				retval.width = line_width * win.layout_size;
				retval.height = page_height * win.layout_size;
				retval.y = rect.y_position + page_position * win.layout_size;
				retval.x = rect.x_position + rect.width - (line_position * win.layout_size + retval.width);

				
				break;
			case layout_orientation::vertical_left_to_right:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = rect.y_position + line_position * win.layout_size;
				retval.x = rect.x_position + page_position * win.layout_size;

				
				break;
			case layout_orientation::vertical_right_to_left:
				retval.height = line_width * win.layout_size;
				retval.width = page_height * win.layout_size;
				retval.y = rect.y_position + line_position * win.layout_size;
				retval.x = rect.x_position + rect.width - (page_position * win.layout_size + retval.width);

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

		render::background_rectangle(content_rect, win, r.display_flags, r.background_index, under_mouse);

		win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[r.foreground_index], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
	}

	namespace render {

		void update_preserved_rects(ui_rectangle const& r, window_data const& win, ID2D1RectangleGeometry*& screen_rect_geom, ID2D1GeometryGroup*& with_preserved_rects, ID2D1Geometry*& old_rects) {

			if(!screen_rect_geom) {
				win.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ 0.0f,0.0f, float(win.ui_width), float(win.ui_height) }, &screen_rect_geom);
			} else {
				win.d2d_device_context->PopLayer();
			}
			if(old_rects) {
				ID2D1RectangleGeometry* new_rect = nullptr;
				win.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ float(r.x_position), float(r.y_position),
					float(r.x_position + r.width), float(r.y_position + r.height) }, &new_rect);

				ID2D1PathGeometry1* new_rect_geom = nullptr;
				win.d2d_factory->CreatePathGeometry(&new_rect_geom);
				ID2D1GeometrySink* geom_sink = nullptr;
				new_rect_geom->Open(&geom_sink);

				old_rects->CombineWithGeometry(new_rect, D2D1_COMBINE_MODE_UNION, nullptr, geom_sink);
				geom_sink->Close();
				safe_release(geom_sink);

				ID2D1Geometry* ppGeometries[] = { screen_rect_geom, new_rect_geom };

				safe_release(with_preserved_rects);
				win.d2d_factory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE,
					ppGeometries, 2, &with_preserved_rects
				);

				safe_release(old_rects);
				old_rects = new_rect_geom;
				safe_release(new_rect);
			} else {
				ID2D1RectangleGeometry* new_rect = nullptr;
				win.d2d_factory->CreateRectangleGeometry(D2D_RECT_F{ float(r.x_position), float(r.y_position),
					float(r.x_position + r.width), float(r.y_position + r.height) }, &new_rect);
				old_rects = new_rect;
				ID2D1Geometry* ppGeometries[] = { screen_rect_geom, new_rect };

				safe_release(with_preserved_rects);
				win.d2d_factory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE,
					ppGeometries, 2, &with_preserved_rects
				);
			}
			win.d2d_device_context->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), with_preserved_rects, D2D1_ANTIALIAS_MODE_ALIASED), nullptr);

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

		void background_rectangle(D2D_RECT_F content_rect, window_data const& win, uint8_t display_flags, uint8_t brush, bool under_mouse) {

			content_rect = extend_rect_to_edges(content_rect, win);

			if((display_flags & ui_rectangle::flag_skip_bg) == 0) {
				win.d2d_device_context->FillRectangle(content_rect, win.palette[brush]);
			}

			if((display_flags & ui_rectangle::flag_line_highlight) != 0) {
				if(under_mouse && (display_flags & ui_rectangle::flag_interactable) != 0) {
					win.d2d_device_context->FillRectangle(content_rect,
						win.dynamic_settings.brushes[brush].is_light_color ? win.dark_selected_line : win.light_selected_line);
				} else {
					win.d2d_device_context->FillRectangle(content_rect,
						win.dynamic_settings.brushes[brush].is_light_color ? win.dark_line : win.light_line);
				}
			} else if(under_mouse && (display_flags & ui_rectangle::flag_interactable) != 0) {
				win.d2d_device_context->FillRectangle(content_rect,
					win.dynamic_settings.brushes[brush].is_light_color ? win.dark_selected : win.light_selected);
			}

		}

		void to_display(std::vector<ui_rectangle> const& uirects, window_data& win) {
			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			if(win.prompts != prompt_mode::hidden)
				win.repopulate_interactable_statuses();

			win.d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			win.window_interface->set_text_rendering_parameters(win.d2d_device_context, win.dwrite_factory);

			win.d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			for(ui_reference i = 0; i < uirects.size(); ++i) {
				auto& r = uirects[i];
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					win.d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					win.d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					win.d2d_device_context->PopAxisAlignedClip();
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, win, screen_rect_geom, with_preserved_rects, old_rects);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0) {
					win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

					if(!(r.parent_object.get_render_interface())) {
						D2D1_RECT_F content_rect{
							float(r.x_position), float(r.y_position),
							float(r.x_position + r.width), float(r.y_position + r.height) };

						render::background_rectangle(content_rect, win, r.display_flags, r.background_index, win.last_under_cursor == i);
					} else {
						r.parent_object->render_composite(r, win, win.last_under_cursor == i);
					}
				}

				if((r.display_flags & ui_rectangle::flag_frame) != 0) {
					auto ex_rect = extend_rect_to_edges(screen_space_rect{r.x_position, r.y_position, r.width, r.height}, win);
					if(r.left_border > 0) {
						win.d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y),
								float(ex_rect.x + r.left_border),
								float(ex_rect.y + ex_rect.height) },
								win.palette[r.foreground_index]);
					}
					if(r.right_border > 0) {
						win.d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x + ex_rect.width - r.right_border),
								float(ex_rect.y),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + ex_rect.height) },
								win.palette[r.foreground_index]);
					}
					if(r.top_border > 0) {
						win.d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + r.top_border) },
								win.palette[r.foreground_index]);
					}
					if(r.bottom_border > 0) {
						win.d2d_device_context->FillRectangle(
							D2D1_RECT_F{
								float(ex_rect.x),
								float(ex_rect.y + ex_rect.height - r.bottom_border),
								float(ex_rect.x + ex_rect.width),
								float(ex_rect.y + ex_rect.height) },
								win.palette[r.foreground_index]);
					}
				}

			}

			win.d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			if(screen_rect_geom) {
				win.d2d_device_context->PopLayer();
			}

			if(win.prompts != prompt_mode::hidden)
				win.remove_interactable_statuses();

			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);
		}

		void update_foregrounds(std::vector<ui_rectangle>& uirects, window_data& win) {


			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			win.d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			win.window_interface->set_text_rendering_parameters(win.d2d_device_context, win.dwrite_factory);

			win.d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			for(auto& r : uirects) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {

				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, win, screen_rect_geom, with_preserved_rects, old_rects);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0 && (r.display_flags & ui_rectangle::flag_needs_update) != 0) {
					win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

					win.d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					win.d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					if(r.parent_object.get_render_interface())
						r.parent_object->render_foreground(r, win);

					win.d2d_device_context->PopAxisAlignedClip();
					r.display_flags &= ~ui_rectangle::flag_needs_update;
				}
			}
			if(screen_rect_geom) {
				win.d2d_device_context->PopLayer();
			}
			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);

		}

		void foregrounds(std::vector<ui_rectangle>& uirects, window_data& win) {

			win.d2d_device_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			win.window_interface->set_text_rendering_parameters(win.d2d_device_context, win.dwrite_factory);

			win.d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			ID2D1RectangleGeometry* screen_rect_geom = nullptr;
			ID2D1GeometryGroup* with_preserved_rects = nullptr;
			ID2D1Geometry* old_rects = nullptr;

			for(auto& r : uirects) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					win.d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
					win.d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
					win.d2d_device_context->PopAxisAlignedClip();
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					update_preserved_rects(r, win, screen_rect_geom, with_preserved_rects, old_rects);
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0) {
					win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

					if(r.parent_object.get_render_interface()) {
						win.d2d_device_context->PushAxisAlignedClip(D2D1_RECT_F{
						float(r.x_position), float(r.y_position),
						float(r.x_position + r.width), float(r.y_position + r.height) }, D2D1_ANTIALIAS_MODE_ALIASED);
						r.parent_object->render_foreground(r, win);
						win.d2d_device_context->PopAxisAlignedClip();
					}
					
					r.display_flags &= ~ui_rectangle::flag_needs_update;
				}
			}
			if(screen_rect_geom) {
				win.d2d_device_context->PopLayer();
			}
			safe_release(screen_rect_geom);
			safe_release(old_rects);
			safe_release(with_preserved_rects);
		}
	}

	void setup_icon_transform(window_data const& win, float edge_padding, int32_t xsize, int32_t ysize) {
	
		switch(win.orientation) {
			case layout_orientation::horizontal_left_to_right:
				win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(
					win.layout_size * edge_padding / 2.0f,
					win.layout_size * edge_padding / 2.0f));
				break;
			case layout_orientation::horizontal_right_to_left:
				win.d2d_device_context->SetTransform(
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
				win.d2d_device_context->SetTransform(
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
				win.d2d_device_context->SetTransform(
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

	void icon::redraw_image(window_data const& win) {
		safe_release(rendered_layer);

		auto resolved_path = win.file_system->resolve_file_path(file_name, win.dynamic_settings.icon_directory);

		if(resolved_path.has_value()) {
			IStream* fstream = nullptr;
			ID2D1SvgDocument* doc = nullptr;

			HRESULT hr = SHCreateStreamOnFileW(resolved_path->c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, &fstream);

			if(SUCCEEDED(hr)) {
				hr = win.d2d_device_context->CreateSvgDocument(fstream, D2D1_SIZE_F{ win.layout_size * (1.0f - edge_padding) + win.layout_size * (xsize - 1), win.layout_size * (1.0f - edge_padding) + win.layout_size * (ysize - 1) }, &doc);
				
			}
			if(SUCCEEDED(hr)) {
				hr = win.d2d_device_context->CreateBitmap(
					D2D1_SIZE_U{ uint32_t(win.layout_size * xsize), uint32_t(win.layout_size * ysize) }, nullptr, 0,
					D2D1_BITMAP_PROPERTIES1{
						D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
						&rendered_layer);
			}
			if(SUCCEEDED(hr)) {
				win.d2d_device_context->BeginDraw();
				win.d2d_device_context->SetTarget(rendered_layer);
				win.d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });
				setup_icon_transform(win, edge_padding, xsize, ysize);
				win.d2d_device_context->DrawSvgDocument(doc);
				win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				win.d2d_device_context->SetTarget(nullptr);
				win.d2d_device_context->EndDraw();
			}

			safe_release(fstream);
			safe_release(doc);
		}
	}

	void icon::present_image(float x, float y, ID2D1DeviceContext5* context, ID2D1Brush* dummy_brush) {
		if(rendered_layer) {
			context->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
			context->FillOpacityMask(
				rendered_layer,
				dummy_brush,
				D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
			context->SetTransform(D2D1::Matrix3x2F::Identity());
		}
	}

	void window_data::init_layout_graphics() {
	}

	void standard_icons::redraw_icons(window_data& win) {
		for(uint32_t i = 0; i < standard_icons::final_icon; ++i) {
			icons[i].redraw_image(win);
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

	void load_base_interactable_tags(window_data& win) {
		win.horizontal_interactable_bg.redraw_image(win);
		win.vertical_interactable_bg.redraw_image(win);
	}

	void window_data::create_interactiable_tags() {

		IDWriteTextFormat3* label_format;

		DWRITE_FONT_AXIS_VALUE fax[] = {
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, DWRITE_FONT_WEIGHT_BOLD},
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, 100.0f } ,
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, 0.0f } };

		float target_pixels = float(layout_size) * 0.70f;

		IDWriteFont3* label_font = nullptr;

		IDWriteFontList2* fl;
		font_collection->GetMatchingFonts(L"Arial", fax, 3, &fl);
		fl->GetFont(0, &label_font);
		safe_release(fl);

		DWRITE_FONT_METRICS label_metrics;
		label_font->GetMetrics(&label_metrics);
		safe_release(label_font);

		auto font_size = float(label_metrics.designUnitsPerEm) * target_pixels / float(label_metrics.ascent + label_metrics.descent + label_metrics.lineGap);
		auto unscaled_font_size = float(label_metrics.designUnitsPerEm) * float(layout_size) / float(label_metrics.ascent + label_metrics.descent + label_metrics.lineGap);
		auto baseline = float(label_metrics.ascent) * ((font_size * 2.0f + unscaled_font_size) / 3.0f) / (float(label_metrics.designUnitsPerEm));

		dwrite_factory->CreateTextFormat(L"Arial", font_collection, fax, 3, font_size, L"", &label_format);
		label_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		label_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		load_base_interactable_tags(*this);

		std::array<ID2D1Bitmap1*, 12> text_bitmaps;

		for(uint32_t i = 0; i < 12; ++i) {
			WCHAR keyname[16];
			auto length = GetKeyNameTextW((uint32_t(sc_values[i]) << 16) | (1ui32 << 25) | 1ui32, keyname, 16);

			d2d_device_context->CreateBitmap(
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
					&text_bitmaps[i]);

			d2d_device_context->SetTarget(text_bitmaps[i]);
			d2d_device_context->BeginDraw();
			d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

			d2d_device_context->DrawTextW(keyname, length, label_format,
				D2D1_RECT_F{ 0.0f, dynamic_settings.primary_font.baseline - baseline, float(layout_size), dynamic_settings.primary_font.baseline - baseline + float(layout_size) },
				dummy_brush);

			d2d_device_context->EndDraw();
		}

		std::array<ID2D1Bitmap1*, 12> button_text_bitmaps;
		wchar_t button_names[] = { L'Y', L'X', L'B', L'A', L'L', L'R', L'-' };

		for(uint32_t i = 0; i < 7; ++i) {

			d2d_device_context->CreateBitmap(
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
					& button_text_bitmaps[i]);

			d2d_device_context->SetTarget(button_text_bitmaps[i]);
			d2d_device_context->BeginDraw();
			d2d_device_context->Clear(D2D1_COLOR_F{ 0.0f,0.0f,0.0f,0.0f });

			d2d_device_context->DrawTextW(button_names + i, 1, label_format,
				D2D1_RECT_F{ 0.0f, dynamic_settings.primary_font.baseline - baseline, float(layout_size), dynamic_settings.primary_font.baseline - baseline + float(layout_size) },
				dummy_brush);

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
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
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
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
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
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
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
				D2D1_SIZE_U{ uint32_t(layout_size), uint32_t(layout_size) }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi, D2D1_BITMAP_OPTIONS_TARGET, nullptr },
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

		safe_release(label_format);
	}


	void window_data::create_palette() {
		create_highlight_brushes();
		HRESULT hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &dummy_brush);

		palette_bitmaps.resize(dynamic_settings.brushes.size(), nullptr);
		palette.resize(dynamic_settings.brushes.size(), nullptr);

		int32_t i = 0;
		for(auto& b : dynamic_settings.brushes) {
			if(SUCCEEDED(hr)) {
				if(b.texture.length() > 0) {
					auto pth = file_system->resolve_file_path(b.texture, dynamic_settings.texture_directory);

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

	void window_data::release_palette() {
		for(int32_t i = 0; i < palette.size(); ++i) {
			safe_release(palette[i]);
			safe_release(palette_bitmaps[i]);
		};
		safe_release(dummy_brush);
	}


	void window_data::refresh_foregound() {
		get_layout();
		if(!redraw_completely_pending) {
			d2d_device_context->BeginDraw();
			d2d_device_context->SetTarget(foreground);
			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
			render::update_foregrounds(get_layout(), *this);
			d2d_device_context->SetTarget(nullptr);
			d2d_device_context->EndDraw();
		} else {
			d2d_device_context->BeginDraw();
			d2d_device_context->SetTarget(foreground);
			d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
			d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

			render::foregrounds(get_layout(), *this);

			d2d_device_context->SetTarget(nullptr);
			d2d_device_context->EndDraw();
			redraw_completely_pending = false;
		}
	}

	void window_data::composite_animation() {
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
			window_interface->invalidate_window();
		}
	}

	void window_data::render() {
		HRESULT hr;

		//static int count = 0;

		create_device_resources();

		if(!back_buffer_target)
			return;

		if(!is_suspended) {

			/* ++count;
			auto pstr = std::to_wstring(count);
			layout.title_bar.set_text_content(pstr.c_str(), pstr.length(), float(ui_width), float(layout_size), this->common_text_format, this->dwrite_factory);
			if(layout.title_bar.l_id != layout_reference_none) {
				auto vr = layout.get_node(layout.title_bar.l_id).visible_rect;
				if(vr != ui_reference_none)
					layout.get_ui_rects()[vr].display_flags |= ui_rectangle::flag_needs_update;
			}*/

			refresh_foregound();
			if(window_interface->is_mouse_cursor_visible()) {
				last_under_cursor = reference_under_point(get_ui_rects(), last_cursor_x_position, last_cursor_y_position);
			}

			update_window_focus();

			previous_frame_in_place_animation =  running_in_place_animation;
			running_in_place_animation = false;

			if(animation_status.is_running == false) {
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(back_buffer_target);
				d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));

				render::to_display(get_layout(), *this);

				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				if(window_border != 0) {
					d2d_device_context->DrawRectangle(D2D1_RECT_F{ window_border / 2.0f, window_border / 2.0f, float(ui_width) - window_border / 2.0f, float(ui_height) - window_border / 2.0f }, palette[1], float(window_border), plain_strokes);
				}
				d2d_device_context->SetTarget(nullptr);
				hr = d2d_device_context->EndDraw();
			} else {
				d2d_device_context->BeginDraw();
				d2d_device_context->SetTarget(animation_foreground);
				d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));

				render::to_display(get_layout(), *this);

				d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
				if(window_border != 0) {
					d2d_device_context->DrawRectangle(D2D1_RECT_F{ window_border / 2.0f, window_border / 2.0f, float(ui_width) - window_border / 2.0f, float(ui_height) - window_border / 2.0f }, palette[1], float(window_border), plain_strokes);
				}

				d2d_device_context->SetTarget(back_buffer_target);
				d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));


				composite_animation();

				d2d_device_context->SetTarget(nullptr);
				hr = d2d_device_context->EndDraw();
			}

			if(running_in_place_animation)
				window_interface->invalidate_window();

			DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
			hr = swap_chain->Present1(1, 0, &params);
		} else {
			DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
			hr = swap_chain->Present1(1, DXGI_PRESENT_TEST, &params);
			if(hr == S_OK) {
				is_suspended = false;
				window_interface->invalidate_window();
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



	namespace render {
		void interactable_or_icon(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical, icon const& ico) {

			interactable(win, dc, location, state, fg_brush, vertical);
			if(win.keyboard_target || (!state.holds_key() && !state.holds_group())) {
				dc->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));
				dc->FillOpacityMask(
					ico.rendered_layer,
					win.palette[fg_brush],
					D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
				dc->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
		void interactable_or_foreground(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical) {

			interactable(win, dc, location, state, fg_brush, vertical);
			if(win.keyboard_target || (!state.holds_key() && !state.holds_group())) {
				D2D1_RECT_F content_rect{
					float(location.x),
					float(location.y),
					float(location.x + win.layout_size),
					float(location.y + win.layout_size) };

				dc->FillOpacityMask(win.foreground, win.palette[fg_brush], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
			}
		}
		void interactable(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical) {

			if(win.keyboard_target)
				return;

			if(win.prompts != prompt_mode::controller) {
				auto is_light = win.dynamic_settings.brushes[fg_brush].is_light_color;
				auto resolved_brush = is_light ?
					(win.dynamic_settings.light_interactable_brush >= 0 ? win.dynamic_settings.light_interactable_brush : fg_brush) :
					(win.dynamic_settings.dark_interactable_brush >= 0 ? win.dynamic_settings.dark_interactable_brush : fg_brush);

				if(state.holds_key()) {
					dc->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));
					dc->FillOpacityMask(
						vertical ? win.vertical_interactable[state.get_key()] : win.horizontal_interactable[state.get_key()],
						win.palette[resolved_brush],
						D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
					dc->SetTransform(D2D1::Matrix3x2F::Identity());
				} else if(state.holds_group()) {
					// TODO
				}
			} else {
				auto is_light = win.dynamic_settings.brushes[fg_brush].is_light_color;
				auto resolved_brush = is_light ?
					(win.dynamic_settings.light_interactable_brush >= 0 ? win.dynamic_settings.light_interactable_brush : fg_brush) :
					(win.dynamic_settings.dark_interactable_brush >= 0 ? win.dynamic_settings.dark_interactable_brush : fg_brush);

				if(state.holds_key()) {
					dc->SetTransform(D2D1::Matrix3x2F::Translation(float(location.x), float(location.y)));

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
					dc->FillOpacityMask(
						vertical ? win.vertical_controller_interactable[resolved_icon] : win.horizontal_controller_interactable[resolved_icon],
						win.palette[resolved_brush],
						D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
					dc->SetTransform(D2D1::Matrix3x2F::Identity());
				} else if(state.holds_group()) {
					// TODO
				}
			}
		}
	}

	void window_data::stop_ui_animations() {
		animation_status.is_running = false;
	}
	void window_data::prepare_ui_animation() {
		if(dynamic_settings.uianimations == false)
			return;

		stop_ui_animations();

		refresh_foregound();

		d2d_device_context->SetTarget(animation_background);
		d2d_device_context->BeginDraw();

		d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
		d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		render::to_display(get_layout(), *this);

		d2d_device_context->EndDraw();
	}

	void window_data::start_ui_animation(animation_description description) {
		if(dynamic_settings.uianimations == false)
			return;

		animation_status.description = description;
		animation_status.description.duration_seconds /= dynamic_settings.animation_speed_multiplier;
		animation_status.description.animated_region = render::extend_rect_to_edges(animation_status.description.animated_region, *this);

		switch(orientation) {
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
		window_interface->invalidate_window();
	}

	void window_data::register_in_place_animation() {
		if(!running_in_place_animation && !previous_frame_in_place_animation) {
			in_place_animation_start = std::chrono::steady_clock::now();
		}
		running_in_place_animation = true;
	}

	int64_t window_data::in_place_animation_running_ms() const {
		if(running_in_place_animation || previous_frame_in_place_animation) {
			auto duration = std::chrono::steady_clock::now() - in_place_animation_start;
			auto in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			return in_ms.count();
		} else {
			return 0;
		}
	}
}