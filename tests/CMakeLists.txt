cmake_minimum_required(VERSION 3.16)
project(nightwave_host_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    nightwave_host_tests
    host_tests.cpp
    ../firmware/components/decoder/wav_parser.cpp
)
target_include_directories(
    nightwave_host_tests
    PRIVATE
        ../firmware/components/audio_pipeline/include
        ../firmware/components/decoder/include
        ../firmware/components/input/include
        ../firmware/components/playback/include
        ../firmware/components/app_state/include
)
if(MSVC)
    target_compile_options(nightwave_host_tests PRIVATE /W4 /WX)
else()
    target_compile_options(nightwave_host_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(
    NAME nightwave_host_tests
    COMMAND nightwave_host_tests ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/02_left_right_stereo_44100.wav
)
