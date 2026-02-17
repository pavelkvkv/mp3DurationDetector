cmake_minimum_required(VERSION 3.16)

project(TestCppApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Пути к Rust-библиотеке
# ---------------------------------------------------------------------------
set(RUST_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../mp3DurationDetectorRs")
set(RUST_LIB_NAME "mp3_duration_detector_rs")
set(RUST_LIB_PATH "${RUST_LIB_DIR}/target/release/lib${RUST_LIB_NAME}.a")

# ---------------------------------------------------------------------------
# Тестовое приложение
# ---------------------------------------------------------------------------
add_executable(TestCppApp
    src/main.cpp
)

target_include_directories(TestCppApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../          # mp3_lib.h
)

# Линкуем DurationMp3Lib (C/C++ bridge), у нас он доступен как target из parent
if(TARGET DurationMp3Lib)
    target_link_libraries(TestCppApp PRIVATE DurationMp3Lib)
else()
    # Standalone fallback — компилируем mp3_lib.cpp напрямую
    target_sources(TestCppApp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../mp3_lib.cpp
    )
    target_compile_definitions(TestCppApp PRIVATE MP3_LIB_NO_LOG)
endif()

# Линкуем Rust static library (если собрана)
# --whole-archive нужен, чтобы Rust-символы перекрыли weak-заглушки в mp3_lib.cpp
if(EXISTS "${RUST_LIB_PATH}")
    target_link_libraries(TestCppApp PRIVATE
        -Wl,--whole-archive ${RUST_LIB_PATH} -Wl,--no-whole-archive
    )
    message(STATUS "TestCppApp: linking Rust lib from ${RUST_LIB_PATH}")
else()
    message(STATUS "TestCppApp: Rust lib not found, weak stubs will be used")
endif()

# Linux зависимости для Rust runtime
if(UNIX AND NOT APPLE)
    target_link_libraries(TestCppApp PRIVATE pthread dl m)
endif()

# ---------------------------------------------------------------------------
# Путь к test_audio по умолчанию
# ---------------------------------------------------------------------------
target_compile_definitions(TestCppApp PRIVATE
    TEST_AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test_audio"
)
