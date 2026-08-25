#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_OPERATIONS 768
#define MAX_LANGUAGES 32
#define MAX_PATH_TEXT 4096
#define MAX_STATE_RECORDS 512

#ifndef TH32CS_SNAPMODULE32
#define TH32CS_SNAPMODULE32 0x00000010
#endif

#ifndef MOVEFILE_REPLACE_EXISTING
#define MOVEFILE_REPLACE_EXISTING 0x00000001
#endif

#ifndef MOVEFILE_DELAY_UNTIL_REBOOT
#define MOVEFILE_DELAY_UNTIL_REBOOT 0x00000004
#endif

#ifndef SM_TABLETPC
#define SM_TABLETPC 86
#endif

#ifndef SM_MEDIACENTER
#define SM_MEDIACENTER 87
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

#pragma pack(push, 1)
typedef struct {
    WORD reserved;
    WORD type;
    WORD count;
} ICON_FILE_HEADER;

typedef struct {
    BYTE width;
    BYTE height;
    BYTE color_count;
    BYTE reserved;
    WORD planes;
    WORD bit_count;
    DWORD bytes_in_resource;
    DWORD image_offset;
} ICON_FILE_ENTRY;

typedef struct {
    BYTE width;
    BYTE height;
    BYTE color_count;
    BYTE reserved;
    WORD planes;
    WORD bit_count;
    DWORD bytes_in_resource;
    WORD resource_id;
} GROUP_ICON_ENTRY;
#pragma pack(pop)

typedef enum {
    OP_ICON_GROUP,
    OP_BITMAP,
    OP_AVI,
    OP_DIALOG,
    OP_STRING
} OPERATION_TYPE;

typedef struct {
    char asset[MAX_PATH_TEXT];
    char resource_name[256];
    OPERATION_TYPE type;
    LANGID languages[MAX_LANGUAGES];
    unsigned language_count;
    WORD first_icon_id;
    BYTE *string_blocks[MAX_LANGUAGES];
    DWORD string_block_sizes[MAX_LANGUAGES];
} RESOURCE_OPERATION;

typedef struct {
    LANGID values[MAX_LANGUAGES];
    unsigned count;
} LANGUAGE_LIST;

typedef struct {
    WORD maximum;
} ICON_ID_STATE;

typedef struct {
    char id[64];
    char variant[64];
    char script[260];
    char target[MAX_PATH_TEXT];
    char backup[MAX_PATH_TEXT];
    DWORD original_crc;
    DWORD patched_crc;
    DWORD original_size;
    DWORD patched_size;
} STATE_RECORD;

typedef struct {
    DWORD major;
    DWORD minor;
    DWORD build;
    WORD service_pack;
    WORD suite_mask;
    BYTE product_type_value;
    char architecture[16];
    char product_type[32];
    char product_name[256];
    int server_r2;
    int tablet_pc;
    int media_center;
    int administrator;
    int supported;
    int resource_profile_ready;
    char profile_id[64];
    char display_name[160];
    char branding_id[64];
    char branding_label[160];
    char support_reason[256];
} DETECTED_OS;

static char g_branding_id[64];

static void print_win32_error(const char *context)
{
    DWORD error = GetLastError();
    char *message = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        0,
        (LPSTR)&message,
        0,
        NULL);
    fprintf(stderr, "%s failed (Win32 %lu)%s%s\n", context, (unsigned long)error,
            message ? ": " : "", message ? message : "");
    if (message) LocalFree(message);
}

static char *trim(char *text)
{
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    if (text[0] == '"' && end > text + 1 && end[-1] == '"') {
        ++text;
        end[-1] = '\0';
    }
    return text;
}

static int equals_ignore_case(const char *left, const char *right)
{
    return lstrcmpiA(left, right) == 0;
}

static int join_path(char *output, size_t output_size, const char *left, const char *right)
{
    size_t left_length = strlen(left);
    const char *separator = (left_length && left[left_length - 1] != '\\' && left[left_length - 1] != '/') ? "\\" : "";
    int written = _snprintf(output, output_size, "%s%s%s", left, separator, right);
    if (written < 0 || (size_t)written >= output_size) {
        fprintf(stderr, "Path is too long: %s + %s\n", left, right);
        return 0;
    }
    return 1;
}

static int path_is_regular_file(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static int resolve_asset_path(char *output, size_t output_size, const char *install_root,
                              const char *asset)
{
    static const char base_prefix[] = "Resources\\eXPerience2K\\";
    char branding_root[MAX_PATH_TEXT], branding_profile[MAX_PATH_TEXT];
    char override[MAX_PATH_TEXT];
    if (g_branding_id[0] &&
        _strnicmp(asset, base_prefix, sizeof(base_prefix) - 1) == 0 &&
        join_path(branding_root, sizeof(branding_root), install_root, "Resources\\Branding") &&
        join_path(branding_profile, sizeof(branding_profile), branding_root, g_branding_id) &&
        join_path(override, sizeof(override), branding_profile,
                  asset + (sizeof(base_prefix) - 1)) &&
        path_is_regular_file(override)) {
        lstrcpynA(output, override, (int)output_size);
        return 1;
    }
    return join_path(output, output_size, install_root, asset);
}

static int read_entire_file(const char *path, BYTE **data, DWORD *size)
{
    HANDLE file;
    DWORD high = 0, low, read = 0;
    BYTE *buffer;
    *data = NULL;
    *size = 0;
    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        print_win32_error(path);
        return 0;
    }
    low = GetFileSize(file, &high);
    if (low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        print_win32_error("GetFileSize");
        CloseHandle(file);
        return 0;
    }
    if (high || low > 0x7FFFFFFFUL) {
        fprintf(stderr, "Resource file is too large: %s\n", path);
        CloseHandle(file);
        return 0;
    }
    buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, low ? low : 1);
    if (!buffer) {
        fprintf(stderr, "Out of memory reading %s\n", path);
        CloseHandle(file);
        return 0;
    }
    if (low && (!ReadFile(file, buffer, low, &read, NULL) || read != low)) {
        print_win32_error("ReadFile");
        HeapFree(GetProcessHeap(), 0, buffer);
        CloseHandle(file);
        return 0;
    }
    CloseHandle(file);
    *data = buffer;
    *size = low;
    return 1;
}

static LPSTR resource_identifier(const char *text)
{
    char *end = NULL;
    unsigned long value;
    if (!text || !*text) return NULL;
    value = strtoul(text, &end, 10);
    if (end && *end == '\0' && value > 0 && value <= 0xFFFFUL)
        return MAKEINTRESOURCEA((WORD)value);
    return (LPSTR)text;
}

static BOOL CALLBACK collect_language(HMODULE module, LPCSTR type, LPCSTR name, WORD language, LONG_PTR parameter)
{
    LANGUAGE_LIST *list = (LANGUAGE_LIST *)parameter;
    (void)module;
    (void)type;
    (void)name;
    if (list->count < MAX_LANGUAGES)
        list->values[list->count++] = language;
    return TRUE;
}

static BOOL CALLBACK collect_icon_id(HMODULE module, LPCSTR type, LPSTR name, LONG_PTR parameter)
{
    ICON_ID_STATE *state = (ICON_ID_STATE *)parameter;
    (void)module;
    (void)type;
    if (IS_INTRESOURCE(name) && (WORD)(ULONG_PTR)name > state->maximum)
        state->maximum = (WORD)(ULONG_PTR)name;
    return TRUE;
}

static void discover_languages(HMODULE module, LPCSTR type, LPSTR name, RESOURCE_OPERATION *operation)
{
    LANGUAGE_LIST list;
    ZeroMemory(&list, sizeof(list));
    if (!EnumResourceLanguagesA(module, type, name, collect_language, (LONG_PTR)&list) || !list.count) {
        operation->languages[0] = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        operation->language_count = 1;
        return;
    }
    operation->language_count = list.count;
    CopyMemory(operation->languages, list.values, list.count * sizeof(LANGID));
}

static int parse_command(char *line, const char *install_root, RESOURCE_OPERATION *operation)
{
    char *first, *second, *third;
    char *asset, *type, *identifier;
    char full_asset[MAX_PATH_TEXT];
    line = trim(line);
    if (_strnicmp(line, "-addoverwrite", 13) != 0) return 0;
    line = trim(line + 13);
    first = strchr(line, ',');
    if (!first) return -1;
    *first++ = '\0';
    second = strchr(first, ',');
    if (!second) return -1;
    *second++ = '\0';
    third = strchr(second, ',');
    if (third) *third = '\0';
    asset = trim(line);
    type = trim(first);
    identifier = trim(second);
    if (!resolve_asset_path(full_asset, sizeof(full_asset), install_root, asset)) return -1;
    ZeroMemory(operation, sizeof(*operation));
    lstrcpynA(operation->asset, full_asset, sizeof(operation->asset));
    lstrcpynA(operation->resource_name, identifier, sizeof(operation->resource_name));
    if (equals_ignore_case(type, "ICONGROUP")) operation->type = OP_ICON_GROUP;
    else if (equals_ignore_case(type, "BITMAP")) operation->type = OP_BITMAP;
    else if (equals_ignore_case(type, "AVI")) operation->type = OP_AVI;
    else if (equals_ignore_case(type, "DIALOG")) operation->type = OP_DIALOG;
    else if (equals_ignore_case(type, "STRING")) operation->type = OP_STRING;
    else {
        fprintf(stderr, "Unsupported resource type: %s\n", type);
        return -1;
    }
    return 1;
}

static int parse_script(const char *script_path, const char *install_root,
                        RESOURCE_OPERATION *operations, unsigned *operation_count)
{
    FILE *file;
    char line[8192];
    int in_commands = 0;
    *operation_count = 0;
    file = fopen(script_path, "rb");
    if (!file) {
        fprintf(stderr, "Cannot open script: %s\n", script_path);
        return 0;
    }
    while (fgets(line, sizeof(line), file)) {
        char *value = trim(line);
        int parsed;
        if (equals_ignore_case(value, "[COMMANDS]")) {
            in_commands = 1;
            continue;
        }
        if (*value == '[') {
            in_commands = 0;
            continue;
        }
        if (!in_commands || !*value || *value == ';' || *value == '#') continue;
        if (*operation_count >= MAX_OPERATIONS) {
            fprintf(stderr, "Too many operations in %s\n", script_path);
            fclose(file);
            return 0;
        }
        parsed = parse_command(value, install_root, &operations[*operation_count]);
        if (parsed < 0) {
            fprintf(stderr, "Malformed command in %s: %s\n", script_path, value);
            fclose(file);
            return 0;
        }
        if (parsed > 0) ++*operation_count;
    }
    fclose(file);
    return 1;
}

