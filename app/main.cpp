/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/common.h"
#include "proxypp/core/tcp_server.h"
#include "proxypp/helper/cli_helper.h"
#include "proxypp/log/log.h"
#include "proxypp/result.h"
#include "proxypp/rule/rule_file.h"
#include "string"
#include <CLI/CLI.hpp>
#include <boost/dll.hpp>
#include <filesystem>
#include <format>
#include <fstream>

namespace
{
  struct GlobalOpts
  {
    std::string log_level = "info";
  };

  struct HttpOpts
  {
    std::string bind = "127.0.0.1";
    std::size_t port = 3000;
    std::string ready_file;
    std::string rule_file;
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

  struct CliHandles
  {
    CLI::App* http = nullptr;
    CLI::Option* rule_file_opt = nullptr;
  };

  CliHandles ConfigureCli(CLI::App& app, AppOpts& opts)
  {
    app.name("proxy++");

    app.add_option("--log-level", opts.global.log_level, "Set log level")
      ->default_val(opts.global.log_level)
      ->check(CLI::IsMember(
        { "trace", "debug", "info", "error", "critical", "off" }));

    app.set_version_flag("-V,--version", "V1.0.0");

    CLI::App* http_command = app.add_subcommand("http", "start http proxy");
    http_command->fallthrough();
    http_command->add_option("-b,--bind", opts.http.bind, "bind address");
    http_command->add_option("-p,--port", opts.http.port, "bind port")
      ->check(CLI::Range(0, 65535));
    http_command
      ->add_option("--ready-file", opts.http.ready_file,
                   "Path to a file created after proxy++ starts listening")
      ->check(CLI::NonexistentPath);
    CLI::Option* rule_file_opt
      = http_command
          ->add_option("-r,--rule-file", opts.http.rule_file, "rule file path")
          ->check(CLI::ExistingFile);
    app.require_subcommand(1);
    return { .http = http_command, .rule_file_opt = rule_file_opt };
  }

  proxypp::Result<proxypp::rule::Config>
  LoadRuleConfig(const std::filesystem::path& rule_file_path)
  {
    const auto load_rules = proxypp::rule::LoadRulesFromFile(rule_file_path);
    if(!load_rules)
      {
        return proxypp::Unexpected(load_rules.error());
      }
    return *load_rules;
  }

  proxypp::Result<std::shared_ptr<proxypp::rule::RuleEngine>> InitRuleEngine()
  {
    auto rule_engine_result = proxypp::rule::RuleEngine::Create();
    if(!rule_engine_result)
      {
        return proxypp::Unexpected(rule_engine_result.error());
      }
    auto rule_engine = std::make_shared<proxypp::rule::RuleEngine>(
      std::move(*rule_engine_result));
    return rule_engine;
  }

  proxypp::Result<void>
  WriteReadyFile(const std::filesystem::path& ready_file,
                 std::string_view host, std::uint16_t port)
  {
    const auto app_logger = proxypp::log::Get(proxypp::log::Module::app);

    auto temporary_file = ready_file;
    temporary_file += ".tmp";

    {
      std::ofstream output { temporary_file,
                             std::ios::binary | std::ios::trunc };
      if(!output)
        {
          app_logger->error("failed to open ready file: path=\"{}\"",
                            temporary_file.string());

          return proxypp::Unexpected(proxypp::Error(
            proxypp::Errc::InternalError, temporary_file.string()));
        }

      output << "{\n"
             << "  \"host\": \"" << host << "\",\n"
             << "  \"port\": " << port << "\n"
             << "}\n";

      output.flush();
      if(!output)
        {
          app_logger->error("failed to write ready file: path=\"{}\"",
                            temporary_file.string());
          return proxypp::Unexpected(proxypp::Error {
            proxypp::Errc::FileOperationFailed,
            std::format("write file {} failed", temporary_file.string()) });
        }
    }
    try
      {
        std::filesystem::rename(temporary_file, ready_file);
        app_logger->info("ready file created: path=\"{}\"",
                         std::filesystem::absolute(ready_file).string());

        return {};
      }
    catch(const std::exception& e)
      {
        app_logger->error(
          R"(failed to rename file: "{}" -> "{}", message="{}" )",
          temporary_file.string(), ready_file.string(), e.what());

        return proxypp::Unexpected(
          proxypp::Error(proxypp::Errc::InternalError,
                         std::format("rename file failed, {}", e.what())));
      }
  }
}

int main(int argc, char** argv)
{
#if BOOST_OS_WINDOWS
  SetConsoleOutputCP(936);
#endif

  CLI::App app;
  argv = app.ensure_utf8(argv);

  AppOpts opts;
  auto cli_handlers = ConfigureCli(app, opts);

  CLI11_PARSE(app, argc, argv);

  proxypp::log::Init("./logs");
  proxypp::log::SetAllLevels(spdlog::level::from_str(opts.global.log_level));

  const auto app_logger = proxypp::log::Get(proxypp::log::Module::app);
  app_logger->debug("using log level {}", opts.global.log_level);

  const auto rule_file_path = proxypp::helper::cli::ResolveRuleFilePath(
    *cli_handlers.rule_file_opt, opts.http.rule_file);

  auto rule_config = LoadRuleConfig(rule_file_path);
  if(!rule_config)
    {
      app_logger->error("program exit with failure");
      return EXIT_FAILURE;
    }

  auto rule_engine = InitRuleEngine();
  if(!rule_engine)
    {
      app_logger->error("program exit with failure");
      return EXIT_FAILURE;
    }

  asio::io_context io;

  if(app.got_subcommand(cli_handlers.http))
    {
      proxypp::core::TcpServerOptions options { .address = opts.http.bind,
                                                .port = opts.http.port,
                                                .http_rule_config
                                                = rule_config->http,
                                                .rule_engine = *rule_engine };

      auto server = std::make_shared<proxypp::core::TcpServer>(
        io.get_executor(), options);

      auto start_result = server->Start();
      if(!start_result)
        {
          app_logger->error("program exit with failure");
          return EXIT_FAILURE;
        }

      if(!opts.http.ready_file.empty())
        {
          auto write_result = WriteReadyFile(
            opts.http.ready_file, start_result->address, start_result->port);
          if(!write_result)
            {
              app_logger->error("program exit with failure");
              return EXIT_FAILURE;
            }
        }

      app_logger->info("proxy++ started");
    }

  io.run();

  return 0;
}
