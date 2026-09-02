/* Read-only guest lifecycle evidence. Including the production declarations
   keeps the setting list aligned with the exact candidate being tested.
   No application entry point, Apply, capture, or restore function is run. */
#define WinMain e2k_unused_application_entry
#include "../src/eXPerience2KConfig.c"
#undef WinMain

static FILE *snapshot;
static unsigned entries, errors;
static REGSAM machine_view;

static void blob(const char *scope, const char *key, const char *name,
                 DWORD type, const BYTE *data, DWORD size)
{
    DWORD i;
    fprintf(snapshot, "%s|%s|%s|%lu|", scope, key, name, (unsigned long)type);
    for (i = 0; i < size; ++i) fprintf(snapshot, "%02X", data[i]);
    fputc('\n', snapshot);
    ++entries;
}

static void value(HKEY root, const char *scope, const char *key, const char *name)
{
    HKEY handle;
    BYTE data[8192];
    DWORD type = 0, size = sizeof(data);
    LONG status = RegOpenKeyExA(root, key, 0,
        KEY_QUERY_VALUE | (root == HKEY_LOCAL_MACHINE ? machine_view : 0), &handle);
    if (status == ERROR_SUCCESS) {
        status = RegQueryValueExA(handle, name, NULL, &type, data, &size);
        RegCloseKey(handle);
    }
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
        blob(scope, key, name, 0xffffffffU, NULL, 0);
    else if (status == ERROR_SUCCESS) blob(scope, key, name, type, data, size);
    else { fprintf(snapshot, "ERROR|%s|%s|%s|%ld\n", scope, key, name, status); ++errors; }
}

int main(int argc, char **argv)
{
    HKEY logon;
    SYSTEM_INFO system;
    NONCLIENTMETRICSA metrics;
    ICONMETRICSA icons;
    size_t i, branch;
    const char *desktop[] = {"FontSmoothing", "UserPreferencesMask", "Wallpaper",
                            "TileWallpaper", "WallpaperStyle", "Pattern"};
    const char *theme = "Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";
    const char *explorer = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";
    const char *policy = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
    if (argc != 2 || !(snapshot = fopen(argv[1], "wb"))) return 2;
    GetNativeSystemInfo(&system);
    machine_view = system.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? KEY_WOW64_64KEY : 0;
    if (RegOpenKeyExA(HKEY_USERS, ".DEFAULT", 0, KEY_READ, &logon) != ERROR_SUCCESS) return 3;
    for (i = 0; i < 2; ++i) {
        HKEY root = i ? logon : HKEY_CURRENT_USER;
        const char *scope = i ? "Logon" : "User";
        size_t n;
        value(root, scope, theme, "ThemeActive");
        for (n = 0; n < sizeof(desktop)/sizeof(desktop[0]); ++n)
            value(root, scope, "Control Panel\\Desktop", desktop[n]);
        for (n = 0; n < sizeof(g_w2k_colors)/sizeof(g_w2k_colors[0]); ++n)
            value(root, scope, "Control Panel\\Colors", g_w2k_colors[n].name);
        for (n = 0; n < sizeof(g_w2k_metrics)/sizeof(g_w2k_metrics[0]); ++n)
            value(root, scope, "Control Panel\\Desktop\\WindowMetrics", g_w2k_metrics[n].name);
        for (n = 0; n < sizeof(g_w2k_font_values)/sizeof(g_w2k_font_values[0]); ++n)
            value(root, scope, "Control Panel\\Desktop\\WindowMetrics", g_w2k_font_values[n]);
    }
    RegCloseKey(logon);
    value(HKEY_CURRENT_USER, "User", explorer, "ShellState");
    value(HKEY_CURRENT_USER, "User", policy, "NoSimpleStartMenu");
    value(HKEY_CURRENT_USER, "User", policy, "ForceClassicControlPanel");
    value(HKEY_CURRENT_USER, "User", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel", "StartupPage");
    for (i = 0; i < sizeof(g_explorer_dwords)/sizeof(g_explorer_dwords[0]); ++i)
        value(HKEY_CURRENT_USER, "User", g_explorer_dwords[i].subkey, g_explorer_dwords[i].name);
    for (i = 0; i < sizeof(g_explorer_strings)/sizeof(g_explorer_strings[0]); ++i)
        value(HKEY_CURRENT_USER, "User", g_explorer_strings[i].subkey, g_explorer_strings[i].name);
    for (i = 0; i < sizeof(g_explorer_binaries)/sizeof(g_explorer_binaries[0]); ++i)
        value(HKEY_CURRENT_USER, "User", g_explorer_binaries[i].subkey, g_explorer_binaries[i].name);
    for (i = 0; i <= sizeof(g_windows_sound_events)/sizeof(g_windows_sound_events[0]); ++i) {
        const WINDOWS_SOUND_EVENT *event = i == sizeof(g_windows_sound_events)/sizeof(g_windows_sound_events[0])
            ? &g_windows_2000_double_click_sound : &g_windows_sound_events[i];
        const char *branches[] = {"Current", "Default"};
        for (branch = 0; branch < 2; ++branch) {
            char key[512];
            if (!build_sound_event_key(event, branches[branch], key, sizeof(key))) return 4;
            value(HKEY_CURRENT_USER, "User", key, "");
        }
    }
    value(HKEY_LOCAL_MACHINE, "Machine", "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "LogonType");
    value(HKEY_LOCAL_MACHINE, "Machine", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "eXPerience2K Resource Reloader");
    for (i = 0; i < sizeof(g_explorer_machine_text)/sizeof(g_explorer_machine_text[0]); ++i)
        value(HKEY_LOCAL_MACHINE, "Machine", g_explorer_machine_text[i].subkey, g_explorer_machine_text[i].name);
    ZeroMemory(&metrics, sizeof(metrics)); metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
        blob("Runtime", "Metrics", "NonClient", REG_BINARY, (BYTE *)&metrics, sizeof(metrics));
    else ++errors;
    ZeroMemory(&icons, sizeof(icons)); icons.cbSize = sizeof(icons);
    if (SystemParametersInfoA(SPI_GETICONMETRICS, sizeof(icons), &icons, 0))
        blob("Runtime", "Metrics", "Icons", REG_BINARY, (BYTE *)&icons, sizeof(icons));
    else ++errors;
    fclose(snapshot);
    printf("Captured %u managed values/runtime structures; %u read errors.\n", entries, errors);
    return errors ? 1 : 0;
}
