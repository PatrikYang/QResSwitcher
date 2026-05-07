#include "qrestray.h"
#include "IniStore.h"
#include "MainDlg.h"

// Globals
HINSTANCE g_hInst        = NULL;
HWND      g_hWnd         = NULL;
Profile   g_profiles[MAX_PROFILES];
int       g_profileCount = 0;
int       g_activeProfile = -1;

// Worker thread for launching a program then restoring resolution
typedef struct {
    int profileIndex;
    QRMODE oldMode;
    char   appPath[MAX_PATH];
    char   args[MAX_PATH];
} LaunchCtx;

static DWORD WINAPI LaunchThread(LPVOID param)
{
    LaunchCtx *ctx = (LaunchCtx *)param;

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    char cmdLine[MAX_PATH * 2];
    wsprintfA(cmdLine, "\"%s\"", ctx->appPath);

    if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        // Restore old resolution
        QUICKRES qr = {0};
        FindQuickResApp(&qr);
        SwitchQResMode(&ctx->oldMode, &qr);
        g_activeProfile = -1;
        UpdateTrayTooltip();
    }

    HeapFree(GetProcessHeap(), 0, ctx);
    return 0;
}

void ApplyProfile(int index)
{
    if (index < 0 || index >= g_profileCount) return;
    Profile *p = &g_profiles[index];

    QRMODE newMode = {0};
    newMode.dwXRes    = p->width;
    newMode.dwYRes    = p->height;
    newMode.wBitsPixel= p->bitsPerPixel;
    newMode.wFreq     = p->refreshRate;

    QRES_PARS pars = {0};
    pars.mNew = newMode;
    CompleteQResPars(&pars);   // fills pars.mOld with current mode

    QUICKRES qr = {0};
    FindQuickResApp(&qr);

    if (pars.dwFlags & QF_NOSWITCH) {
        // Resolution already matches; still launch app if set
    } else {
        SwitchQResMode(&pars.mNew, &qr);
    }

    g_activeProfile = index;
    UpdateTrayTooltip();

    if (p->appPath[0]) {
        if (p->restoreOnExit) {
            LaunchCtx *ctx = (LaunchCtx *)HeapAlloc(GetProcessHeap(), 0, sizeof(LaunchCtx));
            if (ctx) {
                ctx->profileIndex = index;
                ctx->oldMode      = pars.mOld;
                lstrcpyA(ctx->appPath, p->appPath);
                HANDLE hThread = CreateThread(NULL, 0, LaunchThread, ctx, 0, NULL);
                if (hThread) CloseHandle(hThread);
                else HeapFree(GetProcessHeap(), 0, ctx);
            }
        } else {
            STARTUPINFOA si = {0};
            PROCESS_INFORMATION pi = {0};
            si.cb = sizeof(si);
            char cmdLine[MAX_PATH * 2];
            wsprintfA(cmdLine, "\"%s\"", p->appPath);
            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }
    }
}

// ─── Tray Icon ────────────────────────────────────────────────────────────────

void AddTrayIcon(void)
{
    NOTIFYICONDATAA nid = {0};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = g_hWnd;
    nid.uID              = 1;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon            = LoadIconA(g_hInst, MAKEINTRESOURCEA(IDI_TRAY));
    lstrcpyA(nid.szTip, "QResTray");
    Shell_NotifyIconA(NIM_ADD, &nid);
}

void RemoveTrayIcon(void)
{
    NOTIFYICONDATAA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = g_hWnd;
    nid.uID    = 1;
    Shell_NotifyIconA(NIM_DELETE, &nid);
}

void UpdateTrayTooltip(void)
{
    NOTIFYICONDATAA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = g_hWnd;
    nid.uID    = 1;
    nid.uFlags = NIF_TIP;

    if (g_activeProfile >= 0 && g_activeProfile < g_profileCount)
        wsprintfA(nid.szTip, "QResTray — %s", g_profiles[g_activeProfile].name);
    else
        lstrcpyA(nid.szTip, "QResTray");

    Shell_NotifyIconA(NIM_MODIFY, &nid);
}

