#include <date.h>
#include <gmock/gmock.h>
#include <lazy.hpp>
#include <network.hpp>
#include <writer.hpp>
#include "SerializationTestTypes.hpp"

using namespace ::testing;
using namespace detail;
using namespace date;
using namespace std::chrono;

namespace asio = boost::asio;

template <size_t N>
class WriterBase : public Test {
protected:
    constexpr static auto arr_size = N;

    WriterBase() : buf_{asio::buffer(main_buf_)}, writer_{buf_} {}

private:
    std::array<char, arr_size> main_buf_;

protected:
    asio::mutable_buffer buf_;
    writer writer_;

    template <typename T>
    T read_value() {
        T result = *asio::buffer_cast<T *>(buf_);
        buf_ = buf_ + sizeof(T);
        return result;
    }

    std::string read_string(size_t len) {
        auto result = std::string(asio::buffer_cast<const char *>(buf_), len);
        buf_ = buf_ + len;
        return result;
    }
};

class WriterTest : public WriterBase<8u> {};

TEST_F(WriterTest, TestsInitialBufferSize) {
    ASSERT_THAT(asio::buffer_size(buf_), Eq(arr_size));
}

TEST_F(WriterTest, ChangesBufferOffset) {
    auto offset = buf_ + 1u;
    ASSERT_THAT(asio::buffer_size(offset), Eq(arr_size - 1));
    ASSERT_THAT(asio::buffer_size(buf_), Eq(arr_size));
}

TEST_F(WriterTest, WritesSingleIntegralValue) {
    int32_t num{5};
    writer_(num);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 4u));
    ASSERT_THAT(asio::buffer_size(buf_), Eq(arr_size));
}

TEST_F(WriterTest, ReadsSingleIntegralValueAfterWrite) {
    int32_t num{5};
    writer_(num);

    int32_t read_num = read_value<int32_t>();
    ASSERT_THAT(network::ntoh(read_num), Eq(num));
}

TEST_F(WriterTest, WritesTwoDifferentTypeIntegralValues) {
    int16_t small_num(20);
    int32_t num(5);
    writer_(small_num);
    writer_(num);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 6u));
}

TEST_F(WriterTest, ReadsTwoDifferentTypeIntegralValuesAfterWrite) {
    int16_t small_num(20);
    int32_t num(5);
    writer_(small_num);
    writer_(num);

    int16_t read_small_num = read_value<int16_t>();
    int32_t read_num = read_value<int32_t>();

    ASSERT_THAT(network::ntoh(read_small_num), Eq(small_num));
    ASSERT_THAT(network::ntoh(read_num), Eq(num));
}

TEST_F(WriterTest, WritesMoreDataThanBufferSize) {
    int32_t num(5);
    int64_t big_num(10);
    writer_(num);

    ASSERT_THROW(writer_(big_num), std::overflow_error);
}

template <typename T>
class IntegralTypeWriter : public WriterBase<1024u> {};

using MyIntegralTypes = ::testing::Types<char, int8_t, int16_t, int64_t,
                                         uint8_t, uint32_t, uint64_t>;

TYPED_TEST_CASE(IntegralTypeWriter, MyIntegralTypes);

TYPED_TEST(IntegralTypeWriter, WritesVariousIntegralTypes) {
    TypeParam num{5};
    auto &w = this->writer_;
    w(num);
    ASSERT_THAT(asio::buffer_size(w.buf_),
                Eq(TestFixture::arr_size - sizeof(num)));

    TypeParam read_num = this->template read_value<TypeParam>();
    ASSERT_THAT(network::ntoh(read_num), Eq(num));
}

class BooleanTypeWriter : public WriterBase<4u> {
};

TEST_F(BooleanTypeWriter, WritesBooleanValueAsOneByteInteger) {
    writer_(true);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 1u));
}

TEST_F(BooleanTypeWriter, ReadsBooleanValueAfterWrite) {
    writer_(true);
    auto trueValueRead = read_value<bool>();
    ASSERT_TRUE(trueValueRead);
}

