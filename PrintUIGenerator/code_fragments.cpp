#include "code_fragments.hpp"

basic_builder& generate_includes(basic_builder& o) {
	o + "#ifndef UNICODE";
	o + "#define UNICODE";
	o + "#endif";

	o + "#define WIN32_LEAN_AND_MEAN";
	o + "#include <windows.h>";
	o + "#include <cstdint>";
	o + "#include <Windowsx.h>";
	o + "#include <shlwapi.h>";
	o + "#include <cmath>";
	o + "#include <stdlib.h>";
	o + "#include <malloc.h>";
	o + "#include <memory.h>";
	o + "#include <wchar.h>";
	o + "#include <dxgi1_2.h>";
	o + "#include <d3d11_3.h>";
	o + "#include <DirectXMath.h>";
	o + "#include <d2d1_3.h>";
	o + "#include <d2d1helper.h>";
	o + "#include <dwrite_3.h>";
	o + "#include <wincodec.h>";
	o + "#include <unordered_map>";
	o + "#include <vector>";
	o + line_break{};
	o + "#include \"printui_utility.hpp\"";
	o + line_break{};
	o + "#pragma comment(lib, \"d2d1.lib\")";
	o + "#pragma comment(lib, \"D3D11.lib\")";
	o + "#pragma comment(lib, \"Windowscodecs.lib\")";
	o + "#pragma comment(lib, \"Dwrite.lib\")";
	o + "#pragma comment(lib, \"Shlwapi.lib\")";
	o + line_break{};
	return o;
}

basic_builder& generate_class_definition(basic_builder& o, file_contents const& fi) {
	o + substitute{ "palc", std::to_string(fi.palette.size()) };
	o + substitute{ "cfc", std::to_string(fi.custom_fonts.size()) };

	o + "class main_window" + class_block{
		o + "public:";
		o + "main_window() {}";
		o + "~main_window();";
		o + "void create_window();";
		o + "void message_loop();";

		o + "static constexpr int32_t palette_count = @palc@;";
		o + "static constexpr int32_t custom_font_count = @cfc@;";

		o + "window_data win;";

		o + line_break{};
		o + "private:";
		o + "void create_persistent_resources();";
		o + "void create_device_resources();";
		o + "void create_window_size_resources(uint32_t nWidth, uint32_t nHeight);";
		o + "void release_device_resources();";
		o + "void render();";
		o + "void on_resize(uint32_t width, uint32_t height);";
		o + "static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);";
		o + "void create_panes();";
		o + "HRESULT create_palette();";
		o + "void release_palette();";
		o + "void redraw_ui_completely();";
		o + "void on_dpi_change();";

	};
	o + line_break{};
	return o;
}

basic_builder& generate_destructor(basic_builder& o, file_contents const& fi) {
	o + "main_window::~main_window()" + block{
		o + "safe_release(wic_factory);";
		o + "safe_release(d3d_device);";
		o + "safe_release(dxgi_device);";
		o + "safe_release(d2d_device);";
		o + "safe_release(d3d_device_context);";
		o + "safe_release(swap_chain);";
		o + "safe_release(back_buffer_target);";
		o + "safe_release(bg_pane);";
		o + "safe_release(fg_pane);";
		o + "release_palette();";
		o + "safe_release(rendered_close_icon);";
	};
	o + line_break{};
	return o;
}

