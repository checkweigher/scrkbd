cmake_minimum_required(VERSION 3.14)

project(scrkbd)

# FetchContent is available in CMake 3.11+
# If you're using an older version of CMake, you might need to update it.
include(FetchContent)
include(FindPkgConfig)

if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found!" )
endif()

# Build with debug symbol
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# Set this to the appropriate C++ standard you need (C++11 or later is required for ImGui).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FETCHCONTENT_QUIET OFF)
add_definitions(-DIMGUI_IMPL_OPENGL_ES2)

# Fetch ImGui from the git repository
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90-docking
    PATCH_COMMAND  bash -c "git apply ${CMAKE_SOURCE_DIR}/patches/imgui-*.patch"
    UPDATE_DISCONNECTED 1
)

# Make available to the rest of the project
FetchContent_MakeAvailable(imgui)

# Add the ImGui library
add_library(imgui STATIC 
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

# Include directories for ImGui
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Find the package for GLFW which we will use to create the window and OpenGL context
find_package(glfw3 REQUIRED)
pkg_check_modules(glfw3 REQUIRED IMPORTED_TARGET glfw3)

# Link ImGui with GLFW
target_link_libraries(imgui PUBLIC PkgConfig::glfw3)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
# add additional link directories (e.g., if compiled from source, some libraries are installed to /usr/local/)
link_directories(/usr/local/lib64)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)

# Your executable
add_executable(scrkbd
    src/main.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
target_include_directories(scrkbd PUBLIC 
    /usr
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)

# Link your executable with ImGui
target_link_libraries(scrkbd PRIVATE imgui GLESv2 pthread) 

# Assume we have an 'assets' folder in the source directory
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

# The destination directory is the build output directory (e.g., CMAKE_BINARY_DIR)
set(ASSETS_DEST_DIR "${CMAKE_BINARY_DIR}/assets")

# Add a custom command that copies the assets to the output directory
add_custom_command(
  TARGET scrkbd POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${ASSETS_DIR}
          ${ASSETS_DEST_DIR}
  COMMENT "Copying assets to build directory"
)

# Specify the target to install and the destination directory
install(TARGETS scrkbd DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)
