#include "stdafx.h"
#include "MainWindow.h"
#include <sstream>
#include <iomanip>
#include <fstream>

MainWindow::MainWindow() 
    : m_hwnd(NULL), m_hBtnScan(NULL), m_hBtnReset(NULL), m_hBtnSave(NULL), 
      m_hListDevices(NULL), m_hEditInfo(NULL) {
}

MainWindow::~MainWindow() {
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindow::WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ATAAnalyzerWindowClass";

    if (!RegisterClassEx(&wc)) return false;

    m_hwnd = CreateWindowEx(
        0, L"ATAAnalyzerWindowClass", L"ATA Analyzer (Вариант А7 - Low-Level Ports)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 610, 480,
        NULL, NULL, hInstance, this
    );

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    return true;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = NULL;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (msg) {
    case WM_CREATE: {
        pThis->m_hBtnScan = CreateWindow(L"BUTTON", L"Обновить / Сканировать",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 20, 180, 30, hwnd, (HMENU)IDC_BTN_SCAN, NULL, NULL);

        // Кнопка SRST — строго по методичке
        pThis->m_hBtnReset = CreateWindow(L"BUTTON", L"Soft Reset (SRST)",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            220, 20, 160, 30, hwnd, (HMENU)IDC_BTN_RESET, NULL, NULL);

        pThis->m_hBtnSave = CreateWindow(L"BUTTON", L"Сохранить отчёт",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            400, 20, 160, 30, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);

        CreateWindow(L"STATIC", L"Обнаруженные ATA накопители:", WS_VISIBLE | WS_CHILD,
            20, 65, 250, 20, hwnd, NULL, NULL, NULL);

        pThis->m_hListDevices = CreateWindow(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 85, 560, 90, hwnd, (HMENU)IDC_LIST_DEV, NULL, NULL);

        CreateWindow(L"STATIC", L"Подробные свойства устройства (IDENTIFY DEVICE):", WS_VISIBLE | WS_CHILD,
            20, 185, 400, 20, hwnd, NULL, NULL, NULL);

        pThis->m_hEditInfo = CreateWindow(L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            20, 205, 560, 210, hwnd, (HMENU)IDC_EDIT_INF, NULL, NULL);

        pThis->OnScan();
        return 0;
    }

    case WM_COMMAND: {
        int wId = LOWORD(wParam);
        int wEvent = HIWORD(wParam);

        if (wId == IDC_BTN_SCAN && wEvent == BN_CLICKED) {
            pThis->OnScan();
        } else if (wId == IDC_BTN_RESET && wEvent == BN_CLICKED) {
            pThis->OnReset();
        } else if (wId == IDC_BTN_SAVE && wEvent == BN_CLICKED) {
            pThis->OnSaveReport();
        } else if (wId == IDC_LIST_DEV && wEvent == LBN_SELCHANGE) {
            pThis->OnListSelect();
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::OnScan() {
    SendMessage(m_hListDevices, LB_RESETCONTENT, 0, 0);
    SetWindowTextA(m_hEditInfo, "");

    if (!m_ataController.Initialize()) {
        SetWindowTextA(m_hEditInfo, "ОШИБКА: Драйвер HexIO не запущен!");
        return;
    }

    std::string debugLog;
    m_devices = m_ataController.ScanAllChannels(debugLog);

    if (m_devices.empty()) {
        SendMessageA(m_hListDevices, LB_ADDSTRING, 0, (LPARAM)"Устройства не обнаружены или каналы пусты.");
        SetWindowTextA(m_hEditInfo, debugLog.c_str());
    } else {
        for (size_t i = 0; i < m_devices.size(); ++i) {
            std::ostringstream oss;
            oss << "Канал " << m_devices[i].channel 
                << " | " << (m_devices[i].driveSelect == 0 ? "Master" : "Slave") 
                << " : " << m_devices[i].model;

            SendMessageA(m_hListDevices, LB_ADDSTRING, 0, (LPARAM)oss.str().c_str());
        }
        SendMessage(m_hListDevices, LB_SETCURSEL, 0, 0);
        OnListSelect();
    }
}

// ============================================================
// Обработчик кнопки SRST — строго по методичке А7
// "выполнение программного сброса (SRST) через регистр DC"
// ============================================================
void MainWindow::OnReset() {
    SetWindowTextA(m_hEditInfo, "Выполняется программный сброс (SRST) через регистр DC (0x3F6/0x376)...\r\n"
                                "Бит 2 регистра DC устанавливается в 1, затем снимается.\r\n"
                                "Ожидание готовности устройств ~2 сек...");
    
    // Сброс обоих каналов
    m_ataController.SoftwareReset(0);  // Primary (DC = 0x3F6)
    m_ataController.SoftwareReset(1);  // Secondary (DC = 0x376)
    
    SetWindowTextA(m_hEditInfo, "Программный сброс (SRST) выполнен успешно.\r\n"
                                "Нажмите 'Обновить / Сканировать' для перечитывания устройств.");
}

void MainWindow::OnListSelect() {
    int sel = (int)SendMessage(m_hListDevices, LB_GETCURSEL, 0, 0);
    if (sel != LB_ERR && sel >= 0 && sel < (int)m_devices.size()) {
        const AtaDeviceInfo& dev = m_devices[sel];

        std::ostringstream ss;
        ss << "=== ПАСПОРТ НАКОПИТЕЛЯ (IDENTIFY DEVICE) ===\r\n\r\n"
           << "[Регистр DH] Канал:          " << (dev.channel == 0 ? "Primary (0x1F0-0x1F7, AS=0x3F6)" : "Secondary (0x170-0x177, AS=0x376)") << "\r\n"
           << "[Регистр DH] Режим:          " << (dev.driveSelect == 0 ? "Master (0xA0: бит4=0, бит6=1 LBA)" : "Slave (0xB0: бит4=1, бит6=1 LBA)") << "\r\n"
           << "[Word 27-46] Модель:         " << dev.model << "\r\n"
           << "[Word 10-19] Серийный номер: " << dev.serialNumber << "\r\n"
           << "[Word 23-26] Прошивка:       " << dev.firmwareRev << "\r\n"
           << "[Word 217]   Тип накопителя: " << dev.deviceType << "\r\n"
           << "[Word 49]    Поддержка LBA:  " << (dev.lbaSupported ? "Поддерживается" : "Не поддерживается") << "\r\n"
           << "[Word 60-61/100-103] Всего секторов: " << dev.totalSectors << "\r\n"
           << "             Объем памяти:   " << std::fixed << std::setprecision(2) << dev.sizeInGB << " ГБ\r\n";

        SetWindowTextA(m_hEditInfo, ss.str().c_str());
    }
}

void MainWindow::OnSaveReport() {
    if (m_devices.empty()) {
        MessageBoxA(m_hwnd, "Нет данных для сохранения!", "Внимание", MB_ICONWARNING);
        return;
    }

    std::ofstream out("ATA_Report.txt");
    if (!out.is_open()) {
        MessageBoxA(m_hwnd, "Не удалось создать файл отчёта!", "Ошибка", MB_ICONERROR);
        return;
    }

    out << "===========================================\n";
    out << "   ОТЧЁТ АНАЛИЗАТОРА ATA (Вариант А7)    \n";
    out << "   Низкоуровневый доступ через порты     \n";
    out << "===========================================\n\n";

    for (size_t i = 0; i < m_devices.size(); ++i) {
        const AtaDeviceInfo& dev = m_devices[i];
        out << "Устройство #" << (i + 1) << ":\n"
            << "  Канал:          " << (dev.channel == 0 ? "Primary IDE (0x1F0)" : "Secondary IDE (0x170)") << "\n"
            << "  Режим:          " << (dev.driveSelect == 0 ? "Master" : "Slave") << "\n"
            << "  Модель:         " << dev.model << "\n"
            << "  Серийный номер: " << dev.serialNumber << "\n"
            << "  Прошивка:       " << dev.firmwareRev << "\n"
            << "  Тип:            " << dev.deviceType << "\n"
            << "  LBA:            " << (dev.lbaSupported ? "Да" : "Нет") << "\n"
            << "  Секторов:       " << dev.totalSectors << "\n"
            << "  Объем:          " << std::fixed << std::setprecision(2) << dev.sizeInGB << " GB\n"
            << "-------------------------------------------\n";
    }

    out.close();
    MessageBoxA(m_hwnd, "Отчёт успешно сохранён в файл ATA_Report.txt", "Успех", MB_ICONINFORMATION);
}