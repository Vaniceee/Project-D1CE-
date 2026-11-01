#pragma once
#include <iostream>
#include <string>
#include <cstdarg>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#endif

enum class LogLevel {   // показує рівень повідомлення
    DEBUG = 0,          // детальна інформація для розробки (кожен кадр анімації тайл тощо)
    INFO,               // загальні контрольні точки (завантажилась мапа, підключення підсистем)
    WARN,               // потенційна проблема, але гра продовжує працювати (не відображаються номери дверей тощо)
    ERR,                // критичний збій підсистеми гри (відсутній файл текстури тощо)
    FATAL               // неможливість виконання програми через помилку
};

class Logger {
public:
    static void Init() {
#if defined(_WIN32)
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }

    static void SetLogLevel(LogLevel level) {
        GetMinLevel() = level;
    }

    static void Log(LogLevel level, const char* category, const char* format, ...) {
        if (level < GetMinLevel()) return;

        const char* colorCode = "\033[0m";
        const char* levelStr = "INFO";

        switch (level) {
        case LogLevel::DEBUG: colorCode = "\033[90m"; levelStr = "DEBUG"; break;
        case LogLevel::INFO:  colorCode = "\033[32m"; levelStr = "INFO "; break;
        case LogLevel::WARN:  colorCode = "\033[33m"; levelStr = "WARN "; break;
        case LogLevel::ERR:   colorCode = "\033[31m"; levelStr = "ERROR"; break;
        case LogLevel::FATAL: colorCode = "\033[35m"; levelStr = "FATAL"; break;
        }

        std::cout << colorCode << "[" << levelStr << "] [" << category << "] ";

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::cout << buffer << "\033[0m" << std::endl;
    }

private:
    // Зберігає рівень без inline static, повністю сумісно з усіма стандартами C++
    static LogLevel& GetMinLevel() {
        static LogLevel minLevel = LogLevel::DEBUG;
        return minLevel;
    }
};

#define GAME_DEBUG(category, ...) Logger::Log(LogLevel::DEBUG, category, __VA_ARGS__)
#define GAME_INFO(category, ...)  Logger::Log(LogLevel::INFO,  category, __VA_ARGS__)
#define GAME_WARN(category, ...)  Logger::Log(LogLevel::WARN,  category, __VA_ARGS__)
#define GAME_ERROR(category, ...) Logger::Log(LogLevel::ERR,   category, __VA_ARGS__)