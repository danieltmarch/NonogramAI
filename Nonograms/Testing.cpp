#include "Nonogram.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <set>
#include <map>
#include <chrono>
#include <random>
#include <string>
using namespace std;
using chrono::high_resolution_clock;

//funcion headers
//setup functions
set< vector<int> > getLineSetRecursive(int width, int sum);
set< vector<int> > getLineSet(int width, int sum);

vector<char> decodeLineSet(vector<int> gaps, vector<int> streak, int width);

int getWidth(vector<int> streaks);
int getSum(vector<int> streaks, int width, int lineWidth);

vector<set<vector<char>>> getRowOptions(const Nonogram& n);
vector<set<vector<char>>> getColumnOptions(const Nonogram& n);

//Constraint Satisfaction Problem functions
bool revise(int sourceIndex, int destIndex, set<vector<char>>& sourceDomain, const set<vector<char>>& destDomain); //restrict domain of row/column based on a different column/row (works either way)

struct arcType   //a structure for a queue
{
	int source;
	int destination;
	bool sourceIsRow; //if the source element is a row/column and the destination is a column/row
};

bool arcConsistency(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain); //check for arc consistency and restricts domains
bool domainsAreSingular(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign); //basically if the domain infers we have a solution

void backtrack(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign);
void assignRowBacktrack(int rowIndex, vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign);
void assignColumnBacktrack(int columnIndex, vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign);

bool solve(Nonogram& n);
//end function headers




//setup functions
set< vector<int> > getLineSet(int width, int sum)
{
	set<vector<int>> options = getLineSetRecursive(width, sum); //basic sets
	set<vector<int>> formattedSet; //the non first last elmements in each vector be added by one
	for (vector<int> option : options)
	{
		for (int i = 1; i < option.size() - 1; i++) //every element but the 0th and last
			++option[i];
		formattedSet.insert(option);
	}
	return formattedSet;
}
set< vector<int> > getLineSetRecursive(int width, int sum)
{
	set<vector<int>> options;
	if (width == 1) //end case
	{
		options.insert( { sum } );
		return options;
	}

	for (int i = 0; i <= sum; i++) //recursive
	{
		set<vector<int>> subSet = getLineSetRecursive(width-1, sum-i);
		for (vector<int> subOption : subSet)
		{
			vector<int> option = { i };
			option.reserve(1 + subOption.size()); // preallocate memory
			option.insert(option.end(), subOption.begin(), subOption.end()); //add sub vector to the end of the vector
			options.insert(option);
		}
	}
	return options;
}

vector<char> decodeLineSet(vector<int> gaps, vector<int> streak, int width)
{
	vector<char> line(width, ' '); //empty line
	int i = 0;
	int streakIndex = 0;
	int gapIndex = 0;
	while(gapIndex < gaps.size() && streakIndex < streak.size() && i < width)
	{
		i = i + gaps[gapIndex]; //skip the gap

		int startStreakIndex = i;
		for (; i < startStreakIndex + streak[streakIndex]; i++) //fill in the streak
			line[i] = 'X';

		streakIndex++; //move to the next streak
		gapIndex++; //move to the next gap
	}

	return line;
}

int getWidth(vector<int> streaks)
{
	return streaks.size() + 1;
}
int getSum(vector<int> streaks, int width, int lineWidth)
{
	int streakSum = 0;
	for (int streak : streaks)
		streakSum = streakSum + streak;

	if (width > 2)
		return lineWidth - streakSum - (width - 2);
	else //width is 1
		return lineWidth - streakSum;
}

vector<set<vector<char>>> getRowOptions(const Nonogram& n)
{
	vector<set<vector<char>>> options(n.getHeight()); //size
	for (int row = 0; row < n.getHeight(); row++)
	{
		set< vector<char> > rowOptions;
		int width = getWidth(n.getRow(row));
		int sum = getSum(n.getRow(row), width, n.getWidth());
		for (vector<int> optionGaps : getLineSet(width, sum))
			rowOptions.insert( decodeLineSet(optionGaps, n.getRow(row), n.getWidth()) ); //insert the key and value (gap vector, row char vector) into the map

		options[row] = rowOptions;
	}
	return options;
}
vector<set<vector<char>>> getColumnOptions(const Nonogram& n)
{
	vector<set<vector<char>>> options(n.getWidth()); //size
	for (int column = 0; column < n.getWidth(); column++)
	{
		set< vector<char> > columnOptions;
		int width = getWidth(n.getColumn(column));
		int sum = getSum(n.getColumn(column), width, n.getHeight());
		for (vector<int> optionGaps : getLineSet(width, sum))
			columnOptions.insert( decodeLineSet(optionGaps, n.getColumn(column), n.getHeight()) ); //insert the key and value (gap vector, row char vector) into the map

		options[column] = columnOptions;
	}
	return options;
}
//end setup functions


