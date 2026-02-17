/**
 * @file mp3_lib.h
 * @brief ABI-контракт между C/C++ кодом прошивки и Rust-библиотекой разбора MP3
 *
 * Назначение:
 * - C/C++ сторона предоставляет Rust библиотеке ручки чтения данных и памяти.
 * - Rust сторона выполняет разбор MP3 и возвращает результат.
 * - C/C++ пользователь работает через простой lifecycle: init/run/deinit.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque-типы
// ============================================================================

struct mp3_detector_t;
typedef struct mp3_detector_t mp3_detector_t;

struct mp3_session_t;
typedef struct mp3_session_t mp3_session_t;

// ============================================================================
// Результат анализа
// ============================================================================

typedef struct {
    uint32_t sample_rate;       ///< Частота дискретизации (Hz)
    uint16_t channels;          ///< Количество каналов (моно=1, стерео=2)
    uint16_t bits_per_sample;   ///< Бит на сэмпл (8, 16, 24, 32)
    uint32_t bitrate;           ///< Битрейт (bps)
    uint32_t duration_ms;       ///< Длительность в миллисекундах
    uint64_t data_size;         ///< Размер аудиоданных (байт)
    uint8_t valid;              ///< Информация валидна (1=да, 0=нет)
} mp3_audio_info_t;

typedef enum {
    MP3_OK = 0,
    MP3_ERR_INVALID_PTR = 1,
    MP3_ERR_INVALID_ARG = 2,
    MP3_ERR_OUT_OF_MEMORY = 3,
    MP3_ERR_IO = 4,
    MP3_ERR_INVALID_FORMAT = 5,
    MP3_ERR_NOT_IMPLEMENTED = 6,
    MP3_ERR_INTERNAL = 7,
    MP3_ERR_UNKNOWN = 255,
} mp3_result_t;

// ============================================================================
// Callback-API хоста (прошивка -> Rust)
// ============================================================================

/**
 * @brief Прочитать диапазон данных источника
 *
 * @param user_ctx Пользовательский контекст хоста
 * @param offset Смещение в байтах от начала источника
 * @param dst Буфер назначения
 * @param requested Сколько байт запросила Rust-библиотека
 * @param out_read Сколько реально считано
 * @return MP3_OK при успехе
 */
typedef mp3_result_t (*mp3_read_at_fn)(
    void* user_ctx,
    uint64_t offset,
    uint8_t* dst,
    size_t requested,
    size_t* out_read
);

/**
 * @brief Выделить память для Rust-библиотеки
 */
typedef void* (*mp3_alloc_fn)(void* user_ctx, size_t size);

/**
 * @brief Освободить память, выделенную через mp3_alloc_fn
 */
typedef void (*mp3_free_fn)(void* user_ctx, void* ptr);

/**
 * @brief Логирование из Rust в хост (опционально)
 */
typedef void (*mp3_log_fn)(void* user_ctx, int level, const char* msg);

/**
 * @brief Набор ручек, передаваемых в Rust-библиотеку
 */
typedef struct {
    void* user_ctx;                 ///< Контекст источника данных (файл/буфер/стрим)
    uint64_t source_size;           ///< Полный размер источника (0 если неизвестен)
    mp3_read_at_fn read_at;         ///< Обязательная ручка чтения
    mp3_alloc_fn alloc;             ///< Опционально, если NULL Rust использует свой alloc
    mp3_free_fn free;               ///< Опционально, парная к alloc
    mp3_log_fn log;                 ///< Опционально
} mp3_host_api_t;

// ============================================================================
// Lifecycle пользовательского API (стабильный вход в Rust blob)
// ============================================================================

mp3_detector_t* mp3_detector_create(void);

void mp3_detector_destroy(mp3_detector_t* detector);

mp3_detector_t* mp3_detector_instance(void);

/**
 * @brief Инициализировать сессию анализа
 *
 * @param detector Детектор, полученный через create/instance
 * @param host_api Набор callback-ручек хоста
 * @param out_session Выходная сессия
 * @return MP3_OK при успехе
 */
mp3_result_t mp3_session_init(
    mp3_detector_t* detector,
    const mp3_host_api_t* host_api,
    mp3_session_t** out_session
);

/**
 * @brief Выполнить анализ MP3 в Rust-библиотеке
 */
mp3_result_t mp3_session_run(mp3_session_t* session, mp3_audio_info_t* out_info);

/**
 * @brief Завершить работу сессии и освободить ресурсы
 */
void mp3_session_deinit(mp3_session_t* session);

/**
 * @brief Удобная одношаговая функция: init -> run -> deinit
 */
mp3_result_t mp3_analyze(
    mp3_detector_t* detector,
    const mp3_host_api_t* host_api,
    mp3_audio_info_t* out_info
);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * @brief Конвертировать код ошибки в человекочитаемую строку
 */
const char* mp3_error_string(mp3_result_t result);

// ============================================================================
// Константы для Rust FFI
// ============================================================================

#define MP3_RESULT_OK 0

#ifdef __cplusplus
}
#endif
