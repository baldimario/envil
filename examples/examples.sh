#!/bin/sh
echo This will fail because FOO is not set
./bin/envil -e FOO --type integer --gt 10
echo $?

echo This will succeed because FOO is defaulted to 20
./bin/envil -e FOO --type integer --gt 10 --default 20
echo $?

echo This will fail because FOO is not an integer
FOO=asd ./bin/envil -e FOO --type integer --gt 10
echo $?

echo This will succeed because FOO is an integer
FOO=20 ./bin/envil -e FOO --type integer --gt 10
echo $?

echo This will fail because FOO is too small
FOO=5 ./bin/envil -e FOO --type integer --gt 10
echo $?

echo This will execute numeric validation with configuration file
FOO=20 ./bin/envil -c examples/number_validation.yml
echo $?

echo This also will execute string validation with configuration file
PORT=2345 CONFIG='{"foo": "bar"}' API_KEY=12345678901234567890123456789012 GIT_BRANCH=main ./bin/envil -c examples/validation_examples.yml
echo $?