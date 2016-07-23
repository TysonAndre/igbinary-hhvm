#include <stddef.h>
#include <stdint.h>

#include "ext_igbinary.hpp"

#include "hphp/runtime/base/array-iterator.h"
#include "hphp/runtime/base/string-buffer.h"
#include "hphp/runtime/base/type-string.h"


using namespace HPHP;

namespace{

inline static void igbinary_serialize_variant(struct igbinary_serialize_data *igsd, const Variant& self);

typedef hphp_hash_map<const StringData*, uint32_t, string_data_hash, string_data_same> StringIdMap;
/** Serializer data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_serialize_data {
	StringBuffer buffer;
	bool scalar;				/**< Serializing scalar. */
	bool compact_strings;		/**< Check for duplicate strings. */
	StringIdMap strings;		/**< Hash of already serialized strings. */
	// struct hash_si_ptr references;	/**< Hash of already serialized potential references. (non-NULL uintptr_t => int32_t) */
	// int references_id;			/**< Number of things that the unserializer might think are references. >= length of references */
};

/* {{{ igbinary_serialize_data_init */
/** Inits igbinary_serialize_data. */
inline static int igbinary_serialize_data_init(struct igbinary_serialize_data *igsd, bool scalar) {
	int r = 0;

	igsd->buffer.setOutputLimit(StringData::MaxSize);

	igsd->scalar = scalar;  // TODO use?
	if (!igsd->scalar) {
		igsd->strings.reserve(16);
		// hash_si_ptr_init(&igsd->references, 16);
		// igsd->references_id = 0;
	}

	igsd->compact_strings = true; /* FIXME allow ini options parsing */

	return r;
}

/* {{{ igbinary_serialize8 */
/** Serialize 8bit value. */
inline static void igbinary_serialize8(struct igbinary_serialize_data *igsd, uint8_t i) {
	igsd->buffer.append((char)i);
}
/* }}} */
/* {{{ igbinary_serialize16 */
/** Serialize 16bit value. */
inline static void igbinary_serialize16(struct igbinary_serialize_data *igsd, uint16_t i) {
	StringBuffer& buf = igsd->buffer;
	char* const bytes = buf.appendCursor(2);

	bytes[0] = (char) ((i >> 8) & 0xff);
	bytes[1] = (char) (i & 0xff);
	buf.resize(buf.size() + 2);
}
/* }}} */
/* {{{ igbinary_serialize32 */
/** Serialize 32bit value. */
inline static void igbinary_serialize32(struct igbinary_serialize_data *igsd, uint32_t i) {
	StringBuffer& buf = igsd->buffer;
	char* const bytes = buf.appendCursor(4);

	bytes[0] = (char) ((i >> 24) & 0xff);
	bytes[1] = (char) ((i >> 16) & 0xff);
	bytes[2] = (char) ((i >> 8) & 0xff);
	bytes[3] = (char) (i & 0xff);
	buf.resize(buf.size() + 4);
}
/* }}} */
/* {{{ igbinary_serialize64 */
/** Serialize 64bit value. */
inline static int igbinary_serialize64(struct igbinary_serialize_data *igsd, uint64_t i) {
	StringBuffer& buf = igsd->buffer;
	char* const bytes = buf.appendCursor(8);

	bytes[0] = (char) ((i >> 56) & 0xff);
	bytes[1] = (char) ((i >> 48) & 0xff);
	bytes[2] = (char) ((i >> 40) & 0xff);
	bytes[3] = (char) ((i >> 32) & 0xff);
	bytes[4] = (char) ((i >> 24) & 0xff);
	bytes[5] = (char) ((i >> 16) & 0xff);
	bytes[6] = (char) ((i >> 8) & 0xff);
	bytes[7] = (char) (i & 0xff);
	buf.resize(buf.size() + 8);

	return 0;
}
/* }}} */


/* {{{ igbinary_serialize_header */
/** Serializes header. */
inline static void igbinary_serialize_header(struct igbinary_serialize_data *igsd) {
	igbinary_serialize32(igsd, IGBINARY_FORMAT_VERSION); /* version */
}
/* }}} */

