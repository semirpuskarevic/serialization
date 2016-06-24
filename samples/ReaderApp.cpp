#include "Types.h"
#include <boost/asio/buffer.hpp>
#include <iostream>
#include <reader.hpp>
#include <fstream>
// #include <cfloat>

#include <iterator>
#include <algorithm>

namespace asio = boost::asio;
using namespace detail;
using namespace std::chrono;
using namespace date;

std::pair<TestTypes::FixedHeader, asio::const_buffer> readHeader(
    asio::const_buffer buf) {
    return read<TestTypes::FixedHeader>(buf);
}

asio::const_buffer readAndHandleMessage(asio::const_buffer buf,
                                        TestTypes::MsgType msgType) {
    switch (msgType) {
        case TestTypes::MsgType::DataRequest: {
            TestTypes::DataRequest req;
            std::tie(req, buf) = read<decltype(req)>(buf);
            std::cout << "Symbol: " << req.symbol << ", "
                      << "depth: " << req.depth << '\n';
            break;
        }
        case TestTypes::MsgType::DataSeries: {
            TestTypes::DataSeries data;
            std::tie(data, buf) = read<decltype(data)>(buf);
            for (auto i = 0u; i < data.dataPoints.size(); ++i) {
                std::cout << "Data value: " << data.dataPoints.at(i) << ", "
                          << "Time point: " << data.timePoints.at(i) << '\n';
            }
            break;
        }
        default: { std::cout << "Unhandled message type" << '\n'; }
    }
    return buf;
}

int main(int argc, char* argv[]) {
    std::array<char, 1024> mainBuf;
    {

#if defined __EMSCRIPTEN__ && NODE
        EM_ASM(
                FS.mkdir('working');
                FS.mount(NODEFS, {root: '.'}, 'working');
                FS.chdir('working');
              );
#endif

        std::ifstream serializedData;
        serializedData.open("data.bin", std::ios::in | std::ios::binary);
        
        // std::istream_iterator<char> eos;
        // std::istream_iterator<char> iit (serializedData);
        // std::copy(iit, eos, mainBuf.data());
        serializedData.read(mainBuf.data(), mainBuf.max_size());
        if(!serializedData && serializedData.eof())
        {
            std::cout<<"Buffer size: " << mainBuf.size()<<'\n';
        }

        TestTypes::FixedHeader head;
        asio::const_buffer buf{asio::buffer(mainBuf)};

        for(auto i=0u; i<2u; ++i)
        {
            std::tie(head, buf) = readHeader(buf);
            buf = readAndHandleMessage(buf, head.msgType);
        }
        serializedData.close();
    }

    std::cout<<"Sizeof int64_ type: " << sizeof(int64_t) <<'\n';
    std::cout<<"Sizeof time_t type: " << sizeof(std::time_t) <<'\n';

    return 0;
}
