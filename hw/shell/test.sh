#!/bin/sh

echo "echo 'abc def ghi jkl mno pqr stu vwxyz' | wc | sed 's/^ *//g'" | ./a.out
