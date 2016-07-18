/**
 * Function for unserializing. This contains implementation details of  igbinary_unserialize()
 */

#include "ext_igbinary.hpp"

using namespace HPHP;

#define WANT_CLEAR     (0)
#define WANT_OBJECT    (1<<0)
#define WANT_REF       (1<<1)

namespace {

/* {{{ data types */
/** String/len pair for the igbinary_unserializer_data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 * @see igbinary_unserialize_data.
 */
struct igbinary_unserialize_string_pair {
	char *data;		/**< Data. */
	size_t len;		/**< Data length. */
};

/** Unserializer data.
 * Uses RAII to ensure it is de-initialized.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_unserialize_data {
	const uint8_t *buffer;			/**< Buffer. */
	const size_t buffer_size;		/**< Buffer size. */
	size_t buffer_offset;			/**< Current read offset. */

	struct igbinary_unserialize_string_pair *strings; /**< Unserialized strings. */
	size_t strings_count;			/**< Unserialized string count. */
	size_t strings_capacity;		/**< Unserialized string array capacity. */

	// zval **references;				/**< Unserialized Arrays/Objects. */
	size_t references_count;		/**< Unserialized array/objects count. */
	size_t references_capacity;		/**< Unserialized array/object array capacity. */

	int error;						/**< Error number. Not used. */
  public:
	igbinary_unserialize_data(const uint8_t* buf, size_t buf_size);
	~igbinary_unserialize_data();
};
igbinary_unserialize_data::igbinary_unserialize_data(const uint8_t* buf, size_t buf_size) : buffer(buf), buffer_size(buf_size), buffer_offset(0) {
	strings_count = 0;
	strings_capacity = 4;
	strings = (struct igbinary_unserialize_string_pair *) malloc(sizeof(struct igbinary_unserialize_string_pair) * strings_capacity);

	references_count = 0;
	references_capacity = 4;
	// FIXME
	//references = malloc(sizeof(references[0]) * igsd->references_capacity);
}

igbinary_unserialize_data::~igbinary_unserialize_data() {
	free(strings);
	//free(references);
}

/* }}} */

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

/* Unserialize a variant. Same as igbinary7 igbinary_unserialize_zval */
static void igbinary_unserialize_variant(igbinary_unserialize_data *igsd, Variant& v, int flags) {
	enum igbinary_type t;

	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw Exception("igbinary_unserialize_variant: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);
	switch (t) {
		case igbinary_type_string_empty:
			{
				/* TODO: From const StaticString? */
				String s;
				tvMove(make_tv<KindOfString>(s.detach()), *v.asTypedValue());
			}
			return;
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
			throw Exception("TODO implement igbinary_type %x", (int) t);
	}
}
} // namespace

namespace HPHP {

/** Unserialize the data, or clean up and throw an Exception. Effectively constant, unless __sleep modifies something. */
void igbinary_unserialize(const uint8_t *buf, size_t buf_len, Variant& v) {
	igbinary_unserialize_data igsd(buf, buf_len);  // initialized by constructor, freed by destructor

	printf("buf_len=%d\n", (int)buf_len);
	igbinary_unserialize_header(&igsd);  // Unserialize header or throw exception.
	igbinary_unserialize_variant(&igsd, v, WANT_CLEAR);
	/* FIXME finish_wakeup */
}

} // namespace HPHP
