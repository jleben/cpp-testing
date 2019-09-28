#include "testing.h"
#include "arguments/arguments.hpp"

#include <regex>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

namespace Testing {

GlobalOptions & options()
{
    static GlobalOptions opt;
    return opt;
}

bool Test_Set::run(Func f)
{
    if (!options().isolate)
        return f();

    pid_t pid = fork();

    if (pid == 0)
    {
        bool ok = f();
        exit(ok ? 0 : 1);
    }
    else
    {
        int status;
        if (waitpid(pid, &status, 0) <= 0)
        {
            cerr << "ERROR: Failed to wait for the child process." << endl;
            return false;
        }

        if (!WIFEXITED(status))
            return false;

        int exit_status = WEXITSTATUS(status);
        return exit_status == 0;
    }
}

bool Test_Set::run(const Options & options)
{
    string filter_pattern;

    for (auto & pattern : options.filter_regex)
    {
        if (pattern.empty())
            continue;
        if (!filter_pattern.empty())
            filter_pattern += '|';
        filter_pattern += '(';
        filter_pattern += pattern;
        filter_pattern += ')';
    }

    regex filter(filter_pattern);

    vector<string> failed_tests;
    int total_test_count = 0;

    using namespace std;
    for (auto & test : d_tests)
    {
        if (!filter_pattern.empty() && !regex_search(test.first, filter))
        {
            continue;
        }

        ++total_test_count;

        cerr << endl << "-- " << test.first << endl;
        bool ok = run(test.second);
        cerr << "-- " << (ok ? "PASSED" : "FAILED") << endl;

        if (!ok)
            failed_tests.push_back(test.first);
    }

    if (failed_tests.empty())
    {
        cerr << endl << "All tests passed " << "(" << total_test_count << ")." << endl;
    }
    else
    {
        cerr << endl << "The following tests failed: " << endl;
        for (auto & name : failed_tests)
        {
            cerr << name << endl;
        }
        cerr << "(" << failed_tests.size() << " / " << total_test_count << ")" << endl;
    }

    return failed_tests.empty();
}

int run(Test_Set & tests, int argc, char * argv[])
{
    Test_Set::Options options;

    Arguments::Parser args;
    args.add_switch("-v", Testing::options().verbose);
    args.add_switch("-l", Testing::options().list);
    args.add_switch("--no-isolate", Testing::options().isolate, false);
    args.remaining_arguments(options.filter_regex);
    args.parse(argc, argv);

    if (Testing::options().list)
    {
        for (auto & test : tests.tests())
        {
            cerr << test.first << endl;
        }
        return 0;
    }

    return tests.run(options) ? 0 : 1;
}

}
