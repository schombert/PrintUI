#include "display_testbed.hpp"
#include <Windowsx.h>
#include <shlwapi.h>
#include <cmath>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <dxgi1_2.h>
#include <d3d11_3.h>
#include <DirectXMath.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <algorithm>

void main_window::load_default_dynamic_settings() {
	dynamic_settings.window_border = 3;
	dynamic_settings.layout_base_size = 22;
	dynamic_settings.brushes.push_back(printui::brush{ L"yellowpaper.png", printui::brush_color{0.95f,0.95f,0.95f}, true });
	dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.0f,0.0f,0.0f}, false });
	
	dynamic_settings.text_directory = L"locale";
	dynamic_settings.primary_font.name = L"Libre Caslon Text";
	dynamic_settings.primary_font.type = printui::font_type::roman;
	dynamic_settings.primary_font.top_leading = 0;
	dynamic_settings.primary_font.bottom_leading = 1;

	dynamic_settings.small_font.name = L"NewComputerModern10";
	dynamic_settings.small_font.type = printui::font_type::roman;

	// define shared animation effects
	printui::list_control::list_appearance = printui::animation_defintion{printui::animation_type::fade, 0.25f, true};
	printui::list_control::list_disappearance = printui::animation_defintion{ printui::animation_type::fade, 0.25f, false };

	printui::page_footer::footer_appearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, true };
	printui::page_footer::footer_disappearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, false };

	// pre assign fixed text indices
	text_data.register_name("ui_settings_name", text_id::ui_settings_name);
	text_data.register_name("settings_header", text_id::settings_header);

	text_data.register_name("orientation_label", text_id::orientation_label);
	text_data.register_name("orientation_ltr", text_id::orientation_ltr);
	text_data.register_name("orientation_rtl", text_id::orientation_rtl);
	text_data.register_name("orientation_vltr", text_id::orientation_vltr);
	text_data.register_name("orientation_vrtl", text_id::orientation_vrtl);

	text_data.register_name("input_mode_label", text_id::input_mode_label);
	text_data.register_name("input_mode_keyboard_only", text_id::input_mode_keyboard_only);
	text_data.register_name("input_mode_mouse_only", text_id::input_mode_mouse_only);
	text_data.register_name("input_mode_controller_only", text_id::input_mode_controller_only);
	text_data.register_name("input_mode_controller_with_pointer", text_id::input_mode_controller_with_pointer);
	text_data.register_name("input_mode_mouse_and_keyboard", text_id::input_mode_mouse_and_keyboard);
	text_data.register_name("input_mode_follow_input", text_id::input_mode_follow_input);

	text_data.register_name("language_label", text_id::language_label);

	text_data.register_name("page_fraction", text_id::page_fraction);

	text_data.register_name("minimize_info", text_id::minimize_info);
	text_data.register_name("maximize_info", text_id::maximize_info);
	text_data.register_name("restore_info", text_id::restore_info);
	text_data.register_name("settings_info", text_id::settings_info);
	text_data.register_name("info_info", text_id::info_info);
	text_data.register_name("close_info", text_id::close_info);
}

int WINAPI wWinMain(
	HINSTANCE /*hInstance*/,
	HINSTANCE /*hPrevInstance*/,
	LPWSTR /*lpCmdLine*/,
	int /*nCmdShow*/
) {
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if(SUCCEEDED(CoInitialize(NULL))) {
		{
			main_window app;

			app.create_window();
			app.message_loop();
		}
		CoUninitialize();
	}
	return 0;
}

