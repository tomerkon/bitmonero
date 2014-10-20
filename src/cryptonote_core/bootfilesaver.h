#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include "cryptonote_basic.h"
#include "blockchain_storage.h"

using namespace cryptonote;

class bootfilesaver
{
public:
    bool store_blockchain_raw(cryptonote::blockchain_storage* cs, cryptonote::tx_memory_pool* txp);

protected:
	blockchain_storage* m_blockchain_storage;
    tx_memory_pool* m_tx_pool;
    typedef std::vector<char> buffer_type;
    std::ofstream * m_raw_data_file;
	boost::archive::binary_oarchive * m_raw_archive;
    buffer_type m_buffer;
    boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type> > * m_output_stream;
	bool open_raw_file_for_write();
    bool close_raw_file();
    void write_block_to_raw_file(block& block);
	void serialize_block_to_text_buffer(const block& block);
	void buffer_serialize_tx(const transaction& tx);
	void buffer_write_num_txs(const std::list<transaction> txs);
	void flush_chunk();
};
