//! mp3DurationDetectorRs — Rust-реализация парсера MP3
//!
//! Экспортирует три C-ABI функции, которые перекрывают weak-символы
//! в mp3_lib.cpp и вызываются прокладкой автоматически:
//!
//! - `mp3_rust_session_init_impl`
//! - `mp3_rust_session_run_impl`
//! - `mp3_rust_session_deinit_impl`
//!
//! **Текущая реализация**: заглушки, возвращающие фиксированные значения.

use core::ffi::c_void;

// =============================================================================
// Константы результата (совпадают с mp3_result_t в mp3_lib.h)
// =============================================================================
const MP3_OK: i32 = 0;
const MP3_ERR_INVALID_PTR: i32 = 1;
#[allow(dead_code)]
const MP3_ERR_INVALID_ARG: i32 = 2;
#[allow(dead_code)]
const MP3_ERR_OUT_OF_MEMORY: i32 = 3;
#[allow(dead_code)]
const MP3_ERR_IO: i32 = 4;
#[allow(dead_code)]
const MP3_ERR_INVALID_FORMAT: i32 = 5;
#[allow(dead_code)]
const MP3_ERR_NOT_IMPLEMENTED: i32 = 6;

// =============================================================================
// C-совместимые типы (зеркало mp3_lib.h)
// =============================================================================

/// Информация об аудио — зеркало mp3_audio_info_t
#[repr(C)]
pub struct Mp3AudioInfo {
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u16,
    pub bitrate: u32,
    pub duration_ms: u32,
    pub data_size: u64,
    pub valid: u8,
}

/// Тип callback чтения — зеркало mp3_read_at_fn
type ReadAtFn = unsafe extern "C" fn(
    user_ctx: *mut c_void,
    offset: u64,
    dst: *mut u8,
    requested: usize,
    out_read: *mut usize,
) -> i32;

/// Тип callback аллокации
type AllocFn = unsafe extern "C" fn(user_ctx: *mut c_void, size: usize) -> *mut c_void;

/// Тип callback деаллокации
type FreeFn = unsafe extern "C" fn(user_ctx: *mut c_void, ptr: *mut c_void);

/// Тип callback логирования
type LogFn = unsafe extern "C" fn(user_ctx: *mut c_void, level: i32, msg: *const u8);

/// Набор хост-ручек — зеркало mp3_host_api_t
#[repr(C)]
pub struct Mp3HostApi {
    pub user_ctx: *mut c_void,
    pub source_size: u64,
    pub read_at: Option<ReadAtFn>,
    pub alloc: Option<AllocFn>,
    pub free: Option<FreeFn>,
    pub log: Option<LogFn>,
}

// =============================================================================
// Внутренняя структура сессии
// =============================================================================

struct RustSession {
    /// Копия хост-API для обратных вызовов при анализе
    _host_api: Mp3HostApi,
}

// =============================================================================
// Экспортируемые FFI-функции (перекрывают weak-символы в mp3_lib.cpp)
// =============================================================================

/// Инициализация Rust-сессии
///
/// # Safety
/// Вызывается из C/C++. Указатели должны быть валидны.
#[no_mangle]
pub unsafe extern "C" fn mp3_rust_session_init_impl(
    host_api: *const Mp3HostApi,
    out_rust_session: *mut *mut c_void,
) -> i32 {
    if host_api.is_null() || out_rust_session.is_null() {
        return MP3_ERR_INVALID_PTR;
    }

    // Копируем host_api (POD-структура) для хранения в сессии
    let api_copy = core::ptr::read(host_api);

    let session = Box::new(RustSession {
        _host_api: api_copy,
    });

    *out_rust_session = Box::into_raw(session) as *mut c_void;
    MP3_OK
}

/// Выполнить разбор MP3
///
/// **STUB**: всегда возвращает фиксированные значения.
///
/// # Safety
/// Вызывается из C/C++. Указатели должны быть валидны.
#[no_mangle]
pub unsafe extern "C" fn mp3_rust_session_run_impl(
    rust_session: *mut c_void,
    out_info: *mut Mp3AudioInfo,
) -> i32 {
    if rust_session.is_null() || out_info.is_null() {
        return MP3_ERR_INVALID_PTR;
    }

    let _session = &*(rust_session as *const RustSession);

    // =========================================================================
    // TODO: Реальная реализация:
    //   1. Прочитать данные через _session.host_api.read_at
    //   2. Распарсить MP3-фреймы / Xing / VBRI заголовки
    //   3. Заполнить out_info реальными значениями
    // =========================================================================

    *out_info = Mp3AudioInfo {
        sample_rate: 44100,
        channels: 2,
        bits_per_sample: 16,
        bitrate: 128_000,
        duration_ms: 1000, // stub: всегда 1 секунда
        data_size: 0,
        valid: 1,
    };

    MP3_OK
}

/// Завершить сессию и освободить память
///
/// # Safety
/// Вызывается из C/C++.
#[no_mangle]
pub unsafe extern "C" fn mp3_rust_session_deinit_impl(rust_session: *mut c_void) {
    if !rust_session.is_null() {
        let _ = Box::from_raw(rust_session as *mut RustSession);
    }
}
