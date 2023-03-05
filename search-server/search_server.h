#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iterator>
#include <deque>
#include <execution>
#include <future>
#include <type_traits>

#include "read_input_functions.h"
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    explicit SearchServer(const std::string_view& stop_words_text)
            : stop_words_text_(std::string(stop_words_text)) {
        SearchServer(SplitIntoWords(stop_words_text));
    }

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindTopDocuments(const Policy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <typename Policy>
    std::vector<Document> FindTopDocuments(const Policy& policy, const std::string_view& raw_query) const;
    template <typename Policy>
    std::vector<Document> FindTopDocuments(const Policy& policy, const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            const std::execution::sequenced_policy& seq, const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            const std::execution::parallel_policy& par, const std::string_view& raw_query,int document_id) const;

    auto begin() const {
        return document_ids_.begin();
    }

    auto end() const {
        return document_ids_.end();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& seq, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& par, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::string stop_words_text_;
    std::deque<std::string> all_docs_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> id_to_word_freqs_;

    bool IsStopWord(const std::string_view& word) const;

    static bool IsValidWord(const std::string_view& word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view& text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view& text) const;
    Query ParseQuery(const std::execution::parallel_policy& par, const std::string_view& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindAllDocuments(const Policy& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
              : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(const Policy& policy,
                                                     const std::string_view& raw_query,
                                                     DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs) {
                  if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_EPSILON) {
                      return lhs.rating > rhs.rating;
                  } else {
                      return lhs.relevance > rhs.relevance;
                  }
              });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query,
                                                     DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(const Policy& policy, const std::string_view& raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(const Policy& policy,
                                                     const std::string_view& raw_query,
                                                     DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindAllDocuments(const Policy& policy,
                                                     const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::vector<Document> matched_documents;

    if constexpr (std::is_same_v<std::decay_t<Policy>, std::execution::parallel_policy>) {

        static constexpr int NUM_THREADS = 16;
        ConcurrentMap<int, double> document_to_relevance(NUM_THREADS);

        std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
                      [&](const std::string_view& it) {
                          if (word_to_document_freqs_.count(it) != 0) {
                             const double inverse_document_freq = ComputeWordInverseDocumentFreq(it);
                             for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(it)) {
                                 const auto& document_data = documents_.at(document_id);
                                 if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                     document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                                 }
                             }
                          }
        });

        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        matched_documents.reserve(document_to_relevance.size());
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
            matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating});
        }
    } else {

        std::map<int, double> document_to_relevance;
        for (const std::string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        matched_documents.reserve(document_to_relevance.size());
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating});
        }
    }
    return matched_documents;
}