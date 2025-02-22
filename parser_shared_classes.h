#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "lexer.h"
#include <Windows.h>
#include <map>
#include <set>
#include "omni_storage.h"

using namespace std;

class parser_node;



enum type_name { parser_node_, parser_node_select_, parser_node_check_all_, parser_node_repeater_, parser_node_maybe_ };






template<typename parser_state>
class macros
{
public:
	vector<wstring(*)(map<wstring, macros>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state& st)>functions;
	macros(wstring(*func)(map<wstring, macros>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state& st))
	{
		functions.push_back(func);
	}
	macros()
	{

	}
	//	map<int, int>functions_by_timing;
	void add_function(wstring(*func)(map<wstring, macros>& macroses, set< parser_node *>& visited, parser_node * cur, parser_state& st))
	{
		functions.push_back(func);
		//		functions_by_timing.insert(pair<int, int>(timing, functions.size() - 1));
	}

};



class parser_node
{
public:
	wstring name;
	wstring data;
	vector<parser_node *>conditions;
	bool can_be_optimized;
	bool skippable;
	bool skippable_after_brag;
	vector<int>move_direction;
	vector<int>position_in_lexemes;
	parser_node()
	{
		skippable = false;
		skippable_after_brag = false;
		move_direction = {1};
	}
	parser_node(wstring name_, bool can_be_optimized_,vector<parser_node *>conditions_ = {})
	{
		can_be_optimized = can_be_optimized_;
		name = name_;
		conditions = conditions_;
		skippable = false;
		skippable_after_brag = false;
	}
	void deep_move_direction_change(vector<int>move_direction_)
	{
		move_direction = move_direction_;
		for (int i = 0; i < conditions.size(); i++)
		{
			conditions[i]->deep_move_direction_change(move_direction_);
		}
	}
	virtual vector<pair<vector<int>, parser_node*>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		return {};
	}
	template<typename parser_state>
	wstring apply_macroses(map<wstring, macros<parser_state>>& macroses, parser_state& st, set< parser_node *>& visited)
	{
		if (visited.find(this) == visited.end())
		{
			visited.insert(this);
			if (macroses.find(name) != macroses.end())
				return macroses[name].functions[0](macroses, visited, this, st);
			else
			{
				wstring s = data;
				for (int i = 0; i < conditions.size(); i++)
					s += conditions[i]->apply_macroses(macroses, st, visited);
				return s;
			}
		}
		return L"";

	}
	virtual void print(int depth, int color, set< parser_node*>& visited)
	{
		visited.insert(this);

		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		wcout << L"\n";
		for (int i = 0; i < depth; i++)
			wcout << L' ';
		wcout << L"( ";
		SetConsoleTextAttribute(hConsole, color);
		wcout << name << " ";
		SetConsoleTextAttribute(hConsole, 7);
		for (int i = 0; i < conditions.size(); i++)
			if (visited.find(conditions[i]) == visited.end())
				conditions[i]->print(depth + 2, 7, visited);
			else
				wcout <<" <- " << conditions[i]->name << " <- " << endl;
		wcout << L"\n";
		for (int i = 0; i < depth; i++)
			wcout << L' ';
		wcout << L")";

	}
	void skip_skippable(bool after_brag_run, set< parser_node*>& visited)
	{
		if (visited.find(this) == visited.end())
		{
			visited.insert(this);
			for (int i = 0; i < conditions.size(); i++)
				if (conditions[i]->skippable || (after_brag_run && conditions[i]->skippable_after_brag))
				{
					conditions.erase(conditions.begin() + i);
					i--;
				}
			for (int i = 0; i < conditions.size(); i++)
				conditions[i]->skip_skippable(after_brag_run, visited);
		}
	}
	virtual void simplify(set< parser_node*>& visited);


};

parser_node * correct_node_type_creator(wstring node_name, bool can_be_optimized_, map<wstring, type_name>name_to_type_matcher);

class string_condition :public parser_node 
{
public:

	string_condition(wstring data_, bool skippable_, bool skippable_after_brag_, vector<int>position_in_lexemes_ = {})
	{
		 data = data_;
		 skippable = skippable_;
		 skippable_after_brag = skippable_after_brag_;
		 position_in_lexemes = position_in_lexemes_;
	}
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		if (!(*lexemes).inside(n))
			return {};
		if ((*lexemes)[n].data ==  data)
		{
			auto old_n = n;
			for(int i=0;i<n.size();i++)
				n[i] +=  move_direction[i];
			return { {n,new string_condition ( data, skippable, skippable_after_brag,old_n)} };
		}
		return {};
	}
	virtual void print(int depth, int color, set< parser_node *>& visited)
	{
		wcout <<  data << L'{';
		for (int i = 0; i < position_in_lexemes.size(); i++)
		{
			wcout << position_in_lexemes[i] << L' ';

		}
		wcout << L"} ";

	}


};

class id_condition :public parser_node 
{
public:
	int id;

