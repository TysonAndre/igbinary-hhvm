igbinary-hhvm
============

Requirements: HHVM >= 3.22 (May support earlier versions in future commits)

[![Build Status](https://travis-ci.org/TysonAndre/igbinary-hhvm.svg?branch=master)](https://travis-ci.org/TysonAndre/igbinary-hhvm)

(Around 52 tests passing, 7 tests failing)

Native igbinary for HHVM. Mostly working, with some edge cases

# Functionality

So far, the following work:

- Scalars(Strings, Booleans, numbers)
- Arrays of scalars
- Objects
- Arrays/Objects containing duplicate values
- Integration with libraries using igbinary
- Objects with magic methods (`serialize`,`unserialize`,`__sleep`,`__wakeup`)

The following need more work:

- Cyclic data structures (May just be how `var_dump` represents it, need to investigate)
- Objects with magic methods (`__wakeup`, `__destruct`)
- References are still unsupported.
  Haven't decided when/if igbinary should throw, convert to a regular value, etc.


External integration, such as with APCu, Memcached, Redis, session serializers, etc. won't work.
(without their sources being patched)

# Incompatibilities

References to PHP objects (as in `is_object`) are now intended to be treated as non-references to objects when serializing,
to match what the HHVM serializer does in php compatibility mode.

TODO: Check what the current compatibility mode is.

# Installation

```bash
$ git clone https://github.com/TysonAndre/igbinary-hhvm
$ cd igbinary-hhvm
$ hphpize && cmake . && make
$ cp igbinary.so /path/to/your/hhvm/ext/dir
```

`hhvm-dev` must be installed for hphpize to work.

# Testing

```bash
$ make test
```

To add ini options for a specific test (e.g. 042.php), add 042.php.ini?
Global test ini options are in [test/config.ini](test/config.ini)

To port test 00x.phpt to HHVM's test framework, split it up into 00x.php, 00x.php.skipif, 00x.php.expect (or 00x.php.expectf).
If there are any ini settings specific to that test, those go in 00x.php.ini?

# Authors

- Tyson Andre <tysonandre775@hotmail.com>

Based on https://www.github.com/igbinary/igbinary
