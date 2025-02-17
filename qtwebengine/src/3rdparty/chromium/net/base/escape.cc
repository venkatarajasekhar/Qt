// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/escape.h"

#include <algorithm>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_offset_string_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace net {

namespace {

const char kHexString[] = "0123456789ABCDEF";
inline char IntToHex(int i) {
  DCHECK_GE(i, 0) << i << " not a hex value";
  DCHECK_LE(i, 15) << i << " not a hex value";
  return kHexString[i];
}

// A fast bit-vector map for ascii characters.
//
// Internally stores 256 bits in an array of 8 ints.
// Does quick bit-flicking to lookup needed characters.
struct Charmap {
  bool Contains(unsigned char c) const {
    return ((map[c >> 5] & (1 << (c & 31))) != 0);
  }

  uint32 map[8];
};

// Given text to escape and a Charmap defining which values to escape,
// return an escaped string.  If use_plus is true, spaces are converted
// to +, otherwise, if spaces are in the charmap, they are converted to
// %20.
std::string Escape(const std::string& text, const Charmap& charmap,
                   bool use_plus) {
  std::string escaped;
  escaped.reserve(text.length() * 3);
  for (unsigned int i = 0; i < text.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (use_plus && ' ' == c) {
      escaped.push_back('+');
    } else if (charmap.Contains(c)) {
      escaped.push_back('%');
      escaped.push_back(IntToHex(c >> 4));
      escaped.push_back(IntToHex(c & 0xf));
    } else {
      escaped.push_back(c);
    }
  }
  return escaped;
}

// Contains nonzero when the corresponding character is unescapable for normal
// URLs. These characters are the ones that may change the parsing of a URL, so
// we don't want to unescape them sometimes. In many case we won't want to
// unescape spaces, but that is controlled by parameters to Unescape*.
//
// The basic rule is that we can't unescape anything that would changing parsing
// like # or ?. We also can't unescape &, =, or + since that could be part of a
// query and that could change the server's parsing of the query. Nor can we
// unescape \ since src/url/ will convert it to a /.
//
// Lastly, we can't unescape anything that doesn't have a canonical
// representation in a URL. This means that unescaping will change the URL, and
// you could get different behavior if you copy and paste the URL, or press
// enter in the URL bar. The list of characters that fall into this category
// are the ones labeled PASS (allow either escaped or unescaped) in the big
// lookup table at the top of url/url_canon_path.cc.  Also, characters
// that have CHAR_QUERY set in url/url_canon_internal.cc but are not
// allowed in query strings according to http://www.ietf.org/rfc/rfc3261.txt are
// not unescaped, to avoid turning a valid url according to spec into an
// invalid one.
const char kUrlUnescape[128] = {
//   NULL, control chars...
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  ' ' !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
     0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
//   0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0,
//   @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//   P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
//   `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//   p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  <NBSP>
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0
};

// Attempts to unescape the sequence at |index| within |escaped_text|.  If
// successful, sets |value| to the unescaped value.  Returns whether
// unescaping succeeded.
template<typename STR>
bool UnescapeUnsignedCharAtIndex(const STR& escaped_text,
                                 size_t index,
                                 unsigned char* value) {
  if ((index + 2) >= escaped_text.size())
    return false;
  if (escaped_text[index] != '%')
    return false;
  const typename STR::value_type most_sig_digit(
      static_cast<typename STR::value_type>(escaped_text[index + 1]));
  const typename STR::value_type least_sig_digit(
      static_cast<typename STR::value_type>(escaped_text[index + 2]));
  if (IsHexDigit(most_sig_digit) && IsHexDigit(least_sig_digit)) {
    *value = HexDigitToInt(most_sig_digit) * 16 +
      HexDigitToInt(least_sig_digit);
    return true;
  }
  return false;
}

// Unescapes |escaped_text| according to |rules|, returning the resulting
// string.  Fills in an |adjustments| parameter, if non-NULL, so it reflects
// the alterations done to the string that are not one-character-to-one-
// character.  The resulting |adjustments| will always be sorted by increasing
// offset.
template<typename STR>
STR UnescapeURLWithAdjustmentsImpl(
    const STR& escaped_text,
    UnescapeRule::Type rules,
    base::OffsetAdjuster::Adjustments* adjustments) {
  if (adjustments)
    adjustments->clear();
  // Do not unescape anything, return the |escaped_text| text.
  if (rules == UnescapeRule::NONE)
    return escaped_text;

  // The output of the unescaping is always smaller than the input, so we can
  // reserve the input size to make sure we have enough buffer and don't have
  // to allocate in the loop below.
  STR result;
  result.reserve(escaped_text.length());

  // Locations of adjusted text.
  for (size_t i = 0, max = escaped_text.size(); i < max; ++i) {
    if (static_cast<unsigned char>(escaped_text[i]) >= 128) {
      // Non ASCII character, append as is.
      result.push_back(escaped_text[i]);
      continue;
    }

    unsigned char first_byte;
    if (UnescapeUnsignedCharAtIndex(escaped_text, i, &first_byte)) {
      // Per http://tools.ietf.org/html/rfc3987#section-4.1, the following BiDi
      // control characters are not allowed to appear unescaped in URLs:
      //
      // U+200E LEFT-TO-RIGHT MARK         (%E2%80%8E)
      // U+200F RIGHT-TO-LEFT MARK         (%E2%80%8F)
      // U+202A LEFT-TO-RIGHT EMBEDDING    (%E2%80%AA)
      // U+202B RIGHT-TO-LEFT EMBEDDING    (%E2%80%AB)
      // U+202C POP DIRECTIONAL FORMATTING (%E2%80%AC)
      // U+202D LEFT-TO-RIGHT OVERRIDE     (%E2%80%AD)
      // U+202E RIGHT-TO-LEFT OVERRIDE     (%E2%80%AE)
      //
      // Additionally, the Unicode Technical Report (TR9) as referenced by RFC
      // 3987 above has since added some new BiDi control characters.
      // http://www.unicode.org/reports/tr9
      //
      // U+061C ARABIC LETTER MARK         (%D8%9C)
      // U+2066 LEFT-TO-RIGHT ISOLATE      (%E2%81%A6)
      // U+2067 RIGHT-TO-LEFT ISOLATE      (%E2%81%A7)
      // U+2068 FIRST STRONG ISOLATE       (%E2%81%A8)
      // U+2069 POP DIRECTIONAL ISOLATE    (%E2%81%A9)

      unsigned char second_byte;
      // Check for ALM.
      if ((first_byte == 0xD8) &&
          UnescapeUnsignedCharAtIndex(escaped_text, i + 3, &second_byte) &&
          (second_byte == 0x9c)) {
        result.append(escaped_text, i, 6);
        i += 5;
        continue;
      }

      // Check for other BiDi control characters.
      if ((first_byte == 0xE2) &&
          UnescapeUnsignedCharAtIndex(escaped_text, i + 3, &second_byte) &&
          ((second_byte == 0x80) || (second_byte == 0x81))) {
        unsigned char third_byte;
        if (UnescapeUnsignedCharAtIndex(escaped_text, i + 6, &third_byte) &&
            ((second_byte == 0x80) ?
             ((third_byte == 0x8E) || (third_byte == 0x8F) ||
              ((third_byte >= 0xAA) && (third_byte <= 0xAE))) :
             ((third_byte >= 0xA6) && (third_byte <= 0xA9)))) {
          result.append(escaped_text, i, 9);
          i += 8;
          continue;
        }
      }

      if (first_byte >= 0x80 ||  // Unescape all high-bit characters.
          // For 7-bit characters, the lookup table tells us all valid chars.
          (kUrlUnescape[first_byte] ||
           // ...and we allow some additional unescaping when flags are set.
           (first_byte == ' ' && (rules & UnescapeRule::SPACES)) ||
           // Allow any of the prohibited but non-control characters when
           // we're doing "special" chars.
           (first_byte > ' ' && (rules & UnescapeRule::URL_SPECIAL_CHARS)) ||
           // Additionally allow control characters if requested.
           (first_byte < ' ' && (rules & UnescapeRule::CONTROL_CHARS)))) {
        // Use the unescaped version of the character.
        if (adjustments)
          adjustments->push_back(base::OffsetAdjuster::Adjustment(i, 3, 1));
        result.push_back(first_byte);
        i += 2;
      } else {
        // Keep escaped. Append a percent and we'll get the following two
        // digits on the next loops through.
        result.push_back('%');
      }
    } else if ((rules & UnescapeRule::REPLACE_PLUS_WITH_SPACE) &&
               escaped_text[i] == '+') {
      result.push_back(' ');
    } else {
      // Normal case for unescaped characters.
      result.push_back(escaped_text[i]);
    }
  }

  return result;
}

template <class str>
void AppendEscapedCharForHTMLImpl(typename str::value_type c, str* output) {
  static const struct {
    char key;
    const char* replacement;
  } kCharsToEscape[] = {
    { '<', "&lt;" },
    { '>', "&gt;" },
    { '&', "&amp;" },
    { '"', "&quot;" },
    { '\'', "&#39;" },
  };
  size_t k;
  for (k = 0; k < ARRAYSIZE_UNSAFE(kCharsToEscape); ++k) {
    if (c == kCharsToEscape[k].key) {
      const char* p = kCharsToEscape[k].replacement;
      while (*p)
        output->push_back(*p++);
      break;
    }
  }
  if (k == ARRAYSIZE_UNSAFE(kCharsToEscape))
    output->push_back(c);
}

template <class str>
str EscapeForHTMLImpl(const str& input) {
  str result;
  result.reserve(input.size());  // Optimize for no escaping.

  for (typename str::const_iterator i = input.begin(); i != input.end(); ++i)
    AppendEscapedCharForHTMLImpl(*i, &result);

  return result;
}

// Everything except alphanumerics and !'()*-._~
// See RFC 2396 for the list of reserved characters.
static const Charmap kQueryCharmap = {{
  0xffffffffL, 0xfc00987dL, 0x78000001L, 0xb8000001L,
  0xffffffffL, 0xffffffffL, 0xffffffffL, 0xffffffffL
}};

// non-printable, non-7bit, and (including space)  "#%:<>?[\]^`{|}
static const Charmap kPathCharmap = {{
  0xffffffffL, 0xd400002dL, 0x78000000L, 0xb8000001L,
  0xffffffffL, 0xffffffffL, 0xffffffffL, 0xffffffffL
}};

// non-printable, non-7bit, and (including space) ?>=<;+'&%$#"![\]^`{|}
static const Charmap kUrlEscape = {{
  0xffffffffL, 0xf80008fdL, 0x78000001L, 0xb8000001L,
  0xffffffffL, 0xffffffffL, 0xffffffffL, 0xffffffffL
}};

// non-7bit
static const Charmap kNonASCIICharmap = {{
  0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
  0xffffffffL, 0xffffffffL, 0xffffffffL, 0xffffffffL
}};

// Everything except alphanumerics, the reserved characters(;/?:@&=+$,) and
// !'()*-._~%
static const Charmap kExternalHandlerCharmap = {{
  0xffffffffL, 0x5000080dL, 0x68000000L, 0xb8000001L,
  0xffffffffL, 0xffffffffL, 0xffffffffL, 0xffffffffL
}};

}  // namespace

