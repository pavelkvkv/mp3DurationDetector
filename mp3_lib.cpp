/**
 * @file mp3_lib.cpp
 * @brief C/C++ прокладка к Rust-библиотеке разбора MP3
 *
 * Содержит:
 *  - Реализацию lifecycle API (создание детектора, сессий, анализ)
 *  - Weak-символы mp3_rust_session_*_impl, которые Rust-библиотека
 *    перекрывает при линковке (если не линкуется — вернётся NOT_IMPLEMENTED)
 */

#include "mp3_lib.h"

#ifndef MP3_LIB_NO_LOG
    #undef TAG
    #define TAG "Mp3Lib"
    #define LOG_LEVEL LOG_LEVEL_INFO
    #include "log.h"
#else
    #define logI(...)  ((void)0)
    #define logD(...)  ((void)0)
    #define logW(...)  ((void)0)
    #define logE(...)  ((void)0)
#endif

#include <cstring>

// ============================================================================
// Внутренние структуры
// ============================================================================

struct mp3_detector_t {
    uint8_t reserved;
};

struct mp3_session_t {
    void* rust_session;
};

namespace {
mp3_detector_t g_detector{};
}

// ============================================================================
// Weak-символы — проксируют в Rust blob
// Если Rust .a не слинкован — используются эти заглушки
// ============================================================================

extern "C" {

#ifdef __GNUC__
#define MP3_WEAK __attribute__((weak))
#else
#define MP3_WEAK
#endif

MP3_WEAK mp3_result_t mp3_rust_session_init_impl(
    const mp3_host_api_t* host_api,
    void** out_rust_session
) {
    (void)host_api;
    (void)out_rust_session;
    return MP3_ERR_NOT_IMPLEMENTED;
}

MP3_WEAK mp3_result_t mp3_rust_session_run_impl(
    void* rust_session,
    mp3_audio_info_t* out_info
) {
    (void)rust_session;
    if (out_info) {
        std::memset(out_info, 0, sizeof(*out_info));
    }
    return MP3_ERR_NOT_IMPLEMENTED;
}

MP3_WEAK void mp3_rust_session_deinit_impl(void* rust_session) {
    (void)rust_session;
}

// ============================================================================
// Lifecycle API
// ============================================================================

mp3_detector_t* mp3_detector_create(void) {
    return &g_detector;
}

void mp3_detector_destroy(mp3_detector_t* detector) {
    (void)detector;
}

mp3_detector_t* mp3_detector_instance(void) {
    return &g_detector;
}

mp3_result_t mp3_session_init(
    mp3_detector_t* detector,
    const mp3_host_api_t* host_api,
    mp3_session_t** out_session
) {
    if (!detector || !host_api || !out_session) {
        return MP3_ERR_INVALID_PTR;
    }

    if (!host_api->read_at) {
        return MP3_ERR_INVALID_ARG;
    }

    *out_session = nullptr;

    auto* session = new (std::nothrow) mp3_session_t{};
    if (!session) {
        return MP3_ERR_OUT_OF_MEMORY;
    }

    void* rust_session = nullptr;
    const mp3_result_t init_result =
        mp3_rust_session_init_impl(host_api, &rust_session);
    if (init_result != MP3_OK) {
        delete session;
        return init_result;
    }

    session->rust_session = rust_session;
    *out_session = session;
    return MP3_OK;
}

mp3_result_t mp3_session_run(mp3_session_t* session, mp3_audio_info_t* out_info) {
    if (!session || !out_info) {
        return MP3_ERR_INVALID_PTR;
    }

    std::memset(out_info, 0, sizeof(*out_info));
    return mp3_rust_session_run_impl(session->rust_session, out_info);
}

void mp3_session_deinit(mp3_session_t* session) {
    if (!session) {
        return;
    }

    mp3_rust_session_deinit_impl(session->rust_session);
    delete session;
}

mp3_result_t mp3_analyze(
    mp3_detector_t* detector,
    const mp3_host_api_t* host_api,
    mp3_audio_info_t* out_info
) {
    if (!detector || !host_api || !out_info) {
        return MP3_ERR_INVALID_PTR;
    }

    mp3_session_t* session = nullptr;
    const mp3_result_t init_result =
        mp3_session_init(detector, host_api, &session);
    if (init_result != MP3_OK) {
        return init_result;
    }

    const mp3_result_t run_result = mp3_session_run(session, out_info);
    mp3_session_deinit(session);
    return run_result;
}

const char* mp3_error_string(mp3_result_t result) {
    switch (result) {
        case MP3_OK:                  return "OK";
        case MP3_ERR_INVALID_PTR:     return "Invalid pointer";
        case MP3_ERR_INVALID_ARG:     return "Invalid argument";
        case MP3_ERR_OUT_OF_MEMORY:   return "Out of memory";
        case MP3_ERR_IO:              return "I/O error";
        case MP3_ERR_INVALID_FORMAT:  return "Invalid MP3 format";
        case MP3_ERR_NOT_IMPLEMENTED: return "Rust MP3 blob is not linked";
        case MP3_ERR_INTERNAL:        return "Internal error";
        case MP3_ERR_UNKNOWN:         return "Unknown error";
        default:                      return "Unknown error code";
    }
}

} // extern "C"
