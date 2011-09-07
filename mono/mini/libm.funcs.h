/* produced by trampoline generator using libm.funcs Fri Sep 10 16:17:32 +0300 2010, PLEASE DO NOT MODIFY */
#ifndef LIBM_USED
#define LIBM_USED
#define LIBM_ATTRIBUTE __attribute__((pcs("aapcs")))
double soft_atan2(double arg0, double arg1) LIBM_ATTRIBUTE;
float soft_logf(float arg0) LIBM_ATTRIBUTE;
double soft_fmod(double arg0, double arg1) LIBM_ATTRIBUTE;
double soft_sqrt(double arg0) LIBM_ATTRIBUTE;
double soft_acos(double arg0) LIBM_ATTRIBUTE;
double soft_atan(double arg0) LIBM_ATTRIBUTE;
double soft_sin(double arg0) LIBM_ATTRIBUTE;
long int soft_lrint(double arg0) LIBM_ATTRIBUTE;
long int soft_lrintf(float arg0) LIBM_ATTRIBUTE;
double soft_cos(double arg0) LIBM_ATTRIBUTE;
double soft_ceil(double arg0) LIBM_ATTRIBUTE;
double soft_floor(double arg0) LIBM_ATTRIBUTE;
float soft_floorf(float arg0) LIBM_ATTRIBUTE;
double soft_tan(double arg0) LIBM_ATTRIBUTE;
double soft_modf(double arg0, double* arg1) LIBM_ATTRIBUTE;
double soft_frexp(double arg0, int* arg1) LIBM_ATTRIBUTE;
double soft_asin(double arg0) LIBM_ATTRIBUTE;
double soft_exp(double arg0) LIBM_ATTRIBUTE;
double soft_log(double arg0) LIBM_ATTRIBUTE;
void soft_sincos(double arg0, double* arg1, double* arg2) LIBM_ATTRIBUTE;
double soft_pow(double arg0, double arg1) LIBM_ATTRIBUTE;

#ifndef LIBM_IMPLEMENTATION
#define atan2	soft_atan2
#define logf	soft_logf
#define fmod	soft_fmod
#define sqrt	soft_sqrt
#define acos	soft_acos
#define atan	soft_atan
#define sin	soft_sin
#define lrint	soft_lrint
#define lrintf	soft_lrintf
#define cos	soft_cos
#define ceil	soft_ceil
#define floor	soft_floor
#define floorf	soft_floorf
#define tan	soft_tan
#define modf	soft_modf
#define frexp	soft_frexp
#define asin	soft_asin
#define exp	soft_exp
#define log	soft_log
#define sincos	soft_sincos
#define pow	soft_pow
#endif /* LIBM_IMPLEMENTATION */

#endif /* LIBM_USED */