TEST_F(BooleanTypeWriter, WritesTwoBooleanValuesAndReadThemBack) {
    writer_(true);
    writer_(false);
    read_value<bool>();
    auto falseValueRead = read_value<bool>();
    ASSERT_FALSE(falseValueRead);
}



class FloatingPointTypeWriter : public WriterBase<10u> {
protected:
    float e = 2.718281;
    double ePrecise = 2.718281828459;
};

TEST_F(FloatingPointTypeWriter, WritesFloatValueAs4ByteInteger) {
    writer_(e);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 4u));
}

TEST_F(FloatingPointTypeWriter, ReadsFloatValueAfterWrite) {
    writer_(e);
    auto eRead = network::ntoh(read_value<uint32_t>());
    ASSERT_THAT(network::unpack754(eRead), Eq(e));
}

TEST_F(FloatingPointTypeWriter, WritesDoubleValueAs8ByteInteger) {
    writer_(ePrecise);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size -8u));
}

TEST_F(FloatingPointTypeWriter, ReadsDoubleValueAfterWrite) {
    writer_(ePrecise);
    auto ePreciseRead = network::ntoh(read_value<uint64_t>());
    ASSERT_THAT(network::unpack754(ePreciseRead), ePrecise);
}

TEST_F(FloatingPointTypeWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    writer_(e);
    ASSERT_THROW(writer_(ePrecise), std::overflow_error);
}

class EnumTypeWriter : public WriterBase<8u> {
protected:
    enum class CharEnum : char { A = 'A', B = 'B', C = 'C' };

    enum Int16Enum : int16_t { a, b, c = a + b };

    enum class UInt32Enum : uint32_t { X, Y };
};

TEST_F(EnumTypeWriter, WritesCharEnumValue) {
    writer_(CharEnum::B);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 1u));
}

TEST_F(EnumTypeWriter, ReadsCharEnumValueAfterWrite) {
    auto char_enum = CharEnum::B;
    writer_(CharEnum::B);

    char read_char_enum = read_value<char>();
    ASSERT_THAT(static_cast<CharEnum>(network::ntoh(read_char_enum)),
                Eq(char_enum));
}

TEST_F(EnumTypeWriter, WritesInt16EnumValue) {
    writer_(Int16Enum::c);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 2u));
}

TEST_F(EnumTypeWriter, ReadsInt16EnumValueAfterWrite) {
    auto val = Int16Enum::c;
    writer_(val);

    auto read_val = read_value<int16_t>();
    ASSERT_THAT(static_cast<decltype(val)>(network::ntoh(read_val)), Eq(val));
}

TEST_F(EnumTypeWriter, WritesTwoDifferentEnumTypeValues) {
    writer_(UInt32Enum::Y);
    writer_(Int16Enum::b);

    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 6u));
}

TEST_F(EnumTypeWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    writer_(UInt32Enum::Y);
    writer_(Int16Enum::b);

    ASSERT_THROW(writer_(UInt32Enum::X), std::overflow_error);
}

TEST_F(EnumTypeWriter, ConfirmsThatNoDataIsWrittenInCaseOfException) {
    writer_(UInt32Enum::Y);
    writer_(Int16Enum::b);

    size_t bufsize_before = asio::buffer_size(writer_.buf_);
    EXPECT_THROW(writer_(UInt32Enum::X), std::overflow_error);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(bufsize_before));
}

class IntegralConstantWriter : public WriterBase<6u> {
protected:
    using IntConstUInt16 = std::integral_constant<uint16_t, 0xf001>;
    IntConstUInt16 uint16Member;

    using IntConstUInt32 = std::integral_constant<uint32_t, 0xf0010203>;
    IntConstUInt32 uint32Member;
};

TEST_F(IntegralConstantWriter, WritesUInt16IntegralConstantValue) {
    writer_(uint16Member);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 2u));
}

