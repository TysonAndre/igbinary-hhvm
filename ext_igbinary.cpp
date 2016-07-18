/**
 * WIP
 */

/* Same as equivalent php version? */
#define IGBINARY_HHVM_VERSION "1.2.1-dev"

#include "hphp/runtime/ext/extension.h"
#include "ext_igbinary.hpp"

namespace HPHP {

Variant HHVM_FUNCTION(igbinary_serialize, const Variant &var) {
	try {
		return igbinary_serialize(var);
	} catch (Exception& e) {
		raise_warning(e.getMessage());
		return false;
	}
}

Variant HHVM_FUNCTION(igbinary_unserialize, const String &serialized) {
	if (serialized.size() <= 0) {
		return false;
	}
	Variant result;
	try {
		igbinary_unserialize(reinterpret_cast<const uint8_t*>(serialized.c_str()), serialized.size(), result);
		return result;
	} catch (Exception &e) {
		raise_warning(e.getMessage());
		return false;
	}
}

static class IgbinaryExtension : public Extension {
  public:
	IgbinaryExtension() : Extension("igbinary", IGBINARY_HHVM_VERSION) {}
	void moduleInit() override {
		HHVM_FE(igbinary_serialize);
		HHVM_FE(igbinary_unserialize);

		loadSystemlib();
	}
} s_igbinary_extension;

HHVM_GET_MODULE(igbinary);

} // namespace HPHP
