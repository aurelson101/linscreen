#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace {
std::wstring currentExecutablePath()
{
    std::vector<wchar_t> buffer(512);
    while (true) {
        const DWORD length = GetModuleFileNameW(
          nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring quoteArgument(const std::wstring& argument)
{
    if (!argument.empty() &&
        argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
        } else if (character == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted += character;
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted += character;
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted += L'\"';
    return quoted;
}

int callLinScreen(int argc, wchar_t* argv[], bool wait)
{
    std::wstring wrapperPath = currentExecutablePath();
    const size_t separator = wrapperPath.find_last_of(L"\\/");
    if (wrapperPath.empty() || separator == std::wstring::npos) {
        std::wcerr << L"Unable to determine the LinScreen installation path\n";
        return 1;
    }

    const std::wstring directory = wrapperPath.substr(0, separator);
    const std::wstring application = directory + L"\\linscreen.exe";
    std::wstring commandLine = quoteArgument(application);
    for (int i = 1; i < argc; ++i) {
        commandLine += L' ';
        commandLine += quoteArgument(argv[i]);
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(application.c_str(),
                        commandLine.data(),
                        nullptr,
                        nullptr,
                        TRUE,
                        0,
                        nullptr,
                        directory.c_str(),
                        &startupInfo,
                        &processInfo)) {
        std::wcerr << L"Unable to start LinScreen (Windows error "
                   << GetLastError() << L")\n";
        return 1;
    }

    CloseHandle(processInfo.hThread);
    if (!wait) {
        CloseHandle(processInfo.hProcess);
        return 0;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
}
}

// Console wrapper for LinScreen on Windows.
int wmain(int argc, wchar_t* argv[])
{
    if (argc == 1) {
        std::cout << "Starting linscreen in daemon mode" << std::endl;
    }
    const int exitCode = callLinScreen(argc, argv, argc > 1);
    std::cout.flush();
    return exitCode;
}
