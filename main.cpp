// Варіант 14. C++20, компілятор: GCC 15.2 (MSYS2 UCRT64).
// Формат data/sequence_<N>.txt: перший рядок - N, далі N цілих з [-1000000; 1000000], по одному в рядку.


#include "variant14.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef OPT_NAME
#define OPT_NAME "unknown"
#endif
#ifndef HAS_TBB
#define HAS_TBB 0
#endif

using namespace std;

string fileName(size_t length) {
    return "data/sequence_" + to_string(length) + ".txt";
}

void generateFile(size_t length) {
    if (filesystem::exists(fileName(length))) return;
    filesystem::create_directories("data");

    mt19937_64 generator(20261009 + length);
    uniform_int_distribution<Value> distribution(-1'000'000, 1'000'000);
    ofstream file(fileName(length));
    file << length << '\n';
    for (size_t i = 0; i < length; ++i) file << distribution(generator) << '\n';
}

vector<Value> readFile(size_t length) {
    ifstream file(fileName(length));
    size_t n;
    file >> n;
    vector<Value> values(n);
    for (Value& x : values) file >> x;
    return values;
}

template <class Task>
double measure(Task task, long long expected, int repetitions) {
    volatile long long sink = 0;   
    if (task() != expected) throw runtime_error("Results do not match");
    double total = 0;
    for (int i = 0; i < repetitions; ++i) {
        auto start = chrono::steady_clock::now();
        sink = task();
        auto finish = chrono::steady_clock::now();
        total += chrono::duration<double, milli>(finish - start).count();
    }
    (void)sink;
    return total / repetitions;
}


void say(ofstream& file, const string& text) {
    cout << text << '\n';
    file << text << '\n';
}

void printRow(ofstream& file, const string& mode, size_t k, double ms, double speedup) {
    ostringstream line;
    line << fixed << setprecision(4) << left << setw(14) << mode << setw(6);
    if (k == 0) line << "-"; else line << k;
    line << setw(14) << ms << speedup;
    say(file, line.str());
}

int main(int argc, char* argv[]) {
    const string tag = OPT_NAME;
    const int repetitions = argc > 1 ? stoi(argv[1]) : 20;
    const unsigned processors = max(1u, thread::hardware_concurrency());
    const vector<size_t> lengths = {20'000, 100'000, 500'000, 2'000'000};
    const set<size_t> kValues = {1, 2, 4, 8, 16, 32, processors};

    ofstream file("results_" + tag + ".txt");
    say(file, "Variant 14: average |a[i+1] - a[i]| (adjacent_difference)");
    say(file, "Optimization: " + tag + "; repetitions: " + to_string(repetitions) +
              "; logical processors: " + to_string(processors));
    say(file, string("TBB linked: ") + (HAS_TBB ? "yes" : "no"));

    for (size_t length : lengths) {
        generateFile(length);
        const vector<Value> values = readFile(length);
        const long long expected = runNoPolicy(values);

        ostringstream title;
        title << fixed << setprecision(4) << "\nLength " << length << ", average = "
              << static_cast<double>(expected) / (length - 1);
        say(file, title.str());
        say(file, "Mode          K     Time, ms      Speedup vs no_policy");

        const double base = measure([&] { return runNoPolicy(values); }, expected, repetitions);
        printRow(file, "no_policy", 0, base, 1.0);
        double t = measure([&] { return runSeq(values); }, expected, repetitions);
        printRow(file, "seq", 0, t, base / t);
        t = measure([&] { return runPar(values); }, expected, repetitions);
        printRow(file, "par", 0, t, base / t);
        t = measure([&] { return runParUnseq(values); }, expected, repetitions);
        printRow(file, "par_unseq", 0, t, base / t);

        say(file, "\nOwn parallel algorithm (Speedup is vs K=1):");
        say(file, "Mode          K     Time, ms      Speedup");
        double timeK1 = 0, bestTime = 1e18;
        size_t bestK = 1;
        for (size_t k : kValues) {
            const double time = measure([&] { return runOwn(values, k); }, expected, repetitions);
            if (k == 1) timeK1 = time;
            if (time < bestTime) { bestTime = time; bestK = k; }
            printRow(file, "own", k, time, timeK1 / time);
        }

        ostringstream best;
        best << fixed << setprecision(4) << "Best K = " << bestK
             << "; K / processors = " << static_cast<double>(bestK) / processors;
        say(file, best.str());
    }
    return 0;
}