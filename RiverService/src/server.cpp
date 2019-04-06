#if defined(WIN32) || defined(_WIN32)
#include <xlocale>
#else
#include <xlocale.h>
#endif
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <fstream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#include <iostream>
#include <map>
#include <set>
#include <string>
#include "server.h"
#include <thread>

using namespace std;

#define TRACE(msg)            wcout << msg
#define TRACE_ACTION(a, k, v) ucout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;

void display_json(
    json::value const & jvalue,
    utility::string_t const & prefix)
{
    //wcout << prefix << jvalue.serialize() << endl;
}

const std::function<void(http_request)> handle_get_wrapped(health_tracker &health) {
    return ([&health](http_request request) {
        TRACE(L"\nhandle GET\n");

        auto answer = json::value::object();
        string_t status = utility::conversions::to_string_t(health.get_status());

        answer[U("status")] = json::value::string(status);
        chrono::duration<double> up_time_duration = health.get_uptime();
        auto up_time = std::chrono::duration_cast<std::chrono::seconds>(up_time_duration).count();

        answer[U("up_time_seconds")] = json::value(up_time);

        display_json(json::value::null(), U("R: "));
        display_json(answer, U("S: "));

        request.reply(status_codes::OK, answer);
    });
}

void handle_get(http_request request)
{
    TRACE(L"\nhandle GET\n");

    auto answer = json::value::object();

    for (auto const & p : dictionary)
    {
        answer[p.first] = json::value::string(p.second);
    }

    display_json(json::value::null(), U("R: "));
    display_json(answer, U("S: "));

    request.reply(status_codes::OK, answer);
}

void handle_request(
    http_request request,
    function<void(json::value const &, json::value &)> action)
{
    auto answer = json::value::object();

    request
        .extract_json()
        .then([&answer, &action](pplx::task<json::value> task) {
        try
        {
            auto const & jvalue = task.get();
            //display_json(jvalue, L"R: ");

            if (!jvalue.is_null())
            {
                action(jvalue, answer);
            }
        }
        catch (http_exception const & e)
        {
            //wcout << e.what() << endl;
        }
    })
        .wait();


    //display_json(answer, L"S: ");

    request.reply(status_codes::OK, answer);
}

json::value get_available_features(data_store &data) {
    map<utility::string_t, feature_of_interest> feature_map = data.feature_map;
    web::json::value response;
    std::vector<web::json::value> features;
    for (auto const & item : feature_map) {
        feature_of_interest feature = item.second;
        web::json::value feature_item;
        feature_item[U("id")] = json::value(feature.get_id());
        feature_item[U("name")] = json::value::string(feature.get_name());
        feature_item[U("latest_flow")] = json::value(feature.get_latest_flow());
        features.push_back(feature_item);
    }
    response[U("features")] = web::json::value::array(features);

    string last_update_time = data.get_last_updated_time_str();
    string_t last_update_time_t = utility::conversions::to_string_t(last_update_time);
    response[U("last_update_time")] = json::value(last_update_time_t);

    return response;
}



json::value get_flow_response(data_store &data, json::value ids) {
    json::value answer;
    for (auto const & e : ids.as_array())
    {
        if (e.is_string())
        {
            auto key = e.as_string();
            auto pos = data.feature_map.find(key);

            if (pos == data.feature_map.end())
            {
                answer = get_available_features(data);
            }
            else
            {
                feature_of_interest feature = pos->second;
                vector<sensor_obs> flow_history = feature.obs_store.get_as_vector();

                answer[U("id")] = json::value::string(feature.get_id());
                answer[U("name")] = json::value::string(feature.get_name());
                answer[U("last_updated")] = json::value::string(feature.get_last_checked_time());

                std::vector<web::json::value> flowOut;
                for (unsigned int i = 0; i < flow_history.size(); i++) {
                    sensor_obs item = flow_history[i];
                    web::json::value vehicle;
                    vehicle[U("flow")] = json::value(item.get_flow());
                    vehicle[U("time")] = json::value::string(item.get_time());
                    flowOut.push_back(vehicle);
                }
                answer[U("flows")] = web::json::value::array(flowOut);
            }
        }
    }
    return answer;
}

const std::function<void(http_request)> handle_post_wrapped(data_store &data) {
    return ([&data](http_request request) {
        TRACE("\nhandle POST\n");

        handle_request(
            request,
            [&data](json::value const & jvalue, json::value & answer) {
            json::value action = jvalue.at(U("action"));
            string_t action_str = action.as_string();

            if (action_str == utility::conversions::to_string_t("get_flows")) {
                json::value ids = jvalue.at(U("id"));
                answer = get_flow_response(data, ids);
            }

            if (action_str == utility::conversions::to_string_t("get_features")) {
                answer = get_available_features(data);
            }
        });
    });
}


void server_session::create_session(data_store &data, utility::string_t port, health_tracker &health) {
#if defined(WIN32) || defined(_WIN32)
    string host = "http://localhost:";
#else
    string host = "http://0.0.0.0:";
#endif
    utility::string_t address = utility::conversions::to_string_t(host) + port;

    http_listener listener(address);
    wcout << "listening on " << port.c_str() << endl;
    listener.support(methods::GET, handle_get_wrapped(health));
    listener.support(methods::POST, handle_post_wrapped(data));

    try
    {
        listener
            .open()
            .then([&listener]() {TRACE(L"\nstarting to listen\n"); })
            .wait();
        data.get_available_features();
        while (true) {
            // return a value that is the time until the next update is required
            // for use in the sleep function
            data.update_sources();
            std::this_thread::sleep_for(std::chrono::minutes(30));
        }
    }
    catch (exception const & e)
    {
        //wcout << e.what() << endl;
    }

}