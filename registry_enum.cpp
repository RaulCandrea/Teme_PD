/*
 * Tema 1: Afisarea tuturor valorilor unei subchei din Registry
 * Compilare MSVC: cl /nologo /W4 /EHsc registry_enum.cpp advapi32.lib
 */

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Afiseaza tipul valorii in format text
static const char* RegTypeToString(DWORD type)
{
    switch (type) {
        case REG_NONE:                       return "REG_NONE";
        case REG_SZ:                         return "REG_SZ";
        case REG_EXPAND_SZ:                  return "REG_EXPAND_SZ";
        case REG_BINARY:                     return "REG_BINARY";
        case REG_DWORD:                      return "REG_DWORD";
        case REG_DWORD_BIG_ENDIAN:           return "REG_DWORD_BIG_ENDIAN";
        case REG_LINK:                       return "REG_LINK";
        case REG_MULTI_SZ:                   return "REG_MULTI_SZ";
        case REG_RESOURCE_LIST:              return "REG_RESOURCE_LIST";
        case REG_FULL_RESOURCE_DESCRIPTOR:   return "REG_FULL_RESOURCE_DESCRIPTOR";
        case REG_RESOURCE_REQUIREMENTS_LIST: return "REG_RESOURCE_REQUIREMENTS_LIST";
        case REG_QWORD:                      return "REG_QWORD";
        default:                             return "UNKNOWN";
    }
}

// Afiseaza datele valorii in functie de tip
static void PrintValueData(DWORD type, const BYTE *data, DWORD dataSize)
{
    switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            // Sir de caractere terminat cu null
            printf("  Date: \"%s\"\n", reinterpret_cast<const char*>(data));
            break;

        case REG_DWORD:
            if (dataSize >= sizeof(DWORD))
                printf("  Date: 0x%08lX (%lu)\n",
                       *reinterpret_cast<const DWORD*>(data),
                       *reinterpret_cast<const DWORD*>(data));
            break;

        case REG_QWORD:
            if (dataSize >= sizeof(ULONGLONG))
                printf("  Date: 0x%016llX (%llu)\n",
                       *reinterpret_cast<const ULONGLONG*>(data),
                       *reinterpret_cast<const ULONGLONG*>(data));
            break;

        case REG_MULTI_SZ: {
            // Secventa de siruri terminate cu null, terminata cu null suplimentar
            const char *p = reinterpret_cast<const char*>(data);
            int idx = 0;
            printf("  Date (multi-string):\n");
            while (*p) {
                printf("    [%d] \"%s\"\n", idx++, p);
                p += strlen(p) + 1;
            }
            break;
        }

        case REG_BINARY:
        default:
            // Afisare hexadecimala
            printf("  Date (hex, %lu octeti):", dataSize);
            for (DWORD i = 0; i < dataSize; i++) {
                if (i % 16 == 0) printf("\n    ");
                printf("%02X ", data[i]);
            }
            printf("\n");
            break;
    }
}

// Parseaza un sir de forma "HKEY_LOCAL_MACHINE" si returneaza handle-ul predefinit
static HKEY ParseRootKey(const char *name)
{
    if (_stricmp(name, "HKEY_LOCAL_MACHINE") == 0 || _stricmp(name, "HKLM") == 0)
        return HKEY_LOCAL_MACHINE;
    if (_stricmp(name, "HKEY_CURRENT_USER") == 0 || _stricmp(name, "HKCU") == 0)
        return HKEY_CURRENT_USER;
    if (_stricmp(name, "HKEY_CLASSES_ROOT") == 0 || _stricmp(name, "HKCR") == 0)
        return HKEY_CLASSES_ROOT;
    if (_stricmp(name, "HKEY_USERS") == 0 || _stricmp(name, "HKU") == 0)
        return HKEY_USERS;
    if (_stricmp(name, "HKEY_CURRENT_CONFIG") == 0) return HKEY_CURRENT_CONFIG;
    return nullptr;
}

int main(int argc, char *argv[])
{
    const char *fullPath;

    // Cheie implicita de demonstratie
    if (argc < 2) {
        fullPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
        printf("Utilizare: %s <cale_registry>\n", argv[0]);
        printf("Exemplu:   %s HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\n\n", argv[0]);
        printf("Se foloseste cheia implicita: %s\n\n", fullPath);
    } else {
        fullPath = argv[1];
    }

    // Separam root key de subcheie
    char rootName[64];
    strncpy(rootName, fullPath, sizeof(rootName) - 1);
    rootName[sizeof(rootName) - 1] = '\0';
    char *sep = strchr(rootName, '\\');
    if (!sep) {
        fprintf(stderr, "Eroare: formatul trebuie sa fie HKEY_xxx\\subcheie\n");
        return 1;
    }
    *sep = '\0';
    const char *subKeyPath = fullPath + (sep - rootName) + 1;

    HKEY hRootKey = ParseRootKey(rootName);
    if (!hRootKey) {
        fprintf(stderr, "Eroare: root key necunoscut: %s\n", rootName);
        return 1;
    }

    // Deschidem subcheia
    HKEY hKey;
    LONG result = RegOpenKeyExA(hRootKey, subKeyPath, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Eroare la RegOpenKeyEx: cod %ld\n", result);
        return 1;
    }

    // Obtinem informatii despre subcheie
    DWORD valueCount, maxNameLen, maxDataLen;
    result = RegQueryInfoKeyA(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                              &valueCount, &maxNameLen, &maxDataLen, nullptr, nullptr);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Eroare la RegQueryInfoKey: cod %ld\n", result);
        RegCloseKey(hKey);
        return 1;
    }

    maxNameLen++;  // +1 pentru terminatorul null
    printf("=== Valorile subcheii: %s ===\n", fullPath);
    printf("Numar total de valori: %lu\n\n", valueCount);

    // Alocam buffere
    char *valueName = new char[maxNameLen];
    BYTE *data = new BYTE[maxDataLen];

    // Enumeram toate valorile
    for (DWORD index = 0; index < valueCount; index++) {
        DWORD nameLen = maxNameLen;
        DWORD dataLen = maxDataLen;
        DWORD type;

        result = RegEnumValueA(hKey, index, valueName, &nameLen,
                               nullptr, &type, data, &dataLen);
        if (result != ERROR_SUCCESS) {
            printf("[%lu] Eroare la enumerare: cod %ld\n", index, result);
            continue;
        }

        printf("[%lu] Nume: \"%s\"\n", index, valueName[0] ? valueName : "(Implicit)");
        printf("  Tip:  %s (%lu)\n", RegTypeToString(type), type);
        PrintValueData(type, data, dataLen);
        printf("\n");
    }

    delete[] valueName;
    delete[] data;
    RegCloseKey(hKey);

    printf("=== Enumerare completa ===\n");
    return 0;
}
