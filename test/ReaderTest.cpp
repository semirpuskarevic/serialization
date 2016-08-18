#include <date.h>
#include <gmock/gmock.h>
#include <boost/asio/buffer.hpp>
#include <reader.hpp>
#include <writer.hpp>
#include "SerializationTestTypes.hpp"

using namespace testing;
using namespace detail;
using namespace date;
using namespace std::chrono;

namespace asio = boost::asio;

template <size_t N>
class ReaderBase : public Test {
protected:
    constexpr static auto arr_size = N;

    ReaderBase()
        : buf_{asio::buffer(main_buf_)}, writer_{buf_}, reader_{buf_} {}

private:
    std::array<char, arr_size> main_buf_;

protected:
    asio::mutable_buffer buf_;
    writer writer_;
    reader reader_;
};

class ReaderTest : public ReaderBase<10u> {
protected:
    ReaderTest() {
        writer_(fourByteNum);
        writer_(twoByteNum);
    }

    int32_t fourByteNum = 5;
    uint16_t twoByteNum = 15;
};

TEST_F(ReaderTest, ReadsIntegralValueAfterWrite) {
    int32_t readFourByteNum;
    reader_(readFourByteNum);
    ASSERT_THAT(readFourByteNum, Eq(fourByteNum));
}

TEST_F(ReaderTest, ReadsTwoConsecutiveIntegralValuesAfterWrite) {
    int32_t readFourByteNum;
    reader_(readFourByteNum);
    uint16_t readTwoByteNum;
    reader_(readTwoByteNum);
    ASSERT_THAT(readTwoByteNum, Eq(twoByteNum));
}

template <typename T>
class IntegralTypeReader : public ReaderBase<1024u> {
protected:
    IntegralTypeReader() { writer_(num); }

    T num{5};
};

using MyIntegralTypes = ::testing::Types<char, int8_t, int16_t, int64_t,
                                         uint8_t, uint32_t, uint64_t>;

TYPED_TEST_CASE(IntegralTypeReader, MyIntegralTypes);

TYPED_TEST(IntegralTypeReader, ReadssVariousIntegralTypes) {
    TypeParam readNum;
    this->reader_(readNum);
    ASSERT_THAT(readNum, Eq(this->num));
}

class BooleanTypeReader : public ReaderBase<4u> {
protected:
    BooleanTypeReader() {
        writer_(true);
        writer_(false);
    }
};

TEST_F(BooleanTypeReader, ReadsBooleanValuesAfterWrite) {
    bool trueValueRead;
    bool falseValueRead;
    reader_(trueValueRead);
    reader_(falseValueRead);
    ASSERT_TRUE(trueValueRead);
    ASSERT_FALSE(falseValueRead);
}

class FloatingPointTypeReader : public ReaderBase<14u> {
protected:
    FloatingPointTypeReader() {
        writer_(e);
        writer_(ePrecise);
    }
    float e = 2.718281;
    double ePrecise = 2.718281828459;
};

TEST_F(FloatingPointTypeReader, ReadsFloatValueAfterWrite) {
    float eRead;
    reader_(eRead);
    ASSERT_THAT(eRead, Eq(e));
}

TEST_F(FloatingPointTypeReader, ReadsDoubleValueAfterWrite) {
    float eRead;
    reader_(eRead);

    double ePreciseRead;
    reader_(ePreciseRead);
    ASSERT_THAT(ePreciseRead, ePrecise);
}

class EnumTypeReader : public ReaderBase<8u> {
protected:
    enum class CharEnum : char { A = 'A', B = 'B', C = 'C' };
    enum class UInt32Enum : uint32_t { X, Y };

    EnumTypeReader() {
        writer_(CharEnum::B);
        writer_(UInt32Enum::X);
    }
};

TEST_F(EnumTypeReader, ReadsCharEnumValueAfterWrite) {
    CharEnum readValue;
    reader_(readValue);
    ASSERT_THAT(readValue, Eq(CharEnum::B));
}