basic_builder& generate_core_window_functions(basic_builder& o, file_contents const& fi) {
	o + "void main_window::create_window()" + block{
		o + "SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);";

		o + substitute{ "icodir", fi.icon_directory };
		o + "win.icon_directory = L\"@icodir@\";";

		o + "create_persistent_resources();";

		o + "WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };";
		o + "wcex.style = CS_OWNDC;";
		o + "wcex.lpfnWndProc = main_window::WndProc;";
		o + "wcex.cbClsExtra = 0;";
		o + "wcex.cbWndExtra = sizeof(LONG_PTR);";
		o + "wcex.hInstance = GetModuleHandle(nullptr);";
		o + "wcex.hbrBackground = nullptr;";
		o + "wcex.lpszMenuName = nullptr;";
		o + "wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);";
		o + "wcex.lpszClassName = L\"printui_class\";";

		o + "if(RegisterClassEx(&wcex) == 0)" + block{
			o + "std::abort();";
		};

		o + substitute{ "app", fi.name };

		o + "win.m_hwnd = CreateWindowEx(";
		o + "0,";
		o + "L\"printui_class\",";
		o + "L\"@app@\",";
		o + "WS_CAPTION | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,";
		o + "CW_USEDEFAULT,";
		o + "CW_USEDEFAULT,";
		o + "0,";
		o + "0,";
		o + "nullptr,";
		o + "nullptr,";
		o + "GetModuleHandle(nullptr),";
		o + "this";
		o + ");";

		o + "if(win.m_hwnd)" + block{
			o + "win.dpi = float(GetDpiForWindow((HWND)(win.m_hwnd)));";

			o + "SetWindowPos(";
			o + "(HWND)(win.m_hwnd),";
			o + "nullptr,";
			o + "nullptr,";
			o + "nullptr,";
			o + "static_cast<int>(ceil(640.f * win.dpi / 96.f)),";
			o + "static_cast<int>(ceil(480.f * win.dpi / 96.f)),";
			o + "SWP_NOMOVE | SWP_FRAMECHANGED); ";
			o + "ShowWindow((HWND)(win.m_hwnd), SW_SHOWNORMAL); ";
			o + "UpdateWindow((HWND)(win.m_hwnd)); ";
		};
	};

	o + "void main_window::message_loop()" + block{
		o + "MSG msg;";

		o + "while(GetMessage(&msg, NULL, 0, 0))" + block{
			o + "TranslateMessage(&msg);";
			o + "DispatchMessage(&msg);";
		};
	};

	o + line_break{};
	return o;
}

