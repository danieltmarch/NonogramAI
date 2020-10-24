#include "Nonogram.h"
#include <iomanip>
using namespace std;

Nonogram::Nonogram(int width, int height)
{
	srand(time(0)); //set random seed to system time

	w = width;
	h = height;
	column = new vector<int>[w]; //reserve vectors
	row = new vector<int>[h];

	grid = new char* [w]; //allocate the grid
	for (int i = 0; i < w; i++)
		grid[i] = new char[h];

	//fill the grid randomly
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
			grid[x][y] = 32 + 56 * (rand() % 2); //fill grid randomly, ' ' = 32, 'X' = 88, either ' ' or '@'

	//determine the row and column vectors
	for (int x = 0; x < w; x++) //fill the column vector
	{
		int streak = 0;
		for (int y = 0; y < h; y++)
		{
			if (grid[x][y] == 'X')
				streak++;
			else if (streak > 0) //streak broken
			{
				column[x].push_back(streak);
				streak = 0; //reset streak
			}
		}
		if (streak != 0)
			column[x].push_back(streak);
	}
	for (int y = 0; y < h; y++) //fill the row vector
	{
		int streak = 0;
		for (int x = 0; x < w; x++)
		{
			if (grid[x][y] == 'X')
				streak++;
			else if (streak > 0) //streak broken
			{
				row[y].push_back(streak);
				streak = 0; //reset streak
			}
		}
		if (streak != 0)
			row[y].push_back(streak);
	}

	/* reset the grid to an empty array, currently disabled
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
			grid[x][y] = ' ';
	//*/

} //constructor, allocate and make random puzzle //default constructor

Nonogram::Nonogram(const Nonogram& n) //copy constructor
{
	//copy over the size
	w = n.w;
	h = n.h;

	column = new vector<int>[w]; //reserve vectors
	row = new vector<int>[h];

	grid = new char* [w]; //allocate the grid
	for (int i = 0; i < w; i++)
		grid[i] = new char[h];

	for (int x = 0; x < w; x++) //copy the grid
		for (int y = 0; y < h; y++)
			grid[x][y] = n.grid[x][y];

	//copy over the labels
	for (int x = 0; x < w; x++)
		column[x] = n.column[x];
	for (int y = 0; y < h; y++)
		row[y] = n.row[y];
}

//construct from vector labels
Nonogram::Nonogram(vector<vector<int>> rows, vector<vector<int>> columns)
{
	//copy over the size
	w = columns.size();
	h = rows.size();

	column = new vector<int>[w]; //reserve vectors
	row = new vector<int>[h];

	for (int x = 0; x < w; x++)
		column[x] = columns[x];
	for (int y = 0; y < h; y++)
		row[y] = rows[y];

	grid = new char* [w]; //allocate the grid
	for (int i = 0; i < w; i++)
		grid[i] = new char[h];

	for (int x = 0; x < w; x++) //set the grid to empty
		for (int y = 0; y < h; y++)
			grid[x][y] = ' ';
}

Nonogram::~Nonogram() //destructor
{
	for (int x = 0; x < w; x++)
		delete grid[x];
	delete grid;
} //destructor

//asssignment same as copy constructor
Nonogram& Nonogram::operator=(const Nonogram& n)
{
	//check for assignment to the same var
	if (this == &n)
		return *this;

	//do as the copy constructor would do
	//copy over the size
	w = n.w;
	h = n.h;

	column = new vector<int>[w]; //reserve vectors
	row = new vector<int>[h];

	grid = new char* [w]; //allocate the grid
	for (int i = 0; i < w; i++)
		grid[i] = new char[h];

	for (int x = 0; x < w; x++) //copy the grid
		for (int y = 0; y < h; y++)
			grid[x][y] = n.grid[x][y];

	//copy over the labels
	for (int x = 0; x < w; x++)
		column[x] = n.column[x];
	for (int y = 0; y < h; y++)
		row[y] = n.row[y];

	//return the object in case another assignment
	return *this;
}

ostream& operator<<(ostream& outstream, const Nonogram& n) // print. for ease, the label numbers are printed on the bottom and left
{
	int longestColumn = 0; // the most number labels per column, printing is difficult
	for (int x = 0; x < n.w; x++)
		if (n.column[x].size() > longestColumn)
			longestColumn = n.column[x].size();
	for (int y = 0; y < n.h; y++) // print row
	{
		for (int x = 0; x < n.w; x++) // print grid
			outstream << n.grid[x][y] << ' '; // width is always 2
		outstream << '|'; // grid and label seperator
		for (int i = 0; i < n.row[y].size(); i++) // print row labels
			outstream << n.row[y][i] << ' ';
		outstream << '\n';
	}
	for (int x = 0; x < n.w; x++) // grid and label seperator, each grid spot is 2 chars
		outstream << '-' << '-';
	outstream << endl;
	for (int i = 0; i < longestColumn; i++) //print column labels, if the number to be printed is 10 or more, it will be formatted poorly
	{
		for (int x = 0; x < n.w; x++)
			if (i < n.column[x].size()) // out of bounds check for printing label number
				outstream << setw(2) << left << n.column[x][i]; // print left align with size 2
			else
				outstream << ' ' << ' '; //spacing to avoid screwing up formatting
		outstream << '\n';
	}
	return outstream; // return the stream object for printing to non cout streams, i.e. cout << x << n1;
}

void Nonogram::clearGrid() // reset the grid
{
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
			grid[x][y] = ' ';
}

bool Nonogram::isSolved() const
{
	//determine the row and column vectors
	for (int x = 0; x < w; x++) //fill the column vector
	{
		vector<int> columnV;
		int streak = 0;
		for (int y = 0; y < h; y++)
		{
			if (grid[x][y] == 'X')
				streak++;
			else if (streak > 0) //streak broken
			{
				columnV.push_back(streak);
				streak = 0; //reset streak
			}
		}
		if (streak != 0)
			columnV.push_back(streak);
		if (columnV != column[x])
			return false;
	}

	for (int y = 0; y < h; y++) //fill the row vector
	{
		vector<int> rowV;
		int streak = 0;
		for (int x = 0; x < w; x++)
		{
			if (grid[x][y] == 'X')
				streak++;
			else if (streak > 0) //streak broken
			{
				rowV.push_back(streak);
				streak = 0; //reset streak
			}
		}
		if (streak != 0)
			rowV.push_back(streak);
		if (rowV != row[y])
			return false;
	}
	return true;
}

//careful with this, this assumes the boards are of the same dimensions, only checks the cells, this may crash if given incorrect sizes
bool Nonogram::operator==(const Nonogram n) const
{
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
			if (grid[x][y] != n.grid[x][y])
				return false;
	return true; //passed the test
}