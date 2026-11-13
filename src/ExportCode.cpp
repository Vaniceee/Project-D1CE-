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

void ExportProjectCode() {
    namespace fs = std::filesystem;
    fs::path root(PROJECT_ROOT_DIR);

    std::ofstream out(root / "all_code.txt", std::ios::binary);
    if (!out.is_open()) return;

    // Додаємо UTF-8 BOM
    out.write("\xEF\xBB\xBF", 3);

    // Отримуємо поточну локальну дату і час
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

    appendFile(root / "CMakeLists.txt", "CMakeLists.txt");

    std::vector<fs::path> folders = { root / "include", root / "src" };
    for (const auto& folder : folders) {
        if (!fs::exists(folder)) continue;

        for (const auto& entry : fs::recursive_directory_iterator(folder)) {
            auto ext = entry.path().extension();
            if (entry.is_regular_file() && (ext == ".cpp" || ext == ".h")) {
                appendFile(entry.path(), fs::relative(entry.path(), root).string());
            }
        }
    }
}