TEST_F(EnumTypeReader, ReadsMultipleEnumValuesAfterWrite) {
    CharEnum readFirst;
    reader_(readFirst);
    UInt32Enum readSecond;
    reader_(readSecond);
    ASSERT_THAT(readSecond, Eq(UInt32Enum::X));
}

class IntegralConstantReader : public ReaderBase<32u> {
protected:
    using IntConstUInt16 = std::integral_constant<uint16_t, 0xf001>;
    using IntConstUInt32 = std::integral_constant<uint32_t, 0xf0010203>;

    IntegralConstantReader() {
        writer_(uint16Member);
        writer_(regularNum);
        writer_(uint32Member);
        writer_(regularNum);
    }

    IntConstUInt16 uint16Member;
    int32_t regularNum = 5;
    IntConstUInt32 uint32Member;
};

TEST_F(IntegralConstantReader, ReadsUInt16IntegralConstantValueAfterWrite) {
    IntConstUInt16 readUint16Member;
    reader_(readUint16Member);
    int32_t readRegularNum;
    reader_(readRegularNum);
    ASSERT_THAT(readRegularNum, Eq(5));
}

TEST_F(IntegralConstantReader, ReadsMultipleIntegralConstantValuesAfterWrite) {
    IntConstUInt16 readUint16Member;
    reader_(readUint16Member);
    int32_t readRegularNum;
    reader_(readRegularNum);
    IntConstUInt32 readUint32Member;
    reader_(readUint32Member);
    int32_t readSedondRegularNum;
    reader_(readSedondRegularNum);

    ASSERT_THAT(readSedondRegularNum, Eq(5));
}

TEST_F(IntegralConstantReader,
       ConfirmsThatExceptionIsGeneratedWhenConstValueDoesNotMatchReadValue) {
    std::integral_constant<uint16_t, 0xf002> differentConstValue;
    ASSERT_THROW(reader_(differentConstValue), std::domain_error);
}

class StringReader : public ReaderBase<20u> {
protected:
    StringReader() {
        writer_("ABC");
        writer_("12345");
    }
};

TEST_F(StringReader, ReadsStringValueAfterWrite) {
    std::string readWord;
    reader_(readWord);
    ASSERT_THAT(readWord, Eq("ABC"));
}

TEST_F(StringReader, ReadsConsecutiveStringValuesAfterWrite) {
    std::string firstWord;
    reader_(firstWord);
    std::string secondWord;
    reader_(secondWord);
    ASSERT_THAT(secondWord, Eq("12345"));
}

class VectorReader : public ReaderBase<50u> {
protected:
    VectorReader() {
        writer_(numbers);
        writer_(words);
    }
    std::vector<int32_t> numbers{1, 5, 10, 15};
    std::vector<std::string> words{"A", "AB", "ABC"};
};

TEST_F(VectorReader, ReadsVectorOfIntegralValuesAfterWrite) {
    std::vector<int32_t> readNumbers;
    reader_(readNumbers);
    ASSERT_THAT(readNumbers, Eq(numbers));
}

TEST_F(VectorReader, ReadsConsecutiveVectorsOfDifferentTypeslAfterWrite) {
    std::vector<int32_t> readNumbers;
    reader_(readNumbers);
    std::vector<std::string> readWords;
    reader_(readWords);
    ASSERT_THAT(readWords, Eq(words));
}

class DateTimeReader : public ReaderBase<15u> {
protected:
    DateTimeReader() { writer_(microsecDateTime); }

    sys_time<microseconds> microsecDateTime{sys_days(2016_y / 05 / 01) +
                                            hours{5} + minutes{15} +
                                            seconds{0} + microseconds{123456}};
};

TEST_F(DateTimeReader, ReadsTimePointWithMicrosecondsAfterWrite) {
    sys_time<microseconds> microsecDateTimeRead;
    reader_(microsecDateTimeRead);
    ASSERT_THAT(microsecDateTimeRead, Eq(microsecDateTime));
}