static int prepare_operations(const char *target, RESOURCE_OPERATION *operations, unsigned count)
{
    HMODULE module;
    ICON_ID_STATE icon_state;
    unsigned index;
    module = LoadLibraryExA(target, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!module) {
        print_win32_error("LoadLibraryEx");
        return 0;
    }
    ZeroMemory(&icon_state, sizeof(icon_state));
    EnumResourceNamesA(module, RT_ICON, collect_icon_id, (LONG_PTR)&icon_state);
    for (index = 0; index < count; ++index) {
        RESOURCE_OPERATION *operation = &operations[index];
        LPCSTR type;
        LPSTR name;
        WORD string_id = 0, string_block = 0;
        if (operation->type == OP_STRING) {
            char *end = NULL;
            unsigned long parsed = strtoul(operation->resource_name, &end, 10);
            if (!end || *end || parsed > 0xffffUL) {
                fprintf(stderr, "STRING operations require a numeric string ID: %s\n",
                        operation->resource_name);
                FreeLibrary(module);
                return 0;
            }
            string_id = (WORD)parsed;
            string_block = (WORD)(string_id / 16 + 1);
            type = RT_STRING;
            name = MAKEINTRESOURCEA(string_block);
        } else {
            type = operation->type == OP_ICON_GROUP ? RT_GROUP_ICON :
                   operation->type == OP_BITMAP ? RT_BITMAP :
                   operation->type == OP_DIALOG ? RT_DIALOG : "AVI";
            name = resource_identifier(operation->resource_name);
        }
        discover_languages(module, type, name, operation);
        if (operation->type == OP_STRING) {
            unsigned language_index;
            for (language_index = 0; language_index < operation->language_count;
                 ++language_index) {
                HRSRC resource = FindResourceExA(module, RT_STRING,
                    MAKEINTRESOURCEA(string_block), operation->languages[language_index]);
                HGLOBAL loaded;
                const void *locked;
                DWORD size;
                if (!resource || !(size = SizeofResource(module, resource)) ||
                    !(loaded = LoadResource(module, resource)) ||
                    !(locked = LockResource(loaded))) {
                    fprintf(stderr, "Cannot load STRINGTABLE block %u, language %u.\n",
                            (unsigned)string_block,
                            (unsigned)operation->languages[language_index]);
                    FreeLibrary(module);
                    return 0;
                }
                operation->string_blocks[language_index] =
                    (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
                if (!operation->string_blocks[language_index]) {
                    FreeLibrary(module);
                    return 0;
                }
                CopyMemory(operation->string_blocks[language_index], locked, size);
                operation->string_block_sizes[language_index] = size;
            }
        }
        if (operation->type == OP_ICON_GROUP) {
            BYTE *data = NULL;
            DWORD size = 0;
            ICON_FILE_HEADER *header;
            if (!read_entire_file(operation->asset, &data, &size)) {
                FreeLibrary(module);
                return 0;
            }
            if (size < sizeof(ICON_FILE_HEADER)) {
                fprintf(stderr, "Invalid ICO file: %s\n", operation->asset);
                HeapFree(GetProcessHeap(), 0, data);
                FreeLibrary(module);
                return 0;
            }
            header = (ICON_FILE_HEADER *)data;
            if (header->reserved != 0 || header->type != 1 || !header->count ||
                size < sizeof(ICON_FILE_HEADER) + header->count * sizeof(ICON_FILE_ENTRY) ||
                (unsigned long)icon_state.maximum + header->count > 0xFFFFUL) {
                fprintf(stderr, "Unsupported ICO structure: %s\n", operation->asset);
                HeapFree(GetProcessHeap(), 0, data);
                FreeLibrary(module);
                return 0;
            }
            operation->first_icon_id = (WORD)(icon_state.maximum + 1);
            icon_state.maximum = (WORD)(icon_state.maximum + header->count);
            HeapFree(GetProcessHeap(), 0, data);
        }
    }
    FreeLibrary(module);
    return 1;
}

static int apply_icon_group(HANDLE update, const RESOURCE_OPERATION *operation)
{
    BYTE *data = NULL, *group_data = NULL;
    DWORD size = 0, group_size;
    ICON_FILE_HEADER *header;
    ICON_FILE_ENTRY *entries;
    GROUP_ICON_ENTRY *group_entries;
    unsigned image, language_index;
    LPSTR group_name = resource_identifier(operation->resource_name);
    if (!read_entire_file(operation->asset, &data, &size)) return 0;
    header = (ICON_FILE_HEADER *)data;
    entries = (ICON_FILE_ENTRY *)(data + sizeof(ICON_FILE_HEADER));
    group_size = sizeof(ICON_FILE_HEADER) + header->count * sizeof(GROUP_ICON_ENTRY);
    group_data = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, group_size);
    if (!group_data) {
        HeapFree(GetProcessHeap(), 0, data);
        return 0;
    }
    CopyMemory(group_data, header, sizeof(ICON_FILE_HEADER));
    group_entries = (GROUP_ICON_ENTRY *)(group_data + sizeof(ICON_FILE_HEADER));
    for (image = 0; image < header->count; ++image) {
        WORD id = (WORD)(operation->first_icon_id + image);
        if (entries[image].image_offset > size || entries[image].bytes_in_resource > size - entries[image].image_offset) {
            fprintf(stderr, "ICO image extends beyond file: %s\n", operation->asset);
            HeapFree(GetProcessHeap(), 0, group_data);
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
        group_entries[image].width = entries[image].width;
        group_entries[image].height = entries[image].height;
        group_entries[image].color_count = entries[image].color_count;
        group_entries[image].reserved = entries[image].reserved;
        group_entries[image].planes = entries[image].planes;
        group_entries[image].bit_count = entries[image].bit_count;
        group_entries[image].bytes_in_resource = entries[image].bytes_in_resource;
        group_entries[image].resource_id = id;
        for (language_index = 0; language_index < operation->language_count; ++language_index) {
            if (!UpdateResourceA(update, RT_ICON, MAKEINTRESOURCEA(id), operation->languages[language_index],
                                 data + entries[image].image_offset, entries[image].bytes_in_resource)) {
                print_win32_error("UpdateResource(RT_ICON)");
                HeapFree(GetProcessHeap(), 0, group_data);
                HeapFree(GetProcessHeap(), 0, data);
                return 0;
            }
        }
    }
    for (language_index = 0; language_index < operation->language_count; ++language_index) {
        if (!UpdateResourceA(update, RT_GROUP_ICON, group_name, operation->languages[language_index], group_data, group_size)) {
            print_win32_error("UpdateResource(RT_GROUP_ICON)");
            HeapFree(GetProcessHeap(), 0, group_data);
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
    }
    HeapFree(GetProcessHeap(), 0, group_data);
    HeapFree(GetProcessHeap(), 0, data);
    return 1;
}

static int apply_raw_resource(HANDLE update, const RESOURCE_OPERATION *operation)
{
    BYTE *data = NULL, *resource_data;
    DWORD size = 0, resource_size;
    LPCSTR type;
    LPSTR name = resource_identifier(operation->resource_name);
    unsigned language_index;
    if (!read_entire_file(operation->asset, &data, &size)) return 0;
    if (operation->type == OP_BITMAP) {
        if (size < 14 || data[0] != 'B' || data[1] != 'M') {
            fprintf(stderr, "Invalid bitmap: %s\n", operation->asset);
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
        type = RT_BITMAP;
        resource_data = data + 14;
        resource_size = size - 14;
    } else if (operation->type == OP_DIALOG) {
        type = RT_DIALOG;
        resource_data = data;
        resource_size = size;
    } else {
        type = "AVI";
        resource_data = data;
        resource_size = size;
    }
    for (language_index = 0; language_index < operation->language_count; ++language_index) {
        if (!UpdateResourceA(update, type, name, operation->languages[language_index], resource_data, resource_size)) {
            print_win32_error("UpdateResource");
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }
    }
    HeapFree(GetProcessHeap(), 0, data);
    return 1;
}

static int string_operation_block_id(const RESOURCE_OPERATION *operation,
                                     WORD *block_id)
{
    char *end = NULL;
    unsigned long parsed_id;
    if (!operation || operation->type != OP_STRING || !block_id) return 0;
    parsed_id = strtoul(operation->resource_name, &end, 10);
    if (!end || *end || parsed_id > 0xffffUL) return 0;
    *block_id = (WORD)((WORD)parsed_id / 16 + 1);
    return 1;
}

static int read_string_asset(const RESOURCE_OPERATION *operation,
                             WCHAR **wide_result, WORD *wide_length_result)
{
    BYTE *file_data = NULL;
    DWORD file_size = 0;
    char *text = NULL, *value;
    WCHAR *wide = NULL;
    int wide_length;
    int result = 0;
    if (!wide_result || !wide_length_result) return 0;
    *wide_result = NULL;
    *wide_length_result = 0;
    if (!read_entire_file(operation->asset, &file_data, &file_size)) return 0;
    text = (char *)HeapAlloc(GetProcessHeap(), 0, file_size + 1);
    if (!text) goto done;
    CopyMemory(text, file_data, file_size);
    text[file_size] = '\0';
    value = trim(text);
    if ((size_t)(value - text) + 3 <= file_size &&
        (unsigned char)value[0] == 0xef && (unsigned char)value[1] == 0xbb &&
        (unsigned char)value[2] == 0xbf) value += 3;
    wide_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    if (wide_length <= 1) {
        fprintf(stderr, "Invalid or empty UTF-8 STRING asset: %s\n", operation->asset);
        goto done;
    }
    wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, wide_length * sizeof(WCHAR));
    if (!wide || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1,
                                      wide, wide_length)) goto done;
    --wide_length;
    if (wide_length > 0xffff) goto done;
    *wide_result = wide;
    *wide_length_result = (WORD)wide_length;
    wide = NULL;
    result = 1;
done:
    if (wide) HeapFree(GetProcessHeap(), 0, wide);
    if (text) HeapFree(GetProcessHeap(), 0, text);
    if (file_data) HeapFree(GetProcessHeap(), 0, file_data);
    return result;
}

static int replace_string_slot(BYTE **block_data, DWORD *block_size, WORD slot,
                               const WCHAR *wide, WORD wide_length)
{
    const BYTE *source;
    DWORD source_size, offset = 0, suffix_offset, output_size;
    WORD index, old_length;
    BYTE *output;
    if (!block_data || !*block_data || !block_size || !wide || slot >= 16)
        return 0;
    source = *block_data;
    source_size = *block_size;
    if (source_size < sizeof(WORD)) return 0;
    for (index = 0; index < slot; ++index) {
        WORD length;
        if (offset + sizeof(WORD) > source_size) return 0;
        CopyMemory(&length, source + offset, sizeof(length));
        offset += sizeof(WORD) + (DWORD)length * sizeof(WCHAR);
        if (offset > source_size) return 0;
    }
    if (offset + sizeof(WORD) > source_size) return 0;
    CopyMemory(&old_length, source + offset, sizeof(old_length));
    suffix_offset = offset + sizeof(WORD) + (DWORD)old_length * sizeof(WCHAR);
    if (suffix_offset > source_size) return 0;
    output_size = source_size - ((DWORD)old_length * sizeof(WCHAR)) +
                  ((DWORD)wide_length * sizeof(WCHAR));
    output = (BYTE *)HeapAlloc(GetProcessHeap(), 0, output_size);
    if (!output) return 0;
    CopyMemory(output, source, offset);
    CopyMemory(output + offset, &wide_length, sizeof(wide_length));
    CopyMemory(output + offset + sizeof(WORD), wide,
               (DWORD)wide_length * sizeof(WCHAR));
    CopyMemory(output + offset + sizeof(WORD) + (DWORD)wide_length * sizeof(WCHAR),
               source + suffix_offset, source_size - suffix_offset);
    HeapFree(GetProcessHeap(), 0, *block_data);
    *block_data = output;
    *block_size = output_size;
    return 1;
}

static int operation_language_index(const RESOURCE_OPERATION *operation,
                                    LANGID language)
{
    unsigned index;
    for (index = 0; index < operation->language_count; ++index) {
        if (operation->languages[index] == language) return (int)index;
    }
    return -1;
}

static int apply_string_block(HANDLE update, RESOURCE_OPERATION *operations,
                              unsigned operation_count, unsigned first_index)
{
    RESOURCE_OPERATION *first = &operations[first_index];
    WORD block_id;
    unsigned language_index;
    if (!string_operation_block_id(first, &block_id)) return 0;

    for (language_index = 0; language_index < first->language_count;
         ++language_index) {
        BYTE *output;
        DWORD output_size;
        unsigned operation_index;
        LANGID language = first->languages[language_index];
        if (!first->string_blocks[language_index] ||
            !first->string_block_sizes[language_index]) return 0;
        output_size = first->string_block_sizes[language_index];
        output = (BYTE *)HeapAlloc(GetProcessHeap(), 0, output_size);
        if (!output) return 0;
        CopyMemory(output, first->string_blocks[language_index], output_size);

        for (operation_index = first_index; operation_index < operation_count;
             ++operation_index) {
            RESOURCE_OPERATION *operation = &operations[operation_index];
            WORD operation_block, string_id, slot, replacement_length;
            WCHAR *replacement = NULL;
            char *end = NULL;
            unsigned long parsed_id;
            int matching_language;
            if (operation->type != OP_STRING ||
                !string_operation_block_id(operation, &operation_block) ||
                operation_block != block_id) continue;
            matching_language = operation_language_index(operation, language);
            if (matching_language < 0) {
                fprintf(stderr,
                        "STRINGTABLE block %u has inconsistent language coverage.\n",
                        (unsigned)block_id);
                HeapFree(GetProcessHeap(), 0, output);
                return 0;
            }
            parsed_id = strtoul(operation->resource_name, &end, 10);
            if (!end || *end || parsed_id > 0xffffUL ||
                !read_string_asset(operation, &replacement, &replacement_length)) {
                HeapFree(GetProcessHeap(), 0, output);
                return 0;
            }
            string_id = (WORD)parsed_id;
            slot = (WORD)(string_id % 16);
            if (!replace_string_slot(&output, &output_size, slot, replacement,
                                     replacement_length)) {
                HeapFree(GetProcessHeap(), 0, replacement);
                HeapFree(GetProcessHeap(), 0, output);
                return 0;
            }
            HeapFree(GetProcessHeap(), 0, replacement);
        }
        if (!UpdateResourceA(update, RT_STRING, MAKEINTRESOURCEA(block_id),
                             language, output, output_size)) {
            print_win32_error("UpdateResource(RT_STRING)");
            HeapFree(GetProcessHeap(), 0, output);
            return 0;
        }
        HeapFree(GetProcessHeap(), 0, output);
    }
    return 1;
}

static int string_block_already_applied(const RESOURCE_OPERATION *operations,
                                        unsigned index)
{
    WORD current_block;
    unsigned previous;
    if (!string_operation_block_id(&operations[index], &current_block)) return 0;
    for (previous = 0; previous < index; ++previous) {
        WORD previous_block;
        if (operations[previous].type == OP_STRING &&
            string_operation_block_id(&operations[previous], &previous_block) &&
            previous_block == current_block) return 1;
    }
    return 0;
}

static int regular_file_exists(const char *path);
static int make_sibling_path(char *output, size_t output_size, const char *target, const char *suffix);
static int ensure_directory_tree(const char *path);
static int crc32_file(const char *path, DWORD *result);
static WORD read_pe_machine(const char *path);

static int legacy_resource_hacker_patch(const char *input, const char *script,
                                        const char *install_root, const char *output)
{
    char tools[MAX_PATH_TEXT], executable[MAX_PATH_TEXT], new_files[MAX_PATH_TEXT];
    char helper_base[MAX_PATH_TEXT], temporary[MAX_PATH_TEXT];
    char helper_input[MAX_PATH_TEXT], helper_output[MAX_PATH_TEXT];
    char helper_log[MAX_PATH_TEXT], unique[256];
    char command[MAX_PATH_TEXT * 2];
    FILE *source = NULL, *destination = NULL;
    char line[8192];
    int commands = 0, result = 0;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DWORD exit_code = 1, wait_result, input_crc = 0, output_crc = 0;
    WORD input_machine, output_machine;
    if (!join_path(tools, sizeof(tools), install_root, "Tools") ||
        !join_path(executable, sizeof(executable), tools, "ResourceHacker.exe") ||
        !regular_file_exists(executable) ||
        !join_path(new_files, sizeof(new_files), install_root, "NewFiles") ||
        !ensure_directory_tree(new_files))
        return 0;
    _snprintf(unique, sizeof(unique), "rh-%lu-%lu", (unsigned long)GetCurrentProcessId(),
              (unsigned long)GetTickCount());
    if (!join_path(helper_base, sizeof(helper_base), new_files, unique) ||
        !make_sibling_path(helper_input, sizeof(helper_input), helper_base, ".input") ||
        !make_sibling_path(helper_output, sizeof(helper_output), helper_base, ".output") ||
        !make_sibling_path(helper_log, sizeof(helper_log), helper_base, ".log") ||
        !make_sibling_path(temporary, sizeof(temporary), helper_base, ".script.txt"))
        return 0;
    if (!CopyFileA(input, helper_input, FALSE)) {
        print_win32_error("CopyFile(Resource Hacker neutral input)");
        return 0;
    }
    source = fopen(script, "rb");
    destination = fopen(temporary, "wb");
    if (!source || !destination) goto done;
    fprintf(destination, "[FILENAMES]\r\nExe=\"%s\"\r\nSaveAs=\"%s\"\r\nLog=\"%s\"\r\n\r\n",
            helper_input, helper_output, helper_log);
    while (fgets(line, sizeof(line), source)) {
        char copy[8192];
        char *value, *comma, *asset;
        char absolute_asset[MAX_PATH_TEXT];
        lstrcpynA(copy, line, sizeof(copy));
        value = trim(copy);
        if (equals_ignore_case(value, "[COMMANDS]")) commands = 1;
        if (!commands) continue;
        if (_strnicmp(value, "-addoverwrite", 13) == 0 &&
            (comma = strchr(value + 13, ',')) != NULL) {
            *comma++ = '\0';
            asset = trim(value + 13);
            if (join_path(absolute_asset, sizeof(absolute_asset), install_root, asset))
                fprintf(destination, "-addoverwrite \"%s\", %s\r\n", absolute_asset, trim(comma));
        } else {
            fputs(line, destination);
        }
    }
    fclose(source);
    source = NULL;
    fclose(destination);
    destination = NULL;
    if (!commands) goto done;
    if (_snprintf(command, sizeof(command), "\"%s\" -script \"%s\"", executable, temporary) < 0)
        goto done;
    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));
    startup.cb = sizeof(startup);
    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                        install_root, &startup, &process)) {
        print_win32_error("CreateProcess(Resource Hacker fallback)");
        goto done;
    }
    wait_result = WaitForSingleObject(process.hProcess, 300000);
    if (wait_result == WAIT_OBJECT_0) GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    input_machine = read_pe_machine(input);
    output_machine = read_pe_machine(helper_output);
    crc32_file(input, &input_crc);
    crc32_file(helper_output, &output_crc);
    if (wait_result != WAIT_OBJECT_0 || exit_code != 0 || !regular_file_exists(helper_output) ||
        !input_machine || output_machine != input_machine || input_crc == output_crc) {
        fprintf(stderr, "Resource Hacker fallback failed for %s (wait %lu, exit %lu).\n",
                input, (unsigned long)wait_result, (unsigned long)exit_code);
        goto done;
    }
    SetFileAttributesA(output, FILE_ATTRIBUTE_NORMAL);
    if (!CopyFileA(helper_output, output, FALSE)) {
        print_win32_error("CopyFile(Resource Hacker neutral output)");
        goto done;
    }
    printf("Resource Hacker fallback patched %s -> %s.\n", input, output);
    result = 1;
