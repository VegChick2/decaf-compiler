#!/bin/bash
#for i in $(ls tests/*.decaf); do
#	y=${i%.decaf}
#	f=${y##*/}
#	echo $f
#	./dcc < $i
#done
echo @@testing on samples/\*
for i in $(ls samples/*.decaf); do
    echo @@@ $i @@@
    y=${i%.decaf}
    f=${y##*/}
    # ./solution/dcccaenupdate < $i 2> samples/$f.out
    ./dcc < $i > out/$f.my.s
    spim -f out/$f.my.s | tail -n +6 > out/$f.my.out
    cat samples/$f.out | tail -n +1 > out/$f.cor.out
    diff -w out/$f.my.out out/$f.cor.out
done

# echo @@tesing on tests/\*

# for i in $(ls tests/*.decaf); do
#     echo @@@ $i @@@
#     y=${i%.decaf}
#     f=${y##*/}
#     ./solution/dcccaenupdate < $i 2> tests/$f.out
#     ./dcc < $i 2> tests/$f.my
#     diff -w tests/$f.my tests/$f.out
# done

# for i in $(ls tests_2/*.decaf); do
#     echo @@@ $i @@@
#     y=${i%.decaf}
#     f=${y##*/}
#     ./solution/dcccaenupdate < $i 2> tests/$f.out
#     ./dcc < $i 2> tests/$f.my
#     diff -w tests/$f.my tests/$f.out
# done

exit 0


# TEST_DIR=tests
# # echo === generate correct files ===
# cd $TEST_DIR
# for i in $(ls *.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	./dcc < $i &> $f.sol
# done
# # echo === done ===

# cd ..
# for i in $(ls $TEST_DIR/*.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	echo $f
# 	./dcc < $i &> $TEST_DIR/out/$f.out
# 	diff -w $TEST_DIR/out/$f.out $TEST_DIR/$f.sol
# done


# TEST_DIR=more_tests
# echo generate correct files
# cd $TEST_DIR
# for i in $(ls *.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	./dcc < $i &> $f.sol
# done

# cd ..
# for i in $(ls $TEST_DIR/*.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	echo $f
# 	./dcc < $i &> $TEST_DIR/out/$f.out
# 	diff -w $TEST_DIR/out/$f.out $TEST_DIR/$f.sol
# done

