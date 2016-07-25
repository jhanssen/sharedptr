#include <SharedPtr.h>
#include <stdio.h>
#include <vector>

using namespace nonatomic;

class A : public enable_shared_from_this<A>
{
public:
    A(int a_)
        : a(a_)
    {
        printf("constructing A %d\n", a);
    }
    virtual ~A()
    {
        printf("destructing A %d\n", a);
    }

    int a;
};

class C : public A
{
public:
    C()
        : A(-1)
    {
    }
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
    //     shared_ptr<A> s1(new A(200));
    //     shared_ptr<A>& s2 = s1;
    //     printf("base: %d\n", s1->a);
    //     printf("ref: %d\n", s2->a);
    // }
    weak_ptr<A> w1;
    {
        A* aa = new A(10);
        shared_ptr<A> s1(aa);
        printf("s1: %d\n", s1->a);
        w1 = s1;
        {
            shared_ptr<A> s2 = s1;
            printf("s2: %d\n", s2->a);
        }
        if (shared_ptr<A> s3 = w1.lock()) {
            printf("w1: %d\n", s3->a);
        } else {
            printf("unable to lock w1\n");
        }
        if (shared_ptr<A> s4 = aa->shared_from_this()) {
            printf("got sharedFromThis: %d\n", s4->a);
        } else {
            printf("couldn't get sharedFromThis\n");
        }
    }
    if (shared_ptr<A> s3 = w1.lock()) {
        printf("w1: %d\n", s3->a);
    } else {
        printf("unable to lock w1\n");
    }

    {
        shared_ptr<B> sb1(new B(20));
        printf("sb1: %d\n", sb1->b);
    }

    {
        std::vector<shared_ptr<A> > ptrs;
        {
            shared_ptr_pool_scope<A> pool = shared_ptr<A>::make_pool(5);
            ptrs.reserve(5);
            for (int i = 0; i < 5; ++i) {
                A* a = new(pool.mem(i)) A(i + 100);
                ptrs.push_back(shared_ptr<A>(pool, i, a));
            }
            printf("pool count (should be 6?) %u\n", pool.use_count());
        }
        for (int i = 0; i < 5; ++i) {
            shared_ptr<A> ptr = ptrs[i];
            printf("vector1 ptr %d: %d\n", i, ptr->a);
        }
    }

    {
        std::vector<shared_ptr<A> > ptrs;
        {
            ptrs.reserve(5);
            for (int i = 0; i < 5; ++i) {
                A* a = new A(i + 200);
                ptrs.push_back(shared_ptr<A>(a));
            }
        }
        for (int i = 0; i < 5; ++i) {
            shared_ptr<A> ptr = ptrs[i];
            printf("vector2 ptr %d: %d\n", i, ptr->a);
        }
    }

    {
        shared_ptr<C> c(new C);
        shared_ptr<A> a = c;
        //shared_ptr<B> b = c;
    }

    return 0;
}