std::string EscapeQueryParamValue(const std::string& text, bool use_plus) {
  return Escape(text, kQueryCharmap, use_plus);
}

std::string EscapePath(const std::string& path) {
  return Escape(path, kPathCharmap, false);
}

std::string EscapeUrlEncodedData(const std::string& path, bool use_plus) {
  return Escape(path, kUrlEscape, use_plus);
}

std::string EscapeNonASCII(const std::string& input) {
  return Escape(input, kNonASCIICharmap, false);
}

std::string EscapeExternalHandlerValue(const std::string& text) {
  return Escape(text, kExternalHandlerCharmap, false);
}

void AppendEscapedCharForHTML(char c, std::string* output) {
  AppendEscapedCharForHTMLImpl(c, output);
}

std::string EscapeForHTML(const std::string& input) {
  return EscapeForHTMLImpl(input);
}

base::string16 EscapeForHTML(const base::string16& input) {
  return EscapeForHTMLImpl(input);
}

std::string UnescapeURLComponent(const std::string& escaped_text,
                                 UnescapeRule::Type rules) {
  return UnescapeURLWithAdjustmentsImpl(escaped_text, rules, NULL);
}

base::string16 UnescapeURLComponent(const base::string16& escaped_text,
                                    UnescapeRule::Type rules) {
  return UnescapeURLWithAdjustmentsImpl(escaped_text, rules, NULL);
}

