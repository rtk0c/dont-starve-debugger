#include <string.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <WinSock2.h>
#include <afunix.h>
#include <shellapi.h>

namespace fs = std::filesystem;
using namespace std::literals;

struct ProgramInitCleanup {
    ProgramInitCleanup();
    ~ProgramInitCleanup();
};
ProgramInitCleanup::ProgramInitCleanup() {
    WSADATA wsaData;
    if (int res = WSAStartup(MAKEWORD(2, 2), &wsaData); res != 0) {
        fprintf(stderr, "WSAStartup failed with error %d.\n", res);
        throw std::runtime_error("");
    }
}
ProgramInitCleanup::~ProgramInitCleanup() {
    WSACleanup();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    ProgramInitCleanup _pic;

    int argCount;
    LPWSTR* argList = CommandLineToArgvW(pCmdLine, &argCount);

    fs::path dstPath;
    if (argCount >= 1)
        dstPath.assign(argList[0]);
    else
        dstPath.assign("C:/Program Files (x86)/Steam/steamapps/common/Don't Starve Together");

    sockaddr_un listenAddr;
    listenAddr.sun_family = AF_UNIX;
    auto listenAddrPath = (dstPath / L"data/LuaPanda.sock").string();
    strcpy_s(listenAddr.sun_path, sizeof(listenAddr.sun_path), listenAddrPath.c_str());

    

    // LocalFree(argList);

    return 0;
}
