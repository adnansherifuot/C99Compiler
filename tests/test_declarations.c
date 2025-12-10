// Test basic type declarations
int a;
char b;
float c;
double d;

// Test pointer declarations
int *p_a;
char **p_b;

// Test array declarations
int arr1[10];
float arr2[5];

// Test struct declaration and definition
struct Point {
    int x;
    int y;
};

// Test struct variable declaration
struct Point p1;

// Test union declaration
union Data {
    int i;
    float f;
};

union Data data_var;

int main() {
    a = 1;
    b = 'c';
    p1.x = 10;
    p1.y = 20;
    arr1[0] = 100;
    return 0;
}