base::string16 UnescapeAndDecodeUTF8URLComponent(const std::string& text,
                                                 UnescapeRule::Type rules) {
  return UnescapeAndDecodeUTF8URLComponentWithAdjustments(text, rules, NULL);
}

base::string16 UnescapeAndDecodeUTF8URLComponentWithAdjustments(
    const std::string& text,
    UnescapeRule::Type rules,
    base::OffsetAdjuster::Adjustments* adjustments) {
  base::string16 result;
  base::OffsetAdjuster::Adjustments unescape_adjustments;
  std::string unescaped_url(UnescapeURLWithAdjustmentsImpl(
      text, rules, &unescape_adjustments));
  if (base::UTF8ToUTF16WithAdjustments(unescaped_url.data(),
                                       unescaped_url.length(),
                                       &result, adjustments)) {
    // Character set looks like it's valid.
    if (adjustments) {
      base::OffsetAdjuster::MergeSequentialAdjustments(unescape_adjustments,
                                                       adjustments);
    }
    return result;
  }
  // Character set is not valid.  Return the escaped version.
  return base::UTF8ToUTF16WithAdjustments(text, adjustments);
}

base::string16 UnescapeForHTML(const base::string16& input) {
  static const struct {
    const char* ampersand_code;
    const char replacement;
  } kEscapeToChars[] = {
    { "&lt;", '<' },
    { "&gt;", '>' },
    { "&amp;", '&' },
    { "&quot;", '"' },
    { "&#39;", '\''},
  };

  if (input.find(base::ASCIIToUTF16("&")) == std::string::npos)
    return input;

  base::string16 ampersand_chars[ARRAYSIZE_UNSAFE(kEscapeToChars)];
  base::string16 text(input);
  for (base::string16::iterator iter = text.begin();
       iter != text.end(); ++iter) {
    if (*iter == '&') {
      // Potential ampersand encode char.
      size_t index = iter - text.begin();
      for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kEscapeToChars); i++) {
        if (ampersand_chars[i].empty()) {
          ampersand_chars[i] =
              base::ASCIIToUTF16(kEscapeToChars[i].ampersand_code);
        }
        if (text.find(ampersand_chars[i], index) == index) {
          text.replace(iter, iter + ampersand_chars[i].length(),
                       1, kEscapeToChars[i].replacement);
          break;
        }
      }
    }
  }
  return text;
}

}  // namespace net
