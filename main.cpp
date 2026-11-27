#include <iostream>

// Starting off with psuedo-code for Index Integrity Checking function before
// initial implementation

/*
 * Each step costs O(N) for N providers, so it runs on a live feed/replayed
 * history alike
 *
 * Each stage is a functoin; each state is small and explicit
 *
 * One config struct
 *
 * Deterministic
 * */

typedef struct Quote {
  int provider; // dense id 0...N-1
  int64_t t;    // step index
  double total_price;
  int num_gpus;
  double hours;
  Terms terms; // on-demand, reserved...
} Quote;

double normalize(Quote q) {
  double val = q.total_price / (q.num_gpus * q.hours);
  // Multiply val by the factor per contract type to reach the baseline
  return val;
}

int integrity_run(...) {
  // 1. Convert data in rows into Quote(s) -> Quote quote

  // double normalize_quote = Normalize(quote)
}

int main() {
  std::cout << "Hello World\n";
  return 0;
}