basic_builder& generate_create_persistent_resources(basic_builder& o, file_contents const& fi) {
	o + "void main_window::create_persistent_resources()" + block{
		o + "D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &win.d2d_factory);";
		o + "CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&wic_factory));";
		o + "DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(win.dwrite_factory), reinterpret_cast<IUnknown**>(&win.dwrite_factory));";

		// default init launch settings
		o + substitute{ "val", std::to_string(fi.small_width) };
		o + "win.dynamic_settings.small_width = @val@;";
		o + substitute{ "val", std::to_string(fi.line_width) };
		o + "win.dynamic_settings.line_width = @val@;";
		o + substitute{ "val", std::to_string(fi.layout_base_size) };
		o + "win.dynamic_settings.layout_base_size = @val@;";
		o + substitute{ "val", fi.primary_font.name };
		o + "win.dynamic_settings.primary_font.name = L\"@val@\";";
		o + substitute{ "val", fi.primary_font.is_bold ? "true" : "false" };
		o + "win.dynamic_settings.primary_font.is_bold = @val@;";
		o + substitute{ "val", fi.primary_font.is_oblique ? "true" : "false" };
		o + "win.dynamic_settings.primary_font.is_oblique = @val@;";
		o + substitute{ "val", to_string(fi.primary_font.type) };
		o + "win.dynamic_settings.primary_font.type = @val@;";
		o + substitute{ "val", to_string(fi.primary_font.span) };
		o + "win.dynamic_settings.primary_font.span = @val@;";
		o + substitute{ "val", std::to_string(fi.window_border) };
		o + "win.dynamic_settings.window_border = @val@;";

		if(fi.small_font.has_value()) {
			o + substitute{ "val", fi.small_font->name };
			o + "win.dynamic_settings.small_font.name = L\"@val@\";";
			o + substitute{ "val", fi.small_font->is_bold ? "true" : "false" };
			o + "win.dynamic_settings.small_font.is_bold = @val@;";
			o + substitute{ "val", fi.small_font->is_oblique ? "true" : "false" };
			o + "win.dynamic_settings.small_font.is_oblique = @val@;";
			o + substitute{ "val", to_string(fi.small_font->type) };
			o + "win.dynamic_settings.small_font.type = @val@;";
			o + substitute{ "val", to_string(fi.small_font->span) };
			o + "win.dynamic_settings.small_font.span = @val@;";
		} else {
			o + substitute{ "val", fi.primary_font.name };
			o + "win.dynamic_settings.small_font.name = L\"@val@\";";
			o + substitute{ "val", fi.primary_font.is_bold ? "true" : "false" };
			o + "win.dynamic_settings.small_font.is_bold = @val@;";
			o + substitute{ "val", fi.primary_font.is_oblique ? "true" : "false" };
			o + "win.dynamic_settings.small_font.is_oblique = @val@;";
			o + substitute{ "val", to_string(fi.primary_font.type) };
			o + "win.dynamic_settings.small_font.type = @val@;";
			o + substitute{ "val", to_string(fi.primary_font.span) };
			o + "win.dynamic_settings.small_font.span = @val@;";
		}

		int32_t i = 0;
		for(auto& ft : fi.custom_fonts) {
			o + substitute{ "tag", ft.first } + substitute{ "index", std::to_string(i) };
			o + "font_name_to_index.insert_or_assign(\"@tag@\", @index@);";
			o + substitute{ "fname", ft.second.name } + substitute{ "type", to_string(ft.second.type) } + substitute{ "span", to_string(ft.second.span) } + substitute{ "is_bold", ft.second.is_bold ? "true" : "false" } + substitute{ "is_oblique", ft.second.is_oblique ? "true" : "false" };
			o + "win.dynamic_settings.named_fonts.push_back(font_description{ L\"@fname@\", @type@, @span@, @is_bold@, @is_oblique@})";
			++i;
		}

		for(auto& b : fi.palette) {
			o + substitute{ "tex", b.texture } + substitute{ "r", std::to_string(b.r) } + substitute{ "g", std::to_string(b.g) } + substitute{ "b", std::to_string(b.b) } + substitute{ "is_light_color", b.is_light_color ? "true" : "false" };
			o + "win.dynamic_settings.brushes.push_back(brush{ L\"@tex@\", brush_color{ @r@, @g@, @b@ }, @is_light_color@})";
		}
		// end default init launch settings

		o + "load_launch_settings(win.dynamic_settings, font_name_to_index);";

		o + inline_block{
			o + "D2D1_STROKE_STYLE_PROPERTIES style_prop;";
			o + "style_prop.startCap = D2D1_CAP_STYLE_FLAT;";
			o + "style_prop.endCap = D2D1_CAP_STYLE_FLAT;";
			o + "style_prop.dashCap = D2D1_CAP_STYLE_FLAT;";
			o + "style_prop.lineJoin = D2D1_LINE_JOIN_MITER;";
			o + "style_prop.miterLimit = 10.0f;";
			o + "style_prop.dashStyle = D2D1_DASH_STYLE_SOLID;";
			o + "style_prop.dashOffset = 0.0f;";
			o + "win.d2d_factory->CreateStrokeStyle(style_prop, nullptr, 0, &plain_strokes);";
		};

		//fonts
		
		o + substitute{ "font_dir", fi.font_directory };
		o + "printui::create_font_collection(win, L\"@font_dir@\");";
		

		//fallbacks
		o + "auto global_fb = get_global_font_fallbacks();";

		o + inline_block{
			o + "IDWriteFontFallbackBuilder* bldr = nullptr;";
			o + "win.dwrite_factory->CreateFontFallbackBuilder(&bldr);";

			for(auto& fb : fi.primary_font.fallbacks) {
				if(fb.ranges.size() > 0) {
					o + substitute{ "name", fb.name } + substitute{ "range_count", std::to_string(fb.ranges.size()) };
					o + substitute{ "scale", std::to_string(fb.scale) };
					o + inline_block{
						o + "WCHAR const* fn[] = { L\"@name@\" }";
						o + "DWRITE_UNICODE_RANGE ranges[] =" + class_block{
							for(auto& r : fb.ranges) {
								o + substitute{ "rstart", std::to_string(r.start) } + substitute{ "rend", std::to_string(r.end) };
								o + "DWRITE_UNICODE_RANGE{ @rstart@, @rend@ },";
							}
						};
						o + "bldr->AddMapping(ranges, @range_count@, fn, 1, win.font_collection, nullptr, nullptr, @scale@);";
					};
				}
			}

			o + substitute("ftype", to_string(fi.primary_font.type));
			o + "load_fallbacks_by_type(global_fb, @ftype@, bldr, win.font_collection, win.dwrite_factory);";
			o + "bldr->CreateFontFallback(&font_fallbacks);";
			o + "safe_release(bldr);";
		};

		if(fi.small_font.has_value()) {
			o + inline_block{
				o + "IDWriteFontFallbackBuilder* bldr = nullptr;";
				o + "win.dwrite_factory->CreateFontFallbackBuilder(&bldr);";

				for(auto& fb : fi.small_font->fallbacks) {
					o + substitute{ "name", fb.name } + substitute{ "range_count", std::to_string(fb.ranges.size()) };
					o + substitute{ "scale", std::to_string(fb.scale) };
					o + inline_block{
						o + "WCHAR const* fn[] = { L\"@name@\" }";
						o + "DWRITE_UNICODE_RANGE ranges[] =" + class_block{
							for(auto& r : fb.ranges) {
								o + substitute{ "rstart", std::to_string(r.start) } + substitute{ "rend", std::to_string(r.end) };
								o + "DWRITE_UNICODE_RANGE{ @rstart@, @rend@ },";
							}
						};
						o + "bldr->AddMapping(ranges, @range_count@, fn, 1, win.font_collection, nullptr, nullptr, @scale@);";
					};
				}

				o + substitute("ftype", to_string(fi.small_font->type));
				o + "load_fallbacks_by_type(global_fb, @ftype@, bldr, win.font_collection, win.dwrite_factory);";
				o + "bldr->CreateFontFallback(&small_font_fallbacks);";
				o + "safe_release(bldr);";
			};
		}
	};
	o + line_break{};
	return o;
}

