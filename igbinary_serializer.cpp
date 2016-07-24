/*
  +----------------------------------------------------------------------+
  | See COPYING file for further copyright information                   |
  +----------------------------------------------------------------------+
  | Author of hhvm fork: Tyson Andre <tysonandre775@hotmail.com>         |
  | Author of original igbinary: Oleg Grenrus <oleg.grenrus@dynamoid.com>|
  | See CREDITS for contributors                                         |
  +----------------------------------------------------------------------+
*/

#include <stddef.h>
#include <stdint.h>

#include "ext_igbinary.hpp"
#include "hash_ptr.hpp"

#include "hphp/runtime/base/array-iterator.h"
#include "hphp/runtime/base/string-buffer.h"
#include "hphp/runtime/base/type-string.h"


using namespace HPHP;

namespace{

const StaticString
	s_serialize("serialize");

inline static void igbinary_serialize_variant(struct igbinary_serialize_data *igsd, const Variant& self);
inline static int igbinary_serialize_array_ref_by_key(struct igbinary_serialize_data *igsd, const uintptr_t key, bool object);

typedef hphp_hash_map<const StringData*, uint32_t, string_data_hash, string_data_same> StringIdMap;

/** Serializer data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_serialize_data {
	StringBuffer buffer;
	bool scalar;				/**< Serializing scalar. */
	bool compact_strings;		/**< Check for duplicate strings. */
	StringIdMap strings;		/**< Hash of already serialized strings. */
	struct hash_si_ptr references;	/**< Hash of already serialized potential references. (non-NULL uintptr_t => int32_t) */
	int references_id;			/**< Number of things that the unserializer might think are references. >= length of references */
};

inline static int igbinary_serialize_array_ref(struct igbinary_serialize_data *igsd, const Variant& self, bool object);

/* {{{ igbinary_serialize_data_init */
/** Inits igbinary_serialize_data. */
inline static int igbinary_serialize_data_init(struct igbinary_serialize_data *igsd, bool scalar) {
	int r = 0;

	igsd->buffer.setOutputLimit(StringData::MaxSize);

	igsd->scalar = scalar;  // TODO use?
	if (!igsd->scalar) {
		igsd->strings.reserve(16);
		hash_si_ptr_init(&igsd->references, 16);
		igsd->references_id = 0;
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
/* {{{ igbinary_serialize_append_bytes */
inline static void igbinary_serialize_append_bytes(struct igbinary_serialize_data *igsd, const char* data, const size_t len) {
	StringBuffer& buf = igsd->buffer;
	char* const bytes = buf.appendCursor(len);
	memcpy(bytes, data, len);
	buf.resize(buf.size() + len);
}

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

	igbinary_serialize_append_bytes(igsd, string->data(), len);

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

	if (igsd->scalar || !igsd->compact_strings) {
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
inline static void igbinary_serialize_array(struct igbinary_serialize_data *igsd, const Variant& self, bool object) {
	auto tv = self.asTypedValue();
	const ArrayData* arr;

	switch (tv->m_type) {
		case KindOfPersistentArray:
		case KindOfArray:
			arr = tv->m_data.parr;
			break;
		case KindOfRef:
			{
				Variant& self_deref = *tv->m_data.pref->var();
				auto tv_deref = self_deref.asTypedValue();

				switch (tv_deref->m_type) {
					case KindOfPersistentArray:
					case KindOfArray:
						arr = tv_deref->m_data.parr;
						break;
					default:
						throw Exception("igbinary_serialize_array: Did not expect to get ref to DataType 0x%x", (int) tv->m_type);
				}
			}
			break;
		default:
			throw Exception("igbinary_serialize_array: Did not expect to get DataType 0x%x", (int) tv->m_type);
	}
	size_t n = arr->size();

	if (!object && igbinary_serialize_array_ref(igsd, self, false) == 0) {
		return;
	}

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
/* {{{ igbinary_serialize_object_name */
/** Serialize object name. */
inline static void igbinary_serialize_object_name(struct igbinary_serialize_data *igsd, const StringData* class_name) {
	// TODO: optimize
	const auto result = igsd->strings.insert(std::pair<const StringData*, uint32_t>(class_name, (uint32_t)igsd->strings.size()));
	if (result.second) {  // First time the class name was used as a string.
		auto name_len = igsd->strings.size();
		if (name_len <= 0xff) {
			igbinary_serialize8(igsd, (uint8_t) igbinary_type_object8);
			igbinary_serialize8(igsd, (uint8_t) name_len TSRMLS_CC);
		} else if (name_len <= 0xffff) {
			igbinary_serialize8(igsd, (uint8_t) igbinary_type_object16 TSRMLS_CC);
			igbinary_serialize16(igsd, (uint16_t) name_len TSRMLS_CC);
		} else {
			igbinary_serialize8(igsd, (uint8_t) igbinary_type_object32 TSRMLS_CC);
			igbinary_serialize32(igsd, (uint32_t) name_len TSRMLS_CC);
		}
		igbinary_serialize_append_bytes(igsd, class_name->data(), class_name->size());
		return;
	}
	const uint32_t t = result.first->second;
	/* already serialized string */
	if (t <= 0xff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_id8);
		igbinary_serialize8(igsd, (uint8_t) t);
	} else if (t <= 0xffff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_id16);
		igbinary_serialize16(igsd, (uint16_t) t);
	} else {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_id32 TSRMLS_CC);
		igbinary_serialize32(igsd, (uint32_t) t);
	}
}
/* }}} */
/* {{{ igbinary_serialize_object_serialize_data */
inline static void igbinary_serialize_object_serialize_data(struct igbinary_serialize_data* igsd, const StrNR& classname, const String& serializedData) {
	igbinary_serialize_object_name(igsd, classname.get());
	const size_t serialized_len = serializedData.length();
	if (serialized_len <= 0xff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_ser8);
		igbinary_serialize8(igsd, (uint8_t) serialized_len TSRMLS_CC);
	} else if (serialized_len <= 0xffff) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_ser16);
		igbinary_serialize16(igsd, (uint16_t) serialized_len TSRMLS_CC);
	} else if (LIKELY(serialized_len <= 0xffffffffL)) {
		igbinary_serialize8(igsd, (uint8_t) igbinary_type_object_ser32 TSRMLS_CC);
		igbinary_serialize32(igsd, (uint32_t) serialized_len TSRMLS_CC);
	} else {
		throw Exception("igbinary_serialize_object_serialize_data: Data is too long?");
	}

	igbinary_serialize_append_bytes(igsd, serializedData.data(), serialized_len);
}
/* }}} */
/* {{{ igbinary_serialize_object */
/** Serialize object.
 * @see ext/standard/var.c
 * */
