/*
  +----------------------------------------------------------------------+
  | See COPYING file for further copyright information                   |
  +----------------------------------------------------------------------+
  | Author of hhvm fork: Tyson Andre <tysonandre775@hotmail.com>         |
  | Author of original igbinary: Oleg Grenrus <oleg.grenrus@dynamoid.com>|
  | See CREDITS for contributors                                         |
  +----------------------------------------------------------------------+
*/

/**
 * Function for unserializing. This contains implementation details of  igbinary_unserialize()
 */

#include "ext_igbinary.hpp"

#include "hphp/runtime/base/array-init.h"

using namespace HPHP;

#define WANT_CLEAR	 (0)
#define WANT_OBJECT	(1<<0)
#define WANT_REF	   (1<<1)

namespace {

/* {{{ data types */

/** Unserializer data.
 * Uses RAII to ensure it is de-initialized.
 * Based on data structure by Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_unserialize_data {
	const uint8_t *buffer;			/**< Buffer. */
	const size_t buffer_size;		/**< Buffer size. */
	size_t buffer_offset;			/**< Current read offset. */

	std::vector<String> strings;	/**< Unserialized strings. */

	// zval **references;				/**< Unserialized Arrays/Objects. */
	size_t references_count;		/**< Unserialized array/objects count. */
	size_t references_capacity;		/**< Unserialized array/object array capacity. */

	int error;						/**< Error number. Not used. */
  public:
	igbinary_unserialize_data(const uint8_t* buf, size_t buf_size);
	~igbinary_unserialize_data();
};
igbinary_unserialize_data::igbinary_unserialize_data(const uint8_t* buf, size_t buf_size) : buffer(buf), buffer_size(buf_size), buffer_offset(0), strings(0) {
	references_count = 0;
	references_capacity = 4;
	// FIXME
	//references = malloc(sizeof(references[0]) * igsd->references_capacity);
}

igbinary_unserialize_data::~igbinary_unserialize_data() {
	// Nothing to do for strings.
	//free(references);
}

/* }}} */

static void igbinary_unserialize_variant(igbinary_unserialize_data *igsd, Variant& v, int flags);

/* {{{ Unserializing functions prototypes */
/*
inline static void igbinary_unserialize_data_init(struct igbinary_unserialize_data *igsd);
inline static void igbinary_unserialize_data_deinit(struct igbinary_unserialize_data *igsd);

inline static void igbinary_unserialize_header(struct igbinary_unserialize_data *igsd);

inline static uint8_t igbinary_unserialize8(struct igbinary_unserialize_data *igsd);
inline static uint16_t igbinary_unserialize16(struct igbinary_unserialize_data *igsd);
inline static uint32_t igbinary_unserialize32(struct igbinary_unserialize_data *igsd);
inline static uint64_t igbinary_unserialize64(struct igbinary_unserialize_data *igsd);

inline static int64_t igbinary_unserialize_long(struct igbinary_unserialize_data *igsd, enum igbinary_type t);
inline static double igbinary_unserialize_double(struct igbinary_unserialize_data *igsd, enum igbinary_type t);
inline static String igbinary_unserialize_string(struct igbinary_unserialize_data *igsd, enum igbinary_type t);
inline static String igbinary_unserialize_chararray(struct igbinary_unserialize_data *igsd, enum igbinary_type t);

inline static void igbinary_unserialize_array(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Array& self, int flags);
inline static void igbinary_unserialize_object(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Object& self, int flags);
// inline static void igbinary_unserialize_object_ser(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Object& self, zend_class_entry *ce);
// inline static void igbinary_unserialize_ref(struct igbinary_unserialize_data *igsd, enum igbinary_type t, zval *const z, int flags);

static void igbinary_unserialize_variant(struct igbinary_unserialize_data *igsd, Variant& v, int flags);
*/
/* }}} */

