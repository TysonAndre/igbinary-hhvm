/**
 * Function for unserializing. This contains implementation details of  igbinary_unserialize()
 */

/** String/len pair for the igbinary_unserializer_data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 * @see igbinary_unserialize_data.
 */
struct igbinary_unserialize_string_pair {
	char *data;		/**< Data. */
	size_t len;		/**< Data length. */
};

/** Unserializer data.
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 */
struct igbinary_unserialize_data {
	uint8_t *buffer;				/**< Buffer. */
	size_t buffer_size;				/**< Buffer size. */
	size_t buffer_offset;			/**< Current read offset. */

	struct igbinary_unserialize_string_pair *strings; /**< Unserialized strings. */
	size_t strings_count;			/**< Unserialized string count. */
	size_t strings_capacity;		/**< Unserialized string array capacity. */

	zval **references;				/**< Unserialized Arrays/Objects. */
	size_t references_count;		/**< Unserialized array/objects count. */
	size_t references_capacity;		/**< Unserialized array/object array capacity. */

	int error;						/**< Error number. Not used. */
	smart_string string0_buf;			/**< Temporary buffer for strings */
};

Variant igbinary_unserialize(const uint8_t *buf, size_t buf_len) {
	struct igbinary_unserialize_data igsd;
	igbinary_unserialize_data_init(&igsd);
	igsd.buffer_size = buf_len;

	if (igbinary_unserialize_header(&igsd TSRMLS_CC)) {
		igbinary_unserialize_data_deinit(&igsd TSRMLS_CC);
		return 1;
	}

	igsd.buffer = buffer;
}
