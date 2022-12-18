#include "document.h"

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& output, const Document& document) {
    output << "{ "s
           << "document_id = "s << document.id << ", "s
           << "relevance = "s << document.relevance << ", "s
           << "rating = "s << document.rating << " }"s ;
    return output;
}
