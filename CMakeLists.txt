cmake_minimum_required(VERSION 3.14)

project(PXLTool LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

include_directories("./Source")
include_directories("./Libs")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCE_FILES "Source/*.c" "Source/*.cpp")
file(GLOB_RECURSE HEADER_FILES "Source/*.h" "Source/*.hpp")

set(PROJECT_FILES ${SOURCE_FILES} ${HEADER_FILES})

#ImGui
file(GLOB IMGUI_LIB "Libs/imgui/imgui.h" "Libs/imgui/imgui.cpp" "Libs/imgui/imcongif.h" "Libs/imgui/imgui_demo.cpp" "Libs/imgui/imgui_draw.cpp"
"Libs/imgui/imgui_internal.h" "Libs/imgui/imgui_tables.cpp" "Libs/imgui/imgui_widgets.cpp" "Libs/imgui/imstb_rectpack.h" "Libs/imgui/imstb_textedit.h" "Libs/imgui/imstb_truetype.h" "Libs/imgui/backends/imgui_impl_glfw.h" "Libs/imgui/backends/imgui_impl_glfw.cpp"
"Libs/imgui/misc/cpp/imgui_stdlib.h" "Libs/imgui/misc/cpp/imgui_stdlib.cpp" "Libs/imgui/backends/imgui_impl_opengl3.h" "Libs/imgui/backends/imgui_impl_opengl3.cpp")

add_executable(${PROJECT_NAME} ${PROJECT_FILES} ${IMGUI_LIB})


find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

#GLFW3
find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

#VulkanSDK
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_INCLUDE_DIRS})

#ASSIMP
find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

#Glad
file(GLOB_RECURSE GLAD_FILES "Source/glad/*.h" "Libs/glad/*.hpp" "Libs/glad/*.cpp" "Libs/glad/*.c")
include_directories("./Libs/glad/include")
add_library(GLAD ${GLAD_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE GLAD)

#Crow
add_subdirectory("./Libs/Crow")
include_directories("./Libs/Crow/include")
target_link_libraries(${PROJECT_NAME} PRIVATE Crow)

#CPM
file(
   DOWNLOAD
   https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
   EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

#Saucer
CPMAddPackage(
  NAME           saucer
  VERSION        8.0.4
  GIT_REPOSITORY "https://github.com/saucer/saucer"
)
target_link_libraries(${PROJECT_NAME} PRIVATE saucer::saucer)