basic_builder& generate_function_declarations(basic_builder& o, file_contents const& fi) {
}

basic_builder& generate_create_device_resources(basic_builder& o, file_contents const& fi) {
	o + "void main_window::create_device_resources()" + block{
		o + "HRESULT hr = S_OK;";

		o + "if(!d3d_device)" + block{
			o + "IDXGIAdapter* dxgi_adapter = nullptr;";
			o + "IDXGIFactory2* dxgi_factory = nullptr;";

			o + "RECT client_rect;";
			o + "GetClientRect((HWND)(win.m_hwnd), &client_rect);";

			o + "auto nWidth = abs(client_rect.right - client_rect.left);";
			o + "auto nHeight = abs(client_rect.bottom - client_rect.top);";

			o + "D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };";

			o + "hr = D3D11CreateDevice(";
				o + "\tnullptr,";
				o + "\tD3D_DRIVER_TYPE_HARDWARE,";
				o + "\tnullptr,";
				o + "\tD3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,";
				o + "\tlevels, 4,";
				o + "\tD3D11_SDK_VERSION,";
				o + "\t&d3d_device,";
				o + "\tnullptr,";
				o + "\t&d3d_device_context";
			o + ");";

			o + "if(FAILED(hr))" + block{
				o + "hr = D3D11CreateDevice(nullptr,";
					o + "\tD3D_DRIVER_TYPE_WARP,";
					o + "\tnullptr,";
					o + "\tD3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,";
					o + "\tlevels, 4,";
					o + "\tD3D11_SDK_VERSION,";
					o + "\t&d3d_device,";
					o + "\tnullptr,";
					o + "\t&d3d_device_context";
				o + ");";
			};

			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = win.d2d_factory->CreateDevice(dxgi_device, &d2d_device);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d_device_context);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = d3d_device->QueryInterface(&d3d_device);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = d3d_device->QueryInterface(&dxgi_device);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = dxgi_device->GetAdapter(&dxgi_adapter);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "DXGI_SWAP_CHAIN_DESC1 swapDesc;";
				o + "::ZeroMemory(&swapDesc, sizeof(swapDesc));";
				o + "swapDesc.Width = nWidth;";
				o + "swapDesc.Height = nHeight;";
				o + "swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;";
				o + "swapDesc.Stereo = FALSE;";
				o + "swapDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };";
				o + "swapDesc.SampleDesc.Count = 1;";
				o + "swapDesc.SampleDesc.Quality = 0;";
				o + "swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;";
				o + "swapDesc.BufferCount = 2;";
				o + "swapDesc.Scaling = DXGI_SCALING_NONE;";
				o + "swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;";
				o + "swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;";
				o + "swapDesc.Flags = 0;";
				o + "hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, (HWND)(win.m_hwnd), &swapDesc, nullptr, nullptr, &swap_chain);";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = dxgi_device->SetMaximumFrameLatency(1);";
			};

			// TODO load icons
			/*
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
						dpi, dpi,
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
			*/

			o + "if(SUCCEEDED(hr))" + block{
				o + "hr = create_palette();";
			};
			o + "if(SUCCEEDED(hr))" + block{
				o + "on_dpi_change();";
				o + "create_window_size_resources(nWidth, nHeight);";
			} + append{ "else" } + block{
				// TODO: make translatable
				o + "MessageBox((HWND)(win.m_hwnd), L\"Could not create direct X devices, exiting\", L\"Fatal Error\", MB_OK | MB_ICONERROR);";
				o + "std::terminate();";
			};

			o + "create_persistent_resources();";

			o + "safe_release(dxgi_adapter);";
			o + "safe_release(dxgi_factory);";
		};
	};

	o + "void main_window::release_device_resources()" + block{
		o + "safe_release(d3d_device);";
		o + "safe_release(swap_chain);";
		o + "safe_release(rendered_close_icon);";
		o + "release_palette();";
	};

	o + line_break{};
	return o;
}

