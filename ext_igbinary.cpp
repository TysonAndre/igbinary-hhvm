/*
  +----------------------------------------------------------------------+
  | See COPYING file for further copyright information                   |
  +----------------------------------------------------------------------+
  | Author of hhvm fork: Tyson Andre <tysonandre775@hotmail.com>         |
  | Author of original igbinary: Oleg Grenrus <oleg.grenrus@dynamoid.com>|
  | See CREDITS for contributors                                         |
  +----------------------------------------------------------------------+
*/

/* Same as equivalent php version? */
#define IGBINARY_HHVM_VERSION "1.2.5-dev"

#include "ext_igbinary.hpp"

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/ext/extension-registry.h"

namespace HPHP {

Variant HHVM_FUNCTION(igbinary_serialize, const Variant &var) {
	return igbinary_serialize(var);
}

Variant HHVM_FUNCTION(igbinary_unserialize, const String &serialized) {
	if (serialized.size() <= 0) {
		return false;
	}
	Variant result;
	try {
		igbinary_unserialize(reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size(), result);
		return result;
	} catch (Exception &e) {
		raise_warning(e.getMessage());
		return false;
	}
}

struct Igbinary {
  public:
    bool compact_strings{true};
};

const StaticString s_igbinary_ext_name("igbinary");

IMPLEMENT_THREAD_LOCAL_NO_CHECK(Igbinary, s_igbinary);

bool igbinary_should_compact_strings() {
  return s_igbinary->compact_strings;
}

static class IgbinaryExtension : public Extension {
  public:
	IgbinaryExtension() : Extension("igbinary", IGBINARY_HHVM_VERSION) {}
	void moduleInit() override {
		HHVM_FE(igbinary_serialize);
		HHVM_FE(igbinary_unserialize);

		loadSystemlib();
	}

	void threadInit() override {
		assert(s_igbinary.isNull());
		s_igbinary.getCheck();
		Extension* ext = ExtensionRegistry::get(s_igbinary_ext_name);
		assert(ext);
		IniSetting::Bind(ext, IniSetting::PHP_INI_ALL,
		                 "igbinary.compact_strings", "1",
		                 &s_igbinary->compact_strings);
	}

	void threadShutdown() override {
		s_igbinary.destroy();
	}
} s_igbinary_extension;

HHVM_GET_MODULE(igbinary);

} // namespace HPHP
