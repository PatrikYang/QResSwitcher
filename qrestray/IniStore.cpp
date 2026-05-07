#include "IniStore.h"

static void EnsureIniDir(const char *path)
{
    char dir[MAX_PATH];
    strncpy(dir, path, MAX_PATH - 1);
    // strip filename
    char *last = strrchr(dir, '\\');
    if (last) { *last = '\0'; CreateDirectoryA(dir, NULL); }
}

void LoadProfiles(void)
{
    char ini[MAX_PATH];
    GetIniPath(ini, MAX_PATH);

    g_profileCount  = GetPrivateProfileIntA("General", "ProfileCount", 0, ini);
    g_activeProfile = GetPrivateProfileIntA("General", "LastProfile", -1, ini);

    if (g_profileCount > MAX_PROFILES) g_profileCount = MAX_PROFILES;

    for (int i = 0; i < g_profileCount; i++) {
        char sec[32];
        wsprintfA(sec, "Profile%d", i);
        Profile *p = &g_profiles[i];

        GetPrivateProfileStringA(sec, "Name",    "", p->name, sizeof(p->name), ini);
        p->width        = (DWORD)GetPrivateProfileIntA(sec, "Width",        1920, ini);
        p->height       = (DWORD)GetPrivateProfileIntA(sec, "Height",       1080, ini);
        p->bitsPerPixel = (UINT) GetPrivateProfileIntA(sec, "BitsPerPixel",   32, ini);
        p->refreshRate  = (UINT) GetPrivateProfileIntA(sec, "RefreshRate",    60, ini);
        p->restoreOnExit= (BOOL) GetPrivateProfileIntA(sec, "RestoreOnExit",   0, ini);
        GetPrivateProfileStringA(sec, "AppPath", "", p->appPath, sizeof(p->appPath), ini);
    }
}

void SaveProfiles(void)
{
    char ini[MAX_PATH];
    GetIniPath(ini, MAX_PATH);
    EnsureIniDir(ini);

    char buf[32];
    wsprintfA(buf, "%d", g_profileCount);
    WritePrivateProfileStringA("General", "ProfileCount", buf, ini);
    wsprintfA(buf, "%d", g_activeProfile);
    WritePrivateProfileStringA("General", "LastProfile", buf, ini);

    for (int i = 0; i < g_profileCount; i++) {
        char sec[32];
        wsprintfA(sec, "Profile%d", i);
        Profile *p = &g_profiles[i];

        WritePrivateProfileStringA(sec, "Name", p->name, ini);
        wsprintfA(buf, "%lu", p->width);
        WritePrivateProfileStringA(sec, "Width", buf, ini);
        wsprintfA(buf, "%lu", p->height);
        WritePrivateProfileStringA(sec, "Height", buf, ini);
        wsprintfA(buf, "%u", p->bitsPerPixel);
        WritePrivateProfileStringA(sec, "BitsPerPixel", buf, ini);
        wsprintfA(buf, "%u", p->refreshRate);
        WritePrivateProfileStringA(sec, "RefreshRate", buf, ini);
        wsprintfA(buf, "%d", (int)p->restoreOnExit);
        WritePrivateProfileStringA(sec, "RestoreOnExit", buf, ini);
        WritePrivateProfileStringA(sec, "AppPath", p->appPath, ini);
    }

    // Remove stale sections beyond current count
    for (int i = g_profileCount; i < MAX_PROFILES; i++) {
        char sec[32];
        wsprintfA(sec, "Profile%d", i);
        char test[8];
        GetPrivateProfileStringA(sec, "Name", "\x01", test, sizeof(test), ini);
        if (test[0] != '\x01')
            WritePrivateProfileStringA(sec, NULL, NULL, ini); // delete section
        else
            break;
    }
}
