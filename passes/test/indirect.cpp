#include <random>

void foo(int a) {
    a = 1;
    return;
}

void bar(int b) {
    b = 2;
    return;
}

int main(void) {
    int a = rand() % 2;
    void (*f)(int);
    if (a) {
        f = foo;
        f(1);
    } else {
        f = bar;
        f(1);
    }
}
