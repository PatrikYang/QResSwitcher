#ifndef PROFILEDLG_H
#define PROFILEDLG_H

#include "qrestray.h"

// Opens the profile edit dialog. If index == -1, creates a new profile.
// Returns TRUE if user confirmed (OK), FALSE if cancelled.
BOOL ShowProfileDlg(HWND hParent, int index);

#endif
