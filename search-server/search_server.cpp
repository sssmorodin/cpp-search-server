#include "search_server.h"

using namespace std::string_literals;


void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
                               const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    all_docs_.push_back(std::string(document));
    const auto words = SplitIntoWordsNoStop(all_docs_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
        const std::string_view& raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
        const std::execution::sequenced_policy& seq, const std::string_view& raw_query, int document_id) const {
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Document not found"s);
    }
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words(query.plus_words.size());
    //проверить сначала минус-слова с помощью std::any_of
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(),
                    [&](const std::string_view& word) {
                        return word_to_document_freqs_.at(word).count(document_id);
                    })) {
        std::vector<std::string_view> out;
        return {out, documents_.at(document_id).status};
    }

    auto copy_last_it = std::copy_if(query.plus_words.begin(), query.plus_words.end(),
                                     matched_words.begin(),
                                     [&](const auto& word) {
                                         return word_to_document_freqs_.at(word).count(document_id);
                                     });
    matched_words.resize(std::distance(matched_words.begin(), copy_last_it));
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
        const std::execution::parallel_policy& par, const std::string_view& raw_query,int document_id) const {
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Document not found"s);
    }
    const auto query = ParseQuery(par, raw_query);
    //избавиться от дубликатов плюс и минус слов
    //std::sort--std::unique--std::vector::erase.

    /*
     * при создании вектора результатов задайте его размер сразу (подумайте - какой это размер
     * может быть максимальным). И используйте std::copy_if - обратите внимание на возвращаемый
     * этим алгоритмом результат - это поможет откорректировать размер вектора результатов.
     */
    std::vector<std::string_view> matched_words(query.plus_words.size());
    //проверить сначала стоп-слова с помощью std::any_of
    if (std::any_of(par, query.minus_words.begin(), query.minus_words.end(),
                    [&](const std::string_view& word) {
                        return word_to_document_freqs_.at(word).count(document_id);
                    })) {
        std::vector<std::string_view> out;
        return {out, documents_.at(document_id).status};
    }

    auto copy_last_it = std::copy_if(par, query.plus_words.begin(), query.plus_words.end(),
                                     matched_words.begin(),
                                     [&](const auto& word) {
                                         return word_to_document_freqs_.at(word).count(document_id);
    });
    std::sort(matched_words.begin(), copy_last_it);
    auto resize_end_it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.resize(std::distance(matched_words.begin(), --resize_end_it));
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        word = text.substr(1); // нужно ли присвоение?
    } else {
        word = text;
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(word) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    //избавиться от дубликатов плюс и минус слов
    //std::sort--std::unique--std::vector::erase.
    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto erase_start_it = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(erase_start_it, result.minus_words.end());

    std::sort(result.plus_words.begin(), result.plus_words.end());
    erase_start_it = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(erase_start_it, result.plus_words.end());
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& par,
                                             const std::string_view& text) const {
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double>& void_map{};
    if (0 == id_to_word_freqs_.count(document_id)) {
        return void_map;
    }
    return id_to_word_freqs_.at(document_id);
}

// Single thread version (implicit)
void SearchServer::RemoveDocument(int document_id) {
    return RemoveDocument(std::execution::seq, document_id);
}
// Single thread version (explicit)
void SearchServer::RemoveDocument(const std::execution::sequenced_policy& seq, int document_id) {
    if (0 == document_ids_.count(document_id)) {
        return;
    }
    const auto& words_to_freq = GetWordFrequencies(document_id);
    std::vector<const std::string_view*> document_words(words_to_freq.size());
    std::transform(words_to_freq.begin(), words_to_freq.end(),
                   document_words.begin(),
                   [&](const auto& word) { return &word.first;});

    std::for_each(document_words.begin(), document_words.end(),
                  [&](const auto word) {
                      if (word_to_document_freqs_.at(*word).count(document_id)) {
                          word_to_document_freqs_.at(*word).erase(document_id);
                      }
                  });
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
}
// Parallel thread version
void SearchServer::RemoveDocument(const std::execution::parallel_policy& par, int document_id) {
    if (0 == document_ids_.count(document_id)) {
        return;
    }
    const auto& words_to_freq = GetWordFrequencies(document_id);
    std::vector<const std::string_view*> document_words(words_to_freq.size());
    std::transform(words_to_freq.begin(), words_to_freq.end(),
                  document_words.begin(),
                  [&](const auto& word) { return &word.first;});

    std::for_each(par,
                  document_words.begin(), document_words.end(),
                  [&](const auto word) {
        if (word_to_document_freqs_.at(*word).count(document_id)) {
            word_to_document_freqs_.at(*word).erase(document_id);
        }
    });

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
}