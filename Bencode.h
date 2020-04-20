//
// Created by Linux Oid on 20.04.2020.
//

#ifndef BENCODE_BENCODE_H
#define BENCODE_BENCODE_H

#include <variant>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <charconv>

#define PARSE_ERROR_MSG(str) (__PRETTY_FUNCTION__ + " "s + __FILE__  + " "s +  std::to_string(__LINE__) \
                        + ": parse error with \""s + std::string(str) + "\""s)
#define PARSE_ERROR_ASSERT(p, str) do{ if(!(p)) throw ParseError(PARSE_ERROR_MSG(str)); } while(0)

namespace bencode {
    struct BencodeElement;
    using BencodeInteger = long long;
    using BencodeString = std::string;
    using BencodeList = std::vector<BencodeElement>;
    using BencodeDictionary = std::unordered_map<BencodeString, BencodeElement>;

    using string_view_ptr = std::string_view::const_pointer;
    using BencodeStringSizeType = std::string::size_type;
    using namespace std::literals::string_literals;

    inline auto DecodeBencodeDictionary(std::string_view str) -> std::pair<BencodeDictionary, string_view_ptr>;

    inline auto DecodeBencodeList(std::string_view str) -> std::pair<BencodeList, string_view_ptr>;

    inline auto DecodeBencodeInteger(std::string_view str) -> std::pair<BencodeInteger, string_view_ptr>;

    inline auto DecodeBencodeString(std::string_view str) -> std::pair<BencodeString, string_view_ptr>;

    inline auto DecodeBencodeElement(std::string_view str) -> BencodeElement;

    inline auto Decode(BencodeElement const &) -> std::string;

    inline auto Prettier(BencodeElement const &) -> std::string;

    struct BencodeElement {
        std::variant<BencodeInteger, BencodeString, BencodeList, BencodeDictionary> value;
    };

    struct ParseError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    inline auto DecodeBencodeInteger(std::string_view str) -> std::pair<BencodeInteger, string_view_ptr> {
        PARSE_ERROR_ASSERT(!str.empty() && str.at(0) == 'i', str);
        str.remove_prefix(1);
        auto result = BencodeInteger{};
        auto[ptr, errc] = std::from_chars(str.cbegin(), str.cend(), result);
        PARSE_ERROR_ASSERT(errc != std::errc::invalid_argument, str);
        PARSE_ERROR_ASSERT(*ptr == 'e', str);
        return std::pair{result, ptr + 1};
    }

    inline auto DecodeBencodeString(std::string_view str) -> std::pair<BencodeString, string_view_ptr> {
        PARSE_ERROR_ASSERT(!str.empty() && std::isdigit(str.at(0)), str);
        auto length = BencodeStringSizeType{};
        auto[ptr, errc] = std::from_chars(str.cbegin(), str.cend(), length);
        PARSE_ERROR_ASSERT(*ptr == ':', str);
        ++ptr;
        PARSE_ERROR_ASSERT(errc != std::errc::invalid_argument, str);
        auto end = ptr + length;
        return std::pair{BencodeString(ptr, end), end};
    }

    inline auto DecodeBencodeList(std::string_view str) -> std::pair<BencodeList, string_view_ptr> {
        PARSE_ERROR_ASSERT(!str.empty() && str.at(0) == 'l', str);
        str.remove_prefix(1);
        using string_view_sz = std::string_view::size_type;
        BencodeList result;
        auto decode_element_pos = str.cbegin();
        while (*decode_element_pos != 'e') {
            auto argument_view = std::string_view{decode_element_pos,
                                                  static_cast<string_view_sz>(str.cend() - decode_element_pos)};
            switch (*decode_element_pos) {
                case 'i': {
                    auto[integer, end] = DecodeBencodeInteger(argument_view);
                    result.push_back({integer});
                    decode_element_pos = end;
                    break;
                }
                case 'l': {
                    auto[list, end] = DecodeBencodeList(argument_view);
                    result.push_back({list});
                    decode_element_pos = end;
                    break;
                }
                case 'd': {
                    auto[dict, end] = DecodeBencodeDictionary(argument_view);
                    result.push_back({dict});
                    decode_element_pos = end;
                    break;
                }
                default: {
                    auto[str, end] = DecodeBencodeString(argument_view);
                    result.push_back({str});
                    decode_element_pos = end;
                }
            }
        }

        return std::pair{result, decode_element_pos + 1};
    }

