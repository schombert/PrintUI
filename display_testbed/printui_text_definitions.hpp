#pragma once
#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <msctf.h>
#include <usp10.h>

namespace printui::text {
	struct win32_text_services : public text_services_wrapper {
		ITfThreadMgr* manager_ptr = nullptr;
		TfClientId client_id = 0;
		bool send_notifications = true;

		win32_text_services();
		virtual ~win32_text_services();
		virtual void start_text_services() override;
		virtual void end_text_services() override;
		virtual text_services_object* create_text_service_object(window_data&, edit_interface& ei) override;
		virtual void on_text_change(text_services_object*, uint32_t old_start, uint32_t old_end, uint32_t new_end) override;
		virtual void on_selection_change(text_services_object*) override;
		virtual void set_focus(window_data& win, text_services_object*) override;
		virtual void suspend_keystroke_handling() override;
		virtual void resume_keystroke_handling() override;
		virtual bool send_mouse_event_to_tso(text_services_object* ts, int32_t x, int32_t y, uint32_t buttons) override;
	};
}
