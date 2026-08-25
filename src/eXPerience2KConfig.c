#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sddl.h>
#include <stdio.h>
#include <string.h>

#define APP_TITLE "eXPerience2K"
#define CONFIG_KEY "Software\\eXPerience2K\\Config"
#define EXPLORER_MACHINE_STATE_KEY "SOFTWARE\\eXPerience2K\\ExplorerExperiment"
#define MAX_FEATURES 11
#define LOG_CAPACITY 262144
#define IDC_FEATURE_BASE 1000
#define IDC_APPLY 2001
#define IDC_OPEN_LOG 2002
#define IDC_SAVE_LOG 2003
#define IDC_CLOSE 2004
#define IDC_LOG 2005
#define IDC_STATUS 2006
#define IDC_LOGON_CAPTION_PRESET 2007
#define IDC_DESKTOP_CAPTION_PRESET 2008
#define IDC_LOGON_CAPTION_LABEL 2009
#define IDC_DESKTOP_CAPTION_LABEL 2010
#define IDC_REVERT 2011
#define IDC_CAPTION_PRESET_GROUP 2012
#define MAIN_CLIENT_WIDTH 514
#define MAIN_MIN_CLIENT_WIDTH 460
#define MAIN_MIN_CLIENT_HEIGHT 180
#define MAIN_COMPACT_CLIENT_HEIGHT 519
#define MAIN_EXPANDED_CLIENT_HEIGHT 657
#define MAIN_BUTTON_TOP 473
#define MAIN_LOG_TOP 515
#define MAIN_LOG_HEIGHT 126
#define APPLYING_WINDOW_CLASS "eXPerience2KApplyingWindow"

#ifndef SPI_GETMENUANIMATION
#define SPI_GETMENUANIMATION 0x1002
#define SPI_SETMENUANIMATION 0x1003
#define SPI_GETMENUFADE 0x1012
#define SPI_SETMENUFADE 0x1013
#endif
#ifndef SPI_GETGRADIENTCAPTIONS
#define SPI_GETGRADIENTCAPTIONS 0x1008
#define SPI_SETGRADIENTCAPTIONS 0x1009
#endif

typedef struct {
    const char *id;
    const char *label;
    int default_on;
    int administrator_required;
    int implemented;
    HWND checkbox;
    int detected;
} FEATURE;

enum {
    FEATURE_RESOURCE_CONVERSION = 0,
    FEATURE_CLASSIC_THEME,
    FEATURE_CLASSIC_START_MENU,
    FEATURE_CLASSIC_CONTROL_PANEL,
    FEATURE_MENU_SLIDE,
    FEATURE_MENU_FADE,
    FEATURE_CLASSIC_LOGON,
    FEATURE_WALLPAPERS,
    FEATURE_CLASSIC_EXPLORER,
    FEATURE_WINDOWS_2000_SOUNDS,
    FEATURE_WINDOWS_2000_DOUBLE_CLICK_SOUND
};

enum {
    CAPTION_PRESET_SOLID_NAVY = 0,
    CAPTION_PRESET_BLUE_GRADIENT = 1
};

typedef struct {
    int supported;
    int resource_ready;
    int administrator;
    char profile_id[64];
    char display_name[160];
    char architecture[16];
    char branding_label[160];
    char reason[256];
} PROBE_RESULT;

static HINSTANCE g_instance;
static HWND g_window;
static HWND g_log_edit;
static HWND g_status;
static HWND g_applying_dialog;
static HWND g_logon_caption_combo;
static HWND g_desktop_caption_combo;
static HWND g_logon_caption_label;
static HWND g_desktop_caption_label;
static int g_apply_in_progress;
static int g_scroll_y;
static int g_scrollbar_visible = 1;
static char g_install_root[MAX_PATH];
static char g_core_path[MAX_PATH];
static char g_log[LOG_CAPACITY];
static size_t g_log_length;
static char g_interactive_sid[192];
static int g_use_current_user_fallback;
static int g_cross_user;
static PROBE_RESULT g_probe;

static FEATURE g_features[MAX_FEATURES] = {
    {"resource_conversion", "Windows 2000 visual resource conversion", 1, 1, 1, NULL, 0},
    {"classic_theme", "Automatically change the Windows theme to Classic", 1, 0, 1, NULL, 0},
    {"classic_start_menu", "Enable the Classic Start menu and taskbar layout", 1, 0, 1, NULL, 0},
    {"classic_control_panel", "Use Classic Control Panel view by default", 1, 0, 1, NULL, 0},
    {"start_menu_slide", "Use sliding Start menu and submenu animations", 1, 0, 1, NULL, 0},
    {"start_menu_fade", "Use fading Start menu and submenu animations", 0, 0, 1, NULL, 0},
    {"classic_logon", "Windows 2000 style login window", 1, 1, 1, NULL, 0},
    {"install_wallpapers", "Install Windows 2000 style wallpapers to My Pictures", 1, 0, 1, NULL, 0},
    {"classic_explorer", "Windows 2000 Explorer folder interface (experimental)", 1, 1, 1, NULL, 0},
    {"windows_2000_sounds", "Replace Windows XP sounds with Windows 2000 equivalents", 1, 0, 1, NULL, 0},
    {"windows_2000_double_click_sound", "Enable the Windows 2000 folder double-click sound", 1, 0, 1, NULL, 0}
};

typedef struct {
    const char *file_name;
    const char *created_marker;
} WALLPAPER_ITEM;

static const WALLPAPER_ITEM g_wallpapers[] = {
    {"Windows_2000_1.jpg", "WallpaperCreated_Windows2000_1"},
    {"Windows_2000_2.jpg", "WallpaperCreated_Windows2000_2"},
    {"Windows_9x.jpg", "WallpaperCreated_Windows9x"},
    {"Windows_2000_3.jpg", "WallpaperCreated_Windows2000_3"},
    {"Windows_2000_4.jpg", "WallpaperCreated_Windows2000_4"}
};

typedef struct {
    const char *marker;
    const char *application;
    const char *event;
    const char *file_name;
} WINDOWS_SOUND_EVENT;

/* This table covers the base XP Professional x64 sound assignments plus the
   built-in events added by the fully updated reference system.  An empty file
   name is intentional: Windows 2000 defines that event as silent, or does not
   define the XP-only event at all. */
static const WINDOWS_SOUND_EVENT g_windows_sound_events[] = {
    {"Sound_Default", ".Default", ".Default", "ding.wav"},
    {"Sound_AppGPFault", ".Default", "AppGPFault", ""},
    {"Sound_Close", ".Default", "Close", ""},
    {"Sound_CriticalBattery", ".Default", "CriticalBatteryAlarm", "ding.wav"},
    {"Sound_DeviceConnect", ".Default", "DeviceConnect", ""},
    {"Sound_DeviceDisconnect", ".Default", "DeviceDisconnect", ""},
    {"Sound_DeviceFail", ".Default", "DeviceFail", ""},
    {"Sound_LowBattery", ".Default", "LowBatteryAlarm", "ding.wav"},
    {"Sound_MailBeep", ".Default", "MailBeep", "notify.wav"},
    {"Sound_Maximize", ".Default", "Maximize", ""},
    {"Sound_MenuCommand", ".Default", "MenuCommand", ""},
    {"Sound_MenuPopup", ".Default", "MenuPopup", ""},
    {"Sound_Minimize", ".Default", "Minimize", ""},
    {"Sound_Open", ".Default", "Open", ""},
    {"Sound_PrintComplete", ".Default", "PrintComplete", ""},
    {"Sound_RestoreDown", ".Default", "RestoreDown", ""},
    {"Sound_RestoreUp", ".Default", "RestoreUp", ""},
    {"Sound_SystemAsterisk", ".Default", "SystemAsterisk", "chord.wav"},
    {"Sound_SystemExclamation", ".Default", "SystemExclamation", "chord.wav"},
    {"Sound_SystemExit", ".Default", "SystemExit", "logoff.wav"},
    {"Sound_SystemHand", ".Default", "SystemHand", "chord.wav"},
    {"Sound_SystemNotification", ".Default", "SystemNotification", "notify.wav"},
    {"Sound_SystemQuestion", ".Default", "SystemQuestion", "chord.wav"},
    {"Sound_SystemStart", ".Default", "SystemStart", "logon.wav"},
    {"Sound_WindowsLogoff", ".Default", "WindowsLogoff", "logoff.wav"},
    {"Sound_WindowsLogon", ".Default", "WindowsLogon", "logon.wav"},
    {"Sound_ConfPersonJoins", "Conf", "Person Joins", "blip.wav"},
    {"Sound_ConfPersonLeaves", "Conf", "Person Leaves", "blip.wav"},
    {"Sound_ConfReceiveCall", "Conf", "Receive Call", "ringin.wav"},
    {"Sound_ConfReceiveJoinRequest", "Conf", "Receive Request to Join", "ringin.wav"},
    {"Sound_BlockedPopup", "Explorer", "BlockedPopup", ""},
    {"Sound_EmptyRecycleBin", "Explorer", "EmptyRecycleBin", "ding.wav"},
    {"Sound_FeedDiscovered", "Explorer", "FeedDiscovered", ""},
    {"Sound_SecurityBand", "Explorer", "SecurityBand", ""},
    {"Sound_MessengerContactOnline", "MSMSGS", "MSMSGS_ContactOnline", ""},
    {"Sound_MessengerNewAlert", "MSMSGS", "MSMSGS_NewAlert", ""},
    {"Sound_MessengerNewMail", "MSMSGS", "MSMSGS_NewMail", ""},
    {"Sound_MessengerNewMessage", "MSMSGS", "MSMSGS_NewMessage", ""}
};

/* Windows 2000 assigns START.WAV to Explorer's Navigating event.  Keep it
   outside the bulk table so the user can control the familiar folder
   double-click/navigation click independently.  The marker intentionally
   matches the earlier bulk-sound build for seamless upgrades and restoration. */
static const WINDOWS_SOUND_EVENT g_windows_2000_double_click_sound = {
    "Sound_Navigating", "Explorer", "Navigating", "start.wav"
};

typedef struct {
    const char *name;
    const char *text;
    int system_index;
    BYTE red;
    BYTE green;
    BYTE blue;
} W2K_COLOR;

typedef struct {
    const char *name;
    const char *text;
} W2K_METRIC;

static const W2K_COLOR g_w2k_colors[] = {
    {"ActiveBorder", "212 208 200", COLOR_ACTIVEBORDER, 212, 208, 200},
    {"ActiveTitle", "10 36 106", COLOR_ACTIVECAPTION, 10, 36, 106},
    {"AppWorkSpace", "128 128 128", COLOR_APPWORKSPACE, 128, 128, 128},
    {"Background", "58 110 165", COLOR_BACKGROUND, 58, 110, 165},
    {"ButtonAlternateFace", "181 181 181", -1, 181, 181, 181},
    {"ButtonDkShadow", "64 64 64", COLOR_3DDKSHADOW, 64, 64, 64},
    {"ButtonFace", "212 208 200", COLOR_3DFACE, 212, 208, 200},
    {"ButtonHilight", "255 255 255", COLOR_3DHIGHLIGHT, 255, 255, 255},
    {"ButtonLight", "212 208 200", COLOR_3DLIGHT, 212, 208, 200},
    {"ButtonShadow", "128 128 128", COLOR_3DSHADOW, 128, 128, 128},
    {"ButtonText", "0 0 0", COLOR_BTNTEXT, 0, 0, 0},
    {"GradientActiveTitle", "166 202 240", COLOR_GRADIENTACTIVECAPTION, 166, 202, 240},
    {"GradientInactiveTitle", "192 192 192", COLOR_GRADIENTINACTIVECAPTION, 192, 192, 192},
    {"GrayText", "128 128 128", COLOR_GRAYTEXT, 128, 128, 128},
    {"Hilight", "10 36 106", COLOR_HIGHLIGHT, 10, 36, 106},
    {"HilightText", "255 255 255", COLOR_HIGHLIGHTTEXT, 255, 255, 255},
    {"HotTrackingColor", "0 0 128", COLOR_HOTLIGHT, 0, 0, 128},
    {"InactiveBorder", "212 208 200", COLOR_INACTIVEBORDER, 212, 208, 200},
    {"InactiveTitle", "128 128 128", COLOR_INACTIVECAPTION, 128, 128, 128},
    {"InactiveTitleText", "212 208 200", COLOR_INACTIVECAPTIONTEXT, 212, 208, 200},
    {"InfoText", "0 0 0", COLOR_INFOTEXT, 0, 0, 0},
    {"InfoWindow", "255 255 225", COLOR_INFOBK, 255, 255, 225},
    {"Menu", "212 208 200", COLOR_MENU, 212, 208, 200},
    {"MenuText", "0 0 0", COLOR_MENUTEXT, 0, 0, 0},
    {"Scrollbar", "212 208 200", COLOR_SCROLLBAR, 212, 208, 200},
    {"TitleText", "255 255 255", COLOR_CAPTIONTEXT, 255, 255, 255},
    {"Window", "255 255 255", COLOR_WINDOW, 255, 255, 255},
    {"WindowFrame", "0 0 0", COLOR_WINDOWFRAME, 0, 0, 0},
    {"WindowText", "0 0 0", COLOR_WINDOWTEXT, 0, 0, 0}
};

static const W2K_METRIC g_w2k_metrics[] = {
    {"BorderWidth", "1"},
    {"CaptionHeight", "-270"},
    {"CaptionWidth", "-270"},
    {"IconSpacing", "-1125"},
    {"IconTitleWrap", "1"},
    {"IconVerticalspacing", "-1125"},
    {"MenuHeight", "-270"},
    {"MenuWidth", "-270"},
    {"ScrollHeight", "-240"},
    {"ScrollWidth", "-240"},
    {"SmCaptionHeight", "-180"},
    {"SmCaptionWidth", "-180"}
};

static const char *g_w2k_font_values[] = {
    "CaptionFont", "IconFont", "MenuFont", "MessageFont", "SmCaptionFont", "StatusFont"
};

static int join_path(char *output, size_t output_size, const char *left,
                     const char *right);

