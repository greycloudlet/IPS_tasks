#include "fragmentation.h"
#include <locale.h>
#include "stdafx.h"

const double g_l1_max = 12.0;
const double g_l2_max = g_l1_max;
const double g_l1_min = 8.0;
const double g_l2_min = g_l1_min;
const double g_l0 = 5.0;

int main()
{
	setlocale(LC_ALL,"Rus");

	double x_min = -g_l1_min;
	double y_min = 0;
	double width = g_l1_max + g_l2_max + g_l0;
	double height = g_l1_max;

	high_level_analysis main_object(x_min, y_min, width, height);
	//high_level_analysis main_object;
	main_object.GetSolution();
	// Внимание! здесь необходимо определить пути до выходных файлов!
	const char* out_files[3] = { "D:\\solution.txt", "D:\\not_solution.txt", "D:\\boundary.txt" };
	WriteResults( out_files );
	getchar();

	return 0;
}