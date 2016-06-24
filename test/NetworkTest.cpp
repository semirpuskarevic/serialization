#include <gmock/gmock.h>
#include <network.hpp>
#include <boost/endian/conversion.hpp>

constexpr bool is_big_endian() {
#ifdef BOOST_BIG_ENDIAN
    return true;
#else
    return false;
#endif
}

using namespace ::testing;

TEST(NetworkTest, PrintsNativeEndiannessMessage) {
    if(is_big_endian())
    {
        std::cout<<"Native endianness: Big"<<std::endl;
    }
    else
    {
        std::cout<<"Native endianness: Little"<<std::endl;
    }
}

TEST(NetworkTest, ConvertsHostToNetworkLong) {
    uint32_t hnum = 0x11223344;
    if(is_big_endian())
    {
        ASSERT_THAT(network::hton(hnum), Eq(hnum));
    }
    else
    {
        ASSERT_THAT(network::hton(hnum), Eq(0x44332211));
    }
}

TEST(NetworkTest, ConvertsNetworkToHostLong) {
    uint32_t nnum = 0x11223344;
    if(is_big_endian())
    {
        ASSERT_THAT(network::ntoh(nnum), Eq(nnum));
    }
    else
    {
        ASSERT_THAT(network::ntoh(nnum), Eq(0x44332211));
    }
}

TEST(NetworkTest, ConvertsHostToNetworkLongAndBack) {
    uint32_t hnum = 0x11223344;
    uint32_t nnum = network::hton(hnum);

    ASSERT_THAT(network::ntoh(nnum), Eq(hnum));
}


TEST(NetworkTest, ConvertsHostToNetworkLongAndBackSigned) {
    int32_t hnum = -10;
    int32_t nnum = network::hton(hnum);

    ASSERT_THAT(network::ntoh(nnum), Eq(hnum));
}

TEST(NetworkTest, ConvertsHostToNetworkLongLongAndBackSigned) {
    int64_t hnum = 0xff11223344556677;
    int64_t nnum = network::hton(hnum);

    ASSERT_THAT(network::ntoh(nnum), Eq(hnum));
}

TEST(NetworkTest, ConvertsHostToNetworkChar) {
    char hc = 10;

    ASSERT_THAT(network::hton(hc), Eq(hc));
}

TEST(NetworkTest, ConvertsHostToNetworkByte) {
    int8_t hbyte = 0x01;

    ASSERT_THAT(network::hton(hbyte), Eq(hbyte));
}

TEST(NetworkTest, TestsEncodingAndDecodingFloatAsUint32Type) {
    auto number = 3.1415926f;
    uint32_t encoded = network::pack754(number);
    float decoded = network::unpack754(encoded);
    ASSERT_THAT(decoded, Eq(number));
}

TEST(NetworkTest, TestsEncodingAndDecodingDoubleAsUint64Type) {
    auto number = 3.14159265358979323;
    uint64_t encoded = network::pack754(number);
    double decoded = network::unpack754(encoded);
    ASSERT_THAT(decoded, Eq(number));
}
