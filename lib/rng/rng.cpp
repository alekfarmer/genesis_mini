#include "rng.h"

Rng::Rng(uint32_t seed) : state_(seed != 0 ? seed : 0x9E3779B9u) {}

uint32_t Rng::next() {
  // xorshift32 (Marsaglia): never returns 0 for a non-zero state.
  state_ ^= state_ << 13;
  state_ ^= state_ >> 17;
  state_ ^= state_ << 5;
  return state_;
}

uint32_t Rng::below(uint32_t n) { return next() % n; }
