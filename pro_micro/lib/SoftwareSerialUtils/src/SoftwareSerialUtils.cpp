#include <SoftwareSerialUtils.h>

void serialPrintf(Serial_& serial, const char* fmt, ...) {
  char buf[PRINTF_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  serial.print(buf);
}