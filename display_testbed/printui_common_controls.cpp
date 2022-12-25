#include "printui_main_header.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <dwrite_3.h>
#include <d2d1_3.h>

namespace printui {
	//
	// SINGLE LINE HEADER
	//

	single_line_centered_header::~single_line_centered_header() {
	}

	ui_rectangle single_line_centered_header::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = 0ui8;
		retvalue.parent_object = this;

		return retvalue;
	}
	simple_layout_specification single_line_centered_header::get_specification(window_data& win) {
		simple_layout_specification spec;

		text.prepare_text(win);

		spec.minimum_page_size = 1ui16;
		spec.minimum_line_size = uint16_t(text.resolved_text_size.x + 4);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;

		return spec;
	}
	void single_line_centered_header::render_foreground(ui_rectangle const& rect, window_data& win) {

		text.relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - 4 * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - 4 * win.layout_size });

		auto text_rect = screen_rectangle_from_layout_in_ui(win, 2, 0, win.get_node(l_id).width - 4, 1, rect);
		text.draw_text(win, text_rect.x, text_rect.y);
	}
	void single_line_centered_header::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		win.rendering_interface.background_rectangle(rect, rect.display_flags, rect.background_index, under_mouse, win);

		auto new_content = screen_rectangle_from_layout_in_ui(win, 2, 0, win.get_node(l_id).width - 4, 1, rect);

		win.rendering_interface.fill_from_foreground(new_content, rect.foreground_index, true);

		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 0, 1, 1, rect);
		win.rendering_interface.interactable(win, new_top_left, saved_state, rect.foreground_index, true);

		if(saved_state.holds_key()) {
			// TODO
		} else if(saved_state.holds_group()) {
			// TODO
		}
	}
	void single_line_centered_header::recreate_contents(window_data& win, layout_node& self) {
		auto width = self.width;
		auto button = win.create_node(&close_button, 2, 1, false);
		auto& button_node = win.get_node(button);
		button_node.x = uint16_t(width - 2);
		win.immediate_add_child(l_id, button);
	}
	accessibility_object* single_line_centered_header::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_plain_text_accessibility_interface(win, this, &text, false);
		}
		return acc_obj;
	}

	//
	// LARGE HEADER
	//

	large_centered_header::~large_centered_header() {
	}

	ui_rectangle large_centered_header::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = 0ui8;
		retvalue.parent_object = this;

		return retvalue;
	}
	simple_layout_specification large_centered_header::get_specification(window_data& win) {
		simple_layout_specification spec;

		text.prepare_text(win);

		spec.minimum_page_size = 2ui16;
		spec.minimum_line_size = uint16_t(text.resolved_text_size.x + 4);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;

		return spec;
	}
	void large_centered_header::render_foreground(ui_rectangle const& rect, window_data& win) {

		text.relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - 4 * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - 4 * win.layout_size });

		auto text_rect = screen_rectangle_from_layout_in_ui(win, 2, 0, win.get_node(l_id).width - 4, 2, rect);
		text.draw_text(win, text_rect.x, text_rect.y);
	}
	void large_centered_header::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		win.rendering_interface.background_rectangle(rect, rect.display_flags, rect.background_index, under_mouse, win);

		auto new_content = screen_rectangle_from_layout_in_ui(win, 2, 0, win.get_node(l_id).width - 4, 2, rect);

		win.rendering_interface.fill_from_foreground(new_content, rect.foreground_index, true);

		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 1, 1, 2, rect);
		win.rendering_interface.interactable(win, new_top_left, saved_state, rect.foreground_index, true);

		if(saved_state.holds_key()) {
			// TODO
		} else if(saved_state.holds_group()) {
			// TODO
		}
	}
	void large_centered_header::recreate_contents(window_data& win, layout_node& self) {
		auto width = self.width;
		auto button = win.create_node(&close_button, 2, 1, false);
		auto& button_node = win.get_node(button);
		button_node.x = uint16_t(width - 2);
		win.immediate_add_child(l_id, button);
	}
	accessibility_object* large_centered_header::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_plain_text_accessibility_interface(win, this, &text, false);
		}
		return acc_obj;
	}

	//
	// EMPTY HEADER
	//

	ui_rectangle single_line_empty_header::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;
		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = 0ui8;
		retvalue.parent_object = this;
		return retvalue;
	}
	simple_layout_specification single_line_empty_header::get_specification(window_data&) {
		simple_layout_specification spec;
		spec.minimum_page_size = 1ui16;
		spec.minimum_line_size = 4ui16;
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;
		return spec;
	}
	void single_line_empty_header::render_composite(ui_rectangle const& rect, window_data& win, bool) {
		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 0, 1, 1, rect);
		win.rendering_interface.interactable(win, new_top_left, saved_state, rect.foreground_index, true);

		if(saved_state.holds_key()) {
			// TODO
		} else if(saved_state.holds_group()) {
			// TODO
		} else {

		}
	}

	int32_t stored_text::get_lines_height(window_data const& win) const {
		if(formatted_text) {
			return text::get_height(win, formatted_text, text_sz);
		} else {
			return 0;
		}

	}
	stored_text::~stored_text() {
	}
	void stored_text::invalidate() {
		formatted_text = nullptr;
	}
	void stored_text::relayout_text(window_data const& win, screen_space_point sz) {
		if(!std::holds_alternative<std::monostate>(text_content)) {
			prepare_text(win);
			text::adjust_layout_region(formatted_text, sz.x, sz.y);
		}
	}

	void stored_text::prepare_text(window_data const& win) {

		if(std::holds_alternative<wrapped_text_instance>(text_content)) {
			if(std::get<wrapped_text_instance>(text_content).text_generation != win.text_data.text_generation) {
				formatted_text = nullptr;
				std::get<wrapped_text_instance>(text_content).text_generation = win.text_data.text_generation;
			}

		} else if(std::holds_alternative<std::wstring>(text_content)) {
			if(formatted_text && !text::appropriate_directionality(win, formatted_text)) {
				formatted_text = nullptr;
			}
		}

		if(!formatted_text) {
			if(std::holds_alternative<wrapped_text_instance>(text_content)) {
				auto text_with_format = win.text_data.instantiate_text(std::get<wrapped_text_instance>(text_content).text_id, std::get<wrapped_text_instance>(text_content).stored_params, std::get<wrapped_text_instance>(text_content).stored_params + std::get<wrapped_text_instance>(text_content).params_count).text_content;

				auto arrangement = win.text_interface.create_text_arragement(win, text_with_format.text, text_alignment, text_sz, false, text_sz != text_size::note ? win.dynamic_settings.line_width : win.dynamic_settings.small_width, &(text_with_format.formatting));
				formatted_text = arrangement.ptr;
				resolved_text_size = layout_position{int16_t(arrangement.width_used), int16_t(arrangement.lines_used) };
				
			} else if(std::holds_alternative<std::wstring>(text_content)) {

				auto arrangement = win.text_interface.create_text_arragement(win, std::get<std::wstring>(text_content), text_alignment, text_sz, false, text_sz != text_size::note ? win.dynamic_settings.line_width : win.dynamic_settings.small_width);
				formatted_text = arrangement.ptr;
				resolved_text_size = layout_position{ int16_t(arrangement.width_used), int16_t(arrangement.lines_used) };
			}
		}
	}
	void stored_text::draw_text(window_data& win, int32_t x, int32_t y) const {
		win.rendering_interface.text(win, formatted_text, text_sz, x, y);
	}
	void stored_text::set_text(std::wstring const& v) {
		invalidate();
		text_content = v;
	}
	void stored_text::set_text(uint16_t text_id, text::text_parameter const* begin, text::text_parameter const* end) {
		invalidate();
		wrapped_text_instance temp;
		temp.params_count = uint8_t(end - begin);
		std::copy_n(begin, temp.params_count, temp.stored_params);
		temp.text_id = text_id;
		text_content = temp;
	}
	void stored_text::set_text() {
		invalidate();
		text_content = std::monostate{};
	}
	void stored_text::set_text(stored_text const& v) {
		invalidate();
		text_content = v.text_content;
	}
	std::wstring stored_text::get_raw_text(window_data const& win) const {
		if(std::holds_alternative<wrapped_text_instance>(text_content)) {
			return win.text_data.instantiate_text(std::get<wrapped_text_instance>(text_content).text_id, std::get<wrapped_text_instance>(text_content).stored_params, std::get<wrapped_text_instance>(text_content).stored_params + std::get<wrapped_text_instance>(text_content).params_count).text_content.text;
		} else if(std::holds_alternative<std::wstring>(text_content)) {
			return std::get<std::wstring>(text_content);
		} else {
			return L"";
		}
	}

	//
	// SIMPLE EDIT CONTROL
	//

	simple_editable_text::simple_editable_text(content_alignment text_alignment, uint16_t name, uint16_t alt_text, uint8_t minimum_layout_space, uint8_t lines) : analysis_obj(text::make_analysis_object()), text_alignment(text_alignment), name(name), alt_text(alt_text), minimum_layout_space(minimum_layout_space), layout_lines(lines) {
		
	}
	simple_editable_text::~simple_editable_text() {
	}

	void simple_editable_text::get_range_bounds(window_data& win, uint32_t position_start, uint32_t position_end, std::vector<screen_space_rect>& rects) {

		if(position_start == position_end)
			return;

		auto selection_start = std::min(position_start, position_end);
		auto selection_end = std::max(position_start, position_end);

		prepare_text(win);
;
		auto mstorage = text::get_metrics_for_range(formatted_text, selection_start, selection_end - selection_start);

		auto& node = win.get_node(l_id);
		auto location = win.get_current_location(l_id);
		ui_rectangle temp;
		temp.x_position = uint16_t(location.x);
		temp.y_position = uint16_t(location.y);
		temp.width = uint16_t(location.width);
		temp.height = uint16_t(location.height);

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, node.width - (node.left_margin() + node.right_margin()), node.height, temp);

		auto window_rect = win.window_interface.get_window_location();

		for(auto& r : mstorage) {

			if(horizontal(win.orientation)) {
				rects.push_back(screen_space_rect{
					int32_t(std::round(r.left)) + window_rect.x + new_content_rect.x,
					int32_t(std::round(r.top)) + window_rect.y + new_content_rect.y - win.layout_size * line_offset,
					int32_t(std::round(r.width)),
					int32_t(std::round(r.height)) });
			} else if(win.orientation == layout_orientation::vertical_left_to_right) {
				rects.push_back(screen_space_rect{
					int32_t(std::round(r.left)) + window_rect.x + new_content_rect.x - win.layout_size * line_offset,
					int32_t(std::round(r.top)) + window_rect.y + new_content_rect.y,
					int32_t(std::round(r.width)),
					int32_t(std::round(r.height)) });
			} else {
				rects.push_back(screen_space_rect{
					int32_t(std::round(r.left)) + window_rect.x + new_content_rect.x + win.layout_size * line_offset,
					int32_t(std::round(r.top)) + window_rect.y + new_content_rect.y,
					int32_t(std::round(r.width)),
					int32_t(std::round(r.height)) });
			}
		}
	}

	void make_regions_for_range(std::vector<simple_editable_text::selection_run>& cached_selection_region, text::arranged_text* formatted_text, int32_t selection_start, int32_t selection_end, bool horizontal, text::text_analysis_object* analysis_obj) {
		
		bool have_seen_end = false;
		bool have_seen_start = false;
		

		auto mstorage = text::get_metrics_for_range(formatted_text, selection_start, selection_end - selection_start);

		for(auto& r : mstorage) {
			bool left_to_right_section = (r.bidiLevel % 2) == 0;
			int32_t selection_start_coord = horizontal ? int32_t(std::round(r.left)) : int32_t(std::round(r.top));
			int32_t selection_end_coord = horizontal ? int32_t(std::round(r.left + r.width)) : int32_t(std::round(r.top + r.height));

			if(int32_t(r.textPosition) < selection_start && selection_start < int32_t(r.textPosition + r.length)) {
				//  selection start is strictly within region
				auto curmetrics = text::get_metrics_at_position(formatted_text, selection_start);

				int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
				int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, selection_start, (curmetrics.textPosition + curmetrics.length) - selection_start);
				float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

				if(horizontal) {
					selection_start_coord = int32_t(std::round(curmetrics.left + (left_to_right_section ? percentage_through_region * curmetrics.width : (1.0f - percentage_through_region) * curmetrics.width)));
				} else {
					selection_start_coord = int32_t(std::round(curmetrics.top + (left_to_right_section ? percentage_through_region * curmetrics.height : (1.0f - percentage_through_region) * curmetrics.height)));
				}
			}

			if(int32_t(r.textPosition) < selection_end && selection_end < int32_t(r.textPosition + r.length)) {
				//  selection end is strictly within region

				auto curmetrics = text::get_metrics_at_position(formatted_text, selection_end);

				int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
				int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, selection_end , (curmetrics.textPosition + curmetrics.length) - selection_end);
				float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

				if(horizontal) {
					selection_end_coord = int32_t(std::round(curmetrics.left + (left_to_right_section ? percentage_through_region * curmetrics.width : (1.0f - percentage_through_region) * curmetrics.width)));
				} else {
					selection_end_coord = int32_t(std::round(curmetrics.top + (left_to_right_section ? percentage_through_region * curmetrics.height : (1.0f - percentage_through_region) * curmetrics.height)));
				}
			}

			if(int32_t(r.textPosition) <= selection_end - 1 && selection_end - 1 < int32_t(r.textPosition + r.length))
				have_seen_end = true;
			if(int32_t(r.textPosition) <= selection_start && selection_start < int32_t(r.textPosition + r.length))
				have_seen_start = true;

			cached_selection_region.push_back(simple_editable_text::selection_run{ selection_start_coord, selection_end_coord, text::line_of_position(analysis_obj, r.textPosition) });
		}
		if(!have_seen_end) { // missing end of the selection, (why isn't it in the hittest region???)

			auto curmetrics = text::get_metrics_at_position(formatted_text, selection_end - 1);

			bool left_to_right_section = (curmetrics.bidiLevel % 2) == 0;
			int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
			int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, selection_end, (curmetrics.textPosition + curmetrics.length) - selection_end);
			float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

			if(horizontal) {
				if(left_to_right_section) {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.left)),
						int32_t(std::round(curmetrics.left + percentage_through_region * curmetrics.width)),
						text::line_of_position(analysis_obj, selection_end - 1) });
				} else {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.left + (1.0f - percentage_through_region) * curmetrics.width)),
						int32_t(std::round(curmetrics.left + curmetrics.width)),
						text::line_of_position(analysis_obj, selection_end - 1) });
				}
			} else {
				if(left_to_right_section) {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.top)),
						int32_t(std::round(curmetrics.top + percentage_through_region * curmetrics.height)),
						text::line_of_position(analysis_obj, selection_end - 1) });
				} else {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.top + (1.0f - percentage_through_region) * curmetrics.height)),
						int32_t(std::round(curmetrics.top + curmetrics.height)),
						text::line_of_position(analysis_obj, selection_end - 1) });
				}
			}
		}

		if(!have_seen_start) { // missing end of the selection, (why isn't it in the hittest region???)
			auto curmetrics = text::get_metrics_at_position(formatted_text, selection_start);

			bool left_to_right_section = (curmetrics.bidiLevel % 2) == 0;
			int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
			int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, selection_start, (curmetrics.textPosition + curmetrics.length) - selection_start);
			float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

			if(horizontal) {
				if(left_to_right_section) {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.left)),
						int32_t(std::round(curmetrics.left + percentage_through_region * curmetrics.width)),
						text::line_of_position(analysis_obj, selection_start) });
				} else {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.left + (1.0f - percentage_through_region) * curmetrics.width)),
						int32_t(std::round(curmetrics.left + curmetrics.width)),
						text::line_of_position(analysis_obj, selection_start) });
				}
			} else {
				if(left_to_right_section) {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.top)),
						int32_t(std::round(curmetrics.top + percentage_through_region * curmetrics.height)),
						text::line_of_position(analysis_obj, selection_start) });
				} else {
					cached_selection_region.push_back(simple_editable_text::selection_run{
						int32_t(std::round(curmetrics.top + (1.0f - percentage_through_region) * curmetrics.height)),
						int32_t(std::round(curmetrics.top + curmetrics.height)),
						text::line_of_position(analysis_obj, selection_start) });
				}
			}
		}
	}

	void simple_editable_text::prepare_selection_regions(window_data const& win) {
		if(selection_out_of_date) {
			cached_selection_region.clear();

			if(formatted_text) {
				if(anchor_position != cursor_position) {
					auto selection_start = std::min(anchor_position, cursor_position);
					auto selection_end = std::max(anchor_position, cursor_position);

					make_regions_for_range(cached_selection_region, formatted_text, selection_start, selection_end, horizontal(win.orientation), analysis_obj);
				}
				if(temp_text_length > 0) {
					auto selection_start = temp_text_position;
					auto selection_end = temp_text_position + temp_text_length;

					make_regions_for_range(cached_selection_region, formatted_text, selection_start, selection_end, horizontal(win.orientation), analysis_obj);
				}
			}

			cached_cursor_postion = 0;
			if(formatted_text) {
				bool is_at_text_end = cursor_position == text.length();
				auto curmetrics = text::get_metrics_at_position(formatted_text, is_at_text_end ? cursor_position - 1 : cursor_position);

				float coff_pos = 0.0f;
				bool left_to_right_section = (curmetrics.bidiLevel % 2) == 0;

				if(is_at_text_end) {
					if(horizontal(win.orientation)) {
						coff_pos = curmetrics.left + (left_to_right_section ? curmetrics.width : 0.0f);
					} else {
						coff_pos = curmetrics.top + (left_to_right_section ? curmetrics.height : 0.0f);
					}
				} else {
					int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
					if(num_positions_in_metrics == 1 || curmetrics.length == 0) {
						if(horizontal(win.orientation)) {
							coff_pos = curmetrics.left + (left_to_right_section ? 0.0f : curmetrics.width);
						} else {
							coff_pos = curmetrics.top + (left_to_right_section ? 0.0f : curmetrics.height);
						}
					} else {
						int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, cursor_position, (curmetrics.textPosition + curmetrics.length) - cursor_position);
						float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

						if(horizontal(win.orientation)) {
							coff_pos = curmetrics.left + (left_to_right_section ? percentage_through_region * curmetrics.width : (1.0f - percentage_through_region) * curmetrics.width);
						} else {
							coff_pos = curmetrics.top + (left_to_right_section ? percentage_through_region * curmetrics.height : (1.0f - percentage_through_region) * curmetrics.height);
						}
					}
				}

				cached_cursor_postion = int32_t(std::round(coff_pos));
			}

			selection_out_of_date = false;
		}
	}
	bool simple_editable_text::is_word_position(uint32_t v) {
		return text::is_word_position(analysis_obj, int32_t(v));
	}
	bool simple_editable_text::is_valid_cursor_position(uint32_t v) {
		return text::is_cursor_position(analysis_obj, int32_t(v));
	}
	uint32_t simple_editable_text::previous_word_position(uint32_t v) {
		return text::get_previous_word_position(analysis_obj, int32_t(v));
	}
	uint32_t simple_editable_text::next_word_position(uint32_t v) {
		return text::get_next_word_position(analysis_obj, int32_t(v));
	}
	uint32_t simple_editable_text::previous_valid_cursor_position(uint32_t v) {
		return text::get_previous_cursor_position(analysis_obj, int32_t(v));
	}
	uint32_t simple_editable_text::next_valid_cursor_position(uint32_t v) {
		return text::get_next_cursor_position(analysis_obj, int32_t(v));
	}
	bool simple_editable_text::is_start_of_line(uint32_t v) {
		return text::start_of_line(analysis_obj, text::line_of_position(analysis_obj, v)) == int32_t(v);
	}
	uint32_t simple_editable_text::start_of_line(uint32_t v) {
		return text::start_of_line(analysis_obj, v);
	}
	uint32_t simple_editable_text::line_of_position(uint32_t v) {
		return text::line_of_position(analysis_obj, v);
	}
	uint32_t simple_editable_text::number_of_text_lines() {
		return text::number_of_lines(analysis_obj);
	}
	uint32_t simple_editable_text::first_visible_line() {
		return uint32_t(line_offset);
	}

	void simple_editable_text::internal_on_text_changed(window_data& win) {
		formatted_text = nullptr;
		
		selection_out_of_date = true;
		changes_made = true;

		prepare_text(win);
		update_analyzed_text(analysis_obj, formatted_text, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);

		win.flag_for_update_from_interface(this);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_text_content_changed(acc_obj);
			if(edit_type != edit_contents::number)
				win.accessibility_interface.on_text_value_changed(acc_obj);
			else
				win.accessibility_interface.on_text_numeric_value_changed(acc_obj);
		}
		on_text_changed(win, text);
		if(win.is_visible(l_id)) {
			win.window_interface.invalidate_window();
		}
		if(win.keyboard_target != this) {
			on_edit_finished(win, text);
		}
		internal_on_selection_changed(win);
	}
	void simple_editable_text::internal_on_selection_changed(window_data& win) {
		selection_out_of_date = true;
		if(acc_obj && win.is_visible(l_id))
			win.accessibility_interface.on_text_selection_changed(acc_obj);
		if(win.is_visible(l_id)) {
			win.window_interface.invalidate_window();

			if(win.keyboard_target == this) {
				auto& node = win.get_node(l_id);
				auto location = win.get_current_location(l_id);
				ui_rectangle temp;
				temp.x_position = uint16_t(location.x);
				temp.y_position = uint16_t(location.y);
				temp.width = uint16_t(location.width);
				temp.height = uint16_t(location.height);

				auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

				auto cursor_line = text::line_of_position(analysis_obj, cursor_position);

				if(horizontal(win.orientation))
					win.window_interface.move_system_caret(int32_t(cached_cursor_postion + new_content_rect.x), int32_t(new_content_rect.y + win.layout_size * (cursor_line - line_offset)));
				else if(win.orientation == layout_orientation::vertical_left_to_right)
					win.window_interface.move_system_caret(int32_t(new_content_rect.x + win.layout_size * (cursor_line - line_offset)), int32_t(cached_cursor_postion + new_content_rect.y));
				else
					win.window_interface.move_system_caret(int32_t(new_content_rect.x  + new_content_rect.width - win.layout_size * (cursor_line - line_offset + 1)), int32_t(cached_cursor_postion + new_content_rect.y));
			}

			make_line_visible(win, text::line_of_position(analysis_obj, cursor_position));

			if(ts_obj)
				win.text_services_interface.on_selection_change(ts_obj);
		}
	}

	bool simple_editable_text::prepare_text(window_data const& win) {
		if(formatted_text && !text::appropriate_directionality(win, formatted_text)) {
			formatted_text = nullptr;
			selection_out_of_date = true;
		}

		if(!formatted_text) {
			// if the text ends in a space, a zero-width non joiner is placed at the end
			// this "fixes" direct-write's problem of ignoring spaces for alignment
			// UNRESOLVED: there may still be problems if the text is left-aligned and begins
			// with a space. A zero width could be prefixed as well, but we would have to adjust
			// for this in the hit-testing logic as well
			auto temp_text = text;
			if(temp_text.length() > 0 && text::is_space(temp_text.back()))
				temp_text += wchar_t(0x200C);

			formatted_text = win.text_interface.create_text_arragement(win, temp_text, text_alignment, text_size::standard, true, 100, nullptr).ptr;
			if(l_id != layout_reference_none) {
				auto& node = win.get_node(l_id);
				auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, node.width - (node.left_margin() + node.right_margin()), node.height, win.get_current_location(l_id));
				text::adjust_layout_region(formatted_text, new_content_rect.width, new_content_rect.height);
			}
			return true;
		}
		return false;
	}
	bool simple_editable_text::relayout_text(window_data const& win, screen_space_point sz) {
		if(!formatted_text || !text::appropriate_directionality(win, formatted_text)) {
			prepare_text(win);
			return true;
		} else {
			return text::adjust_layout_region(formatted_text, sz.x, sz.y);
		}
	}
	void simple_editable_text::draw_text(window_data& win, int32_t x, int32_t y) const {
		if(horizontal(win.orientation))
			win.rendering_interface.text(win, formatted_text, text_size::standard, x, y - line_offset * win.layout_size);
		else if(win.orientation == layout_orientation::vertical_left_to_right)
			win.rendering_interface.text(win, formatted_text, text_size::standard, x - line_offset * win.layout_size, y);
		else
			win.rendering_interface.text(win, formatted_text, text_size::standard, x + line_offset * win.layout_size, y);
	}

	ui_rectangle simple_editable_text::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;
		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = (!disabled) ? ui_rectangle::flag_interactable : 0ui8;
		retvalue.parent_object = this;
		return retvalue;
	}
	simple_layout_specification simple_editable_text::get_specification(window_data&) {
		simple_layout_specification spec;
		spec.minimum_page_size = layout_lines;
		spec.minimum_line_size = minimum_layout_space + 1;
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;
		return spec;
	}

	void simple_editable_text::populate_selection_rectangles(window_data& win, std::vector<screen_space_rect>& rects)  {
		if(formatted_text && anchor_position != cursor_position) {
			auto& node = win.get_node(l_id);
			auto location = win.get_current_location(l_id);
			ui_rectangle temp;
			temp.x_position = uint16_t(location.x);
			temp.y_position = uint16_t(location.y);
			temp.width = uint16_t(location.width);
			temp.height = uint16_t(location.height);

			auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

			auto window_rect = win.window_interface.get_window_location();

			for(auto& rng : cached_selection_region) {
				if(horizontal(win.orientation)) {
					rects.push_back(screen_space_rect{ rng.start + window_rect.x + new_content_rect .x, window_rect.y + new_content_rect.y + (rng.line - line_offset) * win.layout_size, rng.end - rng.start, win.layout_size});
				} else if(win.orientation == layout_orientation::vertical_left_to_right) {
					rects.push_back(screen_space_rect{  window_rect.x + new_content_rect.x + (rng.line - line_offset) * win.layout_size, rng.start + window_rect.y + new_content_rect.y, win.layout_size, rng.end - rng.start });
				} else {
					rects.push_back(screen_space_rect{ window_rect.x + new_content_rect.width + new_content_rect.x - (rng.line - line_offset + 1) * win.layout_size, rng.start + window_rect.y + new_content_rect.y, win.layout_size, rng.end - rng.start });
				}
			}
		}
	}


	void simple_editable_text::make_line_visible(window_data& win, int32_t line) {
		if(l_id == layout_reference_none)
			return;

		auto visible_lines = win.get_node(l_id).height;
		auto total_lines = text::number_of_lines(analysis_obj);
		if(total_lines <= visible_lines) {
			if(line_offset != 0) {
				line_offset = 0;
				win.flag_for_update_from_interface(this);
				win.window_interface.invalidate_window();
			}
			return;
		}
		if(line < line_offset) {
			line_offset = line;
			win.flag_for_update_from_interface(this);
			win.window_interface.invalidate_window();
			return;
		}

		if(line >= line_offset + visible_lines) {
			line_offset = line - visible_lines + 1;
			win.flag_for_update_from_interface(this);
			win.window_interface.invalidate_window();
			return;
		}
	}

	void simple_editable_text::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		if(!ts_obj)
			ts_obj = win.text_services_interface.create_text_service_object(win, *this);

		win.rendering_interface.background_rectangle(rect, rect.display_flags,
			rect.background_index, under_mouse && !disabled, win);

		auto& node = win.get_node(l_id);

		auto is_light = win.dynamic_settings.brushes[rect.foreground_index].is_light_color;
		auto resolved_brush = uint8_t(is_light ?
			(win.dynamic_settings.light_interactable_brush >= 0 ? win.dynamic_settings.light_interactable_brush : rect.foreground_index) :
			(win.dynamic_settings.dark_interactable_brush >= 0 ? win.dynamic_settings.dark_interactable_brush : rect.foreground_index));


		if(!disabled) {
			auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, ((node.height - 1) / 2), 1, (((node.height - 1) / 2) + 1), rect);
			win.rendering_interface.interactable_or_foreground(win, new_top_left, saved_state, win.keyboard_target != this ? rect.foreground_index : resolved_brush, false);
		}

		

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, rect);

		if(!disabled) {
			prepare_selection_regions(win);
			// render text
			{
				auto text_and_margin = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin()), node.height, rect);
				win.rendering_interface.fill_from_foreground(text_and_margin, rect.foreground_index, true);
			}

			auto num_visible_lines = node.height;

			// render selection, if any
			if((anchor_position != cursor_position || temp_text_length != 0) && formatted_text) {
				for(auto& rng : cached_selection_region) {
					if(rng.line >= line_offset && (rng.line - line_offset) < num_visible_lines) {
						if(horizontal(win.orientation)) {
							auto range_rect = screen_space_rect{
								rng.start + new_content_rect.x,
								new_content_rect.y + (rng.line - line_offset) * win.layout_size,
								rng.end - rng.start, 
								win.layout_size };
							win.rendering_interface.background_rectangle(range_rect, 0, rect.foreground_index, false, win);
							win.rendering_interface.fill_from_foreground(range_rect, rect.background_index, true);
						} else if(win.orientation == layout_orientation::vertical_left_to_right) {
							auto range_rect = screen_space_rect{
								new_content_rect.x + (rng.line - line_offset) * win.layout_size,
								rng.start + new_content_rect.y,
								win.layout_size,
								rng.end - rng.start };
							win.rendering_interface.background_rectangle(range_rect, 0, rect.foreground_index, false, win);
							win.rendering_interface.fill_from_foreground(range_rect, rect.background_index, true);
						} else {
							auto range_rect = screen_space_rect{
								new_content_rect.x + new_content_rect.width - (rng.line - line_offset + 1) * win.layout_size,
								rng.start + new_content_rect.y,
								win.layout_size,
								rng.end - rng.start };
							win.rendering_interface.background_rectangle(range_rect, 0, rect.foreground_index, false, win);
							win.rendering_interface.fill_from_foreground(range_rect, rect.background_index, true);
						}
					}
				}
			}

			if(line_offset > 0) {
				auto height = int32_t(2 * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0);
				auto fill_rect = screen_rectangle_from_relative_rect_in_ui(win,
					screen_space_rect{(node.width * win.layout_size) / 2, 0, win.layout_size,  height },
					rect);
				win.rendering_interface.fill_rectangle(fill_rect, rect.foreground_index);
			}
			if(text::number_of_lines(analysis_obj) > num_visible_lines + line_offset) {
				auto height = int32_t(2 * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0);
				auto fill_rect = screen_rectangle_from_relative_rect_in_ui(win,
					screen_space_rect{ (node.width * win.layout_size) / 2, win.layout_size * node.height -  height, win.layout_size,  height },
					rect);
				win.rendering_interface.fill_rectangle(fill_rect, rect.foreground_index);
			}

			// render cursor
			auto cursor_line = text::line_of_position(analysis_obj, cursor_position);

			if(cursor_visible && formatted_text && cursor_line >= line_offset && (cursor_line - line_offset) < num_visible_lines) {
				if(win.dynamic_settings.caret_blink)
					win.rendering_interface.register_in_place_animation();
	
				auto ms_in_cycle = win.rendering_interface.in_place_animation_running_ms() % win.caret_blink_ms;
				float in_cycle_length = float(ms_in_cycle) * 2.0f * 3.1415f / float(win.caret_blink_ms);
				float intensity = win.dynamic_settings.caret_blink ? (cos(in_cycle_length) + 1.0f) * 0.5f : 1.0f;


				win.rendering_interface.set_brush_opacity(resolved_brush, intensity);
				if(horizontal(win.orientation)) {
					win.rendering_interface.fill_rectangle(
						screen_space_rect{
							cached_cursor_postion + new_content_rect.x,
							new_content_rect.y + (cursor_line - line_offset) * win.layout_size,
							int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)),
							win.layout_size }, resolved_brush);
				} else if(win.orientation == layout_orientation::vertical_left_to_right) {
					win.rendering_interface.fill_rectangle(
						screen_space_rect{
							 new_content_rect.x + (cursor_line - line_offset) * win.layout_size,
							 cached_cursor_postion + new_content_rect.y,
							 win.layout_size,
							 int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f))
							 }, resolved_brush);
				} else {
					win.rendering_interface.fill_rectangle(
						screen_space_rect{
							 new_content_rect.x + new_content_rect.width - (cursor_line - line_offset + 1) * win.layout_size,
							 cached_cursor_postion + new_content_rect.y,
							 win.layout_size,
							 int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f))
						}, resolved_brush);
				}
				win.rendering_interface.set_brush_opacity(resolved_brush, 1.0f);
			}
		} else { // case: disabled
			win.rendering_interface.set_brush_opacity(rect.foreground_index, 0.6f);
			{
				auto text_and_margin = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin()), node.height, rect);
				win.rendering_interface.fill_from_foreground(text_and_margin, rect.foreground_index, true);
			}
			win.rendering_interface.set_brush_opacity(rect.foreground_index, 1.0f);
		}
	}
	void simple_editable_text::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto& node = win.get_node(l_id);

		if(relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - (node.left_margin() + node.right_margin()) * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - (node.left_margin() + node.right_margin()) * win.layout_size })) {

			update_analyzed_text(analysis_obj, formatted_text, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);
		}

		auto icon_location = screen_topleft_from_layout_in_ui(win, 0, ((node.height - 1) / 2), 1, (((node.height - 1) / 2) + 1), rect);
		win.rendering_interface.draw_icon_to_foreground(icon_location.x, icon_location.y, standard_icons::control_text);
		
		// get sub layout positions
		auto text_bounding_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, rect);
		draw_text(win, text_bounding_rect.x, text_bounding_rect.y);
	}

	mouse_test_result simple_editable_text::get_detailed_position(window_data& win, screen_space_point pt) {
		if(!win.is_visible(l_id))
			return mouse_test_result{ 0,0 };

		auto& node = win.get_node(l_id);

		auto location = win.get_current_location(l_id);
		ui_rectangle temp;
		temp.x_position = uint16_t(location.x);
		temp.y_position = uint16_t(location.y);
		temp.width = uint16_t(location.width);
		temp.height = uint16_t(location.height);

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

		auto adj_x = pt.x - new_content_rect.x;
		auto adj_y = pt.y - new_content_rect.y + (horizontal(win.orientation) ? line_offset * win.layout_size : 0);
		if(win.orientation == layout_orientation::vertical_left_to_right) {
			adj_x += line_offset * win.layout_size;
		} else if(win.orientation == layout_orientation::vertical_right_to_left) {
			adj_x -= line_offset * win.layout_size;
		}

		auto ht = text::hit_test_text(formatted_text, adj_x, adj_y);

		if(ht.is_inside == false) {
			if(ht.is_trailing) {
				return mouse_test_result{ std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())), 3 };
			} else {
				return mouse_test_result{ ht.metrics.textPosition, 0};
			}
		} else if(ht.metrics.length == 1) {
			auto percentage = horizontal(win.orientation) ? (adj_x - ht.metrics.left) / ht.metrics.width : (adj_y - ht.metrics.top) / ht.metrics.height;
			bool right_to_left_section = (ht.metrics.bidiLevel % 2) != 0;
			if(right_to_left_section)
				percentage = 1.0f - percentage;
			uint32_t quadrent = 0;
			if(percentage < 0.25f) {
				quadrent = 2;
			} else if(percentage < 0.50f) {
				quadrent = 3;
			} else if(percentage < 0.75f) {
				quadrent = 0;
			} else {
				quadrent = 1;
			}

			return mouse_test_result{ ht.metrics.textPosition + (ht.is_trailing != (right_to_left_section == true) ? 1 : 0), quadrent };
		} else {
			bool right_to_left_section = (ht.metrics.bidiLevel % 2) != 0;

			auto percentage = horizontal(win.orientation) ? (adj_x - ht.metrics.left) / ht.metrics.width : (adj_y - ht.metrics.top) / ht.metrics.height;

			int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, ht.metrics.textPosition, ht.metrics.length), 1);

			auto temp_text_pos = ht.metrics.textPosition;
			int32_t count = 0;
			while(temp_text_pos < ht.metrics.textPosition + ht.metrics.length) {
				auto next_pos = text::get_next_cursor_position(analysis_obj, temp_text_pos);

				if(percentage < float(count + 1) / float(num_positions_in_metrics)) {
					auto sub_percentage = (percentage * float(num_positions_in_metrics) - float(count));
					uint32_t quadrent = 0;
					if(sub_percentage < 0.25f) {
						quadrent = 2;
					} else if(sub_percentage < 0.50f) {
						quadrent = 3;
					} else if(sub_percentage < 0.75f) {
						quadrent = 0;
					} else {
						quadrent = 1;
					}
					if(right_to_left_section)
						quadrent = 3 - quadrent;
					return mouse_test_result{ quadrent < 2 ? temp_text_pos : next_pos, quadrent };
				}
					 
				temp_text_pos = next_pos;
				++count;
			}
			{
				auto next_pos = text::get_next_cursor_position(analysis_obj, temp_text_pos);
				return mouse_test_result{uint32_t(next_pos), uint32_t(right_to_left_section ? 1 : 2) };
			}
		}

	}

	uint32_t simple_editable_text::position_visually_above(window_data& win, uint32_t v) {
		if(prepare_text(win)) {
			update_analyzed_text(analysis_obj, formatted_text, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);
		}
		

		auto current_line = text::line_of_position(analysis_obj, int32_t(v));
		if(current_line == 0)
			return 0;

		bool is_at_text_end = v == text.length();
		auto curmetrics = text::get_metrics_at_position(formatted_text, is_at_text_end ? v - 1 : v);

		float coff_pos = 0.0f;
		float voff_pos = horizontal(win.orientation) ? (curmetrics.top + curmetrics.height / 2.0f) : (curmetrics.left + curmetrics.width / 2.0f);
		{
			bool left_to_right_section = (curmetrics.bidiLevel % 2) == 0;

			if(is_at_text_end) {
				if(horizontal(win.orientation)) {
					coff_pos = curmetrics.left + (left_to_right_section ? curmetrics.width : 0.0f);
				} else {
					coff_pos = curmetrics.top + (left_to_right_section ? curmetrics.height : 0.0f);
				}
			} else {
				int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
				if(num_positions_in_metrics == 1 || curmetrics.length == 0) {
					if(horizontal(win.orientation)) {
						coff_pos = curmetrics.left + (left_to_right_section ? 0.0f : curmetrics.width);
					} else {
						coff_pos = curmetrics.top + (left_to_right_section ? 0.0f : curmetrics.height);
					}
				} else {
					int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, v, (curmetrics.textPosition + curmetrics.length) - v);
					float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

					if(horizontal(win.orientation)) {
						coff_pos = curmetrics.left + (left_to_right_section ? percentage_through_region * curmetrics.width : (1.0f - percentage_through_region) * curmetrics.width);
					} else {
						coff_pos = curmetrics.top + (left_to_right_section ? percentage_through_region * curmetrics.height : (1.0f - percentage_through_region) * curmetrics.height);
					}
				}
			}
		}

		if(horizontal(win.orientation))
			voff_pos -= win.layout_size;
		else if(win.orientation == layout_orientation::vertical_left_to_right) 
			voff_pos -= win.layout_size;
		else
			voff_pos += win.layout_size;

		auto ht = horizontal(win.orientation) ? text::hit_test_text(formatted_text, int32_t(coff_pos), int32_t(voff_pos)) : text::hit_test_text(formatted_text, int32_t(voff_pos), int32_t(coff_pos));

		int32_t new_position = 0;
		if(ht.is_inside == false) {
			if(ht.is_trailing) {
				new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
			} else {
				new_position = int32_t(ht.metrics.textPosition);
			}
		} else if(ht.metrics.length == 1) {
			if(ht.is_trailing) {
				new_position = int32_t(std::min(ht.metrics.textPosition + 1, uint32_t(text.length())));
			} else {
				new_position = int32_t(ht.metrics.textPosition);
			}
		} else {
			int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, ht.metrics.textPosition, ht.metrics.length), 1);

			if(num_positions_in_metrics == 1) {
				if(ht.is_trailing) {
					new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
				} else {
					new_position = int32_t(ht.metrics.textPosition);
				}
			} else {
				bool left_to_right_section = (ht.metrics.bidiLevel % 2) == 0;
				float percentage_in_region = horizontal(win.orientation) ? ((coff_pos - ht.metrics.left) / ht.metrics.width) : ((coff_pos - ht.metrics.top) / ht.metrics.height);
				if(!left_to_right_section)
					percentage_in_region = 1.0f - percentage_in_region;
				float section_size = 1.0f / float(num_positions_in_metrics);
				float running_total = section_size / 2.0f;

				new_position = int32_t(ht.metrics.textPosition);
				[&]() {
					if(percentage_in_region <= running_total) {
						return;
					}
					new_position = text::get_next_cursor_position(analysis_obj, new_position);
					for(int32_t i = 0; i < num_positions_in_metrics; ++i) {
						running_total += section_size;
						if(percentage_in_region <= running_total) {
							return;
						}
						new_position = text::get_next_cursor_position(analysis_obj, new_position);
					}
					new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
				}();
			}
		}
		return uint32_t(new_position);
	}
	uint32_t simple_editable_text::position_visually_below(window_data& win, uint32_t v) {
		if(prepare_text(win)) {
			update_analyzed_text(analysis_obj, formatted_text, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);
		}

		auto current_line = text::line_of_position(analysis_obj, int32_t(v));
		if(current_line >= text::number_of_lines(analysis_obj) - 1)
			return uint32_t(text.length());

		bool is_at_text_end = v == text.length();
		auto curmetrics = text::get_metrics_at_position(formatted_text, is_at_text_end ? v - 1 : v);

		float coff_pos = 0.0f;
		float voff_pos = horizontal(win.orientation) ? (curmetrics.top + curmetrics.height / 2.0f) : (curmetrics.left + curmetrics.width / 2.0f);

		{
			bool left_to_right_section = (curmetrics.bidiLevel % 2) == 0;

			if(is_at_text_end) {
				if(horizontal(win.orientation)) {
					coff_pos = curmetrics.left + (left_to_right_section ? curmetrics.width : 0.0f);
				} else {
					coff_pos = curmetrics.top + (left_to_right_section ? curmetrics.height : 0.0f);
				}
			} else {
				int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, curmetrics.textPosition, curmetrics.length), 1);
				if(num_positions_in_metrics == 1 || curmetrics.length == 0) {
					if(horizontal(win.orientation)) {
						coff_pos = curmetrics.left + (left_to_right_section ? 0.0f : curmetrics.width);
					} else {
						coff_pos = curmetrics.top + (left_to_right_section ? 0.0f : curmetrics.height);
					}
				} else {
					int32_t num_positions_after_cursor = text::number_of_cursor_positions_in_range(analysis_obj, v, (curmetrics.textPosition + curmetrics.length) - v);
					float percentage_through_region = float(num_positions_in_metrics - num_positions_after_cursor) / float(num_positions_in_metrics);

					if(horizontal(win.orientation)) {
						coff_pos = curmetrics.left + (left_to_right_section ? percentage_through_region * curmetrics.width : (1.0f - percentage_through_region) * curmetrics.width);
					} else {
						coff_pos = curmetrics.top + (left_to_right_section ? percentage_through_region * curmetrics.height : (1.0f - percentage_through_region) * curmetrics.height);
					}
				}
			}
		}
		if(horizontal(win.orientation))
			voff_pos += win.layout_size;
		else if(win.orientation == layout_orientation::vertical_left_to_right)
			voff_pos += win.layout_size;
		else
			voff_pos -= win.layout_size;

		auto ht = horizontal(win.orientation) ? text::hit_test_text(formatted_text, int32_t(coff_pos), int32_t(voff_pos)) : text::hit_test_text(formatted_text, int32_t(voff_pos), int32_t(coff_pos));

		int32_t new_position = 0;
		if(ht.is_inside == false) {
			if(ht.is_trailing) {
				new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
			} else {
				new_position = int32_t(ht.metrics.textPosition);
			}
		} else if(ht.metrics.length == 1) {
			if(ht.is_trailing) {
				new_position = int32_t(std::min(ht.metrics.textPosition + 1, uint32_t(text.length())));
			} else {
				new_position = int32_t(ht.metrics.textPosition);
			}
		} else {
			int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, ht.metrics.textPosition, ht.metrics.length), 1);

			if(num_positions_in_metrics == 1) {
				if(ht.is_trailing) {
					new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
				} else {
					new_position = int32_t(ht.metrics.textPosition);
				}
			} else {
				bool left_to_right_section = (ht.metrics.bidiLevel % 2) == 0;
				float percentage_in_region = horizontal(win.orientation) ? ((coff_pos - ht.metrics.left) / ht.metrics.width) : ((coff_pos - ht.metrics.top) / ht.metrics.height);
				if(!left_to_right_section)
					percentage_in_region = 1.0f - percentage_in_region;
				float section_size = 1.0f / float(num_positions_in_metrics);
				float running_total = section_size / 2.0f;

				new_position = int32_t(ht.metrics.textPosition);
				[&]() {
					if(percentage_in_region <= running_total) {
						return;
					}
					new_position = text::get_next_cursor_position(analysis_obj, new_position);
					for(int32_t i = 0; i < num_positions_in_metrics; ++i) {
						running_total += section_size;
						if(percentage_in_region <= running_total) {
							return;
						}
						new_position = text::get_next_cursor_position(analysis_obj, new_position);
					}
					new_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
				}();
			}
		}
		return uint32_t(new_position);
	}

	uint32_t simple_editable_text::get_position_from_screen_point(window_data& win, screen_space_point pt) {
		auto window_rect = win.window_interface.get_window_location();
		pt.x -= window_rect.x;
		pt.y -= window_rect.y;

		if(formatted_text) {
			auto& node = win.get_node(l_id);
			auto location = win.get_current_location(l_id);
			ui_rectangle temp;
			temp.x_position = uint16_t(location.x);
			temp.y_position = uint16_t(location.y);
			temp.width = uint16_t(location.width);
			temp.height = uint16_t(location.height);

			pt.x -= location.x;
			pt.y -= location.y;

			auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

			auto adjusted_x = pt.x - (new_content_rect.x - temp.x_position);
			auto adjusted_y = pt.y - (new_content_rect.y - temp.y_position) + (horizontal(win.orientation) ? line_offset * win.layout_size : 0);
			if(win.orientation == layout_orientation::vertical_left_to_right) {
				adjusted_x += line_offset * win.layout_size;
			} else if(win.orientation == layout_orientation::vertical_right_to_left) {
				adjusted_x -= line_offset * win.layout_size;
			}

			auto ht = text::hit_test_text(formatted_text, adjusted_x, adjusted_y);
			if(ht.is_inside == false) {
				if(ht.is_trailing) {
					return std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length()));
				} else {
					return ht.metrics.textPosition;
				}
			} else if(ht.metrics.length == 1) {
				if(ht.is_trailing) {
					return std::min(ht.metrics.textPosition + 1, uint32_t(text.length()));
				} else {
					return ht.metrics.textPosition;
				}
			} else {
				int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, ht.metrics.textPosition, ht.metrics.length), 1);

				if(num_positions_in_metrics == 1) {
					if(ht.is_trailing) {
						return std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length()));
					} else {
						return ht.metrics.textPosition;
					}
				} else {
					bool left_to_right_section = (ht.metrics.bidiLevel % 2) == 0;
					float percentage_in_region = horizontal(win.orientation) ? ((adjusted_x - ht.metrics.left) / ht.metrics.width) : ((adjusted_y - ht.metrics.top) / ht.metrics.height);
					if(!left_to_right_section)
						percentage_in_region = 1.0f - percentage_in_region;
					float section_size = 1.0f / float(num_positions_in_metrics);
					float running_total = section_size / 2.0f;

					auto temp_text_pos = ht.metrics.textPosition;
					[&]() {
						if(percentage_in_region <= running_total) {
							return;
						}
						temp_text_pos = text::get_next_cursor_position(analysis_obj, temp_text_pos);
						for(int32_t i = 0; i < num_positions_in_metrics; ++i) {
							running_total += section_size;
							if(percentage_in_region <= running_total) {
								return;
							}
							temp_text_pos = text::get_next_cursor_position(analysis_obj, temp_text_pos);
						}
						temp_text_pos = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
					}();
					return temp_text_pos;
				}
			}
		}
		return 0;
	}
	void simple_editable_text::internal_move_cursor_to_point(window_data& win, int32_t x, int32_t y, bool extend_selection) {
		if(!disabled) {
			if(formatted_text) {
				if(x == 0 && y == 0) { // because this is what kb activation gives us
					anchor_position = 0;
					cursor_position = int32_t(text.length());
				} else {
					auto& node = win.get_node(l_id);
					auto location = win.get_current_location(l_id);
					ui_rectangle temp;
					temp.x_position = uint16_t(location.x);
					temp.y_position = uint16_t(location.y);
					temp.width = uint16_t(location.width);
					temp.height = uint16_t(location.height);

					auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

					auto adjusted_x = x - (new_content_rect.x - temp.x_position);
					auto adjusted_y = y - (new_content_rect.y - temp.y_position) + (horizontal(win.orientation) ? line_offset * win.layout_size : 0);
					if(win.orientation == layout_orientation::vertical_left_to_right) {
						adjusted_x += line_offset * win.layout_size;
					} else if(win.orientation == layout_orientation::vertical_right_to_left) {
						adjusted_x -= line_offset * win.layout_size;
					}

					auto ht = text::hit_test_text(formatted_text, adjusted_x, adjusted_y);

					if(ht.is_inside == false) {
						if(ht.is_trailing) {
							cursor_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
						} else {
							cursor_position = int32_t(ht.metrics.textPosition);
						}
					} else if(ht.metrics.length == 1) {
						if(ht.is_trailing) {
							cursor_position = int32_t(std::min(ht.metrics.textPosition + 1, uint32_t(text.length())));
						} else {
							cursor_position = int32_t(ht.metrics.textPosition);
						}
					} else {
						int32_t num_positions_in_metrics = std::max(text::number_of_cursor_positions_in_range(analysis_obj, ht.metrics.textPosition, ht.metrics.length), 1);

						if(num_positions_in_metrics == 1) {
							if(ht.is_trailing) {
								cursor_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
							} else {
								cursor_position = int32_t(ht.metrics.textPosition);
							}
						} else {
							bool left_to_right_section = (ht.metrics.bidiLevel % 2) == 0;
							float percentage_in_region = horizontal(win.orientation) ? ((adjusted_x - ht.metrics.left) / ht.metrics.width) : ((adjusted_y - ht.metrics.top) / ht.metrics.height);
							if(!left_to_right_section)
								percentage_in_region = 1.0f - percentage_in_region;
							float section_size = 1.0f / float(num_positions_in_metrics);
							float running_total = section_size / 2.0f;

							cursor_position = int32_t(ht.metrics.textPosition);
							[&]() {
								if(percentage_in_region <= running_total) {
									return;
								}
								cursor_position = text::get_next_cursor_position(analysis_obj, cursor_position);
								for(int32_t i = 0; i < num_positions_in_metrics; ++i) {
									running_total += section_size;
									if(percentage_in_region <= running_total) {
										return;
									}
									cursor_position = text::get_next_cursor_position(analysis_obj, cursor_position);
								}
								cursor_position = int32_t(std::min(ht.metrics.textPosition + ht.metrics.length, uint32_t(text.length())));
							}();
						}
					}
					if(!extend_selection) {
						anchor_position = cursor_position;
					}
				}
			}


			internal_on_selection_changed(win);
			win.window_interface.invalidate_window();
		}
	}
	void simple_editable_text::on_click(window_data& win, uint32_t x, uint32_t y) {
		internal_move_cursor_to_point(win, int32_t(x), int32_t(y), win.window_interface.is_shift_held_down());
		if(!win.window_interface.is_shift_held_down())
			mouse_entry_position = cursor_position;
		win.set_keyboard_focus(this);
	}
	void simple_editable_text::on_right_click(window_data& win, uint32_t, uint32_t) {
		auto& node = win.get_node(l_id);

		if(alt_text != uint16_t(-1)) {
			win.info_popup.open(win, info_window::parameters{ l_id }.right(text_alignment == content_alignment::trailing).text(alt_text).width(8).internal_margins(node.left_margin(), node.right_margin()));
		}
	}
	accessibility_object* simple_editable_text::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_simple_text_accessibility_interface(win, this, name, alt_text);
		}
		return acc_obj;
	}

	void simple_editable_text::on_finalize(window_data& win) {
		if(!disabled && cursor_visible)
			win.window_interface.destroy_system_caret();

		cursor_visible = false;
		if(changes_made) {
			win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
			changes_made = false;
		}
		clear_temporary_contents(win);
		on_edit_finished(win, text);
		win.window_interface.invalidate_window();
	}
	void simple_editable_text::on_initialize(window_data& win) {
		if(disabled) {
			win.set_keyboard_focus(nullptr);
		} else {
			cursor_visible = true;
			changes_made = false;

			if(acc_obj && win.is_visible(l_id)) {
				win.accessibility_interface.on_focus_change(acc_obj);
			}

			if(horizontal(win.orientation)) {
				win.window_interface.create_system_caret(int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)), win.layout_size);
			} else {
				win.window_interface.create_system_caret(win.layout_size, int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)));
			}

			win.window_interface.invalidate_window();
			if(ts_obj)
				win.text_services_interface.set_focus(win, ts_obj);
		}
	}

	//edit_interface
	// commands sent to control
	void simple_editable_text::insert_codepoint(window_data& win, uint32_t codepoint) {
		if(disabled)
			return;

		auto old_end = std::max(anchor_position, cursor_position);
		auto old_start = std::min(anchor_position, cursor_position);
		if(anchor_position != cursor_position) {
			command(win, edit_command::delete_selection, false);
		}
		if(!changes_made)
			win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
		auto insert_position = std::min(cursor_position, int32_t(text.length()));
		if(codepoint < 0x10000) {
			text.insert(insert_position, 1, uint16_t(codepoint));
			++cursor_position;
		} else {
			auto p = text::make_surrogate_pair(codepoint);
			text.insert(insert_position, 1, uint16_t(p.high));
			text.insert(insert_position + 1, 1, uint16_t(p.low));
			cursor_position += 2;
		}
		anchor_position = cursor_position;
		internal_on_text_changed(win);
		if(ts_obj)
			win.text_services_interface.on_text_change(ts_obj, uint32_t(old_start), uint32_t(old_end), uint32_t(cursor_position));
		win.window_interface.invalidate_window();
	}
	void simple_editable_text::clear(window_data& win) {
		if(disabled)
			return;

		if(text.length() > 0) {
			auto old_length = text.length();
			text.clear();
			cursor_position = 0;
			anchor_position = 0;
			internal_on_text_changed(win);
			if(ts_obj)
				win.text_services_interface.on_text_change(ts_obj, 0, uint32_t(old_length), 0);
			win.window_interface.invalidate_window();
		}
	}
	void simple_editable_text::move_cursor(window_data& win, uint32_t position, bool extend_selection) {
		if(disabled)
			return;

		if(cursor_position != int32_t(position) || (anchor_position != int32_t(position) && extend_selection == false)) {
			cursor_position = int32_t(position);
			if(!extend_selection) {
				anchor_position = int32_t(position);
			}
			internal_on_selection_changed(win);
			win.window_interface.invalidate_window();
		}
	}
	void simple_editable_text::set_selection(window_data& win, uint32_t start, uint32_t end) {
		if(disabled)
			return;

		if(cursor_position != int32_t(end) || anchor_position != int32_t(start)) {
			anchor_position = int32_t(start);
			cursor_position = int32_t(end);
			internal_on_selection_changed(win);
			win.window_interface.invalidate_window();
		}
	}
	bool simple_editable_text::move_cursor_by_screen_pt(window_data& win, screen_space_point pt, bool extend_selection) {
		if(disabled)
			return false;
		if(l_id == layout_reference_none)
			return false;

		auto screen_rect = win.get_current_location(l_id);
		if(pt.y < screen_rect.y || pt.x < screen_rect.x || pt.y > screen_rect.y + screen_rect.height || pt.x > screen_rect.x + screen_rect.width) {
			return false;
		}

		internal_move_cursor_to_point(win, pt.x - screen_rect.x, pt.y - screen_rect.y, extend_selection);
		if(extend_selection == false)
			mouse_entry_position = cursor_position;

		if(win.selecting_edit_text == edit_selection_mode::word) {
			if(!text::is_word_position(analysis_obj, cursor_position)) {
				if(cursor_position >= mouse_entry_position) {
					cursor_position = text::get_next_word_position(analysis_obj, cursor_position);
					anchor_position = text::get_previous_word_position(analysis_obj, mouse_entry_position);
				} else {
					cursor_position = text::get_previous_word_position(analysis_obj, cursor_position);
					anchor_position = text::get_next_word_position(analysis_obj, mouse_entry_position);
				}
				internal_on_selection_changed(win);
			}
		} else if(win.selecting_edit_text == edit_selection_mode::line) {
			if(cursor_position >= mouse_entry_position) {
				cursor_position = text::end_of_line(analysis_obj, text::line_of_position(analysis_obj, cursor_position));
				anchor_position = text::start_of_line(analysis_obj, text::line_of_position(analysis_obj, mouse_entry_position));
			} else {
				cursor_position = text::start_of_line(analysis_obj, text::line_of_position(analysis_obj, cursor_position));
				anchor_position = text::end_of_line(analysis_obj, text::line_of_position(analysis_obj, mouse_entry_position));
			}
			internal_on_selection_changed(win);
		}
		return true;
	}
	void simple_editable_text::command(window_data& win, edit_command cmd, bool extend_selection) {
		switch(cmd) {
			case edit_command::new_line:
				if(layout_lines == 1) {
					win.set_keyboard_focus(nullptr);
				} else {
					insert_codepoint(win, L'\n');
				}
				return;
			case edit_command::backspace:
				if(anchor_position != cursor_position) {
					command(win, edit_command::delete_selection, false);
				} else {
					if(!changes_made)
						win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
					auto previous_position = text::get_previous_cursor_position(analysis_obj, cursor_position);
					if(previous_position != cursor_position) {
						text.erase(size_t(previous_position), size_t(cursor_position - previous_position));
						auto old_cursor = cursor_position;
						cursor_position = previous_position;
						anchor_position = previous_position;
						if(ts_obj)
							win.text_services_interface.on_text_change(ts_obj, uint32_t(previous_position), uint32_t(old_cursor), uint32_t(previous_position));
					}
				}
				internal_on_text_changed(win);
				return;
			case edit_command::delete_char:
				if(anchor_position != cursor_position) {
					command(win, edit_command::delete_selection, false);
				} else {
					if(!changes_made)
						win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
					auto next_position = text::get_next_cursor_position(analysis_obj, cursor_position);
					if(next_position != cursor_position) {
						text.erase(size_t(cursor_position), size_t(next_position - cursor_position));
						if(ts_obj)
							win.text_services_interface.on_text_change(ts_obj, uint32_t(cursor_position), uint32_t(next_position), uint32_t(cursor_position));
					}
				}

				internal_on_text_changed(win);
				return;
			case edit_command::backspace_word:
				if(anchor_position != cursor_position) {
					command(win, edit_command::delete_selection, false);
				} else {
					win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
					auto previous_position = text::get_previous_word_position(analysis_obj, cursor_position);
					if(previous_position != cursor_position) {
						text.erase(size_t(previous_position), size_t(cursor_position - previous_position));
						auto old_cursor = cursor_position;
						cursor_position = previous_position;
						anchor_position = previous_position;
						if(ts_obj)
							win.text_services_interface.on_text_change(ts_obj, uint32_t(previous_position), uint32_t(old_cursor), uint32_t(previous_position));
					}
				}

				internal_on_text_changed(win);
				return;
				return;
			case edit_command::delete_word:
				if(anchor_position != cursor_position) {
					command(win, edit_command::delete_selection, false);
				} else {
					win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
					auto next_position = text::get_next_word_position(analysis_obj, cursor_position);
					if(next_position != cursor_position) {
						text.erase(size_t(cursor_position), size_t(next_position - cursor_position));
						if(ts_obj)
							win.text_services_interface.on_text_change(ts_obj, uint32_t(cursor_position), uint32_t(next_position), uint32_t(cursor_position));
					}
				}
				internal_on_text_changed(win);
				return;
			case edit_command::tab:
				//insert_codepoint(win, 0x2003);
				return;
			case edit_command::cursor_down:
				cursor_position = position_visually_below(win, cursor_position);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cursor_up:
				cursor_position = position_visually_above(win, cursor_position);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cursor_left:
				cursor_position = text::left_visual_cursor_position(analysis_obj, cursor_position, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cursor_right:
				cursor_position = text::right_visual_cursor_position(analysis_obj, cursor_position, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cursor_left_word:
				cursor_position = text::left_visual_word_position(analysis_obj, cursor_position);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cursor_right_word:
				cursor_position = text::right_visual_word_position(analysis_obj, cursor_position);
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::to_line_start:
				cursor_position = text::start_of_line(analysis_obj, text::line_of_position(analysis_obj, cursor_position));
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::to_line_end:
			{
				auto cline = text::line_of_position(analysis_obj, cursor_position);
				cursor_position = text::end_of_line(analysis_obj, cline);
				while(text::line_of_position(analysis_obj, cursor_position - 1) >= cline && cursor_position >= 1 && text::is_space(text[cursor_position - 1])) {
					--cursor_position;
				}
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			}
			case edit_command::to_text_start:
				cursor_position = 0;
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::to_text_end:
				cursor_position = int32_t(text.length());
				if(!extend_selection)
					anchor_position = cursor_position;
				internal_on_selection_changed(win);
				return;
			case edit_command::cut:
				if(anchor_position != cursor_position) {
					command(win, edit_command::copy, false);
					command(win, edit_command::delete_selection, false);
				}
				return;
			case edit_command::copy:
				if(anchor_position != cursor_position) {
					std::wstring_view v(text);
					auto start = std::min(anchor_position, cursor_position);
					auto length = std::max(anchor_position, cursor_position) - start;
					win.window_interface.text_to_clipboard(v.substr(start, length));
				}
				return;
			case edit_command::paste:
			{
				auto old_start_position = std::min(anchor_position, cursor_position);
				auto old_end_position = std::max(anchor_position, cursor_position);
				win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
				if(anchor_position != cursor_position) {
					std::wstring_view v(text);
					auto start = std::min(anchor_position, cursor_position);
					auto length = std::max(anchor_position, cursor_position) - start;
					text.erase(size_t(start), size_t(length));
					cursor_position = start;
					anchor_position = start;
				}
				auto cb = win.window_interface.text_from_clipboard();
				text.insert(size_t(cursor_position), cb);
				cursor_position += int32_t(cb.length());
				anchor_position = cursor_position;

				internal_on_text_changed(win);

				if(ts_obj)
					win.text_services_interface.on_text_change(ts_obj, uint32_t(old_start_position), uint32_t(old_end_position), uint32_t(cursor_position));
				changes_made = false;
			}
				return;
			case edit_command::select_all:
				anchor_position = 0;
				cursor_position = int32_t(text.length());
				internal_on_selection_changed(win);
				return;
			case edit_command::undo:
			{
				auto undostate = win.edit_undo_buffer.undo(undo_item{ this, text, anchor_position, cursor_position });
				if(undostate.has_value()) {
					auto old_length = text.length();
					text = (*undostate).contents;
					cursor_position = (*undostate).cursor;
					anchor_position = (*undostate).anchor;
					
					internal_on_text_changed(win);

					if(ts_obj)
						win.text_services_interface.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(text.length()));
				}
			}
				return;
			case edit_command::redo:
			{
				auto redostate = win.edit_undo_buffer.redo(undo_item{ this, text, anchor_position, cursor_position });
				if(redostate.has_value()) {
					auto old_length = text.length();
					text = (*redostate).contents;
					cursor_position = (*redostate).cursor;
					anchor_position = (*redostate).anchor;
					
					internal_on_text_changed(win);

					if(ts_obj)
						win.text_services_interface.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(text.length()));
				}
			}
				return;
			case edit_command::select_current_word:
				anchor_position = text::get_previous_word_position(analysis_obj, cursor_position);
				cursor_position = text::get_next_word_position(analysis_obj, cursor_position);
				internal_on_selection_changed(win);
				return;
			case edit_command::select_current_section:
				anchor_position = text::start_of_line(analysis_obj, text::line_of_position(analysis_obj, cursor_position));
				cursor_position = text::end_of_line(analysis_obj, text::line_of_position(analysis_obj, cursor_position));
				internal_on_selection_changed(win);
				return;
			case edit_command::delete_selection:
				if(anchor_position != cursor_position) {
					win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });

					auto start = std::min(anchor_position, cursor_position);
					auto length = std::max(anchor_position, cursor_position) - start;
					text.erase(size_t(start), size_t(length));
					cursor_position = start;
					anchor_position = start;
					
					internal_on_text_changed(win);

					if(ts_obj)
						win.text_services_interface.on_text_change(ts_obj, uint32_t(start), uint32_t(start + length), uint32_t(start));
				}
				return;
		}
	}
	void simple_editable_text::register_composition_result(window_data& win) {
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_composition_result(acc_obj, std::wstring_view(text.data() + temp_text_position, size_t(temp_text_length)));
		}
		temp_text_position = 0;
		temp_text_length = 0;
		selection_out_of_date = true;
		if(win.is_visible(l_id)) {
			win.window_interface.invalidate_window();
		}
	}
	void simple_editable_text::register_conversion_target_change(window_data& win) {
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_conversion_target_changed(acc_obj);
		}
	}
	void simple_editable_text::clear_temporary_contents(window_data& win) {
		text.erase(size_t(temp_text_position), size_t(temp_text_length));
		cursor_position = temp_text_position;
		anchor_position = temp_text_position;
		auto old_length = temp_text_length;
		temp_text_length = 0;
		
		internal_on_text_changed(win);

		if(ts_obj)
			win.text_services_interface.on_text_change(ts_obj, uint32_t(temp_text_position), uint32_t(temp_text_position + old_length), uint32_t(temp_text_position));
		
	}
	void simple_editable_text::set_temporary_selection(window_data& win, uint32_t start, uint32_t end) {
		temp_text_position = int32_t(start);
		temp_text_length = int32_t(end - start);
		selection_out_of_date = true;
		if(win.is_visible(l_id)) {
			if(acc_obj) {
				win.accessibility_interface.on_composition_change(acc_obj, std::wstring_view(text.data(), size_t(end - start)));
			}
			win.window_interface.invalidate_window();
		}
	}

	void simple_editable_text::set_cursor_visibility(window_data& win, bool is_visible) {
		if(cursor_visible != is_visible) {
			cursor_visible = is_visible;
			if(!disabled) {
				if(cursor_visible) {
					if(horizontal(win.orientation)) {
						win.window_interface.create_system_caret(int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)), win.layout_size);
					} else {
						win.window_interface.create_system_caret(win.layout_size, int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier  * win.dpi / 96.0f)));
					}
				} else {
					win.window_interface.destroy_system_caret();
				}
				win.window_interface.invalidate_window();
			}
		}
	}

	// retrieve information from control
	bool simple_editable_text::position_is_ltr(uint32_t position) {
		if(position < text.length()) {
			return text::position_is_ltr(analysis_obj, position);
		} else {
			return true;
		}
	}
	uint32_t simple_editable_text::get_cursor() const {
		return uint32_t(cursor_position);
	}
	uint32_t simple_editable_text::get_selection_anchor() const {
		return uint32_t(anchor_position);
	}
	uint32_t simple_editable_text::get_text_length() const {
		return uint32_t(text.length());
	}
	uint32_t simple_editable_text::get_temporary_position() const {
		return uint32_t(temp_text_position);
	}
	uint32_t simple_editable_text::get_temporary_length() const {
		return uint32_t(temp_text_length);
	}
	std::wstring simple_editable_text::get_text() const {
		return text;
	}
	screen_space_rect simple_editable_text::get_edit_bounds(window_data& win) const {
		if(l_id == layout_reference_none)
			return screen_space_rect{ 0,0,0,0 };

		auto& node = win.get_node(l_id);
		auto location = win.get_current_location(l_id);
		ui_rectangle temp;
		temp.x_position = uint16_t(location.x);
		temp.y_position = uint16_t(location.y);
		temp.width = uint16_t(location.width);
		temp.height = uint16_t(location.height);

		return screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);
	}
	bool simple_editable_text::consume_mouse_event(window_data& win, int32_t x, int32_t y, uint32_t buttons) {
		auto text_bounds = get_edit_bounds(win);
		if(x < text_bounds.x || x > text_bounds.x + text_bounds.width || y < text_bounds.y || y > text_bounds.y + text_bounds.height)
			return false;
		if(ts_obj)
			return win.text_services_interface.send_mouse_event_to_tso(ts_obj, x, y, buttons);
		return false;
	}
	screen_space_rect simple_editable_text::get_character_bounds(window_data& win, uint32_t position) const {
		if(l_id == layout_reference_none || !formatted_text)
			return screen_space_rect{ 0,0,0,0 };

		auto& node = win.get_node(l_id);
		auto location = win.get_current_location(l_id);
		ui_rectangle temp;
		temp.x_position = uint16_t(location.x);
		temp.y_position = uint16_t(location.y);
		temp.width = uint16_t(location.width);
		temp.height = uint16_t(location.height);

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), node.height, temp);

		auto m = text::get_metrics_at_position(formatted_text, position);

		if(horizontal(win.orientation)) {
			return screen_space_rect{ int32_t(new_content_rect.x + m.left), int32_t(new_content_rect.y + m.top - line_offset * win.layout_size), int32_t(m.width), int32_t(m.height) };
		} else if(win.orientation == layout_orientation::vertical_left_to_right) {
			return screen_space_rect{ int32_t(new_content_rect.x + m.left - line_offset * win.layout_size), int32_t(new_content_rect.y + m.top), int32_t(m.width), int32_t(m.height) };
		} else {
			return screen_space_rect{ int32_t(new_content_rect.x + m.left + line_offset * win.layout_size), int32_t(new_content_rect.y + m.top), int32_t(m.width), int32_t(m.height) };
		}
	}

	void simple_editable_text::set_disabled(window_data& win, bool v) {
		if(disabled != v) {
			disabled = v;
			if(acc_obj && win.is_visible(l_id))
				win.accessibility_interface.on_enable_disable(acc_obj, v);
			win.window_interface.invalidate_window();

			if(disabled && cursor_visible) {
				win.window_interface.destroy_system_caret();
			}
			if(!disabled && cursor_visible) {
				if(horizontal(win.orientation)) {
					win.window_interface.create_system_caret(int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)), win.layout_size);
				} else {
					win.window_interface.create_system_caret(win.layout_size, int32_t(std::ceil(1.0f * win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f)));
				}
				win.window_interface.invalidate_window();
			}
		}
	}

	void simple_editable_text::set_alt_text(window_data& win, uint16_t alt) {
		alt_text = alt;
		if(acc_obj && win.is_visible(l_id)) {
			if(alt != uint16_t(-1)) {
				win.accessibility_interface.on_change_help_text(acc_obj, win.text_data.instantiate_text(alt).text_content.text);
			} else {
				win.accessibility_interface.on_change_help_text(acc_obj, L"");
			}
		}
	}
	void simple_editable_text::insert_text(window_data& win, uint32_t position_start, uint32_t position_end, std::wstring_view content) {
		if(win.keyboard_target == this) {
			if(!changes_made)
				win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
		}

		if(position_start != position_end)
			text.erase(size_t(position_start), size_t(position_end - position_start));
		text.insert(size_t(position_start), content);

		if(int32_t(position_end) < anchor_position) {
			anchor_position += int32_t(content.length()) - int32_t(position_end - position_start);
			anchor_position = std::max(0, anchor_position);
		} else if(int32_t(position_start) <= anchor_position && anchor_position <= int32_t(position_end)) {
			anchor_position = int32_t(position_start + content.length());
		}
		if(int32_t(position_end) < cursor_position) {
			cursor_position += int32_t(content.length()) - int32_t(position_end - position_start);
			cursor_position = std::max(0, cursor_position);
		} else if(int32_t(position_start) <= cursor_position && cursor_position <= int32_t(position_end)) {
			cursor_position = int32_t(position_start + content.length());
		}

		internal_on_text_changed(win);

		if(ts_obj)
			win.text_services_interface.on_text_change(ts_obj, position_start, position_end, uint32_t(content.length()));
		win.window_interface.invalidate_window();
	}
	void simple_editable_text::set_text(window_data& win, std::wstring const& t) {
		if(win.keyboard_target == this) {
			if(!changes_made)
				win.edit_undo_buffer.push_state(undo_item{ this, text, anchor_position, cursor_position });
		}
		auto old_length = text.length();
		text = t;
		cursor_position = int32_t(t.length());
		anchor_position = int32_t(t.length());
		
		
		internal_on_text_changed(win);

		if(ts_obj)
			win.text_services_interface.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(text.length()));
		win.window_interface.invalidate_window();
	}
	void simple_editable_text::quiet_set_text(window_data& win, std::wstring const& t) {
		auto old_length = text.length();
		text = t;
		cursor_position = int32_t(t.length());
		anchor_position = int32_t(t.length());

		internal_on_selection_changed(win);
		if(ts_obj)
			win.text_services_interface.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(text.length()));

		formatted_text = nullptr;

		prepare_text(win);
		update_analyzed_text(analysis_obj, formatted_text, text, win.orientation != layout_orientation::horizontal_right_to_left, win.text_data);

		selection_out_of_date = true;
		changes_made = true;
		win.flag_for_update_from_interface(this);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_text_content_changed(acc_obj);
			if(edit_type != edit_contents::number)
				win.accessibility_interface.on_text_value_changed(acc_obj);
			else
				win.accessibility_interface.on_text_numeric_value_changed(acc_obj);
		}

		if(win.is_visible(l_id))
			win.window_interface.invalidate_window();
	}
	uint16_t simple_editable_text::get_alt_text() const {
		return alt_text;
	}
	uint16_t simple_editable_text::get_name() const {
		return name;
	}

	//
	// editable_numeric_range
	//

	editable_numeric_range::editable_numeric_range(content_alignment text_alignment, uint16_t name, uint16_t alt_text, uint8_t minimum_layout_space, uint8_t lines, float minimum, float maximum, int8_t precision) : simple_editable_text(text_alignment, name, alt_text, minimum_layout_space, lines), minimum(minimum), maximum(maximum), precision(precision) {
		edit_type = edit_contents::number;
	}

	accessibility_object* editable_numeric_range::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_numeric_range_accessibility_interface(win, *this);
		}
		return acc_obj;
	}
	void editable_numeric_range::set_value(window_data& win, float v) {
		auto result_to_str = win.text_data.format_double(double(v), precision);
		set_text(win, result_to_str.text_content.text);
	}
	float editable_numeric_range::get_value(window_data& win) const {
		auto current_text = get_text();
		return float(win.text_data.text_to_double(current_text.data(), uint32_t(current_text.length())));
	}
	void editable_numeric_range::command(window_data& win, edit_command cmd, bool extend_selection) {
		switch(cmd) {
			case edit_command::cursor_up:
			{
				auto value = get_value(win);
				value += float(1.0 / pow(10.0f, precision));
				value = std::clamp(value, minimum, maximum);
				set_value(win, value);
				break;
			}
			case edit_command::cursor_down:
			{
				auto value = get_value(win);
				value -= float(1.0 / pow(10.0f, precision));
				value = std::clamp(value, minimum, maximum);
				set_value(win, value);
				break;
			}
			case edit_command::to_text_start:
			{
				auto value = get_value(win);
				value += 1.0f;
				value = std::clamp(value, minimum, maximum);
				set_value(win, value);
				break;
			}
			case edit_command::to_text_end:
			{
				auto value = get_value(win);
				value -= 1.0f;
				value = std::clamp(value, minimum, maximum);
				set_value(win, value);
				break;
			}
			default:
				simple_editable_text::command(win, cmd, extend_selection);
				break;
		}
	}
	//
	// LABEL
	//

	ui_rectangle label_control::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.parent_object = this;

		return retvalue;
	}
	simple_layout_specification label_control::get_specification(window_data& win) {
		simple_layout_specification spec;

		label_text.prepare_text(win);

		spec.minimum_page_size = uint16_t(label_text.resolved_text_size.y);
		spec.minimum_line_size = uint16_t(label_text.resolved_text_size.x);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;

		return spec;
	}
	void label_control::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto& node = win.get_node(l_id);
		auto text_bounding_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), label_text.resolved_text_size.y, rect);
		label_text.relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - (node.left_margin() + node.right_margin()) * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - (node.left_margin() + node.right_margin()) * win.layout_size });
		label_text.draw_text(win, text_bounding_rect.x, text_bounding_rect.y);
	}
	void label_control::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		auto& node = win.get_node(l_id);

		win.rendering_interface.background_rectangle(rect, rect.display_flags, rect.background_index, under_mouse, win);
		
		auto text_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin()), label_text.resolved_text_size.y, rect);

		win.rendering_interface.fill_from_foreground(text_rect, rect.foreground_index, true);
	}
	void label_control::on_right_click(window_data& win, uint32_t, uint32_t) {
		auto& node = win.get_node(l_id);

		if(alt_text != uint16_t(-1)) {
			win.info_popup.open(win, info_window::parameters{ l_id }.right(label_text.text_alignment == content_alignment::trailing).text(alt_text).width(8).internal_margins(node.left_margin(), node.right_margin()));
		}
	}
	accessibility_object* label_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_plain_text_accessibility_interface(win, this, &label_text, true);
		}
		return acc_obj;
	}

	void label_control::set_text(window_data& win) {
		label_text.set_text();
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_change_name(acc_obj, L"");
		}
	}
	void label_control::set_text(window_data& win, uint16_t val) {
		label_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			if(val != uint16_t(-1)) {
				win.accessibility_interface.on_change_name(acc_obj, label_text.get_raw_text(win));
			} else {
				win.accessibility_interface.on_change_name(acc_obj, L"");
			}
		}
	}
	void label_control::set_text(window_data& win, std::wstring const& val) {
		label_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_change_name(acc_obj, val);
		}
	}

	//
	// BASIC BUTTON CONTROL
	//

	button_control_base::~button_control_base() {
	}
	layout_node_type button_control_base::get_node_type() {
		return layout_node_type::control;
	}
	ui_rectangle button_control_base::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = (!disabled) ? ui_rectangle::flag_interactable : 0ui8;
		retvalue.parent_object = this;

		return retvalue;
	}
	void button_control_base::recreate_contents(window_data&, layout_node&) {
	}

	simple_layout_specification button_control_base::get_specification(window_data& win) {
		simple_layout_specification spec;

		button_text.prepare_text(win);

		spec.minimum_page_size = uint16_t(button_text.resolved_text_size.y);
		spec.minimum_line_size = uint16_t(button_text.resolved_text_size.x);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;

		return spec;
	}

	void button_control_base::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		// get sub layout positions

		auto fg_index = rect.foreground_index;
		auto bg_index = rect.background_index;

		if(selected)
			std::swap(fg_index, bg_index);

		win.rendering_interface.background_rectangle(rect,
			selected ? (rect.display_flags & ~ui_rectangle::flag_skip_bg) : rect.display_flags,
			bg_index, under_mouse && !disabled, win);

		if(!disabled && !selected) {
			auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, ((button_text.resolved_text_size.y - 1) / 2), 1, (((button_text.resolved_text_size.y - 1) / 2) + 1), rect);
			win.rendering_interface.interactable_or_foreground(win, new_top_left, saved_state, fg_index, false);
		}

		auto& node = win.get_node(l_id);

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin()), button_text.resolved_text_size.y, rect);

		if(!disabled) {
			win.rendering_interface.fill_from_foreground(new_content_rect, fg_index, true);
		} else {
			win.rendering_interface.set_brush_opacity(fg_index, 0.6f);
			win.rendering_interface.fill_from_foreground(new_content_rect, fg_index, true);
			win.rendering_interface.set_brush_opacity(fg_index, 1.0f);
		}
	}
	void button_control_base::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto& node = win.get_node(l_id);

		button_text.relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - (node.left_margin() + node.right_margin()) * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - (node.left_margin() + node.right_margin()) * win.layout_size });

		auto icon_location = screen_topleft_from_layout_in_ui(win, 0, ((button_text.resolved_text_size.y - 1) / 2), 1, (((button_text.resolved_text_size.y - 1) / 2) + 1), rect);

		win.rendering_interface.draw_icon_to_foreground(icon_location.x, icon_location.y, icon);

		// get sub layout positions

		auto text_bounding_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), button_text.resolved_text_size.y, rect);

		button_text.draw_text(win, text_bounding_rect.x, text_bounding_rect.y);
	}

	void button_control_base::on_right_click(window_data& win, uint32_t, uint32_t) {
		if(alt_text != uint16_t(-1)) {
			auto& node = win.get_node(l_id);
			win.info_popup.open(win, info_window::parameters{ l_id }.right(button_text.text_alignment == content_alignment::trailing).text(alt_text).width(8).internal_margins(node.left_margin(), node.right_margin()));
		}
	}

	void button_control_base::on_click(window_data& win, uint32_t, uint32_t) {
		if(!disabled) {
			if(category != button_category::selection_button || selected == false) {
				if(category == button_category::selection_button) {
					selected = true;
				}
				button_action(win);
				if(acc_obj) {
					if(category == button_category::action_button) {
						win.accessibility_interface.on_invoke(acc_obj);
					} else if(category == button_category::selection_button) {
						win.accessibility_interface.on_select_unselect(acc_obj, true);
					} else if(category == button_category::toggle_button) {
						win.accessibility_interface.on_toggle_change(acc_obj);
					}
				}
			}
		}
	}
	accessibility_object* button_control_base::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			if(category == button_category::action_button) {
				acc_obj = win.accessibility_interface.make_action_button_accessibility_interface(win, *this);
			} else if(category == button_category::selection_button) {
				acc_obj = win.accessibility_interface.make_selection_button_accessibility_interface(win, *this);
			} else {
				std::abort();
			}
		}
		return acc_obj;
	}
	void button_control_base::set_disabled(window_data& win, bool v) {
		if(disabled != v) {
			disabled = v;
			if(acc_obj && win.is_visible(l_id)) {
				win.accessibility_interface.on_enable_disable(acc_obj, v);
			}
		}
	}
	void button_control_base::set_selected(window_data& win, bool v) {
		if(selected != v) {
			selected = v;
			if(acc_obj && win.is_visible(l_id)) {
				win.accessibility_interface.on_select_unselect(acc_obj, v);
				auto container = win.get_parent_accessibility_object(l_id);
				if(container)
					win.accessibility_interface.on_selection_change(container);
			}
		}
	}
	void button_control_base::set_alt_text(window_data& win, uint16_t alt) {
		if(alt_text != alt) {
			alt_text = alt;
			if(acc_obj && win.is_visible(l_id)) {
				if(alt != uint16_t(-1)) {
					win.accessibility_interface.on_change_help_text(acc_obj, win.text_data.instantiate_text(alt).text_content.text);
				} else {
					win.accessibility_interface.on_change_help_text(acc_obj, L"");
				}
			}
		}
	}
	uint16_t button_control_base::get_alt_text() const {
		return alt_text;
	}
	void button_control_base::set_text(window_data& win, uint16_t val, text::text_parameter* b, text::text_parameter* e) {
		button_text.set_text(val, b, e);
		if(acc_obj && win.is_visible(l_id)) {
			if(val != uint16_t(-1)) {
				win.accessibility_interface.on_change_name(acc_obj, win.text_data.instantiate_text(val, b, e).text_content.text);
			} else {
				win.accessibility_interface.on_change_name(acc_obj, L"");
			}
		}
	}
	void button_control_base::set_text(window_data& win, std::wstring const& val) {
		button_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_change_name(acc_obj, val);
			
		}
	}
	void button_control_base::set_text(window_data& win) {
		button_text.set_text();
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_change_name(acc_obj, L"");
		}
	}
	void button_control_base::set_text(window_data& win, stored_text const& t) {
		button_text.set_text(t);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_change_name(acc_obj, t.get_raw_text(win));
		}
	}
	std::wstring button_control_base::get_raw_text(window_data const& win) const {
		return button_text.get_raw_text(win);
	}
	void button_control_base::set_text_alignment(content_alignment align) {
		button_text.text_alignment = align;
		button_text.invalidate();
	}

	//
	// TOGGLE SPECIALIZATION
	//

	void button_control_toggle::button_action(window_data& win) {
		change_toggle_state(win, !toggle_is_on);
	}
	accessibility_object* button_control_toggle::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_toggle_button_accessibility_interface(win, *this);
		}
		return acc_obj;
	}

	void button_control_toggle::change_toggle_state(window_data& win, bool toggle_state) {
		if(toggle_state != toggle_is_on) {
			toggle_is_on = toggle_state;
			set_text(win, toggle_is_on ? toggle_on_text : toggle_off_text);
			toggle_action(win, toggle_state);
			win.flag_for_update_from_interface(this);
			if(acc_obj && win.is_visible(l_id)) {
				win.accessibility_interface.on_toggle_change(acc_obj);
			}
		}
	}

	//
	// ICON BUTTON
	//

	layout_node_type icon_button_base::get_node_type() {
		return layout_node_type::control;
	}
	ui_rectangle icon_button_base::prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.display_flags = (!is_disabled(win)) ? ui_rectangle::flag_interactable : 0ui8;
		retvalue.parent_object = this;

		return retvalue;
	}

	simple_layout_specification icon_button_base::get_specification(window_data& win) {
		simple_layout_specification spec;

		auto icon_sz = win.rendering_interface.get_icon_size(ico);
		spec.minimum_page_size = uint16_t(display_vertically ? icon_sz.y + 1 : icon_sz.y);
		spec.minimum_line_size = uint16_t(display_vertically ? icon_sz.x : icon_sz.x + 1);
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;

		return spec;
	}
	void icon_button_base::render_foreground(ui_rectangle const&, window_data&) {

	}
	void icon_button_base::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {

		auto fg_index = rect.foreground_index;
		auto bg_index = rect.background_index;

		if(is_toggled())
			std::swap(fg_index, bg_index);

		win.rendering_interface.background_rectangle(rect,
			is_toggled() ? (rect.display_flags & ~ui_rectangle::flag_skip_bg) : rect.display_flags,
			bg_index, under_mouse && !is_disabled(win), win);

		auto icon_sz = win.rendering_interface.get_icon_size(ico);

		if(!is_disabled(win) && saved_state.holds_key()) {
			auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 0,
				display_vertically ? icon_sz.x : 1,
				display_vertically ? 1 : icon_sz.y,
				rect);
			auto center_amount = win.layout_size * ((display_vertically ? icon_sz.x : icon_sz.y) - 1) / 2;
			if(horizontal(win.orientation)) {
				win.rendering_interface.interactable(win, screen_space_point{ new_top_left.x + center_amount, new_top_left.y }, saved_state, fg_index, display_vertically);
			} else {
				win.rendering_interface.interactable(win, screen_space_point{ new_top_left.x, new_top_left.y + center_amount }, saved_state, fg_index, display_vertically);
			}
		}

		{
			auto new_top_left = screen_topleft_from_layout_in_ui(win, display_vertically ? 0 : 1, display_vertically ? 1 : 0, icon_sz.x, icon_sz.y, rect);
			if(is_disabled(win)) {
				win.rendering_interface.set_brush_opacity(fg_index, 0.6f);
				win.rendering_interface.draw_icon(new_top_left.x, new_top_left.y, ico, fg_index);
				win.rendering_interface.set_brush_opacity(fg_index, 1.0f);
			} else {
				win.rendering_interface.draw_icon(new_top_left.x, new_top_left.y, ico, fg_index);
			}
		}
	}

	void icon_button_base::on_right_click(window_data& win, uint32_t, uint32_t) {
		if(alt_text != uint16_t(-1)) {
			win.info_popup.open(win, info_window::parameters{ l_id }.right().text(alt_text).width(8).internal_margins(display_vertically ? 0 : 1, 0));
		}
	}

	void icon_button_base::on_click(window_data& win, uint32_t, uint32_t) {
		if(!is_disabled(win)) {
			button_action(win);
			if(acc_obj && category == button_category::action_button) {
				win.accessibility_interface.on_invoke(acc_obj);
			}
		}
	}
	accessibility_object* icon_button_base::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			if(category == button_category::action_button) {
				acc_obj = win.accessibility_interface.make_icon_button_accessibility_interface(win, *this);
			} else if(category == button_category::toggle_button) {
				acc_obj = win.accessibility_interface.make_icon_toggle_button_accessibility_interface(win, *this);
			} else {
				std::abort();
			}
		}
		return acc_obj;
	}
	void icon_button_base::set_alt_text(window_data& win, uint16_t alt) {
		if(alt_text != alt) {
			alt_text = alt;
			if(acc_obj && win.is_visible(l_id)) {
				if(alt != uint16_t(-1)) {
					win.accessibility_interface.on_change_help_text(acc_obj, win.text_data.instantiate_text(alt).text_content.text);
				} else {
					win.accessibility_interface.on_change_help_text(acc_obj, L"");
				}
			}
		}
	}
	uint16_t icon_button_base::get_alt_text() const {
		return alt_text;
	}
	void icon_button_base::set_name_text(window_data& win, uint16_t t) {
		if(name_text != t) {
			name_text = t;
			if(acc_obj && win.is_visible(l_id)) {
				if(t != uint16_t(-1)) {
					win.accessibility_interface.on_change_name(acc_obj, win.text_data.instantiate_text(t).text_content.text);
				} else {
					win.accessibility_interface.on_change_name(acc_obj, L"");
				}
			}
		}
	}
	uint16_t icon_button_base::get_name_text() const {
		return name_text;
	}
	std::wstring icon_button_base::get_name(window_data const& win) {
		if(name_text != uint16_t(-1))
			return win.text_data.instantiate_text(name_text).text_content.text;
		else
			return std::wstring(L"");
	}
	//
	// END ICON BUTTON
	//


	void page_footer_button::render_foreground(ui_rectangle const& rect, window_data& win) {

		update_page(win);
		text::text_parameter page_numbers[2] = { text::int_param{ stored_page + 1 }, text::int_param{ stored_page_total } };
		button_control_base::set_text(win, text_id::page_fraction, page_numbers, page_numbers + 2);

		if(stored_page_total > 1) {
			button_control_base::render_foreground(rect, win);
		}
	}
	void page_footer_button::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {
		if(stored_page_total > 1) {
			button_control_base::render_composite(rect, win, under_mouse);
		} else {
			win.rendering_interface.background_rectangle(rect, rect.display_flags, rect.background_index, false, win);
		}
	}
	void page_footer_button::update_page(window_data const& win) {
		if(l_id != layout_reference_none) {
			auto page_container = win.get_containing_page(l_id);
			page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

			auto current_page = parent_pginfo ? uint32_t(parent_pginfo->subpage_offset) : 0;
			auto max_page = parent_pginfo ? uint32_t(parent_pginfo->subpage_divisions.size() + 1) : 0;

			if(current_page != stored_page || max_page != stored_page_total) {
				button_control_base::invalidate_text();
				stored_page = current_page;
				stored_page_total = max_page;
			}
		} else {
			stored_page = 0;
			stored_page_total = 0;
		}
	}

	int32_t page_footer_button::interactable_count(window_data const& w) {
		update_page(w);
		if(stored_page_total > 1)
			return 1;
		else
			return 0;
	}

	void page_footer_button::button_action(window_data& win) {
		if(stored_page_total > 1 && l_id != layout_reference_none) {
			auto parent = win.get_node(l_id).parent;
			if(parent != layout_reference_none) {
				page_footer* cf = static_cast<page_footer*>(win.get_node(parent).l_interface);
				if(cf->is_open()) {
					cf->close(win, true);
				} else {
					cf->open(win, true);
				}
			}
		}
	}

	bool page_back_button::is_disabled(window_data const& win) const {
		if(l_id == layout_reference_none)
			return true;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		return parent_pginfo ? parent_pginfo->subpage_offset == 0 : true;
	}

	void page_back_button::button_action(window_data& win) {
		if(l_id == layout_reference_none)
			return;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;
		auto disabled = parent_pginfo ? parent_pginfo->subpage_offset == 0 : true;

		if(disabled)
			return;

		if(page_footer::page_turn_down.type != animation_type::none)
			win.rendering_interface.prepare_layered_ui_animation(win);

		if(auto li = win.get_node(page_container).l_interface; li)
			li->go_to_page(win, parent_pginfo->subpage_offset - 1, *parent_pginfo);
		else
			parent_pginfo->subpage_offset -= 1ui16;

		win.redraw_ui();

		page_footer::start_page_turn_animation(win, page_container, false);
	}


	std::wstring page_jump_back_button::get_name(window_data const& win) {
		text::text_parameter tp[1] = { text::int_param(jump_size(win), 0) };
		return win.text_data.instantiate_text(text_id::page_prev_prev_name, tp, tp + 1).text_content.text;
	}

	int32_t page_jump_back_button::jump_size(window_data const& win) const {
		if(l_id == layout_reference_none)
			return 0;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		if(!parent_pginfo)
			return 0;

		return std::min(parent_pginfo->subpage_offset, uint16_t(std::ceil(std::sqrt(float(parent_pginfo->subpage_divisions.size() + 1)))));
	}

	bool page_jump_back_button::is_disabled(window_data const& win) const {
		if(l_id == layout_reference_none)
			return true;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		return parent_pginfo ? parent_pginfo->subpage_offset == 0 : true;
	}

	void page_jump_back_button::button_action(window_data& win) {
		if(l_id == layout_reference_none)
			return;
		if(is_disabled(win))
			return;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		if(page_footer::page_turn_down.type != animation_type::none)
			win.rendering_interface.prepare_layered_ui_animation(win);

		auto int_amount = uint16_t(jump_size(win));
		if(auto li = win.get_node(page_container).l_interface; li)
			li->go_to_page(win, parent_pginfo->subpage_offset - int_amount, *parent_pginfo);
		else
			parent_pginfo->subpage_offset -= int_amount;

		win.redraw_ui();

		page_footer::start_page_turn_animation(win, page_container, false);
	}

	bool page_forward_button::is_disabled(window_data const& win) const {
		if(l_id == layout_reference_none)
			return true;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		return parent_pginfo ? parent_pginfo->subpage_offset >= parent_pginfo->subpage_divisions.size() : true;
	}

	void page_forward_button::button_action(window_data& win) {
		if(l_id == layout_reference_none)
			return;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;
		auto disabled = parent_pginfo ? parent_pginfo->subpage_offset >= parent_pginfo->subpage_divisions.size() : true;

		if(disabled)
			return;

		if(page_footer::page_turn_up.type != animation_type::none)
			win.rendering_interface.prepare_layered_ui_animation(win);

		if(auto li = win.get_node(page_container).l_interface; li)
			li->go_to_page(win, parent_pginfo->subpage_offset + 1, *parent_pginfo);
		else
			parent_pginfo->subpage_offset += 1ui16;

		win.redraw_ui();

		page_footer::start_page_turn_animation(win, page_container, true);
	}

	std::wstring page_jump_forward_button::get_name(window_data const& win) {
		text::text_parameter tp[1] = { text::int_param(jump_size(win), 0) };
		return win.text_data.instantiate_text(text_id::page_next_next_name, tp, tp+1).text_content.text;
	}

	bool page_jump_forward_button::is_disabled(window_data const& win) const {
		if(l_id == layout_reference_none)
			return true;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		return parent_pginfo ? parent_pginfo->subpage_offset >= parent_pginfo->subpage_divisions.size() : true;
	}

	int32_t page_jump_forward_button::jump_size(window_data const& win) const {
		if(l_id == layout_reference_none)
			return 0;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		if(!parent_pginfo)
			return 0;

		return int32_t(std::ceil(std::sqrt(float(parent_pginfo->subpage_divisions.size() + 1))));
	}

	void page_jump_forward_button::button_action(window_data& win) {
		if(l_id == layout_reference_none)
			return;
		if(is_disabled(win))
			return;

		if(page_footer::page_turn_up.type != animation_type::none)
			win.rendering_interface.prepare_layered_ui_animation(win);

		auto int_amount = uint16_t(jump_size(win));
		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		if(auto li = win.get_node(page_container).l_interface; li)
			li->go_to_page(win, parent_pginfo->subpage_offset + int_amount, *parent_pginfo);
		else
			parent_pginfo->subpage_offset += int_amount;

		parent_pginfo->subpage_offset = std::min(parent_pginfo->subpage_offset, uint16_t(parent_pginfo->subpage_divisions.size()));

		win.redraw_ui();

		page_footer::start_page_turn_animation(win, page_container, true);
	}


	void page_header_button::button_action(window_data& win) {
		if(l_id == layout_reference_none)
			return;

		auto& this_node = win.get_node(l_id);
		if(this_node.parent == layout_reference_none)
			return;

		auto& header_node = win.get_node(this_node.parent);
		auto page_node_id = header_node.parent;
		if(page_node_id == layout_reference_none)
			return;

		close_action(win, page_node_id);
		win.update_window_focus();
	}

	layout_interface* open_list_control::selected_item() const {
		layout_interface* sel = nullptr;
		for(auto p : options) {
			if(p->is_selected()) {
				sel = p;
				break;
			}
		}
		return sel;
	}

	ui_rectangle open_list_control::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.parent_object = l_id;

		return retvalue;
	}
	simple_layout_specification open_list_control::get_specification(window_data& win) {
		simple_layout_specification spec;

		uint16_t max_line_required = 2;
		uint16_t page_required = 0;
		for(auto b : options) {
			auto sub_spec = b->get_specification(win);
			max_line_required = std::max(max_line_required, uint16_t(sub_spec.minimum_line_size + left_margin + right_margin));
			page_required += sub_spec.minimum_page_size;
		}

		spec.minimum_page_size = page_required;
		spec.minimum_line_size = max_line_required;
		spec.page_flags = size_flags::match_content;
		spec.line_flags = size_flags::none;

		return spec;
	}
	void open_list_control::recreate_contents(window_data& win, layout_node& n) {
		int32_t running_height = 0;
		auto const node_height = n.height;
		auto const node_width = n.width;

		for(auto opt : options) {
			auto child_ref = win.create_node(opt, node_width, node_height, false, true);
			auto& child_node = win.get_node(child_ref);

			child_node.x = 0;
			child_node.y = int16_t(running_height);
			child_node.set_margins(left_margin, right_margin);
			child_node.width = node_width;

			running_height += node_height;
			win.immediate_resize(child_node, node_width, child_node.height);

			win.immediate_add_child(l_id, child_ref);
		}

		auto& self = win.get_node(l_id);
		self.height = std::max(self.height, uint16_t(running_height));
	}
	accessibility_object* open_list_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_open_list_control_accessibility_interface(win, *this);
		}
		return acc_obj;
	}
	void open_list_control::center_contents(window_data& win) {
		if(l_id == layout_reference_none)
			return;

		int32_t min_extent = std::numeric_limits<int32_t>::max();
		int32_t max_extent = 0;

		for(auto opt : options) {
			if(opt->l_id != layout_reference_none) {
				min_extent = std::min(int32_t(win.get_node(opt->l_id).y), min_extent);
				max_extent = std::max(int32_t(win.get_node(opt->l_id).y + win.get_node(opt->l_id).height), max_extent);
			}
		}

		auto self_height = win.get_node(l_id).height;
		auto desired_start = std::max((self_height - (max_extent - min_extent)) / 2, 0);

		if(desired_start != min_extent) {
			for(auto opt : options) {
				win.get_node(opt->l_id).y = uint16_t(win.get_node(opt->l_id).y + desired_start - min_extent);
			}
		}
	}

	void list_open::button_action(window_data& win) {
		if(l_id != layout_reference_none) {
			auto parent = win.get_node(l_id).parent;
			if(parent != layout_reference_none) {
				auto& list_node = win.get_node(parent);
				list_control* container = static_cast<list_control*>(list_node.l_interface);
				if(container) {
					container->open(win, true);
				}
			}
		}
	}
	void list_option::button_action(window_data& win) {
		if(l_id != layout_reference_none) {
			auto parent = win.get_node(l_id).parent;
			if(parent != layout_reference_none) {
				auto& list_node = win.get_node(parent);
				list_control* container = static_cast<list_control*>(list_node.l_interface);
				if(container) {
					container->select_option_by_value(win, data);
				}
			}
		}
	}

	accessibility_object* list_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_expandable_selection_list(win, this, this, name, alt_text_id);
		}
		return acc_obj;
	}
	layout_interface* list_control::selected_item() const {
		layout_interface* sel = nullptr;
		for(auto& p : this->options) {
			if(p->is_selected()) {
				sel = p.get();
				break;
			}
		}
		return sel;
	}
	ui_rectangle list_control::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.parent_object = l_id;

		if(list_is_open) {
			retvalue.left_border = 1;
			retvalue.right_border = 1;
			retvalue.top_border = 1;
			retvalue.bottom_border = 1;
		}


		return retvalue;
	}
	simple_layout_specification list_control::get_specification(window_data& win) {
		simple_layout_specification spec;

		populate_list(win);

		uint16_t max_line_required = 2;
		uint16_t max_page_required = 1;
		for(auto& b : options) {
			auto sub_spec = b->get_specification(win);
			max_line_required = std::max(max_line_required, sub_spec.minimum_line_size);
			max_page_required = std::max(max_page_required, sub_spec.minimum_page_size);
		}

		spec.minimum_page_size = max_page_required;
		spec.minimum_line_size = max_line_required;
		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::none;

		return spec;
	}
	void list_control::recreate_contents(window_data& win, layout_node& n) {
		populate_list(win);
		auto const node_height = n.height;
		auto const node_width = n.width;
		auto const lmarg = n.left_margin();
		auto const rmarg = n.right_margin();

		if(list_is_open) {
			int32_t running_height = 0;
			for(auto& opt : options) {
				auto child_ref = win.create_node(opt.get(),
					node_width, node_height, false, true);
				auto& child_node = win.get_node(child_ref);

				child_node.x = 0;
				child_node.y = int16_t(running_height);
				child_node.set_margins(lmarg, rmarg);

				running_height += node_height;
				win.immediate_resize(child_node, node_width, node_height);

				win.immediate_add_child(l_id,child_ref);
			}

		} else {
			auto child_ref = win.create_node(&open_button,
				node_width, node_height, false, true);
			auto& child_node = win.get_node(child_ref);
			open_button.set_alt_text(win, alt_text_id);

			child_node.x = 0;
			child_node.y = 0;
			child_node.set_margins(lmarg, rmarg);

			win.immediate_resize(child_node, node_width, node_height);
			
			win.immediate_add_child(l_id, child_ref);
		}
	}
	void list_control::on_lose_focus(window_data& win) {
		close(win, false);
	}
	layout_rect list_control::get_content_rectangle(window_data& win) {
		if(list_is_open) {
			auto nheight = l_id != layout_reference_none ? win.get_node(l_id).height : 1ui16;
			return layout_rect{ 0i16, int16_t(-(nheight * currently_selected)), 0i16, int16_t(nheight * (options.size() - 1)) };
		} else {
			return layout_rect{0i16,0i16,0i16,0i16};
		}
	}

	void list_control::open(window_data& win, bool move_focus) {
		if(l_id != layout_reference_none) {
			if(list_control::list_appearance.type != animation_type::none)
				win.rendering_interface.prepare_ui_animation(win);

			list_is_open = true;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}
			
			if(list_control::list_appearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					list_control::list_appearance.type,
					win.get_default_animation_direction(l_id),
					list_control::list_appearance.duration_seconds,
					list_control::list_appearance.animate_in }, win);
			}

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, true);
		}
	}

	void list_control::close(window_data& win, bool move_focus) {
		if(list_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(list_control::list_disappearance.type != animation_type::none) {
				win.rendering_interface.prepare_ui_animation(win);
				loc = win.get_current_location(l_id);
			}

			list_is_open = false;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(list_control::list_disappearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					loc,
					list_control::list_disappearance.type,
					win.get_default_animation_direction(l_id),
					list_control::list_disappearance.duration_seconds,
					list_control::list_disappearance.animate_in }, win);
			} else {
				win.rendering_interface.stop_ui_animations(win);
			}

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, false);
		}
	}
	void list_control::quiet_select_option_by_value(window_data& win, size_t d) {
		if(!options.empty()) {
			int32_t count = 0;
			for(auto& b : options) {
				if(b->data == d) {
					options[currently_selected]->set_selected(win, false);

					currently_selected = count;

					open_button.set_text(win, options[currently_selected]->get_stored_text());
					options[currently_selected]->set_selected(win, true);
					return;
				}
				++count;
			}
		}


	}
	void list_control::select_option_by_value(window_data& win, size_t d) {
		close(win, true);
		quiet_select_option_by_value(win, d);
		on_select(win, d);
	}
	button_control_base* list_control::get_option(size_t d) {
		for(auto& b : options) {
			if(b->data == d) {
				return b.get();
			}
		}
		return nullptr;
	}
	void list_control::populate_list(window_data& win) {
		if(options.empty()) {
			for(uint32_t i = 0; ; ++i) {
				auto option = describe_option(win, i);

				if(option.text_id == uint16_t(-1))
					break;

				options.emplace_back(std::make_unique<list_option>());
				options.back()->set_text(win, option.text_id);
				options.back()->set_alt_text(win, option.alt_text_id);
				options.back()->data = option.data;
				options.back()->set_text_alignment(text_alignment);
			}

			currently_selected = std::min(currently_selected, int32_t(options.size()) - 1);
			open_button.set_text(win, options[currently_selected]->get_stored_text());
			options[currently_selected]->set_selected(win, true);
			open_button.set_text_alignment(text_alignment);
			open_button.set_alt_text(win, alt_text_id);
			open_button.icon = standard_icons::control_list;

			on_create(win);
		}
	}

	animation_defintion list_control::list_appearance;
	animation_defintion list_control::list_disappearance;

	void menu_open::button_action(window_data& win) {
		if(l_id != layout_reference_none) {
			auto parent = win.get_node(l_id).parent;
			if(parent != layout_reference_none) {
				auto& list_node = win.get_node(parent);
				menu_control* container = static_cast<menu_control*>(list_node.l_interface);
				if(container) {
					container->open(win, true);
				}
			}
		}
	}

	ui_rectangle menu_control::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.parent_object = l_id;

		if(list_is_open) {
			retvalue.left_border = 1;
			retvalue.right_border = 1;
			retvalue.top_border = 1;
			retvalue.bottom_border = 1;
		}

		return retvalue;
	}

	simple_layout_specification menu_control::get_specification(window_data&) {
		simple_layout_specification spec;
		spec.minimum_page_size = uint16_t(page_size);
		spec.minimum_line_size = uint16_t(line_size);
		return spec;
	}

	void menu_control::recreate_contents(window_data& win, layout_node& n) {
		auto const lmarg = n.left_margin();
		auto const rmarg = n.right_margin();

		if(list_is_open) {
			auto old_height = n.height;
			auto old_width = n.width;
			auto parent_page = win.get_containing_page(l_id);
			auto parent_height = parent_page != layout_reference_none ? win.get_node(parent_page).height : old_height;
			n.height = parent_height;

			auto const menu_contents = get_options(win);

			page_layout_specification page_spec;
			page_spec.header = &header_space;
			page_spec.footer = &menu_footer;
			page_spec.begin = menu_contents.data();
			page_spec.end = menu_contents.data() + menu_contents.size();
			page_spec.column_left_margin = uint8_t(lmarg);
			page_spec.column_right_margin = uint8_t(rmarg);
			page_spec.ex_inter_column_margin = 0;
			page_spec.ex_page_top_margin = 0;
			page_spec.ex_page_bottom_margin = 1;
			page_spec.ex_page_left_margin = 0;
			page_spec.ex_page_right_margin = 0;
			page_spec.horz_shrink_page_to_content = false;
			page_spec.min_column_horizontal_size = uint8_t(old_width);
			page_spec.max_column_horizontal_size = uint8_t(old_width);
			page_spec.uniform_column_width = true;
			page_spec.max_columns = 1;
			page_spec.vert_shrink_page_to_content = false;
			page_spec.vertical_column_alignment = content_alignment::leading;
			page_spec.horizontal_columns_alignment = content_alignment::centered;
			page_spec.additional_space_to_outer_margins = true;

			
			default_recreate_page(win, this, page_spec);

			auto& nn = win.get_node(l_id);

			if(!vertically_cover_parent) {

				auto pi = nn.page_info();
				int32_t cheight = 2;
				if(pi) {
					for(auto c : pi->view_columns()) {
						cheight = std::max(cheight, win.get_node(c).height + 3);
					}

					for(auto c : pi->view_columns()) {
						win.get_node(c).y = uint16_t(((cheight - 3) - win.get_node(c).height) / 2 + 1);
					}
				}

				win.get_node(menu_footer.l_id).y = uint16_t(cheight - 1);
			}

			nn.width = old_width;
			nn.height = old_height;
		} else {
			open_button.icon = standard_icons::control_list;
			open_button.set_alt_text(win, alt_text);

			auto pi = n.page_info();
			pi->header = layout_reference_none;
			pi->footer = layout_reference_none;
			pi->subpage_offset = 0;
			pi->subpage_divisions.clear();

			auto n_width = n.width;
			auto n_height = n.height;
			auto open_button_id = win.create_node(&open_button, n_width, n_height, false);
			auto& open_button_node = win.get_node(open_button_id);
			open_button_node.x = 0;
			open_button_node.y = 0;
			open_button_node.set_margins(lmarg, rmarg);
			win.immediate_resize(open_button_node, n_width, n_height);
			win.immediate_add_child(l_id, open_button_id);
		}
	}
	void menu_control::on_lose_focus(window_data& win) {
		close(win, false);
	}

	int32_t menu_control::get_menu_height(window_data const& win) const {
		int32_t cheight = 0;
		if(l_id == layout_reference_none)
			return 0;
		if(vertically_cover_parent) {
			auto parent_page = win.get_containing_page(l_id);
			auto parent_height = parent_page != layout_reference_none ? win.get_node(parent_page).height : 0;
			if(parent_page != layout_reference_none) {
				cheight = parent_height;
			}
		} else {
			auto pi = win.get_node(l_id).page_info();
			if(pi) {
				for(auto n : pi->view_columns()) {
					cheight = std::max(cheight, win.get_node(n).height + 3);
				}
			}
		}
		return cheight;
	}

	layout_rect menu_control::get_content_rectangle(window_data& win) {
		if(list_is_open) {
			if(l_id == layout_reference_none)
				return layout_rect{ 0i16,0i16,0i16,0i16 };

			auto parent_page = win.get_containing_page(l_id);
			auto parent_height = parent_page != layout_reference_none ? win.get_node(parent_page).height : 0;

			auto nheight = win.get_node(l_id).height;
			int16_t cheight = int16_t(get_menu_height(win) - nheight);
			
			auto parent_location = parent_page != layout_reference_none ? win.get_current_location(parent_page) : screen_space_rect{0,0};
			auto self_location = win.get_current_location(l_id);

			auto y_offset = -cheight / 2;
			// ensure
			// node pos.y + offset > page_loc. y
			y_offset = std::max(y_offset, parent_location.y - self_location.y);
			// ensure
			// node pos .y + offset + cheight + nheight < page_loc. y + page height
			y_offset = std::min(y_offset, parent_location.y + parent_height - (self_location.y + cheight + nheight));
			// ensure
			// offset >= 0 (i.e. not below the menu button)
			y_offset = std::min(y_offset, 0);

			return layout_rect{ 0i16, int16_t(y_offset), 0i16, std::max(cheight, 0i16) };
		} else {
			return layout_rect{ 0i16,0i16,0i16,0i16 };
		}
	}

	void menu_control::open(window_data& win, bool move_focus) {
		if(l_id != layout_reference_none) {

			if(menu_control::list_appearance.type != animation_type::none)
				win.rendering_interface.prepare_ui_animation(win);

			list_is_open = true;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(menu_control::list_appearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					menu_control::list_appearance.type,
					win.get_default_animation_direction(l_id),
					menu_control::list_appearance.duration_seconds,
					menu_control::list_appearance.animate_in }, win);
			}

			on_open(win);

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, true);
		}
	}
	void menu_control::close(window_data& win, bool move_focus) {
		if(list_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};

			if(menu_control::list_disappearance.type != animation_type::none) {
				win.rendering_interface.prepare_ui_animation(win);
				loc = win.get_current_location(l_id);
			}

			list_is_open = false;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(menu_control::list_disappearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					loc,
					menu_control::list_disappearance.type,
					win.get_default_animation_direction(l_id),
					menu_control::list_disappearance.duration_seconds,
					menu_control::list_disappearance.animate_in }, win);
			} else {
				win.rendering_interface.stop_ui_animations(win);
			}

			on_close(win);

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, false);
		}
	}
	accessibility_object* menu_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_expandable_container(win, this, name, alt_text);
		}
		return acc_obj;
	}


	animation_defintion menu_control::list_appearance;
	animation_defintion menu_control::list_disappearance;

	accessibility_object* page_footer::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_expandable_container(win, this, text_id::page_footer_name, text_id::page_footer_info);
		}
		return acc_obj;
	}
	ui_rectangle page_footer::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.parent_object = l_id;

		if(footer_is_open) {
			retvalue.top_border = 1;
			retvalue.bottom_border = 1;
			retvalue.left_border = 1;
			retvalue.right_border = 1;
		}

		return retvalue;
	}
	simple_layout_specification page_footer::get_specification(window_data&) {
		simple_layout_specification spec;
		spec.minimum_page_size = 1;
		spec.minimum_line_size = 4;
		return spec;
	}
	void page_footer::recreate_contents(window_data& win, layout_node& n) {
		auto const node_height = n.height;
		auto const node_width = n.width;
		auto const node_lm = n.left_margin();
		auto const node_rm = n.right_margin();

		if(footer_is_open) {
			if(node_width <= 8) {
				{
					auto child_ref = win.create_node(&jump_back, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = 0;
					child_node.y = 0;
					win.immediate_add_child(l_id,child_ref);
				}
				{
					auto child_ref = win.create_node(&back, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = 0;
					child_node.y = 1;
					win.immediate_add_child(l_id, child_ref);
				}
				{
					auto child_ref = win.create_node(&forward, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = 2;
					child_node.y = 1;
					win.immediate_add_child(l_id, child_ref);
				}
				{
					auto child_ref = win.create_node(&jump_forward, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = 2;
					child_node.y = 0;
					win.immediate_add_child(l_id, child_ref);
				}
			} else {
				auto base_offset = ((node_width - 8)) / 2;
				auto even_offset = 1 - (node_width % 2);
				{
					auto child_ref = win.create_node(&jump_back, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = uint16_t(base_offset + 0 - even_offset);
					child_node.y = 0;
					win.immediate_add_child(l_id, child_ref);
				}
				{
					auto child_ref = win.create_node(&back, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = uint16_t(base_offset + 2 - even_offset);
					child_node.y = 0;
					win.immediate_add_child(l_id, child_ref);
				}
				{
					auto child_ref = win.create_node(&forward, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = uint16_t(base_offset + 4);
					child_node.y = 0;
					win.immediate_add_child(l_id, child_ref);
				}
				{
					auto child_ref = win.create_node(&jump_forward, 2, 1, false, true);
					auto& child_node = win.get_node(child_ref);
					child_node.x = uint16_t(base_offset + 6);
					child_node.y = 0;
					win.immediate_add_child(l_id, child_ref);
				}
			}

			{
				auto child_ref = win.create_node(&primary_display,
					node_width, node_height, false, true);

				auto& child_node = win.get_node(child_ref);
				child_node.x = 0;
				child_node.y = node_width <= 8 ? 2ui16 : 1ui16;
				child_node.set_margins(node_lm, node_rm);

				win.immediate_resize(child_node, node_width, node_height);

				win.immediate_add_child(l_id, child_ref);
			}

		} else {
			auto child_ref = win.create_node(&primary_display,
				node_width, node_height, false, true);

			auto& child_node = win.get_node(child_ref);
			child_node.x = 0;
			child_node.y = 0;
			child_node.set_margins(node_lm, node_rm);

			win.immediate_resize(child_node, node_width, node_height);

			win.immediate_add_child(l_id, child_ref);
		}
	}
	layout_rect page_footer::get_content_rectangle(window_data& win) {
		if(footer_is_open) {
			auto nwidth = l_id != layout_reference_none ? win.get_node(l_id).width : 0ui16;
			return layout_rect{
				0i16, int16_t((nwidth <= 8ui16) ? -2i16 : -1i16),
				0i16, int16_t((nwidth <= 8ui16) ? 2i16 : 1i16) };
		} else {
			return layout_rect{ 0i16,0i16,0i16,0i16 };
		}
	}
	void page_footer::on_focus(window_data& win) {
		open(win, false);
	}
	void page_footer::on_lose_focus(window_data& win) {
		close(win, false);
	}

	void page_footer::open(window_data& win, bool move_focus) {
		if(l_id == layout_reference_none)
			return;

		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		bool multiple_pages = parent_pginfo ? parent_pginfo->subpage_divisions.size() != 0 : false;

		if(multiple_pages && !footer_is_open && l_id != layout_reference_none) {

			if(page_footer::footer_appearance.type != animation_type::none)
				win.rendering_interface.prepare_ui_animation(win);

			footer_is_open = true;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(page_footer::footer_appearance.type != animation_type::none) {
				auto nwidth = win.get_node(l_id).width;
				auto loc = win.get_layout_rect_in_current_location(
					layout_rect{ 0, 0, -1, int16_t((nwidth <= 8ui16) ? 2 : 1) }, l_id);
				win.rendering_interface.start_ui_animation(animation_description{
					loc, 
					page_footer::footer_appearance.type,
					animation_direction::bottom,
					page_footer::footer_appearance.duration_seconds,
					page_footer::footer_appearance.animate_in }, win);
			}

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, true);
		}
	}
	void page_footer::close(window_data& win, bool move_focus) {
		if(footer_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(page_footer::footer_disappearance.type != animation_type::none) {
				auto nwidth = win.get_node(l_id).width;
				loc = win.get_layout_rect_in_current_location(
					layout_rect{0, 0, -1, int16_t((nwidth <= 8ui16) ? 2 : 1) }, l_id);
				win.rendering_interface.prepare_ui_animation(win);
			}

			footer_is_open = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface.is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(page_footer::footer_disappearance.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					loc,
					page_footer::footer_disappearance.type,
					animation_direction::bottom,
					page_footer::footer_disappearance.duration_seconds,
					page_footer::footer_disappearance.animate_in }, win);
			}

			if(acc_obj)
				win.accessibility_interface.on_expand_collapse(acc_obj, false);
		}
	}

	animation_defintion page_footer::footer_appearance;
	animation_defintion page_footer::footer_disappearance;

	animation_defintion page_footer::page_turn_up;
	animation_defintion page_footer::page_turn_down;

	void page_footer::start_page_turn_animation(window_data& win, layout_reference page_container, bool increasing) {
		if(increasing) {
			if(page_footer::page_turn_up.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					win.get_current_location(page_container),
					page_footer::page_turn_up.type,
					win.get_default_animation_direction(page_container),
					page_footer::page_turn_up.duration_seconds,
					page_footer::page_turn_up.animate_in }, win);
			}
		} else {
			if(page_footer::page_turn_down.type != animation_type::none) {
				win.rendering_interface.start_ui_animation(animation_description{
					win.get_current_location(page_container),
					page_footer::page_turn_down.type,
					win.get_default_animation_direction(page_container),
					page_footer::page_turn_down.duration_seconds,
					page_footer::page_turn_down.animate_in }, win);
			}
		}
	}
}
