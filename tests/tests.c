#include "tests.h"
#include "arklog/arklog.h"
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

int main(void) {
  ARKLOG_TRACE("Hola");
  test_ring_buffer();
  return 0;
}
