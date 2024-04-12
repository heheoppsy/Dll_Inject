#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <iostream>
#include <cstdio>
#include <string>
#include <thread> 

// Make sure C++ lang standard set 17 or above !
namespace fs = std::filesystem;

int exit() {
    printf("\nUsage: \"Dll_Inject.exe processToInjectTo libraryName\"\n");
    printf("So long...\n");
    return 0;
}

HANDLE getProc(fs::path name) {
    try {
        std::wstring fname = name.wstring();

        DWORD procpid = NULL;
        HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        HANDLE h;

        PROCESSENTRY32W cur;
        cur.dwSize = sizeof cur;

        if (!Process32FirstW(s, &cur)) { return 0; } // oops

        std::wstring wfile;
        do {
            wfile = std::wstring(cur.szExeFile);
            if (wfile == fname) {
                procpid = cur.th32ProcessID;
                std::cout << "Got pid @ " << name << "| ID: " << procpid << std::endl;
                break;
            }
        } while (Process32NextW(s, &cur));
        if (procpid == NULL) {
            std::cout << "Fail to find process " << name << "\nSo long..." << std::endl;
            abort();
        }
        h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procpid);
        std::cout << "Handle: " << h << std::endl;
        return h;
    }
    catch (std::exception e) {
        std::cout << "Fail to get handle\nSo long..." << std::endl;
        abort();
    }
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        return exit();
    }

    fs::path exeArg = argv[1];
    fs::path dllArg = argv[2];

    if (exeArg.has_extension() && exeArg.extension() != ".exe") { return exit(); }
    if (!exeArg.has_extension()) { exeArg += ".exe"; }
    if (dllArg.has_extension() && dllArg.extension() != ".dll") { return exit(); }
    if (!dllArg.has_extension()) { dllArg += ".dll"; }

    std::cout << "[exeArg]: " << exeArg << std::endl;
    std::cout << "[dllArg]: " << dllArg << std::endl;

    if (fs::exists(dllArg)) { std::cout << "[dllArg] Valid" << std::endl; }
    else { std::cout << "\n[dllArg] Invalid" << std::endl; return exit(); }

    HANDLE exe = getProc(exeArg);

    dllArg = fs::absolute(dllArg);
    std::string dll_s = dllArg.string();
    const char* dll_c = dll_s.c_str();

    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    std::cout << "Press enter to attempt injection" << std::endl;
    getchar();

    try {
        LPVOID pDllPath = VirtualAllocEx(exe, 0, strlen(dll_c) + 1, MEM_COMMIT, PAGE_READWRITE);
        WriteProcessMemory(exe, pDllPath, (LPVOID)dll_c, strlen(dll_c) + 1, 0);
        HANDLE hLoadThread = CreateRemoteThread(exe, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pDllPath, 0, 0);
        WaitForSingleObject(hLoadThread, INFINITE);
        VirtualFreeEx(exe, pDllPath, strlen(dll_c) + 1, MEM_RELEASE);
        std::cout << "Succesful!\nSo long..." << std::endl;
    }
    catch (std::exception e) {
        std::cout << "Thread creation failure\nSo long..." << std::endl;
        abort();
    }

    CloseHandle(exe);
    return 0;
}