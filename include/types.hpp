#pragma once
template <unsigned long long x>
using u = unsigned _BitInt(x);

template <unsigned long long x>
using s = signed _BitInt(x);

#define unwrap(a, b) using a ## b = a<b>

unwrap(u, 8);
unwrap(u, 16);
unwrap(u, 32);
unwrap(u, 64);
unwrap(u, 128);


unwrap(s, 8);
unwrap(s, 16);
unwrap(s, 32);
unwrap(s, 64);
unwrap(s, 128);

#undef unwrap

using b = u<1>;

using usize = u<sizeof(void*) * 8>;

using f32 = float;
using f64 = double;

template<typename T>
struct buf_t
{
    T *arr;
    usize num;
    inline T& operator[](usize right)
    {
        return arr[right];
    }
};


template<typename T, usize num>
struct vec_t
{
    T arr[num];
    
    inline T& operator[](usize right)
    {
        return arr[right];
    }
    template<typename O>
    inline vec_t<O, num> operator()()
    {
        vec_t<O, num> arr{};
        for(usize i = 0; i < num; i++)
        {
            arr[i] = (T) this[i];
        }
        return arr;
    }

    // vector binary
    inline vec_t<T, num> operator*(vec_t<T, num> a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] * a[i];
        }
        return res;
    }
    inline vec_t<T, num> operator+(vec_t<T, num> a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] + a[i];
        }
        return res;
    }
    inline vec_t<T, num> operator/(vec_t<T, num> a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] / a[i];
        }
        return res;
    }
    inline vec_t<T, num> operator-(vec_t<T, num> a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] - a[i];
        }
        return res;
    }

    // single binary
    inline vec_t<T, num> operator*(T a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] * a;
        }
        return res;
    }
    inline vec_t<T, num> operator+(T a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] + a;
        }
        return res;
    }
    inline vec_t<T, num> operator/(T a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] / a;
        }
        return res;
    }
    inline vec_t<T, num> operator-(T a)
    {
        vec_t<T, num> res{};
        for(usize i = 0; i < num; i++)
        {
            res[i] = arr[i] - a;
        }
        return res;
    }

    // vector double
    inline vec_t<T, num>& operator*=(vec_t<T, num> a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] *= a[i];
        }
        return *this;
    }
    inline vec_t<T, num>& operator+=(vec_t<T, num> a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] += a[i];
        }
        return *this;
    }
    inline vec_t<T, num>& operator/=(vec_t<T, num> a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] /= a[i];
        }
        return *this;
    }
    inline vec_t<T, num>& operator-=(vec_t<T, num> a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] -= a[i];
        }
        return *this;
    }

    // single double
    inline vec_t<T, num>& operator*=(T a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] *= a;
        }
        return *this;
    }
    inline vec_t<T, num>& operator+=(T a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] += a;
        }
        return *this;
    }
    inline vec_t<T, num>& operator/=(T a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] /= a;
        }
        return *this;
    }
    inline vec_t<T, num>& operator-=(T a)
    {
        for(usize i = 0; i < num; i++)
        {
            (*this)[i] -= a;
        }
        return *this;
    }
};

template<typename T, usize num>
static inline T dot(vec_t<T, num> a, vec_t<T, num> b)
{
    T x{};
    for(usize i = 0; i < num; i++)
    {
        x += a[i] * b[i];
    }
    return x;
}

template<typename T, usize num>
static inline T smag(vec_t<T, num> a)
{
    return dot(a, a);
}

template<typename T, usize y, usize x>
using mat_t = vec_t<vec_t<T, x>, y>;

template<usize y, usize x> 
using matNxNf = mat_t<f32, y, x>;

template<usize y, usize x> 
using matNxNd = mat_t<f64, y, x>;

template<usize num> 
using vecNf = vec_t<f32, num>;

template<usize num> 
using vecNd = vec_t<f64, num>;

#define vecFV(N) using vec ## N ## f = vecNf<N>
#define vecDV(N) using vec ## N ## d = vecNd<N>

vecFV(1);
vecFV(2);
vecFV(3);
vecFV(4);

vecDV(1);
vecDV(2);
vecDV(3);
vecDV(4);

#undef vecFV
#undef vecDV

