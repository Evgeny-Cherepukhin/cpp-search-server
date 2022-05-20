//Черепухин Евгений Сергеевич. Итоговыйй проект 8 сплит.

#include "search_server.h"


using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    if (documents_.count(document_id) > 0) {

        throw invalid_argument("The document with this document_id already exists"s);

    }
    vector<string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    map<string, double, less<>> word_to_freqs;
    for (const std::string_view word : words) {        
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        word_to_freqs[string(word)] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, string(document), word_to_freqs });
    document_ids_.insert(document_id);    
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

SearchServer::ConstIterator SearchServer::begin() const {
    return document_ids_.cbegin();
}

SearchServer::ConstIterator SearchServer::end() const {
    return document_ids_.cend();
}

const SearchServer::MapWordFreqs& SearchServer::GetWordFrequencies(int document_id) const {
    static const MapWordFreqs EMPTY_MAP;
    if (document_ids_.count(document_id) > 0) {
        return documents_.at(document_id).word_to_freqs;
    }
    return EMPTY_MAP;
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    VectorStringView words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::QueryPar SearchServer::ParseQueryPar(const string_view text) const {
    QueryPar result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) { 
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }    
    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto last = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last, result.minus_words.end());
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto last_p = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last_p, result.plus_words.end());
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}

SearchServer::FindResult SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

SearchServer::FindResult SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("This document id doesn't exist");
    }
    VectorStringView matched_words;
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            //matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(std::execution::parallel_policy, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQueryPar(raw_query);
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("This document id doesn't exist");
    }
    std::vector<std::string_view> matched_words;
    matched_words.resize(500);
    if (std::any_of(execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](const std::string_view word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return false;
            }
            if (word_to_document_freqs_.at(string(word)).count(document_id)) {
                return true;
            }
            return false;
        })) {
        matched_words.clear();
        return { matched_words, documents_.at(document_id).status };
    }

    transform(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [this, document_id](const std::string_view word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return std::string_view{};
            }
            if (word_to_document_freqs_.at(string(word)).count(document_id)) {
                return word;
            }
            return std::string_view{};
        });
    std::set<std::string_view> set_of_matched_words(matched_words.begin(), matched_words.end());
    set_of_matched_words.erase(std::string_view{});
    std::vector<std::string_view> result(set_of_matched_words.begin(), set_of_matched_words.end());
    return { result, documents_.at(document_id).status };
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(execution::seq, document_id);
}
