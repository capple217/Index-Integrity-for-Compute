#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

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

enum class Terms { on_demand, reserved, spot };

typedef struct Quote {
  int provider; // dense id 0...N-1
  int64_t t;    // step index
  double total_price;
  int num_gpus;
  double hours;
  Terms terms;
} Quote;

typedef struct ProviderObs {
  int provider;
  double price;         // $/GPU-hour, baseline
  int records;          // count this step
  bool present = false; // false = missing
} ProviderObs;

std::vector<Quote> ingest(int64_t t, char *records) {
  std::vector<Quote> vec;
  auto file = open(records);
  // For each listing from a given provider:
  Quote q;
  // read file and fill Quote until step != t
  // If malformed row, reject output and logs them in vector instead
  vec.push_back(q);
  return vec;
}

double normalize(Quote &q) {
  double val = q.total_price / (q.num_gpus * q.hours);
  // Multiply val by the factor per contract type to reach the baseline
  // factors determined by Terms
  // take Log of price starting here instead of outside the function
  return val;
}

// seems to pick the upper median for even sizes
double median(std::vector<double> vec) {
  auto mid = vec.begin() + vec.size() / 2;
  std::nth_element(vec.begin(), mid, vec.end());
  return *mid;
}

std::vector<ProviderObs> panel(std::vector<Quote> &quotes, int N) {
  // hashmap may be replaced with vector iteration since ids are dense
  std::unordered_map<int, std::vector<double>> prices;
  for (const Quote &q : quotes) {
    prices[q.provider].push_back(normalize(q));
  }

  // take all quotes per provider and fill one ProviderObs with the median price
  // If one provider has no listing at time t, then present = false
  std::vector<ProviderObs> p(N);
  for (int i = 0; i < N; ++i) {
    ProviderObs obs;
    obs.provider = i;
    if (prices.find(i) != prices.end()) {
      obs.present = true;
      obs.records = (int)prices[i].size();
      obs.price = median(prices[i]);
    }
    p[i] = obs;
  }
  return p;
}

double consensus_center(std::vector<ProviderObs> &obs) {
  std::vector<double> vec;
  for (const auto &v : obs) {
    if (v.present)
      vec.push_back(v.price);
  }

  return median(vec);
}

double consensus_scale(double center, std::vector<ProviderObs> &obs) {
  std::vector<double> vec;
  for (const auto &v : obs) {
    if (v.present)
      vec.push_back(std::fabs(v.price - center));
  }

  return 1.4826 * median(vec);
}

std::vector<double> residuals(std::vector<ProviderObs> &panel, double center,
                              double scale, int N) {
  std::vector<double> vec(N);
  for (int i = 0; i < N; ++i) {
    if (panel[i].present) {
      vec[i] = ((panel[i].price - center) / scale);
    } else {
      vec[i] = std::numeric_limits<double>::quiet_NaN();
    }
  }

  return vec;
}

int process_step(int64_t t, char *records) {
  std::vector<Quote> vec = ingest(t, records);

  // Define N outside
  int N = 10;
  auto panel_table = panel(vec, N);

  // consensus
  double center = consensus_center(panel_table);
  double scale = consensus_scale(center, panel_table);

  // Check here for enough eligible providers
  // If not, mark the step degraded and skips upcoming steps

  // Residuals
  std::vector<double> z = residuals(panel_table, center, scale, N);
}

int main() {
  std::cout << "Hello World\n";
  return 0;
}
