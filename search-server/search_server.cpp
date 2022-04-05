//Final project of 5-th split. Cherepukhin Evgeny
#include "search_server.h"


using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0)) {
        throw invalid_argument("id �� ����� ���� �������������"s);
    }
    if (documents_.count(document_id) > 0) {

        throw invalid_argument("������� ���������� ��������� � ������������  id");

    }
    auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    map<std::string, double> word_to_freqs;
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_to_freqs[word] += inv_word_count;
    }    
     
    documents_.emplace(document_id,  DocumentData { ComputeAverageRating(ratings), status  });
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

//�� ���� ������, ��� ������ ������� �����.
const SearchServer::MapWordFreqs& SearchServer::GetWordFrequencies(int document_id) const {
    static MapWordFreqs EMPTY_MAP;    
    if (document_ids_.count(document_id) > 0) {
        for (const auto& [word, mapa] : word_to_document_freqs_) {
            if (mapa.count(document_id)) {
                EMPTY_MAP[word] = mapa.at(document_id);
            }
        }
    }
    return EMPTY_MAP;
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query result;
    for (const string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::FindResult SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

SearchServer::FindResult SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    
    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }
    for (const auto& [word, freq] : GetWordFrequencies(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
    }    
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}
