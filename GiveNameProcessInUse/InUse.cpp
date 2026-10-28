#include <windows.h>
#include <restartmanager.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "rstrtmgr.lib")

// Fonction qui vérifie si un fichier spécifique est verrouillé et par quel processus
void CheckFileLock(const std::wstring& filePath) {
    DWORD sessionHandle = 0;
    WCHAR sessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };

    DWORD res = RmStartSession(&sessionHandle, 0, sessionKey);
    if (res != ERROR_SUCCESS) return;

    LPCWSTR resources[] = { filePath.c_str() };
    res = RmRegisterResources(sessionHandle, 1, const_cast<LPCWSTR*>(resources), 0, NULL, 0, NULL);
    if (res != ERROR_SUCCESS) {
        RmEndSession(sessionHandle);
        return;
    }

    DWORD dwReason = 0;
    UINT nProcInfoNeeded = 0;
    UINT nProcInfo = 0;

    res = RmGetList(sessionHandle, &nProcInfoNeeded, &nProcInfo, NULL, &dwReason);
    if (res == ERROR_MORE_DATA || res == ERROR_SUCCESS) {
        if (nProcInfoNeeded > 0) {
            std::vector<RM_PROCESS_INFO> processInfo(nProcInfoNeeded);
            nProcInfo = nProcInfoNeeded;

            res = RmGetList(sessionHandle, &nProcInfoNeeded, &nProcInfo, processInfo.data(), &dwReason);
            if (res == ERROR_SUCCESS && nProcInfo > 0) {
                std::wcout << L"[VERROUILLÉ] " << std::filesystem::path(filePath).filename().wstring() << L"\n";
                for (UINT i = 0; i < nProcInfo; ++i) {
                    std::wcout << L"    -> Bloqué par : " << processInfo[i].strAppName
                        << L" (PID: " << processInfo[i].Process.dwProcessId << L")\n";
                }
            }
        }
    }
    RmEndSession(sessionHandle);
}

int main() {
    // Permet l'affichage correct des caractères accentués dans la console
    _wsetlocale(LC_ALL, L"");

    std::cout << "=== Analyse des verrous dans AppData\\Local\\Temp ===\n\n";

    char* userProfile = nullptr;
    size_t len = 0;
    _dupenv_s(&userProfile, &len, "USERPROFILE");
    if (!userProfile) {
        std::cout << "[ERREUR] Impossible de récupérer le profil utilisateur.\n";
        system("pause");
        return 1;
    }

    std::filesystem::path tempPath = std::string(userProfile) + "\\AppData\\Local\\Temp";
    free(userProfile);

    if (!std::filesystem::exists(tempPath)) {
        std::cout << "[ERREUR] Le dossier Temp introuvable.\n";
        system("pause");
        return 1;
    }

    try {
        // Parcourt tous les fichiers du dossier Temp
        for (const auto& entry : std::filesystem::directory_iterator(tempPath)) {
            if (entry.is_regular_file()) {
                CheckFileLock(entry.path().wstring());
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "[ERREUR] Exception : " << e.what() << "\n";
    }

    std::cout << "\n=== Analyse terminée ===\n";
    system("pause");
    return 0;
}