basic_builder& generate_create_sized_resources(basic_builder& o, file_contents const& fi) {
	o + "void main_window::create_window_size_resources(uint32_t nWidth, uint32_t nHeight)" + block{
		o + "HRESULT hr = S_OK;";
		o + "IDXGISurface * back_buffer = nullptr;";

		o + "if(win.ui_width == nWidth && win.ui_height == nHeight) return;";
		o + "win.ui_width = nWidth;";
		o + "win.ui_height = nHeight;";
		o + "win.dpi = float(GetDpiForWindow((HWND)(win.m_hwnd)));";

		o + "if(!swap_chain) return;";

		o + "safe_release(back_buffer_target);";
		o + "safe_release(bg_pane);";
		o + "safe_release(fg_pane);";

		o + "d2d_device_context->SetTarget(nullptr);";
		o + "hr = swap_chain->ResizeBuffers(0, nWidth, nHeight, DXGI_FORMAT_B8G8R8A8_UNORM, 0);";

		o + "if(SUCCEEDED(hr))" + block{
			o + "hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));";
		};
		o + "if(SUCCEEDED(hr))" + block{
			o + "D2D1_BITMAP_PROPERTIES1 bitmapProperties =";
			o + "D2D1::BitmapProperties1(";
			o + "D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,";
			o + "D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),";
			o + "win.dpi,";
			o + "win.dpi";
			o + ");";
			o + line_break{};
			o + "hr = d2d_device_context->CreateBitmapFromDxgiSurface(";
			o + "back_buffer,";
			o + "&bitmapProperties,";
			o + "&back_buffer_target";
			o + ");";
		};
		o + "create_panes();";
		o + "safe_release(back_buffer);";
	};

	o + "void main_window::create_panes()" + block{
		o + "d2d_device_context->CreateBitmap(D2D1_SIZE_U{ win.ui_width, win.ui_height }, nullptr, 0,";
			o + "D2D1_BITMAP_PROPERTIES1{ D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),";
			o + "win.dpi, win.dpi, D2D1_BITMAP_OPTIONS_TARGET , nullptr },";
			o + "&fg_pane);";
		o + "redraw_ui_completely();";
	};

	o + "void main_window::on_resize(uint32_t width, uint32_t height) " + block{
		o + "if(!d3d_device)" + block{
			o + "create_device_resources();";
		} + append{"else"} + block{
			o + "create_window_size_resources(width, height);";
		};
	};

	o + line_break{};
	return o;
}

