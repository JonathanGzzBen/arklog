#include "tests.h"
#include "arklog/arklog.h"
#include "logger_tests.h"
#include "ring_buffer_tests.h"

#include <stdbool.h>
#include <stdio.h>

void test_condition(const char *const condition_description, bool condition) {
  printf(ANSI_COLOR_YELLOW "TESTING " ANSI_COLOR_RESET "%s: ",
         condition_description);
  if (condition)
    printf(ANSI_COLOR_GREEN "PASSED\n" ANSI_COLOR_RESET);
  else {
    printf(ANSI_COLOR_RED "FAILED\n" ANSI_COLOR_RESET);
  }
}

void start_test_section(const char *const section_name) {
  printf(ANSI_COLOR_BLUE LINE "%20s" ANSI_COLOR_MAGENTA
                              "  START " ANSI_COLOR_BLUE LINE
                              "\n" ANSI_COLOR_RESET,
         section_name);
}

void end_test_section(const char *const section_name) {
  printf(ANSI_COLOR_BLUE LINE "%20s" ANSI_COLOR_MAGENTA
                              "    END " ANSI_COLOR_BLUE LINE
                              "\n" ANSI_COLOR_RESET,
         section_name);
}

int main(void) {
  ARKLOG_TRACE("Hola");
  start_test_section("RING BUFFER");
  test_ring_buffer();
  end_test_section("RING BUFFER");
  start_test_section("LOGGER");
  test_logger();
  end_test_section("LOGGER");
  return 0;
}
