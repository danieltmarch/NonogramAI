//the class for a nonogram
#ifndef NONOGRAM_H
#define NONOGRAM_H

#include <iostream>
#include <vector>
using std::vector;
using std::ostream;

class Nonogram
{
	public:
		Nonogram(int len, int wid); //constructor
		Nonogram(vector<vector<int>> rows, vector<vector<int>> columns); //set constructor (from labels)
		Nonogram(const Nonogram& n); //copy constructor
		~Nonogram(); //destructor
		
		char* operator[](const int i) { return grid[i]; } //i.e. nono[2][3] = '-';
		
		friend ostream& operator<<(ostream& outstream, const Nonogram& n); //print option

		bool isSolved() const;
		Nonogram& operator=(const Nonogram& n);
		bool operator==(const Nonogram n) const;

		void clearGrid(); //remove the '-' and 'X' 's. 

		int getWidth() const { return w; }
		int getHeight() const { return h; }
		vector<int> getColumn(int i) const { return column[i]; } //i.e. vector<int> numList = nono.getColumn(2);
		vector<int> getRow(int i) const { return row[i]; }
	private:
		int w = 0; //width
		int h = 0; //height
		char** grid = 0; //each square is marked with ' ' empty, '-' not possible, or 'X'  filled.
		vector<int>* column = 0; //array of the column labels
		vector<int>* row = 0; //array of the row labels
};

#endif