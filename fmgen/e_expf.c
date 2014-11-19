#include <stdint.h>

typedef union _f32 {
    float f;
    uint32_t i;
} _f32;

/* Constants for expf(x) -- single-precision exponential function evaluated at x (i.e. e^x). */
static const float
halF[2] = {0.5,-0.5,},
ln2HI[2]   ={ 6.9313812256e-01,     /* 0x3f317180 */
         -6.9313812256e-01,},   /* 0xbf317180 */
ln2LO[2]   ={ 9.0580006145e-06,     /* 0x3717f7d1 */
         -9.0580006145e-06,},   /* 0xb717f7d1 */
ln2    = 0.69314718246,
invln2 =  1.4426950216,         /* 0x3fb8aa3b */
T_P1   =  9.999999951334e-01,
T_P2   =  5.0000022321103498e-01,
T_P3   =  1.66662832359450473e-01,
T_P4   =  4.16979462133574885e-02,
T_P5   =  8.2031098e-03;

float expf(float x) /* default IEEE double exp */
{
    _f32 tmp;
    float y,hi,lo,c,t;
    int32_t k,xsb;
    uint32_t hx;

    tmp.f = x;
    hx = tmp.i;
    xsb = (hx>>31)&1;       /* sign bit of x */
    hx &= 0x7fffffff;       /* high word of |x| */

    /* filter out non-finite argument */
    if(hx >= 0x42b17218) {              /* if |x|>=88.721... */
        if(hx>0x7f800000)
            return x+x;                 /* NaN */
        if(hx==0x7f800000)
            return (xsb==0)? x:0.0f;    /* exp(+-inf)={inf,0} */
    }

    /* argument reduction */
    if(hx > 0x3eb17218) {       /* if  |x| > 0.5 ln2 */
        if(hx < 0x3F851592) {   /* and |x| < 1.5 ln2 */
            hi = x-ln2HI[xsb]; lo=ln2LO[xsb]; k = 1-xsb-xsb;
        } else {
            k  = invln2*x+halF[xsb];
            t  = k;
            hi = x - t*ln2HI[0];    /* t*ln2HI is exact here */
            lo = t*ln2LO[0];
        }
        x  = hi - lo;
    }
    else k = 0;

    /* x is now in primary range */
    c = x*(T_P1+x*(T_P2+x*(T_P3+x*(T_P4+x*T_P5))));
    y = 1.0f+c;
    {
        _f32 hy;
        hy.f = y;
        hy.i += (k << 23); /* add k to y\s exponent */
        return hy.f;
    }
}

