#pragma once
#include <vector>
#include <string>
#include <iostream>
using namespace std;

enum lexemes {lexeme_name,comment,string_,keyword,integer_,name,skippable};
map<wstring, lexemes>lexemes_names = { {L"lexeme_name",lexeme_name}, {L"comment",comment}, {L"STRING",string_}, {L"keyword",keyword},{L"INTEGER",integer_}, {L"NAME",name}};
class regex
{
public:
	virtual regex* get_derivative(wchar_t c) { return 0; }
	virtual bool regex_completed() { return false; }

};
class simple_repetitive_regex:public regex
{
public:
	vector<wchar_t>chars_to_repeat;
	simple_repetitive_regex(vector<wchar_t>chars_to_repeat_)
	{
		chars_to_repeat = chars_to_repeat_;
	}
	regex* get_derivative(wchar_t c)
	{
		for (int i = 0; i < chars_to_repeat.size(); i++)
			if (chars_to_repeat[i] == c)
				return this;
		return 0;
	}
	virtual bool regex_completed() { return true; }
};
class words_regex :public regex
{
public:
	vector<wstring>words_to_pick_from;
	words_regex(vector<wstring>words_to_pick_from_)
	{
		words_to_pick_from = words_to_pick_from_;
	}

	regex* get_derivative(wchar_t c)
	{
		words_regex* n = new words_regex(vector<wstring>());
		for (int i = 0; i < words_to_pick_from.size(); i++)
			for (int j = 0; j < words_to_pick_from[i].size(); j++)
				if (words_to_pick_from[i][j] == c)
				{
					n->words_to_pick_from.push_back(words_to_pick_from[i].substr(j + 1));
					break;
				}
		if (n->words_to_pick_from.size() > 0)
			return n;
		return 0;
	}
	virtual bool regex_completed() 
	{
		for (int i = 0; i < words_to_pick_from.size(); i++)
			if (words_to_pick_from[i] == "")
				return true;
		return false;
	}

};
class lexer_rule
{
public:
	regex* from;
	wstring to;
	int number_associated_with_success;
	bool lexeme_is_number;
	lexer_rule()
	{
		from = 0;
		number_associated_with_success = -1;
		lexeme_is_number = false;
	}
};
class lexeme
{
public:
	int id;
	wstring data;
	lexeme(int id_,wstring data_)
	{
		id = id_;
		data = data_;
	}
};
class lexer
{
	
public:
	
	vector<lexer_rule>rules;
	lexer(vector<lexer_rule>rules_)
	{
		rules = rules_;
	}
	

	vector<lexeme> lex(wstring s)
	{
		s += L" ";
		vector<lexeme>lexemes;
		vector<lexer_rule>previous_rules;
		vector<lexer_rule>current_rules;
		current_rules = rules;
		int lexeme_start = 0;
		for (int i = 0; i < s.size(); i++)
		{
			bool regex_still_fits = false;
			for (int j = 0; j < current_rules.size(); j++)
				if (current_rules[j].from != 0)
				{
					current_rules[j].from = current_rules[j].from->get_derivative(s[i]);
					if (current_rules[j].from != 0)
						regex_still_fits = true;
				}
			if (regex_still_fits)
			{
				previous_rules = current_rules;
			}
			else
			{
				if (previous_rules.size() == 0)
				{
					throw exception("unrecognized symbol " + s[i]);
				}
				for (int j = 0; j < previous_rules.size(); j++)
					if (previous_rules[j].from != 0 && previous_rules[j].from->regex_completed())
					{
						if (previous_rules[j].to != "")
						{
							i = s.find(previous_rules[j].to, i + 1) + previous_rules[j].to.size();
						}
						i--;
						if(previous_rules[j].number_associated_with_success!=comment && previous_rules[j].number_associated_with_success != skippable)
							lexemes.push_back(lexeme(previous_rules[j].number_associated_with_success, s.substr(lexeme_start, i - lexeme_start + 1)));
						break;
					}
				lexeme_start = i + 1;
				current_rules = rules;
				previous_rules.clear();
			}
		}
		return lexemes;
	}
};

vector<lexer_rule> basic_lexer_rules()
{
	vector<lexer_rule> rules;


	rules.push_back(lexer_rule());
	rules.back().from = new words_regex({ L"//" });
	rules.back().to = L"\n";
	rules.back().number_associated_with_success = comment;

	rules.push_back(lexer_rule());
	rules.back().from = new words_regex({ L"/\"" });
	rules.back().to = L"\"";
	rules.back().number_associated_with_success = string_;


	rules.push_back(lexer_rule());
	rules.back().from = new words_regex({ L"\"" });
	rules.back().to = L"\"";
	rules.back().number_associated_with_success = string_;

	rules.push_back(lexer_rule());
	rules.back().from = new words_regex({ L"!",L"<",L">",L"+",L"-",L"%",L"*",L"/",L"<=",L">=",L"==",L"!=",L"||",L"&&",L";",L":",L",",L"|",L"(",L"*",L")",L"[",L"]",L"{",L"}",

	L"│", L"┤", L"┐", L"└", L"┴", L"┬", L"├", L"─", L"┼",L"┘", L"┌", L"╬",
	L"╟", L"╢", L"╧", L"╤",
	L"╝",L"║",L"╔", L"═", L"╗", L"╚",
	L"°",L"►",L"◄",L"▼",L"▲",L"●",L"◆",L"■",L"○",L"▧",L"∪",L"⨁",L"⁌",L"⁍",L"`"
		});
	rules.back().number_associated_with_success = keyword;

	rules.push_back(lexer_rule());
	vector<wchar_t>name_parts;
	for (int i = '0'; i <= '9'; i++)
		name_parts.push_back(i);
	name_parts.push_back('-');
	rules.back().from = new simple_repetitive_regex(name_parts);
	rules.back().number_associated_with_success = integer_;


	rules.push_back(lexer_rule());
	name_parts.clear();
	for (int i = 'a'; i <= 'z'; i++)
		name_parts.push_back(i);
	for (int i = 'A'; i <= 'Z'; i++)
		name_parts.push_back(i);
	for (int i = '0'; i <= '9'; i++)
		name_parts.push_back(i);
	name_parts.push_back('_');
	name_parts.push_back('@');
	name_parts.push_back('-');
	rules.back().from = new simple_repetitive_regex(name_parts);
	rules.back().number_associated_with_success = name;

	rules.push_back(lexer_rule());
	name_parts.clear();
	name_parts.push_back(' ');
	name_parts.push_back('\n');
	name_parts.push_back('\r');
	name_parts.push_back('\t');
	name_parts.push_back(wchar_t(65279));
	rules.back().from = new simple_repetitive_regex(name_parts);
	rules.back().number_associated_with_success = skippable;



	return rules;

}
vector<lexer_rule> brag_lexer_rules()
{
	vector<lexer_rule> rules = basic_lexer_rules();
	rules.insert(rules.begin(), lexer_rule());
	vector<wstring> key;
	for (auto it = lexemes_names.begin(); it != lexemes_names.end(); ++it)
		key.push_back(it->first);
	rules.front().from = new words_regex(key);
	rules.front().number_associated_with_success = lexeme_name;
	return rules;
}