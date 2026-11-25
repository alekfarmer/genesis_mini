#include "menu_core.h"

MenuModel::MenuModel(uint8_t count, uint8_t visible_rows) : count_(count), visible_(visible_rows) {}

void MenuModel::move(int32_t steps) {
  if (count_ == 0) {
    return;
  }
  const int32_t count = count_;
  int32_t next = (static_cast<int32_t>(selected_) + steps) % count;
  if (next < 0) {
    next += count;
  }
  selected_ = static_cast<uint8_t>(next);
  scrollToSelection();
}

void MenuModel::select(uint8_t index) {
  if (index < count_) {
    selected_ = index;
    scrollToSelection();
  }
}

void MenuModel::scrollToSelection() {
  if (visible_ == 0 || count_ <= visible_) {
    first_ = 0;
  } else if (selected_ < first_) {
    first_ = selected_;
  } else if (selected_ >= first_ + visible_) {
    first_ = static_cast<uint8_t>(selected_ - visible_ + 1);
  }
}
