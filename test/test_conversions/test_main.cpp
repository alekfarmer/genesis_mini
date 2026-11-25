// Host unit tests for conversions (make test).
#include <unity.h>

#include "conversions.h"

using conversions::celsiusToFahrenheit;
using conversions::panelColor;

void setUp() {}
void tearDown() {}

static void test_celsius_to_fahrenheit() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 32.0f, celsiusToFahrenheit(0.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 212.0f, celsiusToFahrenheit(100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -40.0f, celsiusToFahrenheit(-40.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 78.8f, celsiusToFahrenheit(26.0f));
}

static void test_panel_color_swaps_red_and_blue() {
  TEST_ASSERT_EQUAL_HEX16(0x001F, panelColor(255, 0, 0));  // red lands in the "blue" bits
  TEST_ASSERT_EQUAL_HEX16(0xF800, panelColor(0, 0, 255));  // blue lands in the "red" bits
  TEST_ASSERT_EQUAL_HEX16(0x07E0, panelColor(0, 255, 0));  // green unchanged
}

static void test_panel_color_greys_unchanged() {
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, panelColor(255, 255, 255));
  TEST_ASSERT_EQUAL_HEX16(0x0000, panelColor(0, 0, 0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_celsius_to_fahrenheit);
  RUN_TEST(test_panel_color_swaps_red_and_blue);
  RUN_TEST(test_panel_color_greys_unchanged);
  return UNITY_END();
}
