cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "")

set(CMAKE_PROJECT_NAME space_invaders)
project(${CMAKE_PROJECT_NAME})

find_package(OpenGL REQUIRED)

file(GLOB SOURCES main.c)
file(GLOB HEADERS )

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set(GLFW_LIB_NAME glfw)
set(GLFW_INC_PATH ThirdParty/${GLFW_LIB_NAME}/include)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(GLFW_VULKAN_STATIC  OFF CACHE BOOL "" FORCE)    # OFF by default
# set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)    # OFF by default
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/${GLFW_LIB_NAME})

# Glad
set(GLAD_LIB_NAME "glad")
set(GLAD_SRC_PATH "ThirdParty/${GLAD_LIB_NAME}/src")
set(GLAD_SRC "${GLAD_SRC_PATH}/glad.c")
set(GLAD_INC_PATH "ThirdParty/${GLAD_LIB_NAME}/include")
add_library(${GLAD_LIB_NAME} STATIC ${GLAD_SRC})
target_include_directories(${GLAD_LIB_NAME} PUBLIC ${GLAD_INC_PATH})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${OPENGL_gl_LIBRARY}
    ${GLFW_LIB_NAME}
    ${GLAD_LIB_NAME}
)

    target_include_directories(${PROJECT_NAME}
    PUBLIC
    external
    ${OPENGL_INCLUDE_DIR}
    ThirdParty
)
