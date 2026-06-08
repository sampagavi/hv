cmake_minimum_required(VERSION 3.20)
project(llm-tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ftxui via FetchContent ────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v5.0.0
)
FetchContent_MakeAvailable(ftxui)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/buffer/ring_buffer.cpp
    src/hooks/interceptor.cpp
    src/anomaly/detector.cpp
    src/simulator/mock_model.cpp
    src/replay/serializer.cpp
    src/replay/replayer.cpp
    src/tui/app.cpp
    src/tui/topology_panel.cpp
    src/tui/stream_panel.cpp
    src/tui/attention_panel.cpp
    src/tui/metrics_panel.cpp
    src/tui/anomaly_panel.cpp
)

add_executable(llm-tracer ${SOURCES})

target_include_directories(llm-tracer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(llm-tracer PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)

if(UNIX AND NOT APPLE)
    target_link_libraries(llm-tracer PRIVATE pthread)
endif()

# ── Build type defaults ───────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