/* {{{ igbinary_serialize_null */
/** Serializes null. */
inline static void igbinary_serialize_null(struct igbinary_serialize_data *igsd) {
	igbinary_serialize8(igsd, igbinary_type_null);
}
/* }}} */
/** Serializes a boolean. */
inline static void igbinary_serialize_bool(struct igbinary_serialize_data *igsd, int b) {
	igbinary_serialize8(igsd, b ? igbinary_type_bool_true : igbinary_type_bool_false);
}
/* }}} */
/* {{{ igbinary_serialize_int64 */
/** Serializes 64-bit integer. */
inline static void igbinary_serialize_int64(struct igbinary_serialize_data *igsd, int64_t l) {
	uint64_t k = l >= 0 ? l : -l;
	bool p = l >= 0;

	/* -ZEND_LONG_MIN is 0 otherwise. */
	if (l == INT64_MIN) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_long64n);
		igbinary_serialize64(igsd, (uint64_t) 0x8000000000000000);
		return;
	}

	if (k <= 0xff) {
		igbinary_serialize8(igsd, (uint8_t) (p ? igbinary_type_long8p : igbinary_type_long8n));
		igbinary_serialize8(igsd, (uint8_t) k);
	} else if (k <= 0xffff) {
		igbinary_serialize8(igsd, (uint8_t) (p ? igbinary_type_long16p : igbinary_type_long16n));
		igbinary_serialize16(igsd, (uint16_t) k);
	} else if (k <= 0xffffffff) {
		igbinary_serialize8(igsd, (uint8_t) (p ? igbinary_type_long32p : igbinary_type_long32n));
		igbinary_serialize32(igsd, (uint32_t) k);
	} else {
		igbinary_serialize8(igsd, (uint8_t) (p ? igbinary_type_long64p : igbinary_type_long64n));
		igbinary_serialize64(igsd, (uint64_t) k);
	}
}
/* }}} */
/* {{{ igbinary_serialize_double */
/** Serializes double. */
inline static void igbinary_serialize_double(struct igbinary_serialize_data *igsd, double d) {
	union {
		double d;
		uint64_t u;
	} u;
	igbinary_serialize8(igsd, igbinary_type_double);
	u.d = d;
	igbinary_serialize64(igsd, u.u);
}
/* }}} */
/* {{{ igbinary_serialize_chararray */
/** Serializes string data. */
inline static int igbinary_serialize_chararray(struct igbinary_serialize_data *igsd, const StringData* string) {
	const size_t len = string->size();
	if (len <= 0xff) {
		igbinary_serialize8(igsd, igbinary_type_string8);
		igbinary_serialize8(igsd, len);
	} else if (len <= 0xffff) {
		igbinary_serialize8(igsd, igbinary_type_string16);
		igbinary_serialize16(igsd, len);
	} else if (LIKELY(len <= 0xffffffff)) {
		igbinary_serialize8(igsd, igbinary_type_string32);
		igbinary_serialize32(igsd, len);
	} else {
		throw Exception("igbinary_serialize_chararray: Too long for other igbinary v2 implementations to parse");
	}

	StringBuffer& buf = igsd->buffer;
	char* const bytes = buf.appendCursor(len);
	memcpy(bytes, string->data(), len);
	buf.resize(buf.size() + len);

	return 0;
}
/* }}} */
/* {{{ igbinary_serialize_string */
/** Serializes string.
 * Serializes each string once, after first time uses pointers.
 */
inline static void igbinary_serialize_string(struct igbinary_serialize_data *igsd, const StringData* string) {

	if (string->size() == 0) {
		igbinary_serialize8(igsd, igbinary_type_string_empty);
		return;
	}

	if (igsd->scalar || !igsd->compact_strings || true) {
		igbinary_serialize_chararray(igsd, string);
		return;
	}
	auto result = igsd->strings.insert(std::pair<const StringData*, uint32_t>(string, (uint32_t)igsd->strings.size()));
	if (result.second) {
		igbinary_serialize_chararray(igsd, string);
		return;
	}
	uint32_t t = result.first->second;  // old value.
	if (t <= 0xff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_string_id8);
		igbinary_serialize8(igsd, (uint8_t) t);
	} else if (t <= 0xffff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_string_id16);
		igbinary_serialize16(igsd, (uint16_t) t);
	} else {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_string_id32);
		igbinary_serialize32(igsd, (uint32_t) t);
	}
}
/* }}} */

