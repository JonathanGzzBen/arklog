#ifndef ALOG_TESTS_H
#define ALOG_TESTS_H
#include <stdbool.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
// #define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define LINE "==========="

void test_condition(const char *const condition_description, bool condition);
void start_test_section(const char *const section_name);
void end_test_section(const char *const section_name);

#endif // ALOG_TESTS_H
