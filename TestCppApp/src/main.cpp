/**
 * @file main.cpp
 * @brief Хост-тест mp3DurationDetector
 *
 * Прогоняет все .mp3 файлы из папки test_audio через mp3_analyze()
 * и выводит результат в табличном виде.
 *
 * Использование:
 *   ./TestCppApp                   — сканирует TEST_AUDIO_DIR (compile-time)
 *   ./TestCppApp /path/to/audio    — сканирует указанную папку
 */

#include "mp3_lib.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ============================================================================
// Host-callback: чтение файла через stdio
// ============================================================================

struct FileReadContext {
    FILE* fp;
    uint64_t fileSize;
};

static mp3_result_t host_read_at(
    void* user_ctx,
    uint64_t offset,
    uint8_t* dst,
    size_t requested,
    size_t* out_read
) {
    auto* ctx = static_cast<FileReadContext*>(user_ctx);
    if (!ctx || !ctx->fp) return MP3_ERR_INVALID_PTR;

    if (fseeko(ctx->fp, static_cast<off_t>(offset), SEEK_SET) != 0) {
        return MP3_ERR_IO;
    }

    size_t rd = fread(dst, 1, requested, ctx->fp);
    if (out_read) *out_read = rd;

    return MP3_OK;
}

// ============================================================================
// Утилита: проверка расширения .mp3 (case-insensitive)
// ============================================================================

static bool isMp3(const fs::path& p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp3";
}

// ============================================================================
// Вывод результата анализа одного файла
// ============================================================================

struct TestResult {
    std::string name;
    bool ok;
    mp3_result_t code;
    mp3_audio_info_t info;
};

static TestResult analyzeFile(mp3_detector_t* detector, const fs::path& filePath) {
    TestResult r{};
    r.name = filePath.filename().string();
    r.code = MP3_ERR_IO;
    r.ok   = false;
    std::memset(&r.info, 0, sizeof(r.info));

    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        return r;
    }

    fseeko(fp, 0, SEEK_END);
    uint64_t fileSize = static_cast<uint64_t>(ftello(fp));
    fseeko(fp, 0, SEEK_SET);

    FileReadContext ctx{fp, fileSize};

    mp3_host_api_t host_api{};
    host_api.user_ctx    = &ctx;
    host_api.source_size = fileSize;
    host_api.read_at     = host_read_at;

    r.code = mp3_analyze(detector, &host_api, &r.info);
    r.ok   = (r.code == MP3_OK && r.info.valid);

    fclose(fp);
    return r;
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[]) {
#ifdef TEST_AUDIO_DIR
    const char* defaultDir = TEST_AUDIO_DIR;
#else
    const char* defaultDir = "../test_audio";
#endif

    const char* audioDir = (argc > 1) ? argv[1] : defaultDir;

    printf("=== mp3DurationDetector — TestCppApp ===\n");
    printf("Audio directory: %s\n\n", audioDir);

    if (!fs::exists(audioDir) || !fs::is_directory(audioDir)) {
        fprintf(stderr, "ERROR: directory '%s' does not exist\n", audioDir);
        return 1;
    }

    // Собираем .mp3 файлы
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(audioDir)) {
        if (entry.is_regular_file() && isMp3(entry.path())) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    if (files.empty()) {
        printf("No .mp3 files found in %s\n", audioDir);
        return 0;
    }

    printf("Found %zu MP3 file(s)\n\n", files.size());

    // Получаем singleton-детектор
    mp3_detector_t* detector = mp3_detector_instance();

    // Шапка таблицы
    printf("%-50s  %8s  %8s  %4s  %8s  %s\n",
           "FILE", "DURATION", "RATE", "CH", "BITRATE", "STATUS");
    printf("%-50s  %8s  %8s  %4s  %8s  %s\n",
           std::string(50, '-').c_str(), "--------", "--------",
           "----", "--------", "------");

    int passed = 0;
    int failed = 0;

    for (const auto& filePath : files) {
        auto r = analyzeFile(detector, filePath);

        if (r.ok) {
            printf("%-50s  %5u ms  %5u Hz  %4u  %6u bp  OK\n",
                   r.name.c_str(),
                   r.info.duration_ms,
                   r.info.sample_rate,
                   r.info.channels,
                   r.info.bitrate);
            passed++;
        } else {
            printf("%-50s  %8s  %8s  %4s  %8s  FAIL [%s]\n",
                   r.name.c_str(), "-", "-", "-", "-",
                   mp3_error_string(r.code));
            failed++;
        }
    }

    printf("\n--- Results: %d passed, %d failed, %d total ---\n",
           passed, failed, passed + failed);

    return (failed > 0) ? 1 : 0;
}
