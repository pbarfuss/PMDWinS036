#include <stdint.h>
#include <math.h>

typedef union _f32 {
    float f;
    uint32_t i;
} _f32;

/* Constants for expf(x) -- single-precision exponential function evaluated at x (i.e. e^x). */
static const float
halF[2] = {0.5,-0.5,},
ln2    =  0.69314718246,
invln2 =  1.4426950216,         /* 0x3fb8aa3b */
T_P3   =  1.66662832359450473e-01,
T_P4   =  4.16979462133574885e-02,
T_P5   =  8.2031098e-03;

float expf(float x) /* default IEEE double exp */
{
    _f32 tmp, hy;
    int32_t k = 0;

    /* filter out non-finite argument */
    if (x != x) return x;
    tmp.f = x;
    tmp.i &= 0x7fffffff;       /* high word of |x| */
    if(tmp.i >= 0x42b17218) {              /* if |x|>=88.721... */
        if(tmp.i==0x7f800000)
            return (x < 0.0f)? x:0.0f;    /* exp(+-inf)={inf,0} */
        return x;
    }

    /* argument reduction */
    if(tmp.i > 0x3eb17218) {       /* if  |x| > 0.5 ln2 */
        k = lrintf(invln2*x);
        x = (x - k*ln2);
    }

    /* x is now in primary range */
    hy.f = 1.0f+x*(1.0f+x*(0.5f+x*(T_P3+x*(T_P4+x*T_P5))));
    hy.i += (k << 23); /* add k to y\s exponent */
    return hy.f;
}

