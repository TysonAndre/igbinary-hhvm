#include <stddef.h>

#include "ext_igbinary.hpp"
#include "hphp/runtime/base/string-buffer.h"
#include "hash.h"
#include "hash_ptr.h"

using namespace HPHP;

namespace{
/** Serializer data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_serialize_data {
    StringBuffer buffer;
	bool scalar;				/**< Serializing scalar. */
	bool compact_strings;		/**< Check for duplicate strings. */
	struct hash_si strings;		/**< Hash of already serialized strings. */
	struct hash_si_ptr references;	/**< Hash of already serialized potential references. (non-NULL uintptr_t => int32_t) */
	int references_id;			/**< Number of things that the unserializer might think are references. >= length of references */
	int string_count;			/**< Serialized string count, used for back referencing */
};

/* {{{ igbinary_serialize_data_init */
/** Inits igbinary_serialize_data. */
inline static int igbinary_serialize_data_init(struct igbinary_serialize_data *igsd, bool scalar) {
	int r = 0;

	igsd->buffer.setOutputLimit(StringData::MaxSize);
	igsd->string_count = 0;

	igsd->scalar = scalar;
	if (!igsd->scalar) {
		hash_si_init(&igsd->strings, 16);
		hash_si_ptr_init(&igsd->references, 16);
		igsd->references_id = 0;
	}

	igsd->compact_strings = true; /* FIXME allow ini options parsing */

	return r;
}

/* {{{ igbinary_serialize8 */
/** Serialize 8bit value. */
#define igbinary_serialize8(igsd, i) do{ (igsd)->buffer.append((unsigned char)i); } while(0)
/* }}} */
/* {{{ igbinary_serialize32 */
/** Serialize 32bit value. */
inline static int igbinary_serialize32(struct igbinary_serialize_data *igsd, uint32_t i) {
    StringBuffer& buf = igsd->buffer;
    char* const bytes = buf.appendCursor(4);

    bytes[0] = (char) ((i >> 24) & 0xff);
	bytes[1] = (char) ((i >> 16) & 0xff);
	bytes[2] = (char) ((i >> 8) & 0xff);
	bytes[3] = (char) (i & 0xff);
    buf.resize(buf.size() + 4);

	return 0;
}
/* }}} */


/* {{{ igbinary_serialize_header */
/** Serializes header. */
inline static int igbinary_serialize_header(struct igbinary_serialize_data *igsd) {
	return igbinary_serialize32(igsd, IGBINARY_FORMAT_VERSION); /* version */
}
/* }}} */

/* {{{ igbinary_serialize_null */
/** Serializes null. */
inline static void igbinary_serialize_null(struct igbinary_serialize_data *igsd) {
	igbinary_serialize8(igsd, igbinary_type_null);
}
/* }}} */

/* {{{ igbinary_serialize_variant */
/** Serializes a variant */
inline static void igbinary_serialize_variant(struct igbinary_serialize_data *igsd, const Variant& self) {
    /* Same as igbinary7 igbinary_serialize_zval */
    /* TODO: Figure out how to handle references and garbage collection */
    auto tv = self.asTypedValue();

	switch (tv->m_type) {
		case KindOfUninit:
		case KindOfNull:
            igbinary_serialize_null(igsd);
            return;
        default:
            throw Exception("Not implemented yet for DataType %d", (int) tv->m_type);
    }
}
} // namespace

namespace HPHP {
String igbinary_serialize(const Variant& variant) {
    struct igbinary_serialize_data igsd;
    igbinary_serialize_data_init(&igsd, !variant.isObject() && !variant.isArray());
    igbinary_serialize_header(&igsd);
    igbinary_serialize_variant(&igsd, variant);  // Succeed or throw
    return igsd.buffer.detach();
}
} // HPHP