// ─── Tray Menu ────────────────────────────────────────────────────────────────

void ShowTrayMenu(void)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // Profile items
    for (int i = 0; i < g_profileCount; i++) {
        char label[96];
        lstrcpyA(label, g_profiles[i].name);

        // Append resolution hint
        char hint[32];
        wsprintfA(hint, "  (%lux%lu)", g_profiles[i].width, g_profiles[i].height);
        lstrcatA(label, hint);

        UINT flags = MF_STRING;
        if (i == g_activeProfile) flags |= MF_CHECKED;
        AppendMenuA(hMenu, flags, (UINT_PTR)(ID_PROFILE_BASE + i), label);
    }

    if (g_profileCount > 0) AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_SETTINGS, "设置(&S)...");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "退出(&X)");

    // Must set foreground window or menu won't dismiss correctly
    SetForegroundWindow(g_hWnd);

    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, g_hWnd, NULL);
    PostMessage(g_hWnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

// ─── Auto-start (HKCU Run) ────────────────────────────────────────────────────

#define AUTOSTART_KEY   "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTOSTART_NAME  "QResTray"

void SetAutoStart(BOOL enable)
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    if (enable) {
        char path[MAX_PATH + 4];
        GetModuleFileNameA(NULL, path + 1, MAX_PATH);
        path[0] = '"';
        lstrcatA(path, "\"");
        RegSetValueExA(hKey, AUTOSTART_NAME, 0, REG_SZ,
                       (BYTE *)path, (DWORD)lstrlenA(path) + 1);
    } else {
        RegDeleteValueA(hKey, AUTOSTART_NAME);
    }
    RegCloseKey(hKey);
}

BOOL GetAutoStart(void)
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return FALSE;
    DWORD type, size = 0;
    BOOL  found = (RegQueryValueExA(hKey, AUTOSTART_NAME, NULL, &type, NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return found;
}

// ─── INI Path ─────────────────────────────────────────────────────────────────

void GetIniPath(char *buf, int bufLen)
{
    char appdata[MAX_PATH] = {0};
    if (!GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH))
        GetTempPathA(MAX_PATH, appdata);

    _snprintf(buf, bufLen - 1, "%s\\QResTray\\profiles.ini", appdata);
    buf[bufLen - 1] = '\0';
}

// ─── Window Procedure ─────────────────────────────────────────────────────────

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP)
            ShowTrayMenu();
        else if (lParam == WM_LBUTTONDBLCLK)
            ShowMainDlg();
        break;

    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        if (id >= ID_PROFILE_BASE && id < (UINT)(ID_PROFILE_BASE + g_profileCount)) {
            ApplyProfile((int)(id - ID_PROFILE_BASE));
        } else if (id == ID_TRAY_SETTINGS) {
            ShowMainDlg();
        } else if (id == ID_TRAY_EXIT) {
            RemoveTrayIcon();
            PostQuitMessage(0);
        }
        break;
    }

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ─── WinMain ──────────────────────────────────────────────────────────────────

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrev; (void)lpCmdLine; (void)nCmdShow;

    // Single-instance guard
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "QResTray_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Bring existing window to front via tray message
        CloseHandle(hMutex);
        return 0;
    }

    g_hInst = hInst;

    // Init common controls for ListView
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = APP_CLASS;
    wc.hIcon         = LoadIconA(hInst, MAKEINTRESOURCEA(IDI_TRAY));
    RegisterClassExA(&wc);

    // Create hidden message window
    g_hWnd = CreateWindowExA(0, APP_CLASS, APP_NAME,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              NULL, NULL, hInst, NULL);
    if (!g_hWnd) return 1;

    // Load configuration
    LoadProfiles();

    // Add tray icon
    AddTrayIcon();
    UpdateTrayTooltip();

    // Show main dialog on first run (no profiles yet)
    if (g_profileCount == 0)
        ShowMainDlg();

    // Message loop (handles modeless dialogs too)
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        // Forward to modeless dialog if open
        if (!s_hDlg || !IsDialogMessageA(s_hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    CloseHandle(hMutex);
    return (int)msg.wParam;
}
