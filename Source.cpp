#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_max.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_vector.h>
#include <chrono>

using namespace std::chrono;

/// Функция ReducerMaxTest() определяет максимальный элемент массива,
/// переданного ей в качестве аргумента, и его позицию
/// mass_pointer - указатель исходный массив целых чисел
/// size - количество элементов в массиве
void ReducerMaxTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_max_index<long, int>> maximum;
	cilk_for(long i = 0; i < size; ++i)
	{
		maximum->calc_max(i, mass_pointer[i]);
	}
	printf("Maximal element = %d has index = %d\n",
		maximum->get_reference(), maximum->get_index_reference());
}

/// Функция ReducerMinTest() определяет минимальный элемент массива,
/// выполнена по аналогии с ReducerMaxTest()
void ReducerMinTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_min_index<long, int>> minimum;
	cilk_for(long i = 0; i < size; ++i)
	{
		minimum->calc_min(i, mass_pointer[i]);
	}
	printf("Minimum element = %d has index = %d\n",
		minimum->get_reference(), minimum->get_index_reference());
}

/// Функция ParallelSort() сортирует массив в порядке возрастания
/// begin - указатель на первый элемент исходного массива
/// end - указатель на последний элемент исходного массива
void ParallelSort(int *begin, int *end)
{
	if (begin != end)
	{
		--end;
		int *middle = std::partition(begin, end, std::bind2nd(std::less<int>(), *end));
		std::swap(*end, *middle);
		cilk_spawn ParallelSort(begin, middle);
		ParallelSort(++middle, ++end);
		cilk_sync;
	}
}


void CompareForAndCilk_For(size_t sz) {
	printf("\nFor size of vector %d\n", sz);
	//std::cout << "For size of vec:" << sz << std::endl;
	std::vector<float> std_vec;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (int i = 0; i < sz; i++) {
		std_vec.push_back(rand() % 20000 + 1);
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> duration1 = (t2 - t1);
	printf("\nDuration \"for\" for std vector: %lf", duration1.count());

	cilk::reducer<cilk::op_vector<int>>cilk_vec;
	t1 = high_resolution_clock::now();
	cilk_for(long i = 0; i < sz; ++i)
	{
		cilk_vec->push_back(rand() % 20000 + 1);
	}
	t2 = high_resolution_clock::now();
	duration<double> duration2 = (t2 - t1);
	printf("\nDuration \"for\" for cilk reducer vector: %lf\n", duration2.count());
	printf("\nDifference between duration \"for\" cilk and std: %lf\n", duration2.count()-duration1.count());
}

int main()
{
	srand((unsigned)time(0));

	// устанавливаем количество работающих потоков = 4
	__cilkrts_set_param("nworkers", "4");


	const long mass_size = 10000;
	int k[] = { 1, 10, 50, 100 };
	//int k[] = { 1 };
	int size_k = sizeof(k) / sizeof(k[0]);
	for (int j = 0; j < size_k; j++) {

		const long mass_size_new = mass_size*k[j];
		printf("For mass_size = %d \n", mass_size_new);
		long i;
		int *mass_begin, *mass_end;
		int *mass = new int[mass_size_new];

		for (i = 0; i < mass_size; ++i)
		{
			mass[i] = (rand() % 25000) + 1;
		}

		mass_begin = mass;
		mass_end = mass_begin + mass_size;
		ReducerMaxTest(mass, mass_size);
		ReducerMinTest(mass, mass_size);

		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		ParallelSort(mass_begin, mass_end);
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> duration = (t2 - t1);
		printf("\nDuration of sort is: %lf seconds\n\n", duration.count());
		//std::cout << std::endl;
		//std::cout << "Duration of sort is: " << duration.count() << " seconds" << std::endl;

		ReducerMaxTest(mass, mass_size);
		ReducerMinTest(mass, mass_size);

		delete[]mass;
		printf("-----------------------\n");
	}
	getchar();
	size_t sz_mass[] = { 1000000, 100000, 10000, 1000, 500, 100, 50, 10 };
	int size_sz_mass = sizeof(sz_mass) / sizeof(sz_mass[0]);
	for (int i = 0; i < size_sz_mass; i++) CompareForAndCilk_For(sz_mass[i]);

	getchar();
	return 0;
}