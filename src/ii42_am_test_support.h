#ifndef II42_AM_TEST_SUPPORT_H
#define II42_AM_TEST_SUPPORT_H

#include "postgres.h"

bool ii42_am_test_setting_enabled(
    const char *setting_name,
    const char *description
);
void ii42_am_test_pause_ms(
    const char *setting_name,
    const char *description
);
void ii42_am_test_error_if_enabled(
    const char *setting_name,
    const char *description
);

#endif