static void pump_ui_messages(void)
{
    MSG message;
    if (!g_window) return;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            PostQuitMessage((int)message.wParam);
            break;
        }
        if (!IsDialogMessageA(g_window, &message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    RedrawWindow(g_window, NULL, NULL,
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    if (g_applying_dialog)
        RedrawWindow(g_applying_dialog, NULL, NULL,
            RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

static void append_log(const char *text)
{
    size_t length;
    if (!text) return;
    length = strlen(text);
    if (length > LOG_CAPACITY - 1 - g_log_length)
        length = LOG_CAPACITY - 1 - g_log_length;
    if (!length) return;
    CopyMemory(g_log + g_log_length, text, length);
    g_log_length += length;
    g_log[g_log_length] = '\0';
    if (g_log_edit) {
        SetWindowTextA(g_log_edit, g_log);
        if (g_apply_in_progress) pump_ui_messages();
    }
}

static void append_log_line(const char *text)
{
    append_log(text);
    append_log("\r\n");
}

static void save_unattended_log(void)
{
    char path[MAX_PATH];
    HANDLE file;
    DWORD written = 0;
    if (!join_path(path, sizeof(path), g_install_root,
                   "explorer-experiment-unattended.log")) return;
    file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    WriteFile(file, g_log, (DWORD)g_log_length, &written, NULL);
    CloseHandle(file);
}

static int file_exists(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static void parent_directory(char *path)
{
    char *slash = strrchr(path, '\\');
    if (slash) *slash = '\0';
}

static int join_path(char *output, size_t output_size, const char *left, const char *right)
{
    int written = _snprintf(output, output_size, "%s%s%s", left,
        left[0] && left[strlen(left) - 1] != '\\' ? "\\" : "", right);
    return written >= 0 && (size_t)written < output_size;
}

static int get_token_sid(HANDLE token, char *output, size_t output_size)
{
    DWORD required = 0;
    TOKEN_USER *user;
    LPSTR sid_text = NULL;
    GetTokenInformation(token, TokenUser, NULL, 0, &required);
    if (!required) return 0;
    user = (TOKEN_USER *)HeapAlloc(GetProcessHeap(), 0, required);
    if (!user) return 0;
    if (!GetTokenInformation(token, TokenUser, user, required, &required) ||
        !ConvertSidToStringSidA(user->User.Sid, &sid_text)) {
        HeapFree(GetProcessHeap(), 0, user);
        return 0;
    }
    lstrcpynA(output, sid_text, (int)output_size);
    LocalFree(sid_text);
    HeapFree(GetProcessHeap(), 0, user);
    return 1;
}

static int discover_interactive_user(void)
{
    HWND shell = GetShellWindow();
    DWORD process_id = 0;
    HANDLE process = NULL, token = NULL, current_token = NULL;
    char current_sid[192];
    g_use_current_user_fallback = 1;
    g_cross_user = 0;
    if (!shell) return 0;
    GetWindowThreadProcessId(shell, &process_id);
    if (!process_id) return 0;
    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (!process) return 0;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) {
        CloseHandle(process);
        return 0;
    }
    if (!get_token_sid(token, g_interactive_sid, sizeof(g_interactive_sid))) {
        CloseHandle(token);
        CloseHandle(process);
        return 0;
    }
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &current_token)) {
        if (get_token_sid(current_token, current_sid, sizeof(current_sid)))
            g_cross_user = lstrcmpiA(current_sid, g_interactive_sid) != 0;
        CloseHandle(current_token);
    }
    CloseHandle(token);
    CloseHandle(process);
    g_use_current_user_fallback = 0;
    return 1;
}

static LONG open_user_key(const char *relative, REGSAM access, int create, HKEY *result)
{
    char path[512];
    HKEY root = HKEY_CURRENT_USER;
    DWORD disposition;
    if (!g_use_current_user_fallback) {
        root = HKEY_USERS;
        _snprintf(path, sizeof(path), "%s\\%s", g_interactive_sid, relative);
    } else {
        lstrcpynA(path, relative, sizeof(path));
    }
    if (create)
        return RegCreateKeyExA(root, path, 0, NULL, 0, access, NULL, result, &disposition);
    return RegOpenKeyExA(root, path, 0, access, result);
}

static int read_user_dword(const char *subkey, const char *name, DWORD *value)
{
    HKEY key;
    DWORD type = 0, size = sizeof(*value);
    LONG status = open_user_key(subkey, KEY_QUERY_VALUE, 0, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, &type, (BYTE *)value, &size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS && type == REG_DWORD;
}

static int write_user_dword(const char *subkey, const char *name, DWORD value)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_SET_VALUE, 1, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, REG_DWORD, (BYTE *)&value, sizeof(value));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int read_user_string(const char *subkey, const char *name, char *value, DWORD capacity)
{
    HKEY key;
    DWORD type = 0, size = capacity;
    LONG status = open_user_key(subkey, KEY_QUERY_VALUE, 0, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, &type, (BYTE *)value, &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return 0;
    value[capacity - 1] = '\0';
    return 1;
}

static int write_user_string(const char *subkey, const char *name, const char *value)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_SET_VALUE, 1, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, REG_SZ, (const BYTE *)value,
                            (DWORD)strlen(value) + 1);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int read_user_binary(const char *subkey, const char *name, BYTE *value, DWORD *size)
{
    HKEY key;
    DWORD type = 0;
    LONG status = open_user_key(subkey, KEY_QUERY_VALUE, 0, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, &type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS && type == REG_BINARY;
}

static int write_user_binary(const char *subkey, const char *name,
                             const BYTE *value, DWORD size)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_SET_VALUE, 1, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, REG_BINARY, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int read_user_value(const char *subkey, const char *name, DWORD *type,
                           BYTE *value, DWORD *size)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_QUERY_VALUE, 0, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int write_user_value(const char *subkey, const char *name, DWORD type,
                            const BYTE *value, DWORD size)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_SET_VALUE, 1, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int delete_user_value(const char *subkey, const char *name)
{
    HKEY key;
    LONG status = open_user_key(subkey, KEY_SET_VALUE, 0, &key);
    if (status == ERROR_FILE_NOT_FOUND) return 1;
    if (status != ERROR_SUCCESS) return 0;
    status = RegDeleteValueA(key, name);
    RegCloseKey(key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

static int delete_user_tree(const char *relative)
{
    HKEY root = HKEY_CURRENT_USER;
    char path[512];
    LONG status;
    if (!g_use_current_user_fallback) {
        root = HKEY_USERS;
        _snprintf(path, sizeof(path), "%s\\%s", g_interactive_sid, relative);
    } else {
        lstrcpynA(path, relative, sizeof(path));
    }
    status = SHDeleteKeyA(root, path);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND ||
           status == ERROR_PATH_NOT_FOUND;
}

static int delete_user_key_if_empty(const char *relative)
{
    HKEY key;
    DWORD subkeys = 0, values = 0;
    LONG status = open_user_key(relative,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, 0, &key);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) return 1;
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subkeys, NULL, NULL,
                              &values, NULL, NULL, NULL, NULL);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) return 0;
    if (subkeys || values) return 1;
    return delete_user_tree(relative);
}

static int files_are_identical(const char *left_path, const char *right_path)
{
    HANDLE left = INVALID_HANDLE_VALUE, right = INVALID_HANDLE_VALUE;
    DWORD left_high = 0, right_high = 0, left_low, right_low;
    BYTE *left_buffer = NULL, *right_buffer = NULL;
    const DWORD buffer_size = 32768;
    int identical = 0;

    left = CreateFileA(left_path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    right = CreateFileA(right_path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (left == INVALID_HANDLE_VALUE || right == INVALID_HANDLE_VALUE) goto cleanup;

    SetLastError(NO_ERROR);
    left_low = GetFileSize(left, &left_high);
    if (left_low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) goto cleanup;
    SetLastError(NO_ERROR);
    right_low = GetFileSize(right, &right_high);
    if (right_low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) goto cleanup;
    if (left_low != right_low || left_high != right_high) goto cleanup;

    left_buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, buffer_size);
    right_buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, buffer_size);
    if (!left_buffer || !right_buffer) goto cleanup;

    identical = 1;
    for (;;) {
        DWORD left_read = 0, right_read = 0;
        if (!ReadFile(left, left_buffer, buffer_size, &left_read, NULL) ||
            !ReadFile(right, right_buffer, buffer_size, &right_read, NULL) ||
            left_read != right_read) {
            identical = 0;
            break;
        }
        if (!left_read) break;
        if (memcmp(left_buffer, right_buffer, left_read) != 0) {
            identical = 0;
            break;
        }
    }

cleanup:
    if (left_buffer) HeapFree(GetProcessHeap(), 0, left_buffer);
    if (right_buffer) HeapFree(GetProcessHeap(), 0, right_buffer);
    if (left != INVALID_HANDLE_VALUE) CloseHandle(left);
    if (right != INVALID_HANDLE_VALUE) CloseHandle(right);
    return identical;
}

static int get_interactive_my_pictures(char *output, DWORD capacity)
{
    char profile[MAX_PATH];
    HRESULT result;
    if (!output || !capacity) return 0;
    output[0] = '\0';

    if (read_user_string(
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
            "My Pictures", output, capacity) && output[0])
        return 1;

    if (read_user_string("Volatile Environment", "USERPROFILE",
                         profile, sizeof(profile)) && profile[0])
        return join_path(output, capacity, profile, "My Documents\\My Pictures");

    if (!g_use_current_user_fallback) return 0;
    result = SHGetFolderPathA(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE,
                             NULL, SHGFP_TYPE_CURRENT, output);
    return SUCCEEDED(result) && output[0];
}

static int wallpapers_installed_detected(void)
{
    DWORD enabled = 0;
    return read_user_dword(CONFIG_KEY, "WallpapersEnabled", &enabled) && enabled;
}

static int apply_wallpaper_installation(int enabled)
{
    char my_pictures[MAX_PATH];
    char source_directory[MAX_PATH];
    size_t index;
    int ok = 1;

    if (!get_interactive_my_pictures(my_pictures, sizeof(my_pictures))) {
        append_log_line("ERROR: the interactive user's My Pictures folder could not be located.");
        return 0;
    }
    if (!join_path(source_directory, sizeof(source_directory),
                   g_install_root, "Assets\\Wallpapers")) {
        append_log_line("ERROR: the wallpaper payload path is too long.");
        return 0;
    }

    if (enabled) {
        int directory_result = SHCreateDirectoryExA(NULL, my_pictures, NULL);
        if (directory_result != ERROR_SUCCESS &&
            directory_result != ERROR_ALREADY_EXISTS &&
            directory_result != ERROR_FILE_EXISTS) {
            append_log_line("ERROR: My Pictures could not be created or opened.");
            return 0;
        }
    }

    for (index = 0; index < sizeof(g_wallpapers) / sizeof(g_wallpapers[0]); ++index) {
        char source[MAX_PATH], destination[MAX_PATH], message[320];
        DWORD created = 0;
        int has_created_marker = read_user_dword(
            CONFIG_KEY, g_wallpapers[index].created_marker, &created) && created;

        if (!join_path(source, sizeof(source), source_directory,
                       g_wallpapers[index].file_name) ||
            !join_path(destination, sizeof(destination), my_pictures,
                       g_wallpapers[index].file_name)) {
            append_log_line("ERROR: a wallpaper path is too long.");
            ok = 0;
            continue;
        }

        if (enabled) {
            if (!file_exists(source)) {
                _snprintf(message, sizeof(message),
                    "ERROR: included wallpaper is missing: %s", g_wallpapers[index].file_name);
                append_log_line(message);
                ok = 0;
                continue;
            }
            if (file_exists(destination)) {
                _snprintf(message, sizeof(message),
                    "Preserved existing My Pictures file: %s", g_wallpapers[index].file_name);
                append_log_line(message);
                continue;
            }
            if (!CopyFileA(source, destination, TRUE)) {
                _snprintf(message, sizeof(message),
                    "ERROR: wallpaper could not be copied: %s", g_wallpapers[index].file_name);
                append_log_line(message);
                ok = 0;
                continue;
            }
            if (!write_user_dword(CONFIG_KEY, g_wallpapers[index].created_marker, 1)) {
                _snprintf(message, sizeof(message),
                    "ERROR: wallpaper ownership state could not be recorded: %s",
                    g_wallpapers[index].file_name);
                append_log_line(message);
                ok = 0;
                continue;
            }
            _snprintf(message, sizeof(message),
                "Installed wallpaper: %s", g_wallpapers[index].file_name);
            append_log_line(message);
        } else if (has_created_marker) {
            if (file_exists(destination)) {
                if (file_exists(source) && files_are_identical(source, destination)) {
                    if (!DeleteFileA(destination)) {
                        _snprintf(message, sizeof(message),
                            "ERROR: installed wallpaper could not be removed: %s",
                            g_wallpapers[index].file_name);
                        append_log_line(message);
                        ok = 0;
                        continue;
                    }
                    _snprintf(message, sizeof(message),
                        "Removed unmodified installed wallpaper: %s",
                        g_wallpapers[index].file_name);
                    append_log_line(message);
                } else {
                    _snprintf(message, sizeof(message),
                        "Preserved modified wallpaper: %s", g_wallpapers[index].file_name);
                    append_log_line(message);
                }
            }
            if (!delete_user_value(CONFIG_KEY, g_wallpapers[index].created_marker)) ok = 0;
        }
    }

    if (enabled) {
        if (ok && !write_user_dword(CONFIG_KEY, "WallpapersEnabled", 1)) ok = 0;
    } else if (!write_user_dword(CONFIG_KEY, "WallpapersEnabled", 0)) {
        ok = 0;
    }
    append_log_line(enabled ?
        (ok ? "Windows 2000 style wallpapers installed to My Pictures."
            : "ERROR: one or more wallpapers could not be installed.") :
        (ok ? "Managed wallpaper installation disabled."
            : "ERROR: managed wallpaper cleanup was incomplete."));
    return ok;
}

static LONG open_default_user_key(const char *relative, REGSAM access, int create,
                                  HKEY *result)
{
    char path[512];
    DWORD disposition;
    _snprintf(path, sizeof(path), ".DEFAULT\\%s", relative);
    if (create)
        return RegCreateKeyExA(HKEY_USERS, path, 0, NULL, 0, access, NULL,
                               result, &disposition);
    return RegOpenKeyExA(HKEY_USERS, path, 0, access, result);
}

static int read_default_user_value(const char *subkey, const char *name, DWORD *type,
                                   BYTE *value, DWORD *size)
{
    HKEY key;
    LONG status = open_default_user_key(subkey, KEY_QUERY_VALUE, 0, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int write_default_user_value(const char *subkey, const char *name, DWORD type,
                                    const BYTE *value, DWORD size)
{
    HKEY key;
    LONG status = open_default_user_key(subkey, KEY_SET_VALUE, 1, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int delete_default_user_value(const char *subkey, const char *name)
{
    HKEY key;
    LONG status = open_default_user_key(subkey, KEY_SET_VALUE, 0, &key);
    if (status == ERROR_FILE_NOT_FOUND) return 1;
    if (status != ERROR_SUCCESS) return 0;
    status = RegDeleteValueA(key, name);
    RegCloseKey(key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

static void capture_original_user_value(const char *marker, const char *subkey,
                                        const char *name)
{
    DWORD captured, type = 0, size = 0;
    BYTE *data = NULL;
    char captured_name[160], present_name[160], type_name[160], data_name[160];
    _snprintf(captured_name, sizeof(captured_name), "Original_%s_Captured", marker);
    _snprintf(present_name, sizeof(present_name), "Original_%s_Present", marker);
    _snprintf(type_name, sizeof(type_name), "Original_%s_Type", marker);
    _snprintf(data_name, sizeof(data_name), "Original_%s_Data", marker);
    if (read_user_dword(CONFIG_KEY, captured_name, &captured)) return;
    if (read_user_value(subkey, name, &type, NULL, &size)) {
        if (size) data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if ((!size || data) && read_user_value(subkey, name, &type, data, &size)) {
            write_user_dword(CONFIG_KEY, present_name, 1);
            write_user_dword(CONFIG_KEY, type_name, type);
            write_user_binary(CONFIG_KEY, data_name, data, size);
        } else {
            write_user_dword(CONFIG_KEY, present_name, 0);
        }
    } else {
        write_user_dword(CONFIG_KEY, present_name, 0);
    }
    if (data) HeapFree(GetProcessHeap(), 0, data);
    write_user_dword(CONFIG_KEY, captured_name, 1);
}

static int capture_original_user_value_checked(const char *marker,
                                               const char *subkey,
                                               const char *name)
{
    char captured_name[160];
    DWORD captured = 0;
    capture_original_user_value(marker, subkey, name);
    _snprintf(captured_name, sizeof(captured_name), "Original_%s_Captured", marker);
    return read_user_dword(CONFIG_KEY, captured_name, &captured) && captured;
}

static int restore_original_user_value(const char *marker, const char *subkey,
                                       const char *name)
{
    DWORD present = 0, type = 0, size = 0;
    BYTE *data = NULL;
    char present_name[160], type_name[160], data_name[160];
    _snprintf(present_name, sizeof(present_name), "Original_%s_Present", marker);
    _snprintf(type_name, sizeof(type_name), "Original_%s_Type", marker);
    _snprintf(data_name, sizeof(data_name), "Original_%s_Data", marker);
    if (!read_user_dword(CONFIG_KEY, present_name, &present) || !present)
        return delete_user_value(subkey, name);
    if (!read_user_dword(CONFIG_KEY, type_name, &type)) return 0;
    if (!read_user_binary(CONFIG_KEY, data_name, NULL, &size)) return 0;
    if (size) data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
    if (size && !data) return 0;
    if (!read_user_binary(CONFIG_KEY, data_name, data, &size)) {
        HeapFree(GetProcessHeap(), 0, data);
        return 0;
    }
    present = write_user_value(subkey, name, type, data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return present != 0;
}

static int build_sound_event_key(const WINDOWS_SOUND_EVENT *event,
                                 const char *branch, char *output,
                                 size_t capacity)
{
    int written;
    if (!event || !branch || !output || !capacity) return 0;
    written = _snprintf(output, capacity,
        "AppEvents\\Schemes\\Apps\\%s\\%s\\.%s",
        event->application, event->event, branch);
    if (written < 0 || (size_t)written >= capacity) {
        output[0] = '\0';
        return 0;
    }
    return 1;
}

static int build_windows_2000_sound_value(const WINDOWS_SOUND_EVENT *event,
                                          char *output, size_t capacity)
{
    char relative[MAX_PATH];
    int written;
    if (!event || !output || !capacity) return 0;
    if (!event->file_name[0]) {
        output[0] = '\0';
        return 1;
    }
    written = _snprintf(relative, sizeof(relative),
                        "Sounds\\Windows2000\\%s", event->file_name);
    if (written < 0 || (size_t)written >= sizeof(relative)) return 0;
    return join_path(output, capacity, g_install_root, relative);
}

static int windows_2000_sounds_detected(void)
{
    DWORD enabled = 0;
    size_t index;
    if (!read_user_dword(CONFIG_KEY, "Windows2000SoundsEnabled", &enabled) || !enabled)
        return 0;
    for (index = 0;
         index < sizeof(g_windows_sound_events) / sizeof(g_windows_sound_events[0]);
         ++index) {
        const WINDOWS_SOUND_EVENT *event = &g_windows_sound_events[index];
        const char *branches[] = {"Current", "Default"};
        char expected[MAX_PATH];
        size_t branch;
        if (!build_windows_2000_sound_value(event, expected, sizeof(expected))) return 0;
        if (event->file_name[0] && !file_exists(expected)) return 0;
        for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
            char key[512], actual[MAX_PATH];
            if (!build_sound_event_key(event, branches[branch], key, sizeof(key)) ||
                !read_user_string(key, "", actual, sizeof(actual)) ||
                lstrcmpiA(actual, expected) != 0)
                return 0;
        }
    }
    return 1;
}

static int apply_windows_2000_sounds(int enabled)
{
    DWORD captured = 0;
    int ok = 1;
    size_t index;
    read_user_dword(CONFIG_KEY, "Windows2000SoundsCaptured", &captured);
    if (!enabled && !captured) {
        write_user_dword(CONFIG_KEY, "Windows2000SoundsEnabled", 0);
        return 1;
    }

    for (index = 0;
         index < sizeof(g_windows_sound_events) / sizeof(g_windows_sound_events[0]);
         ++index) {
        const WINDOWS_SOUND_EVENT *event = &g_windows_sound_events[index];
        const char *branches[] = {"Current", "Default"};
        char expected[MAX_PATH];
        size_t branch;
        if (!build_windows_2000_sound_value(event, expected, sizeof(expected))) {
            append_log_line("ERROR: a Windows 2000 sound path exceeds the supported length.");
            ok = 0;
            continue;
        }
        if (enabled && event->file_name[0] && !file_exists(expected)) {
            char message[320];
            _snprintf(message, sizeof(message),
                      "ERROR: included Windows 2000 sound is missing: %s",
                      event->file_name);
            append_log_line(message);
            ok = 0;
            continue;
        }
        for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
            char key[512], marker[160], present_name[160];
            DWORD original_present = 0;
            if (!build_sound_event_key(event, branches[branch], key, sizeof(key))) {
                ok = 0;
                continue;
            }
            _snprintf(marker, sizeof(marker), "%s_%s", event->marker, branches[branch]);
            _snprintf(present_name, sizeof(present_name),
                      "Original_%s_Present", marker);
            if (enabled) {
                capture_original_user_value(marker, key, "");
                if (!read_user_dword(CONFIG_KEY, present_name, &original_present)) {
                    ok = 0;
                    continue;
                }
                ok &= write_user_string(key, "", expected);
            } else {
                if (!read_user_dword(CONFIG_KEY, present_name, &original_present)) {
                    ok = 0;
                    continue;
                }
                if (original_present) {
                    ok &= restore_original_user_value(marker, key, "");
                } else {
                    ok &= delete_user_value(key, "");
                    ok &= delete_user_key_if_empty(key);
                }
            }
        }
    }

    if (enabled) {
        if (ok) ok &= write_user_dword(CONFIG_KEY, "Windows2000SoundsCaptured", 1);
        if (ok) ok &= write_user_dword(CONFIG_KEY, "Windows2000SoundsEnabled", 1);
    } else {
        if (ok) ok &= write_user_dword(CONFIG_KEY, "Windows2000SoundsEnabled", 0);
    }
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)"AppEvents", SMTO_ABORTIFHUNG, 3000, NULL);
    append_log_line(enabled
        ? (ok ? "All Windows XP system-event sounds were replaced with Windows 2000 equivalents."
              : "ERROR: one or more Windows sound events could not be converted.")
        : (ok ? "Original Windows sound assignments restored."
              : "ERROR: one or more original Windows sound assignments could not be restored."));
    return ok;
}

static int windows_2000_double_click_sound_detected(void)
{
    const WINDOWS_SOUND_EVENT *event = &g_windows_2000_double_click_sound;
    const char *branches[] = {"Current", "Default"};
    char expected[MAX_PATH];
    size_t branch;
    if (!build_windows_2000_sound_value(event, expected, sizeof(expected)) ||
        !file_exists(expected))
        return 0;
    for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
        char key[512], actual[MAX_PATH];
        if (!build_sound_event_key(event, branches[branch], key, sizeof(key)) ||
            !read_user_string(key, "", actual, sizeof(actual)) ||
            lstrcmpiA(actual, expected) != 0)
            return 0;
    }
    return 1;
}

static int apply_windows_2000_double_click_sound(int enabled)
{
    const WINDOWS_SOUND_EVENT *event = &g_windows_2000_double_click_sound;
    const char *branches[] = {"Current", "Default"};
    char expected[MAX_PATH];
    int ok = 1;
    size_t branch;

    if (!build_windows_2000_sound_value(event, expected, sizeof(expected))) {
        append_log_line("ERROR: the Windows 2000 folder double-click sound path exceeds the supported length.");
        return 0;
    }
    if (enabled && !file_exists(expected)) {
        append_log_line("ERROR: the exact Windows 2000 folder double-click sound is missing: start.wav");
        return 0;
    }

    for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
        char key[512], marker[160], present_name[160];
        DWORD original_present = 0;
        if (!build_sound_event_key(event, branches[branch], key, sizeof(key))) {
            ok = 0;
            continue;
        }
        _snprintf(marker, sizeof(marker), "%s_%s", event->marker, branches[branch]);
        _snprintf(present_name, sizeof(present_name), "Original_%s_Present", marker);
        if (enabled) {
            capture_original_user_value(marker, key, "");
            if (!read_user_dword(CONFIG_KEY, present_name, &original_present)) {
                ok = 0;
                continue;
            }
            ok &= write_user_string(key, "", expected);
        } else {
            if (!read_user_dword(CONFIG_KEY, present_name, &original_present))
                continue;
            if (original_present) {
                ok &= restore_original_user_value(marker, key, "");
            } else {
                ok &= delete_user_value(key, "");
                ok &= delete_user_key_if_empty(key);
            }
        }
    }

    if (ok)
        ok &= write_user_dword(CONFIG_KEY,
            "Windows2000DoubleClickSoundEnabled", enabled ? 1 : 0);
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)"AppEvents", SMTO_ABORTIFHUNG, 3000, NULL);
    append_log_line(enabled
        ? (ok ? "Exact Windows 2000 folder double-click sound enabled."
              : "ERROR: the Windows 2000 folder double-click sound could not be enabled.")
        : (ok ? "Original folder double-click sound restored."
              : "ERROR: the original folder double-click sound could not be restored."));
    return ok;
}

static void capture_original_default_value(const char *marker, const char *subkey,
                                           const char *name)
{
    DWORD captured, type = 0, size = 0;
    BYTE *data = NULL;
    char captured_name[160], present_name[160], type_name[160], data_name[160];
    _snprintf(captured_name, sizeof(captured_name), "Original_Default_%s_Captured", marker);
    _snprintf(present_name, sizeof(present_name), "Original_Default_%s_Present", marker);
    _snprintf(type_name, sizeof(type_name), "Original_Default_%s_Type", marker);
    _snprintf(data_name, sizeof(data_name), "Original_Default_%s_Data", marker);
    if (read_user_dword(CONFIG_KEY, captured_name, &captured)) return;
    if (read_default_user_value(subkey, name, &type, NULL, &size)) {
        if (size) data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if ((!size || data) && read_default_user_value(subkey, name, &type, data, &size)) {
            write_user_dword(CONFIG_KEY, present_name, 1);
            write_user_dword(CONFIG_KEY, type_name, type);
            write_user_binary(CONFIG_KEY, data_name, data, size);
        } else {
            write_user_dword(CONFIG_KEY, present_name, 0);
        }
    } else {
        write_user_dword(CONFIG_KEY, present_name, 0);
    }
    if (data) HeapFree(GetProcessHeap(), 0, data);
    write_user_dword(CONFIG_KEY, captured_name, 1);
}

static int capture_original_default_value_checked(const char *marker,
                                                  const char *subkey,
                                                  const char *name)
{
    char captured_name[160];
    DWORD captured = 0;
    capture_original_default_value(marker, subkey, name);
    _snprintf(captured_name, sizeof(captured_name),
              "Original_Default_%s_Captured", marker);
    return read_user_dword(CONFIG_KEY, captured_name, &captured) && captured;
}

static int restore_original_default_value(const char *marker, const char *subkey,
                                          const char *name)
{
    DWORD present = 0, type = 0, size = 0;
    BYTE *data = NULL;
    char present_name[160], type_name[160], data_name[160];
    _snprintf(present_name, sizeof(present_name), "Original_Default_%s_Present", marker);
    _snprintf(type_name, sizeof(type_name), "Original_Default_%s_Type", marker);
    _snprintf(data_name, sizeof(data_name), "Original_Default_%s_Data", marker);
    if (!read_user_dword(CONFIG_KEY, present_name, &present) || !present)
        return delete_default_user_value(subkey, name);
    if (!read_user_dword(CONFIG_KEY, type_name, &type)) return 0;
    if (!read_user_binary(CONFIG_KEY, data_name, NULL, &size)) return 0;
    if (size) data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
    if (size && !data) return 0;
    if (!read_user_binary(CONFIG_KEY, data_name, data, &size)) {
        HeapFree(GetProcessHeap(), 0, data);
        return 0;
    }
    present = write_default_user_value(subkey, name, type, data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return present != 0;
}

static int read_start_panel_on(DWORD *panel_on)
{
    BYTE shell_state[256];
    DWORD size = sizeof(shell_state);
    if (!read_user_binary("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                          "ShellState", shell_state, &size) || size < 36) return 0;
    *panel_on = (shell_state[32] & 0x02) ? 1 : 0;
    return 1;
}

static int write_start_panel_on(DWORD panel_on)
{
    BYTE shell_state[256];
    DWORD size = sizeof(shell_state);
    DWORD verified = !panel_on;
    if (!read_user_binary("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                          "ShellState", shell_state, &size) || size < 36) return 0;
    if (panel_on) shell_state[32] |= 0x02;
    else shell_state[32] &= (BYTE)~0x02;
    if (!write_user_binary("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                           "ShellState", shell_state, size)) return 0;
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
        SMTO_ABORTIFHUNG, 3000, NULL);
    return read_start_panel_on(&verified) && verified == panel_on;
}

static int read_machine_dword(const char *subkey, const char *name, DWORD *value)
{
    HKEY key;
    SYSTEM_INFO system_info;
    REGSAM access = KEY_QUERY_VALUE;
    DWORD type = 0, size = sizeof(*value);
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        access |= KEY_WOW64_64KEY;
    LONG status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, access, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, &type, (BYTE *)value, &size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS && type == REG_DWORD;
}

static int write_machine_dword(const char *subkey, const char *name, DWORD value)
{
    HKEY key;
    SYSTEM_INFO system_info;
    REGSAM access = KEY_SET_VALUE;
    DWORD disposition;
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        access |= KEY_WOW64_64KEY;
    LONG status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, NULL, 0,
        access, NULL, &key, &disposition);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, REG_DWORD, (BYTE *)&value, sizeof(value));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int write_machine_string(const char *subkey, const char *name,
                                const char *value)
{
    HKEY key;
    SYSTEM_INFO system_info;
    REGSAM access = KEY_SET_VALUE;
    DWORD disposition;
    LONG status;
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        access |= KEY_WOW64_64KEY;
    status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, NULL, 0,
        access, NULL, &key, &disposition);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, name, 0, REG_SZ, (const BYTE *)value,
                            (DWORD)strlen(value) + 1);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static REGSAM machine_registry_access(REGSAM access)
{
    SYSTEM_INFO system_info;
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        access |= KEY_WOW64_64KEY;
    return access;
}

static int read_machine_value(const char *subkey, const char *name, DWORD *type,
                              BYTE *value, DWORD *size)
{
    HKEY key;
    LONG status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0,
                                machine_registry_access(KEY_QUERY_VALUE), &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, (name && *name) ? name : NULL, NULL,
                              type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int write_machine_value(const char *subkey, const char *name, DWORD type,
                               const BYTE *value, DWORD size)
{
    HKEY key;
    DWORD disposition;
    LONG status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, NULL, 0,
                                  machine_registry_access(KEY_SET_VALUE), NULL,
                                  &key, &disposition);
    if (status != ERROR_SUCCESS) return 0;
    status = RegSetValueExA(key, (name && *name) ? name : NULL, 0,
                            type, value, size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static int delete_machine_value(const char *subkey, const char *name);

static void capture_original_machine_value(const char *marker,
                                           const char *subkey,
                                           const char *name)
{
    char captured_name[160], present_name[160], type_name[160], data_name[160];
    DWORD captured = 0, type = 0, size = 0;
    BYTE *data = NULL;
    _snprintf(captured_name, sizeof(captured_name), "Captured_%s", marker);
    _snprintf(present_name, sizeof(present_name), "Present_%s", marker);
    _snprintf(type_name, sizeof(type_name), "Type_%s", marker);
    _snprintf(data_name, sizeof(data_name), "Data_%s", marker);
    if (read_machine_dword(EXPLORER_MACHINE_STATE_KEY, captured_name, &captured))
        return;
    if (read_machine_value(subkey, name, &type, NULL, &size)) {
        if (size) data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if ((!size || data) && read_machine_value(subkey, name, &type, data, &size)) {
            write_machine_dword(EXPLORER_MACHINE_STATE_KEY, present_name, 1);
            write_machine_dword(EXPLORER_MACHINE_STATE_KEY, type_name, type);
            write_machine_value(EXPLORER_MACHINE_STATE_KEY, data_name,
                                REG_BINARY, data, size);
        } else {
            write_machine_dword(EXPLORER_MACHINE_STATE_KEY, present_name, 0);
        }
    } else {
        write_machine_dword(EXPLORER_MACHINE_STATE_KEY, present_name, 0);
    }
    if (data) HeapFree(GetProcessHeap(), 0, data);
    write_machine_dword(EXPLORER_MACHINE_STATE_KEY, captured_name, 1);
}

static int capture_original_machine_value_checked(const char *marker,
                                                  const char *subkey,
                                                  const char *name)
{
    char captured_name[160];
    DWORD captured = 0;
    capture_original_machine_value(marker, subkey, name);
    _snprintf(captured_name, sizeof(captured_name), "Captured_%s", marker);
    return read_machine_dword(EXPLORER_MACHINE_STATE_KEY,
                              captured_name, &captured) && captured;
}

static int restore_original_machine_value(const char *marker,
                                          const char *subkey,
                                          const char *name)
{
    char present_name[160], type_name[160], data_name[160];
    DWORD present = 0, type = 0, size = 0;
    BYTE *data = NULL;
    int result;
    _snprintf(present_name, sizeof(present_name), "Present_%s", marker);
    _snprintf(type_name, sizeof(type_name), "Type_%s", marker);
    _snprintf(data_name, sizeof(data_name), "Data_%s", marker);
    if (!read_machine_dword(EXPLORER_MACHINE_STATE_KEY, present_name, &present) ||
        !present)
        return delete_machine_value(subkey, name);
    if (!read_machine_dword(EXPLORER_MACHINE_STATE_KEY, type_name, &type) ||
        !read_machine_value(EXPLORER_MACHINE_STATE_KEY, data_name, &type,
                            NULL, &size))
        return 0;
    if (size) {
        data = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if (!data) return 0;
        if (!read_machine_value(EXPLORER_MACHINE_STATE_KEY, data_name, &type,
                                data, &size)) {
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
    }
    {
        DWORD original_type = 0;
        if (!read_machine_dword(EXPLORER_MACHINE_STATE_KEY, type_name,
                                &original_type)) {
            if (data) HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
        result = write_machine_value(subkey, name, original_type, data, size);
    }
    if (data) HeapFree(GetProcessHeap(), 0, data);
    return result;
}

static int delete_machine_value(const char *subkey, const char *name)
{
    HKEY key;
    SYSTEM_INFO system_info;
    REGSAM access = KEY_SET_VALUE;
    LONG status;
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        access |= KEY_WOW64_64KEY;
    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, access, &key);
    if (status == ERROR_FILE_NOT_FOUND) return 1;
    if (status != ERROR_SUCCESS) return 0;
    status = RegDeleteValueA(key, name);
    RegCloseKey(key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

static int delete_machine_tree_view(const char *subkey, REGSAM view)
{
    char parent_path[512], child_name[160];
    char *separator;
    HKEY parent;
    LONG status;
    if (!subkey || !*subkey || strlen(subkey) >= sizeof(parent_path)) return 0;
    lstrcpynA(parent_path, subkey, sizeof(parent_path));
    separator = strrchr(parent_path, '\\');
    if (!separator || !separator[1]) return 0;
    lstrcpynA(child_name, separator + 1, sizeof(child_name));
    *separator = 0;
    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, parent_path, 0,
                           KEY_READ | KEY_WRITE | view, &parent);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) return 1;
    if (status != ERROR_SUCCESS) return 0;
    status = SHDeleteKeyA(parent, child_name);
    RegCloseKey(parent);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND ||
           status == ERROR_PATH_NOT_FOUND;
}

static int delete_machine_tree(const char *subkey)
{
    SYSTEM_INFO system_info;
    int ok;
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64)
        return delete_machine_tree_view(subkey, 0);

    /* XP x64 reflects portions of HKLM\Software\Classes between registry
       views, but a recursive delete performed through one view can leave the
       reflected sibling behind.  These trees are owned uniquely by this
       experiment, so remove both views explicitly during restoration. */
    ok = delete_machine_tree_view(subkey, KEY_WOW64_64KEY);
    ok &= delete_machine_tree_view(subkey, KEY_WOW64_32KEY);
    return ok;
}

static int configure_resource_reloader(int enabled)
{
    const char *run_key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char *value_name = "eXPerience2K Resource Reloader";
    char application[MAX_PATH];
    char command[2 * MAX_PATH + 64];
    if (!enabled)
        return restore_original_machine_value("ResourceReloaderRun",
                                              run_key, value_name);
    if (!join_path(application, sizeof(application), g_install_root,
                   "eXPerience2K.exe") ||
        _snprintf(command, sizeof(command), "\"%s\" /reload-resources",
                  application) < 0)
        return 0;
    return write_machine_string(run_key, value_name, command);
}

static int token_is_administrator(void)
{
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administrators = NULL;
    BOOL member = FALSE;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administrators)) return 0;
    CheckTokenMembership(NULL, administrators, &member);
    FreeSid(administrators);
    return member ? 1 : 0;
}

static void redact_and_append(const char *source)
{
    char buffer[8192];
    char user_profile[MAX_PATH];
    const char *cursor = source;
    size_t used = 0, profile_length = 0;
    if (GetEnvironmentVariableA("USERPROFILE", user_profile, sizeof(user_profile)))
        profile_length = strlen(user_profile);
    while (*cursor && used + 1 < sizeof(buffer)) {
        if (profile_length && _strnicmp(cursor, user_profile, profile_length) == 0) {
            const char *replacement = "%USERPROFILE%";
            size_t length = strlen(replacement);
            if (used + length >= sizeof(buffer)) break;
            CopyMemory(buffer + used, replacement, length);
            used += length;
            cursor += profile_length;
        } else {
            buffer[used++] = *cursor++;
        }
    }
    buffer[used] = '\0';
    append_log(buffer);
}

static int run_captured(const char *command_line)
{
    SECURITY_ATTRIBUTES security;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    HANDLE read_pipe = NULL, write_pipe = NULL;
    char mutable_command[4096];
    char buffer[4096];
    DWORD available, read, exit_code = 1;
    int finished = 0;
    ZeroMemory(&security, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    if (!CreatePipe(&read_pipe, &write_pipe, &security, 0)) return 0;
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = write_pipe;
    startup.hStdError = write_pipe;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    ZeroMemory(&process, sizeof(process));
    lstrcpynA(mutable_command, command_line, sizeof(mutable_command));
    if (!CreateProcessA(NULL, mutable_command, NULL, NULL, TRUE, CREATE_NO_WINDOW,
        NULL, g_install_root, &startup, &process)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return 0;
    }
    CloseHandle(write_pipe);
    while (!finished) {
        while (PeekNamedPipe(read_pipe, NULL, 0, NULL, &available, NULL) && available) {
            DWORD amount = available < sizeof(buffer) - 1 ? available : sizeof(buffer) - 1;
            if (!ReadFile(read_pipe, buffer, amount, &read, NULL) || !read) break;
            buffer[read] = '\0';
            redact_and_append(buffer);
        }
        if (WaitForSingleObject(process.hProcess, 50) == WAIT_OBJECT_0) finished = 1;
        if (g_apply_in_progress) pump_ui_messages();
    }
    while (ReadFile(read_pipe, buffer, sizeof(buffer) - 1, &read, NULL) && read) {
        buffer[read] = '\0';
        redact_and_append(buffer);
    }
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(read_pipe);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exit_code == 0;
}

static int parse_yes(const char *value)
{
    return lstrcmpiA(value, "yes") == 0;
}

static int parse_probe_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char line[1024];
    if (!file) return 0;
    ZeroMemory(&g_probe, sizeof(g_probe));
    while (fgets(line, sizeof(line), file)) {
        char *tab = strchr(line, '\t');
        char *value, *end;
        if (!tab) continue;
        *tab++ = '\0';
        value = tab;
        end = value + strlen(value);
        while (end > value && (end[-1] == '\r' || end[-1] == '\n')) --end;
        *end = '\0';
        if (lstrcmpiA(line, "supported") == 0) g_probe.supported = parse_yes(value);
        else if (lstrcmpiA(line, "resource_profile_ready") == 0) g_probe.resource_ready = parse_yes(value);
        else if (lstrcmpiA(line, "administrator") == 0) g_probe.administrator = parse_yes(value);
        else if (lstrcmpiA(line, "profile_id") == 0) lstrcpynA(g_probe.profile_id, value, sizeof(g_probe.profile_id));
        else if (lstrcmpiA(line, "display_name") == 0) lstrcpynA(g_probe.display_name, value, sizeof(g_probe.display_name));
        else if (lstrcmpiA(line, "architecture") == 0) lstrcpynA(g_probe.architecture, value, sizeof(g_probe.architecture));
        else if (lstrcmpiA(line, "branding_label") == 0) lstrcpynA(g_probe.branding_label, value, sizeof(g_probe.branding_label));
        else if (lstrcmpiA(line, "reason") == 0) lstrcpynA(g_probe.reason, value, sizeof(g_probe.reason));
    }
    fclose(file);
    return 1;
}

static int run_probe(void)
{
    char temp_path[MAX_PATH], temp_file[MAX_PATH], command[1024];
    if (!GetTempPathA(sizeof(temp_path), temp_path) ||
        !GetTempFileNameA(temp_path, "e2k", 0, temp_file)) return 0;
    DeleteFileA(temp_file);
    _snprintf(command, sizeof(command), "\"%s\" probe \"%s\"", g_core_path, temp_file);
    append_log_line("Running privacy-safe operating-system probe...");
    if (!run_captured(command) || !parse_probe_file(temp_file)) {
        DeleteFileA(temp_file);
        return 0;
    }
    DeleteFileA(temp_file);
    return 1;
}

static int resource_conversion_detected(void)
{
    char state[MAX_PATH];
    DWORD configured = 0, enabled = 0;
    if (read_user_dword(CONFIG_KEY, "Configured", &configured) && configured &&
        read_user_dword(CONFIG_KEY, "ResourceConversionEnabled", &enabled))
        return enabled != 0;
    return join_path(state, sizeof(state), g_install_root, "state.tsv") && file_exists(state);
}

static int user_string_equals(const char *subkey, const char *name, const char *expected)
{
    char value[128];
    return read_user_string(subkey, name, value, sizeof(value)) &&
           lstrcmpiA(value, expected) == 0;
}

static int caption_preset_valid(DWORD preset)
{
    return preset == CAPTION_PRESET_SOLID_NAVY ||
           preset == CAPTION_PRESET_BLUE_GRADIENT;
}

static int saved_caption_preset(const char *name, int fallback)
{
    DWORD preset = (DWORD)fallback;
    if (read_user_dword(CONFIG_KEY, name, &preset) && caption_preset_valid(preset))
        return (int)preset;
    return fallback;
}

static int selected_caption_preset(HWND combo, int fallback)
{
    LRESULT selection;
    if (!combo) return fallback;
    selection = SendMessageA(combo, CB_GETCURSEL, 0, 0);
    if (selection == CB_ERR || !caption_preset_valid((DWORD)selection)) return fallback;
    return (int)selection;
}

static int selected_logon_caption_preset(void)
{
    return selected_caption_preset(g_logon_caption_combo,
                                   CAPTION_PRESET_SOLID_NAVY);
}

static int selected_desktop_caption_preset(void)
{
    return selected_caption_preset(g_desktop_caption_combo,
                                   CAPTION_PRESET_BLUE_GRADIENT);
}

static const char *caption_preset_name(int preset)
{
    return preset == CAPTION_PRESET_SOLID_NAVY ? "Solid Navy" : "Blue Gradient";
}

static const char *caption_color_value(const char *color_name,
                                       const char *normal_value,
                                       int preset)
{
    if (preset == CAPTION_PRESET_SOLID_NAVY &&
        (lstrcmpiA(color_name, "ActiveTitle") == 0 ||
         lstrcmpiA(color_name, "GradientActiveTitle") == 0))
        return "0 0 128";
    return normal_value;
}

static int w2k_color_profile_detected(void)
{
    DWORD enabled = 0;
    BYTE preferences[16];
    DWORD preference_size = sizeof(preferences);
    LOGFONTW caption_font, menu_font;
    DWORD caption_size = sizeof(caption_font), menu_size = sizeof(menu_font);
    BOOL gradient = FALSE;
    int preset = saved_caption_preset("DesktopCaptionPreset",
                                      CAPTION_PRESET_BLUE_GRADIENT);
    const char *active_title = preset == CAPTION_PRESET_SOLID_NAVY
        ? "0 0 128" : "10 36 106";
    const char *gradient_active_title = preset == CAPTION_PRESET_SOLID_NAVY
        ? "0 0 128" : "166 202 240";
    BYTE expected_gradient_bit = preset == CAPTION_PRESET_SOLID_NAVY ? 0x00 : 0x10;
    if (!read_user_dword(CONFIG_KEY, "W2KColorProfileEnabled", &enabled) || !enabled ||
        !user_string_equals("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                            "ThemeActive", "0") ||
        !user_string_equals("Control Panel\\Colors", "ActiveTitle", active_title) ||
        !user_string_equals("Control Panel\\Colors", "GradientActiveTitle",
                            gradient_active_title) ||
        !user_string_equals("Control Panel\\Colors", "ButtonFace", "212 208 200") ||
        !user_string_equals("Control Panel\\Colors", "Hilight", "10 36 106") ||
        !user_string_equals("Control Panel\\Desktop\\WindowMetrics", "CaptionHeight", "-270") ||
        !read_user_binary("Control Panel\\Desktop", "UserPreferencesMask",
                          preferences, &preference_size) || preference_size < 4 ||
        (preferences[0] & 0x10) != expected_gradient_bit)
        return 0;
    ZeroMemory(&caption_font, sizeof(caption_font));
    ZeroMemory(&menu_font, sizeof(menu_font));
    if (!read_user_binary("Control Panel\\Desktop\\WindowMetrics", "CaptionFont",
                          (BYTE *)&caption_font, &caption_size) ||
        caption_size != sizeof(caption_font) || caption_font.lfHeight != -11 ||
        caption_font.lfWeight != FW_BOLD ||
        lstrcmpiW(caption_font.lfFaceName, L"Tahoma") != 0 ||
        !read_user_binary("Control Panel\\Desktop\\WindowMetrics", "MenuFont",
                          (BYTE *)&menu_font, &menu_size) ||
        menu_size != sizeof(menu_font) || menu_font.lfHeight != -11 ||
        menu_font.lfWeight != FW_NORMAL ||
        lstrcmpiW(menu_font.lfFaceName, L"Tahoma") != 0)
        return 0;
    if (!g_cross_user) {
        if (!SystemParametersInfoA(SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0)) return 0;
        if ((preset == CAPTION_PRESET_BLUE_GRADIENT && !gradient) ||
            (preset == CAPTION_PRESET_SOLID_NAVY && gradient)) return 0;
    }
    return 1;
}

static int explorer_experiment_detected(void);

static int explorer_experiment_architecture_supported(void)
{
    SYSTEM_INFO information;
    ZeroMemory(&information, sizeof(information));
    GetNativeSystemInfo(&information);
    return information.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ||
           information.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
}

static const char *explorer_band_filename(void)
{
    SYSTEM_INFO information;
    ZeroMemory(&information, sizeof(information));
    GetNativeSystemInfo(&information);
    if (information.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        return "eXPerience2KExplorerBand64.dll";
    if (information.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        return "eXPerience2KExplorerBand32.dll";
    return "";
}

static int detect_feature_state(int index)
{
    DWORD value = 0;
    BOOL enabled = FALSE, fade = TRUE;
    switch (index) {
    case FEATURE_RESOURCE_CONVERSION:
        return resource_conversion_detected();
    case FEATURE_CLASSIC_THEME:
        return w2k_color_profile_detected();
    case FEATURE_CLASSIC_START_MENU:
        if (read_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
            "NoSimpleStartMenu", &value) && value != 0) return 1;
        return read_start_panel_on(&value) && value == 0;
    case FEATURE_CLASSIC_CONTROL_PANEL:
        if (read_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
            "ForceClassicControlPanel", &value) && value != 0) return 1;
        return read_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel",
            "StartupPage", &value) && value == 0;
    case FEATURE_MENU_SLIDE:
        SystemParametersInfoA(SPI_GETMENUANIMATION, 0, &enabled, 0);
        SystemParametersInfoA(SPI_GETMENUFADE, 0, &fade, 0);
        return enabled && !fade;
    case FEATURE_MENU_FADE:
        SystemParametersInfoA(SPI_GETMENUANIMATION, 0, &enabled, 0);
        SystemParametersInfoA(SPI_GETMENUFADE, 0, &fade, 0);
        return enabled && fade;
    case FEATURE_CLASSIC_LOGON:
        return read_machine_dword("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
            "LogonType", &value) && value == 0;
    case FEATURE_WALLPAPERS:
        return wallpapers_installed_detected();
    case FEATURE_CLASSIC_EXPLORER:
        return explorer_experiment_detected();
    case FEATURE_WINDOWS_2000_SOUNDS:
        return windows_2000_sounds_detected();
    case FEATURE_WINDOWS_2000_DOUBLE_CLICK_SOUND:
        return windows_2000_double_click_sound_detected();
    default:
        return 0;
    }
}

typedef struct {
    const char *marker;
    const char *subkey;
    const char *name;
    DWORD value;
} EXPLORER_DWORD_VALUE;

typedef struct {
    const char *marker;
    const char *subkey;
    const char *name;
    const char *value;
} EXPLORER_STRING_VALUE;

typedef struct {
    const char *marker;
    const char *subkey;
    const char *name;
    const char *asset;
} EXPLORER_BINARY_VALUE;

static const EXPLORER_DWORD_VALUE g_explorer_dwords[] = {
    {"ExplorerHideFileExt", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "HideFileExt", 0},
    {"ExplorerHidden", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "Hidden", 2},
    {"ExplorerShowCompColor", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowCompColor", 0},
    {"ExplorerDontPrettyPath", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "DontPrettyPath", 0},
    {"ExplorerShowInfoTip", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowInfoTip", 1},
    {"ExplorerHideIcons", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "HideIcons", 0},
    {"ExplorerMapNetDrvBtn", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "MapNetDrvBtn", 0},
    /* XP's Common Tasks pane must be off because the architecture-matched
       desk band supplies the authentic Windows 2000 information pane. */
    {"ExplorerWebView", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "WebView", 0},
    {"ExplorerFilter", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "Filter", 0},
    {"ExplorerSuperHidden", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "SuperHidden", 0},
    {"ExplorerSeparateProcess", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "SeparateProcess", 0},
    {"ExplorerFullPathAddress", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState", "FullPathAddress", 1},
    {"ExplorerFullPath", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState", "FullPath", 0},
    {"ExplorerToolbarLocked", "Software\\Microsoft\\Internet Explorer\\Toolbar", "Locked", 1},
    /* FOLDERVIEWMODE value 1 is FVM_ICON.  Establish it after the saved
       ShellNoRoam bags are cleared so My Computer and newly opened folders
       do not fall back to XP's FVM_TILE default. */
    {"ExplorerAllFoldersIconMode", "Software\\Microsoft\\Windows\\ShellNoRoam\\Bags\\AllFolders\\Shell", "Mode", 1}
};

static const EXPLORER_STRING_VALUE g_explorer_strings[] = {
    {"ExplorerCascadeControlPanel", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "CascadeControlPanel", "NO"},
    {"ExplorerIntelliMenus", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "IntelliMenus", "No"},
    {"ExplorerCascadeMyDocuments", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "CascadeMyDocuments", "NO"},
    {"ExplorerLinksFolderName", "Software\\Microsoft\\Internet Explorer\\Toolbar", "LinksFolderName", "Links"}
};

static const EXPLORER_BINARY_VALUE g_explorer_binaries[] = {
    {"ExplorerShellState", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "ShellState", "ShellState.bin"},
    {"ExplorerCabinetSettings", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState", "Settings", "CabinetSettings.bin"},
    {"ExplorerCabView", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams\\0", "CabView", "CabView.bin"},
    {"ExplorerViewView20", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams\\0", "ViewView2", "ViewView2-0.bin"},
    {"ExplorerViewView21", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams\\1", "ViewView2", "ViewView2-1.bin"},
    {"ExplorerSaveLinksOrder", "Software\\Microsoft\\Internet Explorer\\Toolbar", "SaveLinksOrder", "SaveLinksOrder.bin"},
    {"ExplorerITBarLayout", "Software\\Microsoft\\Internet Explorer\\Toolbar\\Explorer", "ITBarLayout", "ITBarLayout.bin"},
    {"ExplorerShellBrowserBand", "Software\\Microsoft\\Internet Explorer\\Toolbar\\ShellBrowser", "{01E04581-4EEE-11D0-BFE9-00AA005B4383}", "ShellBrowserBand.bin"}
};

static int read_explorer_state_asset(const char *asset, BYTE **data, DWORD *size)
{
    char directory[MAX_PATH], path[MAX_PATH];
    HANDLE file;
    DWORD high = 0, low, read = 0;
    BYTE *buffer;
    if (!data || !size ||
        !join_path(directory, sizeof(directory), g_install_root, "ExplorerState") ||
        !join_path(path, sizeof(path), directory, asset)) return 0;
    *data = NULL;
    *size = 0;
    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    low = GetFileSize(file, &high);
    if (low == INVALID_FILE_SIZE || high || !low || low > 65536) {
        CloseHandle(file);
        return 0;
    }
    buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, low);
    if (!buffer || !ReadFile(file, buffer, low, &read, NULL) || read != low) {
        if (buffer) HeapFree(GetProcessHeap(), 0, buffer);
        CloseHandle(file);
        return 0;
    }
    CloseHandle(file);
    *data = buffer;
    *size = low;
    return 1;
}

static int write_user_binary_asset(const EXPLORER_BINARY_VALUE *item)
{
    BYTE *data = NULL;
    DWORD size = 0;
    int result;
    if (!read_explorer_state_asset(item->asset, &data, &size)) return 0;
    result = write_user_binary(item->subkey, item->name, data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return result;
}

static int capture_user_tree(const char *marker, const char *source_path)
{
    char captured_name[128], present_name[128], backup_path[512];
    DWORD captured = 0;
    HKEY source = NULL, destination = NULL;
    LONG status;
    _snprintf(captured_name, sizeof(captured_name), "ExplorerTree_%s_Captured", marker);
    _snprintf(present_name, sizeof(present_name), "ExplorerTree_%s_Present", marker);
    _snprintf(backup_path, sizeof(backup_path), "%s\\ExplorerTreeBackup\\%s",
              CONFIG_KEY, marker);
    if (read_user_dword(CONFIG_KEY, captured_name, &captured)) return 1;
    delete_user_tree(backup_path);
    status = open_user_key(source_path, KEY_READ, 0, &source);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) {
        write_user_dword(CONFIG_KEY, present_name, 0);
        write_user_dword(CONFIG_KEY, captured_name, 1);
        return 1;
    }
    if (status != ERROR_SUCCESS ||
        open_user_key(backup_path, KEY_WRITE, 1, &destination) != ERROR_SUCCESS) {
        if (source) RegCloseKey(source);
        return 0;
    }
    status = SHCopyKeyA(source, NULL, destination, 0);
    RegCloseKey(destination);
    RegCloseKey(source);
    if (status != ERROR_SUCCESS) return 0;
    return write_user_dword(CONFIG_KEY, present_name, 1) &&
           write_user_dword(CONFIG_KEY, captured_name, 1);
}

static int restore_user_tree(const char *marker, const char *destination_path)
{
    char present_name[128], backup_path[512];
    DWORD present = 0;
    HKEY source = NULL, destination = NULL;
    LONG status;
    _snprintf(present_name, sizeof(present_name), "ExplorerTree_%s_Present", marker);
    _snprintf(backup_path, sizeof(backup_path), "%s\\ExplorerTreeBackup\\%s",
              CONFIG_KEY, marker);
    if (!delete_user_tree(destination_path)) return 0;
    if (!read_user_dword(CONFIG_KEY, present_name, &present) || !present) return 1;
    if (open_user_key(backup_path, KEY_READ, 0, &source) != ERROR_SUCCESS ||
        open_user_key(destination_path, KEY_WRITE, 1, &destination) != ERROR_SUCCESS) {
        if (source) RegCloseKey(source);
        return 0;
    }
    status = SHCopyKeyA(source, NULL, destination, 0);
    RegCloseKey(destination);
    RegCloseKey(source);
    return status == ERROR_SUCCESS;
}

static int apply_w2k_explorer_user_state(int enabled)
{
    size_t index;
    int ok = 1;
    if (enabled) {
        ok &= capture_user_tree("BagMRU", "Software\\Microsoft\\Windows\\ShellNoRoam\\BagMRU");
        ok &= capture_user_tree("Bags", "Software\\Microsoft\\Windows\\ShellNoRoam\\Bags");
        ok &= capture_user_tree("StreamMRU", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StreamMRU");
        if (ok) {
            ok &= delete_user_tree("Software\\Microsoft\\Windows\\ShellNoRoam\\BagMRU");
            ok &= delete_user_tree("Software\\Microsoft\\Windows\\ShellNoRoam\\Bags");
            ok &= delete_user_tree("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StreamMRU");
        }
    }
    for (index = 0; index < sizeof(g_explorer_dwords) / sizeof(g_explorer_dwords[0]); ++index) {
        capture_original_user_value(g_explorer_dwords[index].marker,
            g_explorer_dwords[index].subkey, g_explorer_dwords[index].name);
        if (enabled)
            ok &= write_user_dword(g_explorer_dwords[index].subkey,
                                   g_explorer_dwords[index].name,
                                   g_explorer_dwords[index].value);
        else
            ok &= restore_original_user_value(g_explorer_dwords[index].marker,
                g_explorer_dwords[index].subkey, g_explorer_dwords[index].name);
    }
    for (index = 0; index < sizeof(g_explorer_strings) / sizeof(g_explorer_strings[0]); ++index) {
        capture_original_user_value(g_explorer_strings[index].marker,
            g_explorer_strings[index].subkey, g_explorer_strings[index].name);
        if (enabled)
            ok &= write_user_string(g_explorer_strings[index].subkey,
                                    g_explorer_strings[index].name,
                                    g_explorer_strings[index].value);
        else
            ok &= restore_original_user_value(g_explorer_strings[index].marker,
                g_explorer_strings[index].subkey, g_explorer_strings[index].name);
    }
    for (index = 0; index < sizeof(g_explorer_binaries) / sizeof(g_explorer_binaries[0]); ++index) {
        capture_original_user_value(g_explorer_binaries[index].marker,
            g_explorer_binaries[index].subkey, g_explorer_binaries[index].name);
        if (enabled)
            ok &= write_user_binary_asset(&g_explorer_binaries[index]);
        else
            ok &= restore_original_user_value(g_explorer_binaries[index].marker,
                g_explorer_binaries[index].subkey, g_explorer_binaries[index].name);
    }
    if (!enabled) {
        ok &= restore_user_tree("BagMRU", "Software\\Microsoft\\Windows\\ShellNoRoam\\BagMRU");
        ok &= restore_user_tree("Bags", "Software\\Microsoft\\Windows\\ShellNoRoam\\Bags");
        ok &= restore_user_tree("StreamMRU", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StreamMRU");
    }
    ok &= write_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", enabled ? 1 : 0);
    append_log_line(enabled
        ? "Exact Windows 2000 cabinet, Web View, address-bar, toolbar, and default folder state enabled."
        : "Original Explorer folder state and saved per-folder views restored.");
    if (!ok) append_log_line("ERROR: one or more Explorer user settings could not be updated or restored.");
    return ok;
}

static int explorer_experiment_detected(void)
{
    DWORD enabled = 0, value = 0;
    size_t index;
    char payload_root[MAX_PATH], payload_file[MAX_PATH], band_file[MAX_PATH];
    char windows[MAX_PATH], web_root[MAX_PATH], installed_file[MAX_PATH];
    if (!read_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", &enabled) ||
        !enabled ||
        !read_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", &enabled) ||
        !enabled) return 0;
    for (index = 0; index < sizeof(g_explorer_dwords) / sizeof(g_explorer_dwords[0]); ++index) {
        if (!read_user_dword(g_explorer_dwords[index].subkey,
                             g_explorer_dwords[index].name, &value) ||
            value != g_explorer_dwords[index].value) return 0;
    }
    for (index = 0; index < sizeof(g_explorer_strings) / sizeof(g_explorer_strings[0]); ++index) {
        if (!user_string_equals(g_explorer_strings[index].subkey,
                                g_explorer_strings[index].name,
                                g_explorer_strings[index].value)) return 0;
    }
    /* Cabinet and toolbar blobs are initial defaults, not durable enablement
       markers.  Normal Explorer actions such as showing the Folders bar or
       resizing a toolbar legitimately mutate them while the pane remains
       installed and active. */
    if (!join_path(payload_root, sizeof(payload_root), g_install_root, "ExplorerWeb") ||
        !join_path(payload_file, sizeof(payload_file), payload_root, "folder.htt") ||
        !join_path(band_file, sizeof(band_file), g_install_root,
                   explorer_band_filename()) ||
        !file_exists(band_file) ||
        !GetWindowsDirectoryA(windows, sizeof(windows)) ||
        !join_path(web_root, sizeof(web_root), windows, "Web") ||
        !join_path(installed_file, sizeof(installed_file), web_root, "folder.htt"))
        return 0;
    return files_are_identical(payload_file, installed_file);
}

static void apply_registered_colors_to_session(void)
{
    int indices[sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0])];
    COLORREF values[sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0])];
    int count = 0;
    size_t index;
    for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
        char text[64];
        unsigned red, green, blue;
        if (g_w2k_colors[index].system_index < 0) continue;
        if (!read_user_string("Control Panel\\Colors", g_w2k_colors[index].name,
                              text, sizeof(text)) ||
            sscanf(text, "%u %u %u", &red, &green, &blue) != 3) continue;
        indices[count] = g_w2k_colors[index].system_index;
        values[count] = RGB(red, green, blue);
        ++count;
    }
    if (count) SetSysColors(count, indices, values);
}

static void set_visual_style_live(int enabled)
{
    HMODULE theme;
    typedef HRESULT (WINAPI *SET_VISUAL_STYLE)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD);
    SET_VISUAL_STYLE set_style;
    if (g_cross_user) return;
    theme = LoadLibraryA("uxtheme.dll");
    if (!theme) return;
    set_style = (SET_VISUAL_STYLE)GetProcAddress(theme, "SetSystemVisualStyle");
    if (set_style) {
        if (enabled) {
            set_style(L"", L"", L"", 0);
        } else {
            char active[16], dll[512], expanded[512], color[128], size[128];
            WCHAR dll_w[512], color_w[128], size_w[128];
            if (read_user_string("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                                 "ThemeActive", active, sizeof(active)) &&
                lstrcmpiA(active, "1") == 0 &&
                read_user_string("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                                 "DllName", dll, sizeof(dll)) &&
                read_user_string("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                                 "ColorName", color, sizeof(color)) &&
                read_user_string("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                                 "SizeName", size, sizeof(size))) {
                if (!ExpandEnvironmentStringsA(dll, expanded, sizeof(expanded)))
                    lstrcpynA(expanded, dll, sizeof(expanded));
                MultiByteToWideChar(CP_ACP, 0, expanded, -1, dll_w,
                                    sizeof(dll_w) / sizeof(dll_w[0]));
                MultiByteToWideChar(CP_ACP, 0, color, -1, color_w,
                                    sizeof(color_w) / sizeof(color_w[0]));
                MultiByteToWideChar(CP_ACP, 0, size, -1, size_w,
                                    sizeof(size_w) / sizeof(size_w[0]));
                set_style(dll_w, color_w, size_w, 0);
            }
        }
    }
    FreeLibrary(theme);
}

static int apply_classic_theme(int enabled, int caption_preset)
{
    int ok = 1;
    size_t index;
    DWORD captured = 0, original_gradient = 1;
    BOOL gradient = TRUE;
    static const BYTE w2k_preferences_gradient[] = {0x9e, 0x3e, 0x00, 0x80};
    static const BYTE w2k_preferences_solid[] = {0x8e, 0x3e, 0x00, 0x80};
    const BYTE *w2k_preferences = caption_preset == CAPTION_PRESET_SOLID_NAVY
        ? w2k_preferences_solid : w2k_preferences_gradient;
    capture_original_user_value("ThemeActive",
        "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
    capture_original_user_value("UserPreferencesMask", "Control Panel\\Desktop",
                                "UserPreferencesMask");
    if (!read_user_dword(CONFIG_KEY, "Original_GradientCaptions_Captured", &captured)) {
        if (!g_cross_user &&
            SystemParametersInfoA(SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0))
            original_gradient = gradient ? 1 : 0;
        write_user_dword(CONFIG_KEY, "Original_GradientCaptions_Value", original_gradient);
        write_user_dword(CONFIG_KEY, "Original_GradientCaptions_Captured", 1);
    }
    for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Color_%s", g_w2k_colors[index].name);
        capture_original_user_value(marker, "Control Panel\\Colors", g_w2k_colors[index].name);
        if (enabled) {
            const char *color_value = caption_color_value(g_w2k_colors[index].name,
                                                          g_w2k_colors[index].text,
                                                          caption_preset);
            ok &= write_user_string("Control Panel\\Colors", g_w2k_colors[index].name,
                                    color_value);
        } else {
            ok &= restore_original_user_value(marker, "Control Panel\\Colors",
                                              g_w2k_colors[index].name);
        }
    }
    if (enabled) {
        ok &= write_user_string("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                               "ThemeActive", "0");
        ok &= write_user_binary("Control Panel\\Desktop", "UserPreferencesMask",
                                w2k_preferences, sizeof(w2k_preferences_gradient));
    } else {
        ok &= restore_original_user_value("ThemeActive",
            "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
        ok &= restore_original_user_value("UserPreferencesMask", "Control Panel\\Desktop",
                                          "UserPreferencesMask");
    }
    ok &= write_user_dword(CONFIG_KEY, "W2KColorProfileEnabled", enabled ? 1 : 0);
    ok &= write_user_dword(CONFIG_KEY, "DesktopCaptionPreset", (DWORD)caption_preset);
    if (!g_cross_user) {
        set_visual_style_live(enabled);
        /* SetSystemVisualStyle resets the process-wide system colors.  Apply the
           captured/Windows-2000 palette after the theme transition. */
        apply_registered_colors_to_session();
        if (!enabled)
            read_user_dword(CONFIG_KEY, "Original_GradientCaptions_Value", &original_gradient);
        ok &= SystemParametersInfoA(SPI_SETGRADIENTCAPTIONS, 0,
                (PVOID)(INT_PTR)(enabled
                    ? (caption_preset == CAPTION_PRESET_BLUE_GRADIENT ? TRUE : FALSE)
                    : (original_gradient ? TRUE : FALSE)),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
    }
    SendMessageTimeoutA(HWND_BROADCAST, WM_THEMECHANGED, 0, 0,
        SMTO_ABORTIFHUNG, 3000, NULL);
    SendMessageTimeoutA(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0,
        SMTO_ABORTIFHUNG, 3000, NULL);
    if (enabled) {
        char message[192];
        _snprintf(message, sizeof(message),
            "Exact Windows 2000 Classic palette enabled for the interactive user (%s captions).",
            caption_preset_name(caption_preset));
        append_log_line(message);
    } else {
        append_log_line("Original theme and system palette restored for the interactive user.");
    }
    if (!ok) append_log_line("ERROR: one or more theme or system-color values could not be updated.");
    return ok;
}

static void initialize_w2k_logfont_w(LOGFONTW *font, LONG weight)
{
    ZeroMemory(font, sizeof(*font));
    font->lfHeight = -11;
    font->lfWeight = weight;
    lstrcpynW(font->lfFaceName, L"Tahoma", LF_FACESIZE);
}

static void initialize_w2k_logfont_a(LOGFONTA *font, LONG weight)
{
    ZeroMemory(font, sizeof(*font));
    font->lfHeight = -11;
    font->lfWeight = weight;
    lstrcpynA(font->lfFaceName, "Tahoma", LF_FACESIZE);
}

static int capture_runtime_metrics(void)
{
    DWORD captured = 0;
    NONCLIENTMETRICSA non_client;
    ICONMETRICSA icon;
    if (read_user_dword(CONFIG_KEY, "Original_RuntimeMetrics_Captured", &captured))
        return 1;
    ZeroMemory(&non_client, sizeof(non_client));
    non_client.cbSize = sizeof(non_client);
    ZeroMemory(&icon, sizeof(icon));
    icon.cbSize = sizeof(icon);
    if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(non_client), &non_client, 0) ||
        !SystemParametersInfoA(SPI_GETICONMETRICS, sizeof(icon), &icon, 0)) return 0;
    if (!write_user_binary(CONFIG_KEY, "Original_RuntimeNonClientMetrics",
                           (const BYTE *)&non_client, sizeof(non_client)) ||
        !write_user_binary(CONFIG_KEY, "Original_RuntimeIconMetrics",
                           (const BYTE *)&icon, sizeof(icon)) ||
        !write_user_dword(CONFIG_KEY, "Original_RuntimeMetrics_Captured", 1)) return 0;
    return 1;
}

static int set_w2k_metrics_live(void)
{
    NONCLIENTMETRICSA non_client;
    ICONMETRICSA icon;
    int ok;
    ZeroMemory(&non_client, sizeof(non_client));
    non_client.cbSize = sizeof(non_client);
    ZeroMemory(&icon, sizeof(icon));
    icon.cbSize = sizeof(icon);
    if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(non_client), &non_client, 0) ||
        !SystemParametersInfoA(SPI_GETICONMETRICS, sizeof(icon), &icon, 0)) return 0;
    non_client.iBorderWidth = 1;
    non_client.iScrollWidth = 16;
    non_client.iScrollHeight = 16;
    non_client.iCaptionWidth = 18;
    non_client.iCaptionHeight = 18;
    non_client.iSmCaptionWidth = 12;
    non_client.iSmCaptionHeight = 12;
    non_client.iMenuWidth = 18;
    non_client.iMenuHeight = 18;
    initialize_w2k_logfont_a(&non_client.lfCaptionFont, FW_BOLD);
    initialize_w2k_logfont_a(&non_client.lfSmCaptionFont, FW_BOLD);
    initialize_w2k_logfont_a(&non_client.lfMenuFont, FW_NORMAL);
    initialize_w2k_logfont_a(&non_client.lfStatusFont, FW_NORMAL);
    initialize_w2k_logfont_a(&non_client.lfMessageFont, FW_NORMAL);
    initialize_w2k_logfont_a(&icon.lfFont, FW_NORMAL);
    ok = SystemParametersInfoA(SPI_SETNONCLIENTMETRICS, sizeof(non_client), &non_client,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
    ok &= SystemParametersInfoA(SPI_SETICONMETRICS, sizeof(icon), &icon,
                                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
    return ok;
}

static int restore_runtime_metrics(void)
{
    NONCLIENTMETRICSA non_client;
    ICONMETRICSA icon;
    DWORD non_client_size = sizeof(non_client), icon_size = sizeof(icon);
    int ok;
    if (!read_user_binary(CONFIG_KEY, "Original_RuntimeNonClientMetrics",
                          (BYTE *)&non_client, &non_client_size) ||
        non_client_size != sizeof(non_client) ||
        !read_user_binary(CONFIG_KEY, "Original_RuntimeIconMetrics",
                          (BYTE *)&icon, &icon_size) || icon_size != sizeof(icon)) return 0;
    ok = SystemParametersInfoA(SPI_SETNONCLIENTMETRICS, sizeof(non_client), &non_client,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
    ok &= SystemParametersInfoA(SPI_SETICONMETRICS, sizeof(icon), &icon,
                                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
    return ok;
}

static int apply_w2k_font_metrics(int enabled)
{
    int ok = 1;
    size_t index;
    LOGFONTW font;
    for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Metric_%s", g_w2k_metrics[index].name);
        capture_original_user_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                    g_w2k_metrics[index].name);
        if (enabled)
            ok &= write_user_string("Control Panel\\Desktop\\WindowMetrics",
                                    g_w2k_metrics[index].name, g_w2k_metrics[index].text);
        else
            ok &= restore_original_user_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                              g_w2k_metrics[index].name);
    }
    for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
        char marker[128];
        LONG weight = (lstrcmpiA(g_w2k_font_values[index], "CaptionFont") == 0 ||
                       lstrcmpiA(g_w2k_font_values[index], "SmCaptionFont") == 0)
                    ? FW_BOLD : FW_NORMAL;
        _snprintf(marker, sizeof(marker), "MetricFont_%s", g_w2k_font_values[index]);
        capture_original_user_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                    g_w2k_font_values[index]);
        if (enabled) {
            initialize_w2k_logfont_w(&font, weight);
            ok &= write_user_binary("Control Panel\\Desktop\\WindowMetrics",
                                    g_w2k_font_values[index], (const BYTE *)&font, sizeof(font));
        } else {
            ok &= restore_original_user_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                              g_w2k_font_values[index]);
        }
    }
    if (!g_cross_user) {
        if (enabled) ok &= capture_runtime_metrics() && set_w2k_metrics_live();
        else {
            DWORD runtime_captured = 0;
            if (read_user_dword(CONFIG_KEY, "Original_RuntimeMetrics_Captured",
                                &runtime_captured) && runtime_captured)
                ok &= restore_runtime_metrics();
        }
    }
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS,
                        (LPARAM)"WindowMetrics", SMTO_ABORTIFHUNG, 3000, NULL);
    append_log_line(enabled ? "Tahoma 8 interface fonts and 18-pixel title bars enabled."
                            : "Original interface fonts and window metrics restored.");
    if (!ok) append_log_line("ERROR: one or more font or window-metric values could not be updated.");
    return ok;
}

static int apply_user_features(void)
{
    int ok = 1;
    DWORD configured;
    DWORD saved_desktop_preset = CAPTION_PRESET_BLUE_GRADIENT;
    BOOL animation, fade;
    int desired;
    int desktop_caption_preset = selected_desktop_caption_preset();
    int desktop_preset_changed =
        !read_user_dword(CONFIG_KEY, "DesktopCaptionPreset", &saved_desktop_preset) ||
        !caption_preset_valid(saved_desktop_preset) ||
        (int)saved_desktop_preset != desktop_caption_preset;
    desired = Button_GetCheck(g_features[FEATURE_CLASSIC_THEME].checkbox) == BST_CHECKED;
    if (desired != g_features[FEATURE_CLASSIC_THEME].detected ||
        (desired && desktop_preset_changed)) {
        ok &= apply_classic_theme(desired, desktop_caption_preset);
        ok &= apply_w2k_font_metrics(desired);
    } else if (desktop_preset_changed) {
        ok &= write_user_dword(CONFIG_KEY, "DesktopCaptionPreset",
                               (DWORD)desktop_caption_preset);
    }

    desired = Button_GetCheck(g_features[FEATURE_CLASSIC_START_MENU].checkbox) == BST_CHECKED;
    {
        DWORD policy_managed = 0;
        int start_needs_update = desired != g_features[FEATURE_CLASSIC_START_MENU].detected;
        if (desired && (!read_user_dword(CONFIG_KEY, "ClassicStartPolicyManaged",
                                         &policy_managed) || !policy_managed))
            start_needs_update = 1;
        if (start_needs_update) {
            int start_ok;
            capture_original_user_value("NoSimpleStartMenu",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                "NoSimpleStartMenu");
            capture_original_user_value("ClassicStartShellState",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                "ShellState");
            if (desired) {
                start_ok = write_start_panel_on(0);
                start_ok &= write_user_dword(
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                    "NoSimpleStartMenu", 1);
                start_ok &= write_user_dword(CONFIG_KEY, "ClassicStartPolicyManaged", 1);
            } else {
                start_ok = restore_original_user_value("ClassicStartShellState",
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                    "ShellState");
                start_ok &= restore_original_user_value("NoSimpleStartMenu",
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                    "NoSimpleStartMenu");
                start_ok &= write_user_dword(CONFIG_KEY, "ClassicStartPolicyManaged", 0);
            }
            ok &= start_ok;
            append_log_line(desired ? "Classic Start menu/taskbar enabled." : "Classic Start menu/taskbar restored.");
            if (!start_ok) append_log_line("ERROR: Windows did not accept the requested native Start-menu mode.");
        }
    }

    desired = Button_GetCheck(g_features[FEATURE_CLASSIC_CONTROL_PANEL].checkbox) == BST_CHECKED;
    if (desired != g_features[FEATURE_CLASSIC_CONTROL_PANEL].detected) {
        int control_panel_ok;
        capture_original_user_value("ClassicControlPanel", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel", "StartupPage");
        capture_original_user_value("ForceClassicControlPanel", "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "ForceClassicControlPanel");
        if (desired) {
            control_panel_ok = write_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel", "StartupPage", 0);
            control_panel_ok &= write_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "ForceClassicControlPanel", 1);
        } else {
            control_panel_ok = restore_original_user_value("ClassicControlPanel", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel", "StartupPage");
            control_panel_ok &= restore_original_user_value("ForceClassicControlPanel", "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "ForceClassicControlPanel");
        }
        ok &= control_panel_ok;
        append_log_line(desired ? "Classic Control Panel view enabled." : "Control Panel view restored.");
        if (!control_panel_ok) append_log_line("ERROR: the Classic Control Panel preference could not be written.");
    }

    desired = Button_GetCheck(g_features[FEATURE_MENU_SLIDE].checkbox) == BST_CHECKED;
    {
        int desired_fade = Button_GetCheck(
            g_features[FEATURE_MENU_FADE].checkbox) == BST_CHECKED;
        if (!read_user_dword(CONFIG_KEY, "Original_MenuAnimation_Present", &configured)) {
            SystemParametersInfoA(SPI_GETMENUANIMATION, 0, &animation, 0);
            SystemParametersInfoA(SPI_GETMENUFADE, 0, &fade, 0);
            write_user_dword(CONFIG_KEY, "Original_MenuAnimation_Present", 1);
            write_user_dword(CONFIG_KEY, "Original_MenuAnimation_Value", animation ? 1 : 0);
            write_user_dword(CONFIG_KEY, "Original_MenuFade_Value", fade ? 1 : 0);
        }
        if (!desired && !desired_fade) {
            DWORD stored_animation = 0, stored_fade = 1;
            read_user_dword(CONFIG_KEY, "Original_MenuAnimation_Value", &stored_animation);
            read_user_dword(CONFIG_KEY, "Original_MenuFade_Value", &stored_fade);
            animation = stored_animation != 0;
            fade = stored_fade != 0;
        } else if (desired) {
            animation = TRUE;
            fade = FALSE;
        } else {
            animation = TRUE;
            fade = TRUE;
        }
        {
            int animation_ok = SystemParametersInfoA(SPI_SETMENUANIMATION, 0,
                (PVOID)(INT_PTR)(animation ? TRUE : FALSE),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
            int fade_ok = SystemParametersInfoA(SPI_SETMENUFADE, 0,
                (PVOID)(INT_PTR)(fade ? TRUE : FALSE),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0;
            ok &= animation_ok && fade_ok;
            if (!animation_ok) append_log_line("ERROR: Windows rejected the menu-animation setting.");
            if (!fade_ok) append_log_line("ERROR: Windows rejected the menu-fade setting.");
        }
        append_log_line(desired
            ? "Optional sliding Start menu animation enabled and menu fade disabled."
            : (desired_fade
                ? "Normal Windows XP fading Start menu animation enabled."
                : "Original menu animation settings restored."));
    }

    desired = Button_GetCheck(g_features[FEATURE_WALLPAPERS].checkbox) == BST_CHECKED;
    if (desired || desired != g_features[FEATURE_WALLPAPERS].detected)
        ok &= apply_wallpaper_installation(desired);

    desired = Button_GetCheck(g_features[FEATURE_WINDOWS_2000_SOUNDS].checkbox) == BST_CHECKED;
    if (desired || desired != g_features[FEATURE_WINDOWS_2000_SOUNDS].detected)
        ok &= apply_windows_2000_sounds(desired);

    desired = Button_GetCheck(
        g_features[FEATURE_WINDOWS_2000_DOUBLE_CLICK_SOUND].checkbox) == BST_CHECKED;
    if (desired ||
        desired != g_features[FEATURE_WINDOWS_2000_DOUBLE_CLICK_SOUND].detected)
        ok &= apply_windows_2000_double_click_sound(desired);

    if (!write_user_dword(CONFIG_KEY, "Configured", 1)) {
        append_log_line("ERROR: the configured-state marker could not be written.");
        ok = 0;
    }
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
        SMTO_ABORTIFHUNG, 3000, NULL);
    return ok;
}

static int apply_default_w2k_appearance(int enabled, int caption_preset)
{
    DWORD captured = 0;
    int ok = 1;
    size_t index;
    LOGFONTW font;
    char logon_background[MAX_PATH];
    static const BYTE w2k_preferences_gradient[] = {0x9e, 0x3e, 0x00, 0x80};
    static const BYTE w2k_preferences_solid[] = {0x8e, 0x3e, 0x00, 0x80};
    const BYTE *w2k_preferences = caption_preset == CAPTION_PRESET_SOLID_NAVY
        ? w2k_preferences_solid : w2k_preferences_gradient;
    read_user_dword(CONFIG_KEY, "DefaultAppearanceCaptured", &captured);
    if (!enabled && !captured) return 1;

    if (enabled && !captured) {
        capture_original_default_value("ThemeActive",
            "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
        capture_original_default_value("FontSmoothing", "Control Panel\\Desktop", "FontSmoothing");
        capture_original_default_value("UserPreferencesMask", "Control Panel\\Desktop",
                                       "UserPreferencesMask");
        capture_original_default_value("Wallpaper", "Control Panel\\Desktop", "Wallpaper");
        capture_original_default_value("TileWallpaper", "Control Panel\\Desktop",
                                       "TileWallpaper");
        capture_original_default_value("WallpaperStyle", "Control Panel\\Desktop",
                                       "WallpaperStyle");
        capture_original_default_value("Pattern", "Control Panel\\Desktop", "Pattern");
        for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "Color_%s", g_w2k_colors[index].name);
            capture_original_default_value(marker, "Control Panel\\Colors",
                                           g_w2k_colors[index].name);
        }
        for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "Metric_%s", g_w2k_metrics[index].name);
            capture_original_default_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                           g_w2k_metrics[index].name);
        }
        for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "MetricFont_%s", g_w2k_font_values[index]);
            capture_original_default_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                           g_w2k_font_values[index]);
        }
        ok &= write_user_dword(CONFIG_KEY, "DefaultAppearanceCaptured", 1);
    }

    if (enabled) {
        static const char disabled[] = "0";
        ok &= write_default_user_value(
            "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive",
            REG_SZ, (const BYTE *)disabled, sizeof(disabled));
        ok &= write_default_user_value("Control Panel\\Desktop", "FontSmoothing",
                                       REG_SZ, (const BYTE *)disabled, sizeof(disabled));
        ok &= write_default_user_value("Control Panel\\Desktop", "UserPreferencesMask",
                                       REG_BINARY, w2k_preferences,
                                       sizeof(w2k_preferences_gradient));
        for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
            const char *logon_color = caption_color_value(g_w2k_colors[index].name,
                                                          g_w2k_colors[index].text,
                                                          caption_preset);
            ok &= write_default_user_value("Control Panel\\Colors", g_w2k_colors[index].name,
                REG_SZ, (const BYTE *)logon_color,
                (DWORD)strlen(logon_color) + 1);
        }
        /* XP's classic GINA paints an empty secure desktop black even when
           .DEFAULT's COLOR_BACKGROUND is the Windows 2000 value.  Tile the
           bundled one-color 58,110,165 bitmap to reproduce the Windows 2000
           secure desktop exactly while keeping the change reversible. */
        if (join_path(logon_background, sizeof(logon_background), g_install_root,
                      "Assets\\logon-background.bmp") && file_exists(logon_background)) {
            ok &= write_default_user_value("Control Panel\\Desktop", "Wallpaper", REG_SZ,
                (const BYTE *)logon_background, (DWORD)strlen(logon_background) + 1);
        } else {
            append_log_line("ERROR: the Windows 2000 login-background asset is missing.");
            ok = 0;
        }
        ok &= write_default_user_value("Control Panel\\Desktop", "TileWallpaper", REG_SZ,
            (const BYTE *)"1", sizeof("1"));
        ok &= write_default_user_value("Control Panel\\Desktop", "WallpaperStyle", REG_SZ,
            (const BYTE *)"0", sizeof("0"));
        ok &= write_default_user_value("Control Panel\\Desktop", "Pattern", REG_SZ,
            (const BYTE *)"(None)", sizeof("(None)"));
        for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index)
            ok &= write_default_user_value("Control Panel\\Desktop\\WindowMetrics",
                g_w2k_metrics[index].name, REG_SZ, (const BYTE *)g_w2k_metrics[index].text,
                (DWORD)strlen(g_w2k_metrics[index].text) + 1);
        for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
            LONG weight = (lstrcmpiA(g_w2k_font_values[index], "CaptionFont") == 0 ||
                           lstrcmpiA(g_w2k_font_values[index], "SmCaptionFont") == 0)
                        ? FW_BOLD : FW_NORMAL;
            initialize_w2k_logfont_w(&font, weight);
            ok &= write_default_user_value("Control Panel\\Desktop\\WindowMetrics",
                g_w2k_font_values[index], REG_BINARY, (const BYTE *)&font, sizeof(font));
        }
    } else {
        ok &= restore_original_default_value("ThemeActive",
            "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
        ok &= restore_original_default_value("FontSmoothing", "Control Panel\\Desktop",
                                             "FontSmoothing");
        ok &= restore_original_default_value("UserPreferencesMask", "Control Panel\\Desktop",
                                             "UserPreferencesMask");
        ok &= restore_original_default_value("Wallpaper", "Control Panel\\Desktop", "Wallpaper");
        ok &= restore_original_default_value("TileWallpaper", "Control Panel\\Desktop",
                                             "TileWallpaper");
        ok &= restore_original_default_value("WallpaperStyle", "Control Panel\\Desktop",
                                             "WallpaperStyle");
        ok &= restore_original_default_value("Pattern", "Control Panel\\Desktop", "Pattern");
        for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "Color_%s", g_w2k_colors[index].name);
            ok &= restore_original_default_value(marker, "Control Panel\\Colors",
                                                 g_w2k_colors[index].name);
        }
        for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "Metric_%s", g_w2k_metrics[index].name);
            ok &= restore_original_default_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                                 g_w2k_metrics[index].name);
        }
        for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
            char marker[128];
            _snprintf(marker, sizeof(marker), "MetricFont_%s", g_w2k_font_values[index]);
            ok &= restore_original_default_value(marker, "Control Panel\\Desktop\\WindowMetrics",
                                                 g_w2k_font_values[index]);
        }
    }
    ok &= write_user_dword(CONFIG_KEY, "DefaultW2KAppearanceEnabled", enabled ? 1 : 0);
    ok &= write_user_dword(CONFIG_KEY, "LogonCaptionPreset", (DWORD)caption_preset);
    if (enabled) {
        char message[192];
        _snprintf(message, sizeof(message),
            "Exact Windows 2000 colors and metrics applied to the login desktop (%s captions).",
            caption_preset_name(caption_preset));
        append_log_line(message);
    } else {
        append_log_line("Original login-desktop colors and metrics restored.");
    }
    if (!ok) append_log_line("ERROR: one or more login-desktop appearance values could not be updated.");
    return ok;
}

