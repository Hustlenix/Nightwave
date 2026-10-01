cmake_minimum_required(VERSION 3.16)
project(nightwave_host_tests LANGUAGES C CXX)

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
include(FetchContent)
# Same decoder revision bundled by esp-libhelix-mp3 1.0.3. Keep upstream notices.
FetchContent_Declare(helix_source
    GIT_REPOSITORY https://github.com/chmorgan/libhelix-mp3.git
    GIT_TAG f4430799e70a4fff44513e71bd4459411802b819)
FetchContent_MakeAvailable(helix_source)
file(GLOB helix_sources "${helix_source_SOURCE_DIR}/*.c" "${helix_source_SOURCE_DIR}/real/*.c")
add_library(nightwave_host_helix STATIC ${helix_sources})
target_include_directories(nightwave_host_helix PUBLIC "${helix_source_SOURCE_DIR}/pub"
    PRIVATE "${helix_source_SOURCE_DIR}/real")
if(NOT MSVC)
    target_compile_options(nightwave_host_helix PRIVATE
        -include "${CMAKE_CURRENT_LIST_DIR}/helix_host_math.h")
endif()
add_executable(nightwave_mp3_tests mp3_tests.cpp
    ../firmware/components/decoder/mp3_decoder.cpp)
target_include_directories(nightwave_mp3_tests PRIVATE
    ../firmware/components/decoder/include ../firmware/components/audio_pipeline/include)
target_link_libraries(nightwave_mp3_tests PRIVATE nightwave_host_helix)
if(MSVC)
    target_compile_options(nightwave_mp3_tests PRIVATE /W4 /WX)
else()
    target_compile_options(nightwave_mp3_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_mp3_tests COMMAND nightwave_mp3_tests
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-cbr.mp3
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-vbr.mp3)
add_executable(nightwave_audio_sink_tests audio_sink_tests.cpp
    ../firmware/components/audio_i2s/audio_sink.cpp)
target_include_directories(nightwave_audio_sink_tests PRIVATE
    fakes ../firmware/components/audio_i2s/include
    ../firmware/components/audio_pipeline/include
    ../firmware/components/app_state/include)
if(MSVC)
    target_compile_options(nightwave_audio_sink_tests PRIVATE /W4 /WX)
else()
    target_compile_options(nightwave_audio_sink_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_audio_sink_tests COMMAND nightwave_audio_sink_tests)
add_test(
    NAME nightwave_host_tests
    COMMAND nightwave_host_tests ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/02_left_right_stereo_44100.wav
)
