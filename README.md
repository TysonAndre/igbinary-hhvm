igbinary-hhvm
============

Build Status: TODO

Native igbinary for HHVM. Not working yet.

# Functionality

So far, `igbinary_unserialize(igbinary_serialize(NULL)) === NULL` works. Other data types aren't implemented yet.

Only `igbinary_serialize` and `igbinary_unserialize` are supported.
External integration, such as with APC, Memcached, Redis, etc. won't work.
References may take a while to do properly.

# Installation

```bash
$ git clone https://github.com/TysonAndre/igbinary-hhvm --depth=1
$ cd igbinary-hhvm
$ hphpize && cmake . && make
$ cp igbinary.so /path/to/your/hhvm/ext/dir
```

`hhvm-dev` must be installed for hphpize to work.

# Authors

- Tyson Andre <tysonandre775@hotmail.com>

Based on https://www.github.com/igbinary/igbinary