inline static void igbinary_serialize_object(struct igbinary_serialize_data *igsd, const ObjectData* obj) {
	if (!obj) {
		igbinary_serialize_null(igsd);
		return;
	}

	const uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	if (igbinary_serialize_array_ref_by_key(igsd, key, true) == 0) {
		return;
	}

	if (obj->isCollection()) {
		throw Exception("igbinary_serialize_object: Unsupported type isCollection");
	}

	if (obj->instanceof(SystemLib::s_SerializableClass)) {
		assert(!obj->isCollection());
		Variant ret =
			const_cast<ObjectData*>(obj)->o_invoke_few_args(s_serialize, 0);
		if (ret.isString()) {
			igbinary_serialize_object_serialize_data(igsd, obj->getClassName(), ret.toString());
		} else if (ret.isNull()) {
			igbinary_serialize_null(igsd);
		} else {
			raise_error("%s::serialize() must return a string or NULL",
						obj->getClassName().data());
		}
		return;
	}

	bool handleSleep = false;
	auto cls = obj->getVMClass();

	// From serializeObject:
	// Only serialize CPP extension type instances which can actually
	// be deserialized.  Otherwise, raise a warning and serialize
	// null.
	// Similarly, do not try to serialize WaitHandles
	// as they contain internal state via non-NativeData means.
	//
	// FIXME - don't unserialize other internal objects unless a configure option is provided to allow it.
	if ((cls->instanceCtor() && !cls->isCppSerializable()) ||
			obj->getAttribute(ObjectData::IsWaitHandle)) {
		raise_warning("Attempted to serialize unserializable builtin class %s",
					  obj->getVMClass()->preClass()->name()->data());
		igbinary_serialize_null(igsd);
		return;
	}

	if (obj->getAttribute(ObjectData::HasNativeData)) {
		throw Exception("Can't serialize object of class %s with native data?", obj->getClassName().data());
	}
	Variant ret;
	if (obj->getAttribute(ObjectData::HasSleep)) {
		throw Exception("TODO: Handle __sleep (serializing object of class %s)", obj->getClassName().data());
		handleSleep = true;
		ret = const_cast<ObjectData*>(obj)->invokeSleep();
	}

	if (handleSleep) {
		throw Exception("igbinary_serialize_object: TODO: Handle __sleep (serializing object of class %s)", obj->getClassName().data());
		// TODO Equivalent of r = igbinary_serialize_array_sleep(igsd, z, HASH_OF(&h), ce, incomplete_class);
		// return;
	}

	throw Exception("igbinary_serialize_object: TODO: Handle serializing objects");
}
/* }}} */
/* {{{ igbinary_serialize_array_ref_by_key */
inline static int igbinary_serialize_array_ref_by_key(struct igbinary_serialize_data *igsd, const uintptr_t key, bool object) {
	// Serialize it by a key.
	uint32_t t = 0;
	uint32_t *i = &t;

	if (hash_si_ptr_find(&igsd->references, key, i) == 1) {
		t = igsd->references_id++;
		/* FIXME hack? If the top-level element was an array, we assume that it can't be a reference when we serialize it, */
		/* because that's the way it was serialized in php5. */
		/* Does this work with different forms of recursive arrays? */
		if (t > 0 || object) {
			hash_si_ptr_insert(&igsd->references, key, t);  /* TODO: Add a specialization for fixed-length numeric keys? */
		}
		return 1;
	} else {
		enum igbinary_type type;
		if (*i <= 0xff) {
			type = object ? igbinary_type_objref8 : igbinary_type_ref8;
			igbinary_serialize8(igsd, (uint8_t) type);
			igbinary_serialize8(igsd, (uint8_t) *i);
		} else if (*i <= 0xffff) {
			type = object ? igbinary_type_objref16 : igbinary_type_ref16;
			igbinary_serialize8(igsd, (uint8_t) type);
			igbinary_serialize16(igsd, (uint16_t) *i);
		} else {
			type = object ? igbinary_type_objref32 : igbinary_type_ref32;
			igbinary_serialize8(igsd, (uint8_t) type);
			igbinary_serialize32(igsd, (uint32_t) *i);
		}

		return 0;
	}

	return 1;
}

