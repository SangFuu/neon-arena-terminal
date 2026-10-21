#include <stdio.h>
#include <string.h>
#include <windows.h>
static HANDLE input, output;
static void key(WORD vk, int down) {
    INPUT_RECORD e = {0};
    DWORD n;
    e.EventType = KEY_EVENT;
    e.Event.KeyEvent.bKeyDown = down;
    e.Event.KeyEvent.wVirtualKeyCode = vk;
    e.Event.KeyEvent.wRepeatCount = 1;
    WriteConsoleInputW(input, &e, 1, &n);
}
static void tap(WORD vk) {
    key(vk, 1);
    key(vk, 0);
    Sleep(150);
}
static int status(const char *expected) {
    char line[75] = {0};
    DWORD n;
    ReadConsoleOutputCharacterA(output, line, 74, (COORD){0, 31}, &n);
    return strstr(line, expected) != NULL;
}
static void arena(char *buffer) {
    DWORD n;
    int p = 0;
    for (int y = 5; y < 25; y++) {
        ReadConsoleOutputCharacterA(output, buffer + p, 60, (COORD){0, (SHORT)y}, &n);
        p += 60;
    }
    buffer[p] = 0;
}
int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    FreeConsole();
    if (!AllocConsole())
        return 2;
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    input = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);
    output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, 0, NULL);
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi;
    si.cb = sizeof si;
    SetHandleInformation(input, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(output, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = input;
    si.hStdOutput = output;
    si.hStdError = output;
    char command[2048];
    snprintf(command, sizeof command, "\"%s\"", argv[1]);
    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return 3;
    int ok = 1;
    Sleep(500);
    ok &= status("ENTREE");
    tap(VK_RETURN);
    ok &= status("Survivez");
    char before[1201], after[1201];
    arena(before);
    key('D', 1);
    Sleep(250);
    key('D', 0);
    Sleep(100);
    arena(after);
    ok &= strcmp(before, after) != 0;
    tap('P');
    ok &= status("PAUSE");
    key('P', 1);
    key('P', 1);
    key('P', 1);
    Sleep(120);
    ok &= status("Survivez");
    key('P', 0);
    tap('P');
    ok &= status("PAUSE");
    tap(VK_RETURN);
    ok &= status("Survivez");
    tap(VK_SPACE);
    tap('F');
    tap('E');
    tap('R');
    ok &= status("Survivez");
    tap('X');
    if (WaitForSingleObject(pi.hProcess, 3000) != WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess, 9);
        ok = 0;
    }
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    ok &= code == 0;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(input);
    CloseHandle(output);
    FreeConsole();
    printf("Console Windows : clavier reel, mouvement, anti-repetition pause et sortie : %s\n",
           ok ? "OK" : "ECHEC");
    return ok ? 0 : 1;
}
