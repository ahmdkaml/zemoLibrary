#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Magic footer to locate payload: "ZEMOBNDL" + 8-byte uint64 payload length
static const char MAGIC_FOOTER[8] = {'Z', 'E', 'M', 'O', 'B', 'N', 'D', 'L'};

int main() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Determine application directory
    char appDir[MAX_PATH];
    strncpy(appDir, exePath, MAX_PATH);
    char *lastBackslash = strrchr(appDir, '\\');
    if (lastBackslash) {
        *lastBackslash = '\0';
    } else {
        strcpy(appDir, ".");
    }

    char javaExe[MAX_PATH];
    snprintf(javaExe, MAX_PATH, "%s\\runtime\\bin\\java.exe", appDir);

    char jarFile[MAX_PATH];
    snprintf(jarFile, MAX_PATH, "%s\\backend.jar", appDir);

    // 1. Check if runtime and JAR are already extracted
    BOOL needExtract = FALSE;
    if (GetFileAttributesA(javaExe) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesA(jarFile) == INVALID_FILE_ATTRIBUTES) {
        needExtract = TRUE;
    }

    if (needExtract) {
        printf("==> First-time setup: extracting embedded Java runtime & application...\n");

        FILE *f = fopen(exePath, "rb");
        if (!f) {
            fprintf(stderr, "Error: cannot open executable for reading.\n");
            return 1;
        }

        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);

        // Check footer: last 16 bytes = 8 bytes length + 8 bytes magic
        if (fileSize < 16) {
            fprintf(stderr, "Error: executable too small.\n");
            fclose(f);
            return 1;
        }

        fseek(f, fileSize - 16, SEEK_SET);
        unsigned long long payloadSize = 0;
        char footerMagic[8];

        if (fread(&payloadSize, sizeof(unsigned long long), 1, f) != 1 ||
            fread(footerMagic, 1, 8, f) != 8) {
            fprintf(stderr, "Error: failed to read payload footer.\n");
            fclose(f);
            return 1;
        }

        if (memcmp(footerMagic, MAGIC_FOOTER, 8) != 0) {
            fprintf(stderr, "Error: invalid bundle magic footer.\n");
            fclose(f);
            return 1;
        }

        long payloadOffset = fileSize - 16 - (long)payloadSize;
        if (payloadOffset < 0) {
            fprintf(stderr, "Error: invalid payload offset.\n");
            fclose(f);
            return 1;
        }

        // Write bundle.tar.gz
        char bundlePath[MAX_PATH];
        snprintf(bundlePath, MAX_PATH, "%s\\bundle.tar.gz", appDir);

        FILE *outF = fopen(bundlePath, "wb");
        if (!outF) {
            fprintf(stderr, "Error: cannot create %s\n", bundlePath);
            fclose(f);
            return 1;
        }

        fseek(f, payloadOffset, SEEK_SET);
        char buf[65536];
        unsigned long long remaining = payloadSize;
        while (remaining > 0) {
            size_t toRead = (remaining > sizeof(buf)) ? sizeof(buf) : (size_t)remaining;
            size_t n = fread(buf, 1, toRead, f);
            if (n == 0) break;
            fwrite(buf, 1, n, outF);
            remaining -= n;
        }
        fclose(outF);
        fclose(f);

        printf("==> Extracting bundle archive to %s...\n", appDir);

        // Extract with tar.exe (built-in on Windows 10)
        char tarCmd[MAX_PATH * 3];
        snprintf(tarCmd, sizeof(tarCmd), "tar.exe -xzf \"%s\" -C \"%s\"", bundlePath, appDir);

        STARTUPINFOA siTar;
        PROCESS_INFORMATION piTar;
        ZeroMemory(&siTar, sizeof(siTar));
        siTar.cb = sizeof(siTar);
        ZeroMemory(&piTar, sizeof(piTar));

        if (CreateProcessA(NULL, tarCmd, NULL, NULL, FALSE, 0, NULL, appDir, &siTar, &piTar)) {
            WaitForSingleObject(piTar.hProcess, 60000);
            CloseHandle(piTar.hProcess);
            CloseHandle(piTar.hThread);
        } else {
            fprintf(stderr, "Error: tar.exe execution failed.\n");
        }

        // Remove temporary archive
        DeleteFileA(bundlePath);
        printf("==> Extraction complete.\n");
    }

    // 2. Launch Java process
    char cmdLine[MAX_PATH * 3];
    snprintf(cmdLine, sizeof(cmdLine), "\"%s\" -jar \"%s\" --server.port=8080", javaExe, jarFile);

    printf("==> Launching Java application: %s\n", cmdLine);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, appDir, &si, &pi)) {
        fprintf(stderr, "Error: failed to launch Java process (code: %lu)\n", GetLastError());
        return 1;
    }

    // 3. Wait for Java process to exit so watchdog tracks ZemoServer.exe as running
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exitCode;
}
