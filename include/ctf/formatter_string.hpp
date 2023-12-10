//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_FORMATTER_STRING_HPP
#define CTF_FORMATTER_STRING_HPP

// This implements a formatter based on libc++'s implementation details.
#include <version>
#ifndef _LIBCPP_VERSION
#error This header requires libc++'s format implementation
#endif

#include "formatter.hpp"
#include "parse.hpp"
#include "utility.hpp"

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace ctf {
namespace detail {

using CharT = char;

template <std::size_t o, arg_id_status i, std::__format_spec::__parser<CharT> p>
struct parse_status {
  static constexpr std::size_t offset = o;
  static constexpr arg_id_status arg_id = i;
  static constexpr std::__format_spec::__parser<CharT> parser = p;
};

/***** FILL AND ALIGN *****/

// The fill should model a Unicode scalar value.
//
// However the format library uses the individual code units in its algorithms.
// So this function validates it's a valid UTF-8 sequence. When scanning the
// fill it's not sure the value is a fill, that is determined by the character
// after the fill. Since the entire format spec is valid Unicode there are no
// false positives.
template <std::size_t o, std::__format_spec::__code_point<char> v>
struct fill_result {
  static constexpr std::size_t offset = o;
  static constexpr std::__format_spec::__code_point<char> value = v;
};

template <fixed_string fmt, parse_status status> consteval auto parse_fill() {
  auto consume = [&]<fill_result fill, std::size_t index> {
    constexpr char code_unit = fmt.text[fill.offset];
    if constexpr ((code_unit & 0b1100'0000) != 0b1000'0000)
      return create_format_error("expected UTF-8 continuation code unit",
                                 std::string(fmt.text), status.offset,
                                 fill.offset, fill.offset);
    else
      return fill_result<fill.offset + 1,
                         std::__format_spec::__code_point<char>{
                             fill.value.__data[0],
                             (index == 1 ? code_unit : fill.value.__data[1]),
                             (index == 2 ? code_unit : fill.value.__data[2]),
                             (index == 3 ? code_unit
                                         : fill.value.__data[3])}>{};
  };

  constexpr char c = fmt.text[status.offset];
  auto cp_1 = fill_result<status.offset + 1,
                          std::__format_spec::__code_point<char>{c}>{};
  constexpr int bits = std::countl_one(static_cast<unsigned char>(c));
  if constexpr (bits == 0)
    return cp_1;
  else if constexpr (bits == 1)
    return create_format_error(
        "the UTF-8 code point starts with a continuation code unit",
        std::string(fmt.text), status.offset, status.offset, status.offset);
  else if constexpr (bits > 4)
    return create_format_error(
        "the UTF-8 code point starts with an invalid code unit",
        std::string(fmt.text), status.offset, status.offset, status.offset);
  else {
    auto cp_2 = consume.template operator()<cp_1, 1>();
    if constexpr (is_format_error(cp_2) || bits == 2)
      return cp_2;
    else {
      auto cp_3 = consume.template operator()<cp_2, 2>();
      if constexpr (is_format_error(cp_3) || bits == 3)
        return cp_3;
      else /* bits == 4 */
        // regardless whether valid or not this is the final result.
        return consume.template operator()<cp_3, 3>();
    }
  }
}

consteval std::__format_spec::__alignment get_alignment(char c) {
  switch (c) {
  case '<':
    return std::__format_spec::__alignment::__left;
  case '^':
    return std::__format_spec::__alignment::__center;
  case '>':
    return std::__format_spec::__alignment::__right;
  }
  return std::__format_spec::__alignment::__default;
}

template <std::__format_spec::__parser<CharT> parser,
          std::__format_spec::__code_point<char> fill,
          std::__format_spec::__alignment alignment>
consteval std::__format_spec::__parser<CharT> set_fill_align() {
  return {.__alignment_ = alignment,
          .__sign_ = parser.__sign_,
          .__alternate_form_ = parser.__alternate_form_,
          .__locale_specific_form_ = parser.__locale_specific_form_,
          .__clear_brackets_ = parser.__clear_brackets_,
          .__type_ = parser.__type_,
          .__hour_ = parser.__hour_,
          .__weekday_name_ = parser.__weekday_name_,
          .__weekday_ = parser.__weekday_,
          .__day_of_year_ = parser.__day_of_year_,
          .__week_of_year_ = parser.__week_of_year_,
          .__month_name_ = parser.__month_name_,
          .__reserved_0_ = parser.__reserved_0_,
          .__reserved_1_ = parser.__reserved_1_,
          .__width_as_arg_ = parser.__width_as_arg_,
          .__precision_as_arg_ = parser.__precision_as_arg_,
          .__width_ = parser.__width_,
          .__precision_ = parser.__precision_,
          .__fill_ = fill};
}

template <fixed_string fmt, std::size_t begin, parse_status status>
consteval auto parse_fill_align() {

  auto fill = parse_fill<fmt, status>();
  if constexpr (is_format_error(fill))
    return fill;
  else {
    constexpr std::__format_spec::__alignment alignment =
        get_alignment(fmt.text[fill.offset]);
    if constexpr (alignment != std::__format_spec::__alignment::__default)
      // replace text and alignment
      return parse_status<
          fill.offset + 1, status.arg_id,
          set_fill_align<status.parser, fill.value, alignment>()>{};

    else {
      constexpr std::__format_spec::__alignment alignment =
          get_alignment(fmt.text[status.offset]);
      if constexpr (alignment != std::__format_spec::__alignment::__default)
        // keep space and new aligment
        return parse_status<status.offset + 1, status.arg_id,
                            set_fill_align<status.parser, status.parser.__fill_,
                                           alignment>()>{};

      else
        return status;
    }
  }
}

/***** SIGN *****/

template <std::__format_spec::__parser<CharT> parser,
          std::__format_spec::__sign sign>
consteval std::__format_spec::__parser<CharT> set_sign() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = sign,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, parse_status status> consteval auto parse_sign() {
  auto consume = [&]<std::__format_spec::__sign sign> {
    return parse_status<status.offset + 1, status.arg_id,
                        set_sign<status.parser, sign>()>{};
  };
  constexpr auto c = fmt.text[status.offset];
  using enum std::__format_spec::__sign;
  if constexpr (c == CharT('-'))
    return consume.template operator()<__minus>();
  else if constexpr (c == CharT('+'))
    return consume.template operator()<__plus>();
  else if constexpr (c == CharT(' '))
    return consume.template operator()<__space>();
  else
    return status;
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_sign() {
  constexpr auto result = parse_sign<fmt, status>();
  if constexpr (result.offset != status.offset && !fields.__sign_)
    return create_format_error(
        "the format specification does not allow the sign option",
        std::string(fmt.text), begin, status.offset, status.offset);
  else
    return result;
}

/***** ALTERNATE FORM *****/

template <std::__format_spec::__parser<CharT> parser>
consteval std::__format_spec::__parser<CharT> set_alternate_form() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = true,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, parse_status status>
consteval auto parse_alternate_form() {
  if constexpr (fmt.text[status.offset] == CharT('#'))
    return parse_status<status.offset + 1, status.arg_id,
                        set_alternate_form<status.parser>()>{};
  else
    return status;
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_alternate_form() {
  constexpr auto result = parse_alternate_form<fmt, status>();
  if constexpr (result.offset != status.offset && !fields.__alternate_form_)
    return create_format_error(
        "the format specification does not allow the alternate form option",
        std::string(fmt.text), begin, status.offset, status.offset);
  else
    return result;
}

/***** ZERO-PADDING *****/

// Zero padding only has an effect when the align option is not set.
// The usage of the option is still valid so the option needs to be consumed or
// raise an error when invalid.

template <std::__format_spec::__parser<CharT> parser>
consteval std::__format_spec::__parser<CharT> set_zero_padding() {
  return {
      .__alignment_ =
          (parser.__alignment_ == std::__format_spec::__alignment::__default
               ? std::__format_spec::__alignment::__zero_padding
               : parser.__alignment_),
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, parse_status status>
consteval auto parse_zero_padding() {
  if constexpr (fmt.text[status.offset] == CharT('0'))
    return parse_status<status.offset + 1, status.arg_id,
                        set_zero_padding<status.parser>()>{};
  else
    return status;
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_zero_padding() {
  constexpr auto result = parse_zero_padding<fmt, status>();
  if constexpr (result.offset != status.offset && !fields.__zero_padding_)
    return create_format_error(
        "the format specification does not allow the zero-padding option",
        std::string(fmt.text), begin, status.offset, status.offset);
  else
    return result;
}

/***** WIDTH *****/

template <std::__format_spec::__parser<CharT> parser, int32_t width,
          bool width_as_arg>
consteval std::__format_spec::__parser<CharT> set_width() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = width_as_arg,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = width,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <std::size_t O, std::size_t I> struct get_arg_id_result {
  static constexpr std::size_t offset = O;
  static constexpr std::size_t arg_id = I;
};

template <fixed_string fmt, std::size_t begin, // at opening curly
          std::size_t arg_id, class... Args>
consteval auto get_arg_id() {
  constexpr auto c = fmt.text[begin + 1];
  if constexpr (c == CharT('}')) {
    if constexpr (arg_id != -1)
      return create_format_error("expected '}' while in automatic arg-id mode",
                                 std::string(fmt.text), begin, begin + 2,
                                 begin + 2, "}");
    else
      return get_arg_id_result<begin + 2, arg_id + 1>{};
  } else {
    if constexpr (arg_id != -1)
      return create_format_error("expected '}' while in automatic arg-id mode",
                                 std::string(fmt.text), begin, begin + 2,
                                 begin + 2, "}");
  }
}

template <fixed_string fmt, std::size_t begin, parse_status status,
          class... Args>
consteval auto parse_width() {
  constexpr auto c = fmt.text[status.offset];
  if constexpr (c == CharT('{')) {
    // Note this code needs to be shared between width and precision.
    constexpr auto c = fmt.text[status.offset + 1];

    //
    // TODO ADD - TEST IN MAIN PARSER TOO
    //
    //
    if constexpr (c == CharT('-'))
      return create_format_error(
          "the argument index may not be a negative value",
          std::string(fmt.text), begin, status.offset + 1, status.offset + 1);

    auto arg_id = parse_arg_id<fmt, status.offset + 1, status.arg_id>();

    if constexpr (ctf::is_format_error(arg_id))
      return arg_id;
    else {

      constexpr auto c = fmt.text[arg_id.offset];
      if constexpr (c != '}') {
        // TODO both branches have some duplicates.
        // TODO in this code we know the mode,
        if constexpr (arg_id.offset == status.offset + 1)
          // Nothing parsed so it's unknown what the user intended the { to be
          // part of.
          return ctf::create_format_error(
              (arg_id.offset == fmt.size
                   ? "unexpected end of the format string"
                   : "unexpected character in the arg-id"),
              std::string(fmt.text), status.offset + 1, arg_id.offset,
              arg_id.offset,
              "}     -> the end of the arg-id", //
              "[0-9] -> an arg-id");
        else
          // An arg-id was found, to the user intended it to be a
          // replacement-field.
          return ctf::create_format_error(
              (arg_id.offset == fmt.size
                   ? "unexpected end of the format string"
                   : "unexpected character in the arg-id"),
              std::string(fmt.text), status.offset + 1, arg_id.offset,
              arg_id.offset,
              "}     -> the end of the arg-id", //
              "[0-9] -> continuation of the arg-id");

      } else {

        using T =
            std::remove_reference_t<ctf::pack_type<arg_id.index, Args...>>;
        if constexpr (!std::same_as<T, int> && //
                      !std::same_as<T, unsigned int> &&
                      !std::same_as<T, long long> &&
                      !std::same_as<T, unsigned long long>)
          return create_format_error("the type of the arg-id is not a standard "
                                     "signed or unsigned integer type",
                                     std::string(fmt.text), status.offset,
                                     arg_id.offset, arg_id.offset);
        else
          return parse_status<arg_id.offset + 1, arg_id.status,
                              set_width<status.parser, arg_id.index, true>()>{};
      }
    }
  } else if constexpr (c == CharT('0')) {
    return create_format_error(
        "the width option should not have a leading zero",
        std::string(fmt.text), begin, status.offset, status.offset);
  } else if constexpr (c >= CharT('1') && c <= CharT('9')) {
    auto number =
        parse_number<fmt, parse_number_result<status.offset + 1, c - '0'>>();
    if constexpr (number.value == -1)
      return create_format_error("the value of the width option is larger than "
                                 "the implementation supports (2147483647)",
                                 std::string(fmt.text), begin, status.offset,
                                 status.offset);
    else
      return parse_status<number.offset, status.arg_id,
                          set_width<status.parser, number.value, false>()>{};
  } else
    return status;
}

/***** PRECISION *****/

template <std::__format_spec::__parser<CharT> parser, int32_t precision,
          bool precision_as_arg>
consteval std::__format_spec::__parser<CharT> set_precision() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = precision_as_arg,
      .__width_ = parser.__width_,
      .__precision_ = precision,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status,
          class... Args>
consteval auto parse_precision() {

  constexpr auto c = fmt.text[status.offset];

  if constexpr (c != CharT('.'))
    return status;
  else if constexpr (!fields.__precision_)
    return create_format_error(
        "the format specification does not allow the precicion option",
        std::string(fmt.text), begin, status.offset, status.offset);
  else {

    constexpr auto c = fmt.text[status.offset + 1];
    if constexpr (c == CharT('{')) {
      // Note this code needs to be shared between width and precision.
      constexpr auto c = fmt.text[status.offset + 2];

      //
      // TODO ADD - TEST IN MAIN PARSER TOO
      //
      //
      if constexpr (c == CharT('-'))
        return create_format_error(
            "the argument index may not be a negative value",
            std::string(fmt.text), begin, status.offset + 2, status.offset + 2);

      auto arg_id = parse_arg_id<fmt, status.offset + 2, status.arg_id>();

      if constexpr (ctf::is_format_error(arg_id))
        return arg_id;
      else {

        constexpr auto c = fmt.text[arg_id.offset];
        if constexpr (c != '}') {
          // TODO both branches have some duplicates.
          // TODO in this code we know the mode,
          if constexpr (arg_id.offset == status.offset + 2)
            // Nothing parsed so it's unknown what the user intended the { to be
            // part of.
            return ctf::create_format_error(
                (arg_id.offset == fmt.size
                     ? "unexpected end of the format string"
                     : "unexpected character in the arg-id"),
                std::string(fmt.text), status.offset + 2, arg_id.offset,
                arg_id.offset,
                "}     -> the end of the arg-id", //
                "[0-9] -> an arg-id");
          else
            // An arg-id was found, to the user intended it to be a
            // replacement-field.
            return ctf::create_format_error(
                (arg_id.offset == fmt.size
                     ? "unexpected end of the format string"
                     : "unexpected character in the arg-id"),
                std::string(fmt.text), status.offset + 1, arg_id.offset,
                arg_id.offset,
                "}     -> the end of the arg-id", //
                "[0-9] -> continuation of the arg-id");

        } else {

          using T =
              std::remove_reference_t<ctf::pack_type<arg_id.index, Args...>>;
          if constexpr (!std::same_as<T, int> && //
                        !std::same_as<T, unsigned int> &&
                        !std::same_as<T, long long> &&
                        !std::same_as<T, unsigned long long>)
            return create_format_error(
                "the type of the arg-id is not a standard "
                "signed or unsigned integer type",
                std::string(fmt.text), status.offset, arg_id.offset,
                arg_id.offset);
          else
            return parse_status<
                arg_id.offset + 1, arg_id.status,
                set_precision<status.parser, arg_id.index, true>()>{};
        }
      }
    } else if constexpr (c >= CharT('0') && c <= CharT('9')) {
      auto number =
          parse_number<fmt, parse_number_result<status.offset + 2, c - '0'>>();
      if constexpr (number.value == -1)
        return create_format_error(
            "the value of the precision option is larger than "
            "the implementation supports (2147483647)",
            std::string(fmt.text), begin, status.offset, status.offset);
      else
        return parse_status<
            number.offset, status.arg_id,
            set_precision<status.parser, number.value, false>()>{};
    }
  }
}

/***** LOCALE-SPECIFIC FORM *****/

template <std::__format_spec::__parser<CharT> parser>
consteval std::__format_spec::__parser<CharT> set_locale_specific_form() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = true,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, parse_status status>
consteval auto parse_locale_specific_form() {
  if constexpr (fmt.text[status.offset] == CharT('L'))
    return parse_status<status.offset + 1, status.arg_id,
                        set_locale_specific_form<status.parser>()>{};
  else
    return status;
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_locale_specific_form() {
  constexpr auto result = parse_locale_specific_form<fmt, status>();
  if constexpr (result.offset != status.offset &&
                !fields.__locale_specific_form_)
    return create_format_error("the format specification does not allow the "
                               "locale-specific form option",
                               std::string(fmt.text), begin, status.offset,
                               status.offset);
  else
    return result;
}

/***** CLEAR BRACKETS *****/

template <std::__format_spec::__parser<CharT> parser>
consteval std::__format_spec::__parser<CharT> set_clear_brackets() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = true,
      .__type_ = parser.__type_,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, parse_status status>
consteval auto parse_clear_brackets() {
  if constexpr (fmt.text[status.offset] == CharT('n'))
    return parse_status<status.offset + 1, status.arg_id,
                        set_clear_brackets<status.parser>()>{};
  else
    return status;
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_clear_brackets() {
  constexpr auto result = parse_clear_brackets<fmt, status>();
  if constexpr (result.offset != status.offset && !fields.__clear_brackets_)
    return create_format_error(
        "the format specification does not allow the clear brackets option",
        std::string(fmt.text), begin, status.offset, status.offset);
  else
    return result;
}

/***** TYPE *****/

template <std::__format_spec::__parser<CharT> parser,
          std::__format_spec::__type type>
consteval std::__format_spec::__parser<CharT> set_type() {
  return {
      .__alignment_ = parser.__alignment_,
      .__sign_ = parser.__sign_,
      .__alternate_form_ = parser.__alternate_form_,
      .__locale_specific_form_ = parser.__locale_specific_form_,
      .__clear_brackets_ = parser.__clear_brackets_,
      .__type_ = type,
      .__hour_ = parser.__hour_,
      .__weekday_name_ = parser.__weekday_name_,
      .__weekday_ = parser.__weekday_,
      .__day_of_year_ = parser.__day_of_year_,
      .__week_of_year_ = parser.__week_of_year_,
      .__month_name_ = parser.__month_name_,
      .__reserved_0_ = parser.__reserved_0_,
      .__reserved_1_ = parser.__reserved_1_,
      .__width_as_arg_ = parser.__width_as_arg_,
      .__precision_as_arg_ = parser.__precision_as_arg_,
      .__width_ = parser.__width_,
      .__precision_ = parser.__precision_,
      .__fill_ = parser.__fill_,
  };
}

template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status>
consteval auto parse_type() {
  auto consume = [&]<std::__format_spec::__type type> {
    return parse_status<status.offset + 1, status.arg_id,
                        set_type<status.parser, type>()>{};
  };

  if constexpr (!fields.__type_)
    return status;
  else {
    constexpr auto c = fmt.text[status.offset];
    using enum std::__format_spec::__type;
    if constexpr (c == CharT('A'))
      return consume.template operator()<__hexfloat_upper_case>();
    else if constexpr (c == CharT('B'))
      return consume.template operator()<__binary_upper_case>();
    else if constexpr (c == CharT('E'))
      return consume.template operator()<__scientific_upper_case>();
    else if constexpr (c == CharT('F'))
      return consume.template operator()<__fixed_upper_case>();
    else if constexpr (c == CharT('G'))
      return consume.template operator()<__general_upper_case>();
    else if constexpr (c == CharT('X'))
      return consume.template operator()<__hexadecimal_upper_case>();
    else if constexpr (c == CharT('a'))
      return consume.template operator()<__hexfloat_lower_case>();
    else if constexpr (c == CharT('b'))
      return consume.template operator()<__binary_lower_case>();
    else if constexpr (c == CharT('c'))
      return consume.template operator()<__char>();
    else if constexpr (c == CharT('d'))
      return consume.template operator()<__decimal>();
    else if constexpr (c == CharT('e'))
      return consume.template operator()<__scientific_lower_case>();
    else if constexpr (c == CharT('f'))
      return consume.template operator()<__fixed_lower_case>();
    else if constexpr (c == CharT('g'))
      return consume.template operator()<__general_lower_case>();
    else if constexpr (c == CharT('o'))
      return consume.template operator()<__octal>();
    else if constexpr (c == CharT('p'))
      return consume.template operator()<__pointer_lower_case>();
    else if constexpr (c == CharT('P'))
      return consume.template operator()<__pointer_upper_case>();
    else if constexpr (c == CharT('s'))
      return consume.template operator()<__string>();
    else if constexpr (c == CharT('x'))
      return consume.template operator()<__hexadecimal_lower_case>();
#if _LIBCPP_STD_VER >= 23
    else if constexpr (c == CharT('?'))
      return consume.template operator()<__debug>();
#endif
    else
      return status;
  }
}

/***** PARSE *****/

// Note when this function is called arg_id is already set to manual or
// automatic since the replacement-field has an id. -1 is manual other values
// are the last automatically parsed value.
template <fixed_string fmt, std::size_t begin,
          std::__format_spec::__fields fields, parse_status status,
          class... Args>
consteval auto parse() {
  //
  // Note from all fields only 1 flag can be send the function.
  // This reduces the number of instantiations.
  //

  if constexpr (begin == fmt.size)
    return status;
  else {
    auto fill_align = parse_fill_align<fmt, begin, status>();
    if constexpr (ctf::is_format_error(fill_align))
      return fill_align;
    else {
      auto sign = parse_sign<fmt, begin, fields, fill_align>();
      if constexpr (ctf::is_format_error(sign))
        return sign;
      else {
        auto alternate_form = parse_alternate_form<fmt, begin, fields, sign>();
        if constexpr (ctf::is_format_error(alternate_form))
          return alternate_form;
        else {
          auto zero_padding =
              parse_zero_padding<fmt, begin, fields, alternate_form>();
          if constexpr (ctf::is_format_error(zero_padding))
            return zero_padding;
          else {
            auto width = parse_width<fmt, begin, zero_padding, Args...>();
            if constexpr (ctf::is_format_error(width))
              return width;
            else {
              auto precision =
                  parse_precision<fmt, begin, fields, width, Args...>();
              if constexpr (ctf::is_format_error(precision))
                return precision;
              else {
                auto locale_specific_form =
                    parse_locale_specific_form<fmt, begin, fields, precision>();
                if constexpr (ctf::is_format_error(locale_specific_form))
                  return locale_specific_form;
                else {
                  auto clear_brackets =
                      parse_clear_brackets<fmt, begin, fields,
                                           locale_specific_form>();
                  if constexpr (ctf::is_format_error(clear_brackets))
                    return clear_brackets;
                  else {
                    auto type =
                        parse_type<fmt, begin, fields, clear_brackets>();
                    if constexpr (ctf::is_format_error(type))
                      return type;
                    else {
                      if constexpr (fields.__consume_all_ &&
                                    type.offset != fmt.size &&
                                    fmt.text[type.offset] != CharT('}')) {

                        return format_error{
                            std::string("TODO IMPLEMENT proper message")};

                      } else
                        return type;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

template <fixed_string fmt, std::size_t begin, parse_status status>
consteval auto process_parsed_string() {
  constexpr auto type = status.parser.__type_;
  if constexpr (type == std::__format_spec::__type::__default ||
                type == std::__format_spec::__type::__string ||
                type == std::__format_spec::__type::__debug)
    return status;
  else
    return format_error{
        "\nthe display type is not valid for a string argument\n" +
        std::string(fmt.text) + "\n" + //
        std::string(begin, ' ') +
        std::string((status.offset - 1) - begin, '~') + '^'};
}

} // namespace detail

template <fixed_string fmt, std::size_t begin, arg_id_status arg_id,
          class... Args>
struct formatter<std::basic_string_view<char>, fmt, begin, arg_id, Args...> {

  static consteval auto create() {

    auto status = detail::parse<
        fmt, begin, std::__format_spec::__fields_string,
        detail::parse_status<begin, arg_id,
                             std::__format_spec::__parser<char>{
                                 std::__format_spec::__alignment::__left}>{},
        Args...>();

    if constexpr (ctf::is_format_error(status))
      return status;
    else {
      auto result = detail::process_parsed_string<fmt, begin, status>();
      if constexpr (ctf::is_format_error(result))
        return result;
      else {
        using F = std::formatter<std::basic_string_view<char>>;
        return formatter_result<result.offset, result.arg_id, F>{
            F{result.parser}};
      }
    }
  }
};

} // namespace ctf

#endif // CTF_FORMATTER_STRING_HPP
