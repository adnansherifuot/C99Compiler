int main() {
    int i;
    int sum = 0;

    for (i = 0; i < 10; i = i + 1) {
        if (i < 2) {
            continue; // Skip the first two iterations
        }
        if (i > 5) {
            break; // Exit the loop after i=5
        }
        sum = sum + i;
    }

    // sum should be 2 + 3 + 4 + 5 = 14

    return sum;
}