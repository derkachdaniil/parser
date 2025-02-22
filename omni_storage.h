#pragma once
#include <vector>
#include "lexer.h"
using namespace std;

class omni_storage
{
public:
	virtual lexeme operator[](vector<int> address) = 0;
	virtual bool inside(vector<int> address) = 0;

	virtual vector<int> get_start_position() = 0;
};
class oneD_omni_storage:public omni_storage
{
public:
	vector<lexeme> storage;
	oneD_omni_storage(vector<lexeme> storage_)
	{
		storage = storage_;
	}
	lexeme operator[](vector<int> address)
	{
		return storage[address[0]];
	}

	bool inside(vector<int> address)
	{
		return address[0] < storage.size() && address[0]>=0;
	}

	vector<int> get_start_position()
	{
		return { 0 };
	}
};
class twoD_omni_storage :public omni_storage
{
public:
	vector<vector<lexeme>> storage;
	twoD_omni_storage(vector<vector<lexeme>> storage_)
	{
		storage = storage_;
	}
	lexeme operator[](vector<int> address)
	{
		return storage[address[1]][address[0]];
	}

	bool inside(vector<int> address)
	{
		return address[1] < storage.size() && address[1] >= 0
			&& address[0] < storage[1].size() && address[0] >= 0;
	}

	vector<int> get_start_position()
	{
		return { 0,0 };
	}
};
