#include "fragmentation.h"
#include <fstream>

#include <iostream>

/*extern double g_l1_max;
extern const double g_l2_max;
extern const double g_l1_min;
extern const double g_l2_min;
extern const double g_l0;

extern const double g_precision;*/

/// параметры начальной прямоугольной области
const double g_l1_max = 12.0;
const double g_l2_max = g_l1_max;
const double g_l1_min = 8.0;
const double g_l2_min = g_l1_min;
const double g_l0 = 5.0;

/// точность аппроксимации рабочего пространства
const double g_precision = 0.25;

/// вектор, содержащий box-ы, являющиеся частью рабочего пространства
std::vector<Box> solution;
/// вектор, содержащий box-ы, не являющиеся частью рабочего пространства
std::vector<Box> not_solution;
/// вектор, содержащий box-ы, находящиеся на границе между "рабочим" и "нерабочим" пространством
std::vector<Box> boundary;
/// вектор, хранящий box-ы, анализируемые на следующей итерации алгоритма
std::vector<Box> temporary_boxes;



/// функции gj()
//------------------------------------------------------------------------------------------
double g1(double x1, double x2)
{
	return (x1*x1 + x2*x2 - g_l1_max*g_l1_max);
}

//------------------------------------------------------------------------------------------
double g2(double x1, double x2)
{
	return (g_l1_min*g_l1_min - x1*x1 - x2*x2);
}

//------------------------------------------------------------------------------------------
double g3(double x1, double x2)
{
	return ((x1-g_l0)*(x1 - g_l0) + x2*x2 - g_l2_max*g_l2_max);
}

//------------------------------------------------------------------------------------------
double g4(double x1, double x2)
{
	return (g_l2_min*g_l2_min - (x1 - g_l0)*(x1 - g_l0) - x2*x2);
}


//------------------------------------------------------------------------------------------
low_level_fragmentation::low_level_fragmentation(double & min_x, double& min_y, double& x_width, double& y_height) 
{
	current_box = Box( min_x, min_y, x_width, y_height );
}

//------------------------------------------------------------------------------------------
low_level_fragmentation::low_level_fragmentation(const Box& box)
{
	current_box = box;
}

//------------------------------------------------------------------------------------------
void low_level_fragmentation::VerticalSplitter(const Box& box, boxes_pair& vertical_splitter_pair)
{
	// необходимо определить функцию
	double min_x;
	double min_y;
	double x_width;
	double y_height;
	box.GetParameters(min_x, min_y, x_width, y_height);
	box.GetWidhtHeight(x_width, y_height);
	double w_half = x_width / 2;
	Box tmp_box_2 = Box(min_x+w_half, min_y, w_half,  y_height);
	Box tmp_box_1 = Box(min_x, min_y, w_half, y_height);
	vertical_splitter_pair = std::pair<Box, Box>(tmp_box_1, tmp_box_2);
}

//------------------------------------------------------------------------------------------
void low_level_fragmentation::HorizontalSplitter(const Box& box, boxes_pair& horizontal_splitter_pair)
{
	// необходимо определить функцию
	double min_x, min_y, x_width, y_height;
	box.GetParameters(min_x, min_y, x_width, y_height);
	box.GetWidhtHeight(x_width, y_height);
	double h_half = y_height / 2;
	Box tmp_box_1 = Box(min_x, min_y, x_width, h_half);
	Box tmp_box_2 = Box(min_x, min_y + h_half, x_width, h_half);
	horizontal_splitter_pair = std::pair<Box, Box>(tmp_box_1, tmp_box_2);
}

//------------------------------------------------------------------------------------------
void low_level_fragmentation::GetNewBoxes(const Box& box, boxes_pair& new_pair_of_boxes)
{
	// необходимо определить функцию
	//unsigned int lvl = FindTreeDepth();
	double x_width;
	double y_height;
	box.GetWidhtHeight(x_width, y_height);
	if (x_width>y_height) {
		VerticalSplitter(box, new_pair_of_boxes);
	}
	else {
		HorizontalSplitter(box, new_pair_of_boxes);
	}
}