basic_builder& generate_palette_functions(basic_builder& o, file_contents const& fi) {

	o + substitute{ "tex_path", fi.texture_directory };

	o + "void main_window::create_palette()" + block{
		o + "create_highlight_brushes(win, d2d_device_context);";
		o + "HRESULT hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &win.dummy_brush);";

		o + "int32_t i = 0;";
		o + "for(auto& b : win.dynamic_settings.brushes)" + block{
			o + "if(SUCCEEDED(hr))" + block{
				o + "if(b.texture.length() > 0)" + block{
					o + "auto pth = get_path(b.texture, L\"@tex_path@\", L\"textures\");";

					o + "IWICBitmapDecoder* pDecoder = nullptr;";
					o + "IWICBitmapFrameDecode * pSource = nullptr;";
					o + "IWICStream * pStream = nullptr;";
					o + "IWICFormatConverter * pConverter = nullptr;";
					o + "IWICBitmapScaler * pScaler = nullptr;";
					o + "ID2D1BitmapBrush* t = nullptr;";

					o + "if(pth.has_value())" + block{
						o + "hr = wic_factory->CreateDecoderFromFilename(pth->c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);";
					} + append{ "else" } + block{
						o + "hr = E_FAIL;";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "hr = pDecoder->GetFrame(0, &pSource);";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "hr = wic_factory->CreateFormatConverter(&pConverter);";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "hr = pConverter->Initialize(";
						o + "pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,";
						o + "nullptr, 0.f, WICBitmapPaletteTypeMedianCut);";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "hr = d2d_device_context->CreateBitmapFromWicBitmap(pConverter, nullptr, &palette_bitmaps[i]);";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "hr = d2d_device_context->CreateBitmapBrush(palette_bitmaps[i], &t);";
					};
					o + "if(SUCCEEDED(hr))" + block{
						o + "t->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);";
						o + "t->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);";
						o + "palette[i] = t;";
					};

					o + "safe_release(pDecoder);";
					o + "safe_release(pSource);";
					o + "safe_release(pStream);";
					o + "safe_release(pConverter);";
					o + "safe_release(pScaler);";

					o + "if(FAILED(hr))" + block{
						o + "ID2D1SolidColorBrush* t;";
						o + "hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(b.rgb.r, b.rgb.g, b.rgb.b, 1.0f), &t);";
						o + "palette[i] = t;";
					};
				} + append{"else"} + block{
					o + "ID2D1SolidColorBrush* t;";
					o + "hr = d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(b.rgb.r, b.rgb.g, b.rgb.b, 1.0f), &t);";
					o + "palette[i] = t;";
				};
			};
			o + "++i;";
		};
		o + "return hr;";
	};

	o + "void main_window::release_palette()" + block{
		o + "for(int32_t i = 0; i < palette_count; ++i)" + block{
			o + "safe_release(palette[i]);";
			o + "safe_release(palette_bitmaps[i]);";
		};
		o + "safe_release(win.dummy_brush);";
	};

	o + line_break{};
	return o;
}