done:
    if (source) fclose(source);
    if (destination) fclose(destination);
    DeleteFileA(temporary);
    DeleteFileA(helper_input);
    DeleteFileA(helper_output);
    DeleteFileA(helper_log);
    return result;
}

static int patch_file(const char *input, const char *script, const char *install_root, const char *output)
{
    RESOURCE_OPERATION *operations;
    unsigned count = 0, index;
    HANDLE update;
    DWORD input_crc = 0, output_crc = 0;
    int result = 0;
    operations = (RESOURCE_OPERATION *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RESOURCE_OPERATION) * MAX_OPERATIONS);
    if (!operations) {
        fprintf(stderr, "Out of memory allocating the operation manifest.\n");
        return 0;
    }
    if (!CopyFileA(input, output, FALSE)) {
        print_win32_error("CopyFile");
        goto done;
    }
    SetFileAttributesA(output, FILE_ATTRIBUTE_NORMAL);
    if (!parse_script(script, install_root, operations, &count) || !count) {
        fprintf(stderr, "No usable resource operations in %s\n", script);
        DeleteFileA(output);
        goto done;
    }
    if (!prepare_operations(output, operations, count)) {
        DeleteFileA(output);
        goto done;
    }
    update = BeginUpdateResourceA(output, FALSE);
    if (!update) {
        print_win32_error("BeginUpdateResource");
        DeleteFileA(output);
        result = legacy_resource_hacker_patch(input, script, install_root, output);
        goto done;
    }
    for (index = 0; index < count; ++index) {
        int ok;
        if (operations[index].type == OP_ICON_GROUP)
            ok = apply_icon_group(update, &operations[index]);
        else if (operations[index].type == OP_STRING) {
            if (string_block_already_applied(operations, index))
                ok = 1;
            else
                ok = apply_string_block(update, operations, count, index);
        }
        else
            ok = apply_raw_resource(update, &operations[index]);
        if (!ok) {
            EndUpdateResourceA(update, TRUE);
            DeleteFileA(output);
            result = legacy_resource_hacker_patch(input, script, install_root, output);
            goto done;
        }
    }
    if (!EndUpdateResourceA(update, FALSE)) {
        print_win32_error("EndUpdateResource");
        DeleteFileA(output);
        result = legacy_resource_hacker_patch(input, script, install_root, output);
        goto done;
    }
    if (!crc32_file(input, &input_crc) || !crc32_file(output, &output_crc)) {
        fprintf(stderr, "Cannot verify native resource update output for %s.\n", input);
        DeleteFileA(output);
        result = legacy_resource_hacker_patch(input, script, install_root, output);
        goto done;
    }
    if (input_crc == output_crc) {
        printf("All %u requested resource operation(s) already match in %s.\n", count, input);
        result = 1;
        goto done;
    }
    printf("Patched %s -> %s using %u resource operation(s).\n", input, output, count);
    result = 1;
