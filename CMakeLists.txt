cmake_minimum_required(VERSION 3.16)

# =============================================================================
# mp3DurationDetector — Top-level CMake
#
# Может использоваться двумя способами:
#   1) Standalone  — project() + TestCppApp (хост-тест)
#   2) Subdirectory — add_subdirectory() из прошивки
# =============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(mp3DurationDetector LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(MP3_STANDALONE ON)
else()
    set(MP3_STANDALONE OFF)
endif()

# ---------------------------------------------------------------------------
# Core library: DurationMp3Lib  (C/C++ bridge с weak-символами для Rust)
# ---------------------------------------------------------------------------
add_library(DurationMp3Lib STATIC
    mp3_lib.cpp
)

target_include_directories(DurationMp3Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(DurationMp3Lib PRIVATE cxx_std_17)

target_compile_options(DurationMp3Lib PRIVATE
    -Wall
    -Wextra
)

# Если Log-компонент прошивки доступен — используем его
if(TARGET Log)
    target_link_libraries(DurationMp3Lib PUBLIC Log)
else()
    target_compile_definitions(DurationMp3Lib PRIVATE MP3_LIB_NO_LOG)
endif()

# ---------------------------------------------------------------------------
# Rust static library (опционально)
# ---------------------------------------------------------------------------
set(MP3_RUST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mp3DurationDetectorRs")
set(MP3_RUST_LIB_NAME "mp3_duration_detector_rs")

option(MP3_BUILD_RUST "Build Rust library mp3DurationDetectorRs" OFF)
option(MP3_LINK_RUST  "Link pre-built Rust static library"       OFF)

if(MP3_BUILD_RUST)
    # --- Сборка Rust через cargo ---
    if(MP3_RUST_TARGET)
        set(_rust_target_flag "--target" "${MP3_RUST_TARGET}")
        set(MP3_RUST_LIB_PATH
            "${MP3_RUST_DIR}/target/${MP3_RUST_TARGET}/release/lib${MP3_RUST_LIB_NAME}.a")
    else()
        set(_rust_target_flag "")
        set(MP3_RUST_LIB_PATH
            "${MP3_RUST_DIR}/target/release/lib${MP3_RUST_LIB_NAME}.a")
    endif()

    add_custom_command(
        OUTPUT  ${MP3_RUST_LIB_PATH}
        COMMAND cargo build --release ${_rust_target_flag}
        WORKING_DIRECTORY ${MP3_RUST_DIR}
        COMMENT "Building Rust library mp3DurationDetectorRs"
    )
    add_custom_target(mp3_rust_build DEPENDS ${MP3_RUST_LIB_PATH})
    add_dependencies(DurationMp3Lib mp3_rust_build)
    target_link_libraries(DurationMp3Lib PUBLIC
        -Wl,--whole-archive ${MP3_RUST_LIB_PATH} -Wl,--no-whole-archive
    )

elseif(MP3_LINK_RUST)
    # --- Линковка готового .a (путь задан пользователем или по умолчанию) ---
    if(NOT MP3_RUST_LIB_PATH)
        set(MP3_RUST_LIB_PATH
            "${MP3_RUST_DIR}/target/release/lib${MP3_RUST_LIB_NAME}.a")
    endif()

    if(EXISTS "${MP3_RUST_LIB_PATH}")
        target_link_libraries(DurationMp3Lib PUBLIC
            -Wl,--whole-archive ${MP3_RUST_LIB_PATH} -Wl,--no-whole-archive
        )
        message(STATUS "mp3DurationDetector: linking pre-built Rust lib: ${MP3_RUST_LIB_PATH}")
    else()
        message(WARNING "mp3DurationDetector: Rust library not found at ${MP3_RUST_LIB_PATH}, "
                        "weak stubs will be used")
    endif()
endif()

# ---------------------------------------------------------------------------
# TestCppApp (только standalone)
# ---------------------------------------------------------------------------
if(MP3_STANDALONE)
    add_subdirectory(TestCppApp)
endif()