TEST_F(IntegralConstantWriter, ReadsUInt16IntegralConstantValueAfterWrite) {
    writer_(uint16Member);
    auto read_val = read_value<IntConstUInt16::value_type>();
    ASSERT_THAT(network::ntoh(read_val), Eq(IntConstUInt16::value));
}

TEST_F(IntegralConstantWriter, WritesUInt32IntegralConstantValue) {
    writer_(uint32Member);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 4u));
}

TEST_F(IntegralConstantWriter, ReadsUInt32IntegralConstantValueAfterWrite) {
    writer_(uint32Member);
    auto read_val = read_value<IntConstUInt32::value_type>();
    ASSERT_THAT(network::ntoh(read_val), Eq(IntConstUInt32::value));
}

TEST_F(IntegralConstantWriter,
       ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    writer_(uint32Member);
    ASSERT_THROW(writer_(uint32Member), std::overflow_error);
}

class StringWriter : public WriterBase<10u> {
protected:
    std::string short_str = {"ABC"};
    std::string long_str = {"Too long for buffer"};
};

TEST_F(StringWriter, WritesShortStringValue) {
    writer_(short_str);

    // two-byte size encoding + string data
    ASSERT_THAT(asio::buffer_size(writer_.buf_),
                Eq(arr_size - 2u - short_str.length()));
}

TEST_F(StringWriter, ReadStringAfterWrite) {
    writer_(short_str);

    auto len = read_value<uint16_t>();
    len = network::ntoh(len);
    ASSERT_THAT(len, Eq(short_str.length()));

    std::string read_short_str = read_string(len);
    ASSERT_THAT(read_short_str, Eq(short_str));
}

TEST_F(StringWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    ASSERT_THROW(writer_(long_str), std::overflow_error);
}

TEST_F(StringWriter, ConfirmsThatNoDataIsWrittenInCaseOfException) {
    EXPECT_THROW(writer_(long_str), std::overflow_error);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size));
}

class VectorWriter : public WriterBase<20u> {
protected:
    std::vector<int32_t> numbers{1, 5, 10, 15};
    std::vector<std::string> words{"A", "AB", "ABC"};
    std::vector<std::string> too_long_words{"A", "AB", "ABC",
                                            "A very very long string"};
};

TEST_F(VectorWriter, WritesAVectorOfIntegralValues) {
    writer_(numbers);
    auto vec_size = numbers.size() * sizeof(int32_t);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - vec_size - 2u));
}

TEST_F(VectorWriter, ConfirmsThatSizeOfVectorIsProperlyEncoded) {
    writer_(numbers);

    auto read_len = network::ntoh(read_value<uint16_t>());
    ASSERT_THAT(read_len, Eq(numbers.size()));
}

TEST_F(VectorWriter, ReadsVectorOfIntegralValuesAfterWrite) {
    writer_(numbers);

    uint16_t len = numbers.size();
    buf_ = buf_ + sizeof(len);
    decltype(numbers) read_numbers(len);
    for (auto &el : read_numbers) {
        el = network::ntoh(read_value<int32_t>());
    }
    ASSERT_THAT(read_numbers, Eq(numbers));
}

TEST_F(VectorWriter, WritesAVectorOfStringValues) {
    writer_(words);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 14u));
}

TEST_F(VectorWriter, ReadsVectorOfStringValuesAfterWrite) {
    writer_(words);

    uint16_t len = words.size();
    buf_ = buf_ + sizeof(len);
    decltype(words) read_words(len);
    for (auto &el : read_words) {
        uint16_t str_len = network::ntoh(read_value<uint16_t>());
        el = read_string(str_len);
    }
    ASSERT_THAT(read_words, Eq(words));
}

TEST_F(VectorWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    ASSERT_THROW(writer_(too_long_words), std::overflow_error);
}

