#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <functional>
#include <vector>
#include <utility>
#include <initializer_list>
#include <atomic>

namespace Testing {

using std::string;
using std::function;
using std::vector;
using std::pair;
using std::atomic;

struct GlobalOptions
{
    GlobalOptions() {}
    bool verbose = false;
};

GlobalOptions & options();

inline double time_since(const std::chrono::steady_clock::time_point & start)
{
    using namespace std;
    using namespace std::chrono;

    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return s;
}

inline
bool assert(const string & message, bool value)
{
    if (!value || options().verbose)
        printf("%s: %s\n", (value ? "OK" : "ERROR"), message.c_str());
}

class Test
{
public:
    void assert(const string & message, bool value)
    {
        Testing::assert(message, value);

        if (!value)
            d_success = false;
    }

    void assert_critical(const string & message, bool value)
    {
        Testing::assert(message, value);

        if (!value)
            d_success = false;

        throw std::runtime_error("Critical assertion failed: " + message);
    }

    bool success() const { return d_success; }

private:
    atomic<bool> d_success { true };
};

class Test_Set
{
public:
    struct Options
    {
        vector<string> filter_regex;
    };

    using Func = function<bool()>;

    Test_Set() {}

    // Combine test sets.
    // The argument is a pair a set and a name for the set.
    // The new set contains each test from each set,
    // with the name '<set name>.<test-name>'.

    Test_Set(std::initializer_list<pair<string,Test_Set>> test_sets)
    {
        for(auto & entry : test_sets)
        {
            const Test_Set & set = entry.second;
            for (auto & test : set.tests())
            {
                auto name = entry.first + '.' + test.first;
                d_tests.emplace_back(name, test.second);
            }
        }
    }

    Test_Set(std::initializer_list<pair<string,Func>> tests):
        d_tests(tests)
    {}

    bool run(const Options & options = Options());

    const vector<pair<string,Func>> & tests() const
    {
        return d_tests;
    }

private:
    vector<pair<string,Func>> d_tests;
};

int run(Test_Set &, int argc, char * argv[]);

}
