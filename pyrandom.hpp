// pyrandom.hpp
//
// A header-only C++ library that mimics Python's `random` module as
// closely as reasonably possible, both in naming and behavior.
//
// Usage:
//     #include "pyrandom.hpp"
//
//     pyrandom::seed(42);                     // random.seed(42)
//     double x   = pyrandom::random();        // random.random()
//     double u   = pyrandom::uniform(1, 10);  // random.uniform(1, 10)
//     long r     = pyrandom::randint(1, 6);   // random.randint(1, 6)
//     long rr    = pyrandom::randrange(0, 100, 5); // random.randrange(0, 100, 5)
//
//     std::vector<int> v = {1, 2, 3, 4, 5};
//     int c = pyrandom::choice(v);            // random.choice(v)
//     pyrandom::shuffle(v);                   // random.shuffle(v)
//     auto s = pyrandom::sample(v, 3);        // random.sample(v, 3)
//
// Requires C++14 or later.

#ifndef PYRANDOM_HPP
#define PYRANDOM_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace pyrandom {

// ---------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------
namespace detail {

// Python auto-seeds its global Random instance from OS entropy the
// moment the module is imported. We do the same, lazily, per thread.
inline std::mt19937_64& engine() {
    thread_local std::mt19937_64 eng(std::random_device{}());
    return eng;
}

template <typename Container>
using value_type_of =
    typename std::decay<decltype(*std::begin(std::declval<Container&>()))>::type;

} // namespace detail

// ---------------------------------------------------------------------
// random.seed(a=None)
// ---------------------------------------------------------------------

// Re-seed from OS entropy (like calling random.seed() with no args).
inline void seed() {
    detail::engine().seed(std::random_device{}());
}

// Re-seed with a specific value (like random.seed(a)) for reproducible
// sequences.
inline void seed(uint64_t a) {
    detail::engine().seed(a);
}

// ---------------------------------------------------------------------
// random.random() -> float in [0.0, 1.0)
// ---------------------------------------------------------------------
inline double random() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(detail::engine());
}

// ---------------------------------------------------------------------
// random.uniform(a, b) -> float in [a, b]
// ---------------------------------------------------------------------
inline double uniform(double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(detail::engine());
}

// ---------------------------------------------------------------------
// random.randint(a, b) -> integer N such that a <= N <= b
// ---------------------------------------------------------------------
inline long long randint(long long a, long long b) {
    if (a > b) {
        throw std::invalid_argument("randint: a must be <= b");
    }
    std::uniform_int_distribution<long long> dist(a, b);
    return dist(detail::engine());
}

// ---------------------------------------------------------------------
// random.randrange(stop)
// random.randrange(start, stop, step=1)
// ---------------------------------------------------------------------
inline long long randrange(long long start, long long stop, long long step = 1) {
    if (step == 0) {
        throw std::invalid_argument("randrange: step must not be zero");
    }
    long long width = stop - start;
    long long n;
    if (step > 0) {
        if (width <= 0) throw std::invalid_argument("randrange: empty range");
        n = (width + step - 1) / step;
    } else {
        if (width >= 0) throw std::invalid_argument("randrange: empty range");
        n = (width + step + 1) / step;
    }
    long long i = randint(0, n - 1);
    return start + i * step;
}

inline long long randrange(long long stop) {
    return randrange(0, stop, 1);
}

// ---------------------------------------------------------------------
// random.choice(seq) -> a random element of seq
// ---------------------------------------------------------------------
template <typename Container>
detail::value_type_of<const Container> choice(const Container& c) {
    auto size = std::distance(std::begin(c), std::end(c));
    if (size <= 0) {
        throw std::invalid_argument("choice: sequence is empty");
    }
    auto it = std::begin(c);
    std::advance(it, randint(0, static_cast<long long>(size) - 1));
    return *it;
}

