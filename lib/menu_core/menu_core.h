#pragma once

#include <stdint.h>

/// Selection and scrolling for a vertical list menu. The knob moves the
/// selection, wrapping from the last item to the first and back; the visible
/// window scrolls so the selection is always on screen. Pure logic,
/// host-tested.
class MenuModel {
 public:
  /// @param count        number of items.
  /// @param visible_rows how many items fit on screen at once (> 0).
  MenuModel(uint8_t count, uint8_t visible_rows);

  /// Move the selection by `steps` (+ = down), wrapping around.
  void move(int32_t steps);

  /// Select an item directly; out-of-range indexes are ignored.
  void select(uint8_t index);

  uint8_t selected() const { return selected_; }

  /// Index of the top item on screen.
  uint8_t firstVisible() const { return first_; }

 private:
  void scrollToSelection();

  uint8_t count_;
  uint8_t visible_;
  uint8_t selected_ = 0;
  uint8_t first_ = 0;
};
