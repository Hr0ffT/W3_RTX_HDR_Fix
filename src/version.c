#include <windows.h>
#include <aclapi.h>
#include <stdint.h>

typedef uint32_t u32;
static int g_hooked = 0;
static HMODULE hOrig = NULL;

static FARPROC GetOrig(const char* name)
{
    if (!hOrig)
    {
        char sysPath[MAX_PATH];
        GetSystemDirectoryA(sysPath, MAX_PATH);
        strcat(sysPath, "\\version.dll");
        hOrig = LoadLibraryA(sysPath);
    }
    return hOrig ? GetProcAddress(hOrig, name) : NULL;
}

static int __cdecl ProtectProcess_hook(void)
{
    SID_IDENTIFIER_AUTHORITY world = { SECURITY_WORLD_SID_AUTHORITY };
    PSID everyone = NULL;
    union { ACL acl; BYTE b[0x200]; } buf;
    DWORD err = ERROR_INVALID_FUNCTION;
    
    if (!AllocateAndInitializeSid(&world, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone)) return 0;
    
    if (InitializeAcl(&buf.acl, sizeof buf, ACL_REVISION)
        && AddAccessDeniedAce(&buf.acl, ACL_REVISION, 0x001FFFFF & ~PROCESS_QUERY_LIMITED_INFORMATION, everyone)
        && AddAccessAllowedAce(&buf.acl, ACL_REVISION, PROCESS_QUERY_LIMITED_INFORMATION, everyone))
    {
        err = SetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT,
                              DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, &buf.acl, NULL);
    }
    
    FreeSid(everyone);
    return err == ERROR_SUCCESS;
}

void InstallPathQueryFix(void)
{
    if (g_hooked) return;
    g_hooked = 1;

    u32 g_base = (u32)GetModuleHandleA("Game.dll");
    if (!g_base) return;

    u32 at = g_base + 0x00986C;
    
    if (*(uint8_t*)at == 0xe8 && at + 5 + *(int32_t*)(at + 1) == g_base + 0x00BAB0) {
        DWORD old;
        VirtualProtect((void*)at, 5, PAGE_EXECUTE_READWRITE, &old);
        
        uint8_t* patch = (uint8_t*)at;
        *patch = 0xe8;
        int32_t rel = (int32_t)((u32)ProtectProcess_hook - (at + 5));
        memcpy(patch + 1, &rel, 4);
        
        VirtualProtect((void*)at, 5, old, &old);
        FlushInstructionCache(GetCurrentProcess(), (void*)at, 5);
    }
    ProtectProcess_hook();
}


BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL (WINAPI* Fn)(LPCSTR, DWORD, DWORD, LPVOID);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoA");
    return fn ? fn(lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

BOOL WINAPI GetFileVersionInfoByHandle(int hFile, LPCSTR lpFileName, LPVOID lpData, DWORD dwLen) {
    typedef BOOL (WINAPI* Fn)(int, LPCSTR, LPVOID, DWORD);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoByHandle");
    return fn ? fn(hFile, lpFileName, lpData, dwLen) : FALSE;
}

BOOL WINAPI GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL (WINAPI* Fn)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoExA");
    return fn ? fn(dwFlags, lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

BOOL WINAPI GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL (WINAPI* Fn)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoExW");
    return fn ? fn(dwFlags, lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    InstallPathQueryFix();
    typedef DWORD (WINAPI* Fn)(LPCSTR, LPDWORD);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoSizeA");
    return fn ? fn(lptstrFilename, lpdwHandle) : 0;
}

DWORD WINAPI GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPCSTR, LPDWORD);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoSizeExA");
    return fn ? fn(dwFlags, lptstrFilename, lpdwHandle) : 0;
}

DWORD WINAPI GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPCWSTR, LPDWORD);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoSizeExW");
    return fn ? fn(dwFlags, lptstrFilename, lpdwHandle) : 0;
}

DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    typedef DWORD (WINAPI* Fn)(LPCWSTR, LPDWORD);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoSizeW");
    return fn ? fn(lptstrFilename, lpdwHandle) : 0;
}

BOOL WINAPI GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL (WINAPI* Fn)(LPCWSTR, DWORD, DWORD, LPVOID);
    Fn fn = (Fn)GetOrig("GetFileVersionInfoW");
    return fn ? fn(lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

DWORD WINAPI VerFindFileA(DWORD dwFlags, LPSTR szFileName, LPSTR szWinDir, LPSTR szAppDir, LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT);
    Fn fn = (Fn)GetOrig("VerFindFileA");
    return fn ? fn(dwFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen) : 0;
}

DWORD WINAPI VerFindFileW(DWORD dwFlags, LPWSTR szFileName, LPWSTR szWinDir, LPWSTR szAppDir, LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
    Fn fn = (Fn)GetOrig("VerFindFileW");
    return fn ? fn(dwFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen) : 0;
}

DWORD WINAPI VerInstallFileA(DWORD dwFlags, LPSTR szSrcFileName, LPSTR szDestFileName, LPSTR szSrcDir, LPSTR szDestDir, LPSTR szCurDir, LPSTR szTmpFileName, PUINT lpuTmpFileNameLen) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT);
    Fn fn = (Fn)GetOrig("VerInstallFileA");
    return fn ? fn(dwFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFileName, lpuTmpFileNameLen) : 0;
}

DWORD WINAPI VerInstallFileW(DWORD dwFlags, LPWSTR szSrcFileName, LPWSTR szDestFileName, LPWSTR szSrcDir, LPWSTR szDestDir, LPWSTR szCurDir, LPWSTR szTmpFileName, PUINT lpuTmpFileNameLen) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT);
    Fn fn = (Fn)GetOrig("VerInstallFileW");
    return fn ? fn(dwFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFileName, lpuTmpFileNameLen) : 0;
}

DWORD WINAPI VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD nSize) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPSTR, DWORD);
    Fn fn = (Fn)GetOrig("VerLanguageNameA");
    return fn ? fn(wLang, szLang, nSize) : 0;
}

DWORD WINAPI VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD nSize) {
    typedef DWORD (WINAPI* Fn)(DWORD, LPWSTR, DWORD);
    Fn fn = (Fn)GetOrig("VerLanguageNameW");
    return fn ? fn(wLang, szLang, nSize) : 0;
}

BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen) {
    typedef BOOL (WINAPI* Fn)(LPCVOID, LPCSTR, LPVOID*, PUINT);
    Fn fn = (Fn)GetOrig("VerQueryValueA");
    return fn ? fn(pBlock, lpSubBlock, lplpBuffer, puLen) : FALSE;
}

BOOL WINAPI VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen) {
    typedef BOOL (WINAPI* Fn)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
    Fn fn = (Fn)GetOrig("VerQueryValueW");
    return fn ? fn(pBlock, lpSubBlock, lplpBuffer, puLen) : FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
}
