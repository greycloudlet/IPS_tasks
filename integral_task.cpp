#include <stdio.h>
#include <ctime>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_opadd.h>
#include <chrono>
#include <windows.h>

//#define SER

using namespace std::chrono;

// количество кусочков, на которые делить интервал
const int N = 100;

inline double fun_integral(double x) {
	return 8.0/(1.0+x*x);
}

//функция последовательного вычисления интеграла
double serial_int(double h, int n) {
	double sum = 0.0;
	for (int i = 0; i <= n; i++) {
		int k = 1;
		if (i == 0 || i == n) k = 2;
		sum += fun_integral(h*i) / k;
	}
	sum *= h;
	return sum;
}

//функция параллельного вычисления интеграла
double parallel_int(double h, int n) {
	cilk::reducer_opadd<double> parallel_sum(0.0);
	cilk_for(int i = 0; i <= n; ++i) {
		int k = 1;
		if (i == 0 || i == n) k = 2;
		*parallel_sum += fun_integral(h*i) / k;
	}
	return parallel_sum.get_value()*h;
}

int main()
{
	__cilkrts_set_param("nworkers", "16");
	for (int i = 10000; i < 1000000; i*=10)
	{
		int n = N*i;
		double h = 1.0 / n; //1 - is end of interval
		high_resolution_clock::time_point t1;
		high_resolution_clock::time_point t2;
#ifdef SER
		double serial_sum = 0.0;
		printf("%d", n);
		t1 = high_resolution_clock::now();

		serial_sum = serial_int(h, n);

		t2 = high_resolution_clock::now();
		duration<double> duration1 = (t2 - t1);
		printf("\nSerial duration: %lf\n", duration1.count());
		printf("Serial integral equal of %lf\n", serial_sum);
#endif
		double psum = 0.0;
		t1 = high_resolution_clock::now();

		psum = parallel_int(h, n);

		t2 = high_resolution_clock::now();
		duration<double> duration2 = (t2 - t1);
		printf("\nParallel duration: %lf\n", duration2.count());

		printf("Parallel integral equal of %lf\n", psum);
#ifdef SER
		printf("\nDuration ratio (P/S): %lf\n\n", duration2.count()/duration1.count());
#endif

}
	//getchar();
	return 0;
}