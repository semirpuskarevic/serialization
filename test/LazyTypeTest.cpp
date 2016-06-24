#include <gmock/gmock.h>
#include <lazy.hpp>
#include <writer.hpp>
#include <boost/asio/buffer.hpp>
#include <vector>

using namespace ::testing;
using namespace detail;

class LazyTypeTest : public Test {
protected:
    LazyTypeTest() : buf_{asio::buffer(main_buf_)}, writer_{buf_} {}

private:
    std::array<char, 1024> main_buf_;

protected:
    asio::mutable_buffer buf_;
    writer writer_;
};

TEST_F(LazyTypeTest, GetsDefaultConstructedValue) {
    auto lt = lazy<uint32_t>();
    ASSERT_THAT(lt.get(), Eq(0));
}

TEST_F(LazyTypeTest, GetsValueDefinedOnConstruction) {
    auto lt = lazy<std::string>(std::string("ABC"));
    ASSERT_THAT(lt.get(), Eq("ABC"));
}

TEST_F(LazyTypeTest, GetsValueToChangingIt) {
    auto lt = lazy<std::vector<int32_t>>(std::vector<int32_t>{1, 2, 3});
    lt.get().push_back(4);
    ASSERT_THAT(lt.get(), Eq(std::vector<int32_t>{1, 2, 3, 4}));
}

TEST_F(LazyTypeTest, LazyTypeConstructedFromBufferInitiallyHasNoValue) {
    auto collection = std::vector<std::string>{"1","2","3"};
    writer_(collection);

    auto const& lt = lazy<decltype(collection)>(buf_);
    ASSERT_FALSE(lt.has_value());
}


TEST_F(LazyTypeTest, LazyTypeConstructedFromBufferHasValueAfterItIsRequested) {
    auto collection = std::vector<std::string>{"1","2","3"};
    writer_(collection);

    auto const& lt = lazy<decltype(collection)>(buf_);
    lt.get();

    ASSERT_TRUE(lt.has_value());
}

TEST_F(LazyTypeTest, GetsValueFromBuffer) {
    auto collection = std::vector<std::string>{"1","2","3"};
    writer_(collection);

    auto const& lt = lazy<decltype(collection)>(buf_);
    ASSERT_THAT(lt.get(), Eq(collection));
}

TEST_F(LazyTypeTest, GetsValueFromBufferAndChangeIt) {
    auto collection = std::vector<std::string>{"1","2","3"};
    writer_(collection);

    auto lt = lazy<decltype(collection)>(buf_);
    lt.get().push_back("4");
    ASSERT_THAT(lt.get(), Eq(std::vector<std::string>{"1", "2", "3", "4"}));
}

TEST_F(LazyTypeTest, ConfirmsThatBufferSizeIsZeroWhenNotConstructedFromBuffer){
    auto lt = lazy<uint32_t>();
    ASSERT_THAT(lt.buffer_size(), Eq(0));

    lt = lazy<uint32_t>(5u);
    ASSERT_THAT(lt.buffer_size(), Eq(0));
}

TEST_F(LazyTypeTest, ConfirmsThatBufferSizeIsNotZeroWhenConstructedFromBuffer){
    auto collection = std::vector<std::string>{"1","2","3"};
    writer_(collection);

    auto lt = lazy<decltype(collection)>(buf_);
    ASSERT_THAT(lt.buffer_size(), Ne(0));
}