	id_condition(int id_, bool skippable_, bool skippable_after_brag_, vector<int>position_in_lexemes_ = {}, wstring data_ = L"")
	{
		id = id_;
		 data = data_;
		 skippable = skippable_;
		 skippable_after_brag = skippable_after_brag_;
		 position_in_lexemes = position_in_lexemes_;
	}

	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		if (!(*lexemes).inside(n))
			return {};
		if ((*lexemes)[n].id == id)
		{
			auto temp = (*lexemes)[n].data;
			auto old_n = n;
			for (int i = 0; i < n.size(); i++)
				n[i] +=  move_direction[i];
			return { {n, new id_condition(id, skippable,  skippable_after_brag,old_n, temp)} };
		}
		return {};

	}
	virtual void print(int depth, int color, set< parser_node *>& visited)
	{
		wcout << L"[ " << id << L' ' <<  data << L" ] ";
	}


};

class lex_condition :public parser_node 
{
public:
	int id;
	vector<lexer_rule>rules;
	lex_condition(vector<lexer_rule>rules_, vector<int>position_in_lexemes_ = {}, wstring data_ = L"")
	{
		rules = rules_;
		data = data_;
		position_in_lexemes = position_in_lexemes_;

	}
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		if ( !(*lexemes).inside(n))
			return {};
		if ((*lexemes)[n].id == string_)
		{
			bool skippable_after_brag = false;
			lexer l(rules);
			wstring s;

			if ((*lexemes)[n].data[0] == '/')
			{
				s = (*lexemes)[n].data.substr(2, (*lexemes)[n].data.size() - 3);
				skippable_after_brag = true;
			}
			else
				s = (*lexemes)[n].data.substr(1, (*lexemes)[n].data.size() - 2);

			auto temp = l.lex(s);
			auto old_n = n;
			for (int i = 0; i < n.size(); i++)
				n[i] +=  move_direction[i];
			return  { {n,new string_condition (temp[0].data, skippable, skippable_after_brag,old_n)} };
		}
		if ((*lexemes)[n].id == lexeme_name)
		{
			auto temp = (*lexemes)[n].data;
			auto old_n = n;
			for (int i = 0; i < n.size(); i++)
				n[i] +=  move_direction[i];
			return { {n,new id_condition (lexemes_names[temp], skippable,false,old_n, temp)} };
		}
		return {};

	}
	virtual void print(int depth, int color, set< parser_node *>& visited)
	{
		wcout << L"[ lexed " << id << ' ' << parser_node ::data << L" ] ";
	}


};




class parser_node_select :public parser_node 
{
public:
	parser_node_select(wstring name_, bool can_be_optimized_, vector<parser_node *>conditions_) :parser_node (name_, can_be_optimized_, conditions_) {};
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		vector<pair<vector<int>, parser_node *>> result;

		for (int i = 0; i <  conditions.size(); i++)
		{
			vector<int> backup_n = n;
			auto temp =  conditions[i]->run(lexemes, n, name_to_type_matcher);
			if (temp.size() > 0)
			{
				for (int k = 0; k < temp.size(); k++)
				{
					result.push_back({ temp[k].first,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher) });
					result.back().second->conditions.push_back(temp[k].second);
				}
			}
			n = backup_n;
		}
		return result;
	}
	void print(int depth, int color, set< parser_node *>& visited)
	{
		parser_node ::print(depth, BACKGROUND_BLUE, visited);
	}

};

class parser_node_check_all :public parser_node 
{
public:
	parser_node_check_all(wstring name_, bool can_be_optimized_, vector<parser_node *>conditions_) :parser_node (name_, can_be_optimized_, conditions_) {};
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		vector<pair<vector<int>, parser_node *>> result = { {n,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher)} };
		bool good = true;
		for (int i = 0; i <  conditions.size(); i++)
		{
			int cur = result.size();
			for (int j = 0; j < cur; j++)
			{
				auto temp =  conditions[i]->run(lexemes, result[j].first, name_to_type_matcher);

				for (int k = 0; k < temp.size(); k++)
				{
					result.push_back({ temp[k].first,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher) });
					(*result.back().second) = *result[j].second;
					result.back().second->conditions.push_back(temp[k].second);
				}
				result.erase(result.begin() + j);
				j--;
				cur--;

			}
		}
		return result;
	}
	void print(int depth, int color, set< parser_node *>& visited)
	{
		parser_node::print(depth, BACKGROUND_GREEN, visited);
	}



};

class parser_node_repeater :public parser_node 
{
public:
	parser_node_repeater(wstring name_, bool can_be_optimized_, vector<parser_node *>conditions_) :parser_node (name_, can_be_optimized_, conditions_) {};
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		vector<pair<vector<int>, parser_node *>> result;
		vector<pair<vector<int>, parser_node *>> result_latest = { {n,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher)} };
		while ((*lexemes).inside(n))
		{
			int latest_size = result_latest.size();
			for (int i = 0; i < latest_size; i++)
			{
				vector<int> backup_n = result_latest[i].first;
				auto temp =  conditions[0]->run(lexemes, result_latest[i].first, name_to_type_matcher);
//				if (temp.size() > 1)
//					cout << 1 << endl;
				for (int k = 0; k < temp.size(); k++)
					if (temp[k].first != backup_n)
					{
						result_latest.push_back({ temp[k].first,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher) });
						(*result_latest.back().second) = *result_latest[i].second;
						result_latest.back().second->conditions.push_back(temp[k].second);
					}
			}
			if (latest_size == 0)
				return result;
			for (int i = 0; i < latest_size; i++)
			{
				result.push_back(result_latest.front());
				result_latest.erase(result_latest.begin());
			}
		}
		return result;
	}
	void print(int depth, int color, set< parser_node *>& visited)
	{
		parser_node::print(depth, BACKGROUND_RED, visited);
	}


};

