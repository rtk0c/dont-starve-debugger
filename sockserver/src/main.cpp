#include "pch.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WSADATA wsaData;
    if (int res = WSAStartup(MAKEWORD(2, 2), &wsaData); res != 0) {
        fprintf(stderr, "WSAStartup failed with error %d.\n", res);
        return -1;
    }

    // TODO

    WSACleanup();

    return 0;
}
