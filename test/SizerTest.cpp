#include <gmock/gmock.h>
#include <sizer.hpp>
#include <writer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <date.h>

using namespace ::testing;
using namespace detail;
using namespace date;
using namespace std::chrono;

namespace asio = boost::asio;

class SizerBase : public Test {
protected:
    SizerBase() : buf_{asio::buffer(main_buf_)}, writer_{buf_} {}

private:
    std::array<char, 1024u> main_buf_;

protected:
    asio::mutable_buffer buf_;
    writer writer_;
};

class IntegralSizer : public SizerBase {
protected:
    IntegralSizer() {
        writer_(uint32Value);
        writer_(uint16Value);
    }

    uint32_t uint32Value{5};
    uint16_t uint16Value{10};
};

TEST_F(IntegralSizer, GetsSizeOfSingleIntegralType) {
    ASSERT_THAT(get_size<uint32_t>(buf_), Eq(4u));
}

TEST_F(IntegralSizer, GetsTotalSizeOfMultipleIntegralTypes) {
    sizer s(buf_);
    s(uint32_t{});
    s(uint16_t{});
    ASSERT_THAT(s.size(), Eq(6u));
}

class FloatingPointTypeSizer : public SizerBase {
    protected:
        FloatingPointTypeSizer() {
            writer_(e);
            writer_(ePrecise);
        }

    float e = 2.718281;
    double ePrecise = 2.718281828459;
};

TEST_F(FloatingPointTypeSizer, GetsSizeOfFloatType) {
    ASSERT_THAT(get_size<float>(buf_), Eq(4u));
}

TEST_F(FloatingPointTypeSizer, GetsTotalSizeOfMultipleFloatingPointTypes) {
    sizer s{buf_};
    s(float{});
    s(double{});
    ASSERT_THAT(s.size(), Eq(12u));
}

class EnumSizer : public SizerBase {
protected:
    EnumSizer() { writer_(number); }

    enum class Int32BasedEnum : int32_t { A, B, C };
    enum FruitsEnum : char { apple = 'a', banana = 'b', lemon = 'l' };
    Int32BasedEnum number = Int32BasedEnum::B;
    FruitsEnum fruit = FruitsEnum::lemon;
};

TEST_F(EnumSizer, GetsSizeOfSingleIntegralBasedEnumType) {
    ASSERT_THAT(get_size<Int32BasedEnum>(buf_), Eq(4u));
}

TEST_F(EnumSizer, GetsTotalSizeOfMultipleEnumTypes) {
    sizer s(buf_);
    s(Int32BasedEnum{});

    s(FruitsEnum{});
    ASSERT_THAT(s.size(), Eq(5u));
}

class IntegralConstantSizer : public SizerBase {
protected:
    IntegralConstantSizer() { writer_(uint16Member); }

    using IntConstUInt16 = std::integral_constant<uint16_t, 0xf001>;
    IntConstUInt16 uint16Member;

    using IntConstUInt32 = std::integral_constant<uint32_t, 0xf0010203>;
    IntConstUInt32 uint32Member;
};

TEST_F(IntegralConstantSizer, GetsSizeOfIntegralType){
    ASSERT_THAT(get_size<IntConstUInt16>(buf_), Eq(2u));
}

TEST_F(IntegralConstantSizer, GetsTotalSizeOfMultipleIntConstantTypes){
    sizer s{buf_};

    s(IntConstUInt16{});
    s(IntConstUInt32{});
    ASSERT_THAT(s.size(), Eq(6u));
}


class StringSizer : public SizerBase {
protected:
    StringSizer() {
        writer_(firstMsg);
        writer_(secondMsg);
    }

    std::string firstMsg{"Some text."};
    std::string secondMsg{"1234567"};
};

TEST_F(StringSizer, GetsSizeOfStringType) {
    size_t len = firstMsg.length() + 2u;
    ASSERT_THAT(get_size<std::string>(buf_), Eq(len));
}

TEST_F(StringSizer, GetsTotalSizeOfMultipleStringTypes) {
    sizer s(buf_);

    s(std::string{});
    s(std::string{});

    size_t len = firstMsg.length() + secondMsg.length() + 4u;
    ASSERT_THAT(s.size(), Eq(len));
}

