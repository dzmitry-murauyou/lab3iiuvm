#include "stdafx.h"
#include "MainWindow.h"
#include <sstream>
#include <iomanip>
#include <fstream>

MainWindow::MainWindow()
    : m_hwnd(NULL), m_hBtnScan(NULL), m_hBtnReset(NULL), m_hBtnSave(NULL),
      m_hListDevices(NULL), m_hEditInfo(NULL),
      m_hBkBrush(NULL), m_hEditBrush(NULL), m_hFont(NULL), m_hFontMono(NULL) {
}

MainWindow::~MainWindow() {
    if (m_hFont)     DeleteObject(m_hFont);
    if (m_hFontMono) DeleteObject(m_hFontMono);
    if (m_hBkBrush)  DeleteObject(m_hBkBrush);
    if (m_hEditBrush) DeleteObject(m_hEditBrush);
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    // Тёмная тема: фон окна и фон полей ввода/списка
    m_hBkBrush   = CreateSolidBrush(RGB(32, 32, 32));
    m_hEditBrush = CreateSolidBrush(RGB(45, 45, 45));

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindow::WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = m_hBkBrush;
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
        // Шрифты есть в чистой Windows XP: Tahoma и Courier New
        pThis->m_hFont = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
        pThis->m_hFontMono = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");

        // Кнопки рисуем сами (BS_OWNERDRAW), иначе в XP они останутся светлыми
        pThis->m_hBtnScan = CreateWindow(L"BUTTON", L"Обновить / Сканировать",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20, 20, 180, 30, hwnd, (HMENU)IDC_BTN_SCAN, NULL, NULL);
        SendMessage(pThis->m_hBtnScan, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        // Кнопка SRST — строго по методичке
        pThis->m_hBtnReset = CreateWindow(L"BUTTON", L"Soft Reset (SRST)",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            220, 20, 160, 30, hwnd, (HMENU)IDC_BTN_RESET, NULL, NULL);
        SendMessage(pThis->m_hBtnReset, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        pThis->m_hBtnSave = CreateWindow(L"BUTTON", L"Сохранить отчёт",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            400, 20, 160, 30, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        SendMessage(pThis->m_hBtnSave, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        HWND hLabel1 = CreateWindow(L"STATIC", L"Обнаруженные ATA накопители:", WS_VISIBLE | WS_CHILD,
            20, 65, 250, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel1, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        pThis->m_hListDevices = CreateWindow(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 85, 560, 90, hwnd, (HMENU)IDC_LIST_DEV, NULL, NULL);
        SendMessage(pThis->m_hListDevices, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        HWND hLabel2 = CreateWindow(L"STATIC", L"Подробные свойства устройства (IDENTIFY DEVICE):", WS_VISIBLE | WS_CHILD,
            20, 185, 400, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel2, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);

        pThis->m_hEditInfo = CreateWindow(L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            20, 205, 560, 210, hwnd, (HMENU)IDC_EDIT_INF, NULL, NULL);
        SendMessage(pThis->m_hEditInfo, WM_SETFONT, (WPARAM)pThis->m_hFontMono, TRUE);

        pThis->OnScan();
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(220, 220, 220));

        // Read-only EDIT красится через WM_CTLCOLORSTATIC, а не WM_CTLCOLOREDIT
        if ((HWND)lParam == pThis->m_hEditInfo) {
            SetBkColor(hdcStatic, RGB(45, 45, 45));
            return (LRESULT)pThis->m_hEditBrush;
        }

        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)pThis->m_hBkBrush;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(220, 220, 220));
        SetBkColor(hdcEdit, RGB(45, 45, 45));
        return (LRESULT)pThis->m_hEditBrush;
    }

    case WM_DRAWITEM: {
        pThis->OnDrawButton((DRAWITEMSTRUCT*)lParam);
        return TRUE;
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

// Тёмная кнопка: серая заливка, светлая рамка и текст по центру
void MainWindow::OnDrawButton(DRAWITEMSTRUCT* pDis) {
    if (pDis == NULL || pDis->CtlType != ODT_BUTTON) return;

    HDC hdc = pDis->hDC;
    RECT rc = pDis->rcItem;
    BOOL pressed = (pDis->itemState & ODS_SELECTED) != 0;

    HBRUSH hBrush = CreateSolidBrush(pressed ? RGB(85, 85, 85) : RGB(60, 60, 60));
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    HBRUSH hBorder = CreateSolidBrush(RGB(120, 120, 120));
    FrameRect(hdc, &rc, hBorder);
    DeleteObject(hBorder);

    TCHAR text[64] = { 0 };
    GetWindowText(pDis->hwndItem, text, 64);

    HFONT hOldFont = NULL;
    if (m_hFont != NULL) hOldFont = (HFONT)SelectObject(hdc, m_hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 230));
    DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (hOldFont != NULL) SelectObject(hdc, hOldFont);

    if ((pDis->itemState & ODS_FOCUS) != 0) {
        RECT fr = rc;
        InflateRect(&fr, -3, -3);
        DrawFocusRect(hdc, &fr);
    }
}

void MainWindow::OnScan() {
    SendMessage(m_hListDevices, LB_RESETCONTENT, 0, 0);
    SetWindowTextA(m_hEditInfo, "");

    if (!m_ataController.Initialize()) {
        std::string errText = "ОШИБКА: Драйвер HexIO не запущен!";
        if (!m_ataController.GetLastErrorText().empty())
            errText += "\r\n" + m_ataController.GetLastErrorText();
        SetWindowTextA(m_hEditInfo, errText.c_str());
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
    if (!m_ataController.Initialize()) {
        std::string errText = "ОШИБКА: Драйвер HexIO не запущен!";
        if (!m_ataController.GetLastErrorText().empty())
            errText += "\r\n" + m_ataController.GetLastErrorText();
        SetWindowTextA(m_hEditInfo, errText.c_str());
        return;
    }

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
           << "[Регистр DH] Режим:          " << (dev.driveSelect == 0 ? "Master (DH=0xE0: бит4=0, бит6=1 LBA)" : "Slave (DH=0xF0: бит4=1, бит6=1 LBA)") << "\r\n"
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