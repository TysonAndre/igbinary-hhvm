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

// For HHVM_VERSION_*
#include "hphp/runtime/version.h"

#if HHVM_VERSION_MAJOR < 3 || (HHVM_VERSION_MAJOR == 3 && HHVM_VERSION_MINOR < 22)
#error Unsupported HHVM version
#endif

#include "hphp/runtime/base/array-init.h"
// for ::HPHP::collections::isType
#include "hphp/runtime/base/collections.h"
#include "hphp/runtime/base/execution-context.h"
// for req::vector

#include "hphp/runtime/base/req-containers.h"
#include "hphp/runtime/base/type-variant.h"


using namespace HPHP;

// TODO: Get rid of WANT_REF, similar to igbinary 2.0.5 for Zend
#define WANT_CLEAR  (0)
#define WANT_REF    (1<<1)

namespace {

const StaticString
  s_unserialize("unserialize"),
  s_PHP_Incomplete_Class("__PHP_Incomplete_Class"),
  s_PHP_Incomplete_Class_Name("__PHP_Incomplete_Class_Name"),
  s___wakeup("__wakeup");

/* {{{ data types */

/** Unserializer data.
 * Uses RAII to ensure it is de-initialized.
 * Based on data structure by Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_unserialize_data {
	const uint8_t *buffer;			/**< Buffer. */
	const size_t buffer_size;		/**< Buffer size. */
	size_t buffer_offset;			/**< Current read offset. */

	// Containers using thread-local memory.
	req::vector<String> strings;	/**< Unserialized strings. */
	req::vector<Variant*> references;  /**< non-refcounted pointers to objects, arrays, and references being deserialized */
	req::vector<Object> wakeup;    /* objects for which to call __wakeup after unserialization is finished */

    Array m_overwrittenList;  /* Reference counted values that were overwritten. See base/variable-unserializer.cpp */
  public:
	igbinary_unserialize_data(const uint8_t* buf, size_t buf_size);
	~igbinary_unserialize_data();
};
igbinary_unserialize_data::igbinary_unserialize_data(const uint8_t* buf, size_t buf_size) : buffer(buf), buffer_size(buf_size), buffer_offset(0), strings(0), references(0) {
}

igbinary_unserialize_data::~igbinary_unserialize_data() {
	// Nothing to do for strings or references
	// req::vector<Object> wakeup automatically handles reference counts?
}

/* }}} */

static void igbinary_unserialize_variant(igbinary_unserialize_data *igsd, Variant& v, int flags);
static bool igbinary_unserialize_array_key(igbinary_unserialize_data *igsd, Variant& v);

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

