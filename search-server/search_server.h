//Черепухин Евгений Сергеевич. Итоговый проект 8 спринт.
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <execution>
#include <iterator>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <vector>


#include "concurrent_map.h"
#include "document.h"
#include "string_processing.h"



const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double PRECISION = 1e-6;

class SearchServer {
public:

    using FindResult = std::vector<Document>;
    using MatchDocumentResult = std::tuple<std::vector<std::string_view>, DocumentStatus>;
    using ConstIterator = std::set<int>::const_iterator;
    using MapWordFreqs = std::map<std::string, double, std::less<>>;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    FindResult FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    FindResult FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    FindResult FindTopDocuments(const std::string_view raw_query) const;

    template <typename DocumentPredicate, typename Policy>
    FindResult FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename Policy>
    FindResult FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename Policy>
    FindResult FindTopDocuments(Policy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    ConstIterator begin() const;

    ConstIterator end() const;

    const MapWordFreqs& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    template <typename Policy>
    void RemoveDocument(Policy& policy, int document_id);

    MatchDocumentResult MatchDocument(const std::string_view raw_query, int document_id) const;

    MatchDocumentResult MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query, int document_id) const;

    MatchDocumentResult MatchDocument(std::execution::parallel_policy, const std::string_view raw_query, int document_id) const;


private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string data;
        MapWordFreqs word_to_freqs;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    struct QueryPar
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    const SetString stop_words_;
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(const std::string_view text) const;

    Query ParseQuery(const std::string_view text) const;

    QueryPar ParseQueryPar(const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    FindResult FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate, typename Policy>
    FindResult FindAllDocumentsPar(Policy& policy, const QueryPar& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
SearchServer::FindResult SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {

    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < PRECISION) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate, typename Policy>
SearchServer::FindResult SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQueryPar(raw_query);
    auto matched_documents = FindAllDocumentsPar(policy, query, document_predicate);
    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            return (std::abs(lhs.relevance - rhs.relevance) < 1e-6 && lhs.rating > rhs.rating)
                || (lhs.relevance > rhs.relevance);
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename Policy>
SearchServer::FindResult SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int, DocumentStatus document_status, int) {
        return document_status == status;
        });
}

template <typename Policy>
SearchServer::FindResult SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename Policy>
void SearchServer::RemoveDocument(Policy& policy, int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }
    std::for_each(policy,
        documents_.at(document_id).word_to_freqs.begin(),
        documents_.at(document_id).word_to_freqs.end(),
        [this, document_id](const auto& word) {
            word_to_document_freqs_[word.first].erase(document_id);
        });
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

template <typename DocumentPredicate>
SearchServer::FindResult SearchServer::FindAllDocuments(const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {                
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename Policy>
SearchServer::FindResult SearchServer::FindAllDocumentsPar(Policy& policy, const SearchServer::QueryPar& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance;
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this, &document_to_relevance, &document_predicate, &policy](const std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) > 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            std::for_each(policy, word_to_document_freqs_.at(std::string(word)).begin(), word_to_document_freqs_.at(std::string(word)).end(), [this, &document_to_relevance, &document_predicate, &inverse_document_freq](const auto& pair) {
                const auto& document_data = documents_.at(pair.first);
                if (document_predicate(pair.first, document_data.status, document_data.rating)) {
                    document_to_relevance[pair.first].ref_to_value += pair.second * inverse_document_freq;
                }
            });
        }
    });
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance, &policy](const std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) > 0) {
            std::for_each(policy, word_to_document_freqs_.at(std::string(word)).begin(), word_to_document_freqs_.at(std::string(word)).end(), [&document_to_relevance](const auto& pair) {
                document_to_relevance.Erase(pair.first);
            });
        }
    });
    auto result = document_to_relevance.BuildOrdinaryMap();
    FindResult matched_documents(result.size());
    std::atomic_int index = 0;
    std::for_each(policy, result.begin(), result.end(), [this, &matched_documents, &index](const auto& pair) {
        matched_documents[index++] = Document(pair.first, pair.second, documents_.at(pair.first).rating);
    });
    return matched_documents;
}