class IntegralAndStringSizer : public SizerBase {
protected:
    IntegralAndStringSizer() {
        writer_(number);
        writer_(firstMsg);

        writer_(fruit);
        writer_(secondMsg);
    }
    uint32_t number{5};
    std::string firstMsg{"Some text."};

    enum FruitsEnum : char { apple = 'a', banana = 'b', lemon = 'l' };
    FruitsEnum fruit = FruitsEnum::lemon;

    std::string secondMsg{"1234567"};
};

TEST_F(IntegralAndStringSizer, GetsTotalSizeOfIntegralAndStringTypes) {
    sizer s(buf_);

    s(uint32_t{});
    s(std::string{});

    size_t len = firstMsg.length() + 6u;
    ASSERT_THAT(s.size(), Eq(len));
}

TEST_F(IntegralAndStringSizer, GetsTotalSizeOfIntegralEnumAndStringTypes) {
    sizer s(buf_);

    s(uint32_t{});
    s(std::string{});
    s(FruitsEnum{});
    s(std::string{});

    size_t len = firstMsg.length() + secondMsg.length() + 9u;
    ASSERT_THAT(s.size(), Eq(len));
}

class VectorSizer : public SizerBase {
protected:
    VectorSizer() {
        writer_(numbers);
        writer_(words);
    }
    std::vector<int32_t> numbers{1, 2, 3, 4};
    std::vector<std::string> words{"A", "B", "AB"};
};

TEST_F(VectorSizer, GetsSizeOfVectorOfInegralTypes) {
    size_t len = numbers.size() * sizeof(uint32_t) + 2u;
    ASSERT_THAT(get_size<std::vector<uint32_t>>(buf_), Eq(len));
}

TEST_F(VectorSizer, GetsTotalSizeOfVectorsOfIntAndStringTypes) {
    sizer s(buf_);

    s(std::vector<uint32_t>{});
    s(std::vector<std::string>{});
    size_t len = numbers.size() * sizeof(uint32_t) + 14u;
    ASSERT_THAT(s.size(), Eq(len));
}

class UnorderedMapSizer : public SizerBase {
    protected:
        UnorderedMapSizer(){
            writer_(numsToWords);
            writer_(wordsToNums);
        }

        using num_to_str_t = std::unordered_map<uint32_t, std::string>;
        using str_to_num_t = std::unordered_map<std::string, uint16_t>;

        num_to_str_t numsToWords{{1, "A"}, {2, "B"}, {3, "AB"}};
        str_to_num_t wordsToNums{{"A", 1u}, {"AB", 2}, {"ABC", 3}};
};

TEST_F(UnorderedMapSizer, GetsSizerOfUnorderedMapType){
    constexpr size_t len = 4*sizeof(uint16_t) + 3*sizeof(uint32_t) + 4u;
    ASSERT_THAT(get_size<num_to_str_t>(buf_), Eq(len));
}

TEST_F(UnorderedMapSizer, GetsTotalSizeOfMultipleUnorderedMapTypes){
    sizer s{buf_};

    s(num_to_str_t{});
    s(str_to_num_t{});
    constexpr size_t len = 11*sizeof(uint16_t) + 3*sizeof(uint32_t) + 10u;
    ASSERT_THAT(s.size(), Eq(len));
}



class DateTimeSizer: public SizerBase {
protected:
    DateTimeSizer(){
        writer_(microsecDateTime);
        writer_(microsecDateTime);
    }

    sys_time<microseconds> microsecDateTime{sys_days(2016_y / 05 / 01) +
                                            hours{5} + minutes{15} +
                                            seconds{0} + microseconds{123456}};
};


TEST_F(DateTimeSizer, GetsSizeOfTimePointWithMicrosecondPrecisionType){
    ASSERT_THAT(get_size<sys_time<microseconds>>(buf_), Eq(8u));
}

TEST_F(DateTimeSizer, GetsTotalSizeOfMultipleTimePointWithMicrosecondPrecisionTypes){
    sizer s{buf_};

    s(sys_time<microseconds>{});
    s(sys_time<microseconds>{});
    ASSERT_THAT(s.size(), Eq(16u));
}

namespace  SizerTestTypes
{
    enum class msg_type_t : uint16_t { A, B, C};
} /* SizerTestTypes */ 

BOOST_FUSION_DEFINE_STRUCT(
        (SizerTestTypes), header_t,
        (uint32_t, seq_num)
        (SizerTestTypes::msg_type_t, msg_type)
        (std::string, text_flag)
        )