/* {{{ igsd_defer_wakeup */
/* Defer wakeup */
static inline void igsd_defer_wakeup(struct igbinary_unserialize_data *igsd, const Object& o) {
	igsd->wakeup.emplace_back(o);
}
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
		throw IgbinaryWarning("igbinary_unserialize_data: unexpected end of buffer reading version %d > %d", (int) igsd->buffer_offset, (int) igsd->buffer_size);
	}

	version = igbinary_unserialize32(igsd);

	/* Support older version 1 and the current format 2 */
	if (version == IGBINARY_FORMAT_VERSION || version == 0x00000001) {
		return;
	} else {
		throw IgbinaryWarning("igbinary_unserialize_header: unsupported version: %d, should be %d or %u", (int) version, 0x00000001, (int) IGBINARY_FORMAT_VERSION);
	}
}
/* }}} */
/* {{{ igbinary_unserialize_long */
/** Unserializes zend_long */
inline static int igbinary_unserialize_long(struct igbinary_unserialize_data *igsd, enum igbinary_type t) {
	if (t == igbinary_type_long8p || t == igbinary_type_long8n) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_long: end-of-data");
		}

		return (t == igbinary_type_long8n ? -1 : 1) * igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_long16p || t == igbinary_type_long16n) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_long: end-of-data");
		}

		return (t == igbinary_type_long16n ? -1 : 1) * igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_long32p || t == igbinary_type_long32n) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_long: end-of-data");
		}

		/* check for boundaries */
		return (t == igbinary_type_long32n ? -1 : 1) * igbinary_unserialize32(igsd);
	} else if (t == igbinary_type_long64p || t == igbinary_type_long64n) {
		if (igsd->buffer_offset + 8 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_long: end-of-data");
		}

		/* check for boundaries */
		uint64_t tmp64 = igbinary_unserialize64(igsd);
		if (tmp64 > 0x8000000000000000 || (tmp64 == 0x8000000000000000 && t == igbinary_type_long64p)) {
			throw IgbinaryWarning("igbinary_unserialize_long: too big 64bit long.");
		}

		return (t == igbinary_type_long64n ? -1 : 1) * tmp64;
	} else {
		// FIXME is format string correct? Not reachable.
		throw IgbinaryWarning("igbinary_unserialize_long: unknown type '%02x', position %ld", (unsigned char)t, igsd->buffer_offset);
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
		throw IgbinaryWarning("igbinary_unserialize_double: end-of-data");
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
			throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_string16 || t == igbinary_type_object16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize16(igsd);
		if (igsd->buffer_offset + l > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
		}
	} else if (t == igbinary_type_string32 || t == igbinary_type_object32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
		}
		l = igbinary_unserialize32(igsd);
		if (igsd->buffer_offset + l > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
		}
	} else {
		throw IgbinaryWarning("igbinary_unserialize_chararray: unknown type '0x%x', position %ld", (int)t, (int64_t)igsd->buffer_offset);
	}
	if (igsd->buffer_offset + l > igsd->buffer_size) {
		throw IgbinaryWarning("igbinary_unserialize_chararray: end-of-data");
	}


	/** TODO : Optimize after implementing it the simple way and testing.. */
	// Make a copy of every occurence of the string.
	igsd->strings.emplace_back(reinterpret_cast<const char*>(igsd->buffer + igsd->buffer_offset), l, CopyString);
	igsd->buffer_offset += l;

	return igsd->strings.back();
}
/* }}} */
/* {{{ igbinary_unserialize_string */
/** Unserializes string. Unserializes by string id. */
inline static const String& igbinary_unserialize_string(struct igbinary_unserialize_data *igsd, enum igbinary_type t) {
	size_t i;
	if (t == igbinary_type_string_id8 || t == igbinary_type_object_id8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_string_id16 || t == igbinary_type_object_id16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_string_id32 || t == igbinary_type_object_id32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_string: end-of-data");
		}
		i = igbinary_unserialize32(igsd);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_string: unknown type '0x%x', position %ld", (int)t, (uint64_t)igsd->buffer_offset);
	}

	if (i >= igsd->strings.size()) {
		throw IgbinaryWarning("igbinary_unserialize_string: string index is out-of-bounds");
	}

	return igsd->strings[i];
}
/* }}} */
/* {{{ igbinary_unserialize_object_prop */
/* Similar to unserializeProp. nProp is the number of remaining dynamic properties. */
inline static void igbinary_unserialize_object_prop(igbinary_unserialize_data *igsd, ObjectData* obj, const Variant& key, int nProp) {
	// Do a two-step look up
	// FIXME not sure how protected variables are handled in igbinary in php5. Try to imitate that.
	// For now, assume it can be from the class or any parent class.
	Variant* t;
	if (UNLIKELY(key.isInteger())) {
		// Integer keys are guaranteed to be dynamic properties.
		t = &obj->reserveProperties(nProp).lvalAt(key, AccessFlags::Key);
	} else {
		const String strKey = key.toString();
		auto const lookup = obj->getProp(nullptr, strKey.get());
		if (!lookup.prop || !lookup.accessible) {
			// Dynamic property. If this is the first, and we're using MixedArray,
			// we need to pre-allocate space in the array to ensure the elements
			// dont move during unserialization.
			t = &obj->reserveProperties(nProp).lvalAt(strKey, AccessFlags::Key);
		} else {
			t = &tvAsVariant(lookup.prop);
		}
	}

	if (UNLIKELY(isRefcountedType(t->getRawType()))) {
        igsd->m_overwrittenList.append(*t);
		// throw IgbinaryWarning("igbinary_unserialize_object_prop: TODO handle duplicate keys or overriding existing data");
		//uns->putInOverwrittenList(*t);
	}

	igbinary_unserialize_variant(igsd, *t, WANT_CLEAR);
	// FIXME Type check the unserialized data.
	/*
	if (!RuntimeOption::RepoAuthoritative) return;
	if (!Repo::get().global().HardPrivatePropInference) return;
	*/

	/*
	 * We assume for performance reasons in repo authoriative mode that
	 * we can see all the sets to private properties in a class.
	 *
	 * It's a hole in this if we don't check unserialization doesn't
	 * violate what we've seen, which we handle by throwing if the repo
	 * was built with this option.
	 */
	/*
	auto const cls	= obj->getVMClass();
	auto const slot = cls->lookupDeclProp(key.get());
	if (UNLIKELY(slot == kInvalidSlot)) return;
	auto const repoTy = obj->getVMClass()->declPropRepoAuthType(slot);
	if (LIKELY(tvMatchesRepoAuthType(*t->asTypedValue(), repoTy))) {
		return;
	}
	throwUnexpectedType(key, obj, *t);
	*/
}
/* }}} */
/* {{{ igbinary_unserialize_object_new_contents_leftover */
/**
 * Inefficiently unserialize the remaining properties of an object, given an incomplete object with class set but no properties.
 * TODO: Pass this a list.
 */
