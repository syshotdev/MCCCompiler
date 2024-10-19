#ifndef enum_utilities_h
#define enum_utilities_h

// Macro for generating enum values
#define GENERATE_ENUM(ENUM) ENUM,

// Macro for generating string representations
#define GENERATE_STRING(STRING) #STRING,

// Generic function that gets a string value from enum value
const char *enum_to_string(int enum_value, const char *enum_strings[]) {
    return enum_strings[enum_value];
}

#endif
