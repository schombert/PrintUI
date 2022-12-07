#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <dwrite_3.h>
#include <d2d1_3.h>

namespace printui {
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


		auto tpoint_a = screen_point_from_layout(win.orientation, win.layout_size * 2, 0, rect);
		auto tpoint_b = screen_point_from_layout(win.orientation, win.layout_size * 2, win.layout_size, rect);

		text.draw_text(win, std::min(tpoint_a.x, tpoint_b.x), std::min(tpoint_a.y, tpoint_b.y));
	}
	void single_line_centered_header::render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		render::background_rectangle(content_rect, win, rect.display_flags, rect.background_index, under_mouse);

		auto new_content_top_left = screen_topleft_from_layout_in_ui(win,
			2, 0, 2, 1, rect);

		D2D1_RECT_F adjusted_content_rect{
			float(new_content_top_left.x), float(new_content_top_left.y),
			content_rect.right - float(new_content_top_left.x - rect.x_position),
			content_rect.bottom
		};

		win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[rect.foreground_index], D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL, adjusted_content_rect, adjusted_content_rect);

		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 0, 1, 1, rect);
		render::interactable(win, win.d2d_device_context, new_top_left, saved_state, rect.foreground_index, true);

		if(saved_state.holds_key()) {
			// TODO
		} else if(saved_state.holds_group()) {
			// TODO
		}
	}
	void single_line_centered_header::recreate_contents(window_data& win, layout_node& self) {
		auto button = win.create_node(&close_button, 2, 1, false);
		auto& button_node = win.get_node(button);
		button_node.x = uint16_t(self.width - 2);
		win.immediate_add_child(l_id, button);
	}
	accessibility_object* single_line_centered_header::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_plain_text_accessibility_interface(win, this, &text, false);
		}
		return acc_obj;
	}
	
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
		render::interactable(win, win.d2d_device_context, new_top_left, saved_state, rect.foreground_index, true);

		if(saved_state.holds_key()) {
			// TODO
		} else if(saved_state.holds_group()) {
			// TODO
		} else {

		}
	}

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

	layout_position common_prepare_base_text_layout(window_data const& win, IDWriteTextLayout*& formatted_text, std::wstring const& text, content_alignment text_alignment, bool use_large_size) {
		layout_position result;

		auto text_format = use_large_size ? win.common_text_format : win.small_text_format;
		auto& font = use_large_size ? win.dynamic_settings.primary_font : win.dynamic_settings.small_font;
		auto const line_width = use_large_size ? win.dynamic_settings.line_width : win.dynamic_settings.small_width;

		text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		text_format->SetReadingDirection(DWRITE_READING_DIRECTION(reading_direction_from_orientation(win.orientation)));
		text_format->SetFlowDirection(DWRITE_FLOW_DIRECTION(flow_direction_from_orientation(win.orientation)));
		text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		text_format->SetOpticalAlignment(DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS);

		if(!horizontal(win.orientation)) {
			text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.vertical_baseline);
		} else {
			text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.baseline);
		}

		if(horizontal(win.orientation)) {

			win.dwrite_factory->CreateTextLayout(text.c_str(), uint32_t(text.length()), text_format, float(font.line_spacing), float(font.line_spacing), &formatted_text);

			DWRITE_TEXT_METRICS text_metrics;
			formatted_text->GetMetrics(&text_metrics);

			result.x = int16_t(std::ceil((text_metrics.width) / float(win.layout_size)));

			if(result.x <= line_width) {
				formatted_text->SetMaxWidth(float(result.x * win.layout_size));
				formatted_text->SetMaxHeight(float(font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
				result.y = 1;
			} else {
				formatted_text->SetMaxWidth(float(line_width * win.layout_size));
				formatted_text->SetMaxHeight(float(100 * font.line_spacing));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

				formatted_text->GetMetrics(&text_metrics);
				result.y = int16_t(std::ceil(text_metrics.lineCount * font.line_spacing / float(win.layout_size)));
				result.x = int16_t(line_width);
				formatted_text->SetMaxHeight(float(text_metrics.lineCount * font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
			}

		} else {
			win.dwrite_factory->CreateTextLayout(text.c_str(), uint32_t(text.length()), text_format, float(font.line_spacing), float(font.line_spacing), &formatted_text);

			DWRITE_TEXT_METRICS text_metrics;
			formatted_text->GetMetrics(&text_metrics);

			result.x = int16_t(std::ceil(text_metrics.height / float(win.layout_size)));
			if(result.x <= line_width) {
				formatted_text->SetMaxHeight(float(result.x * font.line_spacing));
				formatted_text->SetMaxWidth(float(font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
				result.y = 1;
			} else {
				formatted_text->SetMaxHeight(float(line_width * win.layout_size));
				formatted_text->SetMaxWidth(float(100 * font.line_spacing));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

				formatted_text->GetMetrics(&text_metrics);
				result.y = int16_t(std::ceil(text_metrics.lineCount * font.line_spacing / float(win.layout_size)));
				result.x = int16_t(line_width);
				formatted_text->SetMaxWidth(float(text_metrics.lineCount * font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
			}
		}

		return result;
	}

	int32_t stored_text::get_lines_height(window_data const& win) const {
		if(formatted_text) {
			DWRITE_TEXT_METRICS text_metrics;
			formatted_text->GetMetrics(&text_metrics);
			return int32_t(text_metrics.lineCount * (draw_standard_size ? win.dynamic_settings.primary_font.line_spacing : win.dynamic_settings.small_font.line_spacing));
		} else {
			return 0;
		}

	}
	stored_text::~stored_text() {
		safe_release(formatted_text);
	}
	void stored_text::invalidate() {
		safe_release(formatted_text);
	}
	void stored_text::relayout_text(window_data const& win, screen_space_point sz) {
		if(!std::holds_alternative<std::monostate>(text_content)) {
			if(std::holds_alternative<wrapped_text_instance>(text_content)) {
				if(std::get<wrapped_text_instance>(text_content).text_generation != win.text_data.text_generation) {
					safe_release(formatted_text);
					std::get<wrapped_text_instance>(text_content).text_generation = win.text_data.text_generation;
				}
			} else if(std::holds_alternative<std::wstring>(text_content)) {
				if(formatted_text) {
					if(formatted_text->GetReadingDirection() !=
						DWRITE_READING_DIRECTION(reading_direction_from_orientation(win.orientation)) ||
						formatted_text->GetFlowDirection() != DWRITE_FLOW_DIRECTION(flow_direction_from_orientation(win.orientation))) {
						safe_release(formatted_text);
					}
				}
			}

			if(!formatted_text) {
				prepare_text(win);
			}
			if(formatted_text->GetMaxWidth() != float(sz.x))
				formatted_text->SetMaxWidth(float(sz.x));
			if(formatted_text->GetMaxHeight() != float(sz.y))
				formatted_text->SetMaxHeight(float(sz.y));
		}
	}

	void stored_text::prepare_text(window_data const& win) {

		if(std::holds_alternative<wrapped_text_instance>(text_content)) {
			if(std::get<wrapped_text_instance>(text_content).text_generation != win.text_data.text_generation) {
				safe_release(formatted_text);
				std::get<wrapped_text_instance>(text_content).text_generation = win.text_data.text_generation;
			}

		} else if(std::holds_alternative<std::wstring>(text_content)) {
			if(formatted_text) {
				if(formatted_text->GetReadingDirection() !=
					DWRITE_READING_DIRECTION(reading_direction_from_orientation(win.orientation)) ||
					formatted_text->GetFlowDirection() != DWRITE_FLOW_DIRECTION(flow_direction_from_orientation(win.orientation))) {
					safe_release(formatted_text);
				}
			}
		}

		if(!formatted_text) {
			if(std::holds_alternative<wrapped_text_instance>(text_content)) {
				auto text_with_format = win.text_data.instantiate_text(std::get<wrapped_text_instance>(text_content).text_id, std::get<wrapped_text_instance>(text_content).stored_params, std::get<wrapped_text_instance>(text_content).stored_params + std::get<wrapped_text_instance>(text_content).params_count).text_content;

				resolved_text_size = common_prepare_base_text_layout(win, formatted_text, text_with_format.text, text_alignment, draw_standard_size);

				text::apply_formatting(formatted_text, text_with_format.formatting, win.dynamic_settings.named_fonts, win.dwrite_factory);
			} else if(std::holds_alternative<std::wstring>(text_content)) {
				resolved_text_size = common_prepare_base_text_layout(win, formatted_text, std::get<std::wstring>(text_content), text_alignment, draw_standard_size);
			}
		}
	}
	void stored_text::draw_text(window_data const& win, int32_t x, int32_t y) const {
		win.d2d_device_context->DrawTextLayout(D2D1_POINT_2F{ float(x), float(y) }, formatted_text, win.dummy_brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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

		D2D1_RECT_F bg_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		render::background_rectangle(bg_rect, win, rect.display_flags, rect.background_index, under_mouse);
		
		auto text_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), label_text.resolved_text_size.y, rect);

		D2D1_RECT_F content_rect{
			float(text_rect.x), float(text_rect.y),
			float(text_rect.x + text_rect.width), float(text_rect.y + text_rect.height) };

		win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[rect.foreground_index], D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL, content_rect, content_rect);
	}
	void label_control::on_right_click(window_data& win, uint32_t, uint32_t) {
		auto& node = win.get_node(l_id);

		if(alt_text != uint16_t(-1)) {
			win.info_popup.open(win, info_window::parameters{ l_id }.right(label_text.text_alignment == content_alignment::trailing).text(alt_text).width(8).internal_margins(node.left_margin(), node.right_margin()));
		}
	}
	accessibility_object* label_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_plain_text_accessibility_interface(win, this, &label_text, true);
		}
		return acc_obj;
	}

	void label_control::set_text(window_data& win) {
		label_text.set_text();
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface->on_change_name(acc_obj, L"");
		}
	}
	void label_control::set_text(window_data& win, uint16_t val) {
		label_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			if(val != uint16_t(-1)) {
				win.accessibility_interface->on_change_name(acc_obj, label_text.get_raw_text(win));
			} else {
				win.accessibility_interface->on_change_name(acc_obj, L"");
			}
		}
	}
	void label_control::set_text(window_data& win, std::wstring const& val) {
		label_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface->on_change_name(acc_obj, val);
		}
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

		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		render::background_rectangle(content_rect, win,
			selected ? (rect.display_flags & ~ui_rectangle::flag_skip_bg) : rect.display_flags,
			bg_index, under_mouse && !disabled);

		if(!disabled && !selected) {
			auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, ((button_text.resolved_text_size.y - 1) / 2), 1, (((button_text.resolved_text_size.y - 1) / 2) + 1), rect);
			render::interactable_or_foreground(win, win.d2d_device_context, new_top_left, saved_state, fg_index, false);
		}

		auto& node = win.get_node(l_id);

		auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, win.get_node(l_id).width - (node.left_margin() + node.right_margin()), button_text.resolved_text_size.y, rect);

		D2D1_RECT_F adjusted_content_rect{
			float(new_content_rect.x), float(new_content_rect.y),
			float(new_content_rect.x + new_content_rect.width), float(new_content_rect.y + new_content_rect.height)
		};

		if(!disabled) {
			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[fg_index], D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL, adjusted_content_rect, adjusted_content_rect);
		} else {
			win.palette[fg_index]->SetOpacity(0.6f);
			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[fg_index], D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL, adjusted_content_rect, adjusted_content_rect);
			win.palette[fg_index]->SetOpacity(1.0f);
		}
	}
	void button_control_base::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto& node = win.get_node(l_id);

		button_text.relayout_text(win, horizontal(win.orientation) ? screen_space_point{ rect.width - (node.left_margin() + node.right_margin()) * win.layout_size, rect.height } : screen_space_point{ rect.width, rect.height - (node.left_margin() + node.right_margin()) * win.layout_size });

		auto icon_location = screen_topleft_from_layout_in_ui(win, 0, ((button_text.resolved_text_size.y - 1) / 2), 1, (((button_text.resolved_text_size.y - 1) / 2) + 1), rect);

		win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Translation(float(icon_location.x), float(icon_location.y)));

		win.d2d_device_context->FillOpacityMask(
			win.common_icons.icons[icon].rendered_layer,
			win.dummy_brush,
			D2D1_OPACITY_MASK_CONTENT_GRAPHICS);

		win.d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());

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
						win.accessibility_interface->on_invoke(acc_obj);
					} else if(category == button_category::selection_button) {
						win.accessibility_interface->on_select_unselect(acc_obj, true);
					} else if(category == button_category::toggle_button) {
						win.accessibility_interface->on_toggle_change(acc_obj);
					}
				}
			}
		}
	}
	accessibility_object* button_control_base::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			if(category == button_category::action_button) {
				acc_obj = win.accessibility_interface->make_action_button_accessibility_interface(win, *this);
			} else if(category == button_category::selection_button) {
				acc_obj = win.accessibility_interface->make_selection_button_accessibility_interface(win, *this);
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
				win.accessibility_interface->on_enable_disable(acc_obj, v);
			}
		}
	}
	void button_control_base::set_selected(window_data& win, bool v) {
		if(selected != v) {
			selected = v;
			if(acc_obj && win.is_visible(l_id)) {
				win.accessibility_interface->on_select_unselect(acc_obj, v);
				auto container = win.get_parent_accessibility_object(l_id);
				if(container)
					win.accessibility_interface->on_selection_change(container);
			}
		}
	}
	void button_control_base::set_alt_text(window_data& win, uint16_t alt) {
		if(alt_text != alt) {
			alt_text = alt;
			if(acc_obj && win.is_visible(l_id)) {
				if(alt != uint16_t(-1)) {
					win.accessibility_interface->on_change_help_text(acc_obj, win.text_data.instantiate_text(alt).text_content.text);
				} else {
					win.accessibility_interface->on_change_help_text(acc_obj, L"");
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
				win.accessibility_interface->on_change_name(acc_obj, win.text_data.instantiate_text(val, b, e).text_content.text);
			} else {
				win.accessibility_interface->on_change_name(acc_obj, L"");
			}
		}
	}
	void button_control_base::set_text(window_data& win, std::wstring const& val) {
		button_text.set_text(val);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface->on_change_name(acc_obj, val);
			
		}
	}
	void button_control_base::set_text(window_data& win) {
		button_text.set_text();
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface->on_change_name(acc_obj, L"");
		}
	}
	void button_control_base::set_text(window_data& win, stored_text const& t) {
		button_text.set_text(t);
		if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface->on_change_name(acc_obj, t.get_raw_text(win));
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
			acc_obj = win.accessibility_interface->make_toggle_button_accessibility_interface(win, *this);
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
				win.accessibility_interface->on_toggle_change(acc_obj);
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

		spec.minimum_page_size = uint16_t(display_vertically ? win.common_icons.icons[ico].ysize + 1 : win.common_icons.icons[ico].ysize);
		spec.minimum_line_size = uint16_t(display_vertically ? win.common_icons.icons[ico].xsize : win.common_icons.icons[ico].xsize + 1);
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

		D2D1_RECT_F content_rect{
			float(rect.x_position), float(rect.y_position),
			float(rect.x_position + rect.width), float(rect.y_position + rect.height) };

		render::background_rectangle(content_rect, win,
			is_toggled() ? (rect.display_flags & ~ui_rectangle::flag_skip_bg) : rect.display_flags,
			bg_index, under_mouse && !is_disabled(win));

		if(!is_disabled(win) && saved_state.holds_key()) {
			auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 0,
				display_vertically ? win.common_icons.icons[ico].xsize : 1, 
				display_vertically ? 1 : win.common_icons.icons[ico].ysize,
				rect);
			auto center_amount = win.layout_size * ((display_vertically ? win.common_icons.icons[ico].xsize : win.common_icons.icons[ico].ysize) - 1) / 2;
			if(horizontal(win.orientation)) {
				render::interactable(win, win.d2d_device_context, screen_space_point{ new_top_left.x + center_amount, new_top_left.y }, saved_state, fg_index, display_vertically);
			} else {
				render::interactable(win, win.d2d_device_context, screen_space_point{ new_top_left.x, new_top_left.y + center_amount }, saved_state, fg_index, display_vertically);
			}
		}

		{
			auto new_top_left = screen_topleft_from_layout_in_ui(win, display_vertically ? 0 : 1, display_vertically ? 1 : 0, win.common_icons.icons[ico].xsize, win.common_icons.icons[ico].ysize, rect);
			if(is_disabled(win)) {
				win.palette[rect.foreground_index]->SetOpacity(0.6f);
				win.common_icons.icons[ico].present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.palette[fg_index]);
				win.palette[rect.foreground_index]->SetOpacity(1.0f);
			} else {
				win.common_icons.icons[ico].present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.palette[fg_index]);
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
				win.accessibility_interface->on_invoke(acc_obj);
			}
		}
	}
	accessibility_object* icon_button_base::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			if(category == button_category::action_button) {
				acc_obj = win.accessibility_interface->make_icon_button_accessibility_interface(win, *this);
			} else if(category == button_category::toggle_button) {
				acc_obj = win.accessibility_interface->make_icon_toggle_button_accessibility_interface(win, *this);
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
					win.accessibility_interface->on_change_help_text(acc_obj, win.text_data.instantiate_text(alt).text_content.text);
				} else {
					win.accessibility_interface->on_change_help_text(acc_obj, L"");
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
					win.accessibility_interface->on_change_name(acc_obj, win.text_data.instantiate_text(t).text_content.text);
				} else {
					win.accessibility_interface->on_change_name(acc_obj, L"");
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

		button_control_base::render_foreground(rect, win);
	}
	void page_footer_button::update_page(window_data& win) {
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

	void page_footer_button::button_action(window_data& win) {
		if(l_id != layout_reference_none) {
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
				win.prepare_ui_animation();

		parent_pginfo->subpage_offset -= 1ui16;

		if(page_footer::page_turn_down.type != animation_type::none) {
			if(page_footer::page_turn_down.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					page_footer::page_turn_down.type,
					win.get_default_animation_direction(l_id),
					page_footer::page_turn_down.duration_seconds,
					page_footer::page_turn_down.animate_in });
			}
		}

		win.redraw_ui();
	}


	std::wstring page_jump_back_button::get_name(window_data const& win) {
		text::text_parameter tp[1] = { text::int_param(jump_size(win)) };
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
			win.prepare_ui_animation();

		auto int_amount = uint16_t(jump_size(win));
		parent_pginfo->subpage_offset -= int_amount;

		if(page_footer::page_turn_down.type != animation_type::none) {
			if(page_footer::page_turn_down.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					page_footer::page_turn_down.type,
					win.get_default_animation_direction(l_id),
					page_footer::page_turn_down.duration_seconds,
					page_footer::page_turn_down.animate_in });
			}
		}

		win.redraw_ui();
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
			win.prepare_ui_animation();

		parent_pginfo->subpage_offset += 1ui16;

		if(page_footer::page_turn_up.type != animation_type::none) {
			if(page_footer::page_turn_up.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					page_footer::page_turn_up.type,
					win.get_default_animation_direction(l_id),
					page_footer::page_turn_up.duration_seconds,
					page_footer::page_turn_up.animate_in });
			}
		}

		win.redraw_ui();
	}

	std::wstring page_jump_forward_button::get_name(window_data const& win) {
		text::text_parameter tp[1] = { text::int_param(jump_size(win)) };
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
			win.prepare_ui_animation();

		auto int_amount = uint16_t(jump_size(win));
		auto page_container = win.get_containing_page(l_id);
		page_information* parent_pginfo = (page_container != layout_reference_none) ? win.get_node(page_container).page_info() : nullptr;

		parent_pginfo->subpage_offset += int_amount;
		parent_pginfo->subpage_offset = std::min(parent_pginfo->subpage_offset, uint16_t(parent_pginfo->subpage_divisions.size()));

		if(page_footer::page_turn_up.type != animation_type::none) {
			if(page_footer::page_turn_up.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					page_footer::page_turn_up.type,
					win.get_default_animation_direction(l_id),
					page_footer::page_turn_up.duration_seconds,
					page_footer::page_turn_up.animate_in });
			}
		}

		win.redraw_ui();
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
			acc_obj = win.accessibility_interface->make_open_list_control_accessibility_interface(win, *this);
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
			acc_obj = win.accessibility_interface->make_expandable_selection_list(win, this, this, name, alt_text_id);
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
				win.prepare_ui_animation();

			list_is_open = true;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}
			
			if(list_control::list_appearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					list_control::list_appearance.type,
					win.get_default_animation_direction(l_id),
					list_control::list_appearance.duration_seconds,
					list_control::list_appearance.animate_in });
			}

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, true);
		}
	}

	void list_control::close(window_data& win, bool move_focus) {
		if(list_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(list_control::list_disappearance.type != animation_type::none) {
				win.prepare_ui_animation();
				loc = win.get_current_location(l_id);
			}

			list_is_open = false;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(list_control::list_disappearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					loc,
					list_control::list_disappearance.type,
					win.get_default_animation_direction(l_id),
					list_control::list_disappearance.duration_seconds,
					list_control::list_disappearance.animate_in });
			} else {
				win.stop_ui_animations();
			}

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, false);
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
				win.prepare_ui_animation();

			list_is_open = true;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(menu_control::list_appearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					win.get_current_location(l_id),
					menu_control::list_appearance.type,
					win.get_default_animation_direction(l_id),
					menu_control::list_appearance.duration_seconds,
					menu_control::list_appearance.animate_in });
			}

			on_open(win);

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, true);
		}
	}
	void menu_control::close(window_data& win, bool move_focus) {
		if(list_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};

			if(menu_control::list_disappearance.type != animation_type::none) {
				win.prepare_ui_animation();
				loc = win.get_current_location(l_id);
			}

			list_is_open = false;
			win.info_popup.currently_visible = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(menu_control::list_disappearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					loc,
					menu_control::list_disappearance.type,
					win.get_default_animation_direction(l_id),
					menu_control::list_disappearance.duration_seconds,
					menu_control::list_disappearance.animate_in });
			} else {
				win.stop_ui_animations();
			}

			on_close(win);

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, false);
		}
	}
	accessibility_object* menu_control::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_expandable_container(win, this, name, alt_text);
		}
		return acc_obj;
	}


	animation_defintion menu_control::list_appearance;
	animation_defintion menu_control::list_disappearance;

	accessibility_object* page_footer::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_expandable_container(win, this, text_id::page_footer_name, text_id::page_footer_info);
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
			return layout_rect{ 0i16, (nwidth <= 8ui16) ? -2i16 : -1i16, 0i16, (nwidth <= 8ui16) ? 2i16 : 1i16 };
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
		if(!footer_is_open && l_id != layout_reference_none) {

			if(page_footer::footer_appearance.type != animation_type::none)
				win.prepare_ui_animation();

			footer_is_open = true;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.set_window_focus(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(page_footer::footer_appearance.type != animation_type::none) {
				auto nwidth = win.get_node(l_id).width;
				auto loc = win.get_layout_rect_in_current_location(
					layout_rect{ 0, 0, -1, (nwidth <= 8ui16) ? 2 : 1 }, l_id);
				win.start_ui_animation(animation_description{
					loc, 
					page_footer::footer_appearance.type,
					animation_direction::bottom,
					page_footer::footer_appearance.duration_seconds,
					page_footer::footer_appearance.animate_in });
			}

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, true);
		}
	}
	void page_footer::close(window_data& win, bool move_focus) {
		if(footer_is_open && l_id != layout_reference_none) {
			screen_space_rect loc{};
			if(page_footer::footer_disappearance.type != animation_type::none) {
				auto nwidth = win.get_node(l_id).width;
				loc = win.get_layout_rect_in_current_location(
					layout_rect{0, 0, -1, (nwidth <= 8ui16) ? 2 : 1 }, l_id);
				win.prepare_ui_animation();
			}

			footer_is_open = false;

			win.recreate_contents(this, &(win.get_node(l_id)));
			win.redraw_ui();
			win.get_layout();

			if(move_focus) {
				if(!win.window_interface->is_mouse_cursor_visible())
					win.back_focus_out_of(this);
				else
					win.update_window_focus();
			} else {
				win.repopulate_key_actions();
			}

			if(page_footer::footer_disappearance.type != animation_type::none) {
				win.start_ui_animation(animation_description{
					loc,
					page_footer::footer_disappearance.type,
					animation_direction::bottom,
					page_footer::footer_disappearance.duration_seconds,
					page_footer::footer_disappearance.animate_in });
			}

			if(acc_obj)
				win.accessibility_interface->on_expand_collapse(acc_obj, false);
		}
	}

	animation_defintion page_footer::footer_appearance;
	animation_defintion page_footer::footer_disappearance;

	animation_defintion page_footer::page_turn_up;
	animation_defintion page_footer::page_turn_down;
}
