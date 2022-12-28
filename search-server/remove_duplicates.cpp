#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    for (auto it_out = search_server.begin(); it_out != search_server.end(); ++it_out) {
        auto out_words = search_server.GetWordFrequencies(*it_out);
        for (auto it_in = next(it_out); it_in != search_server.end(); ) {
            auto in_words = search_server.GetWordFrequencies(*it_in);
            if (std::equal(out_words.begin(), out_words.end(), in_words.begin(), in_words.end(),
                           [] (auto a, auto b) { return a.first == b.first; })) {
                std::cout << "Found duplicate document id " << *it_in << std::endl;
                search_server.RemoveDocument(*it_in);
            } else {
                ++it_in;
            }
        }
    }
}