basic_builder& generate_win_proc(basic_builder& o, file_contents const& fi) {
	o + "LRESULT CALLBACK main_window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)" + block{
		o + "if(message == WM_CREATE)" + block{
			o + "LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;";
			o + "main_window* app = (main_window*)pcs->lpCreateParams;";
			o + "SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));";
			o + "return 1;";
		} + append{"else"} + block{
			o + "main_window* app = reinterpret_cast<main_window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));";

			o + "if(app)" + block{
				o + "switch(message)" + block{
					o + "case WM_DPICHANGED:";
					o + inline_block{
						o + "app->win.dpi = float(HIWORD(wParam));";
						o + "app->on_dpi_change();";
						o + "auto lprcNewScale = reinterpret_cast<RECT*>(lParam);";
						o + "SetWindowPos(hwnd, nullptr, lprcNewScale->left, lprcNewScale->top,";
							o + "lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,";
							o + "SWP_NOZORDER | SWP_NOACTIVATE);";
						o + "break;";
					};
					o + "case WM_LBUTTONDOWN:";
					o + inline_block{
						o + "auto x = GET_X_LPARAM(lParam);";
						o + "auto y = GET_Y_LPARAM(lParam);";
						o + "if(x >= 10 && x <= 60 && y >= 250 && y <= 300) DestroyWindow(hwnd);";
						o + "break;";
					};
					o + "case WM_NCCALCSIZE:";
					o + inline_block{
						o + "if(wParam == TRUE) return 0;";
						o + "break;";
					};
					o + "case WM_NCHITTEST:";
					o + inline_block{
						o + "POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };";

						o + "RECT rcWindow;";
						o + "GetWindowRect(hwnd, &rcWindow);";

						o + "USHORT uRow = 1;";
						o + "USHORT uCol = 1;";
						o + "bool fOnResizeBorder = false;";

						o + "if(ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + 25)" + block{
							o + "fOnResizeBorder = (ptMouse.y < (rcWindow.top + (app->win.window_border + 3)));";
							o + "uRow = 0;";
						} + append{ "else" };
						o + "if(ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - (app->win.window_border + 3))" + block{
							o + "uRow = 2;";
						};

						o + "if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + (app->win.window_border + 3))" + block{
							o + "uCol = 0;";
						} + append{ "else" };
						o + "if(ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - (app->win.window_border + 3))" + block{
							o + "uCol = 2;";
						};

						o + "LRESULT hitTests[3][3] =" + class_block{
							o + "{ HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },";
							o + "{ HTLEFT,       HTNOWHERE,     HTRIGHT },";
							o + "{ HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },";
						};

						o + "if(hitTests[uRow][uCol] != HTNOWHERE) return hitTests[uRow][uCol];";
						o + "break;";
					};
					o + "case WM_ENTERSIZEMOVE:";
					o + inline_block{
						o + "app->win.is_sizeing = true;";
						o + "return 0;";
					};
					o + "case WM_EXITSIZEMOVE:";
					o + inline_block{
						o + "app->win.is_sizeing = false;";
						o + "return 0;";
					};
					o + "case WM_SIZE:";
					o + inline_block{
						o + "if(wParam != SIZE_MINIMIZED && wParam != SIZE_MAXHIDE && wParam != SIZE_MAXSHOW)" + block{
							o + "app->on_resize(LOWORD(lParam), HIWORD(lParam));";
							o + "InvalidateRect(hwnd, nullptr, FALSE);";
							o + "return 0;";
						};
						o + "break;";
					};
					o + "case WM_GETMINMAXINFO:";
					o + inline_block{
						o + "MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);";
						o + "pMinMaxInfo->ptMinTrackSize.x = static_cast<UINT>(ceil(200.f * app->win.dpi / 96.f));";
						o + "pMinMaxInfo->ptMinTrackSize.y = static_cast<UINT>(ceil(200.f * app->win.dpi / 96.f));";
						o + "return 0;";
					};
					o + "case WM_PAINT:";
					o + "case WM_DISPLAYCHANGE:";
					o + inline_block{
						o + "PAINTSTRUCT ps;";
						o + "BeginPaint(hwnd, &ps);";
						o + "app->render();";
						o + "EndPaint(hwnd, &ps);";
						o + "return 0;";
					};
					o + "case WM_DESTROY:";
					o + inline_block{
						o + "PostQuitMessage(0);";
						o + "return 1;";
					};
					o + "default:";
						o + "\tbreak;";
				};
			};
			o + "return DefWindowProc(hwnd, message, wParam, lParam);";
		};
	};

	o + line_break{};
	return o;
}

