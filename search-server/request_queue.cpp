//Final project of 5-th split. Cherepukhin Evgeny
#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    :search_server_(search_server) {}

RequestQueue::FindResult RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {

    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

RequestQueue::FindResult RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}

void RequestQueue::AddRequest(int count_results) {
    ++current_time_;
    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (requests_.front().results == 0) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }
    requests_.push_back(QueryResult{ current_time_, count_results });
    if (count_results == 0) {
        ++no_result_requests_;
    }
}