class SimpleFusionSequenceSizer : public SizerBase {
    protected:
        SimpleFusionSequenceSizer() 
        {
            writer_(header);
        }
        SizerTestTypes::header_t header = {1, SizerTestTypes::msg_type_t::B, "ABC"};
};

TEST_F(SimpleFusionSequenceSizer, GetsSizeOfSimpleFusionSequenceType){
    ASSERT_THAT(get_size<SizerTestTypes::header_t>(buf_), Eq(11u));
}

namespace SizerTestTypes
{
struct collection_t{
    std::vector<std::string> value;
};
    
} /* SizerTestTypes */ 

BOOST_FUSION_ADAPT_STRUCT(
        SizerTestTypes::collection_t,
        (std::vector<std::string>, value)
        )

BOOST_FUSION_DEFINE_STRUCT(
        (SizerTestTypes), some_message_t,
        (std::string, id)
        (SizerTestTypes::collection_t, properties)
        )

class NestedFusionSequenceSizer : public SizerBase {
    protected:
        NestedFusionSequenceSizer()
        {
            writer_(msg);
        }

        SizerTestTypes::some_message_t msg = {{"12"}, {{"AB", "C"}}};
};

TEST_F(NestedFusionSequenceSizer, GetsSizeOfNestedFusionSequenceType){
    ASSERT_THAT(get_size<SizerTestTypes::some_message_t>(buf_), Eq(13u));
}

class OptionalFieldSizer : public SizerBase {
    protected:
        OptionalFieldSizer()
        {
            writer_(opt_fields_mask);
            writer_(opt_int_field);
            writer_(opt_msg_type);
            writer_(description);

            writer_(second_opt_fields_mask);
            writer_(second_opt_int_field);
            writer_(second_opt_msg_type);
        }

        using opt_fields = optional_field_set<uint16_t>;
        using opt_int32_field = optional_field<int32_t, 0>;
        using opt_msg_type_t = optional_field<SizerTestTypes::msg_type_t, 1>;
        using opt_string_t = optional_field<std::string, 2>;

        opt_fields opt_fields_mask;
        opt_int32_field opt_int_field = 5;
        opt_msg_type_t opt_msg_type;
        opt_string_t description = {"AB"};

        opt_fields second_opt_fields_mask;
        opt_int32_field second_opt_int_field = 5;
        opt_msg_type_t second_opt_msg_type = {SizerTestTypes::msg_type_t::B};
};

TEST_F(OptionalFieldSizer, GetsSizeOfOptionalFieldSetType){
    ASSERT_THAT(get_size<opt_fields>(buf_), Eq(2u));
}

TEST_F(OptionalFieldSizer, GetsTotalSizeOfOptionalFieldSetAndIntTypes){
    sizer s{buf_};

    s(opt_fields{});
    s(opt_int32_field{});

    ASSERT_THAT(s.size(), Eq(6u));
}

TEST_F(OptionalFieldSizer, GetsTotalSizeOfOptionalFieldSetAndIntAndEnumTypes){
    sizer s{buf_};

    s(opt_fields{});
    s(opt_int32_field{});
    s(opt_msg_type_t{});

    ASSERT_THAT(s.size(), Eq(6u));
}

TEST_F(OptionalFieldSizer, GetsSizeOfOptionalFieldWhenFieldSetIsEmpty){
    ASSERT_THAT(get_size<opt_int32_field>(buf_), Eq(0u));
}

TEST_F(OptionalFieldSizer, GetsTotalSizeOfMultipleOptionalFieldsWithSetAndNotSet){
    sizer s{buf_};

    s(opt_fields{});
    s(opt_int32_field{});
    s(opt_msg_type_t{});
    s(opt_string_t{});

    ASSERT_THAT(s.size(), Eq(10u));
}

TEST_F(OptionalFieldSizer, GestTotatSizeOfOptionalFieldsInPresenceOfMultipleFieldSets){
    sizer s{buf_};

    s(opt_fields{});
    s(opt_int32_field{});
    s(opt_msg_type_t{});
    s(opt_string_t{});

    s(opt_fields{});
    s(opt_int32_field{});
    s(opt_msg_type_t{});

    ASSERT_THAT(s.size(), Eq(18u));
}
