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
find_package(Threads REQUIRED)
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
add_executable(nightwave_streaming_tests streaming_tests.cpp
    ../firmware/components/playback/streaming_player.cpp
    ../firmware/components/decoder/mp3_decoder.cpp
    ../firmware/components/decoder/wav_parser.cpp)
target_include_directories(nightwave_streaming_tests PRIVATE fakes
    ../firmware/components/playback/include ../firmware/components/decoder/include
    ../firmware/components/audio_pipeline/include ../firmware/components/audio_i2s/include
    ../firmware/components/app_state/include ../firmware/components/ui/include)
target_link_libraries(nightwave_streaming_tests PRIVATE nightwave_host_helix Threads::Threads)
if(NOT MSVC)
    target_compile_options(nightwave_streaming_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_streaming_tests COMMAND nightwave_streaming_tests
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/02_left_right_stereo_44100.wav
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-cbr.mp3)
add_executable(nightwave_frontend_tests frontend_tests.cpp
    ../firmware/components/ui/player_frontend.cpp
    ../firmware/components/library/media_documents.cpp)
target_include_directories(nightwave_frontend_tests PRIVATE fakes
    ../firmware/components/playback/include ../firmware/components/decoder/include
    ../firmware/components/audio_pipeline/include ../firmware/components/audio_i2s/include
    ../firmware/components/app_state/include ../firmware/components/ui/include
    ../firmware/components/storage/include ../firmware/components/settings/include
    ../firmware/components/library/include
    ../firmware/components/input/include)
if(NOT MSVC)
    target_compile_options(nightwave_frontend_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_frontend_tests COMMAND nightwave_frontend_tests
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media)
add_executable(nightwave_product_adapter_tests product_adapter_tests.cpp
    ../firmware/components/ui/oled_display.cpp
    ../firmware/components/settings/settings.cpp)
target_include_directories(nightwave_product_adapter_tests PRIVATE fakes
    ../firmware/components/ui/include ../firmware/components/settings/include
    ../firmware/components/app_state/include)
if(NOT MSVC)
    target_compile_options(nightwave_product_adapter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_product_adapter_tests COMMAND nightwave_product_adapter_tests)
add_executable(nightwave_media_tests media_tests.cpp ../firmware/components/library/media_documents.cpp)
target_include_directories(nightwave_media_tests PRIVATE ../firmware/components/library/include)
if(NOT MSVC)
    target_compile_options(nightwave_media_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_media_tests COMMAND nightwave_media_tests ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media)
if(MSVC)
    target_compile_options(nightwave_mp3_tests PRIVATE /W4 /WX)
else()
    target_compile_options(nightwave_mp3_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME nightwave_mp3_tests COMMAND nightwave_mp3_tests
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-cbr.mp3
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-vbr.mp3
    ${CMAKE_CURRENT_LIST_DIR}/../.generated/test-media/test-mpeg2.mp3)
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
