#include "proxypp/common.h"
#include "proxypp/core/tcp_server.h"
#include "proxypp/log/log.h"

#include <CLI/CLI.hpp>

struct GlobalOpts
{
  bool verbose = false;
};

struct HttpOpts
{
  std::string bind = "127.0.0.1";
  std::size_t port = 3000;
};

struct SocksOpts
{
  std::string bind = "127.0.0.1";
  std::size_t port = 3000;
};

struct AppOpts
{
  GlobalOpts global;
  HttpOpts http;
  SocksOpts socks;
};

int main(int argc, char **argv)
{
#if BOOST_OS_WINDOWS
  SetConsoleOutputCP(936);
#endif

  // init loggers
  proxypp::log::Init();
  proxypp::log::SetAllLevels(spdlog::level::debug);

  CLI::App app;
  app.name("proxy++");
  argv = app.ensure_utf8(argv);

  AppOpts opts;
  app.add_flag("-v,--verbose", opts.global.verbose, "run in verbose mode");
  app.set_version_flag("-V,--version", "V1.0.0");

  auto http = app.add_subcommand("http", "start http proxy");
  http->add_option("-b,--bind", opts.http.bind, "bind address");
  http->add_option("-p,--port", opts.http.port, "bind port");

  auto socks = app.add_subcommand("socks", "start socks proxy");
  socks->add_option("-b,--bind", opts.socks.bind, "bind address");
  socks->add_option("-p,--port", opts.socks.port, "bind port");

  app.require_subcommand(1);

  CLI11_PARSE(app, argc, argv);

  asio::io_context io;
  if(app.got_subcommand(http))
    {
      auto server = std::make_shared<proxypp::core::TcpServer>(
        io.get_executor(), opts.http.bind, opts.http.port);
      server->Run();
    }

  if(app.got_subcommand(socks)) {}

  io.run();

  return 0;
}
