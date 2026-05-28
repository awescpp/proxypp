#include <CLI/CLI.hpp>

int main(int argc, char **argv) {

#ifdef BOOST_OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    CLI::App app{"a simple proxy tool for http/https/socks and so on."};
    argv = app.ensure_utf8(argv);
    CLI11_PARSE(app, argc, argv);
    return 0;
}
