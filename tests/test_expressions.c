int main() {
    int a = 10;
    int b = 20;
    int c;

    // Arithmetic
    c = a + b;
    c = b - a;
    c = a * b;
    c = b / a;
    c = 15 % 4;

    // Relational and Logical
    if (a < b) {
        c = 1;
    }

    if (a > b || c == 1) {
        c = 2;
    }

    if (a < b && b > 5) {
        c = 3;
    }

    return c;
}