done:
    for (index = 0; index < count; ++index) {
        unsigned language_index;
        for (language_index = 0; language_index < operations[index].language_count;
             ++language_index) {
            if (operations[index].string_blocks[language_index])
                HeapFree(GetProcessHeap(), 0,
                         operations[index].string_blocks[language_index]);
        }
    }
    HeapFree(GetProcessHeap(), 0, operations);
    return result;
}

static int expand_target(const char *expression, char *output, size_t output_size)
{
    char base[MAX_PATH_TEXT];
    const char *suffix;
    if (_strnicmp(expression, "$WINDIR\\", 8) == 0) {
        if (!GetWindowsDirectoryA(base, sizeof(base))) return 0;
        suffix = expression + 8;
    } else if (_strnicmp(expression, "$SYSDIR\\", 8) == 0) {
        if (!GetSystemDirectoryA(base, sizeof(base))) return 0;
        suffix = expression + 8;
    } else if (_strnicmp(expression, "$PROGRAM_FILES\\", 15) == 0) {
        if (!GetEnvironmentVariableA("ProgramFiles", base, sizeof(base))) return 0;
        suffix = expression + 15;
    } else {
        lstrcpynA(output, expression, (int)output_size);
        return 1;
    }
    return join_path(output, output_size, base, suffix);
}

static WORD read_pe_machine(const char *path)
{
    HANDLE file;
    IMAGE_DOS_HEADER dos_header;
    DWORD read = 0;
    DWORD signature = 0;
    IMAGE_FILE_HEADER file_header;
    WORD result = 0;
    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(file, &dos_header, sizeof(dos_header), &read, NULL) ||
        read != sizeof(dos_header) || dos_header.e_magic != IMAGE_DOS_SIGNATURE)
        goto done;
    if (SetFilePointer(file, dos_header.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
        GetLastError() != NO_ERROR)
        goto done;
    if (!ReadFile(file, &signature, sizeof(signature), &read, NULL) ||
        read != sizeof(signature) || signature != IMAGE_NT_SIGNATURE)
        goto done;
    if (!ReadFile(file, &file_header, sizeof(file_header), &read, NULL) || read != sizeof(file_header))
        goto done;
    result = file_header.Machine;
done:
    CloseHandle(file);
    return result;
}

static DWORD file_size_low(const char *path)
{
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attributes) || attributes.nFileSizeHigh)
        return 0;
    return attributes.nFileSizeLow;
}

static void count_matching_resources(const char *path, const char *script_name, const char *operations_tsv,
                                     unsigned *matching, unsigned *total)
{
    FILE *file;
    char line[1024];
    HMODULE module;
    *matching = 0;
    *total = 0;
    module = LoadLibraryExA(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!module) return;
    file = fopen(operations_tsv, "rb");
    if (!file) {
        FreeLibrary(module);
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        char *script, *type_name, *id, *separator;
        LPCSTR type;
        script = trim(line);
        separator = strchr(script, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        type_name = separator;
        separator = strchr(type_name, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        id = trim(separator);
        if (equals_ignore_case(script, "script") || !equals_ignore_case(script, script_name)) continue;
        ++*total;
        if (equals_ignore_case(type_name, "ICONGROUP")) type = RT_GROUP_ICON;
        else if (equals_ignore_case(type_name, "BITMAP")) type = RT_BITMAP;
        else if (equals_ignore_case(type_name, "AVI")) type = "AVI";
        else if (equals_ignore_case(type_name, "DIALOG")) type = RT_DIALOG;
        else continue;
        if (FindResourceA(module, resource_identifier(id), type)) ++*matching;
    }
    fclose(file);
    FreeLibrary(module);
}

static void write_inventory_record(FILE *report, const char *id, const char *variant, const char *script,
                                   const char *expression, const char *path, const char *operations_tsv)
{
    DWORD attributes = GetFileAttributesA(path);
    int exists = attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    WORD machine = 0;
    DWORD size = 0;
    unsigned matching = 0, total = 0;
    if (exists) {
        machine = read_pe_machine(path);
        size = file_size_low(path);
        count_matching_resources(path, script, operations_tsv, &matching, &total);
    }
    fprintf(report, "%s\t%s\t%d\t%04X\t%lu\t%u\t%u\t%s\t%s\t%s\r\n",
            id, variant, exists, (unsigned)machine, (unsigned long)size, matching, total,
            script, expression, path);
}

static int inventory_targets(const char *targets_tsv, const char *operations_tsv, const char *report_path)
{
    FILE *targets = NULL, *report = NULL;
    char line[8192];
    int result = 0;
    targets = fopen(targets_tsv, "rb");
    if (!targets) {
        fprintf(stderr, "Cannot open targets manifest: %s\n", targets_tsv);
        return 0;
    }
    report = fopen(report_path, "wb");
    if (!report) {
        fprintf(stderr, "Cannot create inventory report: %s\n", report_path);
        fclose(targets);
        return 0;
    }
    fprintf(report, "id\tvariant\texists\tmachine\tsize\tmatching_operations\ttotal_operations\tscript\texpression\tpath\r\n");
    while (fgets(line, sizeof(line), targets)) {
        char *id, *script, *expression, *separator;
        char path[MAX_PATH_TEXT];
        id = trim(line);
        separator = strchr(id, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        script = separator;
        separator = strchr(script, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        expression = trim(separator);
        if (equals_ignore_case(id, "id")) continue;
        if (!expand_target(expression, path, sizeof(path))) continue;
        write_inventory_record(report, id, "native", script, expression, path, operations_tsv);
        if (_strnicmp(expression, "$SYSDIR\\", 8) == 0) {
            char windows[MAX_PATH_TEXT], wow64[MAX_PATH_TEXT];
            if (GetWindowsDirectoryA(windows, sizeof(windows)) &&
                join_path(wow64, sizeof(wow64), windows, "SysWOW64") &&
                join_path(path, sizeof(path), wow64, expression + 8))
                write_inventory_record(report, id, "wow64", script, expression, path, operations_tsv);
        }
    }
    result = 1;
    fclose(report);
    fclose(targets);
    printf("Inventory written to %s.\n", report_path);
    return result;
}

static int regular_file_exists(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static int ensure_directory_tree(const char *path)
{
    char buffer[MAX_PATH_TEXT];
    char *cursor;
    DWORD attributes;
    lstrcpynA(buffer, path, sizeof(buffer));
    for (cursor = buffer; *cursor; ++cursor) {
        if ((*cursor == '\\' || *cursor == '/') && cursor > buffer + 2) {
            char saved = *cursor;
            *cursor = '\0';
            attributes = GetFileAttributesA(buffer);
            if (attributes == INVALID_FILE_ATTRIBUTES && !CreateDirectoryA(buffer, NULL) &&
                GetLastError() != ERROR_ALREADY_EXISTS) {
                print_win32_error(buffer);
                *cursor = saved;
                return 0;
            }
            *cursor = saved;
        }
    }
    attributes = GetFileAttributesA(buffer);
    if (attributes == INVALID_FILE_ATTRIBUTES && !CreateDirectoryA(buffer, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        print_win32_error(buffer);
        return 0;
    }
    return 1;
}

static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '\\');
    const char *forward = strrchr(path, '/');
    if (forward && (!slash || forward > slash)) slash = forward;
    return slash ? slash + 1 : path;
}

static void sanitize_component(char *output, size_t output_size, const char *input)
{
    size_t index = 0;
    while (*input && index + 1 < output_size) {
        unsigned char value = (unsigned char)*input++;
        output[index++] = (isalnum(value) || value == '.' || value == '-' || value == '_') ?
                          (char)value : '_';
    }
    output[index] = '\0';
}

static DWORD crc32_buffer(DWORD crc, const BYTE *data, DWORD size)
{
    DWORD index, bit;
    crc = ~crc;
    for (index = 0; index < size; ++index) {
        crc ^= data[index];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320UL & (DWORD)-(LONG)(crc & 1));
    }
    return ~crc;
}

static int crc32_file(const char *path, DWORD *result)
{
    HANDLE file;
    BYTE buffer[32768];
    DWORD read = 0, crc = 0;
    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!ReadFile(file, buffer, sizeof(buffer), &read, NULL)) {
            CloseHandle(file);
            return 0;
        }
        if (read) crc = crc32_buffer(crc, buffer, read);
    } while (read);
    CloseHandle(file);
    *result = crc;
    return 1;
}

static int enable_debug_privilege(void)
{
    HANDLE token = NULL;
    TOKEN_PRIVILEGES privileges;
    LUID luid;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        print_win32_error("OpenProcessToken");
        return 0;
    }
    if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
        print_win32_error("LookupPrivilegeValue");
        CloseHandle(token);
        return 0;
    }
    ZeroMemory(&privileges, sizeof(privileges));
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    if (!AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), NULL, NULL) ||
        GetLastError() != ERROR_SUCCESS) {
        print_win32_error("AdjustTokenPrivileges");
        CloseHandle(token);
        return 0;
    }
    CloseHandle(token);
    return 1;
}

