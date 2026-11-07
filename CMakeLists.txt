cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

project("music-visualizer")

# SFML

# include(FetchContent)

# FetchContent_Declare(SFML
#   GIT_REPOSITORY https://github.com/SFML/SFML.git
#   GIT_TAG 3.1.0
#   GIT_SHALLOW ON
#   EXCLUDE_FROM_ALL
#   SYSTEM
# )

# FetchContent_MakeAvailable(SFML)

find_package(AudioFile REQUIRED)
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

file(GLOB_RECURSE ALL_SOURCES "src/*.cpp" "include/*.h" "include/*.hpp" "include/*.tpp")

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE AudioFile::AudioFile SFML::Graphics SFML::Audio)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 26)

if (CMAKE_COMPILER_IS_GNUCC)
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()
