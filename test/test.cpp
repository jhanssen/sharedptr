#include <SharedPtr.h>
#include <stdio.h>

using namespace nonatomic;

class A
{
public:
    A(int a_)
        : a(a_)
    {
        printf("constructing A %d\n", a);
    }
    ~A()
    {
        printf("destructing A %d\n", a);
    }

    int a;
};

int main(int, char**)
{
    WeakPtr<A> w1;
    {
        SharedPtr<A> s1(new A(10));
        printf("s1: %d\n", s1->a);
        w1 = s1;
        {
            SharedPtr<A> s2 = s1;
            printf("s2: %d\n", s2->a);
        }
        if (SharedPtr<A> s3 = w1.lock()) {
            printf("w1: %d\n", s3->a);
        } else {
            printf("unable to lock w1\n");
        }
    }
    if (SharedPtr<A> s3 = w1.lock()) {
        printf("w1: %d\n", s3->a);
    } else {
        printf("unable to lock w1\n");
    }

    return 0;
}
