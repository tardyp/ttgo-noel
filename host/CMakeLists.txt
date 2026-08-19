cmake_minimum_required(VERSION 3.20)
project(ttgo_noel_sdl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG QUIET)
if (NOT SDL2_FOUND)
  find_package(SDL2 REQUIRED)
endif()

set(GAME_CORE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/game_logic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/game_renderer.cpp)
set(GAME_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package(ZLIB REQUIRED)
set(TFT_ESPI_ROOT
  "${CMAKE_CURRENT_SOURCE_DIR}/../.pio/libdeps/lilygo-t-display-s3/TFT_eSPI"
  CACHE PATH "TFT_eSPI source directory")
if (NOT EXISTS "${TFT_ESPI_ROOT}/Fonts/glcdfont.c" OR
    NOT EXISTS "${TFT_ESPI_ROOT}/Fonts/Font16.c")
  message(FATAL_ERROR
    "TFT_eSPI font sources not found. Run PlatformIO first or set -DTFT_ESPI_ROOT=/path/to/TFT_eSPI")
endif()

add_library(ttgo-noel-game-core STATIC ${GAME_CORE})
target_include_directories(ttgo-noel-game-core PUBLIC ${GAME_INCLUDE})

add_executable(ttgo-noel-sdl main.cpp sdl_tft_stub.cpp)
target_link_libraries(ttgo-noel-sdl PRIVATE ttgo-noel-game-core ZLIB::ZLIB)
target_compile_definitions(ttgo-noel-sdl PRIVATE
  TTGO_NOEL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data")
target_include_directories(ttgo-noel-sdl PRIVATE ${TFT_ESPI_ROOT})

if (TARGET SDL2::SDL2)
  target_link_libraries(ttgo-noel-sdl PRIVATE SDL2::SDL2)
else()
  target_include_directories(ttgo-noel-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(ttgo-noel-sdl PRIVATE ${SDL2_LIBRARIES})
endif()

add_executable(ttgo-noel-core-test test_game.cpp)
target_link_libraries(ttgo-noel-core-test PRIVATE ttgo-noel-game-core)

add_executable(ttgo-noel-renderer-test renderer_test.cpp sdl_tft_stub.cpp)
target_link_libraries(ttgo-noel-renderer-test PRIVATE ttgo-noel-game-core ZLIB::ZLIB)
target_include_directories(ttgo-noel-renderer-test PRIVATE ${TFT_ESPI_ROOT})
if (TARGET SDL2::SDL2)
  target_link_libraries(ttgo-noel-renderer-test PRIVATE SDL2::SDL2)
else()
  target_include_directories(ttgo-noel-renderer-test PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(ttgo-noel-renderer-test PRIVATE ${SDL2_LIBRARIES})
endif()

enable_testing()
add_test(NAME ttgo-noel-core-test COMMAND ttgo-noel-core-test)
add_test(NAME ttgo-noel-renderer-test COMMAND ttgo-noel-renderer-test)
