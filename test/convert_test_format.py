#!/usr/bin/env python3
import os
import re
import sys

TEST_COMMENT    = re.compile(r"--TEST--\r?\n([^\n\r]+)\r?\n--", re.MULTILINE)
FILE_CONTENTS   = re.compile(r"--FILE--\r?\n<\?php\s*\r?\n?\s*(.*)(\r?\n?(\?>)?)?\r?\n--EXPECT(F|REGEX)?--", re.MULTILINE | re.DOTALL)
EXPECT_CONTENTS = re.compile(r"--(EXPECT(F|REGEX)?)--\r?\n(.*)$", re.MULTILINE | re.DOTALL)

def get_normalized_filename_as_php(phpt_filename):
    phpt_filename = re.sub(r"(^|/)igbinary_([^/.]+.phpt)", r"\1\2", phpt_filename)
    php_filename = re.sub("\.phpt$", ".php", phpt_filename)
    return php_filename

def parse_phpt_sections(contents, phpt_filename):
    comment_match = TEST_COMMENT.search(contents)
    if comment_match is None:
        sys.stderr.write("Could not find comment in {0}\n".format(phpt_filename))
        sys.exit(1)
    comment = comment_match.group(1)
    php_code_match = FILE_CONTENTS.search(contents)
    if php_code_match is None:
        sys.stderr.write("Could not find php test code in {0}\n".format(phpt_filename))
        sys.exit(1)
    php_code = php_code_match.group(1)
    expect_match = EXPECT_CONTENTS.search(contents)
    if expect_match is None:
        sys.stderr.write("Could not find expectated output (EXPECT or EXPECTF) in {0}\n".format(phpt_filename))
        sys.exit(1)
    is_expectf = expect_match.group(1) in ("EXPECTF", "EXPECTREGEX")
    expect = expect_match.group(3)
    return [comment, php_code, expect, is_expectf]

def main():
    files = sys.argv[1:]

    if len(files) == 0:
        sys.stderr.write("Usage: {0} path/to/igbinary_0xy.phpt...\n".format(sys.argv[0]))
        sys.exit(1)

    for filename in files:
        if filename[-5:] != '.phpt':
            sys.stderr.write("{0} is not a file of type phpt\n".format(filename))
            sys.exit(1)

    for filename in files:
        with open(filename) as file:
            contents = file.read()
            [comment, php_code, expect, is_expectf] = parse_phpt_sections(contents, filename)
            result_filename = get_normalized_filename_as_php(filename)
            result_contents = "<?php\n// {0}\n{1}".format(comment.strip().replace("\n", "\n// "), php_code)
            with open(result_filename, "w") as result_file:
                result_file.write(result_contents)
            expect_filename = result_filename + (".expectf" if is_expectf else ".expect")
            with open(expect_filename, "w") as expect_file:
                expect_file.write(expect)
            print("Wrote {0}: {1}".format(result_filename, "; ".join(re.split("\r?\n", comment))))

if __name__ == "__main__":
     main()
