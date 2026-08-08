cmake_minimum_required(VERSION 3.10)
project(Raytracer)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# OpenGL finden
find_package(OpenGL REQUIRED)

# GLAD einbinden
add_subdirectory(external/glad)

# GLFW einbinden (GLFW bringt eigene CMakeLists.txt mit)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# CGLM einbinden
add_subdirectory(external/cglm)

# Executable
add_executable(Raytracer
    src/main.c
    src/utils.c
    src/shader.c
    src/stb_image.c
    src/stb_image_write.c
    src/camera.c
    src/sphere.c
    src/material.c
)

# Includes
target_include_directories(Raytracer
    PUBLIC external/glad/include/
    PUBLIC headers/
)

# Linken
target_link_libraries(Raytracer
    glfw
    glad
    cglm
    ${OPENGL_gl_LIBRARY}
)
