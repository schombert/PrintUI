#pragma once
#include "source_builder.hpp"
#include "parsing.hpp"

[[nodiscard]] basic_builder& generate_includes(basic_builder& o);
[[nodiscard]] basic_builder& generate_class_definition(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_function_declarations(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_destructor(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_core_window_functions(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_create_persistent_resources(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_create_device_resources(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_create_sized_resources(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_dpi_functions(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_palette_functions(basic_builder& o, file_contents const& fi);
[[nodiscard]] basic_builder& generate_win_proc(basic_builder& o, file_contents const& fi);
