#include "snake_core.h"

namespace snake {
namespace {

constexpr uint32_t kFirstStepMs = 180;
constexpr uint32_t kStepSpeedupMs = 5;
constexpr uint32_t kFastestStepMs = 70;

Dir turned(Dir d, int8_t turn) { return static_cast<Dir>((static_cast<int>(d) + turn + 4) % 4); }

}  // namespace

Game::Game(uint32_t seed) : rng_(seed) { reset(seed); }

void Game::reset(uint32_t seed) {
  rng_ = Rng(seed);
  // Tail first in the ring buffer, head last: (14,7) (15,7) (16,7).
  const int8_t mid_x = kCols / 2;
  const int8_t mid_y = kRows / 2;
  for (int i = 0; i < kStartLength; ++i) {
    body_[i] = {static_cast<int8_t>(mid_x - (kStartLength - 1) + i), mid_y};
  }
  head_ = kStartLength - 1;
  length_ = kStartLength;
  dir_ = Dir::kRight;
  turn_count_ = 0;
  state_ = State::kPlaying;
  score_ = 0;
  placeFood();
  ++revision_;
}

void Game::turnLeft() {
  if (turn_count_ < 2) turns_[turn_count_++] = -1;
}

void Game::turnRight() {
  if (turn_count_ < 2) turns_[turn_count_++] = 1;
}

bool Game::step() {
  if (state_ != State::kPlaying) {
    return false;
  }
  if (turn_count_ > 0) {
    dir_ = turned(dir_, turns_[0]);
    turns_[0] = turns_[1];
    --turn_count_;
  }
  const Cell head = segment(0);
  Cell next = head;
  switch (dir_) {
    case Dir::kUp:    --next.y; break;
    case Dir::kRight: ++next.x; break;
    case Dir::kDown:  ++next.y; break;
    case Dir::kLeft:  --next.x; break;
  }
  ++revision_;
  if (next.x < 0 || next.x >= kCols || next.y < 0 || next.y >= kRows) {
    state_ = State::kGameOver;
    return false;
  }
  const bool eating = (next.x == food_.x && next.y == food_.y);
  // The tail moves out of its square this step unless the snake is growing,
  // so moving into that square is allowed.
  const uint16_t checked = eating ? length_ : static_cast<uint16_t>(length_ - 1);
  for (uint16_t i = 0; i < checked; ++i) {
    const Cell s = segment(i);
    if (s.x == next.x && s.y == next.y) {
      state_ = State::kGameOver;
      return false;
    }
  }
  head_ = static_cast<uint16_t>((head_ + 1) % kMaxLength);
  body_[head_] = next;
  if (eating) {
    ++length_;
    ++score_;
    if (length_ == kMaxLength) {
      state_ = State::kGameOver;  // the whole grid is snake: nothing left to win
    } else {
      placeFood();
    }
  }
  return eating;
}

Cell Game::segment(uint16_t i) const { return body_[(head_ + kMaxLength - i) % kMaxLength]; }

bool Game::occupies(int x, int y) const {
  for (uint16_t i = 0; i < length_; ++i) {
    const Cell s = segment(i);
    if (s.x == x && s.y == y) return true;
  }
  return false;
}

uint32_t Game::stepIntervalMs() const {
  const uint32_t speedup = score_ * kStepSpeedupMs;
  return speedup >= kFirstStepMs - kFastestStepMs ? kFastestStepMs : kFirstStepMs - speedup;
}

void Game::setFoodForTest(int x, int y) {
  food_ = {static_cast<int8_t>(x), static_cast<int8_t>(y)};
  ++revision_;
}

void Game::placeFood() {
  // Pick uniformly among the free squares: choose the k-th free one.
  uint32_t k = rng_.below(static_cast<uint32_t>(kMaxLength - length_));
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (occupies(x, y)) continue;
      if (k == 0) {
        food_ = {static_cast<int8_t>(x), static_cast<int8_t>(y)};
        return;
      }
      --k;
    }
  }
}

}  // namespace snake
