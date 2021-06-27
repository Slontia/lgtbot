set -e

game=$1
cd build
make
rm -f ./games/CMakeFiles/test_game_${game}.dir/${game}/mygame.cpp.gcda
./test_game_${game}
gcov ./games/CMakeFiles/test_game_${game}.dir/${game}/mygame.cpp.gcno > /dev/null
grep "^ *\(#####\|[0-9]*\*\):" mygame.cpp.gcov -B 3 -A 3
rm ./*.gcov