// ---------------------------------------------------------------------
// random.shuffle(x) -> shuffles x in place
// ---------------------------------------------------------------------
template <typename Container>
void shuffle(Container& c) {
    std::shuffle(std::begin(c), std::end(c), detail::engine());
}

// ---------------------------------------------------------------------
// random.sample(population, k) -> k unique elements, order preserved
//                                  from the random draw (not the input)
// ---------------------------------------------------------------------
template <typename Container>
std::vector<detail::value_type_of<const Container>>
sample(const Container& c, std::size_t k) {
    using T = detail::value_type_of<const Container>;
    std::vector<T> pool(std::begin(c), std::end(c));
    if (k > pool.size()) {
        throw std::invalid_argument("sample: sample larger than population");
    }
    std::vector<T> result;
    result.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        std::size_t j = static_cast<std::size_t>(
            randint(static_cast<long long>(i), static_cast<long long>(pool.size()) - 1));
        std::swap(pool[i], pool[j]);
        result.push_back(pool[i]);
    }
    return result;
}

// ---------------------------------------------------------------------
// random.choices(population, k=1) -> k elements, with replacement
// random.choices(population, weights, k) -> weighted, with replacement
// ---------------------------------------------------------------------
template <typename Container>
std::vector<detail::value_type_of<const Container>>
choices(const Container& c, std::size_t k = 1) {
    using T = detail::value_type_of<const Container>;
    std::vector<T> pool(std::begin(c), std::end(c));
    if (pool.empty()) {
        throw std::invalid_argument("choices: population is empty");
    }
    std::vector<T> result;
    result.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        result.push_back(choice(pool));
    }
    return result;
}

template <typename Container>
std::vector<detail::value_type_of<const Container>>
choices(const Container& c, const std::vector<double>& weights, std::size_t k) {
    using T = detail::value_type_of<const Container>;
    std::vector<T> pool(std::begin(c), std::end(c));
    if (pool.size() != weights.size()) {
        throw std::invalid_argument("choices: weights must be the same length as population");
    }
    std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
    std::vector<T> result;
    result.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        result.push_back(pool[dist(detail::engine())]);
    }
    return result;
}

// ---------------------------------------------------------------------
// random.gauss(mu, sigma) / random.normalvariate(mu, sigma)
// ---------------------------------------------------------------------
inline double gauss(double mu = 0.0, double sigma = 1.0) {
    std::normal_distribution<double> dist(mu, sigma);
    return dist(detail::engine());
}

inline double normalvariate(double mu = 0.0, double sigma = 1.0) {
    return gauss(mu, sigma);
}

// ---------------------------------------------------------------------
// random.expovariate(lambd)
// ---------------------------------------------------------------------
inline double expovariate(double lambd) {
    std::exponential_distribution<double> dist(lambd);
    return dist(detail::engine());
}

// ---------------------------------------------------------------------
// random.triangular(low=0.0, high=1.0, mode=None)
// ---------------------------------------------------------------------
inline double triangular(double low = 0.0, double high = 1.0, double mode = -1.0) {
    if (mode < 0.0) {
        mode = (low + high) / 2.0;
    }
    double u = random();
    double c = (mode - low) / (high - low);
    if (u <= c) {
        return low + std::sqrt(u * (high - low) * (mode - low));
    }
    return high - std::sqrt((1.0 - u) * (high - low) * (high - mode));
}

// ---------------------------------------------------------------------
// random.getrandbits(k) -> a non-negative integer with k random bits
// ---------------------------------------------------------------------
inline uint64_t getrandbits(unsigned int k) {
    if (k == 0 || k > 64) {
        throw std::invalid_argument("getrandbits: k must be in [1, 64]");
    }
    uint64_t max_val = (k == 64) ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << k) - 1);
    std::uniform_int_distribution<uint64_t> dist(0, max_val);
    return dist(detail::engine());
}

} // namespace pyrandom

#endif // PYRANDOM_HPP
