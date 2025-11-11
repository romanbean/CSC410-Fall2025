#include <stdio.h>

// default size 100,000
// #define N 100000 // intervals

// user determined size 100,000,000,000
#define N 100000000000

double f(double x) {
    return 4.0 / (1.0 + x * x); // Function to integrate
}

double trapezoidalRule() 
{
	double a = 0.0;
	double b = 1.0;
	int n = N;

	double h = (b-a) / n;
	double sum = (f(a) + f(b)) / 2.0;

	for (int i = 1; i < n; i++) {
		double x = a + i * h;
		sum += f(x);
	}
	return sum * h;
}

int main() {
    double pi = trapezoidalRule();
    printf("Estimated value of π: %f\n", pi);
    return 0;
}
