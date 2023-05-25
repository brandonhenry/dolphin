// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "UICommon/CommandLineParse.h"

#include <list>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>

#include <OptionParser.h>
#include "Core/Config/NetplaySettings.h"
#include "Core/ConfigManager.h"

#include "Common/Config/Config.h"
#include "Common/StringUtil.h"
#include "Common/Version.h"
#include "Core/Config/MainSettings.h"

namespace CommandLineParse
{
class CommandLineConfigLayerLoader final : public Config::ConfigLayerLoader
{
public:
  CommandLineConfigLayerLoader(const std::list<std::string>& args, const std::string& video_backend,
                               const std::string& audio_backend, bool batch, bool debugger)
      : ConfigLayerLoader(Config::LayerType::CommandLine)
  {
    if (!video_backend.empty())
      m_values.emplace_back(Config::MAIN_GFX_BACKEND.GetLocation(), video_backend);

    if (!audio_backend.empty())
    {
      m_values.emplace_back(Config::MAIN_DSP_HLE.GetLocation(),
                            ValueToString(audio_backend == "HLE"));
    }

    // Batch mode hides the main window, and render to main hides the render window. To avoid a
    // situation where we would have no window at all, disable render to main when using batch mode.
    if (batch)
      m_values.emplace_back(Config::MAIN_RENDER_TO_MAIN.GetLocation(), ValueToString(false));

    if (debugger)
      m_values.emplace_back(Config::MAIN_ENABLE_DEBUGGING.GetLocation(), ValueToString(true));

    // Arguments are in the format of <System>.<Section>.<Key>=Value
    for (const auto& arg : args)
    {
      std::istringstream buffer(arg);
      std::string system_str, section, key, value;
      std::getline(buffer, system_str, '.');
      std::getline(buffer, section, '.');
      std::getline(buffer, key, '=');
      std::getline(buffer, value, '=');
      std::optional<Config::System> system = Config::GetSystemFromName(system_str);
      if (system)
      {
        m_values.emplace_back(
            Config::Location{std::move(*system), std::move(section), std::move(key)},
            std::move(value));
      }
    }
  }

  void Load(Config::Layer* layer) override
  {
    for (auto& value : m_values)
    {
      layer->Set(std::get<0>(value), std::get<1>(value));
    }
  }

  void Save(Config::Layer* layer) override
  {
    // Save Nothing
  }

private:
  std::list<std::tuple<Config::Location, std::string>> m_values;
};

std::unique_ptr<optparse::OptionParser> CreateParser(ParserOptions options)
{
  auto parser = std::make_unique<optparse::OptionParser>();
  parser->usage("usage: %prog [options]... [FILE]...").version(Common::GetScmRevStr());

  parser->add_option("-u", "--user").action("store").help("User folder path");
  parser->add_option("-m", "--movie").action("store").help("Play a movie file");
  parser->add_option("-e", "--exec")
      .action("append")
      .metavar("<file>")
      .type("string")
      .help("Load the specified file");
  parser->add_option("-n", "--nand_title")
      .action("store")
      .metavar("<16-character ASCII title ID>")
      .type("string")
      .help("Launch a NAND title");
  parser->add_option("-C", "--config")
      .action("append")
      .metavar("<System>.<Section>.<Key>=<Value>")
      .type("string")
      .help("Set a configuration option");
  parser->add_option("-s", "--save_state")
      .action("store")
      .metavar("<file>")
      .type("string")
      .help("Load the initial save state");


  parser->add_option("-p", "--netplay-port").action("store").help("Netplay port number");
  parser->add_option("-s", "--netplay-server").action("store").help("Netplay server address");
  parser->add_option("-N", "--netplay-nickname").action("store").help("Netplay session name");
  parser->add_option("-P", "--netplay-password").action("store").help("Netplay session password");


  if (options == ParserOptions::IncludeGUIOptions)
  {
    parser->add_option("-d", "--debugger")
        .action("store_true")
        .help("Show the debugger pane and additional View menu options");
    parser->add_option("-l", "--logger").action("store_true").help("Open the logger");
    parser->add_option("-b", "--batch")
        .action("store_true")
        .help("Run Dolphin without the user interface (Requires --exec or --nand-title)");
    parser->add_option("-c", "--confirm").action("store_true").help("Set Confirm on Stop");
  }

  parser->set_defaults("video_backend", "");
  parser->set_defaults("audio_emulation", "");
  parser->add_option("-v", "--video_backend").action("store").help("Specify a video backend");
  parser->add_option("-a", "--audio_emulation")
      .choices({"HLE", "LLE"})
      .help("Choose audio emulation from [%choices]");

  return parser;
}

static void AddConfigLayer(const optparse::Values& options)
{
  std::list<std::string> config_args;
  if (options.is_set_by_user("config"))
    config_args = options.all("config");

  Config::AddLayer(std::make_unique<CommandLineConfigLayerLoader>(
      std::move(config_args), static_cast<const char*>(options.get("video_backend")),
      static_cast<const char*>(options.get("audio_emulation")),
      static_cast<bool>(options.get("batch")), static_cast<bool>(options.get("debugger"))));

  std::unique_ptr<Config::ConfigLayerLoader> loader = std::make_unique<Config::CommandLineConfigLayerLoader>(options);

  if (options.HasOption("netplay-server"))
  {
    std::string server = options.Get<std::string>("netplay-server");
    Config::SetBaseOrCurrent(Config::NETPLAY_TRAVERSAL_SERVER, server);
  }
  
  if (options.HasOption("netplay-port"))
  {
    u16 port = options.Get<u16>("netplay-port");
    Config::SetBaseOrCurrent(Config::NETPLAY_HOST_PORT, port);
  }
  
  if (options.HasOption("netplay-nickname"))
  {
    std::string nickname = options.Get<std::string>("netplay-nickname");
    Config::SetBaseOrCurrent(Config::NETPLAY_NICKNAME, nickname);
  }

  if (options.HasOption("netplay-password"))
  {
    std::string password = options.Get<std::string>("netplay-password");
    Config::SetBaseOrCurrent(Config::NETPLAY_INDEX_PASSWORD, password);
  }

  Config::AddLayer(std::move(loader));
}

optparse::Values& ParseArguments(optparse::OptionParser* parser, int argc, char** argv)
{
  optparse::Values& options = parser->parse_args(argc, argv);
  AddConfigLayer(options);
  return options;
}

optparse::Values& ParseArguments(optparse::OptionParser* parser,
                                 const std::vector<std::string>& arguments)
{
  optparse::Values& options = parser->parse_args(arguments);
  AddConfigLayer(options);
  return options;
}
}  // namespace CommandLineParse