class UnorderedMapReader : public ReaderBase<50u> {
protected:
    UnorderedMapReader() {
        writer_(elements);
        writer_(numElements);
    }
    std::unordered_map<int32_t, std::string> elements{
        {1, "A"}, {2, "B"}, {3, "AB"}};
    std::unordered_map<int16_t, uint32_t> numElements{{1, 5}, {2, 10}, {3, 15}};
};

TEST_F(UnorderedMapReader, ReadsUnorderedMapValueAfterWrite) {
    std::unordered_map<int32_t, std::string> readElements;
    reader_(readElements);
    ASSERT_THAT(readElements, Eq(elements));
}

TEST_F(UnorderedMapReader, ReadsMultipleUnorderedMapValuesAfterWrite) {
    std::unordered_map<int32_t, std::string> readElements;
    reader_(readElements);

    std::unordered_map<int16_t, uint32_t> readNumElements;
    reader_(readNumElements);
    ASSERT_THAT(readNumElements, Eq(numElements));
}

class SimpleFusionSequenceReader : public ReaderBase<20u> {
protected:
    SimpleFusionSequenceReader() { writer_(header); }
    SerializationTestTypes::header_t header = {
        {}, 1, SerializationTestTypes::msg_type_t::B};
};

TEST_F(SimpleFusionSequenceReader, ReadsSequenceValuesAfterWrite) {
    SerializationTestTypes::header_t readHeader;
    reader_(readHeader);
    ASSERT_THAT(readHeader.version, Eq(header.version));
    ASSERT_THAT(readHeader.msg_type, Eq(header.msg_type));
    ASSERT_THAT(readHeader.seq_num, Eq(header.seq_num));
}

class NestedFusionSequenceReader : public ReaderBase<30u> {
protected:
    NestedFusionSequenceReader() { writer_(msg); }

    SerializationTestTypes::some_message_t msg = {{"12"}, {{"AB", "C"}}};
};

TEST_F(NestedFusionSequenceReader, ReadsNestedFusionSequenceValueAfterWrite) {
    SerializationTestTypes::some_message_t readMsg;
    reader_(readMsg);
    ASSERT_THAT(readMsg.id, Eq(msg.id));
    ASSERT_THAT(readMsg.properties.value, Eq(msg.properties.value));
}

class OptionalFieldReader : public ReaderBase<10u> {
protected:
    OptionalFieldReader() {
        writer_(optFieldsMask);
        writer_(optIntField);
        writer_(optMsg);
        writer_(description);
    }

    SerializationTestTypes::opt_fields optFieldsMask;
    SerializationTestTypes::opt_int32_field optIntField = 5;
    SerializationTestTypes::opt_msg_type_t optMsg;
    SerializationTestTypes::opt_string_t description = {"AB"};
};

TEST_F(OptionalFieldReader, ReadsOptionalFieldSetAndOptionalValueAfterWrite) {
    SerializationTestTypes::opt_fields readOptFieldsMask;
    reader_(readOptFieldsMask);
    SerializationTestTypes::opt_int32_field readOptIntField;
    reader_(readOptIntField);
    ASSERT_THAT(*readOptIntField, Eq(*optIntField));
}

TEST_F(OptionalFieldReader, ReadsNonSetOptionalFieldValueAfterWrite) {
    SerializationTestTypes::opt_fields readOptFieldsMask;
    reader_(readOptFieldsMask);
    SerializationTestTypes::opt_int32_field readOptIntField;
    reader_(readOptIntField);
    SerializationTestTypes::opt_msg_type_t readOptMsg;
    reader_(readOptMsg);
    ASSERT_FALSE(readOptMsg);
}

TEST_F(OptionalFieldReader, ReadsOptionalFieldValueAfterNonSetValueAfterWrite) {
    SerializationTestTypes::opt_fields readOptFieldsMask;
    reader_(readOptFieldsMask);
    SerializationTestTypes::opt_int32_field readOptIntField;
    reader_(readOptIntField);
    SerializationTestTypes::opt_msg_type_t readOptMsg;
    reader_(readOptMsg);
    SerializationTestTypes::opt_string_t readDescription;
    reader_(readDescription);
    ASSERT_THAT(*readDescription, Eq(description));
}

