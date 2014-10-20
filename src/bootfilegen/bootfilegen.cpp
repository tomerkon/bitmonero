#include "include_base_utils.h"
#include "version.h"

using namespace epee;

#include <boost/program_options.hpp>

#include "console_handler.h"
#include "p2p/net_node.h"
#include "cryptonote_config.h"
#include "cryptonote_core/blockchain_storage.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "version.h"
#include "cryptonote_core/bootfilesaver.h"
#include <iostream>
#include <exception>
#include <string>
#if defined(WIN32)
#include <crtdbg.h>
#endif

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", "Specify configuration file", std::string(CRYPTONOTE_NAME ".conf")};
  const command_line::arg_descriptor<bool>        arg_os_version  = {"os-version", ""};
  const command_line::arg_descriptor<std::string> arg_log_file    = {"log-file", "", ""};
  const command_line::arg_descriptor<int>         arg_log_level   = {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<bool>        arg_console     = {"no-console", "Disable daemon console commands"};
  const command_line::arg_descriptor<bool>        arg_testnet_on  = {
      "testnet"
    , "Run on testnet. The wallet must be launched with --testnet flag."
    , false
    };
}

using namespace cryptonote;

int main(int argc, char* argv[])
{
  string_tools::set_module_name_and_folder(argv[0]);
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  LOG_PRINT_L0("Starting...");
  boost::filesystem::path default_data_path {tools::get_default_data_dir()};
  boost::filesystem::path default_testnet_data_path {default_data_path / "testnet"};

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, arg_os_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, default_data_path.string());
  command_line::add_arg(desc_cmd_only, command_line::arg_testnet_data_dir, default_testnet_data_path.string());
  command_line::add_arg(desc_cmd_only, arg_config_file);

  command_line::add_arg(desc_cmd_sett, arg_log_file);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_console);
  command_line::add_arg(desc_cmd_sett, arg_testnet_on);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);

    return true;
  });
  if (!r)
    return 1;

  //initialize core here
  LOG_PRINT_L0("Initializing core storage...");
  bool testnet_mode = false;
  std::string m_config_folder;
  auto data_dir_arg = testnet_mode ? command_line::arg_testnet_data_dir : command_line::arg_data_dir;
  m_config_folder = command_line::get_arg(vm, data_dir_arg);
  printf("config folder is %s\n", m_config_folder.c_str());
  blockchain_storage* _blockchain_storage = NULL;
  tx_memory_pool m_mempool(*_blockchain_storage);
  _blockchain_storage = new blockchain_storage(m_mempool);
  r = _blockchain_storage->init(m_config_folder, testnet_mode);
  CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");
  LOG_PRINT_L0("CoreStorage initialized OK");
  LOG_PRINT_L0("saving blockchain raw data...");
  bootfilesaver bfs;
  r = bfs.store_blockchain_raw(_blockchain_storage, &m_mempool);
  CHECK_AND_ASSERT_MES(r, false, "Failed to save blockchain raw storage");
  LOG_PRINT_L0("CoreStorage raw data saved OK");
}

