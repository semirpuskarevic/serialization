#ifndef TYPES_H_
#define TYPES_H_ 

#include <boost/fusion/include/define_struct.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cmath>
#include <date.h>

namespace TestTypes {
using VersionType = std::integral_constant<uint16_t, 0xf001>;
using HeaderPopertiesType = std::unordered_map<std::string, std::string>;

enum class MsgType : uint32_t { DataRequest, DataSeries, BookUpdate, Unknown };

enum class Side : uint8_t { Buy, Sell, Unknown };

struct DecimalType {
    int8_t exponent;
    uint32_t mantissa;

    DecimalType(int8_t e = 0, uint32_t m = 0) : exponent{e}, mantissa{m} {}

    operator double() const { return mantissa * pow(10, exponent); }
};

struct DataRequest {
    std::string symbol;
    uint16_t depth = 1;

    DataRequest() = default;
    DataRequest(const std::string& sym, uint16_t d = 1)
        : symbol{sym}, depth{d} {}
};

struct FixedHeader {
    VersionType version;
    MsgType msgType{MsgType::Unknown};
    uint32_t length{0u};
};

// BookLevel
// BookRequest
// BookResponse
// BookUpdate
// TradeUpdate

} /* TestTypes */

BOOST_FUSION_ADAPT_STRUCT(TestTypes::FixedHeader,
                          (TestTypes::VersionType, version)(TestTypes::MsgType,
                                                            msgType)(uint32_t,
                                                                     length))

BOOST_FUSION_ADAPT_STRUCT(TestTypes::DecimalType,
                          (int8_t, exponent)(uint32_t, mantissa))

BOOST_FUSION_ADAPT_STRUCT(TestTypes::DataRequest,
                          (std::string, symbol)(uint16_t, depth))

BOOST_FUSION_DEFINE_STRUCT(
    (TestTypes), DataSeries,
    (std::vector<double>,
     dataPoints)(std::vector<date::sys_time<std::chrono::microseconds>>,
                 timePoints))
#endif /* ifndef TYPES_H_ */

