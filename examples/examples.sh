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

echo "Testing equality check"
FOO=hello ./bin/envil -e FOO --eq "hello"
echo $?
FOO=world ./bin/envil -e FOO --eq "hello"
echo $?

echo "Testing inequality check"
FOO=test@example.com ./bin/envil -e FOO --ne "admin@example.com"
echo $?
FOO=admin@example.com ./bin/envil -e FOO --ne "admin@example.com"
echo $?

echo "Testing greater than or equal"
FOO=10 ./bin/envil -e FOO --type integer --ge 10
echo $?
FOO=9 ./bin/envil -e FOO --type integer --ge 10
echo $?

echo "Testing less than or equal"
FOO=10 ./bin/envil -e FOO --type integer --le 10
echo $?
FOO=11 ./bin/envil -e FOO --type integer --le 10
echo $?

echo "Testing string length greater than"
FOO=password123 ./bin/envil -e FOO --lengt 7
echo $?
FOO=pass ./bin/envil -e FOO --lengt 7
echo $?

echo "Testing string length less than"
FOO=short ./bin/envil -e FOO --lenlt 10
echo $?
FOO=verylongpassword ./bin/envil -e FOO --lenlt 10
echo $?

echo "Testing regex pattern matching"
FOO=test@example.com ./bin/envil -e FOO --regex "^[^@]+@[^@]+\.[^@]+$"
echo $?
FOO=invalid-email ./bin/envil -e FOO --regex "^[^@]+@[^@]+\.[^@]+$"
echo $?

echo "Testing combined validation with regex and length"
FOO=strong-pass123 ./bin/envil -e FOO --regex "^[A-Za-z0-9-]+$" --lengt 7 --lenlt 33
echo $?
FOO=weak ./bin/envil -e FOO --regex "^[A-Za-z0-9-]+$" --lengt 7 --lenlt 33
echo $?