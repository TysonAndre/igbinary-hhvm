igbinary-hhvm
============

Build Status: TODO

Native igbinary for HHVM. Not working yet.

# Functionality

Only `igbinary_serialize` and `igbinary_unserialize`are supported.
References may take a while to do properly

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
