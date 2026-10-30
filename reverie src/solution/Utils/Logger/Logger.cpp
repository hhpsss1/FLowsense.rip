#include <shared.h>

void Logger_t::Attach(const char* title) {
	if (!AllocConsole())
		return;

    freopen_s(&this->file_out, xs("CONOUT$"), xs("w"), stdout);
    freopen_s(&this->file_in, xs("CONIN$"), xs("r"), stdin);

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SetConsoleTitleA(title);
}

void Logger_t::Detach() {
    if (this->file_out)
        fclose(this->file_out);

    if (this->file_in)
        fclose(this->file_in);

    FreeConsole();

    if (HWND console_window = GetConsoleWindow())
        PostMessageA(console_window, WM_CLOSE, 0, 0);
}
