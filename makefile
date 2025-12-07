test: main.o
	g++ -o test2 main2.o -Lsrc/lib -lsfml-graphics -lsfml-window -lsfml-system
main.o: main.cpp
	g++ -c main2.cpp -Isrc/include