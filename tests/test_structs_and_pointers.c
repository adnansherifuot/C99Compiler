

int main() {
    struct Data {
    int a;
    int b;
    }; 
    
    struct Data s, *ptr;

    s.a = 10;
    s.b = 20;

    ptr = &s;

    // Test member access and pointer member access
    ptr->b = s.a + ptr->a;

    return s.b;
}