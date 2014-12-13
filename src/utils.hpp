#ifndef UTILS_HPP
#define UTILS_HPP

#define FREE_RAM freeRam(__LINE__)
void freeRam (int line);

// see the GCC docs for variadic macros for why ## before __VA_ARGS__
// *_MSG functions use Serial.println() so a newline isn't needed in the fmt string.
#if 1
#define DEBUG_MSG(fmt, ...) serial_printf(F(fmt), ##__VA_ARGS__)
#else
#define DEBUG_MSG(fmt, ...) /* empty */
#endif

#define MSG(fmt, ...) serial_printf(F(fmt), ##__VA_ARGS__)
#define WARN_MSG(fmt, ...) serial_printf(F("WARN: " fmt), ##__VA_ARGS__)
#define ERROR_MSG(fmt, ...) serial_printf(F("ERR: " fmt), ##__VA_ARGS__)

void serial_printf(const char *fmt, ... );
void serial_printf(const __FlashStringHelper *fmt, ... );

#endif /* UTILS_HPP */
