#include <stdlib.h>

void foo(int a) {
    a = 1;
    return;
}

void bar(int b) {
    b = 2;
    return;
}

void mon(int c) {
    c = 3;
    return;
}

int main(void) {
    // srand(time(NULL));
    int a = 2;
    void (*f)(int);
    if (a == 0) {
        f = foo;
        f(1);
    } else if (a == 1) {
        f = bar;
        f(1);
    } else {
        f = mon;
        f(1);
    }
}
