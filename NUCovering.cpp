#include "fragmentation.h"
#include <locale.h>

/// ��������� ��������� ������������� �������
const double g_l1_max = 12.0;
const double g_l2_max = g_l1_max;
const double g_l1_min = 8.0;
const double g_l2_min = g_l1_min;
const double g_l0 = 5.0;

/// �������� ������������� �������� ������������
const double g_precision = 0.25;


int main()
{
	setlocale(LC_ALL,"Rus");

	high_level_analysis main_object;
	main_object.GetSolution();
	// ��������! ����� ���������� ���������� ���� �� �������� ������!
	const char* out_files[3] = { "", "", "" };
	WriteResults( out_files );

	return 0;
}