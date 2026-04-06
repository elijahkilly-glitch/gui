/*
 * console.cpp - Debug console with colored timestamped output
 */

#include "console.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

static HANDLE g_hConsole = NULL;

static void PrintTimestamp() {
    time_t now = time(nullptr);
    tm t;
    localtime_s(&t, &now);
    printf("[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
}

void Console::Init() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTitleA("Roblox External - Debug Console");
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    printf("========================================\n");
    printf("       Roblox External Debug Console    \n");
    printf("========================================\n\n");
}

void Console::Log(const char* fmt, ...) {
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    PrintTimestamp();
    printf("[INFO] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void Console::Warn(const char* fmt, ...) {
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    PrintTimestamp();
    printf("[WARN] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void Console::Error(const char* fmt, ...) {
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    PrintTimestamp();
    printf("[ERROR] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void Console::Success(const char* fmt, ...) {
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    PrintTimestamp();
    printf("[OK] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}
