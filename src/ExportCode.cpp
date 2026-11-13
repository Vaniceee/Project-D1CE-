#include <filesystem>
#include <fstream>
#include <string>

void ExportProjectCode() {
    namespace fs = std::filesystem;

    // __FILE__ вказує на .../Project-D1CE-/src/ExportProjectCode.cpp
    // Беремо батьківську папку двічі, щоб отримати корінь проєкту на будь-якому ПК
    fs::path root = fs::path(__FILE__).parent_path().parent_path();

    std::ofstream out(root / "all_code.txt", std::ios::binary);
    if (!out.is_open()) return;

    out.write("\xEF\xBB\xBF", 3);

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        std::string path = entry.path().string();

        if (path.find("\\out\\") != std::string::npos ||
            path.find("\\build\\") != std::string::npos ||
            path.find("\\.git\\") != std::string::npos ||
            path.find("\\.vs\\") != std::string::npos ||
            path.find("\\_deps\\") != std::string::npos) {
            continue;
        }

        auto ext = entry.path().extension();
        if (entry.is_regular_file() && (ext == ".cpp" || ext == ".h" || entry.path().filename() == "CMakeLists.txt")) {
            // Записуємо відносний шлях (наприклад, src\Player.cpp), щоб не світити локальні папки
            std::string relPath = fs::relative(entry.path(), root).string();
            std::string header = "========================================\r\nFILE: " + relPath + "\r\n========================================\r\n";
            out << header;
            std::ifstream in(entry.path(), std::ios::binary);
            out << in.rdbuf() << "\r\n\r\n";
        }
    }
}