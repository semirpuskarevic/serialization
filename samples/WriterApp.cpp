#include "Types.h"
#include <boost/asio/buffer.hpp>
#include <iostream>
#include <writer.hpp>
#include <fstream>
// #include <cfloat>

namespace asio = boost::asio;
using namespace detail;
using namespace std::chrono;
using namespace date;

template <typename T>
asio::mutable_buffer writeMessage(asio::mutable_buffer buf, const T& val,
                                  TestTypes::MsgType mType) {
    auto bufRest = buf + 10u;
    bufRest = write(bufRest, val);

    TestTypes::FixedHeader head;
    head.length = asio::buffer_size(bufRest) - asio::buffer_size(buf);
    head.msgType = mType;
    write(asio::buffer(buf), head);
    return bufRest;
}

int main(int argc, char* argv[]) {
    std::array<char, 1024> mainBuf;
    {
        std::ofstream serializedData;
        serializedData.open("data.bin", std::ios::out | std::ios::binary);
        
        auto req = TestTypes::DataRequest{"GOOGL", 1};

        auto tp = system_clock::now();
        sys_time<microseconds> currTimePoint =
            time_point_cast<microseconds>(tp);
        TestTypes::DataSeries data = {
            {2.5, -56.789, 5.56},
            {currTimePoint, currTimePoint + seconds{5},
             currTimePoint + seconds{7}}};


        for (auto i = 0u; i < data.dataPoints.size(); ++i) {
            std::cout << "Data value: " << data.dataPoints.at(i) << ", "
                << "Time point: " << data.timePoints.at(i) << '\n';
        }
        auto buf = writeMessage(asio::buffer(mainBuf), req,
                                TestTypes::MsgType::DataRequest);
        buf = writeMessage(buf, data, TestTypes::MsgType::DataSeries);

        
        auto sz = mainBuf.size()-asio::buffer_size(buf);
        std::cout<<"Write buffer size: " << sz <<'\n';
        serializedData.write(mainBuf.data(), sz);
        serializedData.close();
    }
    return 0;
}
