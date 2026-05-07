#include "ProfileDlg.h"
#include "IniStore.h"

// Context passed into DialogBoxParam
typedef struct {
    int     index;   // -1 = new
    Profile edit;    // working copy
} ProfileDlgCtx;

// Enumerate available display modes and collect unique widths/heights/depths/freqs
typedef struct { DWORD w, h; UINT bpp, freq; } DisplayMode;
static DisplayMode s_modes[512];
static int         s_modeCount;

static void EnumModes(void)
{
    s_modeCount = 0;
    DEVMODEA dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsA(NULL, i, &dm); i++) {
        if (s_modeCount < 512) {
            s_modes[s_modeCount].w    = dm.dmPelsWidth;
            s_modes[s_modeCount].h    = dm.dmPelsHeight;
            s_modes[s_modeCount].bpp  = dm.dmBitsPerPel;
            s_modes[s_modeCount].freq = dm.dmDisplayFrequency;
            s_modeCount++;
        }
    }
}

// Populate width combo with unique values
static void FillWidthCombo(HWND hDlg)
{
    HWND hCb = GetDlgItem(hDlg, IDC_COMBO_WIDTH);
    SendMessage(hCb, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < s_modeCount; i++) {
        char buf[16];
        wsprintfA(buf, "%lu", s_modes[i].w);
        if (SendMessageA(hCb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)buf) == CB_ERR)
            SendMessageA(hCb, CB_ADDSTRING, 0, (LPARAM)buf);
    }
}

// Populate height combo filtered by selected width
static void FillHeightCombo(HWND hDlg, DWORD selWidth)
{
    HWND hCb = GetDlgItem(hDlg, IDC_COMBO_HEIGHT);
    SendMessage(hCb, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < s_modeCount; i++) {
        if (s_modes[i].w != selWidth) continue;
        char buf[16];
        wsprintfA(buf, "%lu", s_modes[i].h);
        if (SendMessageA(hCb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)buf) == CB_ERR)
            SendMessageA(hCb, CB_ADDSTRING, 0, (LPARAM)buf);
    }
}

// Populate freq combo filtered by width+height
static void FillFreqCombo(HWND hDlg, DWORD selWidth, DWORD selHeight)
{
    HWND hCb = GetDlgItem(hDlg, IDC_COMBO_FREQ);
    SendMessage(hCb, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < s_modeCount; i++) {
        if (s_modes[i].w != selWidth || s_modes[i].h != selHeight) continue;
        char buf[16];
        wsprintfA(buf, "%u", s_modes[i].freq);
        if (SendMessageA(hCb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)buf) == CB_ERR)
            SendMessageA(hCb, CB_ADDSTRING, 0, (LPARAM)buf);
    }
}

static void SelectComboByValue(HWND hDlg, int ctrlId, const char *val)
{
    HWND hCb = GetDlgItem(hDlg, ctrlId);
    LRESULT idx = SendMessageA(hCb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)val);
    if (idx != CB_ERR) SendMessage(hCb, CB_SETCURSEL, (WPARAM)idx, 0);
    else if (SendMessage(hCb, CB_GETCOUNT, 0, 0) > 0)
        SendMessage(hCb, CB_SETCURSEL, 0, 0);
}

static DWORD GetComboUInt(HWND hDlg, int ctrlId)
{
    char buf[32] = {0};
    HWND hCb = GetDlgItem(hDlg, ctrlId);
    int idx = (int)SendMessage(hCb, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) SendMessageA(hCb, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)buf);
    return (DWORD)atol(buf);
}

static void OnWidthChanged(HWND hDlg, Profile *p)
{
    DWORD w = GetComboUInt(hDlg, IDC_COMBO_WIDTH);
    FillHeightCombo(hDlg, w);
    char buf[16]; wsprintfA(buf, "%lu", p->height);
    SelectComboByValue(hDlg, IDC_COMBO_HEIGHT, buf);

    DWORD h = GetComboUInt(hDlg, IDC_COMBO_HEIGHT);
    FillFreqCombo(hDlg, w, h);
    wsprintfA(buf, "%u", p->refreshRate);
    SelectComboByValue(hDlg, IDC_COMBO_FREQ, buf);
}

static void OnHeightChanged(HWND hDlg, Profile *p)
{
    DWORD w = GetComboUInt(hDlg, IDC_COMBO_WIDTH);
    DWORD h = GetComboUInt(hDlg, IDC_COMBO_HEIGHT);
    FillFreqCombo(hDlg, w, h);
    char buf[16]; wsprintfA(buf, "%u", p->refreshRate);
    SelectComboByValue(hDlg, IDC_COMBO_FREQ, buf);
}

