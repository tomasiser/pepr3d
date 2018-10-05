
#ifdef _TEST_
#include <gtest/gtest.h>

#if _WIN32 || _WIN64
#include <WinBase.h>
#include <shellapi.h>
#include <vector>
#include <memory>

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    LPWSTR* str = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::unique_ptr<char[]>> argBuffer;
    for(int i = 0; i < argc; i++) {
        const size_t stringSize = wcslen(str[i]);
        argBuffer.emplace_back(std::make_unique<char[]>(stringSize));
        size_t bytesWritten = 0;
        wcstombs(argBuffer.at(i).get(), str[i], stringSize);
    }

    std::vector<char*> argv;
    for(auto& a : argBuffer)
        argv.push_back(a.get());

    ::testing::InitGoogleTest(&argc, argv.data());
    return RUN_ALL_TESTS();
}

#endif
#endif