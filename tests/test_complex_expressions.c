#include <stdio.h>

/**
 * @brief A simple function that adds 10.0 to a float.
 *
 * @param value The input float.
 * @return The value plus 10.0.
 */
float add_ten(float value) {
    return value + 10.0;
}

int main() {
    // Initialize variables of different types
    int count = 5;
    float pi = 3.14;
    double result; // Use double for more precision in the result
    int a=10;
    

    // This is a compound expression that includes:
    // 1. A unary operator: count++ (post-increment)
    // 2. A function call: add_ten(pi)
    // 3. Different types: 'count' is an int, 'pi' is a float.
    //
    // Evaluation steps:
    // - The value of 'count' (5) is read for the expression.
    // - The function add_ten(pi) is called, which returns 13.14.
    // - The multiplication 5 * 13.14 is performed.
    // - The result (65.7) is stored in the 'result' variable.
    // - 'count' is then incremented to 6.
    result = (count++) * add_ten(pi)*-a;
    printf("Result of complex expression: %f\n", result);
    

    return 0;
}
