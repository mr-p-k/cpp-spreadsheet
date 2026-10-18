#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>

#include "common.h"

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

constexpr Position Position::NONE = {-1, -1};

namespace {

int ParseColumn(std::string_view str) {
    int result = 0;
    for (const char kC : str) {
        if (kC < 'A' || kC > 'Z') {
            return -1;
        }

        const auto kCh = static_cast<unsigned char>(kC);

        const int kValue = kCh - 'A' + 1;

        result = (result * LETTERS) + kValue;

        if (result > Position::MAX_COLS) {
            return -1;
        }
    }

    return result;
}

int ParseRow(std::string_view str) {
    int result = 0;

    auto [ptr, err] =
        std::from_chars(str.data(), str.data() + str.size(), result);

    if (err != std::errc{} || ptr != str.data() + str.size() ||
        result > Position::MAX_ROWS) {
        //
        return -1;
    }

    return result;
}

char DigitToChar(int d) {
    assert(d >= 0 && d < LETTERS);

    return static_cast<char>('A' + d);
}

std::string NumberToString(int num, int base) {
    assert(0 < num);
    assert(0 < base);

    std::string tmp;
    tmp.reserve(MAX_POS_LETTER_COUNT);

    while (num > 0) {
        --num;

        tmp.push_back(DigitToChar(num % base));

        num /= base;
    }

    std::reverse(tmp.begin(), tmp.end());

    return tmp;
}

}  // namespace

// Реализуйте методы:
bool Position::operator==(const Position rhs) const {
    return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    if (*this == Position::NONE) {
        return false;
    }

    return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }

    return std::string(NumberToString(col + 1, LETTERS) +
                       std::to_string(row + 1));
}

Position Position::FromString(std::string_view str) {
    if (str.empty() || MAX_POSITION_LENGTH < str.size()) {
        //
        return Position::NONE;
    }

    const auto* digit_it = std::find_if(str.cbegin(), str.cend(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });

    const size_t kDigitPos = std::distance(str.cbegin(), digit_it);

    if (kDigitPos == 0 || kDigitPos > MAX_POS_LETTER_COUNT) {
        return Position::NONE;
    }

    const auto kDigits = str.substr(kDigitPos);

    if (kDigits.empty() || '0' == kDigits.front()) {
        return Position::NONE;
    }

    const int kCol = ParseColumn(str.substr(0, kDigitPos)) - 1;
    const int kRow = ParseRow(kDigits) - 1;

    if (0 > kCol || 0 > kRow) {
        return Position::NONE;
    }

    return {kRow, kCol};
}

bool Size::operator==(Size rhs) const {
    return std::tie(rows, cols) == std::tie(rhs.rows, rhs.cols);
}