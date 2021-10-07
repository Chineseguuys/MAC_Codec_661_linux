#include <iostream>
#include <stdlib.h>


int main(int argc, char* argv[]) {
    char* char_array = new char[0];

    if (char_array != NULL) 
        delete[] char_array;

    return 0;
}