/* {{{ igbinary_unserialize8 */
/** Unserialize 8bit value. */
inline static uint8_t igbinary_unserialize8(igbinary_unserialize_data *igsd) {
	uint8_t ret = 0;
	ret = igsd->buffer[igsd->buffer_offset++];
	return ret;
}
/* }}} */
/* {{{ igbinary_unserialize16 */
/** Unserialize 16bit value. */
inline static uint16_t igbinary_unserialize16(igbinary_unserialize_data *igsd) {
	uint16_t ret = 0;
	ret |= ((uint16_t) igsd->buffer[igsd->buffer_offset++] << 8);
	ret |= ((uint16_t) igsd->buffer[igsd->buffer_offset++] << 0);
	return ret;
}
/* }}} */
/* {{{ igbinary_unserialize32 */
/** Unserialize 32bit value. */
inline static uint32_t igbinary_unserialize32(igbinary_unserialize_data *igsd) {
	uint32_t ret = 0;
	ret |= ((uint32_t) igsd->buffer[igsd->buffer_offset++] << 24);
	ret |= ((uint32_t) igsd->buffer[igsd->buffer_offset++] << 16);
	ret |= ((uint32_t) igsd->buffer[igsd->buffer_offset++] << 8);
	ret |= ((uint32_t) igsd->buffer[igsd->buffer_offset++] << 0);
	return ret;
}
/* }}} */
/* {{{ igbinary_unserialize64 */
/** Unserialize 64bit value. */
inline static uint64_t igbinary_unserialize64(igbinary_unserialize_data *igsd) {
	uint64_t ret = 0;
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 56);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 48);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 40);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 32);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 24);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 16);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 8);
	ret |= ((uint64_t) igsd->buffer[igsd->buffer_offset++] << 0);
	return ret;
}
/* }}} */

