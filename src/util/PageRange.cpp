#include "util/PageRange.h"

#include <charconv>
#include <cstddef>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

#include "util/i18n.h"

/**
 * @brief Parse a string of page ranges.
 *
 * This function parses a string of page ranges into a vector of pairs of page
 * numbers. A page range is of the form n, n-, -m, n-m where n, m are positive
 * integers. The input - is also accepted. Page ranges are separated by `,`, `;`
 * and `:`. Whitespace is ignored. The parameter 'pageCount' is the largest page
 * number that may be refered to. The function silently corrects page numbers
 * equal to 0 or larger than pageCount by truncating to the nearest
 * acceptable integer.
 *
 * Example input and output:
 *
 *     parse("1, 2-, -3, 4-5, -, 0-42", 10)
 *     ===>
 *     {0, 0}, {1, 9}, {0, 2}, {3, 4}, {0, 9}, {0, 9}
 *
 * Note that the page numbers are parsed in the format 1 - pageCount
 * (permissive) and the return values are in the range 0 - (pageCount-1).
 *
 * @param s The string containing page ranges to parse.
 * @param pageCount The largest page that is possible to refer to.
 * @exception std::logic_error If `pageCount == 0`.
 * @exception std::invalid_argument If the input doesn't match any acceptable
 * page ranges.
 * @return A vector containing the page ranges.
 */
std::vector<PageRangeEntry> PageRange::parse(const std::string& s, size_t pageCount) {
    // break the string into comma (or ;:) separated tokens.
    const std::regex separators("[,;:]");
    auto begin = std::sregex_token_iterator(s.cbegin(), s.cend(), separators, -1);
    auto end = std::sregex_token_iterator();

    std::vector<PageRangeEntry> entries;
    if (pageCount == 0) {
        throw std::logic_error("PageRange::parse(): pageCount is zero.");
    }

    /* The following input cases are considered:
     *
     *   1) n,   parsed by singlePage,
     *   2) n-,  parsed by rightOpenRange,
     *   3) -m,  parsed by leftOpenRange,
     *   4) n-m, parsed by pageRange,
     *   5) -,   parsed by bothOpenRange,
     *   6) For everything else, an exception is thrown.
     */
    const std::regex singlePage("\\s*(\\d+)\\s*");
    const std::regex rightOpenRange("\\s*(\\d+)\\s*-\\s*");
    const std::regex leftOpenRange("\\s*-\\s*(\\d+)\\s*");
    const std::regex pageRange("\\s*(\\d+)\\s*-\\s*(\\d+)\\s*");
    const std::regex bothOpenRange("\\s*-\\s*");

    // for each token separated by `separators`
    for (auto it = begin; it != end; it++) {
        std::smatch match;
        PageRangeEntry entry;
        const std::string str = it->str();
        // This if-else block determines which case of 1) through 6) we deal with
        if (std::regex_match(str, match, singlePage)) {
            const auto token = match[1].str();
            std::from_chars(token.data(), token.data() + token.size(), entry.first);
            entry.last = entry.first;
        } else if (std::regex_match(str, match, rightOpenRange)) {
            const auto token = match[1].str();
            std::from_chars(token.data(), token.data() + token.size(), entry.first);
            entry.last = pageCount;
        } else if (std::regex_match(str, match, leftOpenRange)) {
            const auto token = match[1].str();
            std::from_chars(token.data(), token.data() + token.size(), entry.last);
            entry.first = 1;
        } else if (std::regex_match(str, match, pageRange)) {
            const auto tokenFirst = match[1].str();
            const auto tokenLast = match[2].str();
            std::from_chars(tokenFirst.data(), tokenFirst.data() + tokenFirst.size(), entry.first);
            std::from_chars(tokenLast.data(), tokenLast.data() + tokenLast.size(), entry.last);
        } else if (std::regex_match(str, match, bothOpenRange)) {
            entry.first = 1;
            entry.last = pageCount;
        } else {
            throw std::invalid_argument(_("PageRange::parse(): invalid page range."));
        }

        if (entry.first > pageCount || entry.last > pageCount) {
            throw std::invalid_argument(_("PageRange::parse(): given page number is larger than maximum count."));
        }
        if (entry.last < entry.first) {
            throw std::invalid_argument(_("PageRange::parse(): interval bounds must be in increasing order."));
        }
        if (entry.first == 0) {  // entry.last cannot be 0 unless entry.first is.
            throw std::invalid_argument(_("PageRange::parse(): page numbers start with 1"));
        }

        // decrease the ranges because they should start from 0 instead of 1
        entry.first--;  // Cannot already be 0
        entry.last--;   // Cannot already be 0
        entries.push_back(entry);
    }
    return entries;
}
