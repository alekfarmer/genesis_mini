// Host unit tests for MenuModel (make test).
#include <unity.h>

#include "menu_core.h"

void setUp() {}
void tearDown() {}

static void test_starts_at_top() {
  MenuModel m(6, 4);
  TEST_ASSERT_EQUAL_UINT8(0, m.selected());
  TEST_ASSERT_EQUAL_UINT8(0, m.firstVisible());
}

static void test_moves_and_wraps_both_ways() {
  MenuModel m(6, 4);
  m.move(1);
  TEST_ASSERT_EQUAL_UINT8(1, m.selected());
  m.move(5);  // 1 + 5 = 6 -> wraps to 0
  TEST_ASSERT_EQUAL_UINT8(0, m.selected());
  m.move(-1);
  TEST_ASSERT_EQUAL_UINT8(5, m.selected());
  m.move(-13);  // 5 - 13 = -8 -> 4 (mod 6)
  TEST_ASSERT_EQUAL_UINT8(4, m.selected());
}

static void test_window_scrolls_to_keep_selection_visible() {
  MenuModel m(6, 4);
  m.move(3);
  TEST_ASSERT_EQUAL_UINT8(0, m.firstVisible());  // rows 0-3 visible
  m.move(1);                                      // select 4
  TEST_ASSERT_EQUAL_UINT8(1, m.firstVisible());
  m.move(1);                                      // select 5
  TEST_ASSERT_EQUAL_UINT8(2, m.firstVisible());
  m.move(1);                                      // wrap to 0
  TEST_ASSERT_EQUAL_UINT8(0, m.firstVisible());
  m.move(-1);                                     // wrap to 5
  TEST_ASSERT_EQUAL_UINT8(2, m.firstVisible());
}

static void test_short_list_never_scrolls() {
  MenuModel m(2, 4);
  m.move(3);
  TEST_ASSERT_EQUAL_UINT8(1, m.selected());
  TEST_ASSERT_EQUAL_UINT8(0, m.firstVisible());
}

static void test_select_directly() {
  MenuModel m(6, 4);
  m.select(5);
  TEST_ASSERT_EQUAL_UINT8(5, m.selected());
  TEST_ASSERT_EQUAL_UINT8(2, m.firstVisible());
  m.select(9);  // out of range: ignored
  TEST_ASSERT_EQUAL_UINT8(5, m.selected());
}

static void test_empty_menu_is_safe() {
  MenuModel m(0, 4);
  m.move(3);
  TEST_ASSERT_EQUAL_UINT8(0, m.selected());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_at_top);
  RUN_TEST(test_moves_and_wraps_both_ways);
  RUN_TEST(test_window_scrolls_to_keep_selection_visible);
  RUN_TEST(test_short_list_never_scrolls);
  RUN_TEST(test_select_directly);
  RUN_TEST(test_empty_menu_is_safe);
  return UNITY_END();
}
