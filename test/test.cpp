#include <SharedPtr.h>
#include <stdio.h>
#include <vector>

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
    // {
    //     SharedPtr<A> s1(new A(200));
    //     SharedPtr<A>& s2 = s1;
    //     printf("base: %d\n", s1->a);
    //     printf("ref: %d\n", s2->a);
    // }
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

    {
        std::vector<SharedPtr<A> > ptrs;
        {
            SharedPtrPoolScope<A> pool = SharedPtr<A>::makePool(5);
            ptrs.reserve(5);
            for (int i = 0; i < 5; ++i) {
                A* a = new(pool.mem(i)) A(i + 100);
                ptrs.push_back(SharedPtr<A>(pool, i, a));
            }
            printf("pool count (should be 6?) %u\n", pool.useCount());
        }
        for (int i = 0; i < 5; ++i) {
            SharedPtr<A> ptr = ptrs[i];
            printf("vector1 ptr %d: %d\n", i, ptr->a);
        }
    }

    {
        std::vector<SharedPtr<A> > ptrs;
        {
            ptrs.reserve(5);
            for (int i = 0; i < 5; ++i) {
                A* a = new A(i + 200);
                ptrs.push_back(SharedPtr<A>(a));
            }
        }
        for (int i = 0; i < 5; ++i) {
            SharedPtr<A> ptr = ptrs[i];
            printf("vector2 ptr %d: %d\n", i, ptr->a);
        }
    }

    return 0;
}
