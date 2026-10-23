#pragma once
#include <windows.h>
#include <vector>
#include "AtaController.h"

#define IDC_BTN_SCAN  1001
#define IDC_BTN_RESET 1005  // Кнопка SRST (по методичке)
#define IDC_BTN_SAVE  1002
#define IDC_LIST_DEV  1003
#define IDC_EDIT_INF  1004

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    
private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnScan();
    void OnReset();    // Обработчик SRST
    void OnListSelect();
    void OnSaveReport();

    HWND m_hwnd;
    HWND m_hBtnScan;
    HWND m_hBtnReset;  // Кнопка Soft Reset
    HWND m_hBtnSave;
    HWND m_hListDevices;
    HWND m_hEditInfo;

    AtaController m_ataController;
    std::vector<AtaDeviceInfo> m_devices;
};