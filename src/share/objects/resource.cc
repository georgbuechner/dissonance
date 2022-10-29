#include "share/objects/resource.h"
#include "spdlog/spdlog.h"

Resource::Resource(double init, int max, int distributed_iron, bool to_int, position_t pos) 
    : to_int_(to_int), pos_(pos) {
  free_ = init;
  limit_ = max;
  bound_ = 0;
  distributed_iron_ = distributed_iron;
  blocked_ = false;
  total_ = init;
  spent_ = 0;
}

// getter
double Resource::cur() const {
  return (to_int_) ? (int)free_ : free_;
}
double Resource::bound() const {
  return bound_;
}
int Resource::limit() const {
  return limit_;
}
int Resource::distributed_iron() const {
  return distributed_iron_;
}
bool Resource::blocked() const {
  return blocked_;
}
position_t Resource::pos() const {
  return pos_;
}
double Resource::total() const {
  return total_;
}
double Resource::spent() const {
  return spent_;
}
double Resource::average_boost() const {
  if (average_boost_.size() == 0)
    return 0;
  return std::accumulate(average_boost_.begin(), average_boost_.end(), 0.0) / average_boost_.size();
}
double Resource::average_bound() const {
  if (average_bound_.size() == 0)
    return 0;
  return std::accumulate(average_bound_.begin(), average_bound_.end(), 0.0) / average_bound_.size();
}
double Resource::average_neg_factor() const {
  if (average_neg_factor_.size() == 0)
    return 0;
  return std::accumulate(average_neg_factor_.begin(), average_neg_factor_.end(), 0.0) / average_neg_factor_.size();
}
double Resource::active_percent() const {
  return (double)active_percent_.first/(double)active_percent_.second;
}

// setter 
void Resource::set_cur(double value) {
  free_ = value;
}
void Resource::set_bound(double value) {
  bound_ = value;
}
void Resource::set_distributed_iron(int value) {
  distributed_iron_ = value;
}
void Resource::set_limit(int value) {
  limit_ = value;
}
void Resource::set_blocked(bool value) {
  blocked_ = value;
}

// functions


bool Resource::Active() const {
  return distributed_iron_ >= 2;
}

void Resource::Call() {
  active_percent_.second++;
  active_percent_.first += (Active()) ? 1 : 0;
}

void Resource::Increase(double gain, double slowdown) {
  double calc_boast = 1 + static_cast<double>(distributed_iron_)/10;
  double calc_negative_factor = 1-(free_+bound_)/limit_;
  double val = (calc_boast * gain * calc_negative_factor)/slowdown;
  if (val < 0) {
    spdlog::get(LOGGER)->error("Increasing by neg value!! boast: {} gain: {} neg: {}, others {}, {}, {}", 
        calc_boast, gain, calc_negative_factor, free_, bound_, limit_);
  }
  else {
    if (val+free_+bound_ > limit_)
      val = 0;
    free_ += val;
    total_ += val;
    average_boost_.push_back(calc_boast);
    average_bound_.push_back(bound_);
    average_neg_factor_.push_back(calc_negative_factor);
  }
}

void Resource::Decrease(double val, bool bind_resources) {
  if (val < 0) 
    spdlog::get(LOGGER)->error("Decreasing by negative value: {}", val);
  free_ -= val;
  spent_ += val;
  if (bind_resources)
    bound_ += val;
}
