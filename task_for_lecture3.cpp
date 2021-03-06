#include <stdio.h>
#include <ctime>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_opadd.h>
#include <chrono>

using namespace std::chrono;

// количество строк в исходной квадратной матрице
const int MATRIX_SIZE = 1500;

/// Функция InitMatrix() заполняет переданную в качестве 
/// параметра квадратную матрицу случайными значениями
/// matrix - исходная матрица СЛАУ
void InitMatrix( double** matrix1, double** matrix2)
{
	for ( int i = 0; i < MATRIX_SIZE; ++i )
	{
		matrix1[i] = new double[MATRIX_SIZE + 1];
		matrix2[i] = new double[MATRIX_SIZE + 1];
	}

	for ( int i = 0; i < MATRIX_SIZE; ++i )
	{
		for ( int j = 0; j <= MATRIX_SIZE; ++j )
		{
			matrix1[i][j] = rand() % 2500 + 1;
			matrix2[i][j] = matrix1[i][j];
		}
	}
}

/// Функция SerialGaussMethod() решает СЛАУ методом Гаусса 
/// matrix - исходная матрица коэффиициентов уравнений, входящих в СЛАУ,
/// последний столбей матрицы - значения правых частей уравнений
/// rows - количество строк в исходной матрице
/// result - массив ответов СЛАУ
double SerialGaussMethod( double **matrix, const int rows, double* result )
{
	int k;
	double koef;

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	// прямой ход метода Гаусса
	for ( k = 0; k < rows; ++k )
	{
		//
		for (int i = k + 1; i < rows; ++i ) //здесь k - столбец (осн), i - строка
		{
			koef = -matrix[i][k] / matrix[k][k];

			for(int j = k; j <= rows; ++j )//здесь k - строка, j - столбец
			{
				matrix[i][j] += koef * matrix[k][j];
			}
		}
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> duration1 = (t2 - t1);
	printf("\nSerial duration forward stroke: %lf\n", duration1.count());

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for ( k = rows - 2; k >= 0; --k )
	{
		result[k] = matrix[k][rows];
		//
		for(int j = k + 1; j < rows; ++j )
		{
			result[k] -= matrix[k][j] * result[j];			
		}

		result[k] /= matrix[k][k];
	}
	return duration1.count();
}


double ParallelGaussMethod(double **matrix, const int rows, double* result)
{
	int k;	

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	// прямой ход метода Гаусса
	for (k = 0; k < rows; ++k)
	{
		//
		cilk_for (int i = k + 1; i < rows; ++i) //здесь k - столбец (осн), i - строка
		{
			double koef = -matrix[i][k] / matrix[k][k];
		
			for(int j = k; j <= rows; ++j)//здесь k - строка, j - столбец
			{
				matrix[i][j] += koef * matrix[k][j];
			}
		}
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> duration1 = (t2 - t1);
	printf("\nParallel duration forward stroke: %lf\n", duration1.count());

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for (k = rows - 2; k >= 0; --k)
	{
		//result[k] = matrix[k][rows];
		cilk::reducer< cilk::op_add<double> > result_p(matrix[k][rows]);
		//
		cilk_for(int j = k + 1; j < rows; ++j)
		{
			//result[k] -= matrix[k][j] * result[j];
			*result_p += (-1)*matrix[k][j] * result[j];
		}

	    result[k] = result_p.get_value();
		result[k] /= matrix[k][k];
	}
	return duration1.count();
}


int main()
{
	//srand( (unsigned) time( 0 ) );
	srand(0);
	__cilkrts_set_param("nworkers", "4");


	int i;

#ifdef TEST
	// кол-во строк в матрице, приводимой в качестве примера
	const int test_matrix_lines = 4;

	double **test_matrix = new double*[test_matrix_lines];

	// цикл по строкам
	for ( i = 0; i < test_matrix_lines; ++i )
	{
		// (test_matrix_lines + 1)- количество столбцов в тестовой матрице,
		// последний столбец матрицы отведен под правые части уравнений, входящих в СЛАУ
		test_matrix[i] = new double[test_matrix_lines + 1];
	}

	// инициализация тестовой матрицы
	test_matrix[0][0] = 2; test_matrix[0][1] = 5;  test_matrix[0][2] = 4;  test_matrix[0][3] = 1;  test_matrix[0][4] = 20;
	test_matrix[1][0] = 1; test_matrix[1][1] = 3;  test_matrix[1][2] = 2;  test_matrix[1][3] = 1;  test_matrix[1][4] = 11;
	test_matrix[2][0] = 2; test_matrix[2][1] = 10; test_matrix[2][2] = 9;  test_matrix[2][3] = 7;  test_matrix[2][4] = 40;
	test_matrix[3][0] = 3; test_matrix[3][1] = 8;  test_matrix[3][2] = 9;  test_matrix[3][3] = 2;  test_matrix[3][4] = 37;
#else
	const int test_matrix_lines = MATRIX_SIZE;

	double **test_matrix = new double*[test_matrix_lines];

	double **test_matrix_с = new double*[test_matrix_lines];

	InitMatrix(test_matrix, test_matrix_с);	
#endif

	
	// массив решений СЛАУ
	double *result1 = new double[test_matrix_lines];
	double *result2 = new double[test_matrix_lines];

	double diff = ParallelGaussMethod(test_matrix, test_matrix_lines, result1);
	diff /= SerialGaussMethod(test_matrix_с, test_matrix_lines, result2);
	printf("\nParallel/serial duration forward stroke: %lf\n", diff);

	for ( i = 0; i < test_matrix_lines; ++i )
	{
		delete[]test_matrix[i];
	}

	for (i = 0; i < test_matrix_lines; ++i)
	{
		delete[]test_matrix_с[i];
	}

	printf( "Parall solution:\n" );

	for ( i = 0; i < 10; ++i )
	{
		printf( "x(%d) = %lf\n", i, result1[i] );
	}

	printf("Serial solution:\n");

	for (i = 0; i < 10; ++i)
	{
		printf("x(%d) = %lf\n", i, result2[i]);
	}

	delete[] result1;
	delete[] result2;

	getchar();
	printf("end");

	return 0;
}