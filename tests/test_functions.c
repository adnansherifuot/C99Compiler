// Test function declarations and calls
// Function with no arguments and no return value
void func_void_void() {
    // Do nothing
}

// Function with one argument and no return value
void func_void_int(int a) {
    // Do nothing
}

// Function with two arguments and an int return value
int func_int_int_int(int a, int b) {
    return a + b;
}

// Function with multiple argument types
float func_float_int_float_char(int x, float y, char z) {
    return x ;
}

// Function with a pointer argument
void func_ptr_int(int *ptr) {
    *ptr = *ptr + 1;
}

int main() {
    int x = 10;
    int y = 20;
    int sum;
    float f_val;
    char c_val = 'A';
    int *p_x = &x;

    // Test calls to functions
    func_void_void(); // Call function with no args, no return

    func_void_int(x); // Call function with one int arg

    sum = func_int_int_int(x, y); // Call function with two int args, int return

    f_val = func_float_int_float_char(x, 5.5, c_val); // Call function with mixed args, float return

    func_ptr_int(p_x); // Call function with pointer arg, modifies x

    // Verify results (these would typically be checked by assertions in a real test framework)
    // sum should be 30
    // f_val should be 10 + 5.5 + 65 = 80.5 (assuming ASCII 'A' is 65)
    // x should be 11 after func_ptr_int

    return sum + f_val + x; // Expected: 30 + 80 + 11 = 121
}

