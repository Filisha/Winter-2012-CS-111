# To run tests, simply use
#    sh runTests.sh

make clean
make
sh 2script.sh > test.exp
./timetrash 2script.sh > test.act
diff -u test.exp test.act
rm test.exp
rm test.act