//------------------------------------------------------------------------------------------
unsigned int low_level_fragmentation::FindTreeDepth()
{
	double box_diagonal = current_box.GetDiagonal();

	if (box_diagonal <= g_precision)
	{
		return 0;
	}
	else
	{
		boxes_pair new_boxes;
		// допустим, разобьем начальную область по ширине
		VerticalSplitter(current_box, new_boxes);
		unsigned int tree_depth = 1;

		box_diagonal = new_boxes.first.GetDiagonal();

		if (box_diagonal <= g_precision)
		{
			return tree_depth;
		}
		else
		{
			for (;;)
			{
				GetNewBoxes(new_boxes.first, new_boxes);
				++tree_depth;
				box_diagonal = new_boxes.first.GetDiagonal();

				if (box_diagonal <= g_precision)
				{
					break;
				}
			}
			return tree_depth;
		}
	}
}

//------------------------------------------------------------------------------------------
// функция ClasifyBox() анализирует box и классифицирует его
int low_level_fragmentation::ClasifyBox(const min_max_vectors& vects)
{
	// необходимо определить функцию
	bool proc, inrange;
	proc = false;
	inrange = true;
	int i = 0;
	while (inrange && i<vects.first.size()){
		double val = vects.first[i];
		//std::cout << "val min" << val<<std::endl;
		if (val > 0) inrange = false; //значит, вне области, не решение, выброс, (5) удовлетворено
		i++;
	}
	if (inrange) {
		i = 0;
		bool flag = true;
		while (flag && i<vects.second.size()) {
			double val = vects.second[i];
			if (val > 0) flag = false; //еще не решение, продолжить поиск, не уд. (4)
			i++;
		}
		if (flag) inrange = true; //решение
		else {
			proc = true;   
			inrange = false;
		}
	}
	
	int res = proc?2:inrange?1:0;
	//std::cout << res<<std::endl;
	return res;
}

//------------------------------------------------------------------------------------------
// Функция GetBoxType() добавляет классифицированный ранее box во множество решений, 
// или удаляет его из анализа, или добавляет его к граничной области, 
// или относит его к тем, что подлежат дальнейшему анализу
void low_level_fragmentation::GetBoxType(const Box& box)
{
	// необходимо определить функцию
	min_max_vectors vects;
	GetMinMax(box, vects);
	int b_type = ClasifyBox(vects);
	switch (b_type) {
	case 0:
		not_solution.push_back(box);
		break;
	case 1:
		solution.push_back(box);
		break;
	case 2:
	{
		boxes_pair new_boxes;
		GetNewBoxes(box, new_boxes);
		if (new_boxes.first.GetDiagonal() < g_precision) {
			boundary.push_back(new_boxes.first);
			boundary.push_back(new_boxes.second);
		}
		else{
			temporary_boxes.push_back(new_boxes.first);
			temporary_boxes.push_back(new_boxes.second);
		}
		break;
	}
	default:
		break;
	}
}


//------------------------------------------------------------------------------------------
high_level_analysis::high_level_analysis( double& min_x, double& min_y, double& x_width, double& y_height ) :
					low_level_fragmentation(min_x, min_y, x_width, y_height) {}

//------------------------------------------------------------------------------------------
high_level_analysis::high_level_analysis( Box& box ) : low_level_fragmentation( box ) {}

