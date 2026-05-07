#ifndef QRESTRAY_H
#define QRESTRAY_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>

#include "..\qreslib\qreslib.h"
#include "resource.h"

#define WM_TRAYICON     (WM_USER + 1)
#define MAX_PROFILES    32
#define APP_NAME        "QResTray"
#define APP_CLASS       "QResTrayWnd"

typedef struct {
    char  name[64];
    DWORD width;
    DWORD height;
    UINT  bitsPerPixel;
    UINT  refreshRate;
    char  appPath[MAX_PATH];
    BOOL  restoreOnExit;
} Profile;

// Globals defined in qrestray.cpp
extern HINSTANCE g_hInst;
extern HWND      g_hWnd;
extern Profile   g_profiles[MAX_PROFILES];
extern int       g_profileCount;
extern int       g_activeProfile;

// Core functions (qrestray.cpp)
void ApplyProfile(int index);
void AddTrayIcon(void);
void RemoveTrayIcon(void);
void UpdateTrayTooltip(void);
void ShowTrayMenu(void);
void SetAutoStart(BOOL enable);
BOOL GetAutoStart(void);
void GetIniPath(char *buf, int bufLen);

// Main dialog (MainDlg.cpp)
void ShowMainDlg(void);

#endif