//Constraint Satisfaction Problem functions
bool revise(int sourceIndex, int destIndex, set<vector<char>>& sourceDomain, const set<vector<char>>& destDomain)
{
	bool isRevised = false;
	for (auto iter = sourceDomain.begin(); iter != sourceDomain.end();)
	{
		vector<char> sourceOption = *iter; //the option from the iterator
		bool optionIsRevised = true; //assume revision for option until disproven
		for (vector<char> destOption : destDomain)
			if (sourceOption[destIndex] == destOption[sourceIndex]) //if match then option is possible
			{
				optionIsRevised = false;
				break;
			}
		if (optionIsRevised)
		{
			iter = sourceDomain.erase(iter); //delete this option by iterator, make sure iter value stays consistent
			isRevised = true;
		}
		else
			++iter; //if we don't remove an element we can move the iterator forward
	}
	return isRevised;
}

bool arcConsistency(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain)
{
	queue<arcType> toRevise; //list of things to revise
	for(int rowI = 0; rowI < rowDomain.size(); rowI++) //put all possible row column options to initially revise
		for (int colI = 0; colI < columnDomain.size(); colI++)
		{
			//put both types of revision on the queue
			toRevise.push( arcType{rowI, colI, true} );
			toRevise.push( arcType{colI, rowI, false} );
		}

	while (!toRevise.empty()) //while revisions are still necessary
	{
		arcType arcRevise = toRevise.front();
		toRevise.pop(); //remove elmeent

		bool revised; //if a revision happened
		if (arcRevise.sourceIsRow)
			revised = revise(arcRevise.source, arcRevise.destination, rowDomain[arcRevise.source], columnDomain[arcRevise.destination]);
		else //source is a column
			revised = revise(arcRevise.source, arcRevise.destination, columnDomain[arcRevise.source], rowDomain[arcRevise.destination]);

		if (revised)
		{
			//if the new domain of the source is null, then we have an inconsistent nonogram
			if (arcRevise.sourceIsRow && rowDomain[arcRevise.source].empty())
				return false;
			else if (!arcRevise.sourceIsRow && columnDomain[arcRevise.source].empty()) //arcRevise source is a columnIndex
				return false;

			//since the domain of source is now smaller we have to revise all the domains it affects
			if (arcRevise.sourceIsRow)
				for (int colI = 0; colI < columnDomain.size(); colI++)
					toRevise.push( arcType{ colI, arcRevise.source, false } );
			else //source is a column
				for (int rowI = 0; rowI < rowDomain.size(); rowI++)
					toRevise.push( arcType{ rowI, arcRevise.source, true } );
		}
	}
	return true; //consistent
}

void backtrack(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign)
{
	if (domainsAreSingular(rowDomain, columnDomain, rowAssign, columnAssign)) //solution found
		return;

	int smallestRowIndex = 0;
	int smallestRowSize = INT_MAX;
	for (int i = 0; i < rowDomain.size(); i++)
		if (!rowAssign[i] && rowDomain[i].size() < smallestRowSize)
		{
			smallestRowIndex = i;
			smallestRowSize = rowDomain[i].size();
		}
	
	int smallestColumnIndex = 0;
	int smallestColumnSize = INT_MAX;
	for (int i = 0; i < columnDomain.size(); i++)
		if (!columnAssign[i] && columnDomain[i].size() < smallestColumnSize)
		{
			smallestColumnIndex = i;
			smallestColumnSize = columnDomain[i].size();
		}
	
	if (smallestRowSize < smallestColumnSize)
		return assignRowBacktrack(smallestRowIndex, rowDomain, columnDomain, rowAssign, columnAssign); //row is assigned
	else //smallestColumnSize <= smallestRowSize
		return assignColumnBacktrack(smallestColumnIndex, rowDomain, columnDomain, rowAssign, columnAssign); //inverted order, so column is assigned
}

