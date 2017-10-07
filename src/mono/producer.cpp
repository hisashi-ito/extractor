
#include "producer.h"
#include "warcfilter.h"
#include "langcollector.h"
#include "../langsplit/langsplitfilter.h"
#include "../utils/curldownloader.h"
#include "../utils/common.h"
#include "../utils/compression.h"
#include "../utils/logging.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/filter/aggregate.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/null.hpp>

#include <string>


namespace mono {

    void run_producer(shared_vector_string *files_to_process,
                      utils::progress *prog, bool curl, std::string output_folder,
                      utils::compression_option input_compr,
                      utils::compression_option output_compr) {

      while (files_to_process->size() > 0) {
        std::string path = files_to_process->pop();
        if (curl) {
          producer_curl(path, output_folder, input_compr, output_compr);
        } else {
          producer_file(path, output_folder, input_compr, output_compr);
        }

        prog->increment();

      }
    }

    void
    producer_file(std::string path, std::string output_folder, utils::compression_option input_compr,
                  utils::compression_option output_compr) {

      std::ios_base::openmode flags = std::ofstream::in;
      if (input_compr == utils::gzip) {
        flags |= std::ofstream::binary;
      }

      std::ifstream input_file(path, flags);
      if (!boost::filesystem::exists(path)) {
        std::cerr << "File not found!" << std::endl;
        return;
      }

      boost::iostreams::filtering_streambuf<boost::iostreams::input> qin(input_file);
      boost::iostreams::filtering_streambuf<boost::iostreams::output> qout;

      add_decompression(&qout, input_compr);

      qout.push(WARCFilter());
      qout.push(LangsplitFilter(output_folder));
      qout.push(LangCollectorFilter(output_folder, output_compr));
      qout.push(boost::iostreams::null_sink());

      boost::iostreams::copy(qin, qout);

    }

    void producer_curl(std::string url, std::string output_folder, utils::compression_option input_compr,
                       utils::compression_option output_compr) {

      boost::iostreams::filtering_streambuf<boost::iostreams::output> qout;

      if (input_compr == utils::gzip) {
        qout.push(boost::iostreams::gzip_decompressor());
      }

      qout.push(WARCFilter());
      qout.push(LangsplitFilter(output_folder));
      qout.push(LangCollectorFilter(output_folder, output_compr));
      qout.push(boost::iostreams::null_sink());

      std::ostream oqout(&qout);
      HTTPDownloader downloader;
      downloader.download(url, &oqout);

    }

}
