#include "rename.h"

// Main function
int main(){
    for (int i =0 ; i<10;i++){
        int *test = MALLOC(sizeof(int) * 1000000);
        FREE(test);  
    }


    return 0;
}
