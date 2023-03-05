#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view& text) {
    std::vector<std::string_view> words;
    int64_t pos = 0;
    const int64_t pos_end = text.npos;
    while (true) {
        int64_t space = text.find(' ', pos);
        words.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
        if (space == pos_end) {
            break;
        } else {
            pos = space + 1;
        }
    }
    return words;
}