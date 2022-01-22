/*��������� ������� ���������. �������� ������� ������-2  with correction*/
#include <cassert>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double COMPARE_DOUBLE = 1e-6;
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
        }
        else {
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
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
/* ���������� ���� ���������� ������ SearchServer ���� */
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return  FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) {
            return status == DocumentStatus::ACTUAL;
            });
    }


    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicat) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicat);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < COMPARE_DOUBLE) {
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

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }

        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>

    vector<Document> FindAllDocuments(const Query& query, Predicate predicat) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (predicat(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};
/*
   ���������� ���� ���� ���������� ��������
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT � RUN_TEST
*/
template<typename Data_k, typename Data_v>
void Print(ostream& out, const map<Data_k, Data_v>& data) {
    bool is_first = true;
    for (const auto& [k, v] : data) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << k;
        out << ": "s;
        out << v;
    }
}
template<typename Data>
void Print(ostream& out, const Data& data) {
    bool is_first = true;
    for (const auto& element : data) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}

template <typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}
template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}
template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

void AssertImpl(bool value, const string value_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (value != true) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << value_str << ") failed. "s;;
        if (!hint.empty()) {
            cout << "Hint: " << hint;
        }
        cout << endl;
        abort();
    }

}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr , __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <class TestFunc>
void RunTestImpl(TestFunc func_test, const string& func) {
    func_test();
    cerr << func << " OK"s << endl;

}

#define RUN_TEST(func) RunTestImpl((func), #func)
// -------- ������ ��������� ������ ��������� ������� ----------

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents."s);
    }
}

/*
���������� ��� ��������� ������ �����
*/
/*��������� ���������� ����������*/
void TestAddDocumentContent() {

    /*���������, ��� ����� server �� �������� ����������*/
    SearchServer server;
    ASSERT(server.GetDocumentCount() == 0);
    /*������ ���������  ��������� ���������� � ���������, ��� ���-���� ���������*/
    server.AddDocument(0, "cat in the car"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "dog in the car"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "bat in the car"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    ASSERT(server.GetDocumentCount() == 3);
    /*����������, ��� ��������s ������ ��� ���������*/
    const auto found_docs = server.FindTopDocuments("car"s);
    for (int i = 0; i < found_docs.size(); ++i) {
        ASSERT_EQUAL(found_docs[i].id, i);
    }
}
// ������������������ �����-����. ����������, ��� ���������, ���������� ����� ����� ���������� �������, �� ��������� � ���������� ������.
void TestExcludeMinusWordsFromFindQueryContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    /*������� ����������, ��� ����� �����, �� ����������� ����� ������  ������� ������ ��������*/
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("��� city"s);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id);
    /*����� ����������, ��� ��� �� �����,����������� ��� �����-�����, ����������� �� ����������� ������*/
    ASSERT_HINT(server.FindTopDocuments("cat -cat"s).empty(), "Minus words must exclude the document."s);

}
/*��������� ������� ����������*/
void TestMatchigDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    /*������� ����������, ��� ��� �������� ��������� �� ���������� �������
    ������������ ��� ����� ���������� �������, �������������� � ���������*/
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto [m_w, status] = server.MatchDocument("cat city"s, 42);
    vector<string> q_w = { "cat"s, "city"s };
    ASSERT_EQUAL(m_w, q_w);
    /*��������� ���� ���� ����� �����, ������������ ������ ������ ����*/
    const auto [m_words, status_1] = server.MatchDocument("cat -cat"s, 42);
    ASSERT_HINT(m_words.empty(), "Minus words must exclude the document."s);
}