TEST_F(VectorWriter, ConfirmsThatWhenExceptionIsGeneratedSomeDataMayBeWritten) {
    size_t bufsize_before = asio::buffer_size(writer_.buf_);
    EXPECT_THROW(writer_(too_long_words), std::overflow_error);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Ne(bufsize_before));
}

class DateTimeWriter : public WriterBase<15u> {
protected:
    sys_time<microseconds> microsecDateTime{date::sys_days(2016_y / 05 / 01) +
                                            hours{5} + minutes{15} +
                                            seconds{0} + microseconds{123456}};

    int64_t microsecSinceEpoch = microsecDateTime.time_since_epoch().count();
};

TEST_F(DateTimeWriter, WritesTimePointWithMicrosecondPrecisionAs8Bytes) {
    writer_(microsecDateTime);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 8u));
}

TEST_F(DateTimeWriter, ChecksThatTimeIsProperlyConvertedToInteger) {
    ASSERT_THAT(microsecSinceEpoch, Eq(1462079700123456l));
}

TEST_F(DateTimeWriter, ReadsTimePointWithMicrosecondsAfterWrite) {
    auto microsecSinceEpochRead = read_value<int64_t>();
    microsecSinceEpochRead = network::ntoh(microsecSinceEpochRead);
    ASSERT_THAT(microsecSinceEpochRead, Eq(microsecSinceEpoch));
}

TEST_F(DateTimeWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    writer_(microsecDateTime);
    ASSERT_THROW(writer_(microsecDateTime), std::overflow_error);
}

class UnorderedMapWriter : public WriterBase<30u> {
protected:
    std::unordered_map<int32_t, std::string> elements{
        {1, "A"}, {2, "B"}, {3, "AB"}};
    std::unordered_map<int16_t, uint32_t> num_elements{
        {1, 5}, {2, 10}, {3, 15}};
    std::unordered_map<int32_t, uint32_t> too_long_container_for_buffer{
        {1, 5}, {2, 10}, {3, 15}, {4, 30}};
};

TEST_F(UnorderedMapWriter, WritesUnorderedMapValue) {
    writer_(elements);
    constexpr size_t total_size =
        3 * sizeof(int32_t) + 4 * sizeof(uint16_t) + 4u;
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - total_size));
}

TEST_F(UnorderedMapWriter, ReadsUnorderedMapValueAfterWrite) {
    writer_(elements);
    auto num_el = network::ntoh(read_value<uint16_t>());
    ASSERT_THAT(num_el, Eq(elements.size()));

    std::unordered_map<int32_t, std::string> read_elements;
    for (size_t i = 0; i < num_el; ++i) {
        auto k = network::ntoh(read_value<int32_t>());
        auto v_size = network::ntoh(read_value<uint16_t>());
        auto v = read_string(v_size);

        read_elements.emplace(std::make_pair(std::move(k), std::move(v)));
    }
    ASSERT_THAT(read_elements, Eq(elements));
}

TEST_F(UnorderedMapWriter, WritesUnorderedMapIntValue) {
    writer_(num_elements);
    auto num_el = network::ntoh(read_value<uint16_t>());
    ASSERT_THAT(num_el, Eq(num_elements.size()));

    std::unordered_map<int16_t, uint32_t> read_elements;
    for (size_t i = 0; i < num_el; ++i) {
        auto k = network::ntoh(read_value<int16_t>());
        auto v = network::ntoh(read_value<uint32_t>());

        read_elements.emplace(std::make_pair(std::move(k), std::move(v)));
    }
    ASSERT_THAT(read_elements, Eq(num_elements));
}

TEST_F(UnorderedMapWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    ASSERT_THROW(writer_(too_long_container_for_buffer), std::overflow_error);
}

class SimpleFusionSequenceWriter : public WriterBase<20u> {
protected:
    SerializationTestTypes::header_t header = {
        {}, 1, SerializationTestTypes::msg_type_t::B};
};

TEST_F(SimpleFusionSequenceWriter, WritesSequenceValues) {
    writer_(header);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 8u));
}

TEST_F(SimpleFusionSequenceWriter, ReadsSequenceValuesAfterWrite) {
    writer_(header);
    auto version = network::ntoh(read_value<uint16_t>());
    auto seq_num = network::ntoh(read_value<uint32_t>());
    auto msg_type = static_cast<SerializationTestTypes::msg_type_t>(
        network::ntoh(read_value<uint16_t>()));
    ASSERT_THAT(version, Eq(decltype(header.version)::value));
    ASSERT_THAT(seq_num, Eq(header.seq_num));
    ASSERT_THAT(msg_type, Eq(header.msg_type));
}

class NestedFusionSequenceWriter : public WriterBase<30u> {
protected:
    SerializationTestTypes::some_message_t msg = {{"12"}, {{"AB", "C"}}};
};

TEST_F(NestedFusionSequenceWriter, WritesSequenceValues) {
    writer_(msg);
    constexpr size_t seq_size = 4 * sizeof(uint16_t) + 5u;
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - seq_size));
}

TEST_F(NestedFusionSequenceWriter, ReadsValuesAfterWrite) {
    writer_(msg);
    auto id_len = network::ntoh(read_value<uint16_t>());
    auto id = read_string(id_len);
    ASSERT_THAT(id, Eq(msg.id));

    auto props_len = network::ntoh(read_value<uint16_t>());
    ASSERT_THAT(props_len, Eq(2u));

    auto first_prop_len = network::ntoh(read_value<uint16_t>());
    auto first_prop = read_string(first_prop_len);
    ASSERT_THAT(first_prop, Eq("AB"));
}

class OptionalFieldWriter : public WriterBase<10u> {
protected:
    SerializationTestTypes::opt_fields opt_fields_mask;
    SerializationTestTypes::opt_int32_field opt_int_field = 5;
    SerializationTestTypes::opt_msg_type_t opt_msg_type;
    SerializationTestTypes::opt_string_t description = {"AB"};
};

TEST_F(OptionalFieldWriter, WritesOptionalFieldSetValue) {
    writer_(opt_fields_mask);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 2u));
}

TEST_F(OptionalFieldWriter, ReadsOptionalFieldSetValueAfterWrite) {
    writer_(opt_fields_mask);
    auto read_opt_field_set_val = network::ntoh(read_value<uint16_t>());
    ASSERT_THAT(read_opt_field_set_val, Eq(0u));
}

TEST_F(OptionalFieldWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    std::string filler_string("1234567");
    writer_(filler_string);
    ASSERT_THROW(writer_(opt_fields_mask), std::overflow_error);
}

TEST_F(OptionalFieldWriter, WritesOptionalFieldValue) {
    writer_(opt_fields_mask);
    writer_(opt_int_field);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 6u));
}

TEST_F(OptionalFieldWriter, ReadsOptionalFieldAfterWrite) {
    writer_(opt_fields_mask);
    writer_(opt_int_field);

    auto read_opt_field_set_val = network::ntoh(read_value<uint16_t>());
    auto read_opt_int_field = network::ntoh(read_value<int32_t>());
    ASSERT_THAT(read_opt_int_field, Eq(5));
    ASSERT_THAT(read_opt_field_set_val, Eq(1u));
}

TEST_F(OptionalFieldWriter, ConfirmsThatNonSetOptioanlFieldIsNotWritten) {
    writer_(opt_fields_mask);
    writer_(opt_msg_type);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 2u));
}

TEST_F(OptionalFieldWriter, WritesMultipleFieldValues) {
    writer_(opt_fields_mask);
    writer_(opt_msg_type);
    writer_(description);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - 6u));
}

TEST_F(OptionalFieldWriter, ReadsMultipleFieldsAfterWrite) {
    writer_(opt_fields_mask);
    writer_(opt_msg_type);
    writer_(description);

    auto read_opt_field_set_val = network::ntoh(read_value<uint16_t>());
    auto desc_len = network::ntoh(read_value<uint16_t>());
    auto read_description = read_string(desc_len);

    ASSERT_THAT(read_opt_field_set_val, Eq(4u));
    ASSERT_THAT(read_description, Eq(description));
}

