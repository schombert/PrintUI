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

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Windowscodecs.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dxguid.lib")

namespace printui {

	void ui_rectangle::rotate_borders(layout_orientation o) {
		switch(o) {
			case layout_orientation::horizontal_left_to_right:
				return;
			case layout_orientation::horizontal_right_to_left:
			{
				auto const old_left = left_border;
				left_border = right_border;
				right_border = old_left;
				return;
			}
			case layout_orientation::vertical_left_to_right:
			{
				auto const old_left = left_border;
				left_border = top_border;
				top_border = old_left;

				auto const old_right = right_border;
				right_border = bottom_border;
				bottom_border = old_right;
				return;
			}
			case layout_orientation::vertical_right_to_left:
			{
				auto const old_left = left_border;
				left_border = bottom_border;
				bottom_border = right_border;
				right_border = top_border;
				top_border = old_left;
				return;
			}
		}
	}

	uint32_t content_alignment_to_text_alignment(content_alignment align) {
		switch(align) {
			case content_alignment::leading:
				return DWRITE_TEXT_ALIGNMENT_LEADING;
			case content_alignment::trailing:
				return DWRITE_TEXT_ALIGNMENT_TRAILING;
			case content_alignment::centered:
				return DWRITE_TEXT_ALIGNMENT_CENTER;
			case content_alignment::justified:
				return DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		return DWRITE_TEXT_ALIGNMENT_LEADING;
	}
	
	standard_icons::standard_icons() {
		icons[header_minimize].file_name = L"min_i.svg";
		icons[header_minimize].xsize = 1;
		icons[header_minimize].ysize = 1;
		icons[header_minimize].edge_padding = 0.25f;

		icons[header_close].file_name = L"close_i.svg";
		icons[header_close].xsize = 1;
		icons[header_close].ysize = 1;
		icons[header_close].edge_padding = 0.25f;

		icons[control_button].file_name = L"control_button.svg";
		icons[control_button].xsize = 1;
		icons[control_button].ysize = 1;
		icons[control_button].edge_padding = 0.2f;

		icons[control_menu].file_name = L"control_menu.svg";
		icons[control_menu].xsize = 1;
		icons[control_menu].ysize = 1;
		icons[control_menu].edge_padding = 0.0f;

		icons[control_pages].file_name = L"control_pages.svg";
		icons[control_pages].xsize = 1;
		icons[control_pages].ysize = 1;
		icons[control_pages].edge_padding = 0.0f;

		icons[control_list].file_name = L"control_list.svg";
		icons[control_list].xsize = 1;
		icons[control_list].ysize = 1;
		icons[control_list].edge_padding = 0.0f;

		icons[control_next].file_name = L"control_next.svg";
		icons[control_next].xsize = 1;
		icons[control_next].ysize = 1;
		icons[control_next].edge_padding = 0.0f;

		icons[control_next_next].file_name = L"control_next_next.svg";
		icons[control_next_next].xsize = 1;
		icons[control_next_next].ysize = 1;
		icons[control_next_next].edge_padding = 0.0f;

		icons[control_prev].file_name = L"control_prev.svg";
		icons[control_prev].xsize = 1;
		icons[control_prev].ysize = 1;
		icons[control_prev].edge_padding = 0.0f;

		icons[control_prev_prev].file_name = L"control_prev_prev.svg";
		icons[control_prev_prev].xsize = 1;
		icons[control_prev_prev].ysize = 1;
		icons[control_prev_prev].edge_padding = 0.0f;

		icons[control_toggle].file_name = L"control_toggle.svg";
		icons[control_toggle].xsize = 1;
		icons[control_toggle].ysize = 1;
		icons[control_toggle].edge_padding = 0.0f;

		icons[control_text].file_name = L"control_text.svg";
		icons[control_text].xsize = 1;
		icons[control_text].ysize = 1;
		icons[control_text].edge_padding = 0.0f;

		icons[window_settings].file_name = L"settings_i.svg";
		icons[window_settings].xsize = 2;
		icons[window_settings].ysize = 2;
		icons[window_settings].edge_padding = 0.5f;

		icons[window_max].file_name = L"max_i.svg";
		icons[window_max].xsize = 2;
		icons[window_max].ysize = 2;
		icons[window_max].edge_padding = 0.5f;

		icons[window_restore].file_name = L"restore_i.svg";
		icons[window_restore].xsize = 2;
		icons[window_restore].ysize = 2;
		icons[window_restore].edge_padding = 0.5f;

		icons[window_min].file_name = L"min_i.svg";
		icons[window_min].xsize = 2;
		icons[window_min].ysize = 2;
		icons[window_min].edge_padding = 0.5f;

		icons[window_info].file_name = L"alt_i.svg";
		icons[window_info].xsize = 2;
		icons[window_info].ysize = 2;
		icons[window_info].edge_padding = 0.5f;

		icons[window_close].file_name = L"close_i.svg";
		icons[window_close].xsize = 2;
		icons[window_close].ysize = 2;
		icons[window_close].edge_padding = 0.5f;
	}

	int32_t reading_direction_from_orientation(layout_orientation o) {
		switch(o) {
			case layout_orientation::horizontal_left_to_right:
				return int32_t(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
			case layout_orientation::horizontal_right_to_left:
				return int32_t(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
			case layout_orientation::vertical_left_to_right:
				return int32_t(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			case layout_orientation::vertical_right_to_left:
				return int32_t(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
		}

		return int32_t(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
	}
	int32_t flow_direction_from_orientation(layout_orientation o) {
		switch(o) {
			case layout_orientation::horizontal_left_to_right:
				return int32_t(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
			case layout_orientation::horizontal_right_to_left:
				return int32_t(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
			case layout_orientation::vertical_left_to_right:
				return int32_t(DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
			case layout_orientation::vertical_right_to_left:
				return int32_t(DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT);
		}

		return int32_t(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
	}

	animation_direction window_data::get_default_animation_direction(layout_reference id) const {
		if(right_node_id != layout_reference_none && is_child_of(right_node_id, id))
			return animation_direction::right;
		else if(left_node_id != layout_reference_none && is_child_of(left_node_id, id))
			return animation_direction::left;
		else if(top_node_id != layout_reference_none && is_child_of(top_node_id, id))
			return animation_direction::top;
		else if(bottom_node_id != layout_reference_none && is_child_of(bottom_node_id, id))
			return animation_direction::bottom;
		else
			return animation_direction::left;
	}

	screen_space_rect window_data::get_current_location(layout_reference r) const {
		auto& node = get_node(r);
		if(node.visible_rect < get_ui_rects().size()) {
			auto& uirect = get_ui_rects()[node.visible_rect];
			return screen_space_rect{ uirect.x_position, uirect.y_position, uirect.width, uirect.height };
		} else {
			return screen_space_rect{ 0, 0, 0, 0 };
		}
	}

	screen_space_rect window_data::get_layout_rect_in_current_location(layout_rect const& rect, layout_reference r) const {

		auto& node = get_node(r);
		if(node.visible_rect < get_ui_rects().size()) {
			auto& uirect = get_ui_rects()[node.visible_rect];
			return screen_rectangle_from_layout_in_ui(*this,
				rect.x, rect.y,
				rect.width < 0 ? node.width : rect.width, rect.height < 0 ? node.height : rect.height,
				uirect);
		} else {
			return screen_space_rect{ 0, 0, 0, 0 };
		}
	}

	void window_data::remove_references_to(layout_interface* l) {
		if(last_layout_under_cursor == l)
			last_layout_under_cursor = nullptr;
		for(size_t i = 0; i < focus_stack.size(); ++i) {
			if(focus_stack[i].l_interface == l) {
				focus_stack.resize(i);
				repopulate_key_actions();
				break;
			}
		}
		if(keyboard_target && keyboard_target->get_layout_interface() == l)
			set_keyboard_focus(nullptr);
	}

	void window_data::safely_release_interface(layout_interface* v) {
		auto count_vis_rects = get_ui_rects().size();
		remove_references_to(v);
		if(v->l_id != layout_reference_none) {
			auto& node = get_node(v->l_id);
			node.l_interface = nullptr;
			if(node.visible_rect < count_vis_rects) {
				auto& vr = get_ui_rects()[node.visible_rect];
				vr.parent_object = nullptr;
			}
		}
	}
	void window_data::safely_clear_vector(std::vector<std::unique_ptr<layout_interface>>& v) {
		auto count_vis_rects = get_ui_rects().size();
		for(auto& ptr : v) {
			remove_references_to(ptr.get());
			if(ptr->l_id != layout_reference_none) {
				auto& node = get_node(ptr->l_id);
				node.l_interface = nullptr;
				if(node.visible_rect < count_vis_rects) {
					auto& vr = get_ui_rects()[node.visible_rect];
					vr.parent_object = nullptr;
				}
			}
		}
		v.clear();
	}

	std::optional<undo_item> undo_buffer::undo(undo_item current_state) {
		push_state(current_state);

		auto temp_back = buffer_position - 1;
		if(temp_back < 0) {
			temp_back += total_size;
		}

		if(interal_buffer[temp_back].from == current_state.from) {
			buffer_position = temp_back;
			return interal_buffer[temp_back];
		}

		return std::optional<undo_item>{};
	}
	std::optional<undo_item> undo_buffer::redo(undo_item current_state) {
		if(interal_buffer[buffer_position] == current_state) {
			auto temp_back = buffer_position + 1;
			if(temp_back >= total_size) {
				temp_back -= total_size;
			}

			if(interal_buffer[temp_back].from == current_state.from) {
				buffer_position = temp_back;
				return interal_buffer[buffer_position];
			}
		} 
		return std::optional<undo_item>{};
	}
	void undo_buffer::push_state(undo_item state) {
		if(interal_buffer[buffer_position] != state) {
			++buffer_position;
			if(buffer_position >= total_size) {
				buffer_position -= total_size;
			}
			
			interal_buffer[buffer_position] = state;
			auto temp_next = buffer_position + 1;
			if(temp_next >= total_size) {
				temp_next -= total_size;
			}
			interal_buffer[temp_next].from = nullptr;
		}
	}
}
