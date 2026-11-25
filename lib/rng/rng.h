#pragma once

#include <stdint.h>

/// Small deterministic random generator (xorshift32). Same seed, same
/// numbers, which keeps game tests repeatable; on the board, games seed it
/// from the hardware random number generator. Pure logic, host-tested.
class Rng {
 public:
  explicit Rng(uint32_t seed);

  /// Next 32-bit value (never 0).
  uint32_t next();

  /// A value in [0, n). n must be > 0.
  uint32_t below(uint32_t n);

 private:
  uint32_t state_;
};
