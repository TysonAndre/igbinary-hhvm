#!/usr/bin/env bash
set -xeu
# This builds igbinary and runs unit tests, or fails with a non zero exit code
docker build -t igbinary-hhvm-3.22.1 -f ci/Dockerfile .
