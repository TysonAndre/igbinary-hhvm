/* Same as equivalent php version? */
#define IGBINARY_HHVM_VERSION "1.2.1-dev"

#include "hphp/runtime/ext/extension.h"

namespace HPHP {

Variant HHVM_FUNCTION(igbinary_serialize, const Variant &var) {
  raise_warning("Not implemented yet");
  return false;
}

Variant HHVM_FUNCTION(igbinary_unserialize, const String &pack) {
  raise_warning("Not implemented yet");
  return false;
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
