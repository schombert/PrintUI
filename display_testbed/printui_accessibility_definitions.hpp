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
    struct win32_accessibility : public accessibility_framework_wrapper {
        window_data& win;
        iunk_ptr<root_window_provider> window_interface;

        win32_accessibility(window_data& win) : win(win) {
        }
        virtual ~win32_accessibility();
        virtual bool has_keyboard_preference();
        virtual void notify_window_state_change(resize_type r);
        virtual void notify_window_moved(int32_t x, int32_t y, int32_t width, int32_t height);
        virtual void notify_window_closed();
        virtual root_window_provider* get_root_window_provider();
        virtual void release_root_provider();
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
}