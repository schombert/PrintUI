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
        /*
        size_t ptr_value = reinterpret_cast<size_t>(hwnd);
        int rId[] = { UiaAppendRuntimeId, int32_t(ptr_value & 0xFFFFFFFF), int32_t(ptr_value >> 32) };
        SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 3);
        if(psa == NULL) {
            return E_OUTOFMEMORY;
        }
        for(LONG i = 0; i < 3; i++) {
            SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
        }

        *pRetVal = psa;
        * */
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
        // TODO when edit controls are implemented
        *pRetVal = nullptr;
        return S_OK;
    }

    IFACEMETHODIMP root_window_provider::Move(double x, double y) {
        if(!win.window_interface->is_maximized() && hwnd) {
            win.window_interface->move_window(screen_space_rect{ int32_t(x), int32_t(y), int32_t(win.ui_width), int32_t(win.ui_height) });
        }
        return S_OK;
    }
    IFACEMETHODIMP root_window_provider::Resize(double width, double height) {
        if(!win.window_interface->is_maximized() && hwnd) {
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
            return S_OK;

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
                pRetVal->bstrVal = SysAllocString(win.get_window_title());
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
                if(button) {
                    auto resolved_text = button->get_raw_text(win);
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_HelpTextPropertyId:
                if(button) {
                    auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(button) {
                    pRetVal->vt = VT_R8 | VT_ARRAY;
                    SAFEARRAY* psa = SafeArrayCreateVector(VT_R8, 0, 4);
                    pRetVal->parray = psa;

                    if(psa == nullptr) {
                        return E_OUTOFMEMORY;
                    }
                    auto window_rect = win.window_interface->get_window_location();
                    auto client_rect = button->l_id != layout_reference_none ?
                        win.get_current_location(button->l_id) :
                        screen_space_rect{0,0,0,0};
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

    IFACEMETHODIMP text_button_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
        *pRetVal = nullptr;

        if(!button)
            return S_OK;

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
        if(button)
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
        if(iid == UIA_SelectionPatternId) {
            *pRetVal = static_cast<ISelectionItemProvider*>(this);
            AddRef();
        }
        return S_OK;
    }

    IFACEMETHODIMP text_list_button_provider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
        VariantInit(pRetVal);
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
            case UIA_IsSelectionPatternAvailablePropertyId:
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
                if(button) {
                    auto resolved_text = button->get_raw_text(win);
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_HelpTextPropertyId:
                if(button) {
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
            return S_OK;

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
        if(button && !button->is_selected())
            button->on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP text_list_button_provider::AddToSelection() {
        if(button && !button->is_selected())
            button->on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP text_list_button_provider::RemoveFromSelection() {
        if(button && button->is_selected())
            button->on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_IsSelected(__RPC__out BOOL* pRetVal) {
        *pRetVal = button && button->is_selected() ? TRUE : FALSE;
        return S_OK;
    }
    IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_SelectionContainer(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal) {
        auto parent_ptr = button ? accessibility_object_to_iunk(win.get_parent_accessibility_object(button->l_id)) : nullptr;
        if(parent_ptr)
            return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        else
            return S_OK;
    }
    void text_list_button_provider::disconnect() {
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
                if(button) {
                    auto resolved_text = button->get_name(win);
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_HelpTextPropertyId:
                if(button) {
                    auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(button) {
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
            return S_OK;

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
        if(button)
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
                if(button) {
                    auto resolved_text = button->get_name(win);
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_HelpTextPropertyId:
                if(button) {
                    auto resolved_text = win.text_data.instantiate_text(button->get_alt_text()).text_content.text;
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(button) {
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
            return S_OK;

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
        if(button)
            button->on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP icon_toggle_button_provider::get_ToggleState(__RPC__out enum ToggleState* pRetVal) {
        *pRetVal = button && button->is_toggled() ? ToggleState::ToggleState_On : ToggleState::ToggleState_Off;
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
            case UIA_IsTogglePatternAvailablePropertyId:
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
                if(control) {
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
                    iu_ptr->AddRef();
                    LONG index = 0;
                    SafeArrayPutElement(psa, &index, iu_ptr);
                }
                break;
            case UIA_NamePropertyId:
                if(control) {
                    auto resolved_text = win.text_data.instantiate_text(control->name_id).text_content.text;
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(control) {
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
            return S_OK;

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
                if(li && name != uint16_t(-1)) {
                    auto resolved_text = win.text_data.instantiate_text(name).text_content.text;
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(li) {
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
            return S_OK;

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
                if(li && txt) {
                    auto resolved_text = txt->get_raw_text(win);
                    pRetVal->vt = VT_BSTR;
                    pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                }
                break;
            case UIA_BoundingRectanglePropertyId:
                if(li) {
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
            return S_OK;

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
    accessibility_object* win32_accessibility::make_plain_text_accessibility_interface(window_data& w, layout_interface* b, stored_text* t, bool is_content) {
        auto iunk = static_cast<disconnectable*>(new plain_text_provider(w, b, t, is_content));
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