#include <functional>
#include <queue>
#include <vector>
#include <iostream>

#include "data_store.h"

using namespace std;
using namespace chrono;

void data_store::get_available_features() {
    for (auto &source : _data_sources) {
        source.get_available_features(feature_map);
    }

    // create the initial priority_queue
    int count = 0;
    for (auto &entry : feature_map) {
        count++;
        if (count < 2) {
            entry.second.update();
            update_queue.push(&entry.second);
        }
    }

    cout << "queue is created, the current order is:" << endl;
    std::priority_queue<feature_of_interest*, std::vector<feature_of_interest*>, OrderUpdateQueue> update_queue_copy = update_queue;
    while (!update_queue_copy.empty()) {
        feature_of_interest* temp_feature = update_queue_copy.top();
        chrono::duration <double> time = temp_feature->next_update_time;
        cout << temp_feature->get_name_str() << ", with next update time = " << time.count() << endl;
        update_queue_copy.pop();
    }
}

void data_store::update_sources() {
    last_updated = system_clock::now();

    system_clock::time_point current_time = system_clock::now();
    // get flows
    cout << "updating flows" << endl;
    chrono::duration<double> current_time_ref = current_time - utils::convert_time_str(utils::ref_time_str());
    cout << "The current time reference is " << current_time_ref.count() << endl;

    vector<feature_of_interest*> updated_features;
    while (!update_queue.empty() && update_queue.top()->next_update_time < current_time_ref) {
        feature_of_interest* temp_feature = update_queue.top();
        cout << "updating feature with name " << temp_feature->get_name_str() << endl;
        temp_feature->update();
        update_queue.pop();
        updated_features.push_back(temp_feature);
    }

    for (auto &feature_item : updated_features) {
        update_queue.push(feature_item);
    }

    cout << "queue is updated, the new order is:" << endl;
    std::priority_queue<feature_of_interest*, std::vector<feature_of_interest*>, OrderUpdateQueue> update_queue_copy = update_queue;
    while (!update_queue_copy.empty()) {
        feature_of_interest* temp_feature = update_queue_copy.top();
        chrono::duration <double> time = temp_feature->next_update_time;
        cout << temp_feature->get_name_str() << ", with next update time = " << time.count() << endl;
        update_queue_copy.pop();
    }
}