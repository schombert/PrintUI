#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <d2d1_3.h>
#include <dwrite_3.h>

namespace printui {
	title_bar_element::~title_bar_element() {
	}

	layout_node_type title_bar_element::get_node_type() {
		return layout_node_type::visible;
	}

	void title_bar_element::set_text_content(uint16_t text_id) {
		text.set_text(text_id);
		text.text_alignment = content_alignment::centered;
	}

	void title_bar_element::render_foreground(ui_rectangle const& rect, window_data& win) {
		text.relayout_text(win, screen_space_point{rect.width, rect.height});
		text.draw_text(win, rect.x_position, rect.y_position);
	}

	ui_rectangle title_bar_element::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_foreground_index;
		retvalue.foreground_index = parent_background_index;
		retvalue.parent_object = this;

		return retvalue;
	}

	void title_bar_element::recreate_contents(window_data& , layout_node& ) {
	}

	simple_layout_specification title_bar_element::get_specification(window_data const&) {
		simple_layout_specification spec;

		spec.minimum_page_size = 1ui16;
		spec.minimum_line_size = 4ui16;

		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;
		
		return spec;
	}

	vertical_2x2_max_icon::vertical_2x2_max_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"max_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;

		restore_ico.file_name = L"restore_i.svg";
		restore_ico.xsize = 2;
		restore_ico.ysize = 2;
		restore_ico.edge_padding = 0.5f;
	}
	void vertical_2x2_max_icon::on_click(window_data& win, uint32_t, uint32_t) {
		if(win.window_interface->is_maximized())
			win.window_interface->restore(win);
		else
			win.window_interface->maximize(win);
	}
	void vertical_2x2_max_icon::on_right_click(window_data& win, uint32_t, uint32_t) {
		if(win.window_interface->is_maximized())
			win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::restore_info).width(8));
		else
			win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::maximize_info).width(8));
	}
	void vertical_2x2_max_icon::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 1, 2, 2, rect);

		if(win.window_interface->is_maximized())
			restore_ico.present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.dummy_brush);
		else
			ico.present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.dummy_brush);

	}

	vertical_2x2_min_icon::vertical_2x2_min_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"min_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_min_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.window_interface->minimize(win);
	}
	void vertical_2x2_min_icon::on_right_click(window_data& win, uint32_t, uint32_t) {
		win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::minimize_info).width(8));
	}

	vertical_2x2_settings_icon::vertical_2x2_settings_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"settings_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_settings_icon::on_click(window_data& win, uint32_t, uint32_t) {
		if(win.window_bar.expanded_show_settings)
			win.hide_settings_panel();
		else
			win.show_settings_panel();

		
	}
	void vertical_2x2_settings_icon::on_right_click(window_data& win, uint32_t, uint32_t) {
		win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::settings_info).width(8));
	}
	void vertical_2x2_info_icon::mark_for_update(window_data& win) {
		if(l_id != layout_reference_none && win.get_node(l_id).visible_rect != ui_reference_none) {
			auto const u_id = win.get_node(l_id).visible_rect;
			auto& ui_rects = win.get_ui_rects();

			RECT icon_rect{ ui_rects[u_id].x_position, ui_rects[u_id].y_position, ui_rects[u_id].x_position + ui_rects[u_id].width, ui_rects[u_id].y_position + ui_rects[u_id].height };

			win.window_interface->invalidate_window();
		}
	}
	void vertical_2x2_info_icon::render_composite(ui_rectangle const& r, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(r.x_position), float(r.y_position),
			float(r.x_position + r.width), float(r.y_position + r.height) };

		if(!win.pending_right_click) {
			render::background_rectangle(content_rect, win, r.display_flags, r.background_index, under_mouse);

			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[r.foreground_index], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
		} else {
			render::background_rectangle(content_rect, win, r.display_flags & ~ui_rectangle::flag_skip_bg, r.foreground_index, under_mouse);

			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[r.background_index], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
		}
	}
	vertical_2x2_info_icon::vertical_2x2_info_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"alt_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_info_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.pending_right_click = !win.pending_right_click;
		mark_for_update(win);
	}
	void vertical_2x2_info_icon::on_right_click(window_data& win, uint32_t, uint32_t) {
		win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::info_info).width(8));
	}
	ui_rectangle vertical_2x2_info_icon::prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle revalue = vertical_2x2_icon_base::prototype_ui_rectangle(win, parent_foreground_index, parent_background_index);
		standard_fg = parent_foreground_index;
		standard_bg = parent_background_index;
		return revalue;
	}


	vertical_2x2_close_icon::vertical_2x2_close_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"close_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_close_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.window_interface->close(win);
	}
	void vertical_2x2_close_icon::on_right_click(window_data& win, uint32_t, uint32_t) {
		win.info_popup.open(win, info_window::parameters{ l_id }.right().text(text_id::close_info).width(8));
	}

	window_bar_element::window_bar_element(window_data const& win, bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& app_settings) : info_i(), close_i(), settings_pages(win, app_settings) {
		if(mn)
			min_i.emplace();
		if(mx)
			max_i.emplace();
		if(settings)
			setting_i.emplace();
	}
	layout_node_type window_bar_element::get_node_type() {
		return layout_node_type::container;
	}
	ui_rectangle window_bar_element::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.right_border = 1ui8;

		return retvalue;
	}
	layout_rect window_bar_element::get_content_rectangle(window_data& win) {
		if(!expanded_show_settings || settings_pages.l_id == layout_reference_none) {
			return layout_rect{ 0i16, 0i16, 0i16, 0i16 };
		} else {
			return layout_rect{ 0i16, 0i16, int16_t(win.get_node(settings_pages.l_id).width), 0i16 };
		}
	}
	simple_layout_specification window_bar_element::get_specification(window_data const& ) {
		simple_layout_specification spec;

		spec.minimum_page_size = 4ui16;
		spec.minimum_line_size = 2ui16;

		spec.page_flags = size_flags::fill_to_max_single_col;
		spec.line_flags = size_flags::none;

		return spec;
	}

	auto_layout_specification window_bar_element::get_auto_layout_specification(window_data const& ) {
		auto_layout_specification spec;

		spec.orientation = content_orientation::page;
		spec.internal_alignment = content_alignment::trailing;
		spec.group_alignment = content_alignment::trailing;

		return spec;
	}
	void window_bar_element::recreate_contents(window_data& win, layout_node& n) {
		std::vector<layout_interface*> buttons;
		buttons.reserve(5);

		auto const bar_height = n.height;

		if(setting_i.has_value()) {
			buttons.push_back(&(*setting_i));
		}
		buttons.push_back(&info_i);

		if(min_i.has_value()) {
			buttons.push_back(&(*min_i));
		}
		if(max_i.has_value()) {
			buttons.push_back(&(*max_i));
		}
		buttons.push_back(&close_i);

		default_recreate_container(win, this, &n, buttons);

		auto existing_sp_width = settings_pages.l_id != layout_reference_none ? win.get_node(settings_pages.l_id).width : 6;
		auto settings_pages_id = win.create_node(&settings_pages, existing_sp_width, bar_height, true);
		auto& pages_node = win.get_node(settings_pages_id);
		pages_node.parent = l_id;
		win.get_node(l_id).container_info()->children.push_back(settings_pages_id);
		pages_node.x = 2;
		pages_node.ignore = !expanded_show_settings;
	}

	void close_info_window::on_click(window_data& win, uint32_t, uint32_t) {
		win.info_popup.close(win, true);
	}

	ui_rectangle info_window::prototype_ui_rectangle(window_data const&, uint8_t , uint8_t ) {
		ui_rectangle retvalue;

		retvalue.background_index = background;
		retvalue.foreground_index = foreground;
		retvalue.parent_object = this;
		
		return retvalue;
	}
	simple_layout_specification info_window::get_specification(window_data const& win) {
		simple_layout_specification spec;

		text.prepare_text(win);

		spec.minimum_page_size = uint16_t(text.resolved_text_size.y + 2);
		spec.minimum_line_size = uint16_t(text.resolved_text_size.x + 2);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;

		return spec;
	}
	void info_window::render_foreground(ui_rectangle const& rect, window_data& win) {
		text.relayout_text(win, screen_space_point{ rect.width - 2 * win.layout_size, rect.height - 2 * win.layout_size });

		auto text_height = text.get_lines_height(win);
		auto desired_top_px = connected_to_line * win.layout_size + win.layout_size / 2 - text_height / 2;
		auto actual_top = std::min(desired_top_px, rect.height - text_height - win.layout_size);
		actual_top = std::max(actual_top, win.layout_size);

		auto top_left = screen_point_from_layout(win.orientation, win.layout_size, actual_top, rect);
		auto bottom_right = screen_point_from_layout(win.orientation, rect.width - 2 * win.layout_size, actual_top + text_height, rect);

		text.draw_text(win, std::min(top_left.x, bottom_right.x), std::min(top_left.y, bottom_right.y));
	}
	void info_window::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		render::background_rectangle(content_rect, win, rect.display_flags, rect.background_index, under_mouse);

		win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[rect.foreground_index], D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL, content_rect, content_rect);
	}
	void info_window::recreate_contents(window_data& win, layout_node& n) {
		auto const n_width = n.width;
		auto* ci = n.container_info();
		auto close_button_id = win.create_node(&close_button, 2, 1, false);
		ci->children.push_back(close_button_id);
		win.get_node(close_button_id).x = n_width - 2ui16;
		win.get_node(close_button_id).parent = l_id;
	}
	void info_window::on_lose_focus(window_data& win) {
		if(currently_visible)
			close(win, false);
	}
	void info_window::on_focus(window_data&) {
		seeking_rect = screen_space_rect{ 0,0,0,0 };
	}
	void info_window::open(window_data& win, parameters const& params) {
		
		if(info_window::info_appearance.type != animation_type::none)
			win.prepare_ui_animation();

		currently_visible = true;
		

		if(params.text_id != uint16_t(-1)) {
			text.set_text(params.text_id, params.b, params.e);
		} else {
			text.set_text(L"ERROR TEXT MISSING");
		}

		win.create_node(this, win.layout_width, win.layout_height, true, true);
		auto& self_node = win.get_node(l_id);
		self_node.parent = params.attached_to;

		// move into position
		auto& attached_to_node = win.get_node(params.attached_to);
		if(attached_to_node.visible_rect >= win.get_ui_rects().size()) {
			currently_visible = false;
			return;
		}
		auto& attached_to_ui_rect = win.get_ui_rects()[attached_to_node.visible_rect];
		auto derotated_attached_rect = reverse_screen_space_orientation(win,
			screen_space_rect{ attached_to_ui_rect.x_position, attached_to_ui_rect.y_position,
			attached_to_ui_rect.width, attached_to_ui_rect.height });

		foreground = attached_to_ui_rect.foreground_index;
		background = attached_to_ui_rect.background_index;
		if(params.reverse_fg_bg)
			std::swap(foreground, background);

		auto inside = win.get_containing_proper_page(params.attached_to);
		if(inside != layout_reference_none) {
			auto enclosing_size = win.get_current_location(inside);
			auto derotated_enclosing_rect = reverse_screen_space_orientation(win, enclosing_size);

			self_node.y = uint16_t((derotated_enclosing_rect.y - win.window_border) / win.layout_size);
			self_node.height = uint16_t(derotated_enclosing_rect.height / win.layout_size);
			connected_to_line = int16_t((derotated_attached_rect.y + derotated_attached_rect.height / 2 - derotated_enclosing_rect.y) / win.layout_size);
		} else {
			self_node.y = uint16_t((win.is_title_set() ? 1 : 0));
			self_node.height = uint16_t(win.layout_height - (win.is_title_set() ? 1 : 0));
			connected_to_line = int16_t(((derotated_attached_rect.y + derotated_attached_rect.height / 2 - win.window_border) / win.layout_size) - 1);
		}
		self_node.width = uint16_t(params.width_value);
		auto space_on_left = (derotated_attached_rect.x - win.window_border) / win.layout_size;

		if(params.appear_on_left && params.width_value <= space_on_left + params.left_margin) {
			self_node.x = uint16_t(space_on_left - params.width_value + params.left_margin);
			appearance_direction = animation_direction::right;
			text.text_alignment = content_alignment::trailing;
		} else {
			self_node.x = uint16_t((derotated_attached_rect.x + derotated_attached_rect.width - win.window_border) / win.layout_size - params.right_margin);
			appearance_direction = animation_direction::left;
			text.text_alignment = content_alignment::leading;
		}
		text.invalidate();

		auto screen_location = screen_rectangle_from_layout(win, self_node.x, self_node.y, self_node.width, self_node.height);
		if(int32_t(win.last_cursor_x_position) < screen_location.x) {
			screen_location.width += (screen_location.x - win.last_cursor_x_position);
			screen_location.x = win.last_cursor_x_position;
		}
		if(int32_t(win.last_cursor_y_position) < screen_location.y) {
			screen_location.height += (screen_location.y - win.last_cursor_y_position);
			screen_location.y = win.last_cursor_y_position;
		}
		if(int32_t(win.last_cursor_x_position) > screen_location.x + screen_location.width) {
			screen_location.width = (screen_location.x - win.last_cursor_x_position);
		}
		if(int32_t(win.last_cursor_y_position) > screen_location.y + screen_location.height) {
			screen_location.height = (screen_location.y - win.last_cursor_y_position);
		}
		screen_location.x -= int32_t(5 * win.dpi / 96.0f);
		screen_location.y -= int32_t(5 * win.dpi / 96.0f);
		screen_location.width += int32_t(10 * win.dpi / 96.0f);
		screen_location.height += int32_t(10 * win.dpi / 96.0f);

		seeking_rect = screen_location;

		win.recreate_contents(this, &(win.get_node(l_id)));
		win.redraw_ui();
		win.get_layout();

		if(!win.window_interface->is_mouse_cursor_visible())
			win.set_window_focus(this);
		

		if(info_window::info_appearance.type != animation_type::none) {
			win.start_ui_animation(animation_description{
				screen_rectangle_from_layout(win, self_node.x, self_node.y, self_node.width, self_node.height),
				info_window::info_appearance.type,
				appearance_direction,
				info_window::info_appearance.duration_seconds,
				info_window::info_appearance.animate_in });
		}
		
	}
	void info_window::close(window_data& win, bool move_focus) {
		if(currently_visible && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(info_window::info_disappearance.type != animation_type::none) {
				win.prepare_ui_animation();
				loc = win.get_current_location(l_id);
			}

			currently_visible = false;
			seeking_rect = screen_space_rect{0,0,0,0};

			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.back_focus_out_of(this);
			}

			if(info_window::info_disappearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					loc,
					info_window::info_disappearance.type,
					appearance_direction,
					info_window::info_disappearance.duration_seconds,
					info_window::info_disappearance.animate_in });
			}
		}
	}

	animation_defintion info_window::info_appearance;
	animation_defintion info_window::info_disappearance;
}
