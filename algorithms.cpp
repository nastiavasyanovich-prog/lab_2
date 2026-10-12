#include "variant14.h"

#include <algorithm>
#include <cstdlib>
#include <execution>
#include <numeric>
#include <thread>

using namespace std;

namespace {

long long sumAbsDiff(vector<Value>::const_iterator first,
                     vector<Value>::const_iterator last) {
    if (last - first < 2) return 0;
    vector<Value> d(last - first);
    adjacent_difference(first, last, d.begin());   
    transform(d.begin() + 1, d.end(), d.begin() + 1,
              [](Value x) { return llabs(x); });
    return reduce(d.begin() + 1, d.end(), 0LL);
}

template <class Policy>
long long sumAbsDiff(Policy policy, const vector<Value>& v) {
    if (v.size() < 2) return 0;
    vector<Value> d(v.size());
    adjacent_difference(policy, v.begin(), v.end(), d.begin());
    transform(policy, d.begin() + 1, d.end(), d.begin() + 1,
              [](Value x) { return llabs(x); });
    return reduce(policy, d.begin() + 1, d.end(), 0LL);
}

} 

long long runNoPolicy(const vector<Value>& v) { return sumAbsDiff(v.begin(), v.end()); }
long long runSeq(const vector<Value>& v) { return sumAbsDiff(execution::seq, v); }
long long runPar(const vector<Value>& v) { return sumAbsDiff(execution::par, v); }
long long runParUnseq(const vector<Value>& v) { return sumAbsDiff(execution::par_unseq, v); }

long long runOwn(const vector<Value>& v, size_t k) {
    if (v.size() < 2) return 0;
    const size_t n = v.size() - 1;          
    if (k == 0) k = 1;
    if (k > n) k = n;

    vector<long long> sums(k);
    vector<thread> threads;
    for (size_t i = 0; i < k; ++i) {
        const size_t from = n * i / k;       
        const size_t to = n * (i + 1) / k;   
        threads.emplace_back([&v, &sums, i, from, to]() {
            sums[i] = sumAbsDiff(v.begin() + from, v.begin() + to + 1);
        });
    }
    for (auto& t : threads) t.join();

    return accumulate(sums.begin(), sums.end(), 0LL);   
}