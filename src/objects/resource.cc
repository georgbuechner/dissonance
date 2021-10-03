#include "objects/resource.h"

Resource::Resource(double init, unsigned int max, int distributed_iron, bool to_int) : to_int_(to_int) {
  free_ = init;
  limit_ = max;
  bound_ = 0;
  distributed_iron_ = distributed_iron;
  blocked_ = false;
}

// getter
double Resource::cur() const {
  return (to_int_) ? (int)free_ : free_;
}
double Resource::bound() const {
  return bound_;
}
unsigned int Resource::limit() const {
  return limit_;
}
unsigned int Resource::distributed_iron() const {
  return distributed_iron_;
}

// setter 
void Resource::set_cur(double value) {
  free_ = value;
}
void Resource::set_bound(double value) {
  bound_ = value;
}
void Resource::set_distribited_iron(unsigned int value) {
  distributed_iron_ = value;
}
void Resource::set_limit(unsigned int value) {
  limit_ = value;
}

// functions

bool Resource::Active() const {
  return distributed_iron_ >= 2;
}

std::string Resource::Print() const {
  return utils::Dtos(cur()) + "+" + utils::Dtos(bound_) + "/" + utils::Dtos(limit_);
}

void Resource::IncreaseResource(double gain, int slowdown) {
  auto calc_boast = [](int boast) -> double { return 1+static_cast<double>(boast)/10; };
  auto calc_negative_factor = [](double cur, double max) -> double { return 1-cur/max; };
  double val = (calc_boast(distributed_iron_) * ((false) ? 1 : gain) * calc_negative_factor(free_+bound_, limit_))/slowdown;
  if (val < 0)
    spdlog::get(LOGGER)->error("Increasing by neg value!! boast: {} gain: {} neg: {}, others {}, {}, {}", 
        calc_boast(distributed_iron_), gain, calc_negative_factor(free_+bound_, limit_), free_, bound_, limit_);
  free_ += val;
}
