# mp3DurationDetector

Библиотека определения длительности MP3-файлов, разделённая на C/C++ прокладку
и Rust-реализацию парсера.

## Структура

```
mp3DurationDetector/
├── CMakeLists.txt              # Сборка библиотеки (standalone / subdirectory)
├── mp3_lib.h                   # ABI-контракт (C header)
├── mp3_lib.cpp                 # C/C++ bridge с weak-символами
├── mp3DurationDetectorRs/      # Rust-библиотека (staticlib)
│   ├── Cargo.toml
│   └── src/lib.rs              # FFI-экспорт (пока заглушки)
├── TestCppApp/                 # Хост-тест
│   ├── CMakeLists.txt
│   └── src/main.cpp
└── test_audio/                 # Тестовые MP3-файлы
```

## Архитектура

```
┌─────────────────────────────────┐
│  C/C++ пользователь             │
│  (Mp3Detector / TestCppApp)     │
│                                 │
│  mp3_analyze(detector,          │
│              host_api,          │
│              &out_info)         │
└────────────┬────────────────────┘
             │  mp3_lib.h ABI
             ▼
┌─────────────────────────────────┐
│  mp3_lib.cpp                    │
│  (C/C++ bridge)                 │
│                                 │
│  mp3_rust_session_*_impl()      │
│  ← weak symbol (заглушка)       │
│  ← перекрыт Rust .a при линке   │
└────────────┬────────────────────┘
             │  weak override
             ▼
┌─────────────────────────────────┐
│  mp3DurationDetectorRs          │
│  (Rust staticlib)               │
│                                 │
│  Парсинг MP3 фреймов,           │
│  Xing/VBRI заголовков           │
└─────────────────────────────────┘
```

## Использование в прошивке

Добавить как git submodule:

```bash
cd ML_AT32_FW_2026
git submodule add <url> Components/FilesIndexer/mp3DurationDetector
```

В `FilesIndexer/CMakeLists.txt`:
```cmake
add_subdirectory(mp3DurationDetector)
target_link_libraries(FilesIndexer PUBLIC DurationMp3Lib)
```

## Сборка TestCppApp (хост)

```bash
# 1. Собрать Rust-библиотеку
cd mp3DurationDetectorRs && cargo build --release && cd ..

# 2. Собрать TestCppApp
cmake -B build -DMP3_BUILD_RUST=OFF
cmake --build build

# 3. Запустить тесты
./build/TestCppApp/TestCppApp
```

## Сборка без Rust (weak stubs)

Если Rust blob не слинкован, все вызовы `mp3_analyze()` вернут
`MP3_ERR_NOT_IMPLEMENTED` — прошивка запустится, но MP3-длительность
не определится.