inline static void igbinary_unserialize_object_new_contents_leftover(struct igbinary_unserialize_data* igsd, enum igbinary_type t, Object* obj, int n) {
	//Class* objCls = obj->getVMClass();
	for (int remainingProps = n; remainingProps > 0; --remainingProps) {
		/*
			use the number of properties remaining as an estimate for
			the total number of dynamic properties when we see the
			first dynamic prop.	see getVariantPtr
		*/
		Variant v;
		if (!igbinary_unserialize_array_key(igsd, v)) {
			continue;
		}
		igbinary_unserialize_object_prop(igsd, obj->get(), v, remainingProps);
	}
}
/* }}} */
/* {{{igbinary_unserialize_object_new_contents */
/**
 * Unserialize the properties of an object, given an incomplete object with class set but no properties.
 */
inline static void igbinary_unserialize_object_new_contents(struct igbinary_unserialize_data* igsd, enum igbinary_type t, Object* obj) {
	int n;
	if (t == igbinary_type_array8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_array16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_array32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_object_contents: end-of-data");
		}
		n = igbinary_unserialize32(igsd);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_object_contents: unknown type '%02x', position %lld", (int) t, (long long) igsd->buffer_offset);
	}
	/* n cannot be larger than the number of minimum "objects" in the array */
	if (n > igsd->buffer_size - igsd->buffer_offset) {
		throw IgbinaryWarning("igbinary_unserialize_object_contents: data size %lld smaller than requested array length %lld.", (long long)(igsd->buffer_size - igsd->buffer_offset), (long long)n);
	}
	if (obj->get()->isCollection()) {
		throw IgbinaryWarning("igbinary_unserialize_object_contents: Cannot unserialize HPHP collections");
	}
	if (n == 0) {
		return;
	}
	// FIXME: Iterate over object properties first(and figure out demangling), it's probably faster that way.
	igbinary_unserialize_object_new_contents_leftover(igsd, t, obj, n);

	// Wakeup will be deferred by caller.
}
/* }}} */
inline static void igbinary_unserialize_object_ser(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Object& obj) {
    if (!obj->instanceof(SystemLib::s_SerializableClass)) {
        raise_error("igbinary_unserialize: Class %s has no unserializer",
                      obj->getClassName().data());
		return;
	}
	size_t n;
	if (t == igbinary_type_object_ser8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_object_ser: end-of-data");
		}
		n = igbinary_unserialize8(igsd TSRMLS_CC);
	} else if (t == igbinary_type_object_ser16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_object_ser: end-of-data");
		}
		n = igbinary_unserialize16(igsd TSRMLS_CC);
	} else if (t == igbinary_type_object_ser32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_object_ser: end-of-data");
		}
		n = igbinary_unserialize32(igsd TSRMLS_CC);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_object_ser: unknown type '%02x', position %llu", (int)t, (unsigned long long)igsd->buffer_offset);
	}

	if (igsd->buffer_offset + n > igsd->buffer_size) {
		throw IgbinaryWarning("igbinary_unserialize_object_ser: end-of-data");
	}

	obj->o_invoke_few_args(s_unserialize, 1, String(reinterpret_cast<const char*>(igsd->buffer + igsd->buffer_offset), n, CopyString));
	obj.get()->clearNoDestruct();  // Allow destructor to be called (???)
}