static DWORD find_process_id(const char *name)
{
    HANDLE snapshot;
    PROCESSENTRY32 entry;
    DWORD result = 0;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);
    if (Process32First(snapshot, &entry)) {
        do {
            if (equals_ignore_case(entry.szExeFile, name)) {
                result = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return result;
}

static BYTE *find_remote_module_base(DWORD process_id, const char *name)
{
    HANDLE snapshot;
    MODULEENTRY32 entry;
    BYTE *result = NULL;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;
    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);
    if (Module32First(snapshot, &entry)) {
        do {
            if (equals_ignore_case(entry.szModule, name) ||
                equals_ignore_case(base_name(entry.szExePath), name)) {
                result = entry.modBaseAddr;
                break;
            }
        } while (Module32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return result;
}

static int list_process_modules(const char *process_name)
{
    DWORD process_id = find_process_id(process_name);
    HANDLE snapshot;
    MODULEENTRY32 entry;
    if (!process_id) {
        fprintf(stderr, "Cannot locate process %s.\n", process_name);
        return 0;
    }
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snapshot == INVALID_HANDLE_VALUE) {
        print_win32_error("CreateToolhelp32Snapshot(modules)");
        return 0;
    }
    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);
    if (!Module32First(snapshot, &entry)) {
        print_win32_error("Module32First");
        CloseHandle(snapshot);
        return 0;
    }
    printf("Modules for %s (PID %lu):\n", process_name, (unsigned long)process_id);
    do {
        printf("%s\t%s\n", entry.szModule, entry.szExePath);
    } while (Module32Next(snapshot, &entry));
    CloseHandle(snapshot);
    return 1;
}

static int disable_windows_file_protection(void)
{
    HMODULE local_module;
    FARPROC local_entry;
    BYTE *remote_base;
    LPTHREAD_START_ROUTINE remote_entry;
    HANDLE process = NULL, thread = NULL;
    DWORD process_id, wait_result;
    int result = 0;

    printf("Requesting temporary Windows File Protection suspension (isolated XP guest only).\n");
    if (!enable_debug_privilege()) return 0;
    process_id = find_process_id("winlogon.exe");
    if (!process_id) {
        fprintf(stderr, "Cannot locate WINLOGON.EXE.\n");
        return 0;
    }
    local_module = LoadLibraryA("sfc_os.dll");
    if (!local_module) {
        print_win32_error("LoadLibrary(sfc_os.dll)");
        return 0;
    }
    local_entry = GetProcAddress(local_module, MAKEINTRESOURCEA(2));
    remote_base = find_remote_module_base(process_id, "sfc_os.dll");
    if (!local_entry || !remote_base) {
        fprintf(stderr, "Cannot resolve SFC watcher shutdown export in WINLOGON.EXE.\n");
        FreeLibrary(local_module);
        return 0;
    }
    remote_entry = (LPTHREAD_START_ROUTINE)(remote_base + ((BYTE *)local_entry - (BYTE *)local_module));
    process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                          PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE, FALSE, process_id);
    if (!process) {
        print_win32_error("OpenProcess(winlogon)");
        FreeLibrary(local_module);
        return 0;
    }
    thread = CreateRemoteThread(process, NULL, 0, remote_entry, NULL, 0, NULL);
    if (!thread) {
        print_win32_error("CreateRemoteThread(SFC watcher shutdown)");
        goto done;
    }
    wait_result = WaitForSingleObject(thread, 240000);
    if (wait_result != WAIT_OBJECT_0) {
        fprintf(stderr, "SFC watcher shutdown did not complete (wait result %lu).\n",
                (unsigned long)wait_result);
        goto done;
    }
    printf("Windows File Protection watcher suspended until reboot.\n");
    result = 1;
done:
    if (thread) CloseHandle(thread);
    if (process) CloseHandle(process);
    FreeLibrary(local_module);
    return result;
}

static int path_has_prefix(const char *path, const char *prefix)
{
    size_t length = strlen(prefix);
    return _strnicmp(path, prefix, length) == 0 &&
           (path[length] == '\0' || path[length] == '\\' || path[length] == '/');
}

static int make_sibling_path(char *output, size_t output_size, const char *target, const char *suffix)
{
    int written = _snprintf(output, output_size, "%s%s", target, suffix);
    if (written < 0 || (size_t)written >= output_size) {
        fprintf(stderr, "Staging path is too long for %s.\n", target);
        return 0;
    }
    return 1;
}

static int schedule_file_replace(const char *source, const char *target)
{
    SetFileAttributesA(source, FILE_ATTRIBUTE_NORMAL);
    SetFileAttributesA(target, FILE_ATTRIBUTE_NORMAL);
    if (!MoveFileExA(source, target, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING)) {
        print_win32_error("MoveFileEx(schedule replace)");
        if (!CopyFileA(source, target, FALSE)) {
            print_win32_error("CopyFile(immediate replace fallback)");
            return 0;
        }
        DeleteFileA(source);
        printf("Replaced immediately: %s\n", target);
        return 1;
    }
    printf("Scheduled replacement: %s\n", target);
    return 1;
}

