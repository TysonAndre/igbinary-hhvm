FROM hhvm/hhvm:3.22.1

RUN apt-get update -y && \
    apt-mark hold hhvm && \
    apt-get install hhvm-dev -y && \
	apt-get clean

RUN mkdir /usr/src/igbinary-hhvm
WORKDIR /usr/src/igbinary-hhvm
ADD config.cmake \
	ext_igbinary.cpp \
	ext_igbinary.hpp \
	ext_igbinary.php \
	hash_si_ptr.cpp \
	hash_ptr.hpp \
	igbinary_serializer.cpp \
	igbinary_unserializer.cpp \
	igbinary_utils.cpp \
	./
RUN hphpize && cmake . && make
ADD test/ ./test
RUN make test
