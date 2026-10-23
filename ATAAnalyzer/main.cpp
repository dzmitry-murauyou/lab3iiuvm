#include "stdafx.h"
#include "MainWindow.h"
#include <string.h>

// HexIOWrapper в своём конструкторе строит путь к драйверу из ТЕКУЩЕГО
// каталога: GetCurrentDirectory() + "\hexio.sys". Поэтому до создания
// главного окна (в нём живёт AtaController с HexIOWrapper) делаем текущим
// каталогом папку с exe - HexIO.sys будет искаться рядом с программой,
// откуда бы её ни запустили.
static void SetWorkingDirectoryToExe()
{
    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) return;
    char* slash = strrchr(path, '\\');
    if (slash != NULL)
    {
        *slash = '\0';
        SetCurrentDirectoryA(path);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetWorkingDirectoryToExe();

    MainWindow window;
    if (!window.Create(hInstance, nCmdShow)) {
        return 0;
    }

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}