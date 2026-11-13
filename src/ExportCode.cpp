#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

// Конвертує текст із Windows-1251 у UTF-8, якщо файл збережено у старому кодуванні
static std::string EnsureUtf8(const std::string& raw) {
    if (raw.empty()) return raw;

    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, raw.data(), (int)raw.size(), nullptr, 0);
    if (wlen > 0) {
        return raw;
    }

    wlen = MultiByteToWideChar(1251, 0, raw.data(), (int)raw.size(), nullptr, 0);
    if (wlen <= 0) return raw;

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(1251, 0, raw.data(), (int)raw.size(), &wstr[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, nullptr, 0, nullptr, nullptr);
    std::string u8str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, &u8str[0], u8len, nullptr, nullptr);

    return u8str;
}

static bool IsIgnoredPath(const std::filesystem::path& path) {
    for (const auto& part : path) {
        std::string p = part.string();
        if (p == "raylib" || p == "external" || p == "vendor" || p == "third_party" || p == "build") {
            return true;
        }
    }
    std::string fname = path.filename().string();
    return (fname == "raylib.h" || fname == "raymath.h" || fname == "rlgl.h" || fname == "ExportCode.cpp");
}

void ExportProjectCode() {
    namespace fs = std::filesystem;
    fs::path root(PROJECT_ROOT_DIR);

    std::ofstream out(root / "all_code.txt", std::ios::binary);
    if (!out.is_open()) {
        std::cout << "[ExportProjectCode] Error: cannot open " << (root / "all_code.txt") << "\n";
        return;
    }

    // Додаємо UTF-8 BOM для коректного читання кирилиці
    out.write("\xEF\xBB\xBF", 3);

    auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    std::string timestamp = std::format("{:%d.%m.%Y %H:%M:%S}", std::chrono::floor<std::chrono::seconds>(now));

    out << "========================================\r\n"
        << "Останнє оновлення: " << timestamp << "\r\n"
        << "========================================\r\n\r\n";

    auto appendFile = [&](const fs::path& filePath, const std::string& label) {
        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) return;

        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        out << "========================================\r\nFILE: " << label << "\r\n========================================\r\n";
        out << EnsureUtf8(content) << "\r\n\r\n";
        };

    // 1. CMakeLists.txt
    appendFile(root / "CMakeLists.txt", "CMakeLists.txt");

    // 2. Збираємо шляхи з include, src та assets
    std::vector<fs::path> collectedFiles;

    auto collectFromFolder = [&](const fs::path& folder, const std::vector<std::string>& allowedExts) {
        if (!fs::exists(folder)) return;

        for (const auto& entry : fs::recursive_directory_iterator(folder)) {
            if (!entry.is_regular_file()) continue;

            fs::path relPath = fs::relative(entry.path(), root);
            if (IsIgnoredPath(relPath)) continue;

            std::string ext = entry.path().extension().string();
            if (std::find(allowedExts.begin(), allowedExts.end(), ext) != allowedExts.end()) {
                collectedFiles.push_back(entry.path());
            }
        }
        };

    collectFromFolder(root / "include", { ".h", ".hpp" });
    collectFromFolder(root / "src", { ".cpp", ".h", ".hpp" });
    collectFromFolder(root / "assets", { ".fs", ".vs", ".glsl", ".frag", ".vert" });

    // Сортуємо для охайного порядку в файлі
    std::sort(collectedFiles.begin(), collectedFiles.end());

    for (const auto& filePath : collectedFiles) {
        appendFile(filePath, fs::relative(filePath, root).generic_string());
    }

    std::cout << "[ExportProjectCode] Updated all_code.txt (" << collectedFiles.size() + 1 << " files) at " << timestamp << "\n";
}