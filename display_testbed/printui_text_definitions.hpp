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

		virtual ~win32_text_services() {
		}
		virtual void start_text_services() override;
		virtual void end_text_services() override;
		virtual text_services_object* create_text_service_object(window_data&, edit_interface& ei) override;
	};
}
