#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    PushToResultDeque(raw_query, result);
    return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    PushToResultDeque(raw_query, result);
    return result;
}
int RequestQueue::GetNoResultRequests() const {
    int out = 0;
    for (auto i : requests_) {
        if (i.result.empty()) {
            ++out;
        }
    }
    return out;
}

void RequestQueue::PushToResultDeque(const std::string& raw_query, const std::vector<Document>& result) {
    if (requests_.size() >= min_in_day_) {
        requests_.pop_front();
    }
    requests_.push_back({raw_query, result});
}
