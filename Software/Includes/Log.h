/** @file Log.h
 * Use the UART to output log messages.
 * @author Adrien RICCIARDI
 */
#ifndef H_LOG_H
#define H_LOG_H

#include <stdio.h>

//-------------------------------------------------------------------------------------------------
// Constants and macros
//-------------------------------------------------------------------------------------------------
/** Print a printf() like message on the serial port preceded by the function name and the line.
 * @param Is_Enabled Set to 1 to compile the message in, set to 0 do remove the message at the compilation time.
 * @param Format A printf() like format string.
 * @note The internal buffer is limited to 256 bytes, do not write too long strings.
 */
#ifdef LOG_ENABLE_LOGGING
	#define LOG(Is_Enabled, Format, ...) do { if (Is_Enabled) printf("[%s:%d] " Format "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)
#else
	#define LOG(Is_Enabled, Format, ...) do {} while (0)
#endif

/** Discard the code surrounded by LOG_BEGIN_SECTION() and LOG_END_SECTION() if the logs are disabled.
 * @param Is_Enabled If set to 0, the code is also discarded when the logs are enabled (this is useful to enable or disable a specific module logs).
 */
#ifdef LOG_ENABLE_LOGGING
	#define LOG_BEGIN_SECTION(Is_Enabled) if (Is_Enabled) {
#else
	#define LOG_BEGIN_SECTION(Is_Enabled) if (0) {
#endif
/** End a code section opened with LOG_BEGIN_SECTION(). */
#define LOG_END_SECTION() }
#endif
