#ifndef RESOURCE_H
#define RESOURCE_H

// Icons
#define IDI_TRAY                    101
#define IDI_TRAY_ACTIVE             102

// Dialogs
#define IDD_MAIN                    200
#define IDD_PROFILE                 201

// Main dialog controls
#define IDC_PROFILE_LIST            1001
#define IDC_BTN_NEW                 1002
#define IDC_BTN_EDIT                1003
#define IDC_BTN_DELETE              1004
#define IDC_BTN_UP                  1005
#define IDC_BTN_DOWN                1006
#define IDC_CHK_AUTOSTART           1007
#define IDC_BTN_CLOSE               1008

// Profile dialog controls
#define IDC_EDIT_NAME               2001
#define IDC_COMBO_WIDTH             2002
#define IDC_COMBO_HEIGHT            2003
#define IDC_COMBO_DEPTH             2004
#define IDC_COMBO_FREQ              2005
#define IDC_EDIT_APPPATH            2006
#define IDC_BTN_BROWSE              2007
#define IDC_CHK_RESTORE             2008
#define IDC_BTN_OK                  IDOK
#define IDC_BTN_CANCEL              IDCANCEL

// Tray menu commands
#define ID_TRAY_SETTINGS            3001
#define ID_TRAY_EXIT                3002
#define ID_PROFILE_BASE             4000  // profile 0 = 4000, 1 = 4001, ...

// String table
#define IDS_APP_TITLE               5001
#define IDS_TRAY_TOOLTIP            5002

#endif
