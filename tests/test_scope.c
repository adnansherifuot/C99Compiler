int x = 10; // Global x

int main() {
    int x = 20; // Shadowing global x

    if (x == 20) {
        int x = 30; // Shadowing main's x
        if (x != 30) {
            return -1; // Error
        }
    }

    return x; // Should return 20
}