    inline auto DecodeBencodeDictionary(std::string_view str) -> std::pair<BencodeDictionary, string_view_ptr> {
        PARSE_ERROR_ASSERT(!str.empty() && str.at(0) == 'd', str);
        str.remove_prefix(1);
        using string_view_sz = std::string_view::size_type;
        BencodeDictionary result;
        auto decode_element_pos = str.cbegin();
        while (*decode_element_pos != 'e') {
            auto argument_view = std::string_view{decode_element_pos,
                                                  static_cast<string_view_sz>(str.cend() - decode_element_pos)};
            auto[key, end] = DecodeBencodeString(argument_view);
            argument_view.remove_prefix(end - decode_element_pos);
            decode_element_pos = end;
            switch (*decode_element_pos) {
                case 'i': {
                    auto[integer, end] = DecodeBencodeInteger(argument_view);
                    result.insert({key, {integer}});
                    decode_element_pos = end;
                    break;
                }
                case 'l': {
                    auto[list, end] = DecodeBencodeList(argument_view);
                    result.insert({key, {list}});
                    decode_element_pos = end;
                    break;
                }
                case 'd': {
                    auto[dict, end] = DecodeBencodeDictionary(argument_view);
                    result.insert({key, {dict}});
                    decode_element_pos = end;
                    break;
                }
                default: {
                    auto[str, end] = DecodeBencodeString(argument_view);
                    result.insert({key, {str}});
                    decode_element_pos = end;
                }
            }
        }

        return std::pair{result, decode_element_pos + 1};
    }

    inline auto DecodeBencodeElement(std::string_view str) -> BencodeElement {
        auto argument_view = str;
        switch (str.at(0)) {
            case 'i': {
                auto[integer, end] = DecodeBencodeInteger(argument_view);
                PARSE_ERROR_ASSERT(end == str.cend(), str);
                return {integer};
            }
            case 'l': {
                auto[list, end] = DecodeBencodeList(argument_view);
                PARSE_ERROR_ASSERT(end == str.cend(), str);
                return {list};
            }
            case 'd': {
                auto[dict, end] = DecodeBencodeDictionary(argument_view);
                PARSE_ERROR_ASSERT(end == str.cend(), str);
                return {dict};
            }
            default: {
                auto[s, end] = DecodeBencodeString(argument_view);
                PARSE_ERROR_ASSERT(end == str.cend(), str);
                return {s};
            }
        }
    }

    namespace details {
        struct Decoder {
            auto operator()(BencodeInteger const &integer) -> std::string {
                return "i"s + std::to_string(integer) + "e"s;
            }

            auto operator()(BencodeString const &str) -> std::string {
                return std::to_string(str.size()) + ":"s + str;
            }

            auto operator()(BencodeList const &list) -> std::string {
                std::ostringstream output{};

                output << 'l';
                for (auto &&el : list) {
                    output << std::visit(Decoder(), el.value);
                }
                output << 'e';

                return output.str();
            }

            auto operator()(BencodeDictionary const &dict) -> std::string {
                std::ostringstream output{};

                output << 'd';
                for (auto &&[key, el] : dict) {
                    output << key << std::visit(Decoder(), el.value);
                }
                output << 'e';

                return output.str();
            }
        };

        struct Prettier {
            auto operator()(BencodeInteger const &integer) -> std::string {
                return std::to_string(integer);
            }

            auto operator()(BencodeString const &str) -> std::string {
                return str;
            }

            auto operator()(BencodeList const &list) -> std::string {
                std::ostringstream output{};

                output << '[';
                for (auto &&el : list) {
                    output << std::visit(Prettier(), el.value) << " , ";
                }

                auto output_str = output.str();
                return output_str.erase(output_str.size() - 3) + "]";
            }

            auto operator()(BencodeDictionary const &dict) -> std::string {
                std::ostringstream output{};

                output << '{';
                for (auto &&[key, el] : dict) {
                    output << key << " : " << std::visit(Prettier(), el.value) << " , ";
                }

                auto output_str = output.str();

                return output_str.erase(output_str.size() - 3) + "}";
            }
        };
    }

    inline auto Decode(BencodeElement const &el) -> std::string {
        return std::visit(details::Decoder(), el.value);
    }

    inline auto Prettier(BencodeElement const &el) -> std::string {
        return std::visit(details::Prettier(), el.value);
    }
}

#endif //BENCODE_BENCODE_H
