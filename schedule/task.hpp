#include <string>
using namespace std;
#include <chrono>
#include <functional>

class Task {
    public:
        bool is_due(chrono::steady_clock::time_point now) const; 
        bool execute(chrono::steady_clock::time_point now); 

    private:
        string name;
        int missed_deadlines;
        int execution_count;
        chrono::microseconds period;
        chrono::steady_clock::time_point last_run_time;
        chrono::steady_clock::time_point next_run_time;
        function<void()> callback;
};
