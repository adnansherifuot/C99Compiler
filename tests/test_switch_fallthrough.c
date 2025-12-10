int main() {
    int i = 2;
    int result = 0;

    // This switch tests fall-through behavior.
    switch (i) {
        case 1:
            result = result + 1;
        case 2: // Execution starts here
            result = result + 2; // result becomes 2
        case 3:
            result = result + 3; // result becomes 5
            break; // Exits the switch
        case 4:
            result = result + 4;
        default:
            result = -100; // This should not be executed
    }

    // The final result should be 5.
    return result;
}