#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "bootfileloader.h"
#include "include_base_utils.h"
#include "cryptonote_basic.h"
#include "cryptonote_format_utils.h"
#include "verification_context.h"
#include "blockchain_storage.h"
#include "cryptonote_boost_serialization.h"

#define NUM_BLOCKS_PER_CHUNK 1
#define BUFFER_SIZE 100000
#define STR_LENGTH_OF_INT 9
#define STR_FORMAT_OF_INT "%09d"

static char largebuffer[BUFFER_SIZE];
using namespace cryptonote;

bool bootfileloader::load_from_raw_file(void* bcs, const std::string& raw_file_name)
{

//  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_4);
  boost::filesystem::path raw_file_path(raw_file_name);
  boost::system::error_code ec;
  if (!boost::filesystem::exists(raw_file_path, ec)) 
  {
      return false;
  }
  std::ifstream data_file;  
  data_file.open( raw_file_name, std::ios_base::binary | std::ifstream::in);
  int h = 0;
  if (data_file.fail())
    return false;
  LOG_PRINT_L0("Loading blockchain from raw file...");
  char buffer1[STR_LENGTH_OF_INT + 1];
  while (true) 
  {
    int chunkSize;
    data_file.read (buffer1, STR_LENGTH_OF_INT);
    if (!data_file) {
      LOG_PRINT_L0("end of rawfile reached");
      return true;
    }
    buffer1[STR_LENGTH_OF_INT] = '\0';
    chunkSize = atoi(buffer1);
    data_file.read (largebuffer, chunkSize);
    if (!data_file) {
      LOG_PRINT_L0("end of rawfile reached");
      return true;
    }
//  LOG_PRINT_L0("read "<< chunkSize << " bytes into buffer");
    boost::iostreams::basic_array_source<char> device(largebuffer, chunkSize);
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
    boost::archive::binary_iarchive a(s);

    for (int chunk_ind = 0; chunk_ind < NUM_BLOCKS_PER_CHUNK; chunk_ind ++)
    {
      if (h % 10000 == 0) {
        LOG_PRINT_L0("loading block height " << h);
      } else {
        LOG_PRINT_L1("loading block height " << h);
      }
      block b;
      try {
        a >> b;
      } catch (const std::exception& e) {
        break;
      }
      int num_txs;
      try {
        a >> num_txs;
      } catch (const std::exception& e) {} // catch when end-of-file reached.
      for(int tx_num = 1; tx_num <= num_txs; tx_num++)
      {
        LOG_PRINT_L1("tx_num " << tx_num);
        transaction tx;
        try {
		  a >> tx;
 	    } catch (const std::exception& e) {} // catch when end-of-file reached.
        if (tx_num == 1) {
            continue; // coinbase transaction. no need to insert to tx_pool.
        }
        crypto::hash h = null_hash;
        size_t blob_size = 0;
        get_transaction_hash(tx, h, blob_size);
        tx_verification_context tvc = AUTO_VAL_INIT(tvc);
        bool r = true;
///        r = m_tx_pool.add_tx(tx, tvc, true);
        if (!r) 
        { 
          LOG_PRINT_RED_L0("failed to add transaction to transaction pool");
          return false;
        }
      }
      block_verification_context bvc = boost::value_initialized<block_verification_context>();
      blockchain_storage* bcs1 = (blockchain_storage*) bcs;
      bcs1 -> add_new_block(b, bvc);

      if (bvc.m_verifivation_failed || ! bvc.m_added_to_main_chain) {
  	    LOG_PRINT_L0("Failed to add block to blockchain, height = " << h);
  	    LOG_PRINT_L0("skipping rest of raw file");
  		return true;
      }
      h++;
    }
  }
  LOG_PRINT_L0( h << "blocks read from raw file and added to blockchain");

  return true;
}


