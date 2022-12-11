#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_utility.hpp"
#include <algorithm>
#include <Windows.h>
#include <ole2.h>
#include <UIAutomation.h>

namespace printui {
    template<typename I>
    class iunk_ptr {
        I* ptr = nullptr;
    public:
        iunk_ptr() noexcept {
        }
        iunk_ptr(I* ptr) noexcept : ptr(ptr) {
        }
        iunk_ptr(iunk_ptr<I> const& o) noexcept {
            ptr = o.ptr;
            if(ptr)
                ptr->AddRef();
        }
        iunk_ptr(iunk_ptr<I>&& o) noexcept {
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        ~iunk_ptr() {
            if(ptr)
                ptr->Release();
            ptr = nullptr;
        }
        iunk_ptr<I>& operator=(iunk_ptr<I> const& o) noexcept {
            if(ptr)
                ptr->Release();
            ptr = o.ptr;
            if(ptr)
                ptr->AddRef();
            return *this;
        }
        iunk_ptr<I>& operator=(iunk_ptr<I>&& o) noexcept {
            if(ptr)
                ptr->Release();
            ptr = o.ptr;
            o.ptr = nullptr;
            return *this;
        }
        I* operator->() const noexcept {
            return ptr;
        }
        operator bool() const noexcept {
            return ptr != nullptr;
        }
        operator I* () const noexcept {
            return ptr;
        }
        template<typename T>
        [[nodiscard]] iunk_ptr<T> query_interface() const noexcept {
            T* temp = nullptr;
            ptr->QueryInterface(__uuidof(T), reinterpret_cast<void**>(&temp));
            return temp;
        }
    };
    template<typename I, typename ... P>
    [[nodiscard]] iunk_ptr<I> make_iunk(P&&...p) {
        return iunk_ptr<I>(new I(std::forward<P>(p)...));
    }

    struct win32_accessibility : public accessibility_framework_wrapper {
        window_data& win;
        iunk_ptr<root_window_provider> window_interface;

        win32_accessibility(window_data& win) : win(win) {
        }
        virtual ~win32_accessibility();
        virtual bool has_keyboard_preference() override;
        virtual void notify_window_state_change(resize_type r) override;
        virtual void notify_window_moved(int32_t x, int32_t y, int32_t width, int32_t height) override;
        virtual void notify_window_closed() override;
        virtual root_window_provider* get_root_window_provider() override;
        virtual root_window_provider* peek_root_window_provider() override;
        virtual void release_root_provider() override;
        virtual accessibility_object* make_action_button_accessibility_interface(window_data& w, button_control_base& b) override;
        virtual accessibility_object* make_selection_button_accessibility_interface(window_data& w, button_control_base& b) override;
        virtual accessibility_object* make_toggle_button_accessibility_interface(window_data& w, button_control_toggle& b) override;
        virtual accessibility_object* make_icon_button_accessibility_interface(window_data& w, icon_button_base& b) override;
        virtual accessibility_object* make_icon_toggle_button_accessibility_interface(window_data& w, icon_button_base& b) override;
        virtual accessibility_object* make_open_list_control_accessibility_interface(window_data& w, open_list_control& b) override;
        virtual accessibility_object* make_container_accessibility_interface(window_data& w, layout_interface* b, uint16_t name) override;
        virtual accessibility_object* make_plain_text_accessibility_interface(window_data& w, layout_interface* b, stored_text* t, bool is_content) override;
        virtual accessibility_object* make_simple_text_accessibility_interface(window_data& w, simple_editable_text& control) override;
        virtual accessibility_object* make_expandable_selection_list(window_data& win, generic_expandable* control, generic_selection_container* sc, uint16_t name, uint16_t alt_text) override;
        virtual accessibility_object* make_expandable_container(window_data& win, generic_expandable* control, uint16_t name, uint16_t alt_text) override;
        virtual void on_invoke(accessibility_object* b) override;
        virtual void on_enable_disable(accessibility_object* b, bool disabled) override;
        virtual void on_select_unselect(accessibility_object* b, bool selection_state) override;
        virtual void on_change_name(accessibility_object* b, std::wstring const& new_name) override;
        virtual void on_change_help_text(accessibility_object* b, std::wstring const& new_text) override;
        virtual void on_toggle_change(accessibility_object* b) override;
        virtual void on_selection_change(accessibility_object* b) override;
        virtual void on_contents_changed(accessibility_object* b) override;
        virtual void on_expand_collapse(accessibility_object* b, bool expanded) override;
        virtual void on_window_layout_changed() override;
        virtual void on_text_content_changed(accessibility_object* b) override;
        virtual void on_text_value_changed(accessibility_object* b) override;
        virtual void on_text_selection_changed(accessibility_object* b) override;
        virtual void on_conversion_target_changed(accessibility_object* b) override;
        virtual void on_composition_change(accessibility_object* b, std::wstring_view comp) override;
        virtual void on_composition_result(accessibility_object* b, std::wstring_view result) override;
        virtual void on_focus_change(accessibility_object* b) override;
        virtual void on_focus_returned_to_root() override;
    };

