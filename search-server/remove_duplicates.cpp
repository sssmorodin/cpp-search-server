#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> invalid_ids;
    std::set<std::set<std::string>> documents_words;

    for (int document_id : search_server) {
        const auto& words_to_freq = search_server.GetWordFrequencies(document_id);
        std::set<std::string> document_words;
        for (const auto& word_with_freq: words_to_freq) {
            document_words.insert(word_with_freq.first);
        }
        if (0 == documents_words.count(document_words)) {
            documents_words.insert(document_words);
        } else {
            std::cout << "Found duplicate document id " << document_id << std::endl;
            invalid_ids.insert(document_id);
        }
    }
    for (int id : invalid_ids) {
        search_server.RemoveDocument(id);
    }
}