cmake_minimum_required(VERSION 3.20)
project(lavender_gg CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.9-docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

add_executable(lavender_gg WIN32
    src/main.cpp
    src/ui.cpp
    src/bridge.cpp
    src/capture.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(lavender_gg PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${httplib_SOURCE_DIR}
    ${json_SOURCE_DIR}/include
    src
)

target_link_libraries(lavender_gg PRIVATE
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    ws2_32.lib
)

target_compile_definitions(lavender_gg PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    CPPHTTPLIB_THREAD_POOL_COUNT=2
)