//------------------------------------------------------------------------------------------
void high_level_analysis::GetMinMax( const Box& box, min_max_vectors& min_max_vecs )
{
	std::vector<double> g_min;
	std::vector<double> g_max;

	double a1min, a2min, a1max, a2max;
	double xmin, xmax, ymin, ymax;

	box.GetParameters(xmin, ymin, xmax, ymax);

	xmax = xmin + xmax;
	ymax = ymin + ymax;

	double curr_box_diagonal = box.GetDiagonal();

	if (curr_box_diagonal <= g_precision)
	{
		g_min.push_back(0);
		g_max.push_back(0);

		min_max_vecs.first = g_min;
		min_max_vecs.second = g_max;

		return;
	}

	// MIN
	// функция g1(x1,x2)
	a1min = __min(abs(xmin), abs(xmax));
	a2min = __min(abs(ymin), abs(ymax));
	g_min.push_back(g1(a1min, a2min));

	// функция g2(x1,x2)
	a1min = __max(abs(xmin), abs(xmax));
	a2min = __max(abs(ymin), abs(ymax));
	g_min.push_back(g2(a1min, a2min));

	// функция g3(x1,x2)
	a1min = __min(abs(xmin - g_l0), abs(xmax - g_l0));
	a2min = __min(abs(ymin), abs(ymax));
	g_min.push_back(g3(a1min, a2min));

	// функция g4(x1,x2)
	a1min = __max(abs(xmin - g_l0), abs(xmax - g_l0));
	a2min = __max(abs(ymin), abs(ymax));
	g_min.push_back(g4(a1min, a2min));

	// MAX
	// функция g1(x1,x2)
	a1max = __max(abs(xmin), abs(xmax));
	a2max = __max(abs(ymin), abs(ymax));
	g_max.push_back(g1(a1max, a2max));

	// функция g2(x1,x2)
	a1max = __min(abs(xmin), abs(xmax));
	a2max = __min(abs(ymin), abs(ymax));
	g_max.push_back(g2(a1max, a2max));

	// функция g3(x1,x2)
	a1max = __max(abs(xmin - g_l0), abs(xmax - g_l0));
	a2max = __max(abs(ymin), abs(ymax));
	g_max.push_back(g3(a1max, a2max));

	// функция g4(x1,x2)
	a1max = __min(abs(xmin - g_l0), abs(xmax - g_l0));
	a2max = __min(abs(ymin), abs(ymax));
	g_max.push_back(g4(a1max, a2max));

	min_max_vecs.first = g_min;
	min_max_vecs.second = g_max;
}

//------------------------------------------------------------------------------------------
void high_level_analysis::GetSolution()
{
	/*int its = FindTreeDepth();
	if (its == 0) solution.push_back(current_box);
	else {
		temporary_boxes.push_back(current_box);
		for (int i = 0; i < its; i++) {
			int node_size = temporary_boxes.size();
			std::cout << node_size;
			//bool last = (its - 1 == i);
			for (int j = 0; j < node_size; j++) {
				GetBoxType(temporary_boxes.at(j));
			}
			temporary_boxes.erase(temporary_boxes.begin(), temporary_boxes.begin()+node_size); 
			std::cout << i;
		}
	}*/
	//int its = FindTreeDepth();
	double diag = current_box.GetDiagonal();
	std::cout << diag<<std::endl;
	int i = 0;
	if (diag<=g_precision) solution.push_back(current_box);
	else {
		temporary_boxes.push_back(current_box);
		while (diag>g_precision){
			int node_size = temporary_boxes.size();
			//std::cout <<"node_size"<< node_size<<std::endl;
			//bool last = (its - 1 == i);
			for (int j = 0; j < node_size; j++) {
				GetBoxType(temporary_boxes.at(j));
			}
			std::cout << temporary_boxes.size()<<std::endl;
			temporary_boxes.erase(temporary_boxes.begin(), temporary_boxes.begin() + node_size);
			if(!temporary_boxes.empty()) {
				diag = temporary_boxes.at(0).GetDiagonal();
				std::cout << diag << std::endl;
			}
			else {
				std::cout << "finish"<<std::endl;
				break;
			}
			std::cout << i;
		}
	}
}


//------------------------------------------------------------------------------------------
void WriteResults( const char* file_names[] )
{
	// необходимо определить функцию
	std::ofstream out;    // поток для записи
	out.open(file_names[0]); // окрываем файл для записи
	double x_min, y_min, width, height;
	Box tmp_box;
	if (out.is_open())
	{
		for (int i = 0; i < solution.size(); i++)
		{
			tmp_box = solution.at(i);
			tmp_box.GetParameters(x_min, y_min, width, height);
			out << x_min << " " << y_min << " " << width << " " << height << std::endl;
		}
	}

	std::cout << std::endl;
	out.close();

	out.open(file_names[1]); // окрываем файл для записи
	if (out.is_open())
	{
		for (int i = 0; i < not_solution.size(); i++)
		{
			tmp_box = not_solution.at(i);
			tmp_box.GetParameters(x_min, y_min, width, height);
			out << x_min<< " " << y_min << " " << width << " " << height << std::endl;
		}
	}
	std::cout << std::endl;
	out.close();

	out.open(file_names[2]); // окрываем файл для записи
	if (out.is_open())
	{
		for (int i = 0; i < boundary.size(); i++)
		{
			tmp_box = boundary.at(i);
			tmp_box.GetParameters(x_min, y_min, width, height);
			out << x_min << " " << y_min << " " << width << " " << height << std::endl;
		}
	}
	std::cout << std::endl;
	out.close();

}