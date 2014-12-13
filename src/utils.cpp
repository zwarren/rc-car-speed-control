
#include <Arduino.h>
#include "utils.hpp"

void freeRam (int line) {
	extern int __heap_start, *__brkval;
	int v;
	int amount = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	DEBUG_MSG("Mem %d:%d", line, amount);
}

#define MAX_SERIAL_PRINTF_LEN 64
static char buf[MAX_SERIAL_PRINTF_LEN];

void serial_printf(const char *fmt, ... )
{
	va_list args;	
	va_start (args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	Serial.println(buf);
}

void serial_printf(const __FlashStringHelper *fmt, ... )
{
	va_list args;
	va_start (args, fmt);
#ifdef __AVR__
	vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
	vsnprintf(buf, sizeof(buf), (const char *)fmt, args);
#endif
	va_end(args);
	Serial.println(buf);
}

