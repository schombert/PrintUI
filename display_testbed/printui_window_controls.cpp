#include "printui_main_header.hpp"

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

	accessibility_object* title_bar_element::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_plain_text_accessibility_interface(win, this, &text, false);
		}
		return acc_obj;
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

	simple_layout_specification title_bar_element::get_specification(window_data&) {
		simple_layout_specification spec;

		spec.minimum_page_size = 1ui16;
		spec.minimum_line_size = 4ui16;

		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;
		
		return spec;
	}

	vertical_2x2_max_icon::vertical_2x2_max_icon() {
		ico = standard_icons::window_max;
		display_vertically = true;
	}
	void vertical_2x2_max_icon::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		update_window_state(win);
		icon_button_base::render_composite(rect, win, under_mouse);
	}
	void vertical_2x2_max_icon::button_action(window_data& win) {
		if(win.window_interface.is_maximized())
			win.window_interface.restore(win);
		else
			win.window_interface.maximize(win);
	}
	void vertical_2x2_max_icon::update_window_state(window_data& win) {
		if(!win.window_interface.is_maximized()) {
			ico = standard_icons::window_max;
			set_name_text(win, text_id::maximize_name);
			set_alt_text(win, text_id::maximize_info);
		} else {
			ico = standard_icons::window_restore;
			set_name_text(win, text_id::restore_name);
			set_alt_text(win, text_id::restore_info);
		}
	}

	vertical_2x2_min_icon::vertical_2x2_min_icon() : icon_button_base(text_id::minimize_name, text_id::minimize_info) {
		ico = standard_icons::window_min;
		display_vertically = true;
	}
	void vertical_2x2_min_icon::button_action(window_data& win) {
		win.window_interface.minimize(win);
	}

	vertical_2x2_settings_icon::vertical_2x2_settings_icon() : icon_button_base(text_id::settings_name, text_id::settings_info) {
		ico = standard_icons::window_settings;
		display_vertically = true;
	}
	void vertical_2x2_settings_icon::button_action(window_data& win) {
		if(win.window_bar.expanded_show_settings)
			win.hide_settings_panel();
		else
			win.show_settings_panel();

		
	}

	void vertical_2x2_info_icon::mark_for_update(window_data& w) {
		if(l_id != layout_reference_none && w.get_node(l_id).visible_rect != ui_reference_none) {
			w.window_interface.invalidate_window();
		}
	}
	bool vertical_2x2_info_icon::is_toggled() const {
		return win.pending_right_click;
	}
	vertical_2x2_info_icon::vertical_2x2_info_icon(window_data& win) : icon_button_base(text_id::info_name, text_id::info_info), win(win) {
		ico = standard_icons::window_info;
		category = button_category::toggle_button;
		display_vertically = true;
	}
	void vertical_2x2_info_icon::button_action(window_data& w) {
		w.pending_right_click = !w.pending_right_click;
		mark_for_update(win);
	}
	void vertical_2x2_info_icon::render_composite(ui_rectangle const& rect, window_data& w, bool under_mouse) {
		if(is_toggled()) {
			set_name_text(w, text_id::info_name_on);
		} else {
			set_name_text(w, text_id::info_name);
		}
		icon_button_base::render_composite(rect, w, under_mouse);
	}

	vertical_2x2_close_icon::vertical_2x2_close_icon() : icon_button_base(text_id::close_name, text_id::close_info) {
		ico = standard_icons::window_close;
		display_vertically = true;
	}
	void vertical_2x2_close_icon::button_action(window_data& win) {
		win.window_interface.close(win);
	}

	window_bar_element::window_bar_element(window_data& win, bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& app_settings) : info_i(win), close_i(), settings_pages(win, app_settings), print_ui_settings() {
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
	simple_layout_specification window_bar_element::get_specification(window_data& ) {
		simple_layout_specification spec;

		spec.minimum_page_size = 4ui16;
		spec.minimum_line_size = 2ui16;

		spec.page_flags = size_flags::fill_to_max_single_col;
		spec.line_flags = size_flags::none;

		return spec;
	}

	void window_bar_element::recreate_contents(window_data& win, layout_node& n) {
		std::vector<layout_interface*> buttons;
		buttons.reserve(5);

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
		
		n.height = std::max(n.height, uint16_t(buttons.size() * 3));
		auto const bar_height = n.height;
		int32_t v_off = bar_height - int32_t(buttons.size()) * 3;
		for(auto b : buttons) {
			auto bid = win.create_node(b, 2, 3, false, false);
			win.immediate_add_child(l_id, bid);
			win.get_node(bid).x = 0;
			win.get_node(bid).y = uint16_t(v_off);
			v_off += 3;
		}

		auto settings_pages_id = win.create_node(&settings_pages, win.layout_width - 2, bar_height, true);
		auto& pages_node = win.get_node(settings_pages_id);
		win.immediate_add_child(l_id, settings_pages_id);
		pages_node.x = 2;
		pages_node.set_ignore(!expanded_show_settings);
	}
	accessibility_object* window_bar_element::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_container_accessibility_interface(win, this, text_id::window_bar_name);
		}
		return acc_obj;
	}

	void close_info_window::button_action(window_data& win) {
		win.info_popup.close(win, true);
	}

	ui_rectangle info_window::prototype_ui_rectangle(window_data const&, uint8_t , uint8_t ) {
		ui_rectangle retvalue;

		retvalue.background_index = background;
		retvalue.foreground_index = foreground;
		retvalue.parent_object = this;
		
		return retvalue;
	}
	simple_layout_specification info_window::get_specification(window_data& win) {
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
		win.rendering_interface.background_rectangle(rect, rect.display_flags, rect.background_index, under_mouse, win);
		win.rendering_interface.fill_from_foreground(screen_rectangle_from_layout_in_ui(win, 0, 1, win.get_node(l_id).width, win.get_node(l_id).height - 1, rect), rect.foreground_index, true);
	}
	void info_window::recreate_contents(window_data& win, layout_node& n) {
		auto const n_width = n.width;
		auto close_button_id = win.create_node(&close_button, 2, 1, false);
		win.immediate_add_child(l_id, close_button_id);
		win.get_node(close_button_id).x = n_width - 2ui16;
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
			win.rendering_interface.prepare_ui_animation(win);

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

		if(!win.window_interface.is_mouse_cursor_visible())
			win.set_window_focus(this);
		

		if(info_window::info_appearance.type != animation_type::none) {
			win.rendering_interface.start_ui_animation(animation_description{
				screen_rectangle_from_layout(win, self_node.x, self_node.y, self_node.width, self_node.height),
				info_window::info_appearance.type,
				appearance_direction,
				info_window::info_appearance.duration_seconds,
				info_window::info_appearance.animate_in }, win);
		}
		
	}
	void info_window::close(window_data& win, bool move_focus) {
		if(currently_visible && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(info_window::info_disappearance.type != animation_type::none) {
				win.rendering_interface.prepare_ui_animation(win);
				loc = win.get_current_location(l_id);
			}

			currently_visible = false;
			seeking_rect = screen_space_rect{0,0,0,0};

			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.back_focus_out_of(this);
			}

			if(info_window::info_disappearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					loc,
					info_window::info_disappearance.type,
					appearance_direction,
					info_window::info_disappearance.duration_seconds,
					info_window::info_disappearance.animate_in }, win);
			}
		}
	}

	animation_defintion info_window::info_appearance;
	animation_defintion info_window::info_disappearance;
}
