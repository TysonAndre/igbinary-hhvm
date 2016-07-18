// HHVM Variant to igbinary serialized binary string implementation

#ifndef EXT_IGBINARY_H__
#define EXT_IGBINARY_H__

#include "hphp/runtime/ext/extension.h"

#define IGBINARY_FORMAT_VERSION 0x00000002

/* {{{ Types */
enum igbinary_type {
	/* 00 */ igbinary_type_null,			/**< Null. */

	/* 01 */ igbinary_type_ref8,			/**< Array reference. */
	/* 02 */ igbinary_type_ref16,			/**< Array reference. */
	/* 03 */ igbinary_type_ref32,			/**< Array reference. */

	/* 04 */ igbinary_type_bool_false,		/**< Boolean true. */
	/* 05 */ igbinary_type_bool_true,		/**< Boolean false. */

	/* 06 */ igbinary_type_long8p,			/**< Long 8bit positive. */
	/* 07 */ igbinary_type_long8n,			/**< Long 8bit negative. */
	/* 08 */ igbinary_type_long16p,			/**< Long 16bit positive. */
	/* 09 */ igbinary_type_long16n,			/**< Long 16bit negative. */
	/* 0a */ igbinary_type_long32p,			/**< Long 32bit positive. */
	/* 0b */ igbinary_type_long32n,			/**< Long 32bit negative. */

	/* 0c */ igbinary_type_double,			/**< Double. */

	/* 0d */ igbinary_type_string_empty,	/**< Empty string. */

	/* 0e */ igbinary_type_string_id8,		/**< String id. */
	/* 0f */ igbinary_type_string_id16,		/**< String id. */
	/* 10 */ igbinary_type_string_id32,		/**< String id. */

	/* 11 */ igbinary_type_string8,			/**< String. */
	/* 12 */ igbinary_type_string16,		/**< String. */
	/* 13 */ igbinary_type_string32,		/**< String. */

	/* 14 */ igbinary_type_array8,			/**< Array. */
	/* 15 */ igbinary_type_array16,			/**< Array. */
	/* 16 */ igbinary_type_array32,			/**< Array. */

	/* 17 */ igbinary_type_object8,			/**< Object. */
	/* 18 */ igbinary_type_object16,		/**< Object. */
	/* 19 */ igbinary_type_object32,		/**< Object. */

	/* 1a */ igbinary_type_object_id8,		/**< Object string id. */
	/* 1b */ igbinary_type_object_id16,		/**< Object string id. */
	/* 1c */ igbinary_type_object_id32,		/**< Object string id. */

	/* 1d */ igbinary_type_object_ser8,		/**< Object serialized data. */
	/* 1e */ igbinary_type_object_ser16,	/**< Object serialized data. */
	/* 1f */ igbinary_type_object_ser32,	/**< Object serialized data. */

	/* 20 */ igbinary_type_long64p,			/**< Long 64bit positive. */
	/* 21 */ igbinary_type_long64n,			/**< Long 64bit negative. */

	/* 22 */ igbinary_type_objref8,			/**< Object reference. */
	/* 23 */ igbinary_type_objref16,		/**< Object reference. */
	/* 24 */ igbinary_type_objref32,		/**< Object reference. */

	/* 25 */ igbinary_type_ref,				/**< Simple reference */
};

namespace HPHP {
/** Return the serialized data, or throw an Exception */
void igbinary_unserialize(const uint8_t *buf, size_t buf_len, Variant& result);
/** Unserialize the data, or clean up and throw an Exception. Effectively constant, unless __sleep modifies something. */
String igbinary_serialize(const Variant& variant);
}

#endif
