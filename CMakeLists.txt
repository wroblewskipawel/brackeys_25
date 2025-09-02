cmake_minimum_required(VERSION 3.16)
project(brackeys_25 VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

add_compile_definitions(GLM_FORCE_QUAT_DATA_XYZW)

add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glm/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glad/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glfw/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/json/)

file(GLOB_RECURSE SOURCE ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB IMGUI_SOURCE ${CMAKE_SOURCE_DIR}/extern/imgui/*.cpp)
file(GLOB IMGUI_HEADERS ${CMAKE_SOURCE_DIR}/extern/imgui/*.h)

set(IMGUI_BACKEND_SOURCE ${CMAKE_SOURCE_DIR}/extern/imgui/backends/imgui_impl_opengl3.cpp ${CMAKE_SOURCE_DIR}/extern/imgui/backends/imgui_impl_glfw.cpp)
set(IMGUI_BACKEND_HEADERS ${CMAKE_SOURCE_DIR}/extern/imgui/backends/imgui_impl_opengl3.h ${CMAKE_SOURCE_DIR}/extern/imgui/backends/imgui_impl_glfw.h)

list(APPEND EXTERNAL_LIBS glad glfw glm nlohmann_json::nlohmann_json)
if(UNIX)
    list(APPEND EXTERNAL_LIBS X11 pthread Xrandr Xi)
endif()

find_package(OpenGL REQUIRED)
list(APPEND EXTERNAL_LIBS ${OpenGL_LIBRARIES})

add_executable(
    ${PROJECT_NAME}
    ${SOURCE} ${HEADERS}
    ${IMGUI_SOURCE} ${IMGUI_HEADERS}
    ${IMGUI_BACKEND_SOURCE} ${IMGUI_BACKEND_HEADERS}
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${EXTERNAL_LIBS})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include/"
    "${CMAKE_SOURCE_DIR}/extern/json/include/"
    "${CMAKE_SOURCE_DIR}/extern/fx-gltf/"
    "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
    "${CMAKE_SOURCE_DIR}/extern/glm/"
    "${CMAKE_SOURCE_DIR}/extern/stb/"
    "${CMAKE_SOURCE_DIR}/extern/magic_enum/"
    "${CMAKE_SOURCE_DIR}/extern/imgui/"
    "${CMAKE_SOURCE_DIR}/extern/imgui/backends/"
    "${CMAKE_SOURCE_DIR}/extern/glad/include/"
)

set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders/")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/")

target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
    SHADER_DIR="${SHADER_DIR}"
    ASSETS_DIR="${ASSETS_DIR}"
)

function(copy_directory TARGET_NAME SOURCE_DIR DEST_DIR)
    add_custom_command(
        TARGET ${TARGET_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR} ${DEST_DIR}
    )
endfunction()

copy_directory(
    ${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/assets/
    ${CMAKE_BINARY_DIR}/assets/
)

copy_directory(
    ${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/shaders/
    ${CMAKE_BINARY_DIR}/shaders/
)
