#include <stdio.h>



int main() {
    // 1. Declare and initialize the array with 10 constant integer values
    //int numbers[10] = {42, 15, 8, 99, 23, 4, 16, 55, 7, 10};
    int numbers[5];
    numbers[0] = 42;
    numbers[1] = 15;
    numbers[2] = 8;
    numbers[3] = 99;
    numbers[4] = 23;
    // Variable to store temporary values during swapping
    int temp;

    
    // 2. Sort the array in ascending order using Bubble Sort
    // The outer loop controls the number of passes
    int i;
    for (i = 0; i < 5 - 1; i++) {
        // The inner loop compares adjacent elements
        int j;
        for (j = 0; j < 5 - i - 1; j++) {
            
            // If the current element is greater than the next one, swap them
            if (numbers[j] > numbers[j + 1]) {
                temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }

    
    return 0;
}