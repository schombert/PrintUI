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
    IUnknown* accessibility_object_to_iunk(accessibility_object* o) {
        IUnknown* p = (IUnknown*)o;
        return p;
    }

    root_window_provider::root_window_provider(window_data& win, HWND hwnd) : win(win), hwnd(hwnd), m_refCount(1) { }
    
    root_window_provider::~root_window_provider() {
        UiaReturnRawElementProvider(hwnd, 0, 0, nullptr);
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

    /*
    NavigateDirection_Parent = 0,
    NavigateDirection_NextSibling = 1,
    NavigateDirection_PreviousSibling = 2,
    NavigateDirection_FirstChild = 3,
    NavigateDirection_LastChild = 4
    */

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
        int rId[] = { UiaAppendRuntimeId, 0 };
        SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 2);
        if(psa == NULL) {
            return E_OUTOFMEMORY;
        }
        for(LONG i = 0; i < 2; i++) {
            SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
        }

        *pRetVal = psa;
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
        if(!win.window_interface->is_maximized()) {
            win.window_interface->move_window(screen_space_rect{ int32_t(x), int32_t(y), int32_t(win.ui_width), int32_t(win.ui_height) });
        }
        return S_OK;
    }
    IFACEMETHODIMP root_window_provider::Resize(double width, double height) {
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


    text_button_provider::text_button_provider(window_data& win, button_control_base& b) : win(win), button(b), m_refCount(1) {
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
                pRetVal->boolVal = button.is_disabled() ? VARIANT_FALSE : VARIANT_TRUE;
                break;
            case UIA_NamePropertyId:
            {
                auto resolved_text = button.get_raw_text(win);
                pRetVal->vt = VT_BSTR;
                pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                break;
            }
            case UIA_HelpTextPropertyId:
            {
                auto resolved_text = win.text_data.instantiate_text(button.get_alt_text()).text_content.text;
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
                auto client_rect = button.l_id != layout_reference_none ?
                    win.get_current_location(button.l_id) :
                    screen_space_rect{0,0,0,0};
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

        if(direction == NavigateDirection_FirstChild) {
            // no children
        } else if(direction == NavigateDirection_LastChild) {
            // no children
        } else if(direction == NavigateDirection_NextSibling) {
            auto sib_ptr = accessibility_object_to_iunk(win.get_next_sibling_accessibility_object(button.l_id));
            if(sib_ptr)
                return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        } else if(direction == NavigateDirection_PreviousSibling) {
            auto sib_ptr = accessibility_object_to_iunk(win.get_previous_sibling_accessibility_object(button.l_id));
            if(sib_ptr)
                return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        } else if(direction == NavigateDirection_Parent) {
            auto parent_ptr = accessibility_object_to_iunk(win.get_parent_accessibility_object(button.l_id));
            if(parent_ptr)
                return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        }
        return S_OK;
    }
    IFACEMETHODIMP text_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
        if(pRetVal == NULL) {
            return E_INVALIDARG;
        }
        size_t ptr_value = reinterpret_cast<size_t>(&button);
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
        auto client_rect = button.l_id != layout_reference_none ?
            win.get_current_location(button.l_id) :
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
        return win.accessibility_interface->get_root_window_provider()->get_FragmentRoot(pRetVal);
    }
    IFACEMETHODIMP text_button_provider::Invoke() {
        button.on_click(win, 0, 0);
        return S_OK;
    }



    text_list_button_provider::text_list_button_provider(window_data& win, button_control_base& b) : win(win), button(b), m_refCount(1) {
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
                pRetVal->boolVal = button.is_disabled() ? VARIANT_FALSE : VARIANT_TRUE;
                break;
            case UIA_SelectionItemIsSelectedPropertyId:
                pRetVal->vt = VT_BOOL;
                pRetVal->boolVal = button.is_selected() ? VARIANT_FALSE : VARIANT_TRUE;
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
                auto resolved_text = button.get_raw_text(win);
                pRetVal->vt = VT_BSTR;
                pRetVal->bstrVal = SysAllocString(resolved_text.c_str());
                break;
            }
            case UIA_HelpTextPropertyId:
            {
                auto resolved_text = win.text_data.instantiate_text(button.get_alt_text()).text_content.text;
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
                auto client_rect = button.l_id != layout_reference_none ?
                    win.get_current_location(button.l_id) :
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

        if(direction == NavigateDirection_FirstChild) {
            // no children
        } else if(direction == NavigateDirection_LastChild) {
            // no children
        } else if(direction == NavigateDirection_NextSibling) {
            auto sib_ptr = accessibility_object_to_iunk(win.get_next_sibling_accessibility_object(button.l_id));
            if(sib_ptr)
                return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        } else if(direction == NavigateDirection_PreviousSibling) {
            auto sib_ptr = accessibility_object_to_iunk(win.get_previous_sibling_accessibility_object(button.l_id));
            if(sib_ptr)
                return sib_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        } else if(direction == NavigateDirection_Parent) {
            auto parent_ptr = accessibility_object_to_iunk(win.get_parent_accessibility_object(button.l_id));
            if(parent_ptr)
                return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        }
        return S_OK;
    }
    IFACEMETHODIMP text_list_button_provider::GetRuntimeId(SAFEARRAY** pRetVal) {
        if(pRetVal == NULL) {
            return E_INVALIDARG;
        }
        size_t ptr_value = reinterpret_cast<size_t>(&button);
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
        auto client_rect = button.l_id != layout_reference_none ?
            win.get_current_location(button.l_id) :
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
        return win.accessibility_interface->get_root_window_provider()->get_FragmentRoot(pRetVal);
    }
    IFACEMETHODIMP text_list_button_provider::Select() {
        if(!button.is_selected())
            button.on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP text_list_button_provider::AddToSelection() {
        if(!button.is_selected())
            button.on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP text_list_button_provider::RemoveFromSelection() {
        if(button.is_selected())
            button.on_click(win, 0, 0);
        return S_OK;
    }
    IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_IsSelected(__RPC__out BOOL* pRetVal) {
        *pRetVal = button.is_selected() ? TRUE : FALSE;
        return S_OK;
    }
    IFACEMETHODIMP STDMETHODCALLTYPE text_list_button_provider::get_SelectionContainer(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal) {
        auto parent_ptr = accessibility_object_to_iunk(win.get_parent_accessibility_object(button.l_id));
        if(parent_ptr)
            return parent_ptr->QueryInterface(IID_PPV_ARGS(pRetVal));
        else
            return S_OK;
    }

	bool win32_accessibility::has_keyboard_preference() {
		BOOL out_value = FALSE;
		SystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &out_value, 0);
		return out_value == TRUE;
	}
    void win32_accessibility::notify_window_state_change(resize_type r) {
        if(window_interface && UiaClientsAreListening()) {
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
        if(window_interface && UiaClientsAreListening()) {
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
        if(window_interface && UiaClientsAreListening()) {
            UiaRaiseAutomationEvent(window_interface, UIA_Window_WindowClosedEventId);
        }
    }
	win32_accessibility::~win32_accessibility() {
		UiaDisconnectAllProviders();
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
    void win32_accessibility::release_root_provider() {
        window_interface = nullptr;
    }
    void release_accessibility_object(accessibility_object* o) {
        IUnknown* p = (IUnknown*)o;
        IRawElementProviderSimple* provider = nullptr;
        p->QueryInterface(IID_PPV_ARGS(&provider));
        if(provider) {
            UiaDisconnectProvider(provider);
            provider->Release();
        }
        p->Release();
    }


    accessibility_object* win32_accessibility::make_action_button_accessibility_interface(window_data& w, button_control_base& b) {
        auto iunk = static_cast<IUnknown*>(new text_button_provider(w, b));
        return (accessibility_object*)iunk;
    }
    accessibility_object* win32_accessibility::make_selection_button_accessibility_interface(window_data& w, button_control_base& b) {
        auto iunk = static_cast<IUnknown*>(new text_list_button_provider(w, b));
        return (accessibility_object*)iunk;
    }
    accessibility_object* win32_accessibility::make_icon_button_accessibility_interface(window_data& w, icon_button_base& b) {
        return nullptr;
    }
    accessibility_object* win32_accessibility::make_icon_toggle_button_accessibility_interface(window_data& w, icon_button_base& b) {
        return nullptr;
    }
    void win32_accessibility::on_invoke(accessibility_object* b) {
        auto iunk = accessibility_object_to_iunk(b);
        IRawElementProviderSimple* provider = nullptr;
        if(UiaClientsAreListening()) {
            iunk->QueryInterface(IID_PPV_ARGS(&provider));
            if(provider) {
                UiaRaiseAutomationEvent(provider, UIA_Invoke_InvokedEventId);
            }
        }
    }
    void win32_accessibility::on_enable_disable(accessibility_object* b, bool disabled) {
        if(UiaClientsAreListening()) {
            auto iunk = accessibility_object_to_iunk(b);
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
            }
        }
    }
    void win32_accessibility::on_select_unselect(accessibility_object* b, bool selection_state) {
        if(UiaClientsAreListening()) {
            auto iunk = accessibility_object_to_iunk(b);
            IRawElementProviderSimple* provider = nullptr;
            iunk->QueryInterface(IID_PPV_ARGS(&provider));
            if(provider) {
                if(selection_state) {
                    //UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementAddedToSelectionEventId);
                    UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementSelectedEventId);
                } else {
                    //UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementRemovedFromSelectionEventId);
                }
            }
        }
    }
    void win32_accessibility::on_change_name(accessibility_object* b, std::wstring const& new_name) {
        if(UiaClientsAreListening()) {
            auto iunk = accessibility_object_to_iunk(b);
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
            }
        }
    }
    void win32_accessibility::on_change_help_text(accessibility_object* b, std::wstring const& new_text) {
        if(UiaClientsAreListening()) {
            auto iunk = accessibility_object_to_iunk(b);
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
            }
        }
    }
}