static int explorer_web_paths(char *payload_root, size_t payload_size,
                              char *target_root, size_t target_size,
                              char *backup_root, size_t backup_size,
                              char *state_path, size_t state_size)
{
    char windows[MAX_PATH], backup_parent[MAX_PATH];
    return GetWindowsDirectoryA(windows, sizeof(windows)) &&
        join_path(payload_root, payload_size, g_install_root, "ExplorerWeb") &&
        join_path(target_root, target_size, windows, "Web") &&
        join_path(backup_parent, sizeof(backup_parent), g_install_root,
                  "ExplorerExperimentBackup") &&
        join_path(backup_root, backup_size, backup_parent, "Web") &&
        join_path(state_path, state_size, backup_parent, "web-state.tsv");
}

static int ensure_parent_directory_for_file(const char *path)
{
    char parent[MAX_PATH];
    int result;
    lstrcpynA(parent, path, sizeof(parent));
    parent_directory(parent);
    result = SHCreateDirectoryExA(NULL, parent, NULL);
    return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS ||
           result == ERROR_FILE_EXISTS;
}

static int deploy_explorer_web_tree(const char *payload_root,
                                    const char *target_root,
                                    const char *backup_root,
                                    const char *relative,
                                    FILE *state,
                                    int capture_original)
{
    char source_directory[MAX_PATH], pattern[MAX_PATH];
    WIN32_FIND_DATAA find_data;
    HANDLE find;
    int ok = 1;
    if (*relative) {
        if (!join_path(source_directory, sizeof(source_directory), payload_root,
                       relative)) return 0;
    } else {
        lstrcpynA(source_directory, payload_root, sizeof(source_directory));
    }
    if (!join_path(pattern, sizeof(pattern), source_directory, "*")) return 0;
    find = FindFirstFileA(pattern, &find_data);
    if (find == INVALID_HANDLE_VALUE) return 0;
    do {
        char child_relative[MAX_PATH], source[MAX_PATH], target[MAX_PATH];
        char backup[MAX_PATH];
        if (lstrcmpA(find_data.cFileName, ".") == 0 ||
            lstrcmpA(find_data.cFileName, "..") == 0) continue;
        if (*relative) {
            if (!join_path(child_relative, sizeof(child_relative), relative,
                           find_data.cFileName)) { ok = 0; break; }
        } else {
            lstrcpynA(child_relative, find_data.cFileName,
                      sizeof(child_relative));
        }
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!deploy_explorer_web_tree(payload_root, target_root, backup_root,
                                          child_relative, state,
                                          capture_original)) {
                ok = 0;
                break;
            }
            continue;
        }
        if (!join_path(source, sizeof(source), payload_root, child_relative) ||
            !join_path(target, sizeof(target), target_root, child_relative) ||
            !join_path(backup, sizeof(backup), backup_root, child_relative) ||
            !ensure_parent_directory_for_file(target)) {
            ok = 0;
            break;
        }
        if (capture_original) {
            DWORD attributes = GetFileAttributesA(target);
            if (attributes != INVALID_FILE_ATTRIBUTES) {
                if (!ensure_parent_directory_for_file(backup) ||
                    !CopyFileA(target, backup, FALSE) ||
                    fprintf(state, "P\t%lu\t%s\r\n",
                            (unsigned long)attributes, child_relative) < 0) {
                    ok = 0;
                    break;
                }
            } else if (fprintf(state, "A\t0\t%s\r\n", child_relative) < 0) {
                ok = 0;
                break;
            }
        }
        if (file_exists(target)) SetFileAttributesA(target, FILE_ATTRIBUTE_NORMAL);
        if (!CopyFileA(source, target, FALSE) ||
            !files_are_identical(source, target)) {
            ok = 0;
            break;
        }
    } while (FindNextFileA(find, &find_data));
    FindClose(find);
    return ok;
}

