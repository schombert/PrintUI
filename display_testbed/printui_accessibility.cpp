#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_utility.hpp"
#include "printui_windows_definitions.hpp"
#include "printui_accessibility_definitions.hpp"
#include <algorithm>
#include <Windows.h>
#include <ole2.h>
#include <UIAutomation.h>

#pragma comment(lib, "Uiautomationcore.lib")

namespace printui {
	constexpr int32_t expandable_list_type_control = 95180;
	constexpr int32_t expandable_container_control = 95181;

	disconnectable* accessibility_object_to_iunk(accessibility_object* o) {
		disconnectable* p = (disconnectable*)o;
		return p;
	}

	HRESULT generic_navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal, window_data& win, layout_reference l_id) {

		if(direction == NavigateDirection_FirstChild) {
			auto ch_ptr = accessibility_object_to_iunk(win.get_first_child_accessibility_object(l_id));
			if(ch_ptr)
				return ch_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else if(direction == NavigateDirection_LastChild) {
			auto ch_ptr = accessibility_object_to_iunk(win.get_last_child_accessibility_object(l_id));
			if(ch_ptr)
				return ch_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else if(direction == NavigateDirection_NextSibling) {
			auto sib_ptr = accessibility_object_to_iunk(win.get_next_sibling_accessibility_object(l_id));
			if(sib_ptr)
				return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else if(direction == NavigateDirection_PreviousSibling) {
			auto sib_ptr = accessibility_object_to_iunk(win.get_previous_sibling_accessibility_object(l_id));
			if(sib_ptr)
				return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else if(direction == NavigateDirection_Parent) {
			auto parent_ptr = accessibility_object_to_iunk(win.get_parent_accessibility_object(l_id));
			if(parent_ptr) {
				return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
			} else {
				if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
					return root->QueryInterface(IID_PPV_ARGS(pRetVal));
				}
			}
		}
		return S_OK;
	}

	//
	// ROOT WINDOW PROVIDER
	//

	root_window_provider::root_window_provider(window_data& win, HWND hwnd) : win(win), hwnd(hwnd), m_refCount(1) { 
	}
	
	root_window_provider::~root_window_provider() {
	}
	IFACEMETHODIMP_(ULONG) root_window_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) root_window_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP root_window_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IRawElementProviderFragmentRoot))
			*ppInterface = static_cast<IRawElementProviderFragmentRoot*>(this);
		else if(riid == __uuidof(ITransformProvider))
			*ppInterface = static_cast<ITransformProvider*>(this);
		else if(riid == __uuidof(IWindowProvider))
			*ppInterface = static_cast<IWindowProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP root_window_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP root_window_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		return UiaHostProviderFromHwnd(hwnd, pRetVal);
	}

	IFACEMETHODIMP root_window_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = NULL;
		if(iid == UIA_TransformPatternId) {
			*pRetVal = static_cast<ITransformProvider*>(this);
			AddRef();
		} else if(iid == UIA_WindowPatternId) {
			*pRetVal = static_cast<IWindowProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP root_window_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(direction == NavigateDirection_FirstChild) {
			auto wbir = accessibility_object_to_iunk(win.window_bar.get_accessibility_interface(win));
			if(wbir)
				return wbir->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else if(direction == NavigateDirection_LastChild) {
			if(auto n = win.get_bottom_node(); n != layout_reference_none) {
				if(auto li = win.get_node(n).l_interface; li) {
					if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
						return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
					}
				}
			}
			if(auto n = win.get_right_node(); n != layout_reference_none) {
				if(auto li = win.get_node(n).l_interface; li) {
					if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
						return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
					}
				}
			}
			if(auto n = win.get_left_node(); n != layout_reference_none) {
				if(auto li = win.get_node(n).l_interface; li) {
					if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
						return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
					}
				}
			}
			if(auto n = win.get_top_node(); n != layout_reference_none) {
				if(auto li = win.get_node(n).l_interface; li) {
					if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
						return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
					}
				}
			}
			auto wbir = accessibility_object_to_iunk(win.window_bar.get_accessibility_interface(win));
			if(wbir)
				return wbir->QueryInterface(IID_PPV_ARGS(pRetVal));
		}
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		auto window_rect = win.window_interface->get_window_location();
		pRetVal->left = window_rect.x;
		pRetVal->top = window_rect.y;
		pRetVal->width = window_rect.width;
		pRetVal->height = window_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = NULL;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		*pRetVal = nullptr;
	   return QueryInterface(IID_PPV_ARGS(pRetVal));
	}

	IFACEMETHODIMP root_window_provider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) {
		auto window_rect = win.window_interface->get_window_location();
		auto new_l_ref = layout_reference_under_point(win.get_ui_rects(), int32_t(x) - window_rect.x, int32_t(y) - window_rect.y);

		while(new_l_ref != layout_reference_none) {
			auto& n = win.get_node(new_l_ref);
			if(auto li = n.l_interface; li) {
				if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
					return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
				}
			}
			new_l_ref = n.parent;
		}

		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::GetFocus(IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;
		if(win.keyboard_target) {
			if(auto li = win.keyboard_target->get_layout_interface(); li) {
				if(auto acc = accessibility_object_to_iunk(li->get_accessibility_interface(win)); acc) {
					return acc->QueryInterface(IID_PPV_ARGS(pRetVal));
				}
			}
		}
		return S_OK;
	}

	IFACEMETHODIMP root_window_provider::Move(double x, double y) {
		if(!hwnd)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!win.window_interface->is_maximized()) {
			win.window_interface->move_window(screen_space_rect{ int32_t(x), int32_t(y), int32_t(win.ui_width), int32_t(win.ui_height) });
		}
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::Resize(double width, double height) {
		if(!hwnd)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!win.window_interface->is_maximized()) {
			auto window_rect = win.window_interface->get_window_location();
			win.window_interface->move_window(screen_space_rect{ window_rect.x, window_rect.y, int32_t(std::max(uint32_t(width), win.minimum_ui_width)), int32_t(std::max(uint32_t(height), win.minimum_ui_height)) });
		}
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::Rotate(double) {
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_CanMove(__RPC__out BOOL* pRetVal) {
		*pRetVal = win.window_interface->is_maximized() ? FALSE : TRUE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_CanResize(__RPC__out BOOL* pRetVal) {
		*pRetVal = win.window_interface->is_maximized() ? FALSE : TRUE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_CanRotate(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::SetVisualState(WindowVisualState state) {
		if(!hwnd)
			return UIA_E_ELEMENTNOTAVAILABLE;

		switch(state) {
			case WindowVisualState_Normal:
				win.window_interface->restore(win);
				break;
			case WindowVisualState_Maximized:
				win.window_interface->maximize(win);
				break;
			case WindowVisualState_Minimized:
				win.window_interface->minimize(win);
				break;
		}
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::Close() {
		win.window_interface->close(win);
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::WaitForInputIdle(int, __RPC__out BOOL* pRetVal) {
		*pRetVal = TRUE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_CanMaximize(__RPC__out BOOL* pRetVal) {
		*pRetVal = win.window_bar.max_i.has_value() ? TRUE : FALSE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_CanMinimize(__RPC__out BOOL* pRetVal) {
		*pRetVal = win.window_bar.min_i.has_value() ? TRUE : FALSE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_IsModal(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_WindowVisualState(__RPC__out enum WindowVisualState* pRetVal) {
		if(win.window_interface->is_minimized()) {
			*pRetVal = WindowVisualState_Minimized;
		} else if(win.window_interface->is_maximized()) {
			*pRetVal = WindowVisualState_Maximized;
		} else {
			*pRetVal = WindowVisualState_Normal;
		}
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_WindowInteractionState(__RPC__out enum WindowInteractionState* pRetVal) {
		*pRetVal = WindowInteractionState_ReadyForUserInteraction;
		return S_OK;
	}
	IFACEMETHODIMP root_window_provider::get_IsTopmost(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		return S_OK;
	}


	IFACEMETHODIMP root_window_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_WindowControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsWindowPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsTransformPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				pRetVal->vt = VT_BSTR;
				if(win.visible_window_title()) {
					pRetVal->bstrVal = SysAllocString(win.get_window_title());
				} else {
					WCHAR module_name[MAX_PATH] = {};
					int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
					while(path_used >= 0 && module_name[path_used] != L'\\') {
						--path_used;
					}
					pRetVal->bstrVal = SysAllocString(module_name + path_used + 1);
				}
				break;
			case UIA_BoundingRectanglePropertyId:
			{
				pRetVal->vt = VT_R8 | VT_ARRAY;
				SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
				pRetVal->parray = psa;

				if(psa == nullptr) {
					return E_OUTOFMEMORY;
				}
				auto window_rect = win.window_interface->get_window_location();
				double uiarect[] = { double(window_rect.x), double(window_rect.y), double(window_rect.width), double(window_rect.height) };
				for(LONG i = 0; i < 4; i++) {
					SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
				}
				break;
			}
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = win.window_interface->window_has_focus() ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_WindowWindowVisualStatePropertyId:
				pRetVal->vt = VT_I4;
				if(win.window_interface->is_minimized()) {
					pRetVal->lVal = WindowVisualState_Minimized;
				} else if(win.window_interface->is_maximized()) {
					pRetVal->lVal = WindowVisualState_Maximized;
				} else {
					pRetVal->lVal = WindowVisualState_Normal;
				}
				break;
			case UIA_TransformCanMovePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = win.window_interface->is_maximized() ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_TransformCanResizePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = win.window_interface->is_maximized() ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_WindowCanMinimizePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = win.window_bar.min_i.has_value() ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_WindowCanMaximizePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = win.window_bar.max_i.has_value() ? VARIANT_TRUE : VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	//
	// TEXT BUTTON PROVIDER
	//

	text_button_provider::text_button_provider(window_data& win, button_control_base& b) : disconnectable(win), button(&b), m_refCount(1) {
	}

	text_button_provider::~text_button_provider() {
	}
	IFACEMETHODIMP_(ULONG) text_button_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) text_button_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP text_button_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IInvokeProvider))
			*ppInterface = static_cast<IInvokeProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP text_button_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP text_button_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP text_button_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_InvokePatternId) {
			*pRetVal = static_cast<IInvokeProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP text_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ButtonControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsInvokePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (button && button->is_disabled()) ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
			{
				auto resolved_text = button->get_raw_text(win);
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
			}
				break;
			case UIA_HelpTextPropertyId:
			{
				auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
			}
				break;
			case UIA_BoundingRectanglePropertyId:
			{
				pRetVal->vt = VT_R8 | VT_ARRAY;
				SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
				pRetVal->parray = psa;

				if(psa == nullptr) {
					return E_OUTOFMEMORY;
				}
				auto window_rect = win.window_interface->get_window_location();
				auto client_rect = button->l_id != layout_reference_none ?
					win.get_current_location(button->l_id) :
					screen_space_rect{ 0,0,0,0 };
				double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
				for(LONG i = 0; i < 4; i++) {
					SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
				}
				break;
			}
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP text_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, button->l_id);
	}
	IFACEMETHODIMP text_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(button);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP text_button_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (button && button->l_id != layout_reference_none) ?
			win.get_current_location(button->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP text_button_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP text_button_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP text_button_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP text_button_provider::Invoke() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		button->on_click(win, 0, 0);
		return S_OK;
	}
	void text_button_provider::disconnect() {
		button = nullptr;
	}

	//
	// TEXT LIST BUTTON PROVIDER
	//

	text_list_button_provider::text_list_button_provider(window_data& win, button_control_base& b) : disconnectable(win), button(&b), m_refCount(1) {
	}

	text_list_button_provider::~text_list_button_provider() {
	}
	IFACEMETHODIMP_(ULONG) text_list_button_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) text_list_button_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP text_list_button_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(ISelectionItemProvider))
			*ppInterface = static_cast<ISelectionItemProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP text_list_button_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP text_list_button_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP text_list_button_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_SelectionItemPatternId) {
			*pRetVal = static_cast<ISelectionItemProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP text_list_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ListItemControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsSelectionItemPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = button && button->is_disabled() ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_SelectionItemIsSelectedPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = button && button->is_selected() ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			/*
			case UIA_SelectionItemSelectionContainerPropertyId:
				pRetVal->vt = VT_UNKNOWN;
				pRetVal->punkVal = accessibility_object_to_iunk(win.get_parent_accessibility_object(button.l_id));
				pRetVal->punkVal->AddRef();
				break;
			*/
			case UIA_NamePropertyId:
			{
				auto resolved_text = button->get_raw_text(win);
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
			}
				break;
			case UIA_HelpTextPropertyId:
			{
				auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
			}
				break;
			case UIA_BoundingRectanglePropertyId:
			{
				pRetVal->vt = VT_R8 | VT_ARRAY;
				SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
				pRetVal->parray = psa;

				if(psa == nullptr) {
					return E_OUTOFMEMORY;
				}
				auto window_rect = win.window_interface->get_window_location();
				auto client_rect = (button && button->l_id != layout_reference_none) ?
					win.get_current_location(button->l_id) :
					screen_space_rect{ 0,0,0,0 };
				double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
				for(LONG i = 0; i < 4; i++) {
					SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
				}
				break;
			}
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP text_list_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, button->l_id);
	}
	IFACEMETHODIMP text_list_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(button);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = button && button->l_id != layout_reference_none ?
			win.get_current_location(button->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP text_list_button_provider::Select() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!button->is_selected())
			button->on_click(win, 0, 0);
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::AddToSelection() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!button->is_selected())
			button->on_click(win, 0, 0);
		return S_OK;
	}
	IFACEMETHODIMP text_list_button_provider::RemoveFromSelection() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(button->is_selected())
			button->on_click(win, 0, 0);
		return S_OK;
	}
	IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_IsSelected(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		*pRetVal = button->is_selected() ? TRUE : FALSE;
		return S_OK;
	}
	IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_SelectionContainer(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal) {
		auto parent_ptr = button ? accessibility_object_to_iunk(win.get_parent_accessibility_object(button->l_id)) : nullptr;
		if(parent_ptr) {
			return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
		} else {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}
	}
	void text_list_button_provider::disconnect() {
		button = nullptr;
	}

	//
	// TEXT TOGGLE BUTTON PROVIDER
	//

	text_toggle_button_provider::text_toggle_button_provider(window_data& win, button_control_toggle& b) : disconnectable(win), button(&b), m_refCount(1) {
	}

	text_toggle_button_provider::~text_toggle_button_provider() {
	}
	IFACEMETHODIMP_(ULONG) text_toggle_button_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) text_toggle_button_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP text_toggle_button_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IToggleProvider))
			*ppInterface = static_cast<IToggleProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP text_toggle_button_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP text_toggle_button_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP text_toggle_button_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_TogglePatternId) {
			*pRetVal = static_cast<IToggleProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP text_toggle_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ButtonControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsTogglePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = button && button->is_disabled() ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_ToggleToggleStatePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = button && button->toggle_is_on ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
				break;
			case UIA_NamePropertyId:
			{
				auto resolved_text = button->get_raw_text(win);
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				break;
			}
			case UIA_HelpTextPropertyId:
			{
				auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				break;
			}
			case UIA_BoundingRectanglePropertyId:
			{
				pRetVal->vt = VT_R8 | VT_ARRAY;
				SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
				pRetVal->parray = psa;

				if(psa == nullptr) {
					return E_OUTOFMEMORY;
				}
				auto window_rect = win.window_interface->get_window_location();
				auto client_rect = (button && button->l_id != layout_reference_none) ?
					win.get_current_location(button->l_id) :
					screen_space_rect{ 0,0,0,0 };
				double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
				for(LONG i = 0; i < 4; i++) {
					SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
				}
				break;
			}
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP text_toggle_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, button->l_id);
	}
	IFACEMETHODIMP text_toggle_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(button);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP text_toggle_button_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = button && button->l_id != layout_reference_none ?
			win.get_current_location(button->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP text_toggle_button_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP text_toggle_button_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP text_toggle_button_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP text_toggle_button_provider::Toggle() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		button->on_click(win, 0, 0);
		return S_OK;
	}
	IFACEMETHODIMP text_toggle_button_provider::get_ToggleState(__RPC__out enum ToggleState* pRetVal) {
		*pRetVal = button && button->toggle_is_on ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return S_OK;
	}
	void text_toggle_button_provider::disconnect() {
		button = nullptr;
	}

	//
	// ICON BUTTON PROVIDER
	//


	icon_button_provider::icon_button_provider(window_data& win, icon_button_base& b) : disconnectable(win), button(&b), m_refCount(1) {
	}

	icon_button_provider::~icon_button_provider() {
	}
	IFACEMETHODIMP_(ULONG) icon_button_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) icon_button_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP icon_button_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IInvokeProvider))
			*ppInterface = static_cast<IInvokeProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP icon_button_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP icon_button_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP icon_button_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_InvokePatternId) {
			*pRetVal = static_cast<IInvokeProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP icon_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ButtonControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsInvokePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (button && button->is_disabled(win)) ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				{
					auto resolved_text = button->get_name(win);
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				{
					auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = button->l_id != layout_reference_none ?
						win.get_current_location(button->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP icon_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, button->l_id);
	}
	IFACEMETHODIMP icon_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(button);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP icon_button_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (button && button->l_id != layout_reference_none) ?
			win.get_current_location(button->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP icon_button_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP icon_button_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP icon_button_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP icon_button_provider::Invoke() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		button->on_click(win, 0, 0);
		return S_OK;
	}
	void icon_button_provider::disconnect() {
		button = nullptr;
	}

	//
	// ICON TOGGLE BUTTON PROVIDER
	//

	icon_toggle_button_provider::icon_toggle_button_provider(window_data& win, icon_button_base& b) : disconnectable(win), button(&b), m_refCount(1) {
	}

	icon_toggle_button_provider::~icon_toggle_button_provider() {
	}
	IFACEMETHODIMP_(ULONG) icon_toggle_button_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) icon_toggle_button_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP icon_toggle_button_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IToggleProvider))
			*ppInterface = static_cast<IToggleProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP icon_toggle_button_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP icon_toggle_button_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP icon_toggle_button_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_TogglePatternId) {
			*pRetVal = static_cast<IToggleProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP icon_toggle_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ButtonControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsTogglePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (button && button->is_disabled(win)) ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_ToggleToggleStatePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = button && button->is_toggled() ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
				break;
			case UIA_NamePropertyId:
				{
					auto resolved_text = button->get_name(win);
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				{
					auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = button->l_id != layout_reference_none ?
						win.get_current_location(button->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP icon_toggle_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, button->l_id);
	}
	IFACEMETHODIMP icon_toggle_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(button);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP icon_toggle_button_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (button && button->l_id != layout_reference_none) ?
			win.get_current_location(button->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP icon_toggle_button_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP icon_toggle_button_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP icon_toggle_button_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP icon_toggle_button_provider::Toggle() {
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		button->on_click(win, 0, 0);
		return S_OK;
	}
	IFACEMETHODIMP icon_toggle_button_provider::get_ToggleState(__RPC__out enum ToggleState* pRetVal) {
		*pRetVal = button && button->is_toggled() ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
		if(!button)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return S_OK;
	}
	void icon_toggle_button_provider::disconnect() {
		button = nullptr;
	}

	//
	// OPEN LIST PROVIDER
	//

	open_list_control_provider::open_list_control_provider(window_data& win, open_list_control& b) : disconnectable(win), control(&b), m_refCount(1) {
	}

	open_list_control_provider::~open_list_control_provider() {
	}
	IFACEMETHODIMP_(ULONG) open_list_control_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) open_list_control_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP open_list_control_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(ISelectionProvider))
			*ppInterface = static_cast<ISelectionProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP open_list_control_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP open_list_control_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP open_list_control_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_SelectionPatternId) {
			*pRetVal = static_cast<ISelectionProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP open_list_control_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_ListControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsSelectionPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_SelectionCanSelectMultiplePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_SelectionIsSelectionRequiredPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_SelectionSelectionPropertyId:
				{
					pRetVal->vt = VT_UNKNOWN | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					pRetVal->parray = psa;
					button_control_base* sel = nullptr;
					for(auto p : control->options) {
						if(p->is_selected()) {
							sel = p;
							break;
						}
					}
					auto acc_ptr = sel ? sel->get_accessibility_interface(win) : nullptr;
					IUnknown* iu_ptr = static_cast<IUnknown*>(accessibility_object_to_iunk(acc_ptr));
					if(iu_ptr)
					   iu_ptr->AddRef();
					LONG index = 0;
					SafeArrayPutElement(psa, &index, iu_ptr);
				}
				break;
			case UIA_NamePropertyId:
				{
					auto resolved_text = win.text_data.instantiate_text(control->name_id).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = control->l_id != layout_reference_none ?
						win.get_current_location(control->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP open_list_control_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, control->l_id);
	}
	IFACEMETHODIMP open_list_control_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(control);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (control && control->l_id != layout_reference_none) ?
			win.get_current_location(control->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP open_list_control_provider::GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;

		if(!control) {
			return UIA_E_ELEMENTNOTAVAILABLE;
		}

		SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
		if(psa == nullptr) {
			return E_OUTOFMEMORY;
		}

		button_control_base* sel = nullptr;
		for(auto p : control->options) {
			if(p->is_selected()) {
				sel = p;
				break;
			}
		}
		auto acc_ptr = sel ? sel->get_accessibility_interface(win) : nullptr;
		IUnknown* iu_ptr = static_cast<IUnknown*>(accessibility_object_to_iunk(acc_ptr));
		IRawElementProviderSimple* stored_provider = nullptr;
		if(iu_ptr)
			iu_ptr->QueryInterface(IID_PPV_ARGS(&stored_provider));
		LONG index = 0;
		SafeArrayPutElement(psa, &index, stored_provider);

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::get_CanSelectMultiple(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		return S_OK;
	}
	IFACEMETHODIMP open_list_control_provider::get_IsSelectionRequired(__RPC__out BOOL* pRetVal) {
		*pRetVal = TRUE;
		return S_OK;
	}
	void open_list_control_provider::disconnect() {
		control = nullptr;
	}

	//
	// CONTAINER PROVIDER
	//

	container_provider::container_provider(window_data& win, layout_interface* li, uint16_t name) : disconnectable(win), li(li), m_refCount(1), name(name) {
	}

	container_provider::~container_provider() {
	}
	IFACEMETHODIMP_(ULONG) container_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) container_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP container_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP container_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP container_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP container_provider::GetPatternProvider(PATTERNID, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP container_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;
		
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_PaneControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				if(name != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(name).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = li->l_id != layout_reference_none ?
						win.get_current_location(li->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP container_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, li->l_id);
	}
	IFACEMETHODIMP container_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		*pRetVal = nullptr;

		size_t ptr_value = reinterpret_cast<size_t>(li);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP container_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (li && li->l_id != layout_reference_none) ?
			win.get_current_location(li->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP container_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP container_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP container_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	void container_provider::disconnect() {
		li = nullptr;
	}

	//
	// PLAIN TEXT PROVIDER
	//

	plain_text_provider::plain_text_provider(window_data& win, layout_interface* b, stored_text* t, bool is_content) : disconnectable(win), li(b), txt(t), m_refCount(1), is_content(is_content) {
	}

	plain_text_provider::~plain_text_provider() {
	}
	IFACEMETHODIMP_(ULONG) plain_text_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) plain_text_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP plain_text_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP plain_text_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP plain_text_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP plain_text_provider::GetPatternProvider(PATTERNID, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP plain_text_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!li || !txt)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_TextControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = is_content ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				{
					auto resolved_text = txt->get_raw_text(win);
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = li->l_id != layout_reference_none ?
						win.get_current_location(li->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP plain_text_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, li->l_id);
	}
	IFACEMETHODIMP plain_text_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		*pRetVal = nullptr;

		size_t ptr_value = reinterpret_cast<size_t>(li);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP plain_text_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (li && li->l_id != layout_reference_none) ?
			win.get_current_location(li->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP plain_text_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP plain_text_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP plain_text_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	void plain_text_provider::disconnect() {
		li = nullptr;
		txt = nullptr;
	}

	//
	// EXPANADABLE LIST PROVIDER
	//

	expandable_selection_provider::expandable_selection_provider(window_data& win, generic_expandable* control, generic_selection_container* sc, uint16_t name, uint16_t alt_text) : disconnectable(win), control(control), sc(sc), name(name), alt_text(alt_text), m_refCount(1) {
	}

	expandable_selection_provider::~expandable_selection_provider() {
	}
	IFACEMETHODIMP_(ULONG) expandable_selection_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) expandable_selection_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP expandable_selection_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(ISelectionProvider))
			*ppInterface = static_cast<ISelectionProvider*>(this);
		else if(riid == __uuidof(IExpandCollapseProvider))
			*ppInterface = static_cast<IExpandCollapseProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP expandable_selection_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP expandable_selection_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP expandable_selection_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_SelectionPatternId) {
			*pRetVal = static_cast<ISelectionProvider*>(this);
			AddRef();
		} else if(iid == UIA_ExpandCollapsePatternId) {
			*pRetVal = static_cast<IExpandCollapseProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP expandable_selection_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!control || !sc)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = expandable_list_type_control;
				break;
			case UIA_LocalizedControlTypePropertyId:
				{
					auto resolved_text = win.text_data.instantiate_text(text_id::selection_list_localized_name).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsSelectionPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsExpandCollapsePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_SelectionCanSelectMultiplePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_SelectionIsSelectionRequiredPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_ExpandCollapseExpandCollapseStatePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = control && control->is_open() ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
				break;
			case UIA_SelectionSelectionPropertyId:
				{
					pRetVal->vt = VT_UNKNOWN | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					pRetVal->parray = psa;
					layout_interface* sel = sc->selected_item();
					auto acc_ptr = sel ? sel->get_accessibility_interface(win) : nullptr;
					IUnknown* iu_ptr = static_cast<IUnknown*>(accessibility_object_to_iunk(acc_ptr));
					if(iu_ptr)
						iu_ptr->AddRef();
					LONG index = 0;
					SafeArrayPutElement(psa, &index, iu_ptr);
				}
				break;
			case UIA_NamePropertyId:
				if(name != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(name).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				if(alt_text != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(alt_text).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = control->l_id != layout_reference_none ?
						win.get_current_location(control->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP expandable_selection_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, control->l_id);
	}
	IFACEMETHODIMP expandable_selection_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(control);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (control && control->l_id != layout_reference_none) ?
			win.get_current_location(control->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP expandable_selection_provider::GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;

		if(!sc)
			return UIA_E_ELEMENTNOTAVAILABLE;

		SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
		if(psa == nullptr) {
			return E_OUTOFMEMORY;
		}

		layout_interface* sel = sc ? sc->selected_item() : nullptr;
		auto acc_ptr = sel ? sel->get_accessibility_interface(win) : nullptr;
		IUnknown* iu_ptr = static_cast<IUnknown*>(accessibility_object_to_iunk(acc_ptr));
		IRawElementProviderSimple* stored_provider = nullptr;
		if(iu_ptr)
			iu_ptr->QueryInterface(IID_PPV_ARGS(&stored_provider));
		LONG index = 0;
		SafeArrayPutElement(psa, &index, stored_provider);

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::get_CanSelectMultiple(__RPC__out BOOL* pRetVal) {
		*pRetVal = FALSE;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::get_IsSelectionRequired(__RPC__out BOOL* pRetVal) {
		*pRetVal = TRUE;
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::Expand() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->open(win, true);
		return S_OK;
	}
	IFACEMETHODIMP expandable_selection_provider::Collapse() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->close(win, true);
		return S_OK;

	}
	IFACEMETHODIMP expandable_selection_provider::get_ExpandCollapseState(__RPC__out enum ExpandCollapseState* pRetVal) {
		*pRetVal = control && control->is_open() ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return S_OK;
	}
	void expandable_selection_provider::disconnect() {
		control = nullptr;
		sc = nullptr;
		name = uint16_t(-1);
		alt_text = uint16_t(-1);
	}

	//
	// EXPANDABLE CONTAINER PROVIDER
	//

	expandable_container::expandable_container(window_data& win, generic_expandable* control, uint16_t name, uint16_t alt_text) : disconnectable(win), control(control), name(name), alt_text(alt_text), m_refCount(1) {
	}

	expandable_container::~expandable_container() {
	}
	IFACEMETHODIMP_(ULONG) expandable_container::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) expandable_container::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP expandable_container::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IExpandCollapseProvider))
			*ppInterface = static_cast<IExpandCollapseProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP expandable_container::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP expandable_container::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP expandable_container::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_ExpandCollapsePatternId) {
			*pRetVal = static_cast<IExpandCollapseProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP expandable_container::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = expandable_container_control;
				break;
			case UIA_LocalizedControlTypePropertyId:
			{
				auto resolved_text = win.text_data.instantiate_text(text_id::expandable_container_localized_name).text_content.text;
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
			}
			break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsExpandCollapsePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_ExpandCollapseExpandCollapseStatePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = control && control->is_open() ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
				break;
			case UIA_NamePropertyId:
				if(name != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(name).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				if(alt_text != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(alt_text).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto window_rect = win.window_interface->get_window_location();
					auto client_rect = control->l_id != layout_reference_none ?
						win.get_current_location(control->l_id) :
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(window_rect.x + client_rect.x), double(window_rect.y + client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP expandable_container::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		return generic_navigate(direction, pRetVal, win, control->l_id);
	}
	IFACEMETHODIMP expandable_container::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(control);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto window_rect = win.window_interface->get_window_location();
		auto client_rect = (control && control->l_id != layout_reference_none) ?
			win.get_current_location(control->l_id) :
			screen_space_rect{ 0,0,0,0 };

		pRetVal->left = window_rect.x + client_rect.x;
		pRetVal->top = window_rect.y + client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::SetFocus() {
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	IFACEMETHODIMP expandable_container::Expand() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		control->open(win, true);
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::Collapse() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		
		control->close(win, true);
		return S_OK;
	}
	IFACEMETHODIMP expandable_container::get_ExpandCollapseState(__RPC__out enum ExpandCollapseState* pRetVal) {
		*pRetVal = control && control->is_open() ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return S_OK;
	}
	void expandable_container::disconnect() {
		control = nullptr;
		name = uint16_t(-1);
		alt_text = uint16_t(-1);
	}

	//
	// SIMPLE EDIT RANGE PROVIDER
	//

	simple_edit_range_provider::simple_edit_range_provider(simple_edit_provider* parent, int32_t start, int32_t end) : parent(parent), start(start), end(end), m_refCount(1) {
		if(parent) {
			parent->child_ranges.push_back(this);
		}
	}

	simple_edit_range_provider::~simple_edit_range_provider() {
		if(parent) {
			int32_t i = int32_t(parent->child_ranges.size()) - 1;
			for(; i >= 0; --i) {
				if(parent->child_ranges[i] == this) {
					parent->child_ranges[i] = parent->child_ranges.back();
					parent->child_ranges.pop_back();
				}
			}
		}
	}
	IFACEMETHODIMP_(ULONG) simple_edit_range_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) simple_edit_range_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	int32_t simple_edit_range_provider::get_start() {
		return start;
	}
	int32_t simple_edit_range_provider::get_end() {
		return end;
	}

	IFACEMETHODIMP simple_edit_range_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<ITextRangeProvider*>(this);
		else if(riid == __uuidof(ITextRangeProvider))
			*ppInterface = static_cast<ITextRangeProvider*>(this);
		else if(riid == __uuidof(IRawRangeValues))
			*ppInterface = static_cast<IRawRangeValues*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_range_provider::Clone(__RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		*pRetVal = new simple_edit_range_provider(parent, start, end);
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::Compare(__RPC__in_opt ITextRangeProvider* range, __RPC__out BOOL* pRetVal) {
		IRawRangeValues* rvals = nullptr;
		range->QueryInterface(IID_PPV_ARGS(&rvals));
		*pRetVal = rvals && ((rvals->get_start() == start && rvals->get_end() == end) || (rvals->get_start() == end && rvals->get_end() == start));
		safe_release(rvals);
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::CompareEndpoints(enum TextPatternRangeEndpoint endpoint, __RPC__in_opt ITextRangeProvider* targetRange, enum TextPatternRangeEndpoint targetEndpoint, __RPC__out int* pRetVal) {
		auto self_pt = (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start) ? start : end;

		IRawRangeValues* rvals = nullptr;
		targetRange->QueryInterface(IID_PPV_ARGS(&rvals));
		auto other_ptr = (rvals) ? (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start ? rvals->get_start() : rvals->get_end()) : 0;
		safe_release(rvals);
		*pRetVal = self_pt - other_ptr;
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::ExpandToEnclosingUnit(enum TextUnit unit) {
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		if(end < start)
			std::swap(start, end);

		auto const max_end = int32_t(control->get_text_length());
		start = std::max(start, 0);
		start = std::min(start, max_end);

		if(unit == TextUnit_Character) {
			control->update_analysis(parent->win);
			if(!control->is_valid_cursor_position(start)) {
				start = control->previous_valid_cursor_position(start);
			}
			end = control->next_valid_cursor_position(start);
		} else if(unit == TextUnit_Format || unit == TextUnit_Word) {
			control->update_analysis(parent->win);
			if(!control->is_word_position(start)) {
				start = control->previous_word_position(start);
			}
			end = control->next_word_position(start);
		} else {
			start = 0;
			end = max_end;
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::FindAttribute(TEXTATTRIBUTEID attributeId, VARIANT val, BOOL /*backward*/ , __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		*pRetVal = nullptr;

		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		switch(attributeId) {
			case UIA_AfterParagraphSpacingAttributeId:
				break;
			case UIA_AnimationStyleAttributeId:
				if(val.lVal == AnimationStyle_None) {
					this->AddRef();
					*pRetVal = this;
				}
				break;
			case UIA_AnnotationObjectsAttributeId:
				break;
			case UIA_AnnotationTypesAttributeId:
				break;
			case UIA_BackgroundColorAttributeId:
				break;
			case UIA_BeforeParagraphSpacingAttributeId:
				break;
			case UIA_BulletStyleAttributeId:
				break;
			case UIA_CapStyleAttributeId:
				break;
			case UIA_CaretBidiModeAttributeId:
				if(val.lVal == CaretBidiMode_LTR) {

				} else if(val.lVal == CaretBidiMode_RTL) {

				}
				break;
			case UIA_CaretPositionAttributeId:
				break;
			case UIA_CultureAttributeId:
				break;
			case UIA_FontNameAttributeId:
				break;
			case UIA_FontSizeAttributeId:
				break;
			case UIA_FontWeightAttributeId:
				break;
			case UIA_ForegroundColorAttributeId:
				break;
			case UIA_HorizontalTextAlignmentAttributeId:
				if(val.lVal == HorizontalTextAlignment_Left) {
					if(control->get_alignment() == content_alignment::leading) {
						this->AddRef();
						*pRetVal = this;
					}
				} else if(val.lVal == HorizontalTextAlignment_Centered) {
					if(control->get_alignment() == content_alignment::centered) {
						this->AddRef();
						*pRetVal = this;
					}
				} else if(val.lVal == HorizontalTextAlignment_Right) {
					if(control->get_alignment() == content_alignment::trailing) {
						this->AddRef();
						*pRetVal = this;
					}
				} else if(val.lVal == HorizontalTextAlignment_Justified) {
					if(control->get_alignment() == content_alignment::justified) {
						this->AddRef();
						*pRetVal = this;
					}
				}
				break;
			case UIA_IndentationFirstLineAttributeId:
				break;
			case UIA_IndentationLeadingAttributeId:
				break;
			case UIA_IndentationTrailingAttributeId:
				break;
			case UIA_IsActiveAttributeId:
				if(val.boolVal == VARIANT_FALSE) {
					if(parent->win.keyboard_target != control) {
						this->AddRef();
						*pRetVal = this;
					}
				} else {
					if(parent->win.keyboard_target == control) {
						this->AddRef();
						*pRetVal = this;
					}
				}
				break;
			case UIA_IsHiddenAttributeId:
				if(val.boolVal == VARIANT_FALSE) {
					this->AddRef();
					*pRetVal = this;
				}
				break;
			case UIA_IsItalicAttributeId:
				break;
			case UIA_IsReadOnlyAttributeId:
				if(val.boolVal == VARIANT_FALSE) {
					if(control->is_read_only() == false) {
						this->AddRef();
						*pRetVal = this;
					}
				} else {
					if(control->is_read_only() == true) {
						this->AddRef();
						*pRetVal = this;
					}
				}
				break;
			case UIA_IsSubscriptAttributeId:
				break;
			case UIA_IsSuperscriptAttributeId:
				break;
			case UIA_LineSpacingAttributeId:
				break;
			case UIA_LinkAttributeId:
				break;
			case UIA_MarginBottomAttributeId:
				break;
			case UIA_MarginLeadingAttributeId:
				break;
			case UIA_MarginTopAttributeId:
				break;
			case UIA_MarginTrailingAttributeId:
				break;
			case UIA_OutlineStylesAttributeId:
				break;
			case UIA_OverlineColorAttributeId:
				break;
			case UIA_OverlineStyleAttributeId:
				break;
			case UIA_SelectionActiveEndAttributeId:
				break;
			case UIA_StrikethroughColorAttributeId:
				break;
			case UIA_StrikethroughStyleAttributeId:
				break;
			case UIA_StyleIdAttributeId:
				break;
			case UIA_StyleNameAttributeId:
				break;
			case UIA_TabsAttributeId:
				break;
			case UIA_TextFlowDirectionsAttributeId:
				break;
			case UIA_UnderlineColorAttributeId:
				break;
			case UIA_UnderlineStyleAttributeId:
				break;
		}

		//VariantClear(&val);
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::FindText(__RPC__in BSTR text, BOOL backward, BOOL ignoreCase, __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		*pRetVal = nullptr;
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto first = std::min(start, end);
		auto length = (std::max(start, end) - first);

		auto base_text = control->get_text();
		int32_t found_length = 0;

		if(first >= base_text.length())
			return E_FAIL;

		int32_t found_position = FindNLSStringEx(
			parent->win.text_data.is_locale_default() ? LOCALE_NAME_USER_DEFAULT : parent->win.text_data.locale_string(),
			(backward ? FIND_FROMEND : FIND_FROMSTART) | (ignoreCase ? LINGUISTIC_IGNORECASE : 0) | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING,
			base_text.c_str() + first,
			length,
			text,
			int32_t(wcslen(text)),
			&found_position,
			nullptr, nullptr, 0);

		if(found_position >= 0) {
			*pRetVal = new simple_edit_range_provider(parent, found_position, found_length);
		} else {
			return E_FAIL;
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::GetAttributeValue(TEXTATTRIBUTEID attributeId, __RPC__out VARIANT* pRetVal) {
		VariantInit(pRetVal);

		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto tstart = std::max(std::min(start, end), 0);
		tstart = std::max(tstart, 0);

		switch(attributeId) {
			case UIA_AfterParagraphSpacingAttributeId:
				break;
			case UIA_AnimationStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = AnimationStyle_None;
				break;
			case UIA_AnnotationObjectsAttributeId:
				break;
			case UIA_AnnotationTypesAttributeId:
				break;
			case UIA_BackgroundColorAttributeId:
				break;
			case UIA_BeforeParagraphSpacingAttributeId:
				break;
			case UIA_BulletStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = BulletStyle_None;
				break;
			case UIA_CapStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = CapStyle_None;
				break;
			case UIA_CaretBidiModeAttributeId:
				control->update_analysis(parent->win);
				pRetVal->vt = VT_I4;
				pRetVal->lVal = control->position_is_ltr(tstart) ? CaretBidiMode_LTR : CaretBidiMode_RTL;
				break;
			case UIA_CaretPositionAttributeId:
				pRetVal->vt = VT_I4;
				if(control->get_cursor() == 0) {
					pRetVal->lVal = CaretPosition_BeginningOfLine;
				} else if(control->get_cursor() == control->get_text_length()) {
					pRetVal->lVal = CaretPosition_EndOfLine;
				} else {
					pRetVal->lVal = CaretPosition_Unknown;
				}
				break;
			case UIA_CultureAttributeId:
				break;
			case UIA_FontNameAttributeId:
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(parent->win.dynamic_settings.primary_font.name.c_str());
				break;
			case UIA_FontSizeAttributeId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = parent->win.dynamic_settings.primary_font.font_size;
				break;
			case UIA_FontWeightAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = int32_t(to_font_weight(parent->win.dynamic_settings.primary_font.is_bold));
				break;
			case UIA_ForegroundColorAttributeId:
				break;
			case UIA_HorizontalTextAlignmentAttributeId:
				pRetVal->vt = VT_I4;
				if(control->get_alignment() == content_alignment::leading)
					pRetVal->lVal = HorizontalTextAlignment_Left;
				else if(control->get_alignment() == content_alignment::centered)
					pRetVal->lVal = HorizontalTextAlignment_Centered;
				else if(control->get_alignment() == content_alignment::trailing)
					pRetVal->lVal = HorizontalTextAlignment_Right;
				else if(control->get_alignment() == content_alignment::justified)
					pRetVal->lVal = HorizontalTextAlignment_Justified;
				break;
			case UIA_IndentationFirstLineAttributeId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = 0.0;
				break;
			case UIA_IndentationLeadingAttributeId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = 0.0;
				break;
			case UIA_IndentationTrailingAttributeId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = 0.0;
				break;
			case UIA_IsActiveAttributeId:
				pRetVal->vt = VT_BOOL;
				if(parent->win.keyboard_target == control)
					pRetVal->boolVal = VARIANT_TRUE;
				else
					pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsHiddenAttributeId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsItalicAttributeId:
				pRetVal->vt = VT_BOOL;
				if(parent->win.dynamic_settings.primary_font.is_oblique == true)
					pRetVal->boolVal = VARIANT_TRUE;
				else
					pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsReadOnlyAttributeId:
				pRetVal->vt = VT_BOOL;
				if(control->is_read_only() == true)
					pRetVal->boolVal = VARIANT_TRUE;
				else
					pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsSubscriptAttributeId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_IsSuperscriptAttributeId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_FALSE;
				break;
			case UIA_LineSpacingAttributeId:
				break;
			case UIA_LinkAttributeId:
				break;
			case UIA_MarginBottomAttributeId:
				break;
			case UIA_MarginLeadingAttributeId:
				break;
			case UIA_MarginTopAttributeId:
				break;
			case UIA_MarginTrailingAttributeId:
				break;
			case UIA_OutlineStylesAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = OutlineStyles_None;
				break;
			case UIA_OverlineColorAttributeId:
				break;
			case UIA_OverlineStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = TextDecorationLineStyle_None;
				break;
			case UIA_SelectionActiveEndAttributeId:
				pRetVal->vt = VT_I4;
				if(int32_t(control->get_cursor()) == start)
					pRetVal->lVal = ActiveEnd_Start;
				else if(int32_t(control->get_cursor()) == end)
					pRetVal->lVal = ActiveEnd_End;
				else
					pRetVal->lVal = ActiveEnd_None;
				break;
			case UIA_StrikethroughColorAttributeId:
				break;
			case UIA_StrikethroughStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = TextDecorationLineStyle_None;
				break;
			case UIA_StyleIdAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = StyleId_Normal;
				break;
			case UIA_StyleNameAttributeId:
				break;
			case UIA_TabsAttributeId:
				break;
			case UIA_TextFlowDirectionsAttributeId:
				break;
			case UIA_UnderlineColorAttributeId:
				break;
			case UIA_UnderlineStyleAttributeId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = TextDecorationLineStyle_None;
				break;
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::GetBoundingRectangles(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto first = std::min(start, end);
		auto length = std::max(start, end) - first;

		std::vector<screen_space_rect> selection_rects;
		control->get_range_bounds(parent->win, uint32_t(first), uint32_t(first + length), selection_rects);

		*pRetVal = SafeArrayCreateVector(VT_R8, 0, uint32_t(selection_rects.size() * 4));
		if(*pRetVal != nullptr) {
			for(int32_t i = 0; i < selection_rects.size(); i++) {
				double data[4];
				data[0] = selection_rects[i].x;
				data[1] = selection_rects[i].y;
				data[2] = selection_rects[i].width;
				data[3] = selection_rects[i].height;

				for(int j = 0; j < 4; j++) {
					long index = i*4 + j;
					SafeArrayPutElement(*pRetVal, &index, (void*)&(data[j]));
				}
			}
		} else {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::GetEnclosingElement(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return parent->QueryInterface(IID_PPV_ARGS(pRetVal));
	}
	IFACEMETHODIMP simple_edit_range_provider::GetText(int maxLength, __RPC__deref_out_opt BSTR* pRetVal) {
		*pRetVal = nullptr;

		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto tstart = std::max(std::min(start, end), 0);
		tstart = std::min(tstart, int32_t(control->get_text_length()));
		auto len = std::max(start, end) - tstart;
		len = std::min(int32_t(control->get_text_length()) - tstart, len);
		if(maxLength >= 0)
			len = std::min(len, maxLength);

		auto tstr = control->get_text();

		*pRetVal = SysAllocStringLen(tstr.data() + start, uint32_t(len));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::Move(enum TextUnit unit, int count, __RPC__out int* pRetVal) {
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		if(end < start)
			std::swap(start, end);

		bool degenerate = (start == end);

		start = std::max(start, 0);
		start = std::min(start, int32_t(control->get_text_length()));
		start = std::max(start, 0);

		*pRetVal = 0;


		if(unit == TextUnit_Character) {
			control->update_analysis(parent->win);
			if(!control->is_valid_cursor_position(start)) {
				start = control->previous_valid_cursor_position(start);
			}
			
			while(count > 0) {
				auto next = control->next_valid_cursor_position(start);
				if(int32_t(next) != start) {
					++(*pRetVal);
					start = next;
				} else {
					break;
				}
				--count;
			}
			while(count < 0) {
				auto next = control->previous_valid_cursor_position(start);
				if(int32_t(next) != start) {
					--(*pRetVal);
					start = next;
				} else {
					break;
				}
				++count;
			}
			if(!degenerate)
				end = control->next_valid_cursor_position(start);
			else
				end = start;
		} else if(unit == TextUnit_Format || unit == TextUnit_Word) {
			control->update_analysis(parent->win);
			if(!control->is_word_position(start)) {
				start = control->previous_word_position(start);
			}
			while(count > 0) {
				auto next = control->next_word_position(start);
				if(int32_t(next) != start) {
					++(*pRetVal);
					start = next;
				} else {
					break;
				}
				--count;
			}
			while(count < 0) {
				auto next = control->previous_word_position(start);
				if(int32_t(next) != start) {
					--(*pRetVal);
					start = next;
				} else {
					break;
				}
				++count;
			}
			if(!degenerate)
				end = control->next_word_position(start);
			else
				end = start;
		} else {
			start = 0;
			end = int32_t(control->get_text_length());
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::MoveEndpointByUnit(enum TextPatternRangeEndpoint endpoint, enum TextUnit unit, int count, __RPC__out int* pRetVal) {
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		if(end < start)
			std::swap(start, end);

		int32_t& to_move = (endpoint == TextPatternRangeEndpoint_Start) ? start : end;

		to_move = std::max(to_move, 0);
		to_move = std::min(to_move, int32_t(control->get_text_length()));

		*pRetVal = 0;

		if(unit == TextUnit_Character) {
			control->update_analysis(parent->win);
			while(count > 0) {
				auto next = control->next_valid_cursor_position(to_move);
				if(int32_t(next) != to_move) {
					++(*pRetVal);
					to_move = next;
				} else {
					break;
				}
				--count;
			}
			while(count < 0) {
				auto next = control->previous_valid_cursor_position(to_move);
				if(int32_t(next) != to_move) {
					--(*pRetVal);
					to_move = next;
				} else {
					break;
				}
				++count;
			}
			if(endpoint == TextPatternRangeEndpoint_Start) {
				if(start > end)
					end = start;
			} else {
				if(start > end)
					start = end;
			}
		} else if(unit == TextUnit_Format || unit == TextUnit_Word) {
			control->update_analysis(parent->win);
			while(count > 0) {
				auto next = control->next_word_position(to_move);
				if(int32_t(next) != to_move) {
					++(*pRetVal);
					to_move = next;
				} else {
					break;
				}
				--count;
			}
			while(count < 0) {
				auto next = control->previous_word_position(to_move);
				if(int32_t(next) != to_move) {
					--(*pRetVal);
					to_move = next;
				} else {
					break;
				}
				++count;
			}
			if(endpoint == TextPatternRangeEndpoint_Start) {
				if(start > end)
					end = start;
			} else {
				if(start > end)
					start = end;
			}
		} else {
			if(count > 0) {
				end = int32_t(control->get_text_length()) - 1;
				start = end;
			}
			if(count < 0) {
				start = 0;
				end = 0;
			}
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::MoveEndpointByRange(enum TextPatternRangeEndpoint endpoint, __RPC__in_opt ITextRangeProvider* targetRange, enum TextPatternRangeEndpoint targetEndpoint) {

		IRawRangeValues* rvals = nullptr;
		targetRange->QueryInterface(IID_PPV_ARGS(&rvals));
		auto other_ptr = (rvals) ? (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start ? rvals->get_start() : rvals->get_end()) : 0;
		safe_release(rvals);
		if(endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start) {
			start = other_ptr;
			if(start > end)
				end = start;
		} else {
			end = other_ptr;
			if(start > end)
				start = end;
		}

		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::Select() {
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->set_selection(parent->win, uint32_t(start), uint32_t(end));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::AddToSelection() {
		if(!parent)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto control = parent->control;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->set_selection(parent->win, uint32_t(start), uint32_t(end));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::RemoveFromSelection() {
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::ScrollIntoView(BOOL ) {
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_range_provider::GetChildren(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
		*pRetVal = psa;
		return S_OK;
	}

	//
	// SIMPLE EDIT CONTROL PROVIDER
	//

	simple_edit_provider::simple_edit_provider(window_data& win, edit_interface* b, uint16_t name, uint16_t alt) : disconnectable(win), control(b), name(name), alt(alt), m_refCount(1) {
	}

	simple_edit_provider::~simple_edit_provider() {
		for(auto c : child_ranges) {
			c->parent = nullptr;
		}
	}
	IFACEMETHODIMP_(ULONG) simple_edit_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) simple_edit_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP simple_edit_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(ITextEditProvider))
			*ppInterface = static_cast<ITextEditProvider*>(this);
		else if(riid == __uuidof(IValueProvider))
			*ppInterface = static_cast<IValueProvider*>(this);
		else if(riid == __uuidof(ITextProvider))
			*ppInterface = static_cast<ITextProvider2*>(this);
		else if(riid == __uuidof(ITextProvider2))
			*ppInterface = static_cast<ITextProvider2*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_TextEditPatternId) {
			*pRetVal = static_cast<ITextEditProvider*>(this);
			AddRef();
		} else if(iid == UIA_TextPatternId) {
			*pRetVal = static_cast<ITextProvider2*>(this);
			AddRef();
		} else if(iid == UIA_TextPattern2Id) {
			*pRetVal = static_cast<ITextProvider2*>(this);
			AddRef();
		} else if(iid == UIA_ValuePatternId) {
			*pRetVal = static_cast<IValueProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_EditControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsValuePatternAvailablePropertyId:
			case UIA_IsTextPatternAvailablePropertyId:
			case UIA_IsTextPattern2AvailablePropertyId:
			case UIA_IsTextEditPatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (control && control->is_read_only()) ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				if(name != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(name).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				if(alt != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(alt).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
				{
					pRetVal->vt = VT_R8 | VT_ARRAY;
					SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
					pRetVal->parray = psa;

					if(psa == nullptr) {
						return E_OUTOFMEMORY;
					}
					auto client_rect = control->get_edit_bounds(win);
						screen_space_rect{ 0,0,0,0 };
					double uiarect[] = { double(client_rect.x), double( client_rect.y), double(client_rect.width), double(client_rect.height) };
					for(LONG i = 0; i < 4; i++) {
						SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
					}
				}
				break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (control && win.keyboard_target == control) ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_ValueValuePropertyId:
				auto resolved_text = control->get_text();
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto li = control->get_layout_interface();
		if(!li)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return generic_navigate(direction, pRetVal, win, li->l_id);
	}
	IFACEMETHODIMP simple_edit_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(control);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto client_rect = control->get_edit_bounds(win);
		pRetVal->left = client_rect.x;
		pRetVal->top = client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::SetFocus() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!control->is_read_only()) {
			win.set_keyboard_focus(control);
			return S_OK;
		} else {
			return E_FAIL;
		}
	}
	IFACEMETHODIMP simple_edit_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	void simple_edit_provider::disconnect() {
		control = nullptr;
	}

	IFACEMETHODIMP simple_edit_provider::SetValue(__RPC__in LPCWSTR val) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->insert_text(win, 0, control->get_text_length(), std::wstring_view(val, wcslen(val)));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::get_Value(__RPC__deref_out_opt BSTR* pRetVal) {
		*pRetVal = nullptr;
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto temp_text = control->get_text();
		*pRetVal = SysAllocString(temp_text.c_str());
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::get_IsReadOnly(__RPC__out BOOL* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		*pRetVal = control->is_read_only() ? TRUE : FALSE;
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}
		SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
		*pRetVal = psa;

		if(!psa) {
			return E_FAIL;
		}
		auto cur = control->get_cursor();
		auto anch = control->get_selection_anchor();
		LONG index = 0;
		SafeArrayPutElement(psa, &index,
			new simple_edit_range_provider(this, int32_t(std::min(cur, anch)), int32_t(std::max(cur, anch))));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::GetVisibleRanges(__RPC__deref_out_opt SAFEARRAY** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}
		SAFEARRAY* psa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
		*pRetVal = psa;

		if(!psa) {
			return E_FAIL;
		}

		LONG index = 0;
		SafeArrayPutElement(psa, &index,
			new simple_edit_range_provider(this, 0, int32_t(control->get_text_length())));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::RangeFromChild(__RPC__in_opt IRawElementProviderSimple* , __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}

		*pRetVal = new simple_edit_range_provider(this, 0, int32_t(control->get_text_length()));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::RangeFromPoint(UiaPoint point, __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}

		auto from_point = control->get_position_from_screen_point(win, screen_space_point{ int32_t(point.x), int32_t(point.y) });
		*pRetVal = new simple_edit_range_provider(this, int32_t(from_point), int32_t(from_point));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::get_DocumentRange(__RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}

		*pRetVal = new simple_edit_range_provider(this, 0, int32_t(control->get_text_length()));
		return S_OK;
	}
	IFACEMETHODIMP simple_edit_provider::get_SupportedTextSelection(__RPC__out enum SupportedTextSelection* pRetVal) {
		*pRetVal = SupportedTextSelection_Single;
		return S_OK;
	}

	IFACEMETHODIMP simple_edit_provider::GetActiveComposition(__RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}
		if(auto tl = control->get_temporary_length(); tl != 0) {
			auto ts = control->get_temporary_position();
			*pRetVal = new simple_edit_range_provider(this, int32_t(ts), int32_t(ts + tl));
			return S_OK;
		} else {
			*pRetVal = nullptr;
			return E_FAIL;
		}
		
	}
	IFACEMETHODIMP simple_edit_provider::GetConversionTarget(__RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}
		*pRetVal = nullptr;
		return UIA_E_NOTSUPPORTED;
	}

	IFACEMETHODIMP simple_edit_provider::RangeFromAnnotation(__RPC__in_opt IRawElementProviderSimple* , __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		*pRetVal = nullptr;
		return E_FAIL;
	}
	IFACEMETHODIMP simple_edit_provider::GetCaretRange(__RPC__out BOOL* isActive, __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
		if(!control) {
			*isActive = FALSE;
			*pRetVal = nullptr;
			return UIA_E_ELEMENTNOTAVAILABLE;
		}

		*isActive = win.keyboard_target == control ? TRUE : FALSE;
		*pRetVal = new simple_edit_range_provider(this, int32_t(control->get_cursor()), int32_t(control->get_cursor()));
		return S_OK;
	}

	//
	// NUMERIC EDIT CONTROL PROVIDER
	//

	numeric_edit_provider::numeric_edit_provider(window_data& win, editable_numeric_range& control) : disconnectable(win), control(&control), m_refCount(1) {
	}

	numeric_edit_provider::~numeric_edit_provider() {
	}
	IFACEMETHODIMP_(ULONG) numeric_edit_provider::AddRef() {
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) numeric_edit_provider::Release() {
		long val = InterlockedDecrement(&m_refCount);
		if(val == 0) {
			delete this;
		}
		return val;
	}

	IFACEMETHODIMP numeric_edit_provider::QueryInterface(REFIID riid, void** ppInterface) {
		if(riid == __uuidof(IUnknown))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderSimple))
			*ppInterface = static_cast<IRawElementProviderSimple*>(this);
		else if(riid == __uuidof(IRawElementProviderFragment))
			*ppInterface = static_cast<IRawElementProviderFragment*>(this);
		else if(riid == __uuidof(IRangeValueProvider))
			*ppInterface = static_cast<IRangeValueProvider*>(this);
		else {
			*ppInterface = NULL;
			return E_NOINTERFACE;
		}
		(static_cast<IUnknown*>(*ppInterface))->AddRef();
		return S_OK;
	}

	IFACEMETHODIMP numeric_edit_provider::get_ProviderOptions(ProviderOptions* pRetVal) {
		*pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_RefuseNonClientSupport | ProviderOptions_UseComThreading;
		return S_OK;
	}

	IFACEMETHODIMP numeric_edit_provider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}

	IFACEMETHODIMP numeric_edit_provider::GetPatternProvider(PATTERNID iid, IUnknown** pRetVal) {
		*pRetVal = nullptr;
		if(iid == UIA_RangeValuePatternId) {
			*pRetVal = static_cast<IRangeValueProvider*>(this);
			AddRef();
		}
		return S_OK;
	}

	IFACEMETHODIMP numeric_edit_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
		VariantInit(pRetVal);
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		switch(propertyId) {
			case UIA_ControlTypePropertyId:
				pRetVal->vt = VT_I4;
				pRetVal->lVal = UIA_EditControlTypeId;
				break;
			case UIA_IsKeyboardFocusablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsContentElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsControlElementPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_IsRangeValuePatternAvailablePropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
				break;
			case UIA_RangeValueIsReadOnlyPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (control && control->is_read_only()) ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_IsEnabledPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (control && control->is_read_only()) ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			case UIA_NamePropertyId:
				if(control->get_name() != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(control->get_name()).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_HelpTextPropertyId:
				if(control->get_alt_text() != uint16_t(-1)) {
					auto resolved_text = win.text_data.instantiate_text(control->get_alt_text()).text_content.text;
					pRetVal->vt = VT_BSTR;
					pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
				}
				break;
			case UIA_BoundingRectanglePropertyId:
			{
				pRetVal->vt = VT_R8 | VT_ARRAY;
				SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
				pRetVal->parray = psa;

				if(psa == nullptr) {
					return E_OUTOFMEMORY;
				}
				auto client_rect = control->get_edit_bounds(win);
				screen_space_rect{ 0,0,0,0 };
				double uiarect[] = { double(client_rect.x), double(client_rect.y), double(client_rect.width), double(client_rect.height) };
				for(LONG i = 0; i < 4; i++) {
					SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
				}
			}
			break;
			case UIA_HasKeyboardFocusPropertyId:
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = (control && win.keyboard_target == control) ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case UIA_RangeValueValuePropertyId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = double(control->get_value(win));
				break;
			case UIA_RangeValueLargeChangePropertyId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = 1.0;
				break;
			case UIA_RangeValueSmallChangePropertyId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = 1.0 / std::pow(10, control->precision);
				break;
			case UIA_RangeValueMaximumPropertyId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = double(control->maximum);
				break;
			case UIA_RangeValueMinimumPropertyId:
				pRetVal->vt = VT_R8;
				pRetVal->dblVal = double(control->minimum);
				break;
		}
		return S_OK;
	}

	IFACEMETHODIMP numeric_edit_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
		*pRetVal = nullptr;

		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		return generic_navigate(direction, pRetVal, win, control->l_id);
	}
	IFACEMETHODIMP numeric_edit_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
		if(pRetVal == NULL) {
			return E_INVALIDARG;
		}
		size_t ptr_value = reinterpret_cast<size_t>(control);
		int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
		SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
		if(psa == NULL) {
			return E_OUTOFMEMORY;
		}
		for(LONG i = 0; i < 3; i++) {
			SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
		}

		*pRetVal = psa;
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_BoundingRectangle(UiaRect* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;

		auto client_rect = control->get_edit_bounds(win);
		pRetVal->left = client_rect.x;
		pRetVal->top = client_rect.y;
		pRetVal->width = client_rect.width;
		pRetVal->height = client_rect.height;
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
		*pRetVal = nullptr;
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::SetFocus() {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!control->is_read_only()) {
			win.set_keyboard_focus(control);
			return S_OK;
		} else {
			return E_FAIL;
		}
	}
	IFACEMETHODIMP numeric_edit_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
		if(auto root = win.accessibility_interface->peek_root_window_provider(); root) {
			return root->get_FragmentRoot(pRetVal);
		} else {
			*pRetVal = nullptr;
			return S_OK;
		}
	}
	void numeric_edit_provider::disconnect() {
		control = nullptr;
	}

	IFACEMETHODIMP numeric_edit_provider::SetValue(double val) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		control->set_value(win, float(val));
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_Value(__RPC__out double* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = double(control->get_value(win));
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_IsReadOnly(__RPC__out BOOL* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = control->is_read_only() ? TRUE : FALSE;
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_Maximum(__RPC__out double* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = double(control->maximum);
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_Minimum(__RPC__out double* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = double(control->minimum);
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_LargeChange(__RPC__out double* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = 1.0;
		return S_OK;
	}
	IFACEMETHODIMP numeric_edit_provider::get_SmallChange(__RPC__out double* pRetVal) {
		if(!control)
			return UIA_E_ELEMENTNOTAVAILABLE;
		if(!pRetVal)
			return E_INVALIDARG;
		*pRetVal = 1.0 / std::pow(10, control->precision);
		return S_OK;
	}

	//
	// WIN32 ACCESSIBILITY IMPLEMENTATION
	//

	bool win32_accessibility::has_keyboard_preference() {
		BOOL out_value = FALSE;
		SystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &out_value, 0);
		return out_value == TRUE;
	}
	void win32_accessibility::notify_window_state_change(resize_type r) {
		if(window_interface && window_interface->hwnd && UiaClientsAreListening()) {
			VARIANT old_state;
			VARIANT new_state;
			VariantInit(&old_state);
			VariantInit(&new_state);

			new_state.vt = VT_I4;
			if(r == resize_type::minimize) {
				new_state.lVal = WindowVisualState_Minimized;
			} else if(r == resize_type::maximize) {
				new_state.lVal = WindowVisualState_Maximized;
			} else {
				new_state.lVal = WindowVisualState_Normal;
			}
			UiaRaiseAutomationPropertyChangedEvent(window_interface, UIA_WindowWindowVisualStatePropertyId, old_state, new_state);
		}
	}
	void win32_accessibility::notify_window_moved(int32_t x, int32_t y, int32_t width, int32_t height) {
		if(window_interface && window_interface->hwnd && UiaClientsAreListening()) {
			VARIANT old_state;
			VARIANT new_state;
			VariantInit(&old_state);
			VariantInit(&new_state);

			new_state.vt = VT_R8 | VT_ARRAY;
			SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
			new_state.parray = psa;

			if(!psa) {
				return;
			}
			auto window_rect = win.window_interface->get_window_location();
			double uiarect[] = { double(x), double(y), double(width), double(height) };
			for(LONG i = 0; i < 4; i++) {
				SafeArrayPutElement(psa, &i, (void*)&(uiarect[i]));
			}
			UiaRaiseAutomationPropertyChangedEvent(window_interface, UIA_BoundingRectanglePropertyId, old_state, new_state);
		}
	}
	void win32_accessibility::notify_window_closed() {
		if(window_interface && window_interface->hwnd && UiaClientsAreListening()) {
			UiaRaiseAutomationEvent(window_interface, UIA_Window_WindowClosedEventId);
		}
	}
	win32_accessibility::~win32_accessibility() {
		//UiaDisconnectAllProviders();
	}

	root_window_provider* win32_accessibility::get_root_window_provider() {
		if(!window_interface) {
			hwnd_direct_access_base* b = static_cast<hwnd_direct_access_base*>(win.window_interface->get_os_access(os_handle_type::windows_hwnd));
			if(b) {
				window_interface = make_iunk<root_window_provider>(win, b->m_hwnd);
			} else {
				return nullptr;
			}
		}
		return window_interface;
	}
	root_window_provider* win32_accessibility::peek_root_window_provider() {
		return window_interface;
	}
	void win32_accessibility::release_root_provider() {
		if(window_interface) {
			//UiaReturnRawElementProvider(window_interface->hwnd, 0, 0, nullptr);

			IRawElementProviderSimple* provider = nullptr;
			window_interface->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
			   UiaDisconnectProvider(provider);
			   provider->Release();
			}
			window_interface->hwnd = nullptr;
			window_interface = nullptr;
		}
	}
	void release_accessibility_object(accessibility_object* o) {
		disconnectable* p = (disconnectable*)o;
		IRawElementProviderSimple* provider = nullptr;
		p->disconnect();
		if(p->win.accessibility_interface->peek_root_window_provider()) {
			p->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaDisconnectProvider(provider);
				provider->Release();
			}
		}
		p->Release();
	}


	accessibility_object* win32_accessibility::make_action_button_accessibility_interface(window_data& w, button_control_base& b) {
		auto iunk = static_cast<disconnectable*>(new text_button_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_selection_button_accessibility_interface(window_data& w, button_control_base& b) {
		auto iunk = static_cast<disconnectable*>(new text_list_button_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_toggle_button_accessibility_interface(window_data& w, button_control_toggle& b) {
		auto iunk = static_cast<disconnectable*>(new text_toggle_button_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_icon_button_accessibility_interface(window_data& w, icon_button_base& b) {
		auto iunk = static_cast<disconnectable*>(new icon_button_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_icon_toggle_button_accessibility_interface(window_data& w, icon_button_base& b) {
		auto iunk = static_cast<disconnectable*>(new icon_toggle_button_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_open_list_control_accessibility_interface(window_data& w, open_list_control& b) {
		auto iunk = static_cast<disconnectable*>(new open_list_control_provider(w, b));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_container_accessibility_interface(window_data& w, layout_interface* b, uint16_t name) {
		auto iunk = static_cast<disconnectable*>(new container_provider(w, b, name));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_expandable_selection_list(window_data& w, generic_expandable* control, generic_selection_container* sc, uint16_t name, uint16_t alt_text) {

		auto iunk = static_cast<disconnectable*>(new expandable_selection_provider(w, control, sc, name, alt_text));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_expandable_container(window_data& w, generic_expandable* control, uint16_t name, uint16_t alt_text) {

		auto iunk = static_cast<disconnectable*>(new expandable_container(w, control, name, alt_text));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_plain_text_accessibility_interface(window_data& w, layout_interface* b, stored_text* t, bool is_content) {
		auto iunk = static_cast<disconnectable*>(new plain_text_provider(w, b, t, is_content));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_simple_text_accessibility_interface(window_data& w, edit_interface* control, uint16_t name, uint16_t alt) {
		auto iunk = static_cast<disconnectable*>(new simple_edit_provider(w, control, name, alt));
		return (accessibility_object*)iunk;
	}
	accessibility_object* win32_accessibility::make_numeric_range_accessibility_interface(window_data& w, editable_numeric_range& control) {
		auto iunk = static_cast<disconnectable*>(new numeric_edit_provider(w, control));
		return (accessibility_object*)iunk;
	}

	void win32_accessibility::on_invoke(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		IRawElementProviderSimple* provider = nullptr;
		if(UiaClientsAreListening() && window_interface) {
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_Invoke_InvokedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_selection_change(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		IRawElementProviderSimple* provider = nullptr;
		if(UiaClientsAreListening() && window_interface) {
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_Selection_InvalidatedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_toggle_change(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		IRawElementProviderSimple* provider = nullptr;
		if(UiaClientsAreListening() && window_interface) {
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);

				provider->GetPropertyValue(UIA_ToggleToggleStatePropertyId, &new_state);
				old_state.vt = VT_I4;
				old_state.lVal = new_state.lVal == ToggleState::ToggleState_Off ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
				
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_IsEnabledPropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}

	void win32_accessibility::on_contents_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_StructureChangedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_focus_change(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_AutomationFocusChangedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_text_value_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		IRawElementProviderSimple* provider = nullptr;
		if(UiaClientsAreListening() && window_interface) {
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);

				provider->GetPropertyValue(UIA_ValueValuePropertyId, &new_state);
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_ValueValuePropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}

	void win32_accessibility::on_text_numeric_value_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		IRawElementProviderSimple* provider = nullptr;
		if(UiaClientsAreListening() && window_interface) {
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);

				provider->GetPropertyValue(UIA_RangeValueValuePropertyId, &new_state);
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_RangeValueValuePropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}

	void win32_accessibility::on_text_content_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_Text_TextChangedEventId);
				provider->Release();
			}
		}
	}
	void win32_accessibility::on_text_selection_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_Text_TextSelectionChangedEventId);
				provider->Release();
			}
		}
	}
	void win32_accessibility::on_conversion_target_changed(accessibility_object* b) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_TextEdit_ConversionTargetChangedEventId);
				provider->Release();
			}
		}
	}
	void win32_accessibility::on_composition_change(accessibility_object* b, std::wstring_view comp) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {

				SAFEARRAY* psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
				if(psa == nullptr) {
					return;
				}

				VARIANT string_payload;
				VariantInit(&string_payload);

				string_payload.vt = VT_BSTR;
				string_payload.bstrVal = SysAllocStringLen(comp.data(), uint32_t(comp.length()));

				LONG index = 0;
				SafeArrayPutElement(psa, &index, (void*)&string_payload);
				
				UiaRaiseTextEditTextChangedEvent(provider, TextEditChangeType_Composition, psa);
				provider->Release();
			}
		}
	}
	void win32_accessibility::on_composition_result(accessibility_object* b, std::wstring_view result) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				SAFEARRAY* psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
				if(psa == nullptr) {
					return;
				}

				VARIANT string_payload;
				VariantInit(&string_payload);

				string_payload.vt = VT_BSTR;
				string_payload.bstrVal = SysAllocStringLen(result.data(), uint32_t(result.length()));

				LONG index = 0;
				SafeArrayPutElement(psa, &index, (void*)&string_payload);

				UiaRaiseTextEditTextChangedEvent(provider, TextEditChangeType_CompositionFinalized, psa);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_window_layout_changed() {
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			window_interface->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_LayoutInvalidatedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_focus_returned_to_root()  {
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			window_interface->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				UiaRaiseAutomationEvent(provider, UIA_AutomationFocusChangedEventId);
				provider->Release();
			}
		}
	}

	void win32_accessibility::on_enable_disable(accessibility_object* b, bool disabled) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);
				old_state.vt = VT_BOOL;
				old_state.boolVal = disabled ? VARIANT_FALSE : VARIANT_TRUE;
				new_state.vt = VT_BOOL;
				new_state.boolVal = !disabled ? VARIANT_FALSE : VARIANT_TRUE;
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_IsEnabledPropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}

	void win32_accessibility::on_expand_collapse(accessibility_object* b, bool expanded) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);
				old_state.vt = VT_I4;
				old_state.lVal =!expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
				new_state.vt = VT_I4;
				new_state.lVal = expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_PartiallyExpanded;
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_ExpandCollapseExpandCollapseStatePropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}

	void win32_accessibility::on_select_unselect(accessibility_object* b, bool selection_state) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				if(selection_state) {
					//UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementAddedToSelectionEventId);
					UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementSelectedEventId);
				} else {
					//UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementRemovedFromSelectionEventId);
				}
				provider->Release();
			}
		}
	}
	void win32_accessibility::on_change_name(accessibility_object* b, std::wstring const& new_name) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);
				new_state.vt = VT_BSTR;
				new_state.bstrVal = SysAllocString(new_name.c_str());
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_NamePropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}
	void win32_accessibility::on_change_help_text(accessibility_object* b, std::wstring const& new_text) {
		auto iunk = accessibility_object_to_iunk(b);
		if(UiaClientsAreListening() && window_interface) {
			IRawElementProviderSimple* provider = nullptr;
			iunk->QueryInterface(IID_PPV_ARGS(&provider));
			if(provider) {
				VARIANT old_state;
				VARIANT new_state;
				VariantInit(&old_state);
				VariantInit(&new_state);
				new_state.vt = VT_BSTR;
				new_state.bstrVal = SysAllocString(new_text.c_str());
				UiaRaiseAutomationPropertyChangedEvent(provider, UIA_HelpTextPropertyId, old_state, new_state);

				provider->Release();
			}
		}
	}
}