void assignRowBacktrack(int rowIndex, vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign)
{
	rowAssign[rowIndex] = true;
	for (auto iter = rowDomain[rowIndex].begin(); iter != rowDomain[rowIndex].end(); ++iter)
	{
		vector<char> assign = *iter;

		set<vector<char>> oldRowDomain = rowDomain[rowIndex];
		rowDomain[rowIndex] = { assign }; //new domain for tihs row is just the assignment
		
		vector<set<vector<char>>> newColumnsDomain = columnDomain;
		//take away newly restriced domain values
		for (int i = 0; i < newColumnsDomain.size(); i++) //go through each column set
			revise(i, rowIndex, newColumnsDomain[i], rowDomain[rowIndex]);


		backtrack(rowDomain, newColumnsDomain, rowAssign, columnAssign); //continue seraching
		
		if (domainsAreSingular(rowDomain, newColumnsDomain, rowAssign, columnAssign))
		{
			columnDomain = newColumnsDomain; //set the newColumn domain
			return; //valid solution
		}

		//revert assignment
		rowDomain[rowIndex] = oldRowDomain;
		iter = rowDomain[rowIndex].find(assign);
	}
	rowAssign[rowIndex] = false;
	return; //failure
}
void assignColumnBacktrack(int columnIndex, vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign)
{
	columnAssign[columnIndex] = true;
	for (auto iter = columnDomain[columnIndex].begin(); iter != columnDomain[columnIndex].end(); ++iter)
	{
		vector<char> assign = *iter;

		set<vector<char>> oldColumnDomain = columnDomain[columnIndex];
		columnDomain[columnIndex] = { assign }; //new domain for tihs row is just the assignment
		
		vector<set<vector<char>>> newRowsDomain = rowDomain;
		//take away newly restriced domain values
		for (int i = 0; i < newRowsDomain.size(); i++) //go through each row set
			revise(i, columnIndex, newRowsDomain[i], columnDomain[columnIndex]);

		backtrack(newRowsDomain, columnDomain, rowAssign, columnAssign); //continue seraching

		if (domainsAreSingular(newRowsDomain, columnDomain, rowAssign, columnAssign))
		{
			rowDomain = newRowsDomain; //set the newColumn domain
			return; //valid solution
		}

		//revert assignment
		columnDomain[columnIndex] = oldColumnDomain;
		iter = columnDomain[columnIndex].find(assign);
	}
	columnAssign[columnIndex] = false;
	return; //failure
}

bool domainsAreSingular(vector<set<vector<char>>>& rowDomain, vector<set<vector<char>>>& columnDomain, vector<bool>& rowAssign, vector<bool>& columnAssign)
{
	for (bool assign : rowAssign)
		if (!assign)
			return false;

	for (bool assign : columnAssign)
		if (!assign)
			return false;

	for (set<vector<char>> rowsOptions : rowDomain)
		if (rowsOptions.size() > 1 || rowsOptions.empty()) //if multiple or no options are left, then the nonogram was invalid
			return false;
	for (set<vector<char>> columnsOptions : columnDomain)
		if (columnsOptions.size() > 1 || columnsOptions.empty()) //if multiple or no options are left, then the nonogram was invalid
			return false;

	return true; //solved!
}

bool solve(Nonogram& n)
{
	cout << "creating options..." << endl;

	vector<set<vector<char>>> rowDomain = getRowOptions(n);
	vector<set<vector<char>>> columnDomain = getColumnOptions(n);
	
	vector<bool> rowAssign(n.getHeight(), false);
	vector<bool> columnAssign(n.getWidth(), false);

	cout << "options created... " << endl;
	cout << "Checking arcs..." << endl;

	if (!arcConsistency(rowDomain, columnDomain))
		return false; //arcs weren't consistent

	cout << "Arcs verified" << endl;
	cout << "Searching..." << endl;

	backtrack(rowDomain, columnDomain, rowAssign, columnAssign);

	if (!domainsAreSingular(rowDomain, columnDomain, rowAssign, columnAssign))
		return 0;

	cout << "Solution found" << endl;

	//else the nonogram must be valid
	for (int x = 0; x < n.getWidth(); x++)
	{
		vector<char> column = *columnDomain[x].begin();
		for (int y = 0; y < n.getHeight(); y++)
			n[x][y] = column[y];
	}
	return true;

}
//end Constraint Satisfaction Problem functions





