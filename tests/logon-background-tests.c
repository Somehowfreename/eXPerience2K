/* Isolated integration tests: predefined registry roots are redirected to a
   throwaway HKCU test tree for this process. Never apply actual host settings. */
#define WinMain e2k_application_entry_not_invoked
#include "../src/eXPerience2KConfig.c"
#undef WinMain

static int failures;
#define CHECK(condition, label) do { \
    if (!(condition)) { printf("FAIL: %s\n", label); ++failures; } \
    else printf("PASS: %s\n", label); \
} while (0)

static int default_equals(const char *key, const char *name, const char *value, DWORD expected_type)
{
    char data[1024] = {0};
    DWORD type = 0, length = sizeof(data);
    return read_default_user_value(key, name, &type, (BYTE *)data, &length) &&
           type == expected_type && length == strlen(value) + 1 && !strcmp(data, value);
}

static int state_test(const WCHAR *directory, const WCHAR *source)
{
    HKEY real_user = NULL, test_root = NULL, roots[3] = {NULL, NULL, NULL};
    const HKEY predefined[3] = {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_USERS};
    const char *names[3] = {"User", "Machine", "Users"};
    char registry_path[160], assets[MAX_PATH], path[MAX_PATH];
    WCHAR temporary[MAX_PATH];
    DWORD size, type, preset;
    BYTE data[64];
    int index, redirected = 0;
    _snprintf(registry_path, sizeof(registry_path),
              "Software\\eXPerience2KTests\\LogonBackground-%lu", (unsigned long)GetCurrentProcessId());
    if (RegOpenCurrentUser(KEY_ALL_ACCESS, &real_user) != ERROR_SUCCESS ||
        RegCreateKeyExA(real_user, registry_path, 0, NULL, 0, KEY_ALL_ACCESS,
                        NULL, &test_root, NULL) != ERROR_SUCCESS) return 2;
    for (index = 0; index < 3; ++index) {
        if (RegCreateKeyExA(test_root, names[index], 0, NULL, 0, KEY_ALL_ACCESS,
                            NULL, &roots[index], NULL) != ERROR_SUCCESS) goto cleanup;
    }
    for (index = 0; index < 3; ++index) {
        if (RegOverridePredefKey(predefined[index], roots[index]) != ERROR_SUCCESS) goto cleanup;
        ++redirected;
    }
    g_instance = GetModuleHandleA(NULL);
    g_use_current_user_fallback = 1;
    g_cross_user = 0;
    if (!WideCharToMultiByte(CP_ACP, 0, directory, -1, g_install_root,
                             MAX_PATH, NULL, NULL)) goto cleanup;
    join_path(assets, sizeof(assets), g_install_root, "Assets");
    SHCreateDirectoryExA(NULL, assets, NULL);
    {
        BYTE preferences[] = {0x9e, 0x3e, 0x07, 0x80, 0x55};
        set_menu_preference_bits(preferences, TRUE, FALSE);
        CHECK(preferences[0] == 0x9e && preferences[1] == 0x3c &&
              preferences[2] == 0x07 && preferences[3] == 0x80 && preferences[4] == 0x55,
              "Run As sliding updates only menu preference bits");
        set_menu_preference_bits(preferences, FALSE, TRUE);
        CHECK(preferences[0] == 0x9c && preferences[1] == 0x3e && preferences[4] == 0x55,
              "Run As animation and fade bits remain independent");
    }
    {
        HANDLE retained_state;
        join_path(path, sizeof(path), g_install_root, "state.tsv");
        retained_state = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
        CHECK(retained_state != INVALID_HANDLE_VALUE, "create retained-backup fixture");
        if (retained_state != INVALID_HANDLE_VALUE) CloseHandle(retained_state);
        CHECK(!resource_conversion_detected(), "retained backups alone are not active conversion for a new user");
        CHECK(configure_resource_reloader(1) && resource_conversion_detected(),
              "new user detects the active installation reloader");
        write_machine_string("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                             "eXPerience2K Resource Reloader", "unrelated-command.exe");
        CHECK(!resource_conversion_detected(), "unrelated Run value is not an active conversion");
        delete_machine_value("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                             "eXPerience2K Resource Reloader");
        write_user_dword(CONFIG_KEY, "Configured", 1);
        write_user_dword(CONFIG_KEY, "ResourceConversionEnabled", 1);
        CHECK(resource_conversion_detected(), "existing transaction marker retains failed-restore recovery state");
        delete_user_value(CONFIG_KEY, "Configured");
        delete_user_value(CONFIG_KEY, "ResourceConversionEnabled");
        DeleteFileA(path);
    }
    {
        size_t preference;
        write_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", 1);
        write_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", 1);
        write_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "WebView", 0);
        CHECK(explorer_enablement_markers_detected(), "Explorer enablement does not require every initial preference");
        for (preference = 0; preference < sizeof(g_explorer_dwords) / sizeof(g_explorer_dwords[0]); ++preference) {
            const EXPLORER_DWORD_VALUE *item = &g_explorer_dwords[preference];
            if (strcmp(item->marker, "ExplorerWebView"))
                write_user_dword(item->subkey, item->name, item->value + 1);
        }
        for (preference = 0; preference < sizeof(g_explorer_strings) / sizeof(g_explorer_strings[0]); ++preference) {
            const EXPLORER_STRING_VALUE *item = &g_explorer_strings[preference];
            write_user_string(item->subkey, item->name, "user preference");
        }
        CHECK(explorer_enablement_markers_detected(), "native folder and toolbar choices do not disable Explorer integration");
        write_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "WebView", 1);
        CHECK(!explorer_enablement_markers_detected(), "Common Tasks remains a structural layout conflict");
        write_user_dword("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "WebView", 0);
        write_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", 0);
        CHECK(!explorer_enablement_markers_detected(), "disabled machine integration is not reported enabled");
        write_machine_dword(EXPLORER_MACHINE_STATE_KEY, "Enabled", 1);
        write_user_dword(CONFIG_KEY, "ExplorerExperimentEnabled", 0);
        CHECK(!explorer_enablement_markers_detected(), "disabled user integration is not reported enabled");
        delete_user_value(CONFIG_KEY, "ExplorerExperimentEnabled");
        delete_machine_value(EXPLORER_MACHINE_STATE_KEY, "Enabled");
    }
    g_features[FEATURE_CLASSIC_LOGON].checkbox = CreateWindowA("BUTTON", "",
        WS_POPUP | BS_AUTOCHECKBOX, 0, 0, 0, 0, NULL, NULL, g_instance, NULL);
    g_logon_background_combo = CreateWindowA("COMBOBOX", "",
        WS_POPUP | CBS_DROPDOWNLIST, 0, 0, 0, 0, NULL, NULL, g_instance, NULL);
    for (index = 0; index < 3; ++index) SendMessageA(g_logon_background_combo, CB_ADDSTRING, 0, (LPARAM)names[index]);
    Button_SetCheck(g_features[FEATURE_CLASSIC_LOGON].checkbox, BST_CHECKED);
    SendMessageA(g_logon_background_combo, CB_SETCURSEL, LOGON_BACKGROUND_BLUE, 0);

    write_user_string("Control Panel\\Desktop", "Wallpaper", "signed-in-desktop-untouched.bmp");
    write_default_user_value("Control Panel\\Desktop", "Wallpaper", REG_EXPAND_SZ,
        (const BYTE *)"%SystemRoot%\\original.bmp", sizeof("%SystemRoot%\\original.bmp"));
    write_default_user_value("Control Panel\\Desktop", "TileWallpaper", REG_SZ, (const BYTE *)"7", 2);
    write_default_user_value("Control Panel\\Desktop", "Pattern", REG_SZ, (const BYTE *)"original pattern", sizeof("original pattern"));
    write_default_user_value("Control Panel\\Colors", "Background", REG_SZ, (const BYTE *)"11 22 33", sizeof("11 22 33"));
    CHECK(capture_exact_machine_baseline(), "capture immutable logon baseline");
    CHECK(prepare_logon_background(), "existing 3.0 blue asset is ready");
    CHECK(apply_default_w2k_appearance(1, CAPTION_PRESET_SOLID_NAVY), "apply blue with solid caption");
    CHECK(default_equals("Control Panel\\Colors", "Background", "58 110 165", REG_SZ), "exact current blue RGB");
    CHECK(default_equals("Control Panel\\Desktop", "TileWallpaper", "1", REG_SZ), "blue bitmap tiles");

    /* Simulate upgrade: schema 1 and the existing first-Apply baseline remain. */
    CHECK(capture_exact_machine_baseline(), "3.0 baseline reused without recapture");
    SendMessageA(g_logon_background_combo, CB_SETCURSEL, LOGON_BACKGROUND_TEAL, 0);
    CHECK(prepare_logon_background(), "generate persistent teal BMP");
    CHECK(apply_default_w2k_appearance(1, CAPTION_PRESET_BLUE_GRADIENT), "apply teal with independent gradient caption");
    CHECK(default_equals("Control Panel\\Colors", "Background", "0 128 128", REG_SZ), "exact Windows 95 teal RGB");
    CHECK(default_equals("Control Panel\\Colors", "ActiveTitle", "10 36 106", REG_SZ), "caption preset remains independent");
    CHECK(detected_logon_background() == LOGON_BACKGROUND_TEAL, "reopen detects teal");

    SendMessageA(g_logon_background_combo, CB_SETCURSEL, LOGON_BACKGROUND_CUSTOM, 0);
    CHECK(!prepare_logon_background(), "missing custom image fails before settings change");
    CHECK(default_equals("Control Panel\\Colors", "Background", "0 128 128", REG_SZ), "failure preserves applied background");
    lstrcpynW(temporary, directory, MAX_PATH);
    lstrcatW(temporary, L"\\prepared.tmp");
    CHECK(e2k_convert_logon_image(source, temporary, 800, 600), "decode custom input to compatible BMP");
    lstrcpynW(g_pending_logon_image, temporary, MAX_PATH);
    CHECK(prepare_logon_background(), "copy custom image outside user profile");
    CHECK(g_pending_logon_image[0] == 0 && GetFileAttributesW(temporary) == INVALID_FILE_ATTRIBUTES,
          "temporary image cleaned after installation");
    CHECK(apply_default_w2k_appearance(1, CAPTION_PRESET_SOLID_NAVY), "apply custom image");
    logon_background_path(LOGON_BACKGROUND_CUSTOM, path, sizeof(path));
    CHECK(default_equals("Control Panel\\Desktop", "Wallpaper", path, REG_SZ), "secure desktop references managed BMP");
    CHECK(default_equals("Control Panel\\Desktop", "TileWallpaper", "0", REG_SZ), "custom image is not tiled");
    CHECK(default_equals("Control Panel\\Desktop", "WallpaperStyle", "0", REG_SZ), "custom image is centered without distortion");
    CHECK(detected_logon_background() == LOGON_BACKGROUND_CUSTOM, "reopen detects custom image");
    CHECK(read_user_dword(CONFIG_KEY, "LogonBackgroundPreset", &preset) && preset == LOGON_BACKGROUND_CUSTOM,
          "custom selection persists");
    CHECK(prepare_logon_background(), "reapply uses installed BMP without original source");
    CHECK(user_string_equals("Control Panel\\Desktop", "Wallpaper", "signed-in-desktop-untouched.bmp"),
          "signed-in desktop wallpaper unchanged");
    CHECK(apply_default_w2k_appearance(0, CAPTION_PRESET_SOLID_NAVY), "clear classic logon restores original settings");
    CHECK(default_equals("Control Panel\\Desktop", "Wallpaper", "%SystemRoot%\\original.bmp", REG_EXPAND_SZ),
          "restore exact original wallpaper data and REG_EXPAND_SZ type");
    CHECK(default_equals("Control Panel\\Desktop", "TileWallpaper", "7", REG_SZ), "restore original tiling rather than guessed default");
    CHECK(default_equals("Control Panel\\Desktop", "Pattern", "original pattern", REG_SZ), "restore original pattern");
    CHECK(default_equals("Control Panel\\Colors", "Background", "11 22 33", REG_SZ), "restore original background color");
    size = sizeof(data); type = 0;
    CHECK(!read_default_user_value("Control Panel\\Desktop", "WallpaperStyle", &type, data, &size),
          "restore original value absence");
    CHECK(capture_exact_machine_baseline() && apply_default_w2k_appearance(1, CAPTION_PRESET_SOLID_NAVY) &&
          apply_default_w2k_appearance(0, CAPTION_PRESET_SOLID_NAVY) &&
          default_equals("Control Panel\\Desktop", "Wallpaper", "%SystemRoot%\\original.bmp", REG_EXPAND_SZ),
          "repeat apply/revert retains immutable baseline");
    CHECK(strstr(g_log, g_install_root) == NULL && strstr(g_log, "original.bmp") == NULL,
          "diagnostics omit image and profile paths");
    DestroyWindow(g_logon_background_combo);
    DestroyWindow(g_features[FEATURE_CLASSIC_LOGON].checkbox);
cleanup:
    for (index = 0; index < redirected; ++index) RegOverridePredefKey(predefined[index], NULL);
    for (index = 0; index < 3; ++index) if (roots[index]) RegCloseKey(roots[index]);
    if (test_root) RegCloseKey(test_root);
    if (real_user) { SHDeleteKeyA(real_user, registry_path); RegCloseKey(real_user); }
    if (redirected != 3) return 2;
    return failures ? 1 : 0;
}

int wmain(int argc, WCHAR **argv)
{
    if (argc == 6 && !lstrcmpW(argv[1], L"convert"))
        return e2k_convert_logon_image(argv[2], argv[3], (UINT)_wtoi(argv[4]), (UINT)_wtoi(argv[5])) ? 0 : 1;
    if (argc == 4 && !lstrcmpW(argv[1], L"state")) return state_test(argv[2], argv[3]);
    return 2;
}
