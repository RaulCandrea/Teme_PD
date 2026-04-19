/*
 * Tema 2: Listarea metaparametrilor unui device conectat la PC
 * Foloseste SetupAPI (apeluri sistem Windows) pentru enumerarea
 * tuturor dispozitivelor si proprietatilor lor.
 *
 * Compilare MSVC: cl /nologo /W4 /EHsc device_info.cpp setupapi.lib advapi32.lib
 */

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Structura pentru asocierea cod proprietate -> nume
struct PropEntry {
    DWORD code;
    const char *name;
};

// Toate proprietatile standard ale unui device (SPDRP_*)
static const PropEntry deviceProperties[] = {
    { SPDRP_DEVICEDESC,                  "Device Description"         },
    { SPDRP_HARDWAREID,                  "Hardware ID"                },
    { SPDRP_COMPATIBLEIDS,               "Compatible IDs"             },
    { SPDRP_SERVICE,                     "Service"                    },
    { SPDRP_CLASS,                       "Class"                      },
    { SPDRP_CLASSGUID,                   "Class GUID"                 },
    { SPDRP_DRIVER,                      "Driver"                     },
    { SPDRP_CONFIGFLAGS,                 "Config Flags"               },
    { SPDRP_MFG,                         "Manufacturer"               },
    { SPDRP_FRIENDLYNAME,               "Friendly Name"              },
    { SPDRP_LOCATION_INFORMATION,        "Location Information"       },
    { SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, "Physical Device Object"     },
    { SPDRP_CAPABILITIES,                "Capabilities"               },
    { SPDRP_UI_NUMBER,                   "UI Number"                  },
    { SPDRP_UPPERFILTERS,                "Upper Filters"              },
    { SPDRP_LOWERFILTERS,                "Lower Filters"              },
    { SPDRP_BUSTYPEGUID,                 "Bus Type GUID"              },
    { SPDRP_LEGACYBUSTYPE,               "Legacy Bus Type"            },
    { SPDRP_BUSNUMBER,                   "Bus Number"                 },
    { SPDRP_ENUMERATOR_NAME,             "Enumerator Name"            },
    { SPDRP_ADDRESS,                     "Address"                    },
    { SPDRP_DEVICE_POWER_DATA,           "Power Data"                 },
    { SPDRP_REMOVAL_POLICY,              "Removal Policy"             },
    { SPDRP_REMOVAL_POLICY_HW_DEFAULT,   "Removal Policy HW Default"  },
    { SPDRP_REMOVAL_POLICY_OVERRIDE,     "Removal Policy Override"    },
    { SPDRP_INSTALL_STATE,               "Install State"              },
    { SPDRP_LOCATION_PATHS,              "Location Paths"             },
    { SPDRP_BASE_CONTAINERID,            "Base Container ID"          },
};

static const int PROP_COUNT = sizeof(deviceProperties) / sizeof(deviceProperties[0]);

// Afiseaza o proprietate in functie de tipul ei
static void PrintProperty(const char *propName, DWORD regType,
                           const BYTE *buf, DWORD bufSize)
{
    switch (regType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            printf("    %-30s : %s\n", propName, reinterpret_cast<const char*>(buf));
            break;

        case REG_MULTI_SZ: {
            // Lista de siruri separate prin null
            printf("    %-30s : ", propName);
            const char *p = reinterpret_cast<const char*>(buf);
            bool first = true;
            while (*p) {
                if (!first) printf(", ");
                printf("%s", p);
                p += strlen(p) + 1;
                first = false;
            }
            printf("\n");
            break;
        }

        case REG_DWORD:
            if (bufSize >= sizeof(DWORD))
                printf("    %-30s : 0x%08lX (%lu)\n",
                       propName,
                       *reinterpret_cast<const DWORD*>(buf),
                       *reinterpret_cast<const DWORD*>(buf));
            break;

        default:
            printf("    %-30s : (date binare, %lu octeti)\n", propName, bufSize);
            break;
    }
}

int main(int argc, char *argv[])
{
    printf("=== Listarea metaparametrilor dispozitivelor conectate ===\n\n");

    bool showAll = true;
    int deviceFilter = -1;

    if (argc >= 2) {
        deviceFilter = atoi(argv[1]);
        showAll = false;
        printf("Se afiseaza doar dispozitivul #%d\n\n", deviceFilter);
    }

    // Obtinem lista tuturor dispozitivelor prezente in sistem
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(
        nullptr,        // Toate clasele
        nullptr,        // Fara enumerator specific
        nullptr,        // Fara fereastra parinte
        DIGCF_PRESENT | DIGCF_ALLCLASSES  // Doar dispozitive prezente
    );

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Eroare la SetupDiGetClassDevs: %lu\n", GetLastError());
        return 1;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DWORD deviceIndex = 0;

    // Iteram prin toate dispozitivele
    while (SetupDiEnumDeviceInfo(hDevInfo, deviceIndex, &devInfoData)) {

        if (!showAll && static_cast<int>(deviceIndex) != deviceFilter) {
            deviceIndex++;
            continue;
        }

        // Obtinem Instance ID-ul dispozitivului
        char instanceId[512];
        if (SetupDiGetDeviceInstanceIdA(hDevInfo, &devInfoData,
                                         instanceId, sizeof(instanceId), nullptr)) {
            printf("--- Dispozitiv #%lu: %s ---\n", deviceIndex, instanceId);
        } else {
            printf("--- Dispozitiv #%lu ---\n", deviceIndex);
        }

        // Enumeram toate proprietatile standard
        for (DWORD propIndex = 0; propIndex < static_cast<DWORD>(PROP_COUNT); propIndex++) {
            BYTE buffer[4096];
            DWORD bufferSize = sizeof(buffer);
            DWORD regType = 0;

            if (SetupDiGetDeviceRegistryPropertyA(
                    hDevInfo,
                    &devInfoData,
                    deviceProperties[propIndex].code,
                    &regType,
                    buffer,
                    bufferSize,
                    &bufferSize))
            {
                PrintProperty(deviceProperties[propIndex].name,
                              regType, buffer, bufferSize);
            }
            // Daca proprietatea nu exista, o sarim fara eroare
        }

        printf("\n");
        deviceIndex++;
    }

    printf("=== Total dispozitive enumerate: %lu ===\n", deviceIndex);

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
}