class parser_node_maybe :public parser_node 
{
public:
	parser_node_maybe(wstring name_, bool can_be_optimized_, vector<parser_node *>conditions_) :parser_node (name_, can_be_optimized_, conditions_) {};
	virtual vector<pair<vector<int>, parser_node *>> run(omni_storage* lexemes, vector<int> n, map<wstring, type_name>name_to_type_matcher)
	{
		vector<pair<vector<int>, parser_node *>> result = { {n,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher)} };

		vector<int> backup_n = n;
		auto temp =  conditions[0]->run(lexemes, n, name_to_type_matcher);
		if (temp.size() > 0 && temp.back().first != backup_n)
		{
			int last_before = result.size() - 1;
			for (int k = 0; k < temp.size(); k++)
			{
				result.push_back({ temp[k].first,correct_node_type_creator ( name,  can_be_optimized, name_to_type_matcher) });
				(*result.back().second) = *result[last_before].second;
				result.back().second->conditions.push_back(temp[k].second);
			}
		}
		else
		{
			n = backup_n;
			return result;
		}
		return result;
	}
	void print(int depth, int color, set< parser_node *>& visited)
	{
		parser_node ::print(depth, BACKGROUND_BLUE | BACKGROUND_INTENSITY, visited);
	}
};

void  parser_node::simplify(set< parser_node *>& visited)
{
	if (visited.find(this) == visited.end())
	{
		visited.insert(this);
		for (int i = 0; i < conditions.size(); i++)
		{
				auto temp = conditions[i];
				if ((typeid(*this) == typeid(*temp)) && temp->can_be_optimized)
				{
					for (int j = 0; j < temp->conditions.size(); j++)
						conditions.insert(conditions.begin() + i + 1 + j, temp->conditions[j]);
					conditions.erase(conditions.begin() + i);
					i--;
				}
			}
		for (int i = 0; i < conditions.size(); i++)
				conditions[i]->simplify(visited);
	}
}

parser_node * correct_node_type_creator(wstring node_name, bool can_be_optimized_, map<wstring, type_name>name_to_type_matcher)
{
	parser_node * res = 0;
	if (name_to_type_matcher.size() == 0)
		res = new parser_node (node_name, can_be_optimized_);
	else
		if (name_to_type_matcher.find(node_name) != name_to_type_matcher.end())
		{
			if (name_to_type_matcher[node_name] == parser_node_)
				res = new parser_node (node_name, can_be_optimized_);
			if (name_to_type_matcher[node_name] == parser_node_select_)
				res = new parser_node_select (node_name, can_be_optimized_, vector<parser_node *>());
			if (name_to_type_matcher[node_name] == parser_node_check_all_)
				res = new parser_node_check_all (node_name, can_be_optimized_, vector<parser_node *>());
			if (name_to_type_matcher[node_name] == parser_node_repeater_)
				res = new parser_node_repeater (node_name, can_be_optimized_, vector<parser_node *>());
			if (name_to_type_matcher[node_name] == parser_node_maybe_)
				res = new parser_node_maybe (node_name, can_be_optimized_, vector<parser_node *>());

		}
		else
			res = new parser_node (node_name, can_be_optimized_);

	return res;
}

void print(parser_node * root)
{
	set< parser_node *>visited;
	root->print(0, 7, visited);
}
template<typename parser_state>
void apply_macroses(parser_node *& root, map<wstring, macros<parser_state>>& macroses, parser_state& st)
{
	set< parser_node *>visited;
	root->apply_macroses(macroses, st, visited);
}

void skip_skippable(parser_node *& root, bool after_brag_run)
{
	set< parser_node *>visited;
	root->skip_skippable(after_brag_run, visited);
}

void simplify(parser_node *& root)
{
	set< parser_node *>visited;
	root->simplify(visited);
}

vector<pair<vector<int>, parser_node*>> run(parser_node*& root, omni_storage* lexemes, map<wstring, type_name>name_to_type_matcher, vector<int> start)
{
	return root->run(lexemes, start, name_to_type_matcher);

}
vector<pair<vector<int>, parser_node*>> run(parser_node *& root, omni_storage* lexemes, map<wstring, type_name>name_to_type_matcher)
{
	vector<int> n = lexemes->get_start_position();
	return root->run(lexemes, n, name_to_type_matcher);
	
}
parser_node* pick_best(vector<pair<vector<int>, parser_node*>> temp)
{
	int max_i = 0;
	for (int i = 1; i < temp.size(); i++)
	{
		if (temp[i].first > temp[max_i].first)
			max_i = i;
	}
	return temp[max_i].second;
}