static void InitProfileDlg(HWND hDlg, ProfileDlgCtx *ctx)
{
    Profile *p = &ctx->edit;

    EnumModes();
    FillWidthCombo(hDlg);

    // Color depth combo
    HWND hDepth = GetDlgItem(hDlg, IDC_COMBO_DEPTH);
    SendMessageA(hDepth, CB_ADDSTRING, 0, (LPARAM)"8");
    SendMessageA(hDepth, CB_ADDSTRING, 0, (LPARAM)"16");
    SendMessageA(hDepth, CB_ADDSTRING, 0, (LPARAM)"32");

    // Set initial values
    SetDlgItemTextA(hDlg, IDC_EDIT_NAME, p->name);
    SetDlgItemTextA(hDlg, IDC_EDIT_APPPATH, p->appPath);
    CheckDlgButton(hDlg, IDC_CHK_RESTORE, p->restoreOnExit ? BST_CHECKED : BST_UNCHECKED);

    char buf[16];
    wsprintfA(buf, "%lu", p->width);
    SelectComboByValue(hDlg, IDC_COMBO_WIDTH, buf);
    OnWidthChanged(hDlg, p);

    wsprintfA(buf, "%lu", p->height);
    SelectComboByValue(hDlg, IDC_COMBO_HEIGHT, buf);
    OnHeightChanged(hDlg, p);

    wsprintfA(buf, "%u", p->bitsPerPixel);
    SelectComboByValue(hDlg, IDC_COMBO_DEPTH, buf);

    wsprintfA(buf, "%u", p->refreshRate);
    SelectComboByValue(hDlg, IDC_COMBO_FREQ, buf);
}

static BOOL CollectProfileDlg(HWND hDlg, ProfileDlgCtx *ctx)
{
    Profile *p = &ctx->edit;

    GetDlgItemTextA(hDlg, IDC_EDIT_NAME, p->name, sizeof(p->name));
    if (!p->name[0]) {
        MessageBoxA(hDlg, "配置文件名称不能为空。", "错误", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hDlg, IDC_EDIT_NAME));
        return FALSE;
    }

    p->width        = GetComboUInt(hDlg, IDC_COMBO_WIDTH);
    p->height       = GetComboUInt(hDlg, IDC_COMBO_HEIGHT);
    p->bitsPerPixel = (UINT)GetComboUInt(hDlg, IDC_COMBO_DEPTH);
    p->refreshRate  = (UINT)GetComboUInt(hDlg, IDC_COMBO_FREQ);
    p->restoreOnExit= (IsDlgButtonChecked(hDlg, IDC_CHK_RESTORE) == BST_CHECKED);
    GetDlgItemTextA(hDlg, IDC_EDIT_APPPATH, p->appPath, sizeof(p->appPath));

    return TRUE;
}

static INT_PTR CALLBACK ProfileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ProfileDlgCtx *ctx = (ProfileDlgCtx *)GetWindowLongPtrA(hDlg, GWLP_USERDATA);

    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrA(hDlg, GWLP_USERDATA, lParam);
        ctx = (ProfileDlgCtx *)lParam;
        InitProfileDlg(hDlg, ctx);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_COMBO_WIDTH:
            if (HIWORD(wParam) == CBN_SELCHANGE) OnWidthChanged(hDlg, &ctx->edit);
            break;
        case IDC_COMBO_HEIGHT:
            if (HIWORD(wParam) == CBN_SELCHANGE) OnHeightChanged(hDlg, &ctx->edit);
            break;
        case IDC_BTN_BROWSE: {
            OPENFILENAMEA ofn;
            char file[MAX_PATH] = {0};
            GetDlgItemTextA(hDlg, IDC_EDIT_APPPATH, file, MAX_PATH);
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hDlg;
            ofn.lpstrFilter = "可执行文件 (*.exe)\0*.exe\0所有文件 (*.*)\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            if (GetOpenFileNameA(&ofn))
                SetDlgItemTextA(hDlg, IDC_EDIT_APPPATH, file);
            break;
        }
        case IDOK:
            if (CollectProfileDlg(hDlg, ctx))
                EndDialog(hDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;
    }
    return FALSE;
}

BOOL ShowProfileDlg(HWND hParent, int index)
{
    ProfileDlgCtx ctx;
    ctx.index = index;

    if (index >= 0 && index < g_profileCount)
        ctx.edit = g_profiles[index];
    else {
        // Default new profile = current display mode
        ZeroMemory(&ctx.edit, sizeof(ctx.edit));
        QRMODE cur;
        GetCurQResMode(&cur);
        ctx.edit.width        = cur.dwXRes;
        ctx.edit.height       = cur.dwYRes;
        ctx.edit.bitsPerPixel = cur.wBitsPixel;
        ctx.edit.refreshRate  = cur.wFreq ? cur.wFreq : 60;
        lstrcpyA(ctx.edit.name, "新配置文件");
    }

    INT_PTR ret = DialogBoxParamA(g_hInst, MAKEINTRESOURCEA(IDD_PROFILE),
                                  hParent, ProfileDlgProc, (LPARAM)&ctx);
    if (ret == IDOK) {
        if (index >= 0 && index < g_profileCount) {
            g_profiles[index] = ctx.edit;
        } else {
            if (g_profileCount < MAX_PROFILES) {
                g_profiles[g_profileCount++] = ctx.edit;
            }
        }
        SaveProfiles();
        return TRUE;
    }
    return FALSE;
}
