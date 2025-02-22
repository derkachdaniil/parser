#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include "parser_shared_classes.h"
#include <Windows.h>
using namespace std;

struct parser_state_generate
{
	map<wstring, parser_node*>names;
	set<wstring>can_be_cut;
};
auto generate_parser(vector<lexer_rule>basic_rules, vector<lexer_rule>lex_condition_rules,string filename)
{

	lexer l1(basic_rules);
	wstring s;
	wstring s2;

	wifstream f(filename, ios::in|ios::binary);
	//setlocale(LC_ALL, "UTF-8");
	std::locale utf8_to_utf16(std::locale(), new std::codecvt_utf8<wchar_t>);
	f.imbue(utf8_to_utf16);
	while (!f.eof())
	{
		getline(f, s2);
		s += s2 + '\n';
	}
	f.close();
	auto lexemes = l1.lex(s);

	parser_node* line_right = new parser_node_select(L"line_right", true, {});

	parser_node* bracket = new parser_node_check_all(L"bracket", true, { new string_condition(L"(",true,false),line_right,new string_condition(L")",true,false) });
	parser_node* repeat_bracket = new parser_node_check_all(L"repeat_bracket", true, { new string_condition(L"(",true,false),line_right,new string_condition(L")",true,false),new string_condition(L"*",true,false) });
	parser_node* repeat_once_or_zero_bracket = new parser_node_check_all(L"repeat_once_or_zero_bracket", true, { new string_condition(L"[",true,false),line_right,new string_condition(L"]",true,false) });
	parser_node* name_element = new parser_node_check_all(L"name_element", true, { new id_condition(name,false,false) });
	parser_node* element = new parser_node_select(L"element", true, { repeat_once_or_zero_bracket,repeat_bracket,bracket,name_element,new lex_condition(lex_condition_rules) });

	parser_node* direction_repeat = new parser_node_repeater(L"direction_repeat", true, { new id_condition(integer_,false,false) });
	parser_node* direction_brackets = new parser_node_check_all(L"direction_brackets", true, { new string_condition(L"{",true,false),direction_repeat,new string_condition(L"}",true,false) });
	parser_node* direction = new parser_node_maybe(L"direction", true, { direction_brackets });

	parser_node* element_with_option = new parser_node_check_all(L"element_with_option", true, { direction,element});

	parser_node* elements = new parser_node_repeater(L"elements", true, { element_with_option });

	parser_node* or_element = new parser_node_check_all(L"or_element", true, { elements,new string_condition(L"|",true,false) });

	parser_node* or_repeat = new parser_node_repeater(L"or_repeat", true, { or_element });

	parser_node* or_chain = new parser_node_check_all(L"or_chain", true, { elements,new string_condition(L"|",true,false),or_repeat,elements });

	line_right->conditions = { or_chain,elements };


	parser_node* line = new parser_node_check_all(L"line", false, { new id_condition(name,false,false),new string_condition(L":",true,false),line_right });


	//	parser_node* line_optional = new parser_node_select("line_optional", true, { new node_condition(line),new string_condition("\n",true,false) });

		//	parser_node* last_line = new parser_node_check_all("last_line", { new id_condition(name),new string_condition(":"),new node_condition(line_right) });

	parser_node* program = new parser_node_repeater(L"program", true, { line/*_optional*/ });


	map<wstring, macros<parser_state_generate>>macroses;
	macroses.insert(make_pair(L"line", macros<parser_state_generate>()));
	macroses[L"line"].add_function(
		[](map<wstring, macros<parser_state_generate>>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state_generate& st)
		{
			cur->name = ((id_condition*)cur->conditions[0])->data;
			cur->conditions.erase(cur->conditions.begin());
			st.names.insert(make_pair(cur->name, cur));
			for (int i = 0; i < cur->conditions.size(); i++)
				cur->conditions[i]->apply_macroses<parser_state_generate>(macroses, st, visited);
			return wstring(L"");
		});
	map<wstring, macros<parser_state_generate>>macroses2;
	macroses2.insert(make_pair(L"name_element", macros<parser_state_generate>()));
	macroses2[L"name_element"].add_function(
		[](map<wstring, macros<parser_state_generate>>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state_generate& st)
		{
			wstring new_name = ((id_condition*)cur->conditions[0])->data;
			if (st.names.find(new_name) != st.names.end() ||
				(new_name[0] == '@' && st.names.find(new_name.substr(1)) != st.names.end()))
			{
				cur->name = new_name;
				if (cur->name[0] == '@')
				{
					cur->can_be_optimized = true;
					cur->name = cur->name.substr(1);
				}
				else
					cur->can_be_optimized = false;

				st.names[cur->name]->can_be_optimized = true;
				cur->conditions.erase(cur->conditions.begin());
				cur->conditions.push_back(st.names[cur->name]);
				st.can_be_cut.insert(cur->name);
			}
			for (int i = 0; i < cur->conditions.size(); i++)
				cur->conditions[i]->apply_macroses<parser_state_generate>(macroses, st, visited);
			return wstring(L"");
		});
	map<wstring, macros<parser_state_generate>>macroses3;
	macroses3.insert(make_pair(L"program", macros<parser_state_generate>()));
	macroses3[L"program"].add_function(
		[](map<wstring, macros<parser_state_generate>>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state_generate& st)
		{
			for (int i = 0; i < cur->conditions.size(); i++)
				if (st.can_be_cut.find(cur->conditions[i]->name) != st.can_be_cut.end())
				{
					cur->conditions.erase(cur->conditions.begin() + i);
					i--;
				}
			for (int i = 0; i < cur->conditions.size(); i++)
				cur->conditions[i]->apply_macroses<parser_state_generate>(macroses, st, visited);
			return wstring(L"");
		});

	map<wstring, macros<parser_state_generate>>macroses4;
	macroses4.insert(make_pair(L"element_with_option", macros<parser_state_generate>()));
	macroses4[L"element_with_option"].add_function(
		[](map<wstring, macros<parser_state_generate>>& macroses, set< parser_node*>& visited, parser_node* cur, parser_state_generate& st)
		{
			auto direction = cur->conditions[0];
			if (direction->conditions.size() > 0)
			{
				vector<int>result_direction;
				auto direction_brackets = direction->conditions[0];
				auto direction_repeat = direction_brackets->conditions[0];
				for (int i = 0; i < direction_repeat->conditions.size(); i++)
					result_direction.push_back(stoi(direction_repeat->conditions[i]->data));
				cur->conditions[1]->deep_move_direction_change(result_direction);
			}
			cur->conditions.erase(cur->conditions.begin());
			for (int i = 0; i < cur->conditions.size(); i++)
				cur->conditions[i]->apply_macroses<parser_state_generate>(macroses, st, visited);

			return wstring(L"");
		});


	int n = 0;
	omni_storage* omni = new oneD_omni_storage(lexemes);
	parser_node* parser = pick_best(run(program, omni, { {L"program",parser_node_check_all_} ,{L"line",parser_node_check_all_} ,{L"bracket",parser_node_check_all_},{L"line_right",parser_node_check_all_},{L"direction",parser_node_check_all_},{L"direction_brackets",parser_node_check_all_},{L"direction_repeat",parser_node_check_all_},{L"element_with_option",parser_node_check_all_} ,{L"element",parser_node_check_all_},{L"or_element",parser_node_check_all_},{L"elements",parser_node_check_all_},{L"name_element",parser_node_check_all_} ,
		{L"repeat_once_or_zero_bracket",parser_node_maybe_},{L"or_chain",parser_node_select_},{L"or_repeat",parser_node_select_},{L"repeat_bracket",parser_node_repeater_} }));


	//print(program);

	parser_state_generate st;

	//print(parser);
	apply_macroses(parser, macroses, st);
	apply_macroses(parser, macroses2, st);
	apply_macroses(parser, macroses3, st);

	skip_skippable(parser, false);

	//wcout << endl << endl;
	//wcout << endl << endl;
	//wcout << endl << endl;
	//
	//print(parser);

	apply_macroses(parser, macroses4, st);
	//print(parser);
	simplify(parser);

	//wcout << endl << endl;
	//wcout << endl << endl;
	//wcout << endl << endl;
	//
	//print(parser);
	return parser;
}