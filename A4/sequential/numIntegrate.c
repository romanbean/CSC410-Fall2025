#include <stdio.h>
#include <omp.h>

#define N 10000000000LL  // 10^10 intervals, adjust as needed

double f(double x) {
    return 4.0 / (1.0 + x * x);
}

double trapezoidalRule() 
{
    double a = 0.0, b = 1.0;
    long long int n = N;
    double h = (b - a) / n;
    double sum = (f(a) + f(b)) / 2.0;

    double sum_local = 0.0;
    double c = 0.0; // Kahan summation compensation

    #pragma omp parallel num_threads(1)
    {
        double local_sum = 0.0;
        #pragma omp for nowait
        for (long long int i = 1; i < n; i++) {
            double x = a + i * h;
            local_sum += f(x);
        }
        #pragma omp critical
        {
            double y = local_sum - c;
            double t = sum_local + y;
            c = (t - sum_local) - y;
            sum_local = t;
        }
    }

    sum += sum_local;

    return sum * h;
}

int main() {
    double pi = trapezoidalRule();
    printf("Estimated value of π: %.15f\n", pi);
    return 0;
}
