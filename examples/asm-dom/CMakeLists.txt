cmake_minimum_required (VERSION 3.11)
project(asm-dom-demos)

# Apply some emscripten optimizations flags if we're in release mode.
include(cmake/release-mode.cmake)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Add the nlohmann/json library.
include(cmake/json.cmake)

# Allow the use of Emscripten's Fetch API.
string(APPEND CMAKE_CXX_FLAGS " -s FETCH=1")
# Enable exceptions.
string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0")

# Create a library of common demo functionality.
add_library(demolib STATIC color.cpp dom.cpp)
set_property(TARGET demolib PROPERTY CXX_STANDARD 17)
target_link_libraries(demolib PRIVATE asm-dom)

# Add the demos target.
include_directories(.)
file(GLOB demo_files "demos/*.cpp")
add_executable(demos demos.cpp sandbox.cpp ${demo_files})
string(APPEND CMAKE_CXX_FLAGS
    " -s EXPORTED_FUNCTIONS='[\"_init_demo\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"UTF8ToString\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s MODULARIZE=1")
string(APPEND CMAKE_CXX_FLAGS
    " -s 'EXPORT_NAME=\"AliaDemos\"'")
string(APPEND CMAKE_CXX_FLAGS
    " -s ENVIRONMENT=web")
set_property(TARGET demos PROPERTY CXX_STANDARD 17)
target_link_libraries(demos PRIVATE asm-dom demolib)

# Add the sandbox support files.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/index.html
  ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sandbox.css
  ${CMAKE_CURRENT_BINARY_DIR}/sandbox.css
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pure-material.css
  ${CMAKE_CURRENT_BINARY_DIR}/pure-material.css
)
