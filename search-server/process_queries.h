#pragma once

#include <vector>
#include <deque>
#include <algorithm>
#include <execution>
#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);

std::deque<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);