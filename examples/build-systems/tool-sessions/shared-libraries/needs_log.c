/* Uses a libm function but the library is not linked with -lm. */
#include <math.h>

double scaled_log(double x)
{
    return log(x) * 2.0;
}