static int patch_and_schedule(const char *input, const char *script_path, const char *install_root,
                              const char *target, DWORD *patched_crc, DWORD *patched_size)
{
    char stage[MAX_PATH_TEXT];
    DWORD input_crc = 0;
    if (!make_sibling_path(stage, sizeof(stage), target, ".experience2k-new")) return 0;
    SetFileAttributesA(stage, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(stage);
    if (!patch_file(input, script_path, install_root, stage)) return 0;
    if (!crc32_file(stage, patched_crc)) {
        fprintf(stderr, "Cannot calculate CRC32 for staged file %s.\n", stage);
        DeleteFileA(stage);
        return 0;
    }
    *patched_size = file_size_low(stage);
    if (crc32_file(input, &input_crc) && input_crc == *patched_crc) {
        DeleteFileA(stage);
        printf("Target already contains every requested resource: %s\n", target);
        return 1;
    }
    if (!schedule_file_replace(stage, target)) {
        DeleteFileA(stage);
        return 0;
    }
    return 1;
}

static int restore_and_schedule(const char *backup, const char *target)
{
    char stage[MAX_PATH_TEXT];
    if (!make_sibling_path(stage, sizeof(stage), target, ".experience2k-restore")) return 0;
    SetFileAttributesA(stage, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(stage);
    if (!CopyFileA(backup, stage, FALSE)) {
        print_win32_error("CopyFile(restore staging)");
        return 0;
    }
    if (!schedule_file_replace(stage, target)) {
        DeleteFileA(stage);
        return 0;
    }
    return 1;
}

static int append_cache_path(char paths[][MAX_PATH_TEXT], unsigned *count, const char *directory,
                             const char *name)
{
    char candidate[MAX_PATH_TEXT];
    unsigned index;
    if (!join_path(candidate, sizeof(candidate), directory, name) || !regular_file_exists(candidate))
        return 1;
    for (index = 0; index < *count; ++index)
        if (equals_ignore_case(paths[index], candidate)) return 1;
    if (*count >= 4) return 0;
    lstrcpynA(paths[(*count)++], candidate, MAX_PATH_TEXT);
    return 1;
}

static unsigned discover_protection_caches(const char *target, char paths[][MAX_PATH_TEXT])
{
    char windows[MAX_PATH_TEXT], system32[MAX_PATH_TEXT], wow64[MAX_PATH_TEXT];
    char directory[MAX_PATH_TEXT];
    const char *name = base_name(target);
    unsigned count = 0;
    if (!GetWindowsDirectoryA(windows, sizeof(windows))) return 0;
    if (!join_path(system32, sizeof(system32), windows, "system32") ||
        !join_path(wow64, sizeof(wow64), windows, "SysWOW64")) return 0;
    if (path_has_prefix(target, system32)) {
        if (join_path(directory, sizeof(directory), system32, "dllcache"))
            append_cache_path(paths, &count, directory, name);
        if (join_path(directory, sizeof(directory), windows, "ServicePackFiles\\amd64"))
            append_cache_path(paths, &count, directory, name);
    } else if (path_has_prefix(target, wow64)) {
        if (join_path(directory, sizeof(directory), wow64, "dllcache"))
            append_cache_path(paths, &count, directory, name);
        if (join_path(directory, sizeof(directory), windows, "ServicePackFiles\\i386"))
            append_cache_path(paths, &count, directory, name);
    }
    return count;
}

static void patch_protection_caches(const char *target, const char *script_path, const char *install_root)
{
    char caches[4][MAX_PATH_TEXT];
    unsigned count = discover_protection_caches(target, caches), index;
    DWORD ignored_crc, ignored_size;
    for (index = 0; index < count; ++index) {
        if (!patch_and_schedule(caches[index], script_path, install_root, caches[index],
                                &ignored_crc, &ignored_size))
            fprintf(stderr, "Warning: could not stage protection cache %s.\n", caches[index]);
    }
}

static void restore_protection_caches(const char *target, const char *backup)
{
    char caches[4][MAX_PATH_TEXT];
    unsigned count = discover_protection_caches(target, caches), index;
    for (index = 0; index < count; ++index)
        if (!restore_and_schedule(backup, caches[index]))
            fprintf(stderr, "Warning: could not stage protection-cache restore %s.\n", caches[index]);
}

static void write_state_header(FILE *file)
{
    fprintf(file, "id\tvariant\tscript\ttarget\tbackup\toriginal_crc32\tpatched_crc32\toriginal_size\tpatched_size\r\n");
}

static void write_state_record(FILE *file, const STATE_RECORD *record)
{
    fprintf(file, "%s\t%s\t%s\t%s\t%s\t%08lX\t%08lX\t%lu\t%lu\r\n",
            record->id, record->variant, record->script, record->target, record->backup,
            (unsigned long)record->original_crc, (unsigned long)record->patched_crc,
            (unsigned long)record->original_size, (unsigned long)record->patched_size);
}

static int parse_state_record(char *line, STATE_RECORD *record)
{
    char *fields[9];
    char *cursor = trim(line);
    unsigned index;
    char *end;
    fields[0] = cursor;
    for (index = 1; index < 9; ++index) {
        cursor = strchr(cursor, '\t');
        if (!cursor) return 0;
        *cursor++ = '\0';
        fields[index] = cursor;
    }
    fields[8] = trim(fields[8]);
    if (equals_ignore_case(fields[0], "id")) return 0;
    ZeroMemory(record, sizeof(*record));
    lstrcpynA(record->id, fields[0], sizeof(record->id));
    lstrcpynA(record->variant, fields[1], sizeof(record->variant));
    lstrcpynA(record->script, fields[2], sizeof(record->script));
    lstrcpynA(record->target, fields[3], sizeof(record->target));
    lstrcpynA(record->backup, fields[4], sizeof(record->backup));
    record->original_crc = strtoul(fields[5], &end, 16);
    if (!end || *end) return 0;
    record->patched_crc = strtoul(fields[6], &end, 16);
    if (!end || *end) return 0;
    record->original_size = strtoul(fields[7], &end, 10);
    if (!end || *end) return 0;
    record->patched_size = strtoul(fields[8], &end, 10);
    return end && !*end;
}

static STATE_RECORD *load_state_records(const char *state_path, unsigned *count)
{
    FILE *file;
    STATE_RECORD *records;
    char line[16384];
    *count = 0;
    file = fopen(state_path, "rb");
    if (!file) {
        fprintf(stderr, "Cannot open state manifest: %s\n", state_path);
        return NULL;
    }
    records = (STATE_RECORD *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof(STATE_RECORD) * MAX_STATE_RECORDS);
    if (!records) {
        fclose(file);
        return NULL;
    }
    while (fgets(line, sizeof(line), file)) {
        if (*count >= MAX_STATE_RECORDS) {
            fprintf(stderr, "State manifest contains too many records.\n");
            HeapFree(GetProcessHeap(), 0, records);
            fclose(file);
            return NULL;
        }
        if (parse_state_record(line, &records[*count])) ++*count;
    }
    fclose(file);
    return records;
}

static int replace_state_file(const char *temporary, const char *state_path)
{
    SetFileAttributesA(state_path, FILE_ATTRIBUTE_NORMAL);
    if (!MoveFileExA(temporary, state_path, MOVEFILE_REPLACE_EXISTING)) {
        print_win32_error("MoveFileEx(state manifest)");
        return 0;
    }
    return 1;
}

static void configure_protected_renames(void)
{
    HKEY key;
    DWORD value = 1;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, NULL, 0,
            KEY_SET_VALUE, NULL, &key, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(key, "AllowProtectedRenames", 0, REG_DWORD,
                       (const BYTE *)&value, sizeof(value));
        RegCloseKey(key);
    }
}

static void remove_icon_cache_file(const char *profile)
{
    char path[MAX_PATH_TEXT];
    if (!join_path(path, sizeof(path), profile, "Local Settings\\Application Data\\IconCache.db"))
        return;
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    /*
     * The protected shell files are replaced during boot.  Deleting the cache
     * only while Explorer is still running lets Explorer recreate it from the
     * old, currently loaded resources before shutdown.  Always queue the same
     * path for deletion during the next boot first, then remove the live copy
     * where possible.  The queued delete also catches a cache Explorer
     * recreates after Apply but before the reboot.
     */
    MoveFileExA(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    if (!DeleteFileA(path) && GetLastError() != ERROR_FILE_NOT_FOUND)
        fprintf(stderr, "An icon cache is in use and will be removed during reboot.\n");
}

static void clear_icon_caches(void)
{
    HKEY profiles;
    DWORD index = 0;
    char name[512], raw[MAX_PATH_TEXT], expanded[MAX_PATH_TEXT];
    DWORD name_size, raw_size, type;
    HKEY profile;
    char current[MAX_PATH_TEXT];
    HMODULE shell;
    FARPROC procedure;
    typedef void (WINAPI *SHCHANGE)(LONG, UINT, LPCVOID, LPCVOID);
    SHCHANGE notify;
    if (GetEnvironmentVariableA("USERPROFILE", current, sizeof(current)))
        remove_icon_cache_file(current);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0,
            KEY_ENUMERATE_SUB_KEYS, &profiles) == ERROR_SUCCESS) {
        for (;;) {
            name_size = sizeof(name);
            if (RegEnumKeyExA(profiles, index++, name, &name_size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;
            if (RegOpenKeyExA(profiles, name, 0, KEY_QUERY_VALUE, &profile) != ERROR_SUCCESS) continue;
            raw_size = sizeof(raw);
            if (RegQueryValueExA(profile, "ProfileImagePath", NULL, &type, (BYTE *)raw, &raw_size) == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ)) {
                raw[sizeof(raw) - 1] = '\0';
                if (type == REG_EXPAND_SZ)
                    ExpandEnvironmentStringsA(raw, expanded, sizeof(expanded));
                else
                    lstrcpynA(expanded, raw, sizeof(expanded));
                remove_icon_cache_file(expanded);
            }
            RegCloseKey(profile);
        }
        RegCloseKey(profiles);
    }
    shell = LoadLibraryA("shell32.dll");
    if (shell) {
        procedure = GetProcAddress(shell, "SHChangeNotify");
        ZeroMemory(&notify, sizeof(notify));
        CopyMemory(&notify, &procedure, sizeof(procedure));
        if (notify) notify(0x08000000L, 0, NULL, NULL);
        FreeLibrary(shell);
    }
}

static int contains_ignore_case(const char *text, const char *needle)
{
    size_t length = strlen(needle);
    if (!length) return 1;
    while (*text) {
        if (_strnicmp(text, needle, length) == 0) return 1;
        ++text;
    }
    return 0;
}

static int read_registry_string(HKEY root, const char *subkey, const char *name,
                                char *output, DWORD output_size)
{
    HKEY key;
    DWORD type = 0, size = output_size;
    LONG status;
    if (!output || output_size == 0) return 0;
    output[0] = '\0';
    status = RegOpenKeyExA(root, subkey, 0, KEY_QUERY_VALUE, &key);
    if (status != ERROR_SUCCESS) return 0;
    status = RegQueryValueExA(key, name, NULL, &type, (BYTE *)output, &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        output[0] = '\0';
        return 0;
    }
    output[output_size - 1] = '\0';
    return 1;
}

static int token_is_administrator(void)
{
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administrators = NULL;
    BOOL member = FALSE;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administrators))
        return 0;
    if (!CheckTokenMembership(NULL, administrators, &member)) member = FALSE;
    FreeSid(administrators);
    return member ? 1 : 0;
}

static void set_detected_profile(DETECTED_OS *detected, const char *profile_id,
                                 const char *display_name, const char *branding_id,
                                 const char *branding_label)
{
    detected->supported = 1;
    lstrcpynA(detected->profile_id, profile_id, sizeof(detected->profile_id));
    lstrcpynA(detected->display_name, display_name, sizeof(detected->display_name));
    lstrcpynA(detected->branding_id, branding_id, sizeof(detected->branding_id));
    lstrcpynA(detected->branding_label, branding_label, sizeof(detected->branding_label));
    lstrcpynA(detected->support_reason, "Exact supported profile match.",
              sizeof(detected->support_reason));
}

static int detect_operating_system(DETECTED_OS *detected)
{
    OSVERSIONINFOEXA version;
    SYSTEM_INFO system_info;
    ZeroMemory(detected, sizeof(*detected));
    ZeroMemory(&version, sizeof(version));
    version.dwOSVersionInfoSize = sizeof(version);
    if (!GetVersionExA((OSVERSIONINFOA *)&version)) {
        lstrcpynA(detected->support_reason, "GetVersionEx failed.",
                  sizeof(detected->support_reason));
        return 0;
    }
    ZeroMemory(&system_info, sizeof(system_info));
    GetNativeSystemInfo(&system_info);
    detected->major = version.dwMajorVersion;
    detected->minor = version.dwMinorVersion;
    detected->build = version.dwBuildNumber;
    detected->service_pack = version.wServicePackMajor;
    detected->suite_mask = version.wSuiteMask;
    detected->product_type_value = version.wProductType;
    detected->server_r2 = GetSystemMetrics(SM_SERVERR2) != 0;
    detected->tablet_pc = GetSystemMetrics(SM_TABLETPC) != 0;
    detected->media_center = GetSystemMetrics(SM_MEDIACENTER) != 0;
    detected->administrator = token_is_administrator();
    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        lstrcpynA(detected->architecture, "x86", sizeof(detected->architecture));
    else if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        lstrcpynA(detected->architecture, "x64", sizeof(detected->architecture));
    else if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        lstrcpynA(detected->architecture, "ia64", sizeof(detected->architecture));
    else
        lstrcpynA(detected->architecture, "unknown", sizeof(detected->architecture));
    if (version.wProductType == VER_NT_WORKSTATION)
        lstrcpynA(detected->product_type, "WinNT", sizeof(detected->product_type));
    else if (version.wProductType == VER_NT_DOMAIN_CONTROLLER)
        lstrcpynA(detected->product_type, "LanmanNT", sizeof(detected->product_type));
    else if (version.wProductType == VER_NT_SERVER)
        lstrcpynA(detected->product_type, "ServerNT", sizeof(detected->product_type));
    else
        lstrcpynA(detected->product_type, "Unknown", sizeof(detected->product_type));
    read_registry_string(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName",
        detected->product_name, sizeof(detected->product_name));

    lstrcpynA(detected->support_reason,
        "No exact supported edition, architecture, build, and service-pack profile matched.",
        sizeof(detected->support_reason));
    if (detected->major == 5 && detected->minor == 1 && detected->build == 2600 &&
        equals_ignore_case(detected->architecture, "x86") &&
        version.wProductType == VER_NT_WORKSTATION) {
        if (!detected->media_center && !detected->tablet_pc &&
            !contains_ignore_case(detected->product_name, "Starter") &&
            (version.wSuiteMask & VER_SUITE_PERSONAL) == 0 &&
            detected->service_pack == 3) {
            set_detected_profile(detected, "xp-pro-x86-sp3",
                "Windows XP Professional x86 SP3", "xp-client-2001",
                "Windows 2001 Professional");
            detected->resource_profile_ready = 1;
        }
    } else if (detected->major == 5 && detected->minor == 2 &&
               detected->build == 3790 && detected->service_pack == 2) {
        if (version.wProductType == VER_NT_WORKSTATION &&
            equals_ignore_case(detected->architecture, "x64")) {
            set_detected_profile(detected, "xp-pro-x64-sp2",
                "Windows XP Professional x64 Edition SP2", "xp-x64-2005",
                "Windows 2005 Professional");
            detected->resource_profile_ready = 1;
        }
    }
    return 1;
}

static const char *yes_no(int value)
{
    return value ? "yes" : "no";
}

