#pragma once

#include <cstddef>
#include <vector>

using Value = long long;

long long runNoPolicy(const std::vector<Value>& v);
long long runSeq(const std::vector<Value>& v);
long long runPar(const std::vector<Value>& v);
long long runParUnseq(const std::vector<Value>& v);
long long runOwn(const std::vector<Value>& v, std::size_t k);