static int restore_explorer_web_files(void)
{
    char payload_root[MAX_PATH], target_root[MAX_PATH], backup_root[MAX_PATH];
    char state_path[MAX_PATH], line[2 * MAX_PATH + 64];
    FILE *state;
    int ok = 1;
    if (!explorer_web_paths(payload_root, sizeof(payload_root),
                            target_root, sizeof(target_root),
                            backup_root, sizeof(backup_root),
                            state_path, sizeof(state_path))) return 0;
    state = fopen(state_path, "rb");
    if (!state) return 1;
    while (fgets(line, sizeof(line), state)) {
        char status = 0, relative[MAX_PATH];
        unsigned long attributes = 0;
        char source[MAX_PATH], target[MAX_PATH], backup[MAX_PATH];
        if (sscanf(line, "%c\t%lu\t%259[^\r\n]", &status, &attributes,
                   relative) != 3 ||
            !join_path(source, sizeof(source), payload_root, relative) ||
            !join_path(target, sizeof(target), target_root, relative) ||
            !join_path(backup, sizeof(backup), backup_root, relative)) {
            ok = 0;
            continue;
        }
        if (status == 'P') {
            if (!file_exists(backup) || !ensure_parent_directory_for_file(target)) {
                ok = 0;
                continue;
            }
            if (file_exists(target)) SetFileAttributesA(target, FILE_ATTRIBUTE_NORMAL);
            if (!CopyFileA(backup, target, FALSE)) {
                ok = 0;
                continue;
            }
            SetFileAttributesA(target, (DWORD)attributes);
        } else if (status == 'A' && file_exists(target) &&
                   files_are_identical(source, target)) {
            SetFileAttributesA(target, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileA(target)) ok = 0;
        }
    }
    fclose(state);
    if (ok) DeleteFileA(state_path);
    return ok;
}