static int write_probe_report(const char *report_path)
{
    DETECTED_OS detected;
    FILE *report;
    if (!detect_operating_system(&detected)) return 0;
    report = fopen(report_path, "wb");
    if (!report) {
        fprintf(stderr, "Cannot create probe report.\n");
        return 0;
    }
    fprintf(report, "field\tvalue\r\n");
    fprintf(report, "schema\texperience2k-probe-v1\r\n");
    fprintf(report, "supported\t%s\r\n", yes_no(detected.supported));
    fprintf(report, "resource_profile_ready\t%s\r\n",
            yes_no(detected.resource_profile_ready));
    fprintf(report, "profile_id\t%s\r\n", detected.profile_id);
    fprintf(report, "display_name\t%s\r\n", detected.display_name);
    fprintf(report, "nt_version\t%lu.%lu\r\n", (unsigned long)detected.major,
            (unsigned long)detected.minor);
    fprintf(report, "build\t%lu\r\n", (unsigned long)detected.build);
    fprintf(report, "service_pack\t%u\r\n", (unsigned)detected.service_pack);
    fprintf(report, "suite_mask\t%04X\r\n", (unsigned)detected.suite_mask);
    fprintf(report, "architecture\t%s\r\n", detected.architecture);
    fprintf(report, "product_type\t%s\r\n", detected.product_type);
    fprintf(report, "server_r2\t%s\r\n", yes_no(detected.server_r2));
    fprintf(report, "tablet_pc\t%s\r\n", yes_no(detected.tablet_pc));
    fprintf(report, "media_center\t%s\r\n", yes_no(detected.media_center));
    fprintf(report, "administrator\t%s\r\n", yes_no(detected.administrator));
    fprintf(report, "branding_id\t%s\r\n", detected.branding_id);
    fprintf(report, "branding_label\t%s\r\n", detected.branding_label);
    fprintf(report, "reason\t%s\r\n", detected.support_reason);
    fclose(report);
    printf("OS probe: supported=%s, profile=%s, architecture=%s, administrator=%s.\n",
           yes_no(detected.supported), detected.profile_id, detected.architecture,
           yes_no(detected.administrator));
    return 1;
}

static int build_script_path(char *output, size_t output_size, const char *install_root,
                             const char *script)
{
    char directory[MAX_PATH_TEXT];
    if (!join_path(directory, sizeof(directory), install_root, "Resources\\scripts")) return 0;
    return join_path(output, output_size, directory, script);
}

static int build_backup_path(char *output, size_t output_size, const char *install_root,
                             const char *id, const char *variant, const char *target)
{
    char directory[MAX_PATH_TEXT], safe_id[128], safe_variant[128], name[MAX_PATH_TEXT];
    int written;
    if (!join_path(directory, sizeof(directory), install_root, "Backup")) return 0;
    sanitize_component(safe_id, sizeof(safe_id), id);
    sanitize_component(safe_variant, sizeof(safe_variant), variant);
    written = _snprintf(name, sizeof(name), "%s-%s-%s", safe_id, safe_variant, base_name(target));
    if (written < 0 || (size_t)written >= sizeof(name)) return 0;
    return join_path(output, output_size, directory, name);
}

static int install_target(FILE *state, const char *install_root, const char *id, const char *variant,
                          const char *script, const char *target)
{
    STATE_RECORD record;
    char script_path[MAX_PATH_TEXT];
    WORD machine;
    if (!regular_file_exists(target)) return 0;
    machine = read_pe_machine(target);
    if (machine != IMAGE_FILE_MACHINE_I386 && machine != IMAGE_FILE_MACHINE_AMD64) {
        fprintf(stderr, "Skipping non-PE target %s.\n", target);
        return 0;
    }
    if (!build_script_path(script_path, sizeof(script_path), install_root, script) ||
        !regular_file_exists(script_path)) {
        fprintf(stderr, "Missing resource script %s for %s.\n", script, target);
        return 0;
    }
    ZeroMemory(&record, sizeof(record));
    lstrcpynA(record.id, id, sizeof(record.id));
    lstrcpynA(record.variant, variant, sizeof(record.variant));
    lstrcpynA(record.script, script, sizeof(record.script));
    lstrcpynA(record.target, target, sizeof(record.target));
    if (!build_backup_path(record.backup, sizeof(record.backup), install_root, id, variant, target))
        return 0;
    if (regular_file_exists(record.backup)) {
        if (!crc32_file(record.backup, &record.original_crc)) {
            fprintf(stderr, "Cannot calculate CRC32 for existing backup %s.\n", record.backup);
            return 0;
        }
        record.original_size = file_size_low(record.backup);
    } else {
        if (!crc32_file(target, &record.original_crc)) {
            fprintf(stderr, "Cannot calculate CRC32 for %s.\n", target);
            return 0;
        }
        record.original_size = file_size_low(target);
        if (!CopyFileA(target, record.backup, TRUE)) {
            print_win32_error("CopyFile(create backup)");
            return 0;
        }
    }
    if (!patch_and_schedule(target, script_path, install_root, target,
                            &record.patched_crc, &record.patched_size))
        return 0;
    patch_protection_caches(target, script_path, install_root);
    write_state_record(state, &record);
    fflush(state);
    return 1;
}

