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
        virtual void release_root_provider() override;
        virtual accessibility_object* make_action_button_accessibility_interface(window_data& w, button_control_base& b) override;
        virtual accessibility_object* make_selection_button_accessibility_interface(window_data& w, button_control_base& b) override;
        virtual accessibility_object* make_icon_button_accessibility_interface(window_data& w, icon_button_base& b) override;
        virtual accessibility_object* make_icon_toggle_button_accessibility_interface(window_data& w, icon_button_base& b) override;
        virtual void on_invoke(accessibility_object* b) override;
        virtual void on_enable_disable(accessibility_object* b, bool disabled) override;
        virtual void on_select_unselect(accessibility_object* b, bool selection_state) override;
        virtual void on_change_name(accessibility_object* b, std::wstring const& new_name) override;
        virtual void on_change_help_text(accessibility_object* b, std::wstring const& new_text) override;
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
    private:
        window_data& win;
        HWND hwnd;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

#pragma warning( push )
#pragma warning( disable : 4584 )

    class text_button_provider : public IUnknown,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public IInvokeProvider {
    public:

        text_button_provider(window_data& win, button_control_base& b);

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
        window_data& win;
        button_control_base& button;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };


    class text_list_button_provider : public IUnknown,
        public IRawElementProviderSimple,
        public IRawElementProviderFragment,
        public ISelectionItemProvider {
    public:

        text_list_button_provider(window_data& win, button_control_base& b);

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
        window_data& win;
        button_control_base& button;

        // Ref counter for this COM object.
        ULONG m_refCount;
    };

#pragma warning( pop )
}