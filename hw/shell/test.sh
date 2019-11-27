#!/bin/sh

# Color
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# test 1
echo "echo 'abc abc def jkl mno pqr stu vwxyz' | wc -w" | ./a.out > tmp_check

check_content=$(cat tmp_check)
if [ $check_content -eq 8 ]
then
    echo "Case#1: ${GREEN}Passed"
else
    echo "Case#1: ${RED}Failed"
fi

rm tmp_check
