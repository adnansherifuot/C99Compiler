int main() {
    int i;
    int sum = 1;

    // If-else statement
    if (sum == 0) {
        sum = 1;
    } else {
        sum = -1;
    }

    // For loop
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }

    // While loop
    i = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }

    // Do-while loop
    i = 0;
    do {
        sum = sum + i;
        i = i + 1;
    } while (i < 5);

    return sum;
}