/* {{{ igbinary_unserialize_header */
/** Unserialize header. Check for version. */
inline static void igbinary_unserialize_header(struct igbinary_unserialize_data *igsd) {
	uint32_t version;

	if (igsd->buffer_offset + 4 >= igsd->buffer_size) {
		throw Exception("igbinary_unserialize_data: unexpected end of buffer reading version %d > %d", (int) igsd->buffer_offset, (int) igsd->buffer_size);
	}

	version = igbinary_unserialize32(igsd);

	/* Support older version 1 and the current format 2 */
	if (version == IGBINARY_FORMAT_VERSION || version == 0x00000001) {
		return;
	} else {
		throw Exception("igbinary_unserialize_header: unsupported version: %d, should be %d or %u", (int) version, 0x00000001, (int) IGBINARY_FORMAT_VERSION);
	}
}
/* }}} */
/* {{{ igbinary_unserialize_long */
/** Unserializes zend_long */
inline static int igbinary_unserialize_long(struct igbinary_unserialize_data *igsd, enum igbinary_type t) {
	if (t == igbinary_type_long8p || t == igbinary_type_long8n) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_long: end-of-data");
		}

		return (t == igbinary_type_long8n ? -1 : 1) * igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_long16p || t == igbinary_type_long16n) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_long: end-of-data");
		}

		return (t == igbinary_type_long16n ? -1 : 1) * igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_long32p || t == igbinary_type_long32n) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_long: end-of-data");
		}

		/* check for boundaries */
		return (t == igbinary_type_long32n ? -1 : 1) * igbinary_unserialize32(igsd);
	} else if (t == igbinary_type_long64p || t == igbinary_type_long64n) {
		if (igsd->buffer_offset + 8 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_long: end-of-data");
		}

		/* check for boundaries */
		uint64_t tmp64 = igbinary_unserialize64(igsd);
		if (tmp64 > 0x8000000000000000 || (tmp64 == 0x8000000000000000 && t == igbinary_type_long64p)) {
			throw Exception("igbinary_unserialize_long: too big 64bit long.");
		}

		return (t == igbinary_type_long64n ? -1 : 1) * tmp64;
	} else {
		// FIXME is format string correct? Not reachable.
		throw Exception("igbinary_unserialize_long: unknown type '%02x', position %ld", (unsigned char)t, igsd->buffer_offset);
	}

	return 0;
}
/* }}} */
/* {{{ igbinary_unserialize_double */
/** Unserializes double. */
inline static double igbinary_unserialize_double(struct igbinary_unserialize_data *igsd) {
	union {
		double d;
		uint64_t u;
	} u;

	if (igsd->buffer_offset + 8 > igsd->buffer_size) {
		throw Exception("igbinary_unserialize_double: end-of-data");
	}

	u.u = igbinary_unserialize64(igsd);

	return u.d;
}
/* }}} */
/* {{{ igbinary_unserialize_chararray */
/** Unserializes chararray of string. */
inline static const String& igbinary_unserialize_chararray(struct igbinary_unserialize_data *igsd, const enum igbinary_type t) {
	size_t l;

	if (t == igbinary_type_string8 || t == igbinary_type_object8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_string16 || t == igbinary_type_object16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize16(igsd);
		if (igsd->buffer_offset + l > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_chararray: end-of-data");
		}
	} else if (t == igbinary_type_string32 || t == igbinary_type_object32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize32(igsd);
		if (igsd->buffer_offset + l > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_chararray: end-of-data");
		}
	} else {
		throw Exception("igbinary_unserialize_chararray: unknown type '0x%x', position %ld", (int)t, (int64_t)igsd->buffer_offset);
	}
	if (igsd->buffer_offset + l > igsd->buffer_size) {
		throw Exception("igbinary_unserialize_chararray: end-of-data");
	}


	/** TODO : Optimize after implementing it the simple way and testing.. */
	// Make a copy of every occurence of the string.
	igsd->strings.emplace_back(reinterpret_cast<const char*>(igsd->buffer + igsd->buffer_offset), l, CopyString);

	return igsd->strings.back();
}
/* }}} */
/* {{{ igbinary_unserialize_string */
/** Unserializes string. Unserializes both actual string or by string id. */
inline static const String& igbinary_unserialize_string(struct igbinary_unserialize_data *igsd, enum igbinary_type t) {
	size_t i;
	if (t == igbinary_type_string_id8 || t == igbinary_type_object_id8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_string_id16 || t == igbinary_type_object_id16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_string_id32 || t == igbinary_type_object_id32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize32(igsd);
	} else {
		throw Exception("igbinary_unserialize_string: unknown type '0x%x', position %ld", (int)t, (uint64_t)igsd->buffer_offset);
	}

	if (i >= igsd->strings.size()) {
		throw Exception("igbinary_unserialize_string: string index is out-of-bounds");
	}

	return igsd->strings[i];
}
/* }}} */
/* Unserialize an array key (int64 or string). Postcondition: v.isInteger() || v.isString(), or Exception thrown. See igbinary_unserialize_variant. */
static void igbinary_unserialize_array_key(igbinary_unserialize_data *igsd, Variant& v) {
	enum igbinary_type t;

	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw Exception("igbinary_unserialize_variant: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);
	switch (t) {
		case igbinary_type_string_empty:
			{
				String s = "";
				// TODO: Make a constant?
				tvMove(make_tv<KindOfString>(s.detach()), *v.asTypedValue());
			}
			break;
		case igbinary_type_long8p:
		case igbinary_type_long8n:
		case igbinary_type_long16p:
		case igbinary_type_long16n:
		case igbinary_type_long32p:
		case igbinary_type_long32n:
		case igbinary_type_long64p:
		case igbinary_type_long64n:
			v = igbinary_unserialize_long(igsd, t);
			break;
		case igbinary_type_string8:
		case igbinary_type_string16:
		case igbinary_type_string32:
			v = igbinary_unserialize_chararray(igsd, t);
			break;
		case igbinary_type_string_id8:
		case igbinary_type_string_id16:
		case igbinary_type_string_id32:
			v = igbinary_unserialize_string(igsd, t);
			break;
		default:
			throw Exception("igbinary_unserialize_array_key: Unexpected igbinary_type 0x%02x", (int) t);
	}
}
/* {{{ igbinary_unserialize_array */
/** Unserializes array. */
inline static Array igbinary_unserialize_array(struct igbinary_unserialize_data *igsd, enum igbinary_type t, int flags) {
	/* WANT_OBJECT means that z will be an object (if dereferenced) - TODO implement or refactor. */
	/* WANT_REF means that z will be wrapped by an IS_REFERENCE */
	size_t n;
	if (t == igbinary_type_array8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_array16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_array32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw Exception("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize32(igsd);
	} else {
		throw Exception("igbinary_unserialize_array: unknown type 0x%02x, position %ld", t, igsd->buffer_offset);
	}

	/* n cannot be larger than the number of minimum "objects" in the array */
	if (n > igsd->buffer_size - igsd->buffer_offset) {
        throw Exception("igbinary_unserialize_array: data size %llu smaller that requested array length %llu.", (long long)(igsd->buffer_size - igsd->buffer_offset), (long long) n);
	}

	if (flags & WANT_REF) {
        throw Exception("igbinary_unserialize_array: references not implemented yet");
	}
	if ((flags & WANT_OBJECT) != 0) {
        // TODO remove
        throw Exception("igbinary_unserialize_array: objects not implemented yet");
	}

	/* empty array */
	if (n == 0) {
		return Array::Create();  // static empty array.
	}

    Array arr = ArrayInit(n, ArrayInit::Mixed{}).toArray();

	for (size_t i = 0; i < n; i++) {
        Variant key;
        igbinary_unserialize_array_key(igsd, key);
        // Postcondition: key.isString() || key.isInteger()

        Variant& value = arr.lvalAt(key, AccessFlags::Key);
        // FIXME: handle case of value already existing? (analogous to putInOverwrittenList)

        // TODO: Any other code for references
        igbinary_unserialize_variant(igsd, value, WANT_CLEAR);
	}
    return arr;
}
/* }}} */

/* Unserialize a variant. Same as igbinary7 igbinary_unserialize_zval */
static void igbinary_unserialize_variant(igbinary_unserialize_data *igsd, Variant& v, int flags) {
	enum igbinary_type t;

	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw Exception("igbinary_unserialize_variant: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);
	switch (t) {
		case igbinary_type_array8:
		case igbinary_type_array16:
		case igbinary_type_array32:
            {
                auto a = igbinary_unserialize_array(igsd, t, flags);
                tvMove(make_tv<KindOfArray>(a.detach()), *v.asTypedValue());
            }
			break;
		case igbinary_type_string_empty:
			{
				String s = "";
				// TODO: Make a constant?
				tvMove(make_tv<KindOfString>(s.detach()), *v.asTypedValue());
			}
			return;
		case igbinary_type_long8p:
		case igbinary_type_long8n:
		case igbinary_type_long16p:
		case igbinary_type_long16n:
		case igbinary_type_long32p:
		case igbinary_type_long32n:
		case igbinary_type_long64p:
		case igbinary_type_long64n:
			v = igbinary_unserialize_long(igsd, t);
			break;
		case igbinary_type_string8:
		case igbinary_type_string16:
		case igbinary_type_string32:
			v = igbinary_unserialize_chararray(igsd, t);
			break;
		case igbinary_type_string_id8:
		case igbinary_type_string_id16:
		case igbinary_type_string_id32:
			v = igbinary_unserialize_string(igsd, t);
			break;
		case igbinary_type_double:
			v = igbinary_unserialize_double(igsd);
			break;
		case igbinary_type_null:
			v.setNull();
			return;
		case igbinary_type_bool_false:
			v = false;
			return;
		case igbinary_type_bool_true:
			v = true;
			return;
		default:
			throw Exception("TODO implement igbinary_unserialize_variant for igbinary_type 0x%02x", (int) t);
	}
}
} // namespace

namespace HPHP {

/** Unserialize the data, or clean up and throw an Exception. Effectively constant, unless __sleep modifies something. */
void igbinary_unserialize(const uint8_t *buf, size_t buf_len, Variant& v) {
	igbinary_unserialize_data igsd(buf, buf_len);  // initialized by constructor, freed by destructor

	igbinary_unserialize_header(&igsd);  // Unserialize header or throw exception.
	igbinary_unserialize_variant(&igsd, v, WANT_CLEAR);
	/* FIXME finish_wakeup */
}

} // namespace HPHP
