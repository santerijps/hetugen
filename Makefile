debug:
	gcc -Og -Wall -Wextra -pedantic -S hetugen.c
	gcc -Og -Wall -Wextra -pedantic hetugen.c -o hetugen.exe

release:
	gcc -Ofast -march=native -DNODEBUG -Wall -Wextra -pedantic -s -S hetugen.c
	gcc -Ofast -march=native -DNODEBUG -Wall -Wextra -pedantic -s hetugen.c -o hetugen.exe