    class root_window_provider : public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IRawElementProviderFragmentRoot,
        public ITransformProvider,
        public IWindowProvider {
    public:
        root_window_provider(window_data& win, HWND hwnd);

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        // IRawElementProviderFragmenRoot methods
        IFACEMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetFocus(IRawElementProviderFragment** pRetVal);

        //ITransformProvicer
        IFACEMETHODIMP Move(double x, double y);
        IFACEMETHODIMP Resize(double width, double height);
        IFACEMETHODIMP Rotate(double degrees);
        IFACEMETHODIMP get_CanMove(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_CanResize(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_CanRotate(__RPC__out BOOL* pRetVal);

        //IWindowProvider
        IFACEMETHODIMP SetVisualState(WindowVisualState state);
        IFACEMETHODIMP Close();
        IFACEMETHODIMP WaitForInputIdle(int milliseconds, __RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_CanMaximize(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_CanMinimize(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_IsModal(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_WindowVisualState(__RPC__out enum WindowVisualState* pRetVal);
        IFACEMETHODIMP get_WindowInteractionState(__RPC__out enum WindowInteractionState* pRetVal);
        IFACEMETHODIMP get_IsTopmost(__RPC__out BOOL* pRetVal);

        virtual ~root_window_provider();

        HWND hwnd;
    private:
        window_data& win;
        
        // Ref counter for this COM object.
        ULONG m_refCount;
    };


    struct disconnectable : public IUnknown {
        window_data& win;

        disconnectable(window_data& win) : win(win) { }
        
        virtual void disconnect() = 0;
        virtual ~disconnectable() { }
    };

    class text_button_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IInvokeProvider {
    public:

        text_button_provider(window_data& win, button_control_base& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //IInvokeProvider
        IFACEMETHODIMP Invoke();

        virtual ~text_button_provider();
    private:
        button_control_base* button = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };


    class text_list_button_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public ISelectionItemProvider {
    public:

        text_list_button_provider(window_data& win, button_control_base& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //ISelectionItemProvider
        IFACEMETHODIMP Select();
        IFACEMETHODIMP AddToSelection();
        IFACEMETHODIMP RemoveFromSelection();
        IFACEMETHODIMP STDMETHODCALLTYPE get_IsSelected(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP STDMETHODCALLTYPE get_SelectionContainer(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal);

        virtual ~text_list_button_provider();
    private:
        button_control_base* button = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class icon_button_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IInvokeProvider {
    public:

        icon_button_provider(window_data& win, icon_button_base& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //IInvokeProvider
        IFACEMETHODIMP Invoke();

        virtual ~icon_button_provider();
    private:
        icon_button_base* button = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class icon_toggle_button_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IToggleProvider {
    public:

        icon_toggle_button_provider(window_data& win, icon_button_base& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //IToggleProvider
        IFACEMETHODIMP Toggle();
        IFACEMETHODIMP get_ToggleState(__RPC__out enum ToggleState* pRetVal);

        virtual ~icon_toggle_button_provider();
    private:
        icon_button_base* button = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class text_toggle_button_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IToggleProvider {
    public:

        text_toggle_button_provider(window_data& win, button_control_toggle& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //IToggleProvider
        IFACEMETHODIMP Toggle();
        IFACEMETHODIMP get_ToggleState(__RPC__out enum ToggleState* pRetVal);

        virtual ~text_toggle_button_provider();
    private:
        button_control_toggle* button = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class open_list_control_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public ISelectionProvider {
    public:

        open_list_control_provider(window_data& win, open_list_control& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //ISelectionProvider
        IFACEMETHODIMP GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_CanSelectMultiple(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_IsSelectionRequired(__RPC__out BOOL* pRetVal);

        virtual ~open_list_control_provider();
    private:
        open_list_control* control = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class container_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment {
    public:

        container_provider(window_data& win, layout_interface* li, uint16_t name);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        virtual ~container_provider();
    private:
        layout_interface* li = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
        uint16_t name;
    };

    class plain_text_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment {
    public:

        plain_text_provider(window_data& win, layout_interface* b, stored_text* t, bool is_content);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        virtual ~plain_text_provider();
    private:
        layout_interface* li = nullptr;
        stored_text* txt = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;
        bool is_content = true;
    };

    class expandable_selection_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public ISelectionProvider,
        public IExpandCollapseProvider {
    public:

        expandable_selection_provider(window_data& win, generic_expandable* control, generic_selection_container* sc, uint16_t name, uint16_t alt_text);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //ISelectionProvider
        IFACEMETHODIMP GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_CanSelectMultiple(__RPC__out BOOL* pRetVal);
        IFACEMETHODIMP get_IsSelectionRequired(__RPC__out BOOL* pRetVal);

        //IExpandCollapseProvider
        IFACEMETHODIMP Expand();
        IFACEMETHODIMP Collapse() ;
        IFACEMETHODIMP get_ExpandCollapseState(__RPC__out enum ExpandCollapseState* pRetVal);

        virtual ~expandable_selection_provider();
    private:
        generic_expandable* control = nullptr;
        generic_selection_container* sc = nullptr;
        uint16_t name = uint16_t(-1);
        uint16_t alt_text = uint16_t(-1);

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class expandable_container : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IExpandCollapseProvider {
    public:

        expandable_container(window_data& win, generic_expandable* control, uint16_t name, uint16_t alt_text);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //IExpandCollapseProvider
        IFACEMETHODIMP Expand();
        IFACEMETHODIMP Collapse();
        IFACEMETHODIMP get_ExpandCollapseState(__RPC__out enum ExpandCollapseState* pRetVal);

        virtual ~expandable_container();
    private:
        generic_expandable* control = nullptr;
        uint16_t name = uint16_t(-1);
        uint16_t alt_text = uint16_t(-1);

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

    class simple_edit_provider;

    interface __declspec(uuid("A401D798-DCFD-42F2-8A00-140282D987D1"))
    IRawRangeValues : public IUnknown {
    public:
        virtual int32_t get_start() = 0;
        virtual int32_t get_end() = 0;
    };

    class simple_edit_range_provider :
        public ITextRangeProvider, public IRawRangeValues {

        simple_edit_range_provider(simple_edit_provider* parent, int32_t start, int32_t end);

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        //ITextRangeProvider
        IFACEMETHODIMP Clone(__RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP Compare(__RPC__in_opt ITextRangeProvider* range, __RPC__out BOOL* pRetVal);
        IFACEMETHODIMP CompareEndpoints(enum TextPatternRangeEndpoint endpoint, __RPC__in_opt ITextRangeProvider* targetRange, enum TextPatternRangeEndpoint targetEndpoint, __RPC__out int* pRetVal);
        IFACEMETHODIMP ExpandToEnclosingUnit(enum TextUnit unit);
        IFACEMETHODIMP FindAttribute(TEXTATTRIBUTEID attributeId, VARIANT val, BOOL backward, __RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP FindText(__RPC__in BSTR text, BOOL backward, BOOL ignoreCase, __RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP GetAttributeValue(TEXTATTRIBUTEID attributeId, __RPC__out VARIANT* pRetVal);
        IFACEMETHODIMP GetBoundingRectangles(__RPC__deref_out_opt SAFEARRAY** pRetVal);
        IFACEMETHODIMP GetEnclosingElement(__RPC__deref_out_opt IRawElementProviderSimple** pRetVal);
        IFACEMETHODIMP GetText(int maxLength, __RPC__deref_out_opt BSTR* pRetVal);
        IFACEMETHODIMP Move(enum TextUnit unit, int count, __RPC__out int* pRetVal);
        IFACEMETHODIMP MoveEndpointByUnit(enum TextPatternRangeEndpoint endpoint, enum TextUnit unit, int count, __RPC__out int* pRetVal);
        IFACEMETHODIMP MoveEndpointByRange(enum TextPatternRangeEndpoint endpoint, __RPC__in_opt ITextRangeProvider* targetRange, enum TextPatternRangeEndpoint targetEndpoint);
        IFACEMETHODIMP Select();
        IFACEMETHODIMP AddToSelection();
        IFACEMETHODIMP RemoveFromSelection();
        IFACEMETHODIMP ScrollIntoView(BOOL alignToTop);
        IFACEMETHODIMP GetChildren(__RPC__deref_out_opt SAFEARRAY** pRetVal);

        // IRawRangeValues methods
        int32_t get_start();
        int32_t get_end();

        virtual ~simple_edit_range_provider();
    private:
        simple_edit_provider* parent = nullptr;
        int32_t start = 0;
        int32_t end = 0;

        // Ref counter for this COM object.
        ULONG m_refCount;

        friend class simple_edit_provider;
    };

    class simple_edit_provider : public disconnectable,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public ITextEditProvider,
        public IValueProvider {
    public:

        simple_edit_provider(window_data& win, simple_editable_text& b);
        void disconnect();

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

        // IRawElementProviderSimple methods
        IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
        IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
        IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
        IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

        // IRawElementProviderFragment methods
        IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
        IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
        IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
        IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
        IFACEMETHODIMP SetFocus();
        IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

        //ITextProvider
        IFACEMETHODIMP GetSelection(__RPC__deref_out_opt SAFEARRAY** pRetVal);
        IFACEMETHODIMP GetVisibleRanges(__RPC__deref_out_opt SAFEARRAY** pRetVal);
        IFACEMETHODIMP RangeFromChild(__RPC__in_opt IRawElementProviderSimple* childElement, __RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP RangeFromPoint(UiaPoint point, __RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP get_DocumentRange(__RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP get_SupportedTextSelection(__RPC__out enum SupportedTextSelection* pRetVal);

        //ITextEditProvider
        IFACEMETHODIMP GetActiveComposition(__RPC__deref_out_opt ITextRangeProvider** pRetVal);
        IFACEMETHODIMP GetConversionTarget(__RPC__deref_out_opt ITextRangeProvider** pRetVal);

        //IValueProvider
        IFACEMETHODIMP SetValue(__RPC__in LPCWSTR val);
        IFACEMETHODIMP get_Value(__RPC__deref_out_opt BSTR* pRetVal);
        IFACEMETHODIMP get_IsReadOnly(__RPC__out BOOL* pRetVal);

        virtual ~simple_edit_provider();
    private:
        std::vector<simple_edit_range_provider*> child_ranges;
        simple_editable_text* control = nullptr;

        // Ref counter for this COM object.
        ULONG m_refCount;

        friend class simple_edit_range_provider;
    };
}