/* {{{ igbinary_serialize_array_key */
/** Serializes an array/object key */
inline static void igbinary_serialize_array_key(struct igbinary_serialize_data *igsd, const Variant& self) {
	/* Same as igbinary7 igbinary_serialize_zval */
	/* TODO: Figure out how to handle references and garbage collection */
	auto tv = self.asTypedValue();

	switch (tv->m_type) {
		case KindOfInt64:
			igbinary_serialize_int64(igsd, tv->m_data.num);
			return;
		case KindOfString:
		case KindOfPersistentString:
			igbinary_serialize_string(igsd, tv->m_data.pstr);
			return;
		default:
			throw Exception("igbinary_serialize_array_key: Did not expect to get DataType 0x%x", (int) tv->m_type);
	}
}
/* }}} */

/* {{{ igbinay_serialize_array */
/** Serializes array or (TODO?) objects inner properties */
inline static void igbinary_serialize_array(struct igbinary_serialize_data *igsd, const ArrayData *arr, bool object) {
	size_t n = arr->size();

	// TODO // igbinary_serialize_array_ref(igsd, z_original, false
	// TODO: Support refs.

	if (n <= 0xff) {
		igbinary_serialize8(igsd, igbinary_type_array8);
		igbinary_serialize8(igsd, n);
	} else if (n <= 0xffff) {
		igbinary_serialize8(igsd, igbinary_type_array16);
		igbinary_serialize16(igsd, n);
	} else {
		igbinary_serialize8(igsd, igbinary_type_array8);
		igbinary_serialize32(igsd, n);
	}

	if (n == 0) {
		return;
	}
	if (arr->isVecArray()) {
		// it wouldn't be unserializable by php5 and php7.
		// Maybe igbinary_keyset, igbinary_vecarray for those, or SPL....
		throw new Exception("igbinary_serialize_array: Unable to handle case of isVecArray");
#if HHVM_VERSION_MAJOR > 3 || (HHVM_VERSION_MAJOR >= 3 && HHVM_VERSION_MINOR >= 15)
	} else if (arr->isKeyset()) {
		// TODO find exact patch version(s)?
		// it wouldn't be unserializable by php5 and php7.
		// Maybe igbinary_keyset, igbinary_vecarray for those, or SPL....
		throw new Exception("igbinary_serialize_array: Unable to handle case of isKeyset");

#endif
	} else {
		for (ArrayIter iter(arr); iter; ++iter) {
			// FIXME check if int or string?
			igbinary_serialize_array_key(igsd, iter.first());
			igbinary_serialize_variant(igsd, iter.secondRef());
		}
	}
}
/* }}} */

/* {{{ igbinary_serialize_variant */
/** Serializes a variant. Guaranteed not to be an array/object key. */
inline static void igbinary_serialize_variant(struct igbinary_serialize_data *igsd, const Variant& self) {
	/* Same as igbinary7 igbinary_serialize_zval */
	/* TODO: Figure out how to handle references and garbage collection */
	auto tv = self.asTypedValue();

	switch (tv->m_type) {
		case KindOfUninit:
		case KindOfNull:
			igbinary_serialize_null(igsd);
			return;
		case KindOfBoolean:
			igbinary_serialize_bool(igsd, tv->m_data.num != 0);
			return;
		case KindOfInt64:
			igbinary_serialize_int64(igsd, tv->m_data.num);
			return;
		case KindOfDouble:
			igbinary_serialize_double(igsd, tv->m_data.dbl);
			return;
		case KindOfString:
		case KindOfPersistentString:
			igbinary_serialize_string(igsd, tv->m_data.pstr);
			return;
		case KindOfPersistentArray:
		case KindOfArray:
			igbinary_serialize_array(igsd, tv->m_data.parr, false);
			return;
		default:
			throw Exception("igbinary_serialize_variant: Not implemented yet for DataType 0x%x", (int) tv->m_type);
	}
}
/* }}} */
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
