#include "ext_igbinary.hpp"

#include "hphp/util/string-vsnprintf.h"

#include <string>

namespace HPHP {
IgbinaryWarning::IgbinaryWarning(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	format(fmt, ap);
	va_end(ap);
}
void throw_igbinary_exception(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	std::string msg;
	string_vsnprintf(msg, fmt, ap);
	va_end(ap);

	SystemLib::throwExceptionObject(Variant(msg));
}
} // Namespace
