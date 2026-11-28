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

typedef struct Config {
  double h;
  double delta;
  double min_providers;
  double min_scale;
} Config;

enum class ProvState { Active, Flagged, Held, Dropped };

enum class Terms { on_demand, reserved, spot };

class Detector {
public:
  virtual ~Detector() = default;
  // feed one residual; return true on alarm
  virtual bool update(double z) = 0;
  virtual double statistic() const = 0;
  virtual void reset() = 0;
};

// behind an interface, so any other robust variant can be swapped in
class TwoSidedCusum : public Detector {
  double w_up, w_down;
  double delta, h; // from Config
public:
  TwoSidedCusum(double up, double down) : w_up{up}, w_down{down} {}
  bool update(double z) {
    w_up = std::max(0.0, w_up + z - delta / 2);
    w_down = std::max(0.0, w_down - z - delta / 2);
    if (w_up >= h || w_down >= h)
      return true;
    return false;
  }

  double statistic() const override;
  void reset() override;
};

struct Quote {
  int provider; // dense id 0...N-1
  int64_t t;    // step index
  double total_price;
  int num_gpus;
  double hours;
  Terms terms;
} Quote;

struct ProviderObs {
  int provider;
  double price;         // $/GPU-hour, baseline
  int records;          // count this step
  bool present = false; // false = missing
} ProviderObs;

struct State {
  std::vector<TwoSidedCusum> det; // One per provider
  std::vector<ProvState> status;  // One per provider
  std::vector<int> steps_in;      // steps spent in current status
  std::vector<int> clean;         // consecutive in-line steps
  std::vector<double> last_good;  // for carry-forward
  double last_index;
};

struct StepResult {
  int64_t t;
  double index_value;
  std::vector<Flag> flags;
  bool degraded;
};

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
  return std::log(val);
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
  if (obs.empty())
    return 0.0;
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
    // also need to check if !held[v.provider] so need to construct container
    // 'held'
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

int process_step(int64_t t, char *records, State state, const Config &cfg) {
  std::vector<Quote> vec = ingest(t, records);

  // Define N outside
  int N = 10;
  auto panel_table = panel(vec, N);

  // Check if we're under config.min_providers here to see if we should
  // skip ahead to later steps aka define container 'config'

  // consensus
  double center = consensus_center(panel_table);
  // floor the spread: scale = max(scale, config.min_scale)
  double scale = consensus_scale(center, panel_table);

  // Check here for enough eligible providers using config
  // If not, mark the step degraded and skips upcoming steps

  // Residuals
  std::vector<double> z = residuals(panel_table, center, scale, N);

  // Might instantiate cusum from outside and call it per step so here
  for (const auto &val : z) {
    cusum::update(val);
  }
}

int main() {
  Config TwoSidedCusum cusum(0, 0);
  std::cout << "Hello World\n";
  return 0;
}