static int install_explorer_web_files(void)
{
    char payload_root[MAX_PATH], target_root[MAX_PATH], backup_root[MAX_PATH];
    char state_path[MAX_PATH];
    DWORD captured = 0;
    FILE *state = NULL;
    int ok;
    if (!explorer_web_paths(payload_root, sizeof(payload_root),
                            target_root, sizeof(target_root),
                            backup_root, sizeof(backup_root),
                            state_path, sizeof(state_path)) ||
        GetFileAttributesA(payload_root) == INVALID_FILE_ATTRIBUTES ||
        !(GetFileAttributesA(payload_root) & FILE_ATTRIBUTE_DIRECTORY)) return 0;
    read_machine_dword(EXPLORER_MACHINE_STATE_KEY, "WebBackupCaptured", &captured);
    if (!captured && file_exists(state_path) && !restore_explorer_web_files())
        return 0;
    if (!captured) {
        if (SHCreateDirectoryExA(NULL, backup_root, NULL) != ERROR_SUCCESS &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            DWORD attributes = GetFileAttributesA(backup_root);
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return 0;
        }
        state = fopen(state_path, "wb");
        if (!state) return 0;
    }
    ok = deploy_explorer_web_tree(payload_root, target_root, backup_root, "",
                                  state, !captured);
    if (state) fclose(state);
    if (ok && !captured)
        ok = write_machine_dword(EXPLORER_MACHINE_STATE_KEY,
                                 "WebBackupCaptured", 1);
    return ok;
}

