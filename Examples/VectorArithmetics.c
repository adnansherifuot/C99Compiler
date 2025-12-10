#include <stdio.h>

int main() {
// Define a struct to represent a point
struct point
{
    int x;
    int y;
} p1,p2;
p1.x = 10;
p1.y = 20;
p2.x = 30;
p2.y = 40;
struct point p3;
// Add two points
p3.x = p1.x + p2.x;
p3.y = p1.y + p2.y;

}
