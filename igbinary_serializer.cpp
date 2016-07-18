#include <stddef.h>

#include "ext_igbinary.hpp"

/** Serializer data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_serialize_data {
	uint8_t *buffer;			/**< Buffer. */
	size_t buffer_size;			/**< Buffer size. */
	size_t buffer_capacity;		/**< Buffer capacity. */
	bool scalar;				/**< Serializing scalar. */
	bool compact_strings;		/**< Check for duplicate strings. */
	struct hash_si strings;		/**< Hash of already serialized strings. */
	struct hash_si_ptr references;	/**< Hash of already serialized potential references. (non-NULL uintptr_t => int32_t) */
	int references_id;			/**< Number of things that the unserializer might think are references. >= length of references */
	int string_count;			/**< Serialized string count, used for back referencing */
	int error;					/**< Error number. Not used. */
	struct igbinary_memory_manager	mm; /**< Memory management functions. */
};
