#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
                                                  const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> out(queries.size());
    std::transform(std::execution::par,
                   queries.begin(), queries.end(),
                   out.begin(),
                   [&search_server](const std::string& query) { return search_server.FindTopDocuments(query); });
    return out;
}

std::deque<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::deque<Document> out;
    std::vector<std::vector<Document>> queries_result = ProcessQueries(search_server, queries);
    for (const auto& query_docs : queries_result) {
        out.insert(out.end(), query_docs.begin(), query_docs.end());
    }
    return out;
}