/*
static const WCHAR sc_helloWorld[] = L"Hello, World!";

namespace printui {

main_window::main_window() :
	win(true, true, true),
	wic_factory(nullptr),
	font_collection(nullptr),
	font_fallbacks(nullptr),
	d3d_device(nullptr),
	dxgi_device(nullptr),
	d2d_device(nullptr),
	d3d_device_context(nullptr),
	d2d_device_context(nullptr),
	swap_chain(nullptr),
	back_buffer_target(nullptr),
	plain_strokes(nullptr),
	rendered_close_icon(nullptr) {

	for(int32_t i = 0; i < palette_count; ++i) {
		palette[i] = nullptr;
	}

}


main_window::~main_window() {
	safe_release(win.d2d_factory);
	safe_release(wic_factory);
	safe_release(win.dwrite_factory);
	safe_release(font_collection);
	safe_release(font_fallbacks);
	safe_release(win.common_text_format);
	safe_release(win.small_text_format);
	safe_release(d3d_device);
	safe_release(dxgi_device);
	safe_release(d2d_device);
	safe_release(d3d_device_context);
	safe_release(d2d_device_context);
	if(swap_chain)
		swap_chain->SetFullscreenState(FALSE, nullptr);
	safe_release(swap_chain);
	safe_release(back_buffer_target);
	safe_release(plain_strokes);
	safe_release(rendered_close_icon);

	release_panes();
	release_palette();
}


void main_window::create_window() {
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	win.icon_directory = L"icons";
	win.window_border = 3;

	create_persistent_resources();

	if(win.dynamic_settings.imode == input_mode::system_default) {
		win.dynamic_settings.imode = printui::get_system_default_input_mode();
	}

	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_OWNDC;
	wcex.lpfnWndProc = main_window::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(LONG_PTR);
	wcex.hInstance = GetModuleHandle(nullptr);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.hCursor = nullptr;
	wcex.lpszClassName = L"display_testbed_class";

	if(RegisterClassEx(&wcex) == 0) {
		std::abort();
	}

	win.m_hwnd = CreateWindowEx(
		0,
		L"display_testbed_class",
		L"display testbed",
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		NULL,
		NULL,
		GetModuleHandle(nullptr),
		this
	);

	if(win.m_hwnd) {

		if(win.dynamic_settings.imode == printui::input_mode::mouse_and_keyboard || win.dynamic_settings.imode == printui::input_mode::mouse_only || win.dynamic_settings.imode == printui::input_mode::follow_input) {
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			win.cursor_visible = true;
		}

		win.dpi = float(GetDpiForWindow((HWND)(win.m_hwnd)));

		SetWindowPos(
			(HWND)(win.m_hwnd),
			NULL,
			NULL,
			NULL,
			static_cast<int>(ceil(640.f * win.dpi / 96.f)),
			static_cast<int>(ceil(480.f * win.dpi / 96.f)),
			SWP_NOMOVE | SWP_FRAMECHANGED);
		ShowWindow((HWND)(win.m_hwnd), SW_SHOWNORMAL);
		UpdateWindow((HWND)(win.m_hwnd));
	}
}


void main_window::create_persistent_resources() {
	HRESULT hr;

	// Create D2D factory
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &win.d2d_factory);
	if(SUCCEEDED(hr)) {
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void**>(&wic_factory)
		);
	}
	if(SUCCEEDED(hr)) {
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(win.dwrite_factory),
			reinterpret_cast<IUnknown**>(&win.dwrite_factory)
		);
	}
	if(SUCCEEDED(hr)) {
		D2D1_STROKE_STYLE_PROPERTIES style_prop;
		style_prop.startCap = D2D1_CAP_STYLE_FLAT;
		style_prop.endCap = D2D1_CAP_STYLE_FLAT;
		style_prop.dashCap = D2D1_CAP_STYLE_FLAT;
		style_prop.lineJoin = D2D1_LINE_JOIN_MITER;
		style_prop.miterLimit = 10.0f;
		style_prop.dashStyle = D2D1_DASH_STYLE_SOLID;
		style_prop.dashOffset = 0.0f;
		hr = win.d2d_factory->CreateStrokeStyle(style_prop, nullptr, 0, &plain_strokes);
	}
	if(SUCCEEDED(hr)) {
		IDWriteFontSetBuilder2* bldr = nullptr;
		hr = win.dwrite_factory->CreateFontSetBuilder(&bldr);
		if(!SUCCEEDED(hr)) std::abort();

		if(SUCCEEDED(hr)) {
			hr = bldr->AddFontFile(L"E:\\VSProjects\\PrintUI\\NewCM10-Regular.otf");
		}
		if(!SUCCEEDED(hr)) std::abort();
		if(SUCCEEDED(hr)) {
			hr = bldr->AddFontFile(L"E:\\VSProjects\\PrintUI\\UKIJTuzB.ttf");
		}
		if(!SUCCEEDED(hr)) std::abort();

		IDWriteFontSet* sysfs = nullptr;
		win.dwrite_factory->GetSystemFontSet(&sysfs);
		if(sysfs) bldr->AddFontSet(sysfs);

		IDWriteFontSet* fs = nullptr;
		if(SUCCEEDED(hr)) {
			hr = bldr->CreateFontSet(&fs);
		}
		if(!SUCCEEDED(hr)) std::abort();
		if(SUCCEEDED(hr)) {
			hr = win.dwrite_factory->CreateFontCollectionFromFontSet(fs, DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC, &font_collection);
		}
		if(!SUCCEEDED(hr)) std::abort();

		

		safe_release(sysfs);
		safe_release(fs);
		safe_release(bldr);
	}

	printui::create_font_collection(win, L"fonts");

	if(SUCCEEDED(hr)) {
		IDWriteFontFallbackBuilder* bldr = nullptr;
		hr = win.dwrite_factory->CreateFontFallbackBuilder(&bldr);
		if(!SUCCEEDED(hr)) std::abort();
		{
			WCHAR const* fn[] = { L"UKIJ Tuz Basma" };
			*//*
			Arabic (0600–06FF, 256 characters)
			Arabic Supplement (0750–077F, 48 characters)
			Arabic Extended-B (0870–089F, 41 characters)
			Arabic Extended-A (08A0–08FF, 96 characters)
			Arabic Presentation Forms-A (FB50–FDFF, 631 characters)
			Arabic Presentation Forms-B (FE70–FEFF, 141 characters)
			Rumi Numeral Symbols (10E60–10E7F, 31 characters)
			Arabic Extended-C (10EC0-10EFF, 3 characters)
			Indic Siyaq Numbers (1EC70–1ECBF, 68 characters)
			Ottoman Siyaq Numbers (1ED00–1ED4F, 61 characters)
			Arabic Mathematical Alphabetic Symbols (1EE00–1EEFF, 143 characters)
			*//*
			DWRITE_UNICODE_RANGE ranges[] = {
				DWRITE_UNICODE_RANGE{ 0x0600, 0x06FF },
				DWRITE_UNICODE_RANGE{ 0x0750, 0x077F } ,
				DWRITE_UNICODE_RANGE{ 0x0870, 0x089F } ,
				DWRITE_UNICODE_RANGE{ 0x08A0, 0x08FF } ,
				DWRITE_UNICODE_RANGE{ 0xFB50, 0xFDFF } ,
				DWRITE_UNICODE_RANGE{ 0xFE70, 0xFEFF } ,
				DWRITE_UNICODE_RANGE{ 0x10E60, 0x10E7F } ,
				DWRITE_UNICODE_RANGE{ 0x10EC0, 0x10EFF } ,
				DWRITE_UNICODE_RANGE{ 0x1EC70, 0x1ECBF } ,
				DWRITE_UNICODE_RANGE{ 0x1ED00, 0x1ED4F },
				DWRITE_UNICODE_RANGE{ 0x1EE00, 0x1EEFF } };

			if(SUCCEEDED(hr)) {
				hr = bldr->AddMapping(ranges, 11, fn, 1, font_collection, nullptr, nullptr, 1.0f);
			}
			if(!SUCCEEDED(hr)) std::abort();
		}
		{
			WCHAR const* fn[] = { L"NewComputerModern10" };
			DWRITE_UNICODE_RANGE ranges[] = {
				DWRITE_UNICODE_RANGE{ 0x0000, 0x00FF }
			};

			if(SUCCEEDED(hr)) {
				hr = bldr->AddMapping(ranges, 1, fn, 1, font_collection, nullptr, nullptr, 1.0f);
			}
			if(!SUCCEEDED(hr)) std::abort();
		}
		IDWriteFontFallback* system_fallback;
		win.dwrite_factory->GetSystemFontFallback(&system_fallback);
		bldr->AddMappings(system_fallback);
		safe_release(system_fallback);

		if(SUCCEEDED(hr)) {
			hr = bldr->CreateFontFallback(&font_fallbacks);
		}
		if(!SUCCEEDED(hr)) std::abort();

		safe_release(bldr);
	}

	if(SUCCEEDED(hr)) { //L"en-US"
		{
			IDWriteFont3* f = nullptr;

			IDWriteFontList2* fl;
			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, DWRITE_FONT_WEIGHT_NORMAL},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, 100 } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, 1.0f } };

			font_collection->GetMatchingFonts(L"NewComputerModern10", fax, 3, &fl);
			fl->GetFont(0, &f);
			safe_release(fl);

			win.text_alignment = get_text_alignment(28.0f, f, f);

			safe_release(f);
		}

		DWRITE_FONT_AXIS_VALUE fax[] = {
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, DWRITE_FONT_WEIGHT_NORMAL},
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, 100 } ,
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, 1.0f } };

		hr = win.dwrite_factory->CreateTextFormat(L"NewComputerModern10", font_collection, fax, 3, win.text_alignment.big_font_size, L"", &win.common_text_format);
		if(!SUCCEEDED(hr)) std::abort();

		if(SUCCEEDED(hr)) {
			hr = win.common_text_format->SetFontFallback(font_fallbacks);
		}
		win.common_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		if(!SUCCEEDED(hr)) std::abort();
		win.common_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.text_alignment.big_line_spacing, win.text_alignment.big_baseline);

		win.dwrite_factory->CreateTextFormat(L"NewComputerModern10", font_collection, nullptr, 0, win.text_alignment.small_font_size, L"en", &win.small_text_format);
		win.small_text_format->SetFontFallback(font_fallbacks);
		win.small_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		win.small_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.text_alignment.small_line_spacing, win.text_alignment.small_baseline);
	}
}

void main_window::create_device_resources() {
	HRESULT hr = S_OK;
	
	if(!d3d_device) {
		IDXGIAdapter* pAdapter = nullptr;
		IDXGIFactory2* pDXGIFactory = nullptr;

		RECT rcClient;
		GetClientRect((HWND)(win.m_hwnd), &rcClient);

		auto nWidth = abs(rcClient.right - rcClient.left);
		auto nHeight = abs(rcClient.bottom - rcClient.top);

		D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			levels, 4,
			D3D11_SDK_VERSION,
			&d3d_device,
			nullptr,
			&d3d_device_context
		);

		if(FAILED(hr)) {
			hr = D3D11CreateDevice(nullptr,
				D3D_DRIVER_TYPE_WARP,
				nullptr,
				D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
				levels, 4,
				D3D11_SDK_VERSION,
				&d3d_device,
				nullptr,
				&d3d_device_context
			);
		}

		if(SUCCEEDED(hr)) {
			hr = d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
		}
		if(SUCCEEDED(hr)) {
			hr = win.d2d_factory->CreateDevice(dxgi_device, &d2d_device);
		}
		if(SUCCEEDED(hr)) {
			hr = d2d_device->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				&d2d_device_context
			);
		}
		if(SUCCEEDED(hr)) {
			hr = d3d_device->QueryInterface(&dxgi_device);
		}
		if(SUCCEEDED(hr)) {
			hr = dxgi_device->GetAdapter(&pAdapter);
		}
		if(SUCCEEDED(hr)) {
			hr = pAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
		}
		if(SUCCEEDED(hr)) {
			DXGI_SWAP_CHAIN_DESC1  swapDesc;
			::ZeroMemory(&swapDesc, sizeof(swapDesc));

			swapDesc.Width = nWidth;
			swapDesc.Height = nHeight;
			swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swapDesc.Stereo = FALSE;
			swapDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };
			swapDesc.SampleDesc.Count = 1;
			swapDesc.SampleDesc.Quality = 0;
			swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapDesc.BufferCount = 2;
			swapDesc.Scaling = DXGI_SCALING_NONE;
			swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapDesc.Flags = 0;


			hr = pDXGIFactory->CreateSwapChainForHwnd(d3d_device, (HWND)(win.m_hwnd), &swapDesc, nullptr, nullptr, &swap_chain);
		}
		if(SUCCEEDED(hr)) {
			hr = dxgi_device->SetMaximumFrameLatency(1);
		}

		IStream* fstream = nullptr;
		ID2D1SvgDocument* doc = nullptr;

		if(SUCCEEDED(hr)) {
			hr = SHCreateStreamOnFileW(
				L"E:\\VSProjects\\PrintUI\\close.svg", STGM_READ | STGM_SHARE_DENY_WRITE, &fstream);
		}
		if(SUCCEEDED(hr)) {
			hr = d2d_device_context->CreateSvgDocument(fstream, D2D1_SIZE_F{ 50, 50 }, &doc);
		}
		if(SUCCEEDED(hr)) {
			hr = d2d_device_context->CreateBitmap(D2D1_SIZE_U{ 50, 50 }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(
						DXGI_FORMAT_A8_UNORM,
						D2D1_ALPHA_MODE_PREMULTIPLIED),
					win.dpi, win.dpi,
					D2D1_BITMAP_OPTIONS_TARGET , nullptr },
				&rendered_close_icon);
		}
		if(SUCCEEDED(hr)) {
			d2d_device_context->SetTarget(rendered_close_icon);
			d2d_device_context->BeginDraw();
			d2d_device_context->DrawSvgDocument(doc);
			hr = d2d_device_context->EndDraw();
		}

		safe_release(fstream);
		safe_release(doc);

		win.layout.init_layout_graphics(d2d_device_context);
		//settings_icon.redraw_image(win, d2d_device_context);
		win.layout.set_window_title(L"Window Title");

		create_palette();

		if(SUCCEEDED(hr)) {
			create_window_size_resources(nWidth, nHeight);
		} else {
			MessageBox((HWND)(win.m_hwnd), L"Could not create direct X devices, exiting", L"Fatal Error", MB_OK | MB_ICONERROR);
			std::terminate();
		}
		
		safe_release(pAdapter);
		safe_release(pDXGIFactory);
	}
}

void main_window::create_window_size_resources(uint32_t nWidth, uint32_t nHeight) {
	HRESULT hr = S_OK;
	IDXGISurface* back_buffer = nullptr;

	if(win.ui_width == nWidth && win.ui_height == nHeight) return;

	win.ui_width = nWidth;
	win.ui_height = nHeight;
	win.dpi = float(GetDpiForWindow((HWND)(win.m_hwnd)));

	win.layout_width = win.ui_width / win.layout_size;
	win.layout_height = win.ui_height / win.layout_size;

	win.layout.recreate_layout();

	if(!swap_chain)
		return;

	safe_release(back_buffer_target);
	release_panes();

	// Resize render target buffers
	d2d_device_context->SetTarget(nullptr);
	hr = swap_chain->ResizeBuffers(0, nWidth, nHeight, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	if(SUCCEEDED(hr)) {
		hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	}
	if(SUCCEEDED(hr)) {
		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
				win.dpi,
				win.dpi
			);
		hr = d2d_device_context->CreateBitmapFromDxgiSurface(
			back_buffer,
			&bitmapProperties,
			&back_buffer_target
		);
	}

	if(SUCCEEDED(hr)) {
		create_panes();
	}

	safe_release(back_buffer);

	if(!back_buffer_target)
		std::abort();

}

void main_window::create_panes() {
	d2d_device_context->CreateBitmap(D2D1_SIZE_U{ win.ui_width, win.ui_height }, nullptr, 0,
		D2D1_BITMAP_PROPERTIES1{
			D2D1::PixelFormat(
				DXGI_FORMAT_A8_UNORM,
				D2D1_ALPHA_MODE_PREMULTIPLIED),
			win.dpi, win.dpi,
			D2D1_BITMAP_OPTIONS_TARGET , nullptr },
		&win.foreground);

	redraw_ui_completely();
}

void main_window::create_palette() {

	d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &win.dummy_brush);

	create_highlight_brushes(win, d2d_device_context);
	{
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&win.dummy_brush
		);
	}

	win.dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.0f,0.0f,0.0f}, false});
	win.dynamic_settings.brushes.push_back(printui::brush{ L"E:\\VSProjects\\PrintUI\\gold2.png", printui::brush_color{0.0f,0.0f,0.0f}, true });
	win.dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{1.0f,1.0f,1.0f}, true });
	win.dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{D2D1::ColorF(D2D1::ColorF::Crimson).r,D2D1::ColorF(D2D1::ColorF::Crimson).g,D2D1::ColorF(D2D1::ColorF::Crimson).b}, true });

	{
		ID2D1SolidColorBrush* t;
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			&t
		);
		palette[2] = t;
	}
	{
		ID2D1SolidColorBrush* t;
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&t
		);
		palette[0] = t;
	}
	{
		ID2D1SolidColorBrush* t;
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Crimson),
			&t
		);
		palette[3] = t;
	}
	{
		HRESULT hr;
		
		IWICBitmapDecoder* pDecoder = nullptr;
		IWICBitmapFrameDecode* pSource = nullptr;
		IWICStream* pStream = nullptr;
		IWICFormatConverter* pConverter = nullptr;
		IWICBitmapScaler* pScaler = nullptr;

		hr = wic_factory->CreateDecoderFromFilename(
			L"E:\\VSProjects\\PrintUI\\gold2.png",
			nullptr,
			GENERIC_READ,
			WICDecodeMetadataCacheOnLoad,
			&pDecoder
		);

		if(SUCCEEDED(hr)) {
			hr = pDecoder->GetFrame(0, &pSource);
		}
		if(SUCCEEDED(hr)) {
			hr = wic_factory->CreateFormatConverter(&pConverter);
		}
		if(SUCCEEDED(hr)) {
			hr = pConverter->Initialize(
				pSource,
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.f,
				WICBitmapPaletteTypeMedianCut
			);
		}
		if(SUCCEEDED(hr)) {
			hr = d2d_device_context->CreateBitmapFromWicBitmap(
				pConverter,
				nullptr,
				&palette_bitmaps[3]
			);
		}
		ID2D1BitmapBrush* t = nullptr;
		if(SUCCEEDED(hr)) {
			hr = d2d_device_context->CreateBitmapBrush(
				palette_bitmaps[3],
				&t
			);
			t->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
			t->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
			palette[1] = t;
		}

		safe_release(pDecoder);
		safe_release(pSource);
		safe_release(pStream);
		safe_release(pConverter);
		safe_release(pScaler);
		
		if(!SUCCEEDED(hr)) {
			//TODO: backup solid brush
		}
	}
}

void main_window::release_panes() {
	safe_release(win.foreground);
}
void main_window::release_palette() {
	for(int32_t i = 0; i < palette_count; ++i) {
		safe_release(palette[i]);
		safe_release(palette_bitmaps[i]);
	}
	safe_release(win.dummy_brush);
}

void main_window::release_device_resources() {
	safe_release(d3d_device);
	safe_release(swap_chain);
	safe_release(rendered_close_icon);

	release_palette();
}

void main_window::message_loop() {
	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


void main_window::redraw_ui_completely() {

	// -----
	d2d_device_context->SetTarget(win.foreground);
	d2d_device_context->BeginDraw();
	d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
	d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

	
	render_foregrounds(win.layout.get_layout(), win, d2d_device_context, win.dummy_brush, plain_strokes);

	if(!SUCCEEDED(d2d_device_context->EndDraw()))
		std::abort();
	
}


void main_window::render() {
	HRESULT hr;
	//static float t = 0.0f;
	//static DWORD dwTimeStart = 0;

	create_device_resources();

	if(!back_buffer_target)
		return;

	if(!win.is_suspended) {
		//DWORD dwTimeCur = GetTickCount();
		//if(dwTimeStart == 0) {
		//	dwTimeStart = dwTimeCur;
		//}
		//t = (dwTimeCur - dwTimeStart) / 3000.0f;

		d2d_device_context->SetTarget(win.foreground);
		d2d_device_context->BeginDraw();
		d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
		update_foregrounds(win.layout.get_layout(), win, d2d_device_context, win.dummy_brush, plain_strokes);
		d2d_device_context->EndDraw();

		printui::update_window_focus(win, nullptr);


		d2d_device_context->SetTarget(back_buffer_target);
		d2d_device_context->BeginDraw();
		d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2d_device_context->Clear(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));

		render_to_display(win.layout.get_layout(), win, d2d_device_context, palette);

		if(win.window_border != 0) {
			d2d_device_context->DrawRectangle(D2D1_RECT_F{ win.window_border / 2.0f, win.window_border / 2.0f, float(win.ui_width) - win.window_border / 2.0f, float(win.ui_height) - win.window_border / 2.0f }, palette[1], float(win.window_border), plain_strokes);
		}

		hr = d2d_device_context->EndDraw();

		DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
		hr = swap_chain->Present1(1, 0, &params);
	} else {
		DXGI_PRESENT_PARAMETERS params{ 0, nullptr, nullptr, nullptr };
		hr = swap_chain->Present1(1, DXGI_PRESENT_TEST, &params);
		if(hr == S_OK) {
			win.is_suspended = false;
			InvalidateRect((HWND)(win.m_hwnd), nullptr, FALSE);
		}
	}

	if(hr == DXGI_STATUS_OCCLUDED) {
		win.is_suspended = true;
		hr = S_OK;
	} else if(hr == DXGI_ERROR_DEVICE_RESET ||
		hr == DXGI_ERROR_DEVICE_REMOVED ||
		hr == D2DERR_RECREATE_TARGET) {
		hr = S_OK;

		release_device_resources();
	}
}


void main_window::on_resize(uint32_t width, uint32_t height) {
	if(!d3d_device) {
		create_device_resources();
	} else {
		create_window_size_resources(width, height);
	}
}


LRESULT CALLBACK main_window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_CREATE) {
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		main_window* app = (main_window*)pcs->lpCreateParams;

		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

		return 1;
	} else {
		main_window* app = reinterpret_cast<main_window*>(::GetWindowLongPtrW(hwnd,GWLP_USERDATA));

		if(app) {
			switch(message) {
				case WM_DPICHANGED:
				{
					app->win.dpi = float(HIWORD(wParam));
					auto lprcNewScale = reinterpret_cast<RECT*>(lParam);
					app->win.on_dpi_change();
					SetWindowPos(hwnd, nullptr, lprcNewScale->left, lprcNewScale->top,
						lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,
						SWP_NOZORDER | SWP_NOACTIVATE);
					break;
				}
				case WM_NCMOUSEMOVE:
				{
					RECT rcWindow;
					GetWindowRect(hwnd, &rcWindow);
					auto x = GET_X_LPARAM(lParam);
					auto y = GET_Y_LPARAM(lParam);

					POINTS adj{ SHORT(x - rcWindow.left), SHORT(y - rcWindow.top) };
					memcpy(&lParam, &adj, sizeof(LPARAM));
				}
				case WM_MOUSEMOVE:
				{
					if(app->win.dynamic_settings.imode == input_mode::follow_input) {
						if(!app->win.cursor_visible) {
							SetCursor(LoadCursor(NULL, IDC_ARROW));
							app->win.cursor_visible = true;
						}
						printui::set_prompt_visibility(app->win, printui::prompt_mode::hidden);
					} else if(app->win.dynamic_settings.imode != input_mode::mouse_only && app->win.dynamic_settings.imode != input_mode::mouse_and_keyboard) {
						break;
					}
					auto x = GET_X_LPARAM(lParam);
					auto y = GET_Y_LPARAM(lParam);

					auto& rects = app->win.layout.get_ui_rects();
					auto new_under_cursor = printui::reference_under_point(rects, x, y);
					
					auto new_l_ref = layout_reference_under_point(rects, x, y);
					set_window_focus_from_mouse(app->win, app, new_l_ref);

					if(app->win.last_under_cursor != new_under_cursor) {
						bool last_is_highlighted = app->win.last_under_cursor < rects.size() ? (rects[app->win.last_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;
						bool new_is_highlighted = new_under_cursor < rects.size() ? (rects[new_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;
						app->win.last_under_cursor = new_under_cursor;

						//auto last_interface = app->win.last_under_cursor < rects.size() ? rects[app->win.last_under_cursor].parent_object : nullptr;
						//auto new_interface = new_under_cursor < rects.size() ? rects[new_under_cursor].parent_object : nullptr;

						app->win.last_under_cursor = new_under_cursor;

						if(last_is_highlighted || new_is_highlighted) {
							InvalidateRect(hwnd, nullptr, FALSE);
						}
					}

					return 0;
				}
				case WM_KEYDOWN:
				{
					if((HIWORD(lParam) & KF_REPEAT) != 0)
						return 0;

					// IF text box has focus, process only secondary escape, else ...

					auto scan_code = 0xFF & MapVirtualKey(wParam, MAPVK_VK_TO_VSC);

					if(scan_code == app->win.primary_right_click_modifier_sc || scan_code == app->win.secondary_right_click_modifier_sc) {
						app->win.pending_right_click = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}

					if(app->win.dynamic_settings.imode == input_mode::follow_input) {
						if(app->win.cursor_visible) {
							SetCursor(nullptr);
							app->win.cursor_visible = false;
						}
						printui::set_prompt_visibility(app->win, printui::prompt_mode::keyboard);
					} else if(app->win.dynamic_settings.imode != input_mode::keyboard_only && app->win.dynamic_settings.imode != input_mode::mouse_and_keyboard) {
						break;
					}


					 if(scan_code == app->win.primary_escape_sc || scan_code == app->win.secondary_escape_sc) {
						// go up
						execute_focus_action(app->win, app, app->win.focus_actions.escape);
					} else {
						for(int32_t i = 0; i < 12; i++) {
							if(app->win.sc_values[i] == scan_code) {
								// do something with ith key
								execute_focus_action(app->win, app, app->win.focus_actions.button_actions[i]);
								break;
							}
						}
					}

					return 0;
				}
				case WM_KEYUP:
				{

					// IF text box has focus, process only secondary escape, else ...

					auto scan_code = 0xFF & MapVirtualKey(wParam, MAPVK_VK_TO_VSC);

					if(scan_code == app->win.primary_right_click_modifier_sc_up || scan_code == app->win.secondary_right_click_modifier_sc_up) {
						app->win.pending_right_click = false;
						InvalidateRect(hwnd, nullptr, FALSE);
					}

					if(app->win.dynamic_settings.imode == input_mode::follow_input) {

					} else if(app->win.dynamic_settings.imode != input_mode::keyboard_only && app->win.dynamic_settings.imode != input_mode::mouse_and_keyboard) {
						break;
					}

					return 0;
				}
				case WM_CHAR:
				{
					// only route to text box, if it has focus

					return 0;
				}
				case WM_LBUTTONDOWN:
				{
					if(app->win.dynamic_settings.imode == input_mode::follow_input) {
						if(!app->win.cursor_visible) {
							SetCursor(LoadCursor(NULL, IDC_ARROW));
							app->win.cursor_visible = true;
						}
						printui::set_prompt_visibility(app->win, printui::prompt_mode::hidden);
					} else if(app->win.dynamic_settings.imode != input_mode::mouse_only && app->win.dynamic_settings.imode != input_mode::mouse_and_keyboard) {
						break;
					}

					auto x = GET_X_LPARAM(lParam);
					auto y = GET_Y_LPARAM(lParam);

					auto under_cursor = printui::interface_under_point(app->win.layout.get_ui_rects(), x, y);
					if(under_cursor) {
						if(app->win.pending_right_click) {

							if((GetKeyState(MapVirtualKey(app->win.primary_right_click_modifier_sc, MAPVK_VSC_TO_VK)) == 0 && GetKeyState(MapVirtualKey(app->win.primary_right_click_modifier_sc, MAPVK_VSC_TO_VK)) == 0) || app->win.dynamic_settings.imode == input_mode::mouse_only) {
								app->win.pending_right_click = false; // right click modifier not held down
								app->win.layout.window_bar.info_i.mark_for_update();
							}

							under_cursor->parent_object->on_right_click(nullptr, uint32_t(x - under_cursor->x_position), uint32_t(y - under_cursor->y_position));
						} else {
							under_cursor->parent_object->on_click(nullptr, uint32_t(x - under_cursor->x_position), uint32_t(y - under_cursor->y_position));
						}
						return 0;
					}

					break;
				}
				case WM_NCCALCSIZE:
					if(wParam == TRUE)
						return 0;
					break;
				case WM_NCHITTEST:
				{
					// Get the point coordinates for the hit test.
					POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

					// Get the window rectangle.
					RECT rcWindow;
					GetWindowRect(hwnd, &rcWindow);

					// Determine if the hit test is for resizing. Default middle (1,1).
					USHORT uRow = 1;
					USHORT uCol = 1;

					// Determine if the point is at the top or bottom of the window.
					if(ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + 5) {
						uRow = 0;
					} else if(ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - 5) {
						uRow = 2;
					}

					// Determine if the point is at the left or right of the window.
					if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + 5) {
						uCol = 0; // left side
					} else if(ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - 5) {
						uCol = 2; // right side
					}

					// Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
					LRESULT hitTests[3][3] =
					{
						{ HTTOPLEFT,    HTTOP ,     HTTOPRIGHT },
						{ HTLEFT,       HTNOWHERE,  HTRIGHT },
						{ HTBOTTOMLEFT, HTBOTTOM,   HTBOTTOMRIGHT },
					};

					if(hitTests[uRow][uCol] == HTNOWHERE) {
						if(app->win.layout.visible_window_title() && ptMouse.y < rcWindow.top + app->win.window_border + app->win.layout_size) {
							return HTCAPTION;
						}
						if(app->win.orientation == layout_orientation::horizontal_left_to_right || app->win.orientation == layout_orientation::vertical_left_to_right) {

							if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + app->win.window_border + 2 * app->win.layout_size) {
								if(printui::interface_under_point(app->win.layout.get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top) == nullptr) {
									return HTCAPTION;
								}
							}
						} else {
							if(ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - (app->win.window_border + 2 * app->win.layout_size)) {
								if(printui::interface_under_point(app->win.layout.get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top) == nullptr) {
									return HTCAPTION;
								}
							}
						}
					}

					if(hitTests[uRow][uCol] != HTNOWHERE)
						return hitTests[uRow][uCol];
					break;
				}
				case WM_ENTERSIZEMOVE:
					app->win.is_sizeing = true;
					return 0;
				case WM_EXITSIZEMOVE:
					app->win.is_sizeing = false;
					return 0;
				case WM_SIZE:
					if(wParam == SIZE_MAXIMIZED) {
						auto monitor_handle = MonitorFromWindow((HWND)(app->win.m_hwnd), MONITOR_DEFAULTTOPRIMARY);
						MONITORINFO mi;
						mi.cbSize = sizeof(mi);
						GetMonitorInfo(monitor_handle, &mi);

						app->win.window_saved_border = std::max(app->win.window_border, app->win.window_saved_border);
						app->win.window_border = 0;

						SetWindowPos((HWND)(app->win.m_hwnd), nullptr, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, SWP_NOZORDER | SWP_NOACTIVATE);
						app->on_resize(mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top);
						InvalidateRect(hwnd, nullptr, FALSE);

						return 0;
					} else if(wParam != SIZE_MINIMIZED && wParam != SIZE_MAXHIDE && wParam != SIZE_MAXSHOW) {
						//RECT r = {};
						//GetWindowRect((HWND)(app->win.m_hwnd), &r);
						app->win.window_border = std::max(app->win.window_border, app->win.window_saved_border);
						app->on_resize(LOWORD(lParam), HIWORD(lParam));
						
						if(app->win.minimum_ui_width > app->win.ui_width || app->win.minimum_ui_height > app->win.ui_height) {
							auto monitor_handle = MonitorFromWindow((HWND)(app->win.m_hwnd), MONITOR_DEFAULTTOPRIMARY);
							MONITORINFO mi;
							mi.cbSize = sizeof(mi);
							GetMonitorInfo(monitor_handle, &mi);

							WINDOWPLACEMENT placement;
							placement.length = sizeof(WINDOWPLACEMENT);
							GetWindowPlacement((HWND)(app->win.m_hwnd), &placement);

							RECT new_normal;
							auto new_x_size = std::max(app->win.minimum_ui_width, app->win.ui_width);
							auto new_y_size = std::max(app->win.minimum_ui_height, app->win.ui_height);

							auto const change_width = new_x_size - (placement.rcNormalPosition.right - placement.rcNormalPosition.left);
							auto const change_height = new_y_size - (placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);
							new_normal.left = std::clamp(int32_t(placement.rcNormalPosition.left - change_width / 2), 0, int32_t((mi.rcWork.right - mi.rcWork.left) - new_x_size));
							new_normal.top = std::clamp(int32_t(placement.rcNormalPosition.top - change_height / 2), 0, int32_t((mi.rcWork.bottom - mi.rcWork.top) - new_y_size));
							new_normal.right = new_normal.left + new_x_size;
							new_normal.bottom = new_normal.top + new_y_size;
							placement.rcNormalPosition = new_normal;

							SetWindowPlacement((HWND)(app->win.m_hwnd), &placement);
						} else {
							InvalidateRect(hwnd, nullptr, FALSE);
						}
						return 0;
					}
					break;
				case WM_GETMINMAXINFO:
				{
					MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
					pMinMaxInfo->ptMinTrackSize.x = static_cast<UINT>(app->win.minimum_ui_width);
					pMinMaxInfo->ptMinTrackSize.y = static_cast<UINT>(app->win.minimum_ui_height);
					return 0;
				}
				case WM_PAINT:
				case WM_DISPLAYCHANGE:
				{
					PAINTSTRUCT ps;
					BeginPaint(hwnd, &ps);
					app->render();
					EndPaint(hwnd, &ps);
					return 0;
				}
				case WM_DESTROY:
					app->win.layout.release_all();
					PostQuitMessage(0);
					return 1;
				default:
					break;
			}
		}
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}
}

*/
