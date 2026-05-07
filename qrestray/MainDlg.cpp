#include "MainDlg.h"
#include "ProfileDlg.h"
#include "IniStore.h"

HWND s_hDlg = NULL;  // accessed via extern in qrestray.cpp message loop

// Column indices in the ListView
#define COL_NAME    0
#define COL_RES     1
#define COL_BPP     2
#define COL_FREQ    3
#define COL_APP     4

static void ListView_SetupColumns(HWND hList)
{
    LVCOLUMNA lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    struct { const char *text; int width; } cols[] = {
        { "名称",     120 },
        { "分辨率",   100 },
        { "色深",      55 },
        { "刷新率",    65 },
        { "关联程序", 200 },
    };
    for (int i = 0; i < 5; i++) {
        lvc.iSubItem = i;
        lvc.pszText  = (LPSTR)cols[i].text;
        lvc.cx       = cols[i].width;
        ListView_InsertColumn(hList, i, &lvc);
    }
}

static void RefreshList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_PROFILE_LIST);
    int  sel   = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    ListView_DeleteAllItems(hList);

    for (int i = 0; i < g_profileCount; i++) {
        Profile *p = &g_profiles[i];
        char buf[64];

        LVITEMA lvi = {0};
        lvi.mask     = LVIF_TEXT | LVIF_STATE;
        lvi.iItem    = i;
        lvi.iSubItem = 0;
        lvi.pszText  = p->name;
        lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvi.state     = (i == g_activeProfile) ? (LVIS_SELECTED | LVIS_FOCUSED) : 0;
        ListView_InsertItem(hList, &lvi);

        wsprintfA(buf, "%lux%lu", p->width, p->height);
        ListView_SetItemText(hList, i, COL_RES, buf);

        wsprintfA(buf, "%u bit", p->bitsPerPixel);
        ListView_SetItemText(hList, i, COL_BPP, buf);

        wsprintfA(buf, "%u Hz", p->refreshRate);
        ListView_SetItemText(hList, i, COL_FREQ, buf);

        ListView_SetItemText(hList, i, COL_APP, p->appPath[0] ? p->appPath : "-");
    }

    if (sel >= 0 && sel < g_profileCount)
        ListView_SetItemState(hList, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    else if (g_profileCount > 0)
        ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

static int GetSelectedIndex(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_PROFILE_LIST);
    return ListView_GetNextItem(hList, -1, LVNI_SELECTED);
}

static void UpdateButtons(HWND hDlg)
{
    int sel   = GetSelectedIndex(hDlg);
    BOOL has  = (sel >= 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_EDIT),   has);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE),  has);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_UP),      has && sel > 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_DOWN),    has && sel < g_profileCount - 1);
}

static INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        s_hDlg = hDlg;
        HICON hIco = LoadIconA(g_hInst, MAKEINTRESOURCEA(IDI_TRAY));
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIco);
        SendMessage(hDlg, WM_SETICON, ICON_BIG,   (LPARAM)hIco);

        HWND hList = GetDlgItem(hDlg, IDC_PROFILE_LIST);
        ListView_SetExtendedListViewStyle(hList,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        ListView_SetupColumns(hList);
        RefreshList(hDlg);
        UpdateButtons(hDlg);
        CheckDlgButton(hDlg, IDC_CHK_AUTOSTART,
                       GetAutoStart() ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_NEW:
            if (ShowProfileDlg(hDlg, -1)) RefreshList(hDlg);
            UpdateButtons(hDlg);
            break;

        case IDC_BTN_EDIT: {
            int idx = GetSelectedIndex(hDlg);
            if (idx >= 0 && ShowProfileDlg(hDlg, idx)) RefreshList(hDlg);
            UpdateButtons(hDlg);
            break;
        }

        case IDC_BTN_DELETE: {
            int idx = GetSelectedIndex(hDlg);
            if (idx < 0) break;
            char msg2[128];
            wsprintfA(msg2, "确定要删除配置文件 \"%s\" 吗？", g_profiles[idx].name);
            if (MessageBoxA(hDlg, msg2, "删除确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                for (int i = idx; i < g_profileCount - 1; i++)
                    g_profiles[i] = g_profiles[i + 1];
                g_profileCount--;
                if (g_activeProfile >= g_profileCount) g_activeProfile = g_profileCount - 1;
                SaveProfiles();
                RefreshList(hDlg);
            }
            UpdateButtons(hDlg);
            break;
        }

        case IDC_BTN_UP: {
            int idx = GetSelectedIndex(hDlg);
            if (idx > 0) {
                Profile tmp = g_profiles[idx];
                g_profiles[idx]     = g_profiles[idx - 1];
                g_profiles[idx - 1] = tmp;
                if (g_activeProfile == idx) g_activeProfile = idx - 1;
                else if (g_activeProfile == idx - 1) g_activeProfile = idx;
                SaveProfiles();
                RefreshList(hDlg);
                HWND hList = GetDlgItem(hDlg, IDC_PROFILE_LIST);
                ListView_SetItemState(hList, idx - 1,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            }
            UpdateButtons(hDlg);
            break;
        }

        case IDC_BTN_DOWN: {
            int idx = GetSelectedIndex(hDlg);
            if (idx >= 0 && idx < g_profileCount - 1) {
                Profile tmp = g_profiles[idx];
                g_profiles[idx]     = g_profiles[idx + 1];
                g_profiles[idx + 1] = tmp;
                if (g_activeProfile == idx) g_activeProfile = idx + 1;
                else if (g_activeProfile == idx + 1) g_activeProfile = idx;
                SaveProfiles();
                RefreshList(hDlg);
                HWND hList = GetDlgItem(hDlg, IDC_PROFILE_LIST);
                ListView_SetItemState(hList, idx + 1,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            }
            UpdateButtons(hDlg);
            break;
        }

        case IDC_CHK_AUTOSTART:
            SetAutoStart(IsDlgButtonChecked(hDlg, IDC_CHK_AUTOSTART) == BST_CHECKED);
            break;

        case IDC_BTN_CLOSE:
            ShowWindow(hDlg, SW_HIDE);
            break;
        }
        break;

    case WM_NOTIFY: {
        NMHDR *pnm = (NMHDR *)lParam;
        if (pnm->idFrom == IDC_PROFILE_LIST) {
            if (pnm->code == LVN_ITEMCHANGED || pnm->code == NM_CLICK)
                UpdateButtons(hDlg);
            if (pnm->code == NM_DBLCLK) {
                int idx = GetSelectedIndex(hDlg);
                if (idx >= 0 && ShowProfileDlg(hDlg, idx)) RefreshList(hDlg);
                UpdateButtons(hDlg);
            }
        }
        break;
    }

    case WM_CLOSE:
        ShowWindow(hDlg, SW_HIDE);
        return TRUE;

    case WM_DESTROY:
        s_hDlg = NULL;
        break;
    }
    return FALSE;
}

void ShowMainDlg(void)
{
    if (s_hDlg) {
        ShowWindow(s_hDlg, SW_SHOW);
        SetForegroundWindow(s_hDlg);
        return;
    }
    // Create as modeless so tray still works while open
    s_hDlg = CreateDialogParamA(g_hInst, MAKEINTRESOURCEA(IDD_MAIN),
                                 NULL, MainDlgProc, 0);
    if (s_hDlg) ShowWindow(s_hDlg, SW_SHOW);
}