basic_builder& generate_dpi_functions(basic_builder& o, file_contents const& fi) {

	o + "void main_window::on_dpi_change()" + block{

		o + "win.layout_size = int32_t(std::round(float(win.dynamic_settings.layout_base_size) * win.dpi / 96.0f));";
		o + "win.window_border = int32_t(std::round(float(win.dynamic_settings.window_border) * win.dpi / 96.0f));";

		// metrics
		o + inline_block{
			o + substitute{ "fname", fi.primary_font.name };
			o + substitute{ "weight", to_font_weight(fi.primary_font.is_bold) } + substitute{ "style", to_font_style(fi.primary_font.is_oblique) } + substitute{ "span", to_font_span(fi.primary_font.span) };


			o + "IDWriteFont3* f = nullptr;";
			if(fi.small_font.has_value())
				o + "IDWriteFont3* f2 = nullptr;";

			o + "IDWriteFontList2* fl;";
			o + "DWRITE_FONT_AXIS_VALUE fax[] = {";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, @weight@},";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, @span@ } ,";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, @style@ } };";

			o + "win.font_collection->GetMatchingFonts(L\"@fname@\", fax, 3, &fl);";
			o + "fl->GetFont(0, &f);";
			o + "safe_release(fl);";

			if(fi.small_font.has_value()) {
				o + substitute{ "fnameB", fi.small_font->name };
				o + substitute{ "weightB", to_font_weight(fi.small_font->is_bold) } + substitute{ "styleB", to_font_style(fi.small_font->is_oblique) } + substitute{ "spanB", to_font_span(fi.small_font->span) };

				o + "IDWriteFontList2* fl2;";
				o + "DWRITE_FONT_AXIS_VALUE fax2[] = {";
				o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, @weightB@},";
				o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, @spanB@ } ,";
				o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, @styleB@ } };";

				o + "win.font_collection->GetMatchingFonts(L\"@fnameB@\", fax2, 3, &fl2);";
				o + "fl2->GetFont(0, &f2);";
				o + "safe_release(fl2);";
			}

			o + substitute{ "lsize", std::to_string(fi.layout_base_size) };
			if(fi.small_font.has_value())
				o + "win.text_alignment = get_text_alignment(win.layout_size, f, f2);";
			else
				o + "win.text_alignment = get_text_alignment(win.layout_size, f, f);";

			if(fi.small_font.has_value())
				o + "safe_release(f2);";
			o + "safe_release(f);";
		};

		// text formats
		o + inline_block{
			o + substitute{ "fname", fi.primary_font.name };
			o + substitute{ "weight", to_font_weight(fi.primary_font.is_bold) } + substitute{ "style", to_font_style(fi.primary_font.is_oblique) } + substitute{ "span", to_font_span(fi.primary_font.span) };

			o + "DWRITE_FONT_AXIS_VALUE fax[] = {";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, @weight@},";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, @span@ } ,";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, @style@ } };";

			o + "win.dwrite_factory->CreateTextFormat(L\"@fname@\", win.font_collection, fax, 3, win.text_alignment.big_font_size, L\"\", &win.common_text_format);";
			o + "win.common_text_format->SetFontFallback(font_fallbacks);";
			o + "win.common_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);";
			o + "win.common_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.text_alignment.big_line_spacing, win.text_alignment.big_baseline);";
		};
		o + inline_block{
			o + substitute{ "fname", fi.small_font.has_value() ? fi.small_font->name : fi.primary_font.name };
			o + substitute{ "weight", fi.small_font.has_value() ? to_font_weight(fi.small_font->is_bold) : to_font_weight(fi.primary_font.is_bold) };
			o + substitute{ "style", fi.small_font.has_value() ? to_font_style(fi.small_font->is_oblique) : to_font_style(fi.primary_font.is_oblique) };
			o + substitute{ "span",  fi.small_font.has_value() ? to_font_span(fi.small_font->span) : to_font_span(fi.primary_font.span) };

			o + "DWRITE_FONT_AXIS_VALUE fax[] = {";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, @weight@},";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, @span@ } ,";
			o + "\tDWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, @style@ } };";

			o + "win.dwrite_factory->CreateTextFormat(L\"@fname@\", win.font_collection, fax, 3, win.text_alignment.small_font_size, L\"\", &win.small_text_format);";
			if(fi.small_font.has_value())
				o + "win.small_text_format->SetFontFallback(small_font_fallbacks);";
			else
				o + "win.small_text_format->SetFontFallback(font_fallbacks);";
			o + "win.small_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);";
			o + "win.small_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.text_alignment.samll_line_spacing, win.text_alignment.small_baseline);";
		};

		// redraw icons
	};

	o + line_break{};
	return o;
}
