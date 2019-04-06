#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#if defined(WIN32) || defined(_WIN32)
#include <xlocale>
#else
#include <xlocale.h>
#endif

#include <iostream>
#include <fstream>
#include <cpprest/astreambuf.h>

#include "cpprest/containerstream.h"
#include "cpprest/filestream.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/producerconsumerstream.h"
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <iomanip>

#include "pugixml.hpp"

using namespace ::pplx;
using namespace utility;
using namespace concurrency::streams;
using namespace std;
using namespace chrono;

using namespace web::http;
using namespace web::http::client;
using namespace web::json;

namespace utils {
    chrono::system_clock::time_point convert_time_str(string time);
    string ref_time_str();

    string get_distant_future_time();

    system_clock::time_point get_time_point_utc(string latest_time);

    string get_time_utc(chrono::system_clock::time_point time, string time_zone_diff = "0");
}

#endif