TEST_F(OptionalFieldReader,
       ConfirmsThatExceptionIsGeneratedWhenFieldIsReadBeforeFieldSet) {
    SerializationTestTypes::opt_int32_field readOptIntField;
    ASSERT_THROW(reader_(readOptIntField), std::domain_error);
}

TEST_F(ReaderTest, ReadsIntegralValueWithReadFunction) {
    auto readFourByteNum = read<int32_t>(buf_);
    auto readTwoByteNum = read<uint16_t>(readFourByteNum.second);

    ASSERT_THAT(readFourByteNum.first, Eq(fourByteNum));
    ASSERT_THAT(readTwoByteNum.first, Eq(twoByteNum));
}

TEST_F(EnumTypeReader, ReadsMultipleEnumValuesWithReadFunction) {
    auto readFirst = read<CharEnum>(buf_);
    auto readSecond = read<UInt32Enum>(readFirst.second);

    ASSERT_THAT(readFirst.first, Eq(CharEnum::B));
    ASSERT_THAT(readSecond.first, Eq(UInt32Enum::X));
}

TEST_F(IntegralConstantReader,
       ReadsMultipleIntegralConstantValuesWithReadFunction) {
    auto readUint16Member = read<IntConstUInt16>(buf_);
    auto readRegularNum = read<int32_t>(readUint16Member.second);
    auto readUint32Member = read<IntConstUInt32>(readRegularNum.second);
    auto readSedondRegularNum = read<int32_t>(readUint32Member.second);

    ASSERT_THAT(readSedondRegularNum.first, Eq(5));
}

TEST_F(StringReader, ReadsConsecutiveStringValuesWithReadFunction) {
    auto firstWord = read<std::string>(buf_);
    auto secondWord = read<std::string>(firstWord.second);

    ASSERT_THAT(secondWord.first, Eq("12345"));
}

TEST_F(VectorReader, ReadsConsecutiveVectorsOfDifferentTypesWithReadFunction) {
    auto readNumbes = read<std::vector<int32_t>>(buf_);
    auto readWords = read<std::vector<std::string>>(readNumbes.second);

    ASSERT_THAT(readWords.first, Eq(words));
}

TEST_F(UnorderedMapReader, ReadsMultipleUnorderedMapValuesWithReadFunction) {
    auto readElements = read<std::unordered_map<int32_t, std::string>>(buf_);
    auto readNumElements =
        read<std::unordered_map<int16_t, uint32_t>>(readElements.second);

    ASSERT_THAT(readNumElements.first, Eq(numElements));
}

TEST_F(NestedFusionSequenceReader,
       ReadsNestedFusionSequenceValueWithReadFunction) {
    auto readMsg = read<SerializationTestTypes::some_message_t>(buf_);

    ASSERT_THAT(readMsg.first.id, Eq(msg.id));
    ASSERT_THAT(readMsg.first.properties.value, Eq(msg.properties.value));
}

TEST_F(
    OptionalFieldReader,
    ConfirmsThatExceptionIsGeneratedWhenReadingOptionalFieldsWithReadFunction) {
    auto readOptFieldsMask = read<SerializationTestTypes::opt_fields>(buf_);
    ASSERT_THROW(
        read<SerializationTestTypes::opt_int32_field>(readOptFieldsMask.second),
        std::domain_error);
}

class FusionSequenceWithOptionalFieldReader : public ReaderBase<50u> {
protected:
    FusionSequenceWithOptionalFieldReader() { writer_(msg); }

    SerializationTestTypes::msg_with_opt_fields_t msg = {
        {{"A", "B", "AB"}}, {}, {5}, {}};
};

TEST_F(FusionSequenceWithOptionalFieldReader,
       ReadOptionalFieldsAsPartOfFusionSequenceWithReadFunction) {
    auto readMsg = read<SerializationTestTypes::msg_with_opt_fields_t>(buf_);

    ASSERT_THAT(readMsg.first.properties.value, Eq(msg.properties.value));
    ASSERT_THAT(readMsg.first.number, Eq(5));
}
