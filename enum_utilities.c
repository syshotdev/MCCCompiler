#include "enum_utilities.h"

// Define the function here, so it's compiled only once
const char *enum_to_string(int enum_value, const char *enum_strings[]) {
    return enum_strings[enum_value];
}

