#ifndef SERIALIZATION_TEST_TYPES_H_
#define SERIALIZATION_TEST_TYPES_H_

#include <boost/fusion/include/define_struct.hpp>
#include <string>
#include <vector>
#include <optional.hpp>

namespace SerializationTestTypes {

using version_t = std::integral_constant<uint16_t, 0xf001>;

enum class msg_type_t : uint16_t { A, B, C };

struct collection_t {
    std::vector<std::string> value;
};

using opt_fields = detail::optional_field_set<uint16_t>;
using opt_int32_field = detail::optional_field<int32_t, 0>;
using opt_msg_type_t = detail::optional_field<msg_type_t, 1>;
using opt_string_t = detail::optional_field<std::string, 2>;
} /* SerializationTestTypes */

BOOST_FUSION_DEFINE_STRUCT(
    (SerializationTestTypes), header_t,
    (SerializationTestTypes::version_t,
     version)(uint32_t, seq_num)(SerializationTestTypes::msg_type_t, msg_type))

BOOST_FUSION_ADAPT_STRUCT(SerializationTestTypes::collection_t,
                          (std::vector<std::string>, value))

BOOST_FUSION_DEFINE_STRUCT((SerializationTestTypes), some_message_t,
                           (std::string,
                            id)(SerializationTestTypes::collection_t,
                                properties))
BOOST_FUSION_DEFINE_STRUCT(
    (SerializationTestTypes), msg_with_opt_fields_t,
    (SerializationTestTypes::collection_t, properties)
    (SerializationTestTypes::opt_fields, opt_mask)
    (SerializationTestTypes::opt_int32_field, number)
    (SerializationTestTypes::opt_string_t, description))

#endif /* ifndef SERIALIZATION_TEST_TYPES_H_ */