TEST_F(OptionalFieldWriter,
       ConfirmsThatExceptionIsGeneratedWhenFieldIsWrittenBeforeFieldSet) {
    ASSERT_THROW(writer_(opt_int_field), std::domain_error);
}

class LazyTypeWriter : public WriterBase<20u> {
protected:
    lazy<std::vector<int32_t>> numbers{{1, 5, 10, 15}};
    // std::vector<std::string> words{"A", "AB", "ABC"};
    std::vector<std::string> too_long_words{"A", "AB", "ABC",
                                            "A very very long string"};
};

TEST_F(LazyTypeWriter, WritesLazyTypeOfVectorOfIntegralTypes) {
    writer_(numbers);
    auto vec_size = numbers.get().size() * sizeof(int32_t);
    ASSERT_THAT(asio::buffer_size(writer_.buf_), Eq(arr_size - vec_size - 2u));
}

TEST_F(LazyTypeWriter, ConfirmsThatExceptionIsGeneratedInCaseOfOverflow) {
    ASSERT_THROW(writer_(too_long_words), std::overflow_error);
}

TEST_F(WriterTest, WritesIntegralValueWithWriteFunction) {
    int16_t small_num(20);
    int32_t num(5);
    auto buf = write(buf_, small_num);
    buf = write(buf, num);

    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 6u));
}

TEST_F(EnumTypeWriter, WritesTwoDifferentEnumTypeValuesWithWriteFunction) {
    auto buf = write(buf_, UInt32Enum::Y);
    buf = write(buf, Int16Enum::b);

    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 6u));
}

TEST_F(IntegralConstantWriter,
       WritesTwoDifferentIntegralConstantValuesWithWriteFunction) {
    auto buf = write(buf_, uint16Member);
    buf = write(buf, uint32Member);

    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 6u));
}

TEST_F(StringWriter, WritesShortStringValueWithWriteFunction) {
    auto buf = write(buf_, short_str);

    // two-byte size encoding + string data
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 2u - short_str.length()));
}

TEST_F(VectorWriter, WritesAVectorOfIntegralValuesWithWriteFunction) {
    auto buf = write(buf_, numbers);
    auto vec_size = numbers.size() * sizeof(int32_t);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - vec_size - 2u));
}

TEST_F(VectorWriter, WritesAVectorOfStringValuesWithWriteFunction) {
    auto buf = write(buf_, words);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 14u));
}

TEST_F(DateTimeWriter,
       WritesTimePointWithMicrosecondPrecisionAs8BytesWithWriteFunction) {
    auto buf = write(buf_, microsecDateTime);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 8u));
}

TEST_F(UnorderedMapWriter, WritesUnorderedMapValueWithWriteFunction) {
    auto buf = write(buf_, elements);
    constexpr size_t total_size =
        3 * sizeof(int32_t) + 4 * sizeof(uint16_t) + 4u;
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - total_size));
}

TEST_F(SimpleFusionSequenceWriter, WritesSequenceValuesWithWriteFunction) {
    auto buf = write(buf_, header);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 8u));
}

TEST_F(OptionalFieldWriter, WritesOptionalFieldSetValueWithWriteFunction) {
    auto buf = write(buf_, opt_fields_mask);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 2u));
}

TEST_F(LazyTypeWriter, WritesLazyTypeOfVectorOfIntegralTypesWithWriteFunction) {
    auto buf = write(buf_, numbers);
    auto vec_size = numbers.get().size() * sizeof(int32_t);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - vec_size - 2u));
}

TEST_F(BooleanTypeWriter, WritesBooleanValueWithWriteFunction) {
    auto buf = write(buf_, true);
    ASSERT_THAT(asio::buffer_size(buf), Eq(arr_size - 1u));
}
