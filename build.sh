gcc -g -o main main.c c-vector/vec.c c-hashmap/hashmap.c lexer.c parser.c enum_utilities.c -Wall -Wextra
gcc -g -o main_san main.c c-vector/vec.c c-hashmap/hashmap.c lexer.c parser.c enum_utilities.c -Wall -Wextra -fsanitize=address