static int install_manifest_targets(FILE *state, const char *install_root, const char *targets_path,
                                    unsigned *installed, unsigned *failed)
{
    FILE *targets;
    char line[8192];
    targets = fopen(targets_path, "rb");
    if (!targets) {
        fprintf(stderr, "Cannot open targets manifest: %s\n", targets_path);
        return 0;
    }
    while (fgets(line, sizeof(line), targets)) {
        char *id, *script, *expression, *separator;
        char path[MAX_PATH_TEXT];
        id = trim(line);
        separator = strchr(id, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        script = separator;
        separator = strchr(script, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        expression = trim(separator);
        if (equals_ignore_case(id, "id")) continue;
        if (expand_target(expression, path, sizeof(path)) && regular_file_exists(path)) {
            if (install_target(state, install_root, id, "native", script, path)) ++*installed;
            else ++*failed;
        }
        if (_strnicmp(expression, "$SYSDIR\\", 8) == 0) {
            char windows[MAX_PATH_TEXT], wow64[MAX_PATH_TEXT];
            if (GetWindowsDirectoryA(windows, sizeof(windows)) &&
                join_path(wow64, sizeof(wow64), windows, "SysWOW64") &&
                join_path(path, sizeof(path), wow64, expression + 8) && regular_file_exists(path)) {
                if (install_target(state, install_root, id, "wow64", script, path)) ++*installed;
                else ++*failed;
            }
        }
    }
    fclose(targets);
    return 1;
}

static int install_winsxs_targets(FILE *state, const char *install_root,
                                  unsigned *installed, unsigned *failed)
{
    char windows[MAX_PATH_TEXT], winsxs[MAX_PATH_TEXT], pattern[MAX_PATH_TEXT];
    WIN32_FIND_DATAA data;
    HANDLE find;
    unsigned sequence = 1;
    if (!GetWindowsDirectoryA(windows, sizeof(windows)) ||
        !join_path(winsxs, sizeof(winsxs), windows, "WinSxS") ||
        !join_path(pattern, sizeof(pattern), winsxs, "*")) return 0;
    find = FindFirstFileA(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) return 0;
    do {
        char directory[MAX_PATH_TEXT], target[MAX_PATH_TEXT], id[64];
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || data.cFileName[0] == '.') continue;
        if (!contains_ignore_case(data.cFileName, "microsoft.windows.common-controls")) continue;
        if (!join_path(directory, sizeof(directory), winsxs, data.cFileName) ||
            !join_path(target, sizeof(target), directory, "comctl32.dll") ||
            !regular_file_exists(target)) continue;
        _snprintf(id, sizeof(id), "SXS%03u", sequence++);
        if (install_target(state, install_root, id, "winsxs", "comctl32.dll.txt", target))
            ++*installed;
        else
            ++*failed;
    } while (FindNextFileA(find, &data));
    FindClose(find);
    return 1;
}

static int install_all(const char *install_root, const char *targets_path)
{
    char backup[MAX_PATH_TEXT], logs[MAX_PATH_TEXT], new_files[MAX_PATH_TEXT];
    char state_path[MAX_PATH_TEXT], temporary[MAX_PATH_TEXT];
    DETECTED_OS detected;
    FILE *state;
    unsigned installed = 0, failed = 0;
    if (!detect_operating_system(&detected) || !detected.supported) {
        fprintf(stderr, "Refusing resource installation: this operating-system profile is not explicitly supported.\n");
        return 0;
    }
    if (!detected.resource_profile_ready) {
        fprintf(stderr, "Refusing resource installation: profile %s is recognized but its exact resource payload is not validated yet.\n",
                detected.profile_id);
        return 0;
    }
    lstrcpynA(g_branding_id, detected.branding_id, sizeof(g_branding_id));
    printf("Resource branding profile: %s (%s).\n",
           detected.branding_id, detected.branding_label);
#ifdef _WIN64
    if (!equals_ignore_case(detected.architecture, "x64")) {
        fprintf(stderr, "Refusing resource installation: the x64 engine does not match the native OS architecture.\n");
        return 0;
    }
#else
    if (!equals_ignore_case(detected.architecture, "x86")) {
        fprintf(stderr, "Refusing resource installation: the x86 engine does not match the native OS architecture.\n");
        return 0;
    }
#endif
    if (!join_path(backup, sizeof(backup), install_root, "Backup") ||
        !join_path(logs, sizeof(logs), install_root, "Logs") ||
        !join_path(new_files, sizeof(new_files), install_root, "NewFiles") ||
        !ensure_directory_tree(backup) || !ensure_directory_tree(logs) ||
        !ensure_directory_tree(new_files) ||
        !join_path(state_path, sizeof(state_path), install_root, "state.tsv") ||
        !join_path(temporary, sizeof(temporary), install_root, "state.new.tsv")) return 0;
    configure_protected_renames();
    if (!disable_windows_file_protection())
        fprintf(stderr, "Warning: continuing with complete cache patching and boot reloader.\n");
    state = fopen(temporary, "wb");
    if (!state) {
        fprintf(stderr, "Cannot create state manifest %s.\n", temporary);
        return 0;
    }
    write_state_header(state);
    if (!install_manifest_targets(state, install_root, targets_path, &installed, &failed)) {
        fclose(state);
        return 0;
    }
    install_winsxs_targets(state, install_root, &installed, &failed);
    fclose(state);
    if (!replace_state_file(temporary, state_path)) return 0;
    clear_icon_caches();
    printf("Installation staging complete: %u target(s), %u failure(s). Reboot required.\n",
           installed, failed);
    return installed >= 100;
}

static int reload_all(const char *install_root)
{
    char state_path[MAX_PATH_TEXT], temporary[MAX_PATH_TEXT], script_path[MAX_PATH_TEXT];
    DETECTED_OS detected;
    STATE_RECORD *records;
    unsigned count = 0, index, changed = 0, failed = 0;
    int needs_work = 0;
    FILE *state;
    if (!detect_operating_system(&detected) || !detected.supported ||
        !detected.resource_profile_ready) {
        fprintf(stderr, "Refusing reload: the current operating-system resource profile is not validated.\n");
        return 0;
    }
    lstrcpynA(g_branding_id, detected.branding_id, sizeof(g_branding_id));
    if (!join_path(state_path, sizeof(state_path), install_root, "state.tsv") ||
        !join_path(temporary, sizeof(temporary), install_root, "state.reload.tsv")) return 0;
    records = load_state_records(state_path, &count);
    if (!records) return 0;
    for (index = 0; index < count; ++index) {
        DWORD crc;
        if (crc32_file(records[index].target, &crc) && crc != records[index].patched_crc) {
            needs_work = 1;
            break;
        }
    }
    if (!needs_work) {
        printf("Reloader check complete: all %u managed target(s) are current.\n", count);
        HeapFree(GetProcessHeap(), 0, records);
        return 1;
    }
    configure_protected_renames();
    if (!disable_windows_file_protection())
        fprintf(stderr, "Warning: reloader will rely on cache patching and the next boot.\n");
    state = fopen(temporary, "wb");
    if (!state) {
        HeapFree(GetProcessHeap(), 0, records);
        return 0;
    }
    write_state_header(state);
    for (index = 0; index < count; ++index) {
        DWORD current_crc;
        STATE_RECORD *record = &records[index];
        if (!crc32_file(record->target, &current_crc)) {
            fprintf(stderr, "Reloader cannot read %s.\n", record->target);
            ++failed;
            write_state_record(state, record);
            continue;
        }
        if (current_crc == record->patched_crc) {
            write_state_record(state, record);
            continue;
        }
        SetFileAttributesA(record->backup, FILE_ATTRIBUTE_NORMAL);
        if (!CopyFileA(record->target, record->backup, FALSE)) {
            print_win32_error("CopyFile(update backup)");
            ++failed;
            write_state_record(state, record);
            continue;
        }
        record->original_crc = current_crc;
        record->original_size = file_size_low(record->target);
        if (!build_script_path(script_path, sizeof(script_path), install_root, record->script) ||
            !patch_and_schedule(record->target, script_path, install_root, record->target,
                                &record->patched_crc, &record->patched_size)) {
            ++failed;
            write_state_record(state, record);
            continue;
        }
        patch_protection_caches(record->target, script_path, install_root);
        ++changed;
        write_state_record(state, record);
    }
    fclose(state);
    HeapFree(GetProcessHeap(), 0, records);
    if (!replace_state_file(temporary, state_path)) return 0;
    if (changed) clear_icon_caches();
    printf("Reloader staging complete: %u changed target(s), %u failure(s).%s\n",
           changed, failed, changed ? " Reboot required." : "");
    return failed == 0 || changed > failed;
}

static int uninstall_all(const char *install_root)
{
    char state_path[MAX_PATH_TEXT];
    STATE_RECORD *records;
    unsigned count = 0, index, restored = 0, failed = 0;
    if (!join_path(state_path, sizeof(state_path), install_root, "state.tsv")) return 0;
    records = load_state_records(state_path, &count);
    if (!records) return 0;
    configure_protected_renames();
    if (!disable_windows_file_protection())
        fprintf(stderr, "Warning: uninstall will rely on restored caches and the next boot.\n");
    for (index = count; index > 0; --index) {
        STATE_RECORD *record = &records[index - 1];
        if (!regular_file_exists(record->backup)) {
            fprintf(stderr, "Missing backup for %s: %s\n", record->target, record->backup);
            ++failed;
            continue;
        }
        if (!restore_and_schedule(record->backup, record->target)) {
            ++failed;
            continue;
        }
        restore_protection_caches(record->target, record->backup);
        ++restored;
    }
    clear_icon_caches();
    HeapFree(GetProcessHeap(), 0, records);
    printf("Uninstall staging complete: %u restored target(s), %u failure(s). Reboot required.\n",
           restored, failed);
    return restored > 0 && (failed == 0 || restored > failed);
}

static int verify_all(const char *install_root, const char *report_path)
{
    char state_path[MAX_PATH_TEXT];
    STATE_RECORD *records;
    FILE *report;
    unsigned count = 0, index, patched = 0, original = 0, changed = 0, missing = 0;
    if (!join_path(state_path, sizeof(state_path), install_root, "state.tsv")) return 0;
    records = load_state_records(state_path, &count);
    if (!records) return 0;
    report = fopen(report_path, "wb");
    if (!report) {
        HeapFree(GetProcessHeap(), 0, records);
        return 0;
    }
    fprintf(report, "id\tvariant\tstatus\tcurrent_crc32\tpatched_crc32\toriginal_crc32\ttarget\r\n");
    for (index = 0; index < count; ++index) {
        DWORD crc = 0;
        const char *status;
        if (!crc32_file(records[index].target, &crc)) {
            status = "missing";
            ++missing;
        } else if (crc == records[index].patched_crc) {
            status = "patched";
            ++patched;
        } else if (crc == records[index].original_crc) {
            status = "original";
            ++original;
        } else {
            status = "changed";
            ++changed;
        }
        fprintf(report, "%s\t%s\t%s\t%08lX\t%08lX\t%08lX\t%s\r\n",
                records[index].id, records[index].variant, status, (unsigned long)crc,
                (unsigned long)records[index].patched_crc,
                (unsigned long)records[index].original_crc, records[index].target);
    }
    fprintf(report, "SUMMARY\t-\tpatched=%u;original=%u;changed=%u;missing=%u\t00000000\t00000000\t00000000\t-\r\n",
            patched, original, changed, missing);
    fclose(report);
    HeapFree(GetProcessHeap(), 0, records);
    printf("Verification written to %s: %u patched, %u original, %u changed, %u missing.\n",
           report_path, patched, original, changed, missing);
    return missing == 0;
}

static int delete_registry_tree_native(HKEY root, const char *subkey)
{
    HKEY key;
    LONG status;
    char child[256];
    char nested[1024];
    DWORD length;

    status = RegOpenKeyExA(root, subkey, 0,
                           KEY_READ | KEY_WRITE | KEY_ENUMERATE_SUB_KEYS,
                           &key);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
        return 1;
    if (status != ERROR_SUCCESS) return 0;

    for (;;) {
        length = sizeof(child);
        status = RegEnumKeyExA(key, 0, child, &length, NULL, NULL, NULL, NULL);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) {
            RegCloseKey(key);
            return 0;
        }
        if (_snprintf(nested, sizeof(nested), "%s\\%s", subkey, child) < 0 ||
            !delete_registry_tree_native(root, nested)) {
            RegCloseKey(key);
            return 0;
        }
    }
    RegCloseKey(key);
    status = RegDeleteKeyA(root, subkey);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND ||
           status == ERROR_PATH_NOT_FOUND;
}

static int cleanup_explorer_registrations(void)
{
    static const char *owned_trees[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}",
        "SOFTWARE\\Microsoft\\Internet Explorer\\Explorer Bars\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}",
        "SOFTWARE\\Classes\\CLSID\\{7D298B9A-9BE0-48E9-9733-AD9A17EA6D20}",
        "SOFTWARE\\Classes\\CLSID\\{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}"
    };
    unsigned index;
    int ok = 1;
    for (index = 0; index < sizeof(owned_trees) / sizeof(owned_trees[0]); ++index)
        ok &= delete_registry_tree_native(HKEY_LOCAL_MACHINE,
                                          owned_trees[index]);
    printf(ok ? "Native Explorer registration cleanup complete.\n"
              : "Native Explorer registration cleanup failed.\n");
    return ok;
}

static void usage(void)
{
    fprintf(stderr,
        "eXPerience2K Core\n"
        "Usage:\n"
        "  eXPerience2KCore.exe patch-one <input> <script> <install-root> <output>\n"
        "  eXPerience2KCore.exe fallback-one <input> <script> <install-root> <output>\n"
        "  eXPerience2KCore.exe stage-one <input> <script> <install-root> <target>\n"
        "  eXPerience2KCore.exe inventory <targets.tsv> <operations.tsv> <report.tsv>\n"
        "  eXPerience2KCore.exe probe <report.tsv>\n"
        "  eXPerience2KCore.exe install <install-root> <targets.tsv>\n"
        "  eXPerience2KCore.exe reload <install-root>\n"
        "  eXPerience2KCore.exe uninstall <install-root>\n"
        "  eXPerience2KCore.exe verify <install-root> <report.tsv>\n"
        "  eXPerience2KCore.exe modules <process-name>\n"
        "  eXPerience2KCore.exe cleanup-explorer-registrations\n"
        "  eXPerience2KCore.exe wfp-disable\n");
}

int main(int argc, char **argv)
{
    if (argc == 6 && equals_ignore_case(argv[1], "patch-one"))
        return patch_file(argv[2], argv[3], argv[4], argv[5]) ? 0 : 1;
    if (argc == 6 && equals_ignore_case(argv[1], "fallback-one"))
        return legacy_resource_hacker_patch(argv[2], argv[3], argv[4], argv[5]) ? 0 : 1;
    if (argc == 6 && equals_ignore_case(argv[1], "stage-one")) {
        DWORD patched_crc = 0, patched_size = 0;
        if (!patch_and_schedule(argv[2], argv[3], argv[4], argv[5],
                                &patched_crc, &patched_size))
            return 1;
        printf("Staged CRC32 %08lX, size %lu.\n",
               (unsigned long)patched_crc, (unsigned long)patched_size);
        return 0;
    }
    if (argc == 5 && equals_ignore_case(argv[1], "inventory"))
        return inventory_targets(argv[2], argv[3], argv[4]) ? 0 : 1;
    if (argc == 3 && equals_ignore_case(argv[1], "probe"))
        return write_probe_report(argv[2]) ? 0 : 1;
    if (argc == 4 && equals_ignore_case(argv[1], "install"))
        return install_all(argv[2], argv[3]) ? 0 : 1;
    if (argc == 3 && equals_ignore_case(argv[1], "reload"))
        return reload_all(argv[2]) ? 0 : 1;
    if (argc == 3 && equals_ignore_case(argv[1], "uninstall"))
        return uninstall_all(argv[2]) ? 0 : 1;
    if (argc == 4 && equals_ignore_case(argv[1], "verify"))
        return verify_all(argv[2], argv[3]) ? 0 : 1;
    if (argc == 3 && equals_ignore_case(argv[1], "modules"))
        return list_process_modules(argv[2]) ? 0 : 1;
    if (argc == 2 && equals_ignore_case(argv[1], "cleanup-explorer-registrations"))
        return cleanup_explorer_registrations() ? 0 : 1;
    if (argc == 2 && equals_ignore_case(argv[1], "wfp-disable"))
        return disable_windows_file_protection() ? 0 : 1;
    usage();
    return 2;
}