int main()
{
	int width = 5;
	int height = 5;
	string input = "";
	Nonogram n(5,5);
	while (input != "quit" && input != "q")
	{
		system("cls"); //clear

		cout << "Options: " << endl;
		cout << "q, quit: quit application" << endl;
		cout << "r, rand, random: create random nonogram" << endl;
		cout << "set, create: create nonogram from row and column labels" << endl;
		cout << "file: set the nonogram from the puzzle.txt file" << endl;
		cout << "m, modify: modify the width and height of the nonogram" << endl;
		cout << "p, show, print, display: display nonogram in its current state" << endl << endl;
		cout << "s, solve: solve nonogram" << endl;
		cout << "c, clear: clear nonogram cells" << endl;
		cout << "Current nonogram setting: width: " << width << " height: " << height << endl;

		cout << ">>> ";
		getline(cin, input);

		if (input == "r" || input == "rand") //rand
		{
			n = Nonogram(width, height);
			cout << n;
			system("pause");
		}
		else if (input == "set" || input == "create")
		{
			system("cls");
			string line;
			vector<vector<int>> rows(height);
			vector<vector<int>> columns(width);

			cout << "enter row labels (i.e. like 3 2 4): " << endl;
			for (vector<int>& row : rows) //get row labels
			{
				getline(cin, line);
				string seg;
				for (char c : line)
				{
					if (isdigit(c))
						seg = seg + c;
					else //whitespace or something else
					{
						row.push_back(stoi(seg));
						seg = ""; //reset
					}
				}
				if(seg != "")
					row.push_back(stoi(seg));
			}

			system("cls");
			cout << "Enter column lables (i.e. like 4 1): " << endl;
			for (vector<int>& column : columns) //get row labels
			{
				getline(cin, line);
				string seg;
				for (char c : line)
				{
					if (isdigit(c))
						seg = seg + c;
					else //whitespace or something else
					{
						column.push_back(stoi(seg));
						seg = ""; //reset
					}
				}
				if (seg != "") //empty seg
					column.push_back(stoi(seg));
			}
			n = Nonogram(rows, columns); //create from lables
		}
		else if (input == "file")
		{
			ifstream file;
			string seg;
			file.open("puzzle.txt");
			string line;

			getline(file, line);
			height = stoi(line);
			vector<vector<int>> rows(height);
			for (int i = 0; i < height; i++) //get the rows
			{
				seg = "";
				getline(file, line);
				for (char c : line)
				{
					if (isdigit(c))
						seg = seg + c;
					else //whitespace or something else
					{
						rows[i].push_back(stoi(seg));
						seg = ""; //reset
					}
				}
				if (seg != "")
					rows[i].push_back(stoi(seg));
			}

			getline(file, line); //empty line

			getline(file, line);
			width = stoi(line);
			vector<vector<int>> columns(width);
			for (int i = 0; i < width; i++) //get the columns
			{
				seg = "";
				getline(file, line);
				for (char c : line)
				{
					if (isdigit(c))
						seg = seg + c;
					else //whitespace or something else
					{
						columns[i].push_back(stoi(seg));
						seg = ""; //reset
					}
				}
				if (seg != "")
					columns[i].push_back(stoi(seg));
			}
			n = Nonogram(rows, columns); //create the nonogram from labels
		}
		else if (input == "m" || input == "modify")
		{
			cout << "Enter new width and height: (i.e. like 8 10): ";
			cin >> width;
			cin >> height;
		}
		else if (input == "show" || input == "d" || input == "display" || input == "print")
		{
			cout << n << endl;
			system("pause");
		}
		else if (input == "solve" || input == "s")
		{
			if (solve(n))
			{
				system("cls");
				cout << "Solved: " << endl;
				cout << n;
			}
			else
				cout << "could not solve!" << endl;
			system("pause");
		}
		else if (input == "clear" || input == "c")
			n.clearGrid();
	}
	return 0;
}



