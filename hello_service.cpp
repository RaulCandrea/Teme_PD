/*
 * Tema 3: Serviciu sistem care afiseaza "Hello World!" la initializare
 * Foloseste Service Control Manager (SCM) si Event Log.
 *
 * Compilare MSVC: cl /nologo /W4 /EHsc hello_service.cpp advapi32.lib
 *
 * Instalare (cmd admin):   hello_service.exe install
 * Pornire:                 hello_service.exe start
 * Oprire:                  hello_service.exe stop
 * Dezinstalare:            hello_service.exe uninstall
 */

#include <windows.h>
#include <cstdio>
#include <cstring>

static const char *SERVICE_NAME = "HelloWorldService";

static SERVICE_STATUS        g_serviceStatus;
static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static HANDLE                g_stopEvent    = nullptr;

// Raporteaza starea curenta a serviciului catre SCM
static void ReportStatus(DWORD state, DWORD exitCode, DWORD waitHint)
{
    static DWORD checkPoint = 1;

    g_serviceStatus.dwCurrentState  = state;
    g_serviceStatus.dwWin32ExitCode = exitCode;
    g_serviceStatus.dwWaitHint      = waitHint;

    if (state == SERVICE_START_PENDING)
        g_serviceStatus.dwControlsAccepted = 0;
    else
        g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED)
        g_serviceStatus.dwCheckPoint = 0;
    else
        g_serviceStatus.dwCheckPoint = checkPoint++;

    SetServiceStatus(g_statusHandle, &g_serviceStatus);
}

// Scrie un mesaj in Event Log-ul Windows
static void WriteEventLog(const char *message)
{
    HANDLE hEventLog = RegisterEventSourceA(nullptr, SERVICE_NAME);
    if (hEventLog) {
        const char *messages[] = { message };
        ReportEventA(hEventLog,
                     EVENTLOG_INFORMATION_TYPE,
                     0, 0, nullptr,
                     1, 0,
                     messages,
                     nullptr);
        DeregisterEventSource(hEventLog);
    }
}

// Handler pentru comenzile primite de la SCM (stop, etc.)
static DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD /*eventType*/,
                                        LPVOID /*eventData*/, LPVOID /*context*/)
{
    switch (control) {
        case SERVICE_CONTROL_STOP:
            ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            SetEvent(g_stopEvent);
            return NO_ERROR;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

// Functia principala a serviciului
static void WINAPI ServiceMain(DWORD /*argc*/, LPSTR * /*argv*/)
{
    g_statusHandle = RegisterServiceCtrlHandlerExA(SERVICE_NAME,
                                                    ServiceCtrlHandler,
                                                    nullptr);
    if (!g_statusHandle)
        return;

    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    g_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        ReportStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // === Mesajul "Hello World!" la initializare ===
    WriteEventLog("Hello World! - Serviciul a fost initializat cu succes.");

    ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Asteapta pana cand serviciul primeste comanda de oprire
    WaitForSingleObject(g_stopEvent, INFINITE);
    CloseHandle(g_stopEvent);

    WriteEventLog("Serviciul se opreste.");
    ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

// --- Comenzi de administrare (install / uninstall / start / stop) ---

static void InstallService()
{
    char path[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) {
        fprintf(stderr, "Eroare la GetModuleFileName: %lu\n", GetLastError());
        return;
    }

    SC_HANDLE hSCM = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) {
        fprintf(stderr, "Eroare la OpenSCManager: %lu (rulati ca Administrator)\n",
                GetLastError());
        return;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCM,
        SERVICE_NAME,
        "Hello World Service (Tema 3)",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,       // Pornire manuala
        SERVICE_ERROR_NORMAL,
        path,
        nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (hService) {
        printf("Serviciul '%s' a fost instalat cu succes.\n", SERVICE_NAME);
        CloseServiceHandle(hService);
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS)
            printf("Serviciul exista deja.\n");
        else
            fprintf(stderr, "Eroare la CreateService: %lu\n", err);
    }

    CloseServiceHandle(hSCM);
}

static void UninstallService()
{
    SC_HANDLE hSCM = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) {
        fprintf(stderr, "Eroare la OpenSCManager: %lu\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, SERVICE_NAME, DELETE);
    if (!hService) {
        fprintf(stderr, "Eroare la OpenService: %lu\n", GetLastError());
        CloseServiceHandle(hSCM);
        return;
    }

    if (DeleteService(hService))
        printf("Serviciul '%s' a fost dezinstalat.\n", SERVICE_NAME);
    else
        fprintf(stderr, "Eroare la DeleteService: %lu\n", GetLastError());

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
}

static void StartServiceCmd()
{
    SC_HANDLE hSCM = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) {
        fprintf(stderr, "Eroare la OpenSCManager: %lu\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, SERVICE_NAME, SERVICE_START);
    if (!hService) {
        fprintf(stderr, "Eroare la OpenService: %lu\n", GetLastError());
        CloseServiceHandle(hSCM);
        return;
    }

    if (StartServiceA(hService, 0, nullptr))
        printf("Serviciul '%s' a fost pornit.\n", SERVICE_NAME);
    else {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING)
            printf("Serviciul ruleaza deja.\n");
        else
            fprintf(stderr, "Eroare la StartService: %lu\n", err);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
}

static void StopServiceCmd()
{
    SC_HANDLE hSCM = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) {
        fprintf(stderr, "Eroare la OpenSCManager: %lu\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, SERVICE_NAME, SERVICE_STOP);
    if (!hService) {
        fprintf(stderr, "Eroare la OpenService: %lu\n", GetLastError());
        CloseServiceHandle(hSCM);
        return;
    }

    SERVICE_STATUS status;
    if (ControlService(hService, SERVICE_CONTROL_STOP, &status))
        printf("Serviciul '%s' a fost oprit.\n", SERVICE_NAME);
    else
        fprintf(stderr, "Eroare la ControlService(STOP): %lu\n", GetLastError());

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
}

int main(int argc, char *argv[])
{
    // Daca se ruleaza cu un argument, executam comanda administrativa
    if (argc >= 2) {
        if (_stricmp(argv[1], "install") == 0)        InstallService();
        else if (_stricmp(argv[1], "uninstall") == 0)  UninstallService();
        else if (_stricmp(argv[1], "start") == 0)      StartServiceCmd();
        else if (_stricmp(argv[1], "stop") == 0)       StopServiceCmd();
        else printf("Comenzi: install | uninstall | start | stop\n");
        return 0;
    }

    // Fara argumente -> SCM lanseaza ServiceMain
    SERVICE_TABLE_ENTRYA serviceTable[] = {
        { const_cast<char*>(SERVICE_NAME), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherA(serviceTable)) {
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
            printf("Acest executabil este un serviciu Windows.\n"
                   "Comenzi: %s install | start | stop | uninstall\n", argv[0]);
        else
            fprintf(stderr, "Eroare la StartServiceCtrlDispatcher: %lu\n", err);
    }

    return 0;
}
