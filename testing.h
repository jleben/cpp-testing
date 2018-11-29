#pragma once

#include <string>
#include <sstream>
#include <iostream>
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
    bool list = false;
    bool verbose = false;
    bool isolate = true;
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
    return value;
}

inline
void assert_critical(const string & message, bool value)
{
    if (!assert(message, value))
        throw std::runtime_error("Critical assertion failed: " + message);
}

class Test;

class Assertion : public std::ostringstream
{
    Test & d_test;
    bool d_value = false;
public:
    Assertion(const Assertion & other) = delete;
    Assertion(Assertion && other):
        d_test(other.d_test),
        d_value(other.d_value)
    {}
    Assertion(Test & test, bool value): d_test(test), d_value(value) {}
    ~Assertion();
};

class Information : public std::ostringstream
{
public:
    ~Information()
    {
        if (options().verbose)
            printf("%s", str().c_str());
    }
};

class Test
{
public:
    Assertion assert(bool value)
    {
        return Assertion(*this, value);
    }

    void assert(const string & message, bool value)
    {
        Testing::assert(message, value);

        if (!value)
            d_success = false;
    }

    void assert_critical(const string & message, bool value)
    {
        if (!value)
            d_success = false;

        Testing::assert_critical(message, value);
    }

    bool success() const { return d_success; }

private:
    atomic<bool> d_success { true };
};

inline Assertion::~Assertion()
{
    d_test.assert(str(), d_value);
}

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

    void add_test(const string & name, Func f)
    {
        d_tests.emplace_back(name, f);
    }

    bool run(const Options & options = Options());

    const vector<pair<string,Func>> & tests() const
    {
        return d_tests;
    }

private:
    bool run(Func f);

    vector<pair<string,Func>> d_tests;
};

int run(Test_Set &, int argc, char * argv[]);

}
