#include <SharedPtr.h>
#include <stdio.h>

using namespace nonatomic;

class A : public EnableSharedFromThis<A>
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

class B
{
public:
    B(int b_)
        : b(b_)
    {
        printf("constructing B %d\n", b);
    }
    ~B()
    {
        printf("destructing B %d\n", b);
    }

    int b;
};

int main(int, char**)
{
    WeakPtr<A> w1;
    {
        A* aa = new A(10);
        SharedPtr<A> s1(aa);
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
        if (SharedPtr<A> s4 = aa->sharedFromThis()) {
            printf("got sharedFromThis: %d\n", s4->a);
        } else {
            printf("couldn't get sharedFromThis\n");
        }
    }
    if (SharedPtr<A> s3 = w1.lock()) {
        printf("w1: %d\n", s3->a);
    } else {
        printf("unable to lock w1\n");
    }

    {
        SharedPtr<B> sb1(new B(20));
        printf("sb1: %d\n", sb1->b);
    }

    return 0;
}