typedef struct {
    const char *marker;
    const char *subkey;
    const char *name;
    DWORD type;
    const char *value;
} EXPLORER_MACHINE_TEXT;

static const EXPLORER_MACHINE_TEXT g_explorer_machine_text[] = {
    {"ExplorerBandClsidName", "SOFTWARE\\Classes\\CLSID\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}", "", REG_SZ, "eXPerience2K Windows 2000 Explorer Pane"},
    {"ExplorerBandInproc", "SOFTWARE\\Classes\\CLSID\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}\\InprocServer32", "", REG_SZ, "@EXPLORERBAND@"},
    {"ExplorerBandThreading", "SOFTWARE\\Classes\\CLSID\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}\\InprocServer32", "ThreadingModel", REG_SZ, "Apartment"},
    {"ExplorerHookClsidName", "SOFTWARE\\Classes\\CLSID\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}", "", REG_SZ, "eXPerience2K Explorer Hook"},
    {"ExplorerHookInproc", "SOFTWARE\\Classes\\CLSID\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}\\InprocServer32", "", REG_SZ, "@EXPLORERBAND@"},
    {"ExplorerHookThreading", "SOFTWARE\\Classes\\CLSID\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}\\InprocServer32", "ThreadingModel", REG_SZ, "Apartment"},
    {"ExplorerHookBho", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}", "", REG_SZ, "eXPerience2K Explorer Hook"},
    {"ExplorerHookPreApproved", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}", "", REG_SZ, "eXPerience2K Explorer Hook"},

    {"WebMacroBackground", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\TemplateMacros\\BACKGROUNDIMAGE", "", REG_EXPAND_SZ, "%SystemRoot%\\Web\\wvleft.bmp"},
    {"WebMacroLogoLine", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\TemplateMacros\\LOGOLINE", "", REG_EXPAND_SZ, "%SystemRoot%\\Web\\wvline.gif"},

    {"WebT0Display", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "DisplayName", REG_SZ, "Standard"},
    {"WebT0Custom", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "Customizable", REG_SZ, "O"},
    {"WebT0Template", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "TemplateFile", REG_SZ, "@WEB@\\standard.htt"},
    {"WebT0Preview", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "PreviewBitmapFile", REG_SZ, "@WEB@\\folder.bmp"},
    {"WebT0Description", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "Description", REG_SZ, "This is the default, full-featured template for most folders in Windows 2000. You can modify it to make this folder special, if you wish."},
    {"WebT0Version", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0", "Version", REG_SZ, "IE4"},
    {"WebT0SupportLine", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "wvline.gif", REG_SZ, "@WEB@\\wvline.gif"},
    {"WebT0SupportLeft", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "wvleft.bmp", REG_SZ, "@WEB@\\wvleft.bmp"},
    {"WebT0SupportPlusHot", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "plushot.gif", REG_SZ, "@WEB@\\plushot.gif"},
    {"WebT0SupportPlusCold", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "pluscold.gif", REG_SZ, "@WEB@\\pluscold.gif"},
    {"WebT0SupportMinHot", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "minhot.gif", REG_SZ, "@WEB@\\minhot.gif"},
    {"WebT0SupportMinCold", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\0\\Supporting Files", "mincold.gif", REG_SZ, "@WEB@\\mincold.gif"},

    {"WebT1Display", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "DisplayName", REG_SZ, "Classic (icons only)"},
    {"WebT1Custom", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "Customizable", REG_SZ, "O"},
    {"WebT1Template", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "TemplateFile", REG_SZ, "@WEB@\\classic.htt"},
    {"WebT1Preview", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "PreviewBitmapFile", REG_SZ, "@WEB@\\classic.bmp"},
    {"WebT1Description", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "Description", REG_SZ, "This template is empty except for the folder icons. You can use it as a starting point, or to simulate a classic folder without turning Web View off."},
    {"WebT1Version", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\1", "Version", REG_SZ, "IE4"},

    {"WebT2Display", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "DisplayName", REG_SZ, "Simple"},
    {"WebT2Custom", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "Customizable", REG_SZ, "O"},
    {"WebT2Template", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "TemplateFile", REG_SZ, "@WEB@\\starter.htt"},
    {"WebT2Preview", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "PreviewBitmapFile", REG_SZ, "@WEB@\\starter.bmp"},
    {"WebT2Description", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "Description", REG_SZ, "This is a good starting point for creating your own Web View. It respects system colors, uses the folder icons control, but contains no script."},
    {"WebT2Version", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\2", "Version", REG_SZ, "IE4"},

    {"WebT3Display", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "DisplayName", REG_SZ, "Image Preview"},
    {"WebT3Custom", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "Customizable", REG_SZ, "O"},
    {"WebT3Template", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "TemplateFile", REG_SZ, "@WEB@\\ImgView.htt"},
    {"WebT3Preview", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "PreviewBitmapFile", REG_SZ, "@WEB@\\Preview.bmp"},
    {"WebT3Description", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "Description", REG_SZ, "This is good for viewing a folder that primarily contains image files. It shows both a preview and image-specific properties of the selected file."},
    {"WebT3Version", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates\\3", "Version", REG_SZ, "NT5"},

    {"ViewDirectory", "SOFTWARE\\Classes\\Directory\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\folder.htt"},
    {"ViewFolder", "SOFTWARE\\Classes\\Folder\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\default.htt"},
    {"ViewNetwork", "SOFTWARE\\Classes\\Network\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\nethood.htt"},
    {"ViewBriefcase", "SOFTWARE\\Classes\\CLSID\\{85BBD920-42A0-1069-A2E4-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\default.htt"},
    {"ViewControlPanel", "SOFTWARE\\Classes\\CLSID\\{21EC2020-3AEA-1069-A2DD-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\controlp.htt"},
    {"ViewMyComputer", "SOFTWARE\\Classes\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\folder.htt"},
    {"ViewNetworkPlaces", "SOFTWARE\\Classes\\CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\nethood.htt"},
    {"ViewPrinters", "SOFTWARE\\Classes\\CLSID\\{2227A280-3AEA-1069-A2DE-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\printers.htt"},
    {"ViewPrintersFile", "SOFTWARE\\Classes\\CLSID\\{2227A280-3AEA-1069-A2DE-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistFile", REG_EXPAND_SZ, "%SystemRoot%\\web\\printfld.htm"},
    {"ViewRecycleBin", "SOFTWARE\\Classes\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\recycle.htt"},
    {"ViewScheduledTasks", "SOFTWARE\\Classes\\CLSID\\{D6277990-4C6A-11CF-8D87-00AA0060F5BF}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\schedule.htt"},
    {"ViewDialup", "SOFTWARE\\Classes\\CLSID\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\dialup.htt"},
    {"ViewSearchOne", "SOFTWARE\\Classes\\CLSID\\{e17d4fc0-5564-11d1-83f2-00a0c90dc849}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\fsresult.htt"},
    {"ViewSearchTwo", "SOFTWARE\\Classes\\CLSID\\{1f4de370-d627-11d1-ba4f-00a0c91eedba}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", "PersistMoniker", REG_SZ, "file://%webdir%\\fsresult.htt"}
};

static int apply_explorer_machine_registry(int enabled)
{
    size_t index;
    int ok = 1;
    char windows[MAX_PATH], web_root[MAX_PATH], expanded[MAX_PATH];
    if (!GetWindowsDirectoryA(windows, sizeof(windows)) ||
        !join_path(web_root, sizeof(web_root), windows, "Web")) return 0;
    for (index = 0; index < sizeof(g_explorer_machine_text) /
                              sizeof(g_explorer_machine_text[0]); ++index) {
        const EXPLORER_MACHINE_TEXT *item = &g_explorer_machine_text[index];
        const char *value = item->value;
        capture_original_machine_value(item->marker, item->subkey, item->name);
        if (enabled) {
            if (_strnicmp(item->value, "@WEB@\\", 6) == 0) {
                if (!join_path(expanded, sizeof(expanded), web_root,
                               item->value + 6)) {
                    ok = 0;
                    continue;
                }
                value = expanded;
            } else if (_strnicmp(item->value, "@INSTALL@\\", 10) == 0) {
                if (!join_path(expanded, sizeof(expanded), g_install_root,
                               item->value + 10)) {
                    ok = 0;
                    continue;
                }
                value = expanded;
            } else if (lstrcmpiA(item->value, "@EXPLORERBAND@") == 0) {
                const char *band_name = explorer_band_filename();
                if (!band_name[0] ||
                    !join_path(expanded, sizeof(expanded), g_install_root,
                               band_name)) {
                    ok = 0;
                    continue;
                }
                value = expanded;
            }
            ok &= write_machine_value(item->subkey, item->name, item->type,
                (const BYTE *)value, (DWORD)strlen(value) + 1);
        } else {
            ok &= restore_original_machine_value(item->marker, item->subkey,
                                                 item->name);
        }
    }
    return ok;
}

static int apply_w2k_explorer_machine_state(int enabled)
{
    int ok;
    if (enabled && !explorer_experiment_architecture_supported()) {
        append_log_line("ERROR: the experimental Explorer pane requires a supported Windows XP Professional x86 SP3 or x64 SP2 installation.");
        return 0;
    }
    if (enabled) {
        ok = install_explorer_web_files();
        if (ok) ok &= apply_explorer_machine_registry(1);
        if (ok) ok &= write_machine_dword(EXPLORER_MACHINE_STATE_KEY,
                                          "Enabled", 1);
    } else {
        ok = apply_explorer_machine_registry(0);
        /* These CLSIDs are unique to this experiment.  Removing the complete
           owned trees is required: an empty Browser Helper Objects key would
           still make Explorer attempt to instantiate the hook. */
        ok &= delete_machine_tree("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}");
        ok &= delete_machine_tree("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}");
        ok &= delete_machine_tree("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}");
        ok &= delete_machine_tree("SOFTWARE\\Microsoft\\Internet Explorer\\Explorer Bars\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}");
        ok &= delete_machine_tree("SOFTWARE\\Classes\\CLSID\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}");
        ok &= delete_machine_tree("SOFTWARE\\Classes\\CLSID\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}");
        {
            char command[2 * MAX_PATH + 80];
            _snprintf(command, sizeof(command),
                      "\"%s\" cleanup-explorer-registrations", g_core_path);
            ok &= run_captured(command);
        }
        ok &= restore_explorer_web_files();
        ok &= write_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", 0);
        if (ok) write_machine_dword(EXPLORER_MACHINE_STATE_KEY,
                                    "WebBackupCaptured", 0);
    }
    append_log_line(enabled
        ? (ok ? "Exact Windows 2000 Web View files and shell-folder registrations installed."
              : "ERROR: the Windows 2000 Explorer Web View deployment was incomplete.")
        : (ok ? "Original Explorer Web files and shell-folder registrations restored."
              : "ERROR: the Explorer Web View restoration was incomplete."));
    return ok;
}

static int migrate_legacy_dword_baseline(const char *marker)
{
    char captured_name[160], present_name[160], value_name[160];
    char type_name[160], data_name[160];
    DWORD captured = 0, present = 0, value = 0;
    _snprintf(captured_name, sizeof(captured_name), "Original_%s_Captured", marker);
    if (read_user_dword(CONFIG_KEY, captured_name, &captured) && captured) return 1;
    _snprintf(present_name, sizeof(present_name), "Original_%s_Present", marker);
    if (!read_user_dword(CONFIG_KEY, present_name, &present)) return 0;
    _snprintf(value_name, sizeof(value_name), "Original_%s_Value", marker);
    _snprintf(type_name, sizeof(type_name), "Original_%s_Type", marker);
    _snprintf(data_name, sizeof(data_name), "Original_%s_Data", marker);
    if (present) {
        if (!read_user_dword(CONFIG_KEY, value_name, &value) ||
            !write_user_dword(CONFIG_KEY, type_name, REG_DWORD) ||
            !write_user_binary(CONFIG_KEY, data_name,
                               (const BYTE *)&value, sizeof(value))) return 0;
    }
    return write_user_dword(CONFIG_KEY, captured_name, 1);
}

static int migrate_legacy_start_menu_baseline(void)
{
    char captured_name[] = "Original_ClassicStartShellState_Captured";
    char data_name[] = "Original_ClassicStartShellState_Data";
    DWORD captured = 0, legacy_present = 0, original_panel_on = 1;
    BYTE shell_state[256];
    DWORD size = sizeof(shell_state);
    if (read_user_dword(CONFIG_KEY, captured_name, &captured) && captured) return 1;
    if (!read_user_dword(CONFIG_KEY, "Original_StartPanelOn_Present",
                         &legacy_present) || !legacy_present) return 0;
    if (!capture_original_user_value_checked("ClassicStartShellState",
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
            "ShellState")) return 0;
    if (!read_user_dword(CONFIG_KEY, "Original_StartPanelOn_Value",
                         &original_panel_on) ||
        !read_user_binary(CONFIG_KEY, data_name, shell_state, &size) || size < 36)
        return 0;
    if (original_panel_on) shell_state[32] |= 0x02;
    else shell_state[32] &= (BYTE)~0x02;
    return write_user_binary(CONFIG_KEY, data_name, shell_state, size);
}

static int capture_exact_user_baseline(void)
{
    DWORD schema = 0, captured = 0;
    BOOL animation = FALSE, fade = TRUE, gradient = TRUE;
    size_t index, branch;
    int ok = 1;

    if (read_user_dword(CONFIG_KEY, "BaselineUserSchema", &schema) && schema == 1)
        return 1;
    append_log_line("Capturing the immutable pre-eXPerience2K user baseline...");

    ok &= capture_original_user_value_checked("ThemeActive",
        "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
    ok &= capture_original_user_value_checked("UserPreferencesMask",
        "Control Panel\\Desktop", "UserPreferencesMask");
    for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Color_%s", g_w2k_colors[index].name);
        ok &= capture_original_user_value_checked(marker, "Control Panel\\Colors",
                                                  g_w2k_colors[index].name);
    }
    for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Metric_%s", g_w2k_metrics[index].name);
        ok &= capture_original_user_value_checked(marker,
            "Control Panel\\Desktop\\WindowMetrics", g_w2k_metrics[index].name);
    }
    for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "MetricFont_%s", g_w2k_font_values[index]);
        ok &= capture_original_user_value_checked(marker,
            "Control Panel\\Desktop\\WindowMetrics", g_w2k_font_values[index]);
    }
    if (!g_cross_user) ok &= capture_runtime_metrics();
    if (!read_user_dword(CONFIG_KEY, "Original_GradientCaptions_Captured", &captured)) {
        if (!g_cross_user) {
            if (!SystemParametersInfoA(SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0)) ok = 0;
        } else {
            BYTE preferences[16];
            DWORD preference_size = sizeof(preferences);
            if (!read_user_binary("Control Panel\\Desktop", "UserPreferencesMask",
                                  preferences, &preference_size) || !preference_size)
                ok = 0;
            else
                gradient = (preferences[0] & 0x10) != 0;
        }
        if (ok) {
            ok &= write_user_dword(CONFIG_KEY, "Original_GradientCaptions_Value",
                                   gradient ? 1 : 0);
            ok &= write_user_dword(CONFIG_KEY, "Original_GradientCaptions_Captured", 1);
        }
    }

    if (!migrate_legacy_start_menu_baseline())
        ok &= capture_original_user_value_checked("ClassicStartShellState",
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "ShellState");
    ok &= capture_original_user_value_checked("NoSimpleStartMenu",
        "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
        "NoSimpleStartMenu");

    if (!migrate_legacy_dword_baseline("ClassicControlPanel"))
        ok &= capture_original_user_value_checked("ClassicControlPanel",
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel",
            "StartupPage");
    if (!migrate_legacy_dword_baseline("ForceClassicControlPanel"))
        ok &= capture_original_user_value_checked("ForceClassicControlPanel",
            "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
            "ForceClassicControlPanel");

    if (!read_user_dword(CONFIG_KEY, "Original_MenuAnimation_Present", &captured)) {
        if (!SystemParametersInfoA(SPI_GETMENUANIMATION, 0, &animation, 0) ||
            !SystemParametersInfoA(SPI_GETMENUFADE, 0, &fade, 0)) {
            ok = 0;
        } else {
            ok &= write_user_dword(CONFIG_KEY, "Original_MenuAnimation_Value",
                                   animation ? 1 : 0);
            ok &= write_user_dword(CONFIG_KEY, "Original_MenuFade_Value",
                                   fade ? 1 : 0);
            ok &= write_user_dword(CONFIG_KEY, "Original_MenuAnimation_Present", 1);
        }
    }

    for (index = 0;
         index < sizeof(g_windows_sound_events) / sizeof(g_windows_sound_events[0]);
         ++index) {
        const WINDOWS_SOUND_EVENT *event = &g_windows_sound_events[index];
        const char *branches[] = {"Current", "Default"};
        for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
            char key[512], marker[160];
            _snprintf(marker, sizeof(marker), "%s_%s", event->marker, branches[branch]);
            if (!build_sound_event_key(event, branches[branch], key, sizeof(key))) ok = 0;
            else ok &= capture_original_user_value_checked(marker, key, "");
        }
    }
    {
        const WINDOWS_SOUND_EVENT *event = &g_windows_2000_double_click_sound;
        const char *branches[] = {"Current", "Default"};
        for (branch = 0; branch < sizeof(branches) / sizeof(branches[0]); ++branch) {
            char key[512], marker[160];
            _snprintf(marker, sizeof(marker), "%s_%s", event->marker, branches[branch]);
            if (!build_sound_event_key(event, branches[branch], key, sizeof(key))) ok = 0;
            else ok &= capture_original_user_value_checked(marker, key, "");
        }
    }

    ok &= capture_user_tree("BagMRU", "Software\\Microsoft\\Windows\\ShellNoRoam\\BagMRU");
    ok &= capture_user_tree("Bags", "Software\\Microsoft\\Windows\\ShellNoRoam\\Bags");
    ok &= capture_user_tree("StreamMRU", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StreamMRU");
    for (index = 0; index < sizeof(g_explorer_dwords) / sizeof(g_explorer_dwords[0]); ++index)
        ok &= capture_original_user_value_checked(g_explorer_dwords[index].marker,
            g_explorer_dwords[index].subkey, g_explorer_dwords[index].name);
    for (index = 0; index < sizeof(g_explorer_strings) / sizeof(g_explorer_strings[0]); ++index)
        ok &= capture_original_user_value_checked(g_explorer_strings[index].marker,
            g_explorer_strings[index].subkey, g_explorer_strings[index].name);
    for (index = 0; index < sizeof(g_explorer_binaries) / sizeof(g_explorer_binaries[0]); ++index)
        ok &= capture_original_user_value_checked(g_explorer_binaries[index].marker,
            g_explorer_binaries[index].subkey, g_explorer_binaries[index].name);

    if (ok) ok &= write_user_dword(CONFIG_KEY, "BaselineUserSchema", 1);
    append_log_line(ok
        ? "Immutable user baseline captured successfully."
        : "ERROR: the immutable user baseline could not be captured completely; no requested changes will be applied.");
    return ok;
}

static int capture_exact_machine_baseline(void)
{
    DWORD schema = 0, present = 0, value = 0;
    size_t index;
    int ok = 1;
    if (read_user_dword(CONFIG_KEY, "BaselineMachineSchema", &schema) && schema == 1)
        return 1;
    append_log_line("Capturing the immutable pre-eXPerience2K machine baseline...");

    if (!read_user_dword(CONFIG_KEY, "Original_LogonType_Present", &present)) {
        if (read_machine_dword("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                               "LogonType", &value)) {
            ok &= write_user_dword(CONFIG_KEY, "Original_LogonType_Value", value);
            ok &= write_user_dword(CONFIG_KEY, "Original_LogonType_Present", 1);
        } else {
            ok &= write_user_dword(CONFIG_KEY, "Original_LogonType_Present", 0);
        }
    }

    ok &= capture_original_default_value_checked("ThemeActive",
        "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", "ThemeActive");
    ok &= capture_original_default_value_checked("FontSmoothing",
        "Control Panel\\Desktop", "FontSmoothing");
    ok &= capture_original_default_value_checked("UserPreferencesMask",
        "Control Panel\\Desktop", "UserPreferencesMask");
    ok &= capture_original_default_value_checked("Wallpaper",
        "Control Panel\\Desktop", "Wallpaper");
    ok &= capture_original_default_value_checked("TileWallpaper",
        "Control Panel\\Desktop", "TileWallpaper");
    ok &= capture_original_default_value_checked("WallpaperStyle",
        "Control Panel\\Desktop", "WallpaperStyle");
    ok &= capture_original_default_value_checked("Pattern",
        "Control Panel\\Desktop", "Pattern");
    for (index = 0; index < sizeof(g_w2k_colors) / sizeof(g_w2k_colors[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Color_%s", g_w2k_colors[index].name);
        ok &= capture_original_default_value_checked(marker, "Control Panel\\Colors",
                                                     g_w2k_colors[index].name);
    }
    for (index = 0; index < sizeof(g_w2k_metrics) / sizeof(g_w2k_metrics[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "Metric_%s", g_w2k_metrics[index].name);
        ok &= capture_original_default_value_checked(marker,
            "Control Panel\\Desktop\\WindowMetrics", g_w2k_metrics[index].name);
    }
    for (index = 0; index < sizeof(g_w2k_font_values) / sizeof(g_w2k_font_values[0]); ++index) {
        char marker[128];
        _snprintf(marker, sizeof(marker), "MetricFont_%s", g_w2k_font_values[index]);
        ok &= capture_original_default_value_checked(marker,
            "Control Panel\\Desktop\\WindowMetrics", g_w2k_font_values[index]);
    }
    if (ok) ok &= write_user_dword(CONFIG_KEY, "DefaultAppearanceCaptured", 1);

    ok &= capture_original_machine_value_checked("ResourceReloaderRun",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "eXPerience2K Resource Reloader");
    for (index = 0; index < sizeof(g_explorer_machine_text) /
                              sizeof(g_explorer_machine_text[0]); ++index)
        ok &= capture_original_machine_value_checked(g_explorer_machine_text[index].marker,
            g_explorer_machine_text[index].subkey, g_explorer_machine_text[index].name);

    if (ok) ok &= write_user_dword(CONFIG_KEY, "BaselineMachineSchema", 1);
    append_log_line(ok
        ? "Immutable machine baseline captured successfully."
        : "ERROR: the immutable machine baseline could not be captured completely; no protected changes will be applied.");
    return ok;
}

static int capture_exact_baseline(int include_machine)
{
    int ok = capture_exact_user_baseline();
    if (include_machine) ok &= capture_exact_machine_baseline();
    return ok;
}

static int apply_machine_features(void)
{
    int ok = 1;
    int desired_resource = Button_GetCheck(
        g_features[FEATURE_RESOURCE_CONVERSION].checkbox) == BST_CHECKED;
    int desired_logon = Button_GetCheck(
        g_features[FEATURE_CLASSIC_LOGON].checkbox) == BST_CHECKED;
    int logon_caption_preset = selected_logon_caption_preset();
    int desired_explorer = Button_GetCheck(
        g_features[FEATURE_CLASSIC_EXPLORER].checkbox) == BST_CHECKED;
    int resource_ok = 1;
    char command[4096], targets[MAX_PATH];
    DWORD original, captured;
    if (desired_resource != g_features[FEATURE_RESOURCE_CONVERSION].detected) {
        if (desired_resource) {
            join_path(targets, sizeof(targets), g_install_root, "targets.tsv");
            _snprintf(command, sizeof(command), "\"%s\" install \"%s\" \"%s\"",
                      g_core_path, g_install_root, targets);
            append_log_line("Starting validated resource transaction...");
        } else {
            _snprintf(command, sizeof(command), "\"%s\" uninstall \"%s\"",
                      g_core_path, g_install_root);
            append_log_line("Starting validated resource restoration...");
        }
        resource_ok = run_captured(command);
        if (resource_ok && !write_user_dword(CONFIG_KEY,
                "ResourceConversionEnabled", desired_resource ? 1 : 0)) {
            append_log_line("ERROR: the resource-conversion state marker could not be written.");
            resource_ok = 0;
        }
        ok &= resource_ok;
        append_log_line(resource_ok ? "Resource transaction completed; reboot may be required."
                                    : "ERROR: resource transaction failed before completion.");
    }
    if (resource_ok) {
        int reloader_ok = configure_resource_reloader(desired_resource);
        ok &= reloader_ok;
        if (desired_resource)
            append_log_line(reloader_ok
                ? "Protected-resource reloader enabled for sign-in persistence."
                : "ERROR: the protected-resource reloader could not be enabled.");
        else if (!reloader_ok)
            append_log_line("ERROR: the protected-resource reloader could not be removed.");
    }
    {
        DWORD user_managed = 0, machine_managed = 0;
        int explorer_managed =
            (read_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", &user_managed) &&
             user_managed) ||
            (read_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", &machine_managed) &&
             machine_managed);
        if (desired_explorer != g_features[FEATURE_CLASSIC_EXPLORER].detected ||
            (!desired_explorer && explorer_managed)) {
            int explorer_ok;
            if (desired_explorer) {
                explorer_ok = apply_w2k_explorer_machine_state(1);
                if (explorer_ok)
                    explorer_ok = apply_w2k_explorer_user_state(1);
                /* Explorer state is installed after the user-facing options.
                   Reassert the selected native Start-menu mode last so the
                   Taskbar Properties choice and the forced mode agree. */
                if (explorer_ok &&
                    Button_GetCheck(g_features[FEATURE_CLASSIC_START_MENU].checkbox) == BST_CHECKED) {
                    explorer_ok &= write_start_panel_on(0);
                    explorer_ok &= write_user_dword(
                        "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                        "NoSimpleStartMenu", 1);
                }
                if (!explorer_ok) {
                    apply_w2k_explorer_user_state(0);
                    apply_w2k_explorer_machine_state(0);
                }
            } else {
                explorer_ok = apply_w2k_explorer_user_state(0);
                explorer_ok &= apply_w2k_explorer_machine_state(0);
            }
            ok &= explorer_ok;
        }
    }
    if (desired_logon != g_features[FEATURE_CLASSIC_LOGON].detected) {
        int logon_ok;
        if (!read_user_dword(CONFIG_KEY, "Original_LogonType_Present", &captured)) {
            if (read_machine_dword("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "LogonType", &original)) {
                write_user_dword(CONFIG_KEY, "Original_LogonType_Present", 1);
                write_user_dword(CONFIG_KEY, "Original_LogonType_Value", original);
            } else {
                write_user_dword(CONFIG_KEY, "Original_LogonType_Present", 0);
            }
        }
        if (!desired_logon) {
            DWORD original_present = 0;
            read_user_dword(CONFIG_KEY, "Original_LogonType_Present", &original_present);
            if (original_present) {
                original = 1;
                read_user_dword(CONFIG_KEY, "Original_LogonType_Value", &original);
                logon_ok = write_machine_dword(
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    "LogonType", original);
            } else {
                logon_ok = delete_machine_value(
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    "LogonType");
            }
        } else {
            logon_ok = write_machine_dword(
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                "LogonType", 0);
        }
        ok &= logon_ok;
        append_log_line(desired_logon ? "Windows 2000 style login window enabled."
                                     : "Original login-window selection restored.");
        if (!logon_ok) append_log_line("ERROR: the native Winlogon setting could not be updated.");
    }
    ok &= apply_default_w2k_appearance(desired_logon, logon_caption_preset);
    return ok;
}

static void record_restore_result(int *overall, int result, const char *stage)
{
    char message[320];
    if (result) return;
    *overall = 0;
    _snprintf(message, sizeof(message), "ERROR: restoration stage failed: %s.", stage);
    append_log_line(message);
}

static int restore_all_managed_features(int clear_saved_state)
{
    DWORD configured = 0, marker = 0, value = 0, present = 0;
    DWORD animation_value = 1, fade_value = 1;
    BOOL animation, fade;
    char command[4096], state[MAX_PATH];
    int ok = 1;
    int desktop_caption_preset = saved_caption_preset(
        "DesktopCaptionPreset", CAPTION_PRESET_BLUE_GRADIENT);
    int logon_caption_preset = saved_caption_preset(
        "LogonCaptionPreset", CAPTION_PRESET_SOLID_NAVY);

    append_log_line(clear_saved_state
        ? "Starting complete uninstall restoration for the interactive user and protected system files..."
        : "Starting complete Revert restoration to the immutable pre-Apply baseline...");
    if (!g_probe.administrator) {
        append_log_line("ERROR: complete uninstall restoration requires administrator privileges.");
        return 0;
    }

    record_restore_result(&ok, configure_resource_reloader(0),
                          "resource-reloader cleanup");
    if (join_path(state, sizeof(state), g_install_root, "state.tsv") && file_exists(state)) {
        _snprintf(command, sizeof(command), "\"%s\" uninstall \"%s\"",
                  g_core_path, g_install_root);
        if (!run_captured(command)) {
            append_log_line("ERROR: protected resource restoration failed.");
            ok = 0;
        } else {
            record_restore_result(&ok,
                write_user_dword(CONFIG_KEY, "ResourceConversionEnabled", 0),
                "resource-conversion state marker");
        }
    } else {
        record_restore_result(&ok,
            write_user_dword(CONFIG_KEY, "ResourceConversionEnabled", 0),
            "resource-conversion state marker");
    }

    {
        DWORD explorer_user = 0, explorer_machine = 0;
        char payload_root[MAX_PATH], target_root[MAX_PATH], backup_root[MAX_PATH];
        char explorer_state[MAX_PATH];
        int has_explorer_state =
            explorer_web_paths(payload_root, sizeof(payload_root),
                               target_root, sizeof(target_root),
                               backup_root, sizeof(backup_root),
                               explorer_state, sizeof(explorer_state)) &&
            file_exists(explorer_state);
        if ((read_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", &explorer_user) &&
             explorer_user) ||
            (read_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", &explorer_machine) &&
             explorer_machine) || has_explorer_state) {
            record_restore_result(&ok, apply_w2k_explorer_user_state(0),
                                  "Explorer user state");
            record_restore_result(&ok, apply_w2k_explorer_machine_state(0),
                                  "Explorer machine state");
        }
    }

    if (read_user_dword(CONFIG_KEY, "Configured", &configured) && configured) {
        if (read_user_dword(CONFIG_KEY, "W2KColorProfileEnabled", &value) && value) {
            record_restore_result(&ok, apply_classic_theme(0, desktop_caption_preset),
                                  "interactive-user theme and palette");
            record_restore_result(&ok, apply_w2k_font_metrics(0),
                                  "interactive-user fonts and metrics");
        }

        if (read_user_dword(CONFIG_KEY, "Original_ClassicStartShellState_Captured", &marker))
            record_restore_result(&ok, restore_original_user_value("ClassicStartShellState",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                "ShellState"), "complete Start-menu and taskbar shell state");
        if (read_user_dword(CONFIG_KEY, "Original_NoSimpleStartMenu_Captured", &marker)) {
            record_restore_result(&ok, restore_original_user_value("NoSimpleStartMenu",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                "NoSimpleStartMenu"), "Classic Start-menu policy");
            record_restore_result(&ok,
                write_user_dword(CONFIG_KEY, "ClassicStartPolicyManaged", 0),
                "Classic Start-menu ownership marker");
        }

        if (read_user_dword(CONFIG_KEY, "Original_ClassicControlPanel_Captured", &marker))
            record_restore_result(&ok, restore_original_user_value("ClassicControlPanel",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel",
                "StartupPage"), "Classic Control Panel preference");
        if (read_user_dword(CONFIG_KEY, "Original_ForceClassicControlPanel_Captured", &marker))
            record_restore_result(&ok, restore_original_user_value("ForceClassicControlPanel",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                "ForceClassicControlPanel"), "Classic Control Panel policy");

        if (read_user_dword(CONFIG_KEY, "Original_MenuAnimation_Present", &marker)) {
            read_user_dword(CONFIG_KEY, "Original_MenuAnimation_Value", &animation_value);
            read_user_dword(CONFIG_KEY, "Original_MenuFade_Value", &fade_value);
            animation = animation_value != 0;
            fade = fade_value != 0;
            record_restore_result(&ok, SystemParametersInfoA(SPI_SETMENUANIMATION, 0,
                (PVOID)(INT_PTR)(animation ? TRUE : FALSE),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0,
                "menu-animation setting");
            record_restore_result(&ok, SystemParametersInfoA(SPI_SETMENUFADE, 0,
                (PVOID)(INT_PTR)(fade ? TRUE : FALSE),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != 0,
                "menu-fade setting");
        }

        if (read_user_dword(CONFIG_KEY, "Original_LogonType_Present", &present)) {
            if (present) {
                value = 1;
                read_user_dword(CONFIG_KEY, "Original_LogonType_Value", &value);
                record_restore_result(&ok, write_machine_dword(
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    "LogonType", value), "classic-logon selection");
            } else {
                record_restore_result(&ok, delete_machine_value(
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    "LogonType"), "classic-logon selection");
            }
        }
        record_restore_result(&ok, apply_wallpaper_installation(0),
                              "wallpaper installation");
        record_restore_result(&ok, apply_windows_2000_sounds(0),
                              "Windows sound assignments");
        record_restore_result(&ok, apply_windows_2000_double_click_sound(0),
                              "folder double-click sound assignment");
        record_restore_result(&ok, apply_default_w2k_appearance(0, logon_caption_preset),
                              "login-desktop appearance");
    }

    if (ok && clear_saved_state) {
        ok &= delete_user_tree("Software\\eXPerience2K");
        ok &= delete_user_tree("Software\\eXPerience2K64");
        append_log_line(ok ? "Complete uninstall restoration succeeded."
                           : "ERROR: restored settings but could not remove saved configuration state.");
    } else if (ok) {
        ok &= write_user_dword(CONFIG_KEY, "Configured", 1);
        append_log_line(ok
            ? "Complete Revert restoration succeeded; the immutable baseline was retained."
            : "ERROR: the original setup was restored but its configured-state marker could not be retained.");
    }
    return ok;
}

static int needs_administrator_change(void)
{
    int index;
    DWORD default_appearance = 0;
    DWORD saved_logon_preset = CAPTION_PRESET_SOLID_NAVY;
    for (index = 0; index < MAX_FEATURES; ++index) {
        int desired;
        if (!g_features[index].implemented || !g_features[index].administrator_required) continue;
        desired = Button_GetCheck(g_features[index].checkbox) == BST_CHECKED;
        if (desired != g_features[index].detected) return 1;
    }
    if (Button_GetCheck(g_features[FEATURE_CLASSIC_LOGON].checkbox) == BST_CHECKED) {
        if (!read_user_dword(CONFIG_KEY, "DefaultW2KAppearanceEnabled", &default_appearance) ||
            !default_appearance) return 1;
        if (!read_user_dword(CONFIG_KEY, "LogonCaptionPreset", &saved_logon_preset) ||
            !caption_preset_valid(saved_logon_preset) ||
            (int)saved_logon_preset != selected_logon_caption_preset()) return 1;
    }
    return 0;
}

static int apply_all_defaults_unattended(void)
{
    int index;
    int ok;
    if (!g_probe.supported || !g_probe.resource_ready || !g_probe.administrator) {
        append_log_line("ERROR: unattended default application requires a supported, resource-ready profile and administrator privileges.");
        return 0;
    }
    /* VBox Guest Control launches the test process outside Explorer's visible
       desktop even when it uses the same Administrator account.  The normal
       interactive UI keeps automatic shell-user discovery; this test-only
       switch deliberately targets its own authenticated HKCU. */
    g_use_current_user_fallback = 1;
    g_cross_user = 0;
    for (index = 0; index < MAX_FEATURES; ++index) {
        g_features[index].detected = detect_feature_state(index);
        g_features[index].checkbox = CreateWindowA("BUTTON", "",
            WS_POPUP | BS_AUTOCHECKBOX, 0, 0, 0, 0,
            NULL, NULL, g_instance, NULL);
        if (!g_features[index].checkbox) {
            int cleanup;
            for (cleanup = 0; cleanup < index; ++cleanup) {
                DestroyWindow(g_features[cleanup].checkbox);
                g_features[cleanup].checkbox = NULL;
            }
            append_log_line("ERROR: unattended configuration controls could not be initialized.");
            return 0;
        }
        Button_SetCheck(g_features[index].checkbox,
            g_features[index].default_on ? BST_CHECKED : BST_UNCHECKED);
    }
    append_log_line("Applying all first-launch defaults through the normal feature transaction...");
    ok = capture_exact_baseline(1);
    if (ok) {
        ok = apply_user_features();
        ok &= apply_machine_features();
    }
    for (index = 0; index < MAX_FEATURES; ++index) {
        DestroyWindow(g_features[index].checkbox);
        g_features[index].checkbox = NULL;
    }
    append_log_line(ok ? "All first-launch defaults applied successfully."
                       : "ERROR: one or more first-launch defaults failed.");
    return ok;
}

static void update_caption_preset_controls(void)
{
    if (g_logon_caption_combo)
        EnableWindow(g_logon_caption_combo,
            g_features[FEATURE_CLASSIC_LOGON].implemented &&
            Button_GetCheck(g_features[FEATURE_CLASSIC_LOGON].checkbox) == BST_CHECKED);
    if (g_desktop_caption_combo)
        EnableWindow(g_desktop_caption_combo,
            g_features[FEATURE_CLASSIC_THEME].implemented &&
            Button_GetCheck(g_features[FEATURE_CLASSIC_THEME].checkbox) == BST_CHECKED);
}

static void refresh_caption_presets(int use_first_launch_defaults)
{
    DWORD configured = 0;
    int has_config = read_user_dword(CONFIG_KEY, "Configured", &configured) && configured;
    int logon_preset;
    int desktop_preset;
    if (!has_config && use_first_launch_defaults) {
        logon_preset = CAPTION_PRESET_SOLID_NAVY;
        desktop_preset = CAPTION_PRESET_BLUE_GRADIENT;
    } else {
        logon_preset = saved_caption_preset("LogonCaptionPreset",
                                            CAPTION_PRESET_SOLID_NAVY);
        desktop_preset = saved_caption_preset("DesktopCaptionPreset",
                                              CAPTION_PRESET_BLUE_GRADIENT);
    }
    if (g_logon_caption_combo)
        SendMessageA(g_logon_caption_combo, CB_SETCURSEL, logon_preset, 0);
    if (g_desktop_caption_combo)
        SendMessageA(g_desktop_caption_combo, CB_SETCURSEL, desktop_preset, 0);
    update_caption_preset_controls();
}

static void refresh_states(int use_first_launch_defaults)
{
    int index;
    DWORD configured = 0;
    int has_config = read_user_dword(CONFIG_KEY, "Configured", &configured) && configured;
    for (index = 0; index < MAX_FEATURES; ++index) {
        int checked;
        g_features[index].detected = detect_feature_state(index);
        checked = (!has_config && use_first_launch_defaults && g_features[index].implemented)
            ? g_features[index].default_on : g_features[index].detected;
        Button_SetCheck(g_features[index].checkbox, checked ? BST_CHECKED : BST_UNCHECKED);
    }
    refresh_caption_presets(use_first_launch_defaults);
}

static void set_log_visibility(int visible)
{
    if (!g_window || !g_log_edit) return;
    ShowWindow(g_log_edit, visible ? SW_SHOW : SW_HIDE);
    SetWindowTextA(GetDlgItem(g_window, IDC_OPEN_LOG),
        visible ? "&Hide log" : "&Open log");
    g_scroll_y = visible ? MAIN_EXPANDED_CLIENT_HEIGHT : 0;
    SendMessageA(g_window, WM_SIZE, SIZE_RESTORED, 0);
}

static int main_content_height(void)
{
    return g_log_edit && IsWindowVisible(g_log_edit)
        ? MAIN_EXPANDED_CLIENT_HEIGHT : MAIN_COMPACT_CLIENT_HEIGHT;
}

static void layout_main_controls(HWND window)
{
    RECT client;
    SCROLLINFO scroll;
    int client_width;
    int client_height;
    int content_height;
    int maximum_scroll;
    int y_offset;
    int index;
    int button_width;
    int button_gap = 7;
    int button_x;
    int left_combo_x = 34;
    int left_combo_width;
    int right_combo_x;
    int right_combo_width;
    int combo_gap;
    HWND preset_group;
    HWND button;
    const int button_ids[] = {
        IDC_APPLY, IDC_REVERT, IDC_OPEN_LOG, IDC_SAVE_LOG, IDC_CLOSE
    };

    if (!window || !g_status) return;
    GetClientRect(window, &client);
    client_width = client.right - client.left;
    client_height = client.bottom - client.top;
    content_height = main_content_height();
    maximum_scroll = content_height > client_height
        ? content_height - client_height : 0;
    if (g_scroll_y < 0) g_scroll_y = 0;
    if (g_scroll_y > maximum_scroll) g_scroll_y = maximum_scroll;

    if ((maximum_scroll > 0) != g_scrollbar_visible) {
        g_scrollbar_visible = maximum_scroll > 0;
        ShowScrollBar(window, SB_VERT, g_scrollbar_visible);
        GetClientRect(window, &client);
        client_width = client.right - client.left;
        client_height = client.bottom - client.top;
        maximum_scroll = content_height > client_height
            ? content_height - client_height : 0;
        if (g_scroll_y > maximum_scroll) g_scroll_y = maximum_scroll;
    }

    ZeroMemory(&scroll, sizeof(scroll));
    scroll.cbSize = sizeof(scroll);
    scroll.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll.nMin = 0;
    scroll.nMax = content_height > 0 ? content_height - 1 : 0;
    scroll.nPage = client_height > 0 ? (UINT)client_height : 0;
    scroll.nPos = g_scroll_y;
    SetScrollInfo(window, SB_VERT, &scroll, TRUE);
    y_offset = -g_scroll_y;

    MoveWindow(g_status, 16, 14 + y_offset,
        client_width > 32 ? client_width - 32 : 1, 72, TRUE);
    for (index = 0; index < MAX_FEATURES; ++index) {
        if (g_features[index].checkbox)
            MoveWindow(g_features[index].checkbox, 22,
                92 + index * 27 + y_offset,
                client_width > 42 ? client_width - 42 : 1, 23, TRUE);
    }

    preset_group = GetDlgItem(window, IDC_CAPTION_PRESET_GROUP);
    if (preset_group)
        MoveWindow(preset_group, 22, 390 + y_offset,
            client_width > 42 ? client_width - 42 : 1, 69, TRUE);

    if (client_width >= MAIN_CLIENT_WIDTH) {
        int extra = client_width - MAIN_CLIENT_WIDTH;
        left_combo_width = 200 + extra / 2;
        right_combo_x = 266 + extra / 2;
        right_combo_width = client_width - right_combo_x - 48;
    } else {
        combo_gap = 16;
        left_combo_width = (client_width - 68 - combo_gap) / 2;
        if (left_combo_width < 120) left_combo_width = 120;
        right_combo_x = left_combo_x + left_combo_width + combo_gap;
        right_combo_width = client_width - right_combo_x - 34;
        if (right_combo_width < 120) right_combo_width = 120;
    }
    MoveWindow(g_logon_caption_label, left_combo_x, 408 + y_offset,
        left_combo_width, 17, TRUE);
    MoveWindow(g_desktop_caption_label, right_combo_x, 408 + y_offset,
        right_combo_width, 17, TRUE);
    MoveWindow(g_logon_caption_combo, left_combo_x, 425 + y_offset,
        left_combo_width, 100, TRUE);
    MoveWindow(g_desktop_caption_combo, right_combo_x, 425 + y_offset,
        right_combo_width, 100, TRUE);

    button_width = (client_width - 44 - button_gap * 4) / 5;
    if (button_width < 64) button_width = 64;
    button_x = 22;
    for (index = 0; index < 5; ++index) {
        button = GetDlgItem(window, button_ids[index]);
        if (button)
            MoveWindow(button, button_x, MAIN_BUTTON_TOP + y_offset,
                button_width, 30, TRUE);
        button_x += button_width + button_gap;
    }

    if (g_log_edit)
        MoveWindow(g_log_edit, 22, MAIN_LOG_TOP + y_offset,
            client_width > 38 ? client_width - 38 : 1,
            MAIN_LOG_HEIGHT, TRUE);
}

static void scroll_main_controls(HWND window, int command, int track_position)
{
    RECT client;
    int content_height;
    int page;
    int maximum_scroll;
    int new_position = g_scroll_y;
    GetClientRect(window, &client);
    page = client.bottom - client.top;
    content_height = main_content_height();
    maximum_scroll = content_height > page ? content_height - page : 0;
    switch (command) {
    case SB_TOP: new_position = 0; break;
    case SB_BOTTOM: new_position = maximum_scroll; break;
    case SB_LINEUP: new_position -= 27; break;
    case SB_LINEDOWN: new_position += 27; break;
    case SB_PAGEUP: new_position -= page > 40 ? page - 40 : page; break;
    case SB_PAGEDOWN: new_position += page > 40 ? page - 40 : page; break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK: new_position = track_position; break;
    default: return;
    }
    if (new_position < 0) new_position = 0;
    if (new_position > maximum_scroll) new_position = maximum_scroll;
    if (new_position == g_scroll_y) return;
    g_scroll_y = new_position;
    layout_main_controls(window);
}

static HWND show_progress_dialog(const char *message)
{
    RECT owner;
    HWND dialog;
    HWND label;
    int width = 300;
    int height = 92;
    int x;
    int y;
    GetWindowRect(g_window, &owner);
    x = owner.left + ((owner.right - owner.left) - width) / 2;
    y = owner.top + ((owner.bottom - owner.top) - height) / 2;
    EnableWindow(g_window, FALSE);
    dialog = CreateWindowExA(WS_EX_DLGMODALFRAME, APPLYING_WINDOW_CLASS,
        APP_TITLE, WS_POPUP | WS_CAPTION | WS_BORDER,
        x, y, width, height, g_window, NULL, g_instance, NULL);
    if (!dialog) {
        EnableWindow(g_window, TRUE);
        return NULL;
    }
    label = CreateWindowA("STATIC", message,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        12, 8, 276, 46, dialog, NULL, g_instance, NULL);
    if (label)
        SendMessage(label, WM_SETFONT,
            (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    ShowWindow(dialog, SW_SHOWNORMAL);
    UpdateWindow(dialog);
    g_applying_dialog = dialog;
    RedrawWindow(dialog, NULL, NULL,
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    return dialog;
}

static void close_applying_dialog(HWND dialog)
{
    if (dialog) DestroyWindow(dialog);
    g_applying_dialog = NULL;
    EnableWindow(g_window, TRUE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

static void show_error_guidance(const char *message)
{
    char text[1024];
    _snprintf(text, sizeof(text),
        "%s\n\nNo personal information is added to the diagnostic log. Use Open log to inspect it, or Save log to choose a location. If you report this on GitHub, include the log contents.",
        message);
    MessageBoxA(g_window, text, "eXPerience2K could not apply every requested change",
                MB_OK | MB_ICONERROR);
    set_log_visibility(1);
}

static void apply_requested_configuration(void)
{
    int success;
    HWND applying_dialog;
    if (!g_probe.supported) {
        show_error_guidance("This operating-system profile is not explicitly supported. No changes were attempted.");
        return;
    }
    if (!g_probe.administrator && needs_administrator_change()) {
        MessageBoxA(g_window,
            "Some selected changes modify protected system assets and require administrator privileges. No changes were attempted.\n\nClose eXPerience2K, right-click it, choose Run As, and select an account in the Administrators group. Per-user options are labelled [Current user]; protected options are labelled [Administrator].",
            "Administrator privileges required", MB_OK | MB_ICONWARNING);
        return;
    }
    if (Button_GetCheck(g_features[FEATURE_RESOURCE_CONVERSION].checkbox) == BST_CHECKED &&
        !g_probe.resource_ready) {
        show_error_guidance("This OS is recognized, but its exact resource payload has not passed validation. The system-file conversion was not attempted.");
        return;
    }
    g_apply_in_progress = 1;
    applying_dialog = show_progress_dialog("Applying change. Please wait...");
    pump_ui_messages();
    append_log_line("--- Apply started ---");
    success = capture_exact_baseline(g_probe.administrator);
    if (success) {
        success = apply_user_features();
        if (g_probe.administrator) success &= apply_machine_features();
    }
    close_applying_dialog(applying_dialog);
    g_apply_in_progress = 0;
    if (!success) {
        show_error_guidance("At least one requested operation failed or could not be verified.");
    } else {
        append_log_line("Apply completed successfully.");
        MessageBoxA(g_window,
            "The selected configuration was applied. A sign-out or reboot may be required for shell and protected-file changes to become visible.",
            APP_TITLE, MB_OK | MB_ICONINFORMATION);
    }
    refresh_states(0);
}

static void revert_to_initial_setup(void)
{
    DWORD configured = 0;
    DWORD baseline_schema = 0;
    int success;
    HWND progress_dialog;
    int confirmation;

    if (!g_probe.supported) {
        show_error_guidance("This operating-system profile is not explicitly supported. No changes were attempted.");
        return;
    }
    if (!g_probe.administrator) {
        MessageBoxA(g_window,
            "Reverting every managed change requires administrator privileges. No changes were attempted.\n\nClose eXPerience2K, right-click it, choose Run As, and select an account in the Administrators group.",
            "Administrator privileges required", MB_OK | MB_ICONWARNING);
        return;
    }
    if (!(read_user_dword(CONFIG_KEY, "Configured", &configured) && configured) &&
        !(read_user_dword(CONFIG_KEY, "BaselineUserSchema", &baseline_schema) &&
          baseline_schema == 1)) {
        MessageBoxA(g_window,
            "No saved pre-Apply setup is available to restore. eXPerience2K has not recorded an original configuration on this account.",
            "Nothing to revert", MB_OK | MB_ICONINFORMATION);
        return;
    }

    confirmation = MessageBoxA(g_window,
        "This will revert your setup to the original state captured before eXPerience2K first applied any changes. All currently managed eXPerience2K changes will be restored to that baseline.\n\nAre you sure you want to continue?",
        "Revert all eXPerience2K changes?",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (confirmation != IDYES) return;

    g_apply_in_progress = 1;
    progress_dialog = show_progress_dialog("Reverting changes. Please wait...");
    pump_ui_messages();
    append_log_line("--- Revert started ---");
    success = restore_all_managed_features(0);
    close_applying_dialog(progress_dialog);
    g_apply_in_progress = 0;
    if (!success) {
        show_error_guidance("The original pre-Apply setup could not be fully restored. Recovery data was retained; inspect the log before trying again.");
    } else {
        append_log_line("Revert completed successfully.");
        MessageBoxA(g_window,
            "Your original pre-Apply setup was restored. A sign-out or reboot may be required for shell and protected-file changes to become visible.",
            APP_TITLE, MB_OK | MB_ICONINFORMATION);
    }
    refresh_states(0);
}

static void save_log(void)
{
    OPENFILENAMEA dialog;
    char path[MAX_PATH] = "eXPerience2K-log.txt";
    HANDLE file;
    DWORD written;
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = sizeof(path);
    dialog.lpstrDefExt = "txt";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameA(&dialog)) return;
    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_window, "The selected log file could not be created.", APP_TITLE,
                    MB_OK | MB_ICONERROR);
        return;
    }
    WriteFile(file, g_log, (DWORD)g_log_length, &written, NULL);
    CloseHandle(file);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    int index;
    (void)lparam;
    switch (message) {
    case WM_GETMINMAXINFO: {
        MINMAXINFO *limits = (MINMAXINFO *)lparam;
        RECT minimum;
        DWORD style = (DWORD)GetWindowLongA(window, GWL_STYLE);
        DWORD extended_style = (DWORD)GetWindowLongA(window, GWL_EXSTYLE);
        minimum.left = 0;
        minimum.top = 0;
        minimum.right = MAIN_MIN_CLIENT_WIDTH;
        minimum.bottom = MAIN_MIN_CLIENT_HEIGHT;
        AdjustWindowRectEx(&minimum, style, FALSE, extended_style);
        limits->ptMinTrackSize.x = minimum.right - minimum.left;
        limits->ptMinTrackSize.y = minimum.bottom - minimum.top;
        return 0;
    }
    case WM_CREATE: {
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        char status[512];
        HWND preset_group;
        g_status = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
            16, 14, MAIN_CLIENT_WIDTH - 32, 72, window,
            (HMENU)IDC_STATUS, g_instance, NULL);
        SendMessage(g_status, WM_SETFONT, (WPARAM)font, TRUE);
        for (index = 0; index < MAX_FEATURES; ++index) {
            char label[512];
            _snprintf(label, sizeof(label), "[%s] %s",
                g_features[index].administrator_required ? "Administrator" : "Current user",
                g_features[index].label);
            g_features[index].checkbox = CreateWindowA("BUTTON", label,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                22, 92 + index * 27, MAIN_CLIENT_WIDTH - 42, 23, window,
                (HMENU)(INT_PTR)(IDC_FEATURE_BASE + index), g_instance, NULL);
            SendMessage(g_features[index].checkbox, WM_SETFONT, (WPARAM)font, TRUE);
            if (!g_features[index].implemented || (index == 0 && !g_probe.resource_ready))
                EnableWindow(g_features[index].checkbox, FALSE);
        }
        preset_group = CreateWindowA("BUTTON", "Caption color presets (independent)",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            22, 390, MAIN_CLIENT_WIDTH - 42, 69, window,
            (HMENU)IDC_CAPTION_PRESET_GROUP, g_instance, NULL);
        g_logon_caption_label = CreateWindowA("STATIC", "[Administrator] Logon prompt:",
            WS_CHILD | WS_VISIBLE,
            34, 408, 210, 17, window, (HMENU)IDC_LOGON_CAPTION_LABEL, g_instance, NULL);
        g_desktop_caption_label = CreateWindowA("STATIC", "[Current user] Signed-in Windows:",
            WS_CHILD | WS_VISIBLE,
            266, 408, 210, 17, window, (HMENU)IDC_DESKTOP_CAPTION_LABEL, g_instance, NULL);
        g_logon_caption_combo = CreateWindowA("COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            34, 425, 200, 100, window, (HMENU)IDC_LOGON_CAPTION_PRESET,
            g_instance, NULL);
        g_desktop_caption_combo = CreateWindowA("COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            266, 425, 200, 100, window, (HMENU)IDC_DESKTOP_CAPTION_PRESET,
            g_instance, NULL);
        SendMessageA(g_logon_caption_combo, CB_ADDSTRING, 0, (LPARAM)"Solid Navy");
        SendMessageA(g_logon_caption_combo, CB_ADDSTRING, 0, (LPARAM)"Blue Gradient");
        SendMessageA(g_desktop_caption_combo, CB_ADDSTRING, 0, (LPARAM)"Solid Navy");
        SendMessageA(g_desktop_caption_combo, CB_ADDSTRING, 0, (LPARAM)"Blue Gradient");
        SendMessage(preset_group, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(g_logon_caption_label, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(g_desktop_caption_label, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(g_logon_caption_combo, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(g_desktop_caption_combo, WM_SETFONT, (WPARAM)font, TRUE);
        CreateWindowA("BUTTON", "&Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            22, MAIN_BUTTON_TOP, 88, 30, window, (HMENU)IDC_APPLY, g_instance, NULL);
        CreateWindowA("BUTTON", "&Revert", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            117, MAIN_BUTTON_TOP, 88, 30, window, (HMENU)IDC_REVERT, g_instance, NULL);
        CreateWindowA("BUTTON", "&Open log", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            212, MAIN_BUTTON_TOP, 88, 30, window, (HMENU)IDC_OPEN_LOG, g_instance, NULL);
        CreateWindowA("BUTTON", "&Save log...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            307, MAIN_BUTTON_TOP, 88, 30, window, (HMENU)IDC_SAVE_LOG, g_instance, NULL);
        CreateWindowA("BUTTON", "&Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            402, MAIN_BUTTON_TOP, 88, 30, window,
            (HMENU)IDC_CLOSE, g_instance, NULL);
        g_log_edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", g_log,
            WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            22, MAIN_LOG_TOP, MAIN_CLIENT_WIDTH - 38, MAIN_LOG_HEIGHT, window,
            (HMENU)IDC_LOG, g_instance, NULL);
        SendMessage(g_log_edit, WM_SETFONT, (WPARAM)font, TRUE);
        _snprintf(status, sizeof(status),
            "%s\r\nProfile: %s\r\nPrivileges: %s%s",
            g_probe.supported ? "Supported operating system detected." : "Unsupported operating-system profile: no changes are permitted.",
            g_probe.display_name[0] ? g_probe.display_name : "unknown",
            g_probe.administrator ? "Administrator" : "Current user only",
            g_cross_user ? " (Run As account differs; settings target the interactive desktop user)" : "");
        SetWindowTextA(g_status, status);
        refresh_states(1);
        layout_main_controls(window);
        return 0;
    }
    case WM_SIZE:
        layout_main_controls(window);
        return 0;
    case WM_VSCROLL: {
        SCROLLINFO scroll;
        ZeroMemory(&scroll, sizeof(scroll));
        scroll.cbSize = sizeof(scroll);
        scroll.fMask = SIF_TRACKPOS;
        GetScrollInfo(window, SB_VERT, &scroll);
        scroll_main_controls(window, LOWORD(wparam), scroll.nTrackPos);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int wheel_delta = (short)HIWORD(wparam);
        if (wheel_delta > 0)
            scroll_main_controls(window, SB_LINEUP, 0);
        else if (wheel_delta < 0)
            scroll_main_controls(window, SB_LINEDOWN, 0);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDC_FEATURE_BASE + FEATURE_MENU_SLIDE:
            if (HIWORD(wparam) == BN_CLICKED &&
                Button_GetCheck(g_features[FEATURE_MENU_SLIDE].checkbox) == BST_CHECKED)
                Button_SetCheck(g_features[FEATURE_MENU_FADE].checkbox, BST_UNCHECKED);
            return 0;
        case IDC_FEATURE_BASE + FEATURE_MENU_FADE:
            if (HIWORD(wparam) == BN_CLICKED &&
                Button_GetCheck(g_features[FEATURE_MENU_FADE].checkbox) == BST_CHECKED)
                Button_SetCheck(g_features[FEATURE_MENU_SLIDE].checkbox, BST_UNCHECKED);
            return 0;
        case IDC_FEATURE_BASE + FEATURE_CLASSIC_THEME:
        case IDC_FEATURE_BASE + FEATURE_CLASSIC_LOGON:
            if (HIWORD(wparam) == BN_CLICKED) update_caption_preset_controls();
            return 0;
        case IDC_APPLY:
            apply_requested_configuration();
            return 0;
        case IDC_REVERT:
            revert_to_initial_setup();
            return 0;
        case IDC_OPEN_LOG:
            set_log_visibility(!IsWindowVisible(g_log_edit));
            return 0;
        case IDC_SAVE_LOG:
            save_log();
            return 0;
        case IDC_CLOSE:
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    WNDCLASSA window_class;
    WNDCLASSA applying_class;
    MSG message;
    SYSTEM_INFO system_info;
    INITCOMMONCONTROLSEX controls;
    char executable[MAX_PATH];
    RECT initial_rectangle;
    DWORD main_style = WS_OVERLAPPEDWINDOW | WS_VSCROLL;
    (void)previous;
    g_instance = instance;
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);
    GetModuleFileNameA(NULL, executable, sizeof(executable));
    lstrcpynA(g_install_root, executable, sizeof(g_install_root));
    parent_directory(g_install_root);
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    if (system_info.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64 &&
        system_info.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL) {
        MessageBoxA(NULL,
            "Only Windows XP Professional x86 Service Pack 3 and Windows XP "
            "Professional x64 Edition Service Pack 2 are supported by "
            "eXPerience2K 3.0.0.\n\n"
            "No files or settings have been changed.",
            "eXPerience2K - Unsupported operating system",
            MB_OK | MB_ICONSTOP);
        return 1;
    }
    join_path(g_core_path, sizeof(g_core_path), g_install_root,
        system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
            ? "eXPerience2KCore-x64.exe"
            : "eXPerience2KCore-x86.exe");
    discover_interactive_user();
    g_probe.administrator = token_is_administrator();
    append_log_line("eXPerience2K diagnostic log (privacy-safe; no account names, SIDs, profile paths, or product keys)." );
    if (lstrcmpiA(command_line, "/reload-resources") == 0) {
        char reload_command[2 * MAX_PATH + 64];
        DWORD explorer_enabled = 0;
        int reload_ok;
        _snprintf(reload_command, sizeof(reload_command),
                  "\"%s\" reload \"%s\"", g_core_path, g_install_root);
        reload_ok = run_captured(reload_command);
        if (g_probe.administrator &&
            read_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled",
                               &explorer_enabled) && explorer_enabled) {
            reload_ok &= apply_w2k_explorer_machine_state(1);
            reload_ok &= apply_w2k_explorer_user_state(1);
        }
        return reload_ok ? 0 : 1;
    }
    if (!file_exists(g_core_path) || !run_probe()) {
        lstrcpynA(g_probe.reason, "The native OS probe could not run.", sizeof(g_probe.reason));
        append_log_line("ERROR: native OS probe failed.");
    }
    if (lstrcmpiA(command_line, "/apply-explorer-experiment") == 0) {
        int explorer_ok;
        if (!g_probe.supported || !g_probe.administrator ||
            !explorer_experiment_architecture_supported()) {
            append_log_line("ERROR: Explorer experiment requires a supported Windows XP Professional x86 SP3 or x64 SP2 installation and administrator privileges.");
            save_unattended_log();
            return 1;
        }
        g_use_current_user_fallback = 1;
        g_cross_user = 0;
        explorer_ok = apply_w2k_explorer_machine_state(1);
        if (explorer_ok) explorer_ok = apply_w2k_explorer_user_state(1);
        if (!explorer_ok) {
            apply_w2k_explorer_user_state(0);
            apply_w2k_explorer_machine_state(0);
            save_unattended_log();
            return 1;
        }
        save_unattended_log();
        return 0;
    }
    if (lstrcmpiA(command_line, "/apply-all-defaults-unattended") == 0) {
        int unattended_ok = apply_all_defaults_unattended();
        save_unattended_log();
        return unattended_ok ? 0 : 1;
    }
    if (lstrcmpiA(command_line, "/restore-explorer-experiment") == 0) {
        int explorer_ok;
        if (!g_probe.administrator) {
            append_log_line("ERROR: Explorer experiment restoration requires administrator privileges.");
            save_unattended_log();
            return 1;
        }
        g_use_current_user_fallback = 1;
        g_cross_user = 0;
        explorer_ok = apply_w2k_explorer_user_state(0);
        explorer_ok &= apply_w2k_explorer_machine_state(0);
        save_unattended_log();
        return explorer_ok ? 0 : 1;
    }
    if (lstrcmpiA(command_line, "/restore-all") == 0) {
        int restore_ok = restore_all_managed_features(1);
        save_unattended_log();
        return restore_ok ? 0 : 1;
    }
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    window_class.lpszClassName = "eXPerience2KConfigWindow";
    if (!RegisterClassA(&window_class)) return 1;

    ZeroMemory(&applying_class, sizeof(applying_class));
    applying_class.lpfnWndProc = DefWindowProcA;
    applying_class.hInstance = instance;
    applying_class.hCursor = LoadCursor(NULL, IDC_WAIT);
    applying_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    applying_class.lpszClassName = APPLYING_WINDOW_CLASS;
    if (!RegisterClassA(&applying_class)) return 1;

    initial_rectangle.left = 0;
    initial_rectangle.top = 0;
    initial_rectangle.right = MAIN_CLIENT_WIDTH;
    initial_rectangle.bottom = MAIN_COMPACT_CLIENT_HEIGHT;
    /* Size the initial compact window exactly as before. The vertical scroll
       bar is hidden while all content fits and appears natively after the user
       makes the client area shorter. */
    AdjustWindowRectEx(&initial_rectangle, main_style & ~WS_VSCROLL, FALSE, 0);
    g_window = CreateWindowA(window_class.lpszClassName, APP_TITLE,
        main_style, CW_USEDEFAULT, CW_USEDEFAULT,
        initial_rectangle.right - initial_rectangle.left,
        initial_rectangle.bottom - initial_rectangle.top,
        NULL, NULL, instance, NULL);
    if (!g_window) return 1;
    ShowWindow(g_window, show);
    UpdateWindow(g_window);
    SetFocus(GetDlgItem(g_window, IDC_APPLY));
    if (!g_probe.administrator) {
        MessageBoxA(g_window,
            "eXPerience2K is running without administrator privileges. You can inspect the current configuration and use current-user features, but protected system assets and the login window cannot be changed.\n\nAdministrator access is checked only when you click Apply. To use protected features, close the program, right-click it, choose Run As, and select an account in the Administrators group.",
            "Limited account guidance", MB_OK | MB_ICONINFORMATION);
    }
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(g_window, &message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    return (int)message.wParam;
}