/* }}} */
/* {{{ igbinary_serialize_array_ref */
/** Serializes array reference (or reference in an object). Returns 0 on success. */
inline static int igbinary_serialize_array_ref(struct igbinary_serialize_data *igsd, const Variant& self, bool object) {
	auto tv = self.asTypedValue();
	uintptr_t key = 0;  /* The address of the pointer to the zend_refcounted struct or other struct */

	// Aside: it's a union, so these are all equivalent.
	switch (tv->m_type) {
		case KindOfPersistentArray:
		case KindOfArray:
			key = reinterpret_cast<uintptr_t>(tv->m_data.parr);
			break;
		case KindOfObject:
			// TODO: Does this need to use o_id? Probably not.
			key = reinterpret_cast<uintptr_t>(tv->m_data.pobj);
			break;
		case KindOfRef:
			{
				RefData* pref = tv->m_data.pref;
				Variant& self_deref = *pref->var();
				auto tv_deref = self_deref.asTypedValue();
				if (tv_deref->m_type == KindOfObject) {
					// For objects, give the references and non-references the same id. Dereference it.
					key = reinterpret_cast<uintptr_t>(tv_deref->m_data.pobj);
				} else {
					key = reinterpret_cast<uintptr_t>(tv->m_data.pref);
				}
			}
			// TODO ref to objects, check bool object
			break;
		default:
			throw Exception("igbinary_serialize_array_ref expected object, array, or reference, got none of those : DataType=0x%02x", (int) tv->m_type);
	}
	return igbinary_serialize_array_ref_by_key(igsd, key, object);
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
		case KindOfResource:
			igbinary_serialize_null(igsd);  // Can't serialize resources.
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
		case KindOfObject:
			igbinary_serialize_object(igsd, tv->m_data.pobj);
			return;
		case KindOfPersistentArray:
		case KindOfArray:
			igbinary_serialize_array(igsd, self, false);
			return;
		case KindOfRef:
			{
				Variant& self_deref = *tv->m_data.pref->var();
				if (self_deref.isArray()) {
					igbinary_serialize_array(igsd, self, false);
					return;
				} else if (self_deref.isObject()) {
					// Nothing else to do, already recorded that it was an object and a reference.
				} else {
					if (igbinary_serialize_array_ref(igsd, self, false) == 0) {
						return;  // it was already serialized, and we printed the reference.
					}
				}
				igbinary_serialize_variant(igsd, self_deref);
				return;
			}
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