/*��������� ���������� ���������� �� �������������*/
void  TestRelevanceSortContent() {
    SearchServer server;
    server.AddDocument(1, "cat in the car"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat on the boat"s, DocumentStatus::ACTUAL, { 1,-2,4 });
    server.AddDocument(3, "dog whith blue eyes"s, DocumentStatus::ACTUAL, { 4,3,-1 });
    server.AddDocument(4, "big dog whith the small cat"s, DocumentStatus::ACTUAL, { -1,4,7 });
    server.AddDocument(5, "small cat whith the dog of boat"s, DocumentStatus::ACTUAL, { 3,4,5 });
    server.SetStopWords("in the on of whith"s);
    vector <Document> test_find = server.FindTopDocuments("cat whith the blue boat"s);
    vector<double> relevance_test;
    for (auto [id, relevance, rating] : test_find) {
        relevance_test.push_back(relevance);
    }
    int x = relevance_test.size();
    for (int i = 0; i < x - 1; ++i) {
        ASSERT_HINT(relevance_test[i] > relevance_test[i + 1], "Incorrect sorting. Relevance should decrease."s);
    }
}
/*��������� ����������� ���������� �������� ��������*/
void TestRatingCount() {
    SearchServer server;
    server.AddDocument(0, "cat in the car"s, DocumentStatus::ACTUAL, { 1,2,3,4,5 });
    server.AddDocument(1, "dog in the car"s, DocumentStatus::ACTUAL, { -1,-2,-3,-4,-5 });
    server.AddDocument(2, "bat in the car"s, DocumentStatus::ACTUAL, { 1,2,-3,-4,5 });
    vector<int> test_av_ratings;
    test_av_ratings.push_back((1 + 2 + 3 + 4 + 5) / 5);
    test_av_ratings.push_back((-1 - 2 - 3 - 4 - 5) / 5);
    test_av_ratings.push_back((1 + 2 - 3 - 4 + 5) / 5);
    {
        vector<Document> test_find = server.FindTopDocuments("cat"s);
        ASSERT_HINT(test_av_ratings[0] == test_find[0].rating, "The average rating was calculated incorrectly."s);
    }
    {
        vector<Document> test_find = server.FindTopDocuments("dog"s);
        ASSERT_HINT(test_av_ratings[1] == test_find[0].rating, "The average rating was calculated incorrectly."s);
    }
    {
        vector<Document> test_find = server.FindTopDocuments("bat"s);
        ASSERT_HINT(test_av_ratings[2] == test_find[0].rating, "The average rating was calculated incorrectly."s);
    }


}




/*��������� ���������� ����������� ������ � �������������� ���������*/
void TestPredicateFindContent() {
    SearchServer server;
    server.AddDocument(0, "cat in the car"s, DocumentStatus::ACTUAL, { 1,2,3,4,5 });
    server.AddDocument(1, "cat on the boat"s, DocumentStatus::BANNED, { 6,7,8 });
    server.AddDocument(2, "cat whith blue eyes"s, DocumentStatus::IRRELEVANT, { 9,10,11 });
    server.AddDocument(3, "big cat whith small bells"s, DocumentStatus::REMOVED, { 12,13,14 });
    server.AddDocument(4, "small cat whith dog in boat"s, DocumentStatus::ACTUAL, { 15,16,17 });
    auto test_vec = server.FindTopDocuments("cat in the car", [](int document_id, DocumentStatus status, int rating) {return rating == 7; });
    vector<int> test_id;
    for (auto [id, relevance, rating] : test_vec) {
        test_id.push_back(id);
    }
    ASSERT_HINT(test_id[0] == 1, "Filtering using a predicate is implemented incorrectly."s);
}

/* ��������� ����� ���������� ������� �������� ������*/
void TestFindStatusContent() {
    SearchServer server;
    server.AddDocument(0, "cat in the car"s, DocumentStatus::ACTUAL, { 1,2,3,4,5 });
    server.AddDocument(1, "cat on the boat"s, DocumentStatus::BANNED, { 6,7,8 });
    server.AddDocument(2, "cat whith blue eyes"s, DocumentStatus::IRRELEVANT, { 9,10,11 });
    server.AddDocument(3, "big cat whith small bells"s, DocumentStatus::REMOVED, { 12,13,14 });
    vector<DocumentStatus> test_status = { DocumentStatus::ACTUAL, DocumentStatus::BANNED, DocumentStatus::IRRELEVANT, DocumentStatus::REMOVED };
    int i = 0;
    for (auto status : test_status) {
        auto [id, relevance, rating] = server.FindTopDocuments("cat"s, status)[0];
        ASSERT_HINT(id == i, "Status search is implemented incorrectly."s);
        ++i;
    }
}

/*��������� ������������ ���������� �������������*/
void TestRelevanceCount() {
    SearchServer server;
    server.AddDocument(0, "cat in the car"s, DocumentStatus::ACTUAL, { 1,2,3,4,5 });
    server.AddDocument(1, "cat on the big green boat"s, DocumentStatus::ACTUAL, { 6,7,8 });
    server.AddDocument(2, "little cat on the big green tree"s, DocumentStatus::ACTUAL, { 6,7,8 });
    double test_rel = 0.0;
    {
        double x = 4.0;
        double y = 1.0;
        test_rel = log(3.0) * (y / x);
        auto [id, relevance, rating] = server.FindTopDocuments("car"s)[0];
        ASSERT_HINT((abs(relevance - test_rel) < COMPARE_DOUBLE), "Relevance is calculated incorrectly."s);
    }
    {
        double x = 6.0;
        double y = 1.0;
        test_rel = log(3.0) * (y / x);
        auto [id, relevance, rating] = server.FindTopDocuments("boat"s)[0];
        ASSERT_HINT((abs(relevance - test_rel) < COMPARE_DOUBLE), "Relevance is calculated incorrectly."s);
    }
    {
        double x = 7.0;
        double y = 1.0;
        test_rel = (log(3.0 / 2) * (y / x)) + (log(3.0 / 1) * (y / x));
        auto [id, relevance, rating] = server.FindTopDocuments("big tree"s)[0];
        ASSERT_HINT((abs(relevance - test_rel) < COMPARE_DOUBLE), "Relevance is calculated incorrectly."s);
    }

}
// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // �� �������� �������� ��������� ����� �����
    RUN_TEST(TestAddDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromFindQueryContent);
    RUN_TEST(TestMatchigDocumentContent);
    RUN_TEST(TestRatingCount);
    RUN_TEST(TestPredicateFindContent);
    RUN_TEST(TestFindStatusContent);
    RUN_TEST(TestRelevanceCount);
}

// --------- ��������� ��������� ������ ��������� ������� -----------

int main() {
    setlocale(LC_ALL, "RUS");
    TestSearchServer();
    // ���� �� ������ ��� ������, ������ ��� ����� ������ �������
    cout << "Search server testing finished"s << endl;
}