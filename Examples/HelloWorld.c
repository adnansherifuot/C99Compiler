int add(int a, int b) {
    int temp;
    temp = a + b;
    return temp;
}
int main() {
    int x;
    int y;
    x=5;
    y=10;
    int sum;
    sum=add(x,y);
    int i;
    for(i=0; i<5; i=i+1) {
        // Do nothing
        sum=sum+i;
    }
    while(sum>0) {
        sum=sum-1;
    }
    do{
        sum=sum+1;
    } while(sum<5);
    return 0;
}