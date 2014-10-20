#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include "bootfilesaver.h"
#include "cryptonote_boost_serialization.h"


#include <algorithm>
#include <cstdio>
#include <fstream>
#include <boost/iostreams/copy.hpp>
#include <atomic>

#define NUM_BLOCKS_PER_CHUNK 1
#define BUFFER_SIZE 100000
#define STR_LENGTH_OF_INT 9
#define STR_FORMAT_OF_INT "%09d"
static char largebuffer[BUFFER_SIZE];


bool bootfilesaver::open_raw_file_for_write()
{
    std::string file_path = "/home/ub/.bitmonero/";
	file_path += CRYPTONOTE_BLOCKCHAINDATA_RAW_FILENAME;
    m_raw_data_file = new std::ofstream();
    m_raw_data_file->open(file_path , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if (m_raw_data_file->fail())
      return false;

	m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type> > (m_buffer);
    m_raw_archive = new boost::archive::binary_oarchive (*m_output_stream);
    if (m_raw_archive == NULL)
      return false;

    return true;
}

/*
void bootfilesaver::flush_chunk2()
{
  //flush chunk to m_buffer
  m_output_stream -> flush();

  delete m_raw_archive;
  delete m_output_stream;

  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
///  in.push(boost::iostreams::bzip2_compressor());
  std::istringstream uncompressed_stream(std::string(m_buffer.begin(), m_buffer.end()));
  in.push(uncompressed_stream);
///  std::ostringstream compressed_stream;
///  std::streamsize compressed_size = boost::iostreams::copy(in, compressed_stream);

  char buffer[STR_LENGTH_OF_INT + 1];
  sprintf(buffer, STR_FORMAT_OF_INT, (int) compressed_size);
  m_raw_data_file->write (buffer, STR_LENGTH_OF_INT);
  printf("compressed chunk lengh = %s\n", buffer);
  *m_raw_data_file << in;
  m_raw_data_file->flush();

  m_buffer.clear();
  m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type> > (m_buffer);
  m_raw_archive = new boost::archive::binary_oarchive (*m_output_stream);
}
*/

void bootfilesaver::flush_chunk()
{
  m_output_stream -> flush();
  char buffer[STR_LENGTH_OF_INT + 1];
  sprintf(buffer, STR_FORMAT_OF_INT, (int) m_buffer.size());
  m_raw_data_file->write (buffer, STR_LENGTH_OF_INT);
  printf("chunk lengh = %s\n", buffer);
  std::copy(m_buffer.begin(), m_buffer.end(), std::ostreambuf_iterator<char>(*m_raw_data_file));
  m_raw_data_file->flush();
 
  m_buffer.clear();
  delete m_raw_archive;
  delete m_output_stream;
  m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type> > (m_buffer);
  m_raw_archive = new boost::archive::binary_oarchive (*m_output_stream);
}

void bootfilesaver::serialize_block_to_text_buffer(const block& block)
{
//todo: implement
    *m_raw_archive << block;
}

void bootfilesaver::buffer_serialize_tx(const transaction& tx)
{
//todo: implement
    *m_raw_archive << tx;
}

void bootfilesaver::buffer_write_num_txs(const std::list<transaction> txs)
{
//todo: implement
	int n = txs.size();
    *m_raw_archive << n;
}

void bootfilesaver::write_block_to_raw_file(block& block)
{
    serialize_block_to_text_buffer(block);
    std::list<transaction> txs;

    // put coinbase transaction first
    transaction coinbase_tx = block.miner_tx;
    crypto::hash coinbase_tx_hash = get_transaction_hash(coinbase_tx);
    transaction* cb_tx_full = m_blockchain_storage -> get_tx(coinbase_tx_hash);
    if (cb_tx_full != NULL) {
      txs.push_back(*cb_tx_full);
    }

    // now add all regular transactions
    BOOST_FOREACH(const auto& tx_id, block.tx_hashes)
    {
      transaction* pTx = m_blockchain_storage -> get_tx(tx_id);
      if(pTx == NULL)
      {
        transaction tx;
        if(m_tx_pool->get_transaction(tx_id, tx))
          txs.push_back(tx);
      }
      else
        txs.push_back(*pTx);
    }

    // serialize all txs to the persistant storage
	buffer_write_num_txs(txs);
    BOOST_FOREACH(const auto& tx, txs)
    {
      buffer_serialize_tx(tx);
    }
}

bool bootfilesaver::bootfilesaver::close_raw_file()
{
    if (m_raw_data_file->fail())
      return false;

    m_raw_data_file->flush();
	delete[] m_raw_archive;
	delete[] m_raw_data_file;
	delete[] m_output_stream;
//  delete[] m_buffer;
    return true;
}


bool bootfilesaver::store_blockchain_raw(blockchain_storage* _blockchain_storage, tx_memory_pool* _tx_pool)
{
  m_blockchain_storage = _blockchain_storage;
  m_tx_pool = _tx_pool;
  LOG_PRINT_L0("Storing blocks raw data...");
  if (!open_raw_file_for_write()) {
    LOG_PRINT_L0("failed to open raw file for write");
    return false;
  }
  size_t height;
  block b;
  for (height=0; height < m_blockchain_storage -> get_current_blockchain_height(); ++height) 
  {
    crypto::hash hash = m_blockchain_storage -> get_block_id_by_height(height);
    m_blockchain_storage -> get_block_by_hash(hash, b);
    write_block_to_raw_file(b);
    if (height % NUM_BLOCKS_PER_CHUNK == 0) {
      flush_chunk();
    }
  }
  if (height % NUM_BLOCKS_PER_CHUNK != 0) {
    flush_chunk();
  }

  return true;
}

