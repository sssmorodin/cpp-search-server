#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

////////////////////////////

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
		double weight = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += weight;
		}
		++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    void Print() {
        for (auto& [key, nums] : word_to_document_freqs_) {
		cout << key << ' ';
		    for (const auto& num : nums) {
			    cout << num.first << ' ' << num.second << ' ';
		    }
            cout << endl;
		}
    }


private:

    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;
	
    int document_count_ = 0;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
      

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
		
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
		
		
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                string minus_word = word.substr(1);
                query_words.minus_words.insert(minus_word);
            } else {
                query_words.plus_words.insert(word);
            }
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
		map<int, double> document_to_relevance;
        vector<int> documents_to_delete;
		
        for (const string& word : query_words.plus_words) {
			double idf = log(1.0 * document_count_ / word_to_document_freqs_.at(word).size())
			if (word_to_document_freqs_.count(word)) {
				for (const auto& [id, freq] : word_to_document_freqs_.at(word)) {
					document_to_relevance[id] += freq * idf;	
				}
			}
		}
		
		
		for (const string& word : query_words.minus_words) {
			if (word_to_document_freqs_.count(word)) {
				for (const auto& [id, freq] : word_to_document_freqs_.at(word)) {
					documents_to_delete.push_back(id);
				}
			}
		}
				
		for (const int& id : documents_to_delete) {
			document_to_relevance.erase(id);
		}			    
		
		for (const auto& [id, relevance] : document_to_relevance) {
			matched_documents.push_back({id, relevance});
		}
        
        return matched_documents;
    }

};

////////////////////////////////////////////

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
   
    return search_server;
}



int main() {
    SearchServer search_server = CreateSearchServer();
    
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}