/** Unserialize object, store into v. */
inline static void igbinary_unserialize_object(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Variant& v, int flags) {
	String class_name;
	if (t == igbinary_type_object8 || t == igbinary_type_object16 || t == igbinary_type_object32) {
		class_name = igbinary_unserialize_chararray(igsd, t);
	} else if (t == igbinary_type_object_id8 || t == igbinary_type_object_id16 || t == igbinary_type_object_id32) {
		class_name = igbinary_unserialize_string(igsd, t);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_object: unknown object type '%02x', position %llu", (int)t, (long long)igsd->buffer_offset);
	}

	// Unserialize the inner type (The byte after the class name).
	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw IgbinaryWarning("igbinary_unserialize_object: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);

	Class* cls = Unit::loadClass(class_name.get());  // with autoloading
	Object obj;
	if (cls) {
		// Only unserialize CPP extension types which can actually
		// support it. Otherwise, we risk creating a CPP object
		// without having it initialized completely.
		if (cls->instanceCtor() && !cls->isCppSerializable() &&
				!cls->isCollectionClass()) {
			// TODO: Make corresponding check when serializing?
			throw IgbinaryWarning("igbinary_unserialize_object: Unable to completely unserialize internal cpp class");
		} else {
			if (UNLIKELY(
				collections::isType(cls, CollectionType::Pair)
					)) {  // && (size != 2))) {
				throw IgbinaryWarning("igbinary_unserialize_object: HPHP type Pair unsupported, incompatible with php5/php7 implementation");
			}
			switch(t) {
				case igbinary_type_array8:
				case igbinary_type_array16:
				case igbinary_type_array32:
					obj = Object{cls};
					break;
				default:
					obj = Object::attach(g_context->createObject(cls, init_null_variant, false));
					break;
			}
		}
	} else {
		obj = Object{SystemLib::s___PHP_Incomplete_ClassClass};
		obj->o_set(s_PHP_Incomplete_Class_Name, class_name);
	}

	v = obj;
	if (flags & WANT_REF) {
		v.asRef();
	}

	// FIXME custom unserializer?

	/* add this to the list of unserialized references, get the index */
	// FIXME support refs

	// *obj will remain valid until __wakeup is called, which is done at the very end.
	igsd->references.push_back(&v);  // FIXME: Account for flags & WANT_REF

	switch (t) {
		case igbinary_type_array8:
		case igbinary_type_array16:
		case igbinary_type_array32:
			igbinary_unserialize_object_new_contents(igsd, t, &obj);
			break;
		case igbinary_type_object_ser8:
		case igbinary_type_object_ser16:
		case igbinary_type_object_ser32:
			igbinary_unserialize_object_ser(igsd, t, obj);
			break;
		default:
			throw IgbinaryWarning("igbinary_unserialize_object: unknown object inner type '%02x', position %lld", (int)t, (long long)igsd->buffer_offset);
	}

	if (cls && cls->lookupMethod(s___wakeup.get())) {
		igsd_defer_wakeup(igsd, obj);
	}
}
/* }}} */
/* {{{ igbinary_unserialize_array_key */
/**
 * Unserialize an array key (int64 or string).
 * Postcondition: v.isInteger() || v.isString() || return value is false, or IgbinaryWarning thrown.
 * See igbinary_unserialize_variant.
 * (The return false if the serializer needs to indicate that the serialized entry was skipped)
 */
static bool igbinary_unserialize_array_key(igbinary_unserialize_data *igsd, Variant& v) {
	enum igbinary_type t;

	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw IgbinaryWarning("igbinary_unserialize_array_key: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);
	switch (t) {
		case igbinary_type_string_empty:
			{
				String s = "";
				// TODO: Make a constant?
				tvMove(make_tv<KindOfString>(s.detach()), *v.asTypedValue());
			}
			return true;
		case igbinary_type_long8p:
		case igbinary_type_long8n:
		case igbinary_type_long16p:
		case igbinary_type_long16n:
		case igbinary_type_long32p:
		case igbinary_type_long32n:
		case igbinary_type_long64p:
		case igbinary_type_long64n:
			v = igbinary_unserialize_long(igsd, t);
			return true;
		case igbinary_type_string8:
		case igbinary_type_string16:
		case igbinary_type_string32:
			v = igbinary_unserialize_chararray(igsd, t);
			return true;
		case igbinary_type_string_id8:
		case igbinary_type_string_id16:
		case igbinary_type_string_id32:
			v = igbinary_unserialize_string(igsd, t);
			return true;
		case igbinary_type_null:
			return false;
		default:
			throw IgbinaryWarning("igbinary_unserialize_array_key: Unexpected igbinary_type 0x%02x at offset %lld", (int) t, (long long) igsd->buffer_offset);
	}
	return true;
}
/* {{{ igbinary_unserialize_array */
/** Unserializes array. */
inline static void igbinary_unserialize_array(struct igbinary_unserialize_data *igsd, enum igbinary_type t, Variant& v, bool wantRef) {
	/* wantRef means that z will be wrapped by an IS_REFERENCE */
	size_t n;
	if (t == igbinary_type_array8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize8(igsd);
	} else if (t == igbinary_type_array16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize16(igsd);
	} else if (t == igbinary_type_array32) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_array: end-of-data");
		}
		n = igbinary_unserialize32(igsd);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_array: unknown type 0x%02x, position %ld", t, igsd->buffer_offset);
	}

	/* n cannot be larger than the number of minimum "objects" in the array */
	if (n > igsd->buffer_size - igsd->buffer_offset) {
		throw IgbinaryWarning("igbinary_unserialize_array: data size %llu smaller that requested array length %llu.", (long long)(igsd->buffer_size - igsd->buffer_offset), (long long) n);
	}

	igsd->references.push_back(&v);
	/* empty array */
	if (n == 0) {
		v = Array::Create();  // static empty array.
		if (wantRef) {
			v.asRef();
		}
		return;
	}

	v = ArrayInit(n, ArrayInit::Mixed{}).toArray();

	// FIXME: Need to add a reference just in case of a duplicate key causing the original variant reference count to be decremented.
	Array* arr;
	if (wantRef) {
		v.asRef();
		auto tv = v.asTypedValue();
		Variant& v_deref = *tv->m_data.pref->var();
		arr = &(v_deref.asArrRef());
	} else {
		arr = &(v.asArrRef());
	}

	for (size_t i = 0; i < n; i++) {
		Variant key;
		if (!igbinary_unserialize_array_key(igsd, key)) {
			continue;
		}
		// Postcondition: key.isString() || key.isInteger()

		Variant& value = arr->lvalAt(key, AccessFlags::Key);
		// FIXME: handle case of value already existing? (analogous to putInOverwrittenList)

		// TODO: Any other code for references
		igbinary_unserialize_variant(igsd, value, WANT_CLEAR);
	}
}
/* }}} */
/* {{{ */
static void igbinary_unserialize_ref(igbinary_unserialize_data *igsd, enum igbinary_type t, Variant& v, int flags) {
	size_t n;

	if (t == igbinary_type_ref8 || t == igbinary_type_objref8) {
		if (igsd->buffer_offset + 1 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_ref: end-of-data");
		}
		n = igbinary_unserialize8(igsd TSRMLS_CC);
	} else if (t == igbinary_type_ref16 || t == igbinary_type_objref16) {
		if (igsd->buffer_offset + 2 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_ref: end-of-data");
		}
		n = igbinary_unserialize16(igsd TSRMLS_CC);
	} else if (LIKELY(t == igbinary_type_ref32 || t == igbinary_type_objref32)) {
		if (igsd->buffer_offset + 4 > igsd->buffer_size) {
			throw IgbinaryWarning("igbinary_unserialize_ref: end-of-data");
		}
		n = igbinary_unserialize32(igsd TSRMLS_CC);
	} else {
		throw IgbinaryWarning("igbinary_unserialize_ref: unknown type '%02x', position %lld", (int)t, (long long)igsd->buffer_offset);
	}

	if (n >= igsd->references.size()) {
		throw IgbinaryWarning("igbinary_unserialize_ref: invalid reference %u >= %u", (int) n, (int)igsd->references.size());
	}

	Variant*& data = igsd->references[n];

	if ((flags & WANT_REF) != 0) {
		if (data->getRawType() != KindOfRef) {
			v = *data;
			v.asRef();
			data = &v;
		} else {
			v.assignRef(*data);
		}
	} else {
		// Copy underlying data of ref or non-ref. Deletes old data?
		// v.constructValHelper(*data);
		cellDup(tvToInitCell(data->asTypedValue()), *v.asTypedValue());
	}


}
/* }}} */
/* {{{ igbinary_unserialize_variant */
/* Unserialize a variant. Same as igbinary7 igbinary_unserialize_zval */
static void igbinary_unserialize_variant(igbinary_unserialize_data *igsd, Variant& v, int flags) {
	enum igbinary_type t;

	if (igsd->buffer_offset + 1 > igsd->buffer_size) {
		throw IgbinaryWarning("igbinary_unserialize_variant: end-of-data");
	}
	t = (enum igbinary_type) igbinary_unserialize8(igsd);
	switch (t) {
		case igbinary_type_ref:
			{
				igbinary_unserialize_variant(igsd, v, WANT_REF);
				const DataType type = v.getRawType();
				/* If it is already a ref, nothing to do */
				if (type == KindOfRef) {
					break;
				}
				switch (type) {
					case KindOfString:
					case KindOfInt64:
					case KindOfNull:
					case KindOfDouble:
					case KindOfBoolean:
						igsd->references.push_back(&v);
						break;
					default:
						break;
				}

				/* Convert v to a ref */
				v.asRef();
			}
			break;
		case igbinary_type_objref8:
		case igbinary_type_objref16:
		case igbinary_type_objref32:
		case igbinary_type_ref8:
		case igbinary_type_ref16:
		case igbinary_type_ref32:
			igbinary_unserialize_ref(igsd, t, v, flags);
			return;
		case igbinary_type_object8:
		case igbinary_type_object16:
		case igbinary_type_object32:
		case igbinary_type_object_id8:
		case igbinary_type_object_id16:
		case igbinary_type_object_id32:
			igbinary_unserialize_object(igsd, t, v, flags);
			break;
		case igbinary_type_array8:
		case igbinary_type_array16:
		case igbinary_type_array32:
			{
				igbinary_unserialize_array(igsd, t, v, (flags & WANT_REF) != 0);
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
			throw IgbinaryWarning("TODO implement igbinary_unserialize_variant for igbinary_type 0x%02x, offset %lld", (int) t, (long long) igsd->buffer_offset);
	}
}
/* }}} */
} // namespace

namespace HPHP {

/** Unserialize the data, or clean up and throw an Exception. Effectively constant, unless __sleep modifies something. */
void igbinary_unserialize(const uint8_t *buf, size_t buf_len, Variant& v) {
	igbinary_unserialize_data igsd(buf, buf_len);  // initialized by constructor, freed by destructor
	try {

		igbinary_unserialize_header(&igsd);  // Unserialize header or throw exception.
		igbinary_unserialize_variant(&igsd, v, WANT_CLEAR);
		/* FIXME finish_wakeup */
	} catch (IgbinaryWarning &e) {
		v = false;
		raise_warning(e.getMessage());
		return;
	}
	for (auto& obj : igsd.wakeup) {
		obj->invokeWakeup();
	}
}

} // namespace HPHP
