#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <set>
#include <map>
#include <ctype.h>
#include <vector>
using namespace std;
typedef const string& crstring;

ofstream logFile("DebugFile.txt");

map<char, char> translit =
{
	{'А' , 'A'},
	{'Б' , 'B'},
	{'В' , 'V'},
	{'Г' , 'G'},
	{'Д' , 'D'},
	{'Е' , 'E'},
	{'Ж' , 'J'},
	{'И' , 'I'},
	{'К' , 'K'},
	{'Л' , 'L'},
	{'М' , 'M'},
	{'Н' , 'N'},
	{'О' , 'O'},
	{'П' , 'P'},
	{'Р' , 'R'},
	{'С' , 'S'},
	{'Т' , 'T'},
	{'У' , 'U'},
	{'Ф' , 'F'},
	{'Х' , 'H'},
	{'Ю' , 'U'}
};

/// <summary>
/// Структура, которая реализует обход по дисциплинам, 
/// с вложением по модулям (дисциплина по выбору, факультатив, т.д.)
/// </summary>
struct tree
{
	int zi;
	string name;
	string subject_name;
	vector<tree*> tr;
	tree* prt;

	~tree()
	{
		for (auto et : tr)
			delete et;
	}
};

//Дерево до (root) и после (th) усечения
tree* root;
tree* th;


/// <summary>
/// Структуры, связанные с определением кол-во зачетных единиц и компетенций
/// </summary>
namespace Category
{
	//ofstream outAllStatistic;
	string PlanName = "";

	/// <summary>
	///Структура, хранящая в себе данные о дисциплине(какие входят компетенции)
	/// </summary>
	struct sData
	{
		string name;
		map<string, double> percent; // [ компетенция, процент]
		int VU;
		int employ;
		int employ_whitout_repeat;
	};

	vector<sData> AllStatistic;
	set<string> CompetentionVarible; //Какие в принципе есть компетенции

	//int min_sum_VU = 1e9;
	//int max_sum_VU = 0;
	//int min_employ = 1e9;
	//int max_employ = 0;

	map<string, double> min_procent;
	map<string, double> max_procent;

	/*void outHeader(string out)
	{
		outAllStatistic << "------- ----- ------\n";
		outAllStatistic <<out<<"\n";
	}*/

}

//Вспомогательные функции
namespace RED
{
	bool isFind(crstring line, crstring who)
	{
		return (line.find(who) != string::npos);
	}

	bool isPrefixEqual(crstring line, crstring who)
	{
		for (int i = 0; i < min(line.size(), who.size()); ++i)
			if (line[i] != who[i]) return false;
		return true;
	}

	int CountQuote(crstring line)
	{
		int ret = 0, pos = 0;
		while (pos = line.find("\"", pos) + 1)
			++ret;
		return ret;
	}

}
using namespace RED;

/// <summary>
/// Общие константы и псевдонимы дисциплин
/// </summary>
namespace Statist
{
	const double С_MIL = 10;
	double mid;
	int MAX_Course = 14;

	map<string, string> alise;
}

/// <summary>
/// Структура, хранящая в себе все сведения о дисциплинах (компетенции, кол-во ЗЕ,
/// в каких семестрах проводится)
/// </summary>
struct sGlobal
{
	//{компетенции, значимость (кол-во зачёт единиц)}
	struct Subject
	{
		set<int> Session;

		set<string> Comp;
		double score; // Вещественное среднее
		int VU; // зачётные единицы

		int id;
	};

	// Предмет и его {компетенции, значимость (кол-во зачёт единиц)}
	map<string, Subject> dictory;

	//Вывод всех данных о дисциплинах
	void OutDictory()
	{
		int i = 0;
		for (auto it : dictory)
		{
			string ch_name = it.first;
			if (Statist::alise.count(ch_name)) ch_name = Statist::alise[ch_name];
			cout << i++ << ". " << ch_name <<" | " << it.second.VU << " ЗЕ | " << it.second.score << " вес дисциплины\n";
			for (auto et : it.second.Session) cout << et; cout << "\n";
			
			for (auto et : it.second.Comp)
			{
				cout << "\t" << et << "\n";
			}
		}
	}
};

//Синглетон структуры
unique_ptr<sGlobal> Global;


/// <summary>
/// Методы, связанные с определением принадлежности компетенций дисциплинам
/// </summary>
class UCompetency
{
public:

	UCompetency()
	{
		ifstream open("bad parser prefix.txt");
		string s;

		while (getline(open, s))
			if (isFind(s, "Shift prefix")) break;

		while (getline(open, s))
		{
			if (isFind(s, "End")) break;
			shiftPrefixString.push_back(s);
		}

		while (getline(open, s))
			if (isFind(s, "Competency_anomaly")) break;
		

		while (getline(open, s))
		{
			if (isFind(s, "End")) break;
			anomaly.insert(s);
		}

		positionParserComp = 0; //Где запятая при считывании основной компетенции
		shiftСolumn = ",,";
	}

	//Функция, с которой начинается определение компетенций для конкретного плана
	void WhatComp(crstring line)
	{
		try
		{
			for (const auto& it : this->shiftPrefixString)
				if (isFind(line, it))
				{
					++positionParserComp; shiftСolumn += ",";
					return;
				}
			//for (const auto& it : this->badPrefixString)
			//	if (isPrefixEqual(line, it))
			//		return;

			if (line.at(positionParserComp) != ',')
			{
				this->compName = line.substr(positionParserComp, 
					line.find(",", positionParserComp) - positionParserComp);
				//cout << "Компетенция " << compName << "\n";
				//logFile << this->compName << ":\n";
				return;
			}

			if (RED::isPrefixEqual(line, shiftСolumn))
			{
				string subjectName = line;
				subjectName = subjectName.substr(shiftСolumn.size());
				subjectName = subjectName.substr(subjectName.find(","));
				while (subjectName.at(0) == ',')
					subjectName = subjectName.substr(1);

				if (RED::isFind(subjectName, "\""))
					subjectName = subjectName.substr(0, subjectName.find("\"",1)+1);
				else
					subjectName = subjectName.substr(0, subjectName.find(","));

				if (this->anomaly.count(subjectName)) return;

				//cout << "Предмет " << subjectName << "\n";
				//logFile << subjectName << "\n";
				Global->dictory[subjectName].Comp.insert(compName);
			}
		}
		catch(...){}
	}

	//Функция, с которой начинается определение компетенций для разных планов
	//(на вход подаётся путь до плана)
	void Calculate(auto fpath)
	{
		ifstream in(fpath);
		string line, buf;
		getline(in, buf);// В холостую считываем заголовок
		while (getline(in, buf))
		{
			line += buf;
			//Может быть перенос в самой ячейке, требуется конкатенация
			if (RED::CountQuote(line) % 2 == 0)
			{
				WhatComp(line);
				line.clear();
			}
		}

		//Normalizm();

	}

public:
	string compName; //Текущая компетенция

private:
	vector<string> shiftPrefixString; //= { "Тип задач проф. деятельности:" };
	//vector<string> badPrefixString;
	int positionParserComp;
	string shiftСolumn;

	set<string> anomaly; //В плане могут быть сдвинуты столбцы (аномалии)

} ;

/// <summary>
/// Методы, связанные с определение ЗЕ у дисциплин, а также то,
/// в каких семестрах они проводятся
/// </summary>
class UPlan
{
public:

	UPlan()
	{
		ifstream open("bad parser prefix.txt");
		string s;
		while (getline(open, s))
		{
			if (isFind(s, "Key word")) break;
		}
		getline(open, keyWord);
		getline(open, keyIndex);
		getline(open, keyIndexPhrase);
		getline(open, keyNumberSession);

		shiftColumn = -1; //В какой колонке наши данные
		shiftNumberSession = -1;  shiftColumnkeyIndex = 1;

		Statist::mid = 0;

	}


	void LineParser(string line, string name, sGlobal::Subject& s)
	{
		string nameIndex = "";
		//int dop = 0;
		//for (auto it : name) if (it == ',') ++dop;

		bool bError = false;
		for (int i = 0; i < shiftColumn; ++i)
		{

			while (line.find("\"") < line.find(","))
			{
				line = line.substr(line.find(",") + 1);
				bError = true;
			}
			if (bError)
			{
				line = "Error," + line;
				bError = false;
			}
			if (i == shiftNumberSession) keyNumberSession = line.substr(0, line.find(",")); //Экзамен
			if (i == shiftNumberSession + 1) keyNumberSession += line.substr(0, line.find(",")); //Зачёт
			if (i == shiftNumberSession + 2) keyNumberSession += line.substr(0, line.find(",")); //Зачёт с оц.
			if (i == shiftColumnkeyIndex) nameIndex = line.substr(0, line.find(","));
			line = line.substr(line.find(",") + 1);


		}
		line = line.substr(0, line.find(","));

		//Смотрим, в каких семестрах
		for (auto et : keyNumberSession)
			s.Session.insert(et - '0');

		//Это дисциплина по выбору
		string& n = nameIndex;
		int z = atoi(line.c_str());

		//Построение дерева дисциплин (может быть вложенность из-за модулей)
		
		 
		while (true)
		{
			if ((th->prt == nullptr) || (n.find(th->name) == 0))
			{
				//Это дисциплина по выбору?
				if (th->prt) //Да, так как есть общее название модуля
				{
					if (bErase)
					{
						Global->dictory.erase(name);
					}
					else
					{
						s.VU = th->zi;
						s.score = th->zi;// *Statist::PREC;
						//Statist::mid += s.score;
					}
					bErase = true;
				}
				else
				{
					bErase = false;
					//logFile << name << " = " << atoi(line.c_str()) << "\n";
					s.VU = z;
					s.score = z;// *Statist::PREC;
					//Statist::mid += s.score;
					
				}

				tree* newa = new tree;
				newa->zi = z;
				newa->name = n;
				newa->subject_name = name;
				
				th->tr.push_back(newa); // Запоминаем сына
				newa->prt = th;
				th = newa;
				break;
			}

			th = th->prt; // Поднимаемся вверх
		}
	}

	//Функция, с которой начинается определение компетенций для конкретного плана
	void WhatComp(string line)
	{
		if (shiftColumn == -1)
			if (isFind(line, keyWord))
			{
				string info = line;
				while (isFind(info, keyWord))
				{
					if (isFind(info, keyNumberSession)) ++shiftNumberSession;
					info = info.substr(info.find(",") + 1);
					++shiftColumn;
				}
				return;
			}

		if ((line.front() == '+') || (line.front() == '-'))
		{
			for (auto& it : Global->dictory)
			{
				// Добавил запятую, чтобы находить точное соответствие
				int pos = line.find(it.first + ",");

				if (pos != string::npos)
				{
					//Теперь делаю через дерево (как выше)
					LineParser(line, it.first, it.second);
					return;
				}
			}

			//Пойдёмся в холостую, чтобы добавить вершину в дерево
			sGlobal::Subject empty;
			LineParser(line, "", empty);
		}

	}

	//Усредняем вес каждой дисциплины (в зависимости от ЗЕ и кол-во дисциплин)
	void Normalizm()
	{
		Statist::mid /= Global->dictory.size();
		for (auto& it : Global->dictory)
		{
			//it.second.score *= Statist::PREC;
			it.second.score /= Statist::mid;// Отклонение от среднего
			it.second.score *= Statist::С_MIL;
		}

	}

	//Функция, с которой начинается определение компетенций для разных планов
	//(на вход подаётся путь до плана)
	void Calculate(auto fpath)
	{
		ifstream in(fpath);
		string buf;
		while (getline(in, buf))
			WhatComp(buf);

		for (auto it : root->tr)
		{
			if (it->tr.size() > 1)
			{
				Global->dictory.erase(it->subject_name);
				it->subject_name = it->tr[0]->subject_name;
			}
			
		}

		for (auto it : Global->dictory)
			Statist::mid += it.second.VU;


		Normalizm();
	}

private:
	bool bErase = false;
	string keyWord;
	string keyIndex; string keyIndexPhrase;
	int shiftColumn;
	int shiftColumnkeyIndex; // Ищем колонку, где указан индекс предметов (чтобы отыскать предметы по выбору)
	string keyNumberSession;
	int shiftNumberSession;
} ;

/// <summary>
/// Методы, связанные с построением графа связей дисциплин между собой
/// </summary>
class UGraph
{
private:
	int substrSize;
public:
	UGraph()
	{
		AL.resize(Global->dictory.size());

		ifstream open("bad parser prefix.txt");
		string s;
		while (getline(open, s))
			if (isFind(s, "Substr size")) break;
		getline(open, s);
		substrSize = atoi(s.c_str());
	}

	// Выстраиваем рёбра графа
	void CreateGraph()
	{
		int i = 0, j = 0; //Индексы

		for (auto it : Global->dictory)
		{
			j = 0;
			for (auto et : Global->dictory)
			{
				if (i < j)
				{
					double range = 0;
					for (auto sub : it.second.Comp)
					{
						if (et.second.Comp.count(sub))
							++range;
					}

					//Имеется связь по компетенциям
					if (range)
					{

						range = LinaryTransform(it.second.score,
							et.second.score, range);

						AL[i].push_back({ j,range });
						AL[j].push_back({ i,range });
					}
				}
				++j;
			}
			++i;
		}

	}

	//Усредняем веса ребёр
	double LinaryTransform(double st, double en, double num)
	{
		double mid = (st - en) / 2 + en;
		return  num * mid;

	}

	//Применяем псевдонимы для обозначения вершин
	string OutAlise(string ch_name)
	{
		if (Statist::alise.count(ch_name))
		{
			ch_name = Statist::alise[ch_name];
		}
		ch_name = ch_name.substr(0, substrSize);
		if (ch_name.front() == '"') ch_name += '"';
		return ch_name;
	}

	//Вывод всех рёбер
	string OutRib(auto a, auto b, double d)
	{
		string res;
		res = to_string(a) +"," +
			to_string(b) + "," +
			"Undirected,,,,," + to_string(d);
		return res;
	}

	//Вывод всех вершин и рёбер
	void OutGephiAL(string fileName)
	{
		ofstream out(fileName + Category::PlanName + "Rib.csv");
		ofstream outL(fileName + Category::PlanName + "Label.csv");

		ifstream open("bad parser prefix.txt");

		string s;
		while (getline(open, s))
			if (isFind(s, "HeaderRib")) break;
		getline(open, s);
		out << s << "\n";

		for (int i = 0; i < AL.size(); ++i)
		{
			for (int j = 0; j < AL[i].size(); ++j)
			{
				if (i < AL[i][j].first)
					out << OutRib(i, AL[i][j].first, AL[i][j].second) << "\n";

				/*out << i << "," <<
					AL[i][j].first << "," <<
					"Undirected,,,,," << AL[i][j].second << "\n";*/
			}
		}
		while (getline(open, s))
			if (isFind(s, "HeaderLabel")) break;
		getline(open, s);
		outL << s << "\n";

		int i = 0;
		for (auto& it : Global->dictory)
		{
			outL << i++ << "," << OutAlise(it.first) << "\n";
		}

		
	}

	//Вывод общих сведений про учебный план
	void OutCompWeight(string fileName)
	{
		ofstream out(fileName + Category::PlanName+"CompWeight.csv");

		out << "Comp,Weight\n";

		map<string, float> CW; map<int, map<string, float>> CW_course;
		int sum_all_comp = 0; map<int, float> sum_all_comp_course;
		int sum_VU = 0;
		int sum_VU_2 = 0;

		for (auto it : root->tr)
			sum_VU += it->zi;

		for (auto it : Global->dictory)
		{
			for (auto et : it.second.Comp)
			{
				CW[et] += it.second.VU / float(it.second.Comp.size());
			}
			sum_all_comp += it.second.VU;

			for (auto st : it.second.Session)
			{
				for (auto et : it.second.Comp)
				{
					CW_course[(st - 1) / 2 + 1][et] += it.second.VU / (float(it.second.Session.size()) * float(it.second.Comp.size()));
				}
				sum_all_comp_course[(st - 1) / 2 + 1] += it.second.VU/ float(it.second.Session.size());

			}
		}

		//Префикс компетенции хранит CW, но перевернутый
		map < string, vector<pair<float, string>>> AutoSort;
		for (auto it : CW)
		{
			string name = it.first;
			int pdef = 0;
			for (int i = 0; i < name.size(); ++i)
				if (name[i] == '-')
				{
					pdef = i;
					break;
				}

			name = name.substr(0, pdef);

			AutoSort[name].push_back({ it.second, it.first });
		}
		for (auto& it : AutoSort)
			sort(it.second.begin(), it.second.end(), greater<pair<int, string>>());

		Category::sData statistic; statistic.name = Category::PlanName;

		for (auto et : AutoSort)
		{
			float local_sum = 0;


			for (auto ch : et.first)
				out << (translit.count(ch)?translit[ch]:ch);

			for (auto it : et.second)
			{
				local_sum += it.first;
				out << ";"; 
				for (auto ch : it.second)
					out << (translit.count(ch) ? translit[ch] : ch);
				out << ";" << it.first;
				out << ";" << it.first * 100 / double(sum_all_comp) << "%";
				if (it != *(--et.second.end())) out << "\n";
			}
			out << ";" << local_sum;
			double result = local_sum * 100 / double(sum_all_comp);

			statistic.percent[et.first] = result; Category::CompetentionVarible.insert(et.first);
			out << ";" << result << "%\n";

			if (!Category::max_procent.count(et.first))
			{
				Category::max_procent[et.first] = result;
				Category::min_procent[et.first] = result;
			}
			else
			{
				if (Category::max_procent[et.first] < result)
					Category::max_procent[et.first] = result;

				if (Category::min_procent[et.first] > result)
					Category::min_procent[et.first] = result;
			}

		}
		statistic.VU = sum_VU;
		statistic.employ = Global->dictory.size();
		statistic.employ_whitout_repeat = root->tr.size();

		out << "Количество зачётных единиц на все предметы;" << sum_VU << "\n";
		out << "Количество зачётных единиц учтённых предметы;" << sum_all_comp << "\n";

		out << "Количество дисциплин;" << root->tr.size() << "\n";
		out << "Количество учтённых дисциплин;" << Global->dictory.size() << "\n";
		Category::AllStatistic.push_back(statistic);

		//А теперь распределение по курсам -----------------------------
		for (auto [course, what] : CW_course)
		{
			ofstream out_course(fileName + Category::PlanName + "CompWeight_" + to_string(course)+".csv");

			//Префикс компетенции хранит CW, но перевернутый
			map < string, vector<pair<float, string>>> AutoSort_course;
			for (auto it : CW_course[course])
			{
				string name = it.first;
				int pdef = 0;
				for (int i = 0; i < name.size(); ++i)
					if (name[i] == '-')
					{
						pdef = i;
						break;
					}

				name = name.substr(0, pdef);

				AutoSort_course[name].push_back({ it.second, it.first });
			}
			for (auto& it : AutoSort_course)
				sort(it.second.begin(), it.second.end(), greater<pair<int, string>>());

			for (auto et : AutoSort_course)
			{
				float local_sum = 0;
		
				for (auto ch : et.first)
					out_course << (translit.count(ch) ? translit[ch] : ch);

				for (auto it : et.second)
				{
					local_sum += it.first;
					out_course << ";";
					for (auto ch : it.second)
						out_course << (translit.count(ch) ? translit[ch] : ch);
					out_course << ";" << it.first;
					out_course << ";" << it.first * 100 / double(sum_all_comp_course[course]) << "%";
					if (it != *(--et.second.end())) out_course << "\n";
				}
				out_course << ";" << local_sum;
				double result = local_sum * 100 / double(sum_all_comp_course[course]);

				out_course << ";" << result << "%\n";


			}

		}
	}

	//Вывод рёбер и вершин для графа, в котором дисциплина разбивается по семестрам
	// (получается подобие фрактала)
	void OutWithCourse(string fileName)
	{
		ofstream outL(fileName + Category::PlanName + "LabelCourse.csv");
		ofstream out(fileName + Category::PlanName + "RibCourse.csv");

		ifstream open("bad parser prefix.txt");

		string s;

		while (getline(open, s))
			if (isFind(s, "HeaderLabel")) break;
		getline(open, s);
		outL << s << "\n";

		open.close(); open.open("bad parser prefix.txt");
		while (getline(open, s))
			if (isFind(s, "HeaderRib")) break;
		getline(open, s);
		out << s << "\n";

		int i = 0;
		for (auto& [l, r] : Global->dictory)
		{
			r.id = i++;
			int preSession = -2;
			for (auto et : r.Session)
			{
				outL << r.id * Statist::MAX_Course + et << ","
					<< OutAlise(l) + "_" + to_string(et) << "\n";
				if (preSession == et - 1) //Связь между одной и той же дисциплинной, но в разном семестре
				{
					double range = 0;
					range = LinaryTransform(r.score / r.Session.size(), 0, 1);
					out << OutRib(r.id * Statist::MAX_Course + et, r.id * Statist::MAX_Course + et - 1, range) << "\n";
				}
				preSession = et;
			}

		}

		vector <sGlobal::Subject*> now;
		for (int i = 1; i <= Statist::MAX_Course; ++i)
		{
			now.clear();
			for (auto& [l, r] : Global->dictory)
			{
				if (r.Session.count(i))
					now.push_back(&r);
			}
			for (auto& et : now)
			{
				for (auto& zt : now)
				{
					if (et == zt) continue;
					double range = 0;
					for (auto sub : zt->Comp)
					{
						if (et->Comp.count(sub))
							++range;
					}

					//Имеется связь по компетенциям
					if (range)
					{

						range = LinaryTransform(zt->score / zt->Session.size(),
							et->score / et->Session.size(), range);

						out << OutRib(et->id * Statist::MAX_Course + i, zt->id *	Statist::MAX_Course + i, range) << "\n";
					}
				}
			}

		}
		
	}

public:
	//Введём id, как в упорядоченом порядок 
	vector<vector<pair<int, double>> > AL; //Список смежности (Adjacency list)

} ;


//Обработка всех планов
bool Solve(string input_path, string output_path)
{
	root = new tree;
	root->name = "";
	root->zi = 0;
	root->prt = nullptr;
	th = root;

	Global.reset();
	Global = std::make_unique<sGlobal>();

	//logFile << "Найденные предметы из вкладки \"Компетенции\"" << "\n";
	UCompetency Competency;
	for (auto it : filesystem::directory_iterator(input_path))
	{
		string spath = it.path().string();
		if (isFind(spath, "Компетенции.")) { Competency.Calculate(it); continue; }
	}

	//logFile << "------------------------------------\n";
	//logFile << "Подсчёт ЗЕ из вкладки \"План\"" << "\n";
	UPlan Plan;
	for (auto it : filesystem::directory_iterator(input_path))
	{
		string spath = it.path().string();
		if (isFind(spath, "План.")) { Plan.Calculate(it); continue; }
	}

	UGraph Graph;
	Graph.CreateGraph();
	Graph.OutGephiAL(output_path);
	Graph.OutCompWeight(output_path);

	Graph.OutWithCourse(output_path);

//#ifdef _DEBUG
	Global->OutDictory();
//#endif

	return true;
}

//Очистка данных после подсчёта
void clearingAll()
{
	//Category::min_sum_VU = 1e9;
	//Category::max_sum_VU = 0;

	//Category::min_employ = 1e9;
	//Category::max_employ = 0;

	Category::min_procent.clear();
	Category::max_procent.clear();

	Category::AllStatistic.clear();
	Category::CompetentionVarible.clear();
}

//Вывод общих сведений про все учебные планы
void TotalInfo(string outFile)
{
	string et = outFile + "/general_" + outFile.substr(outFile.find('//')+1) + ".csv";
	ofstream out(et);
	//ofstream& out = Category::outAllStatistic;
	//Category::outHeader("Общее");

	out << "учебный план,";
	for (auto it : Category::CompetentionVarible)
		out << it << ",";
	out << "з.е,кол-во дисциплин,без учёта по выбору\n";

	int min_sum_VU = 1e9, max_sum_VU = 0;
	int min_employ = 1e9, max_employ = 0;

	for (auto et : Category::AllStatistic)
	{
		out << et.name << ",";
		for (auto it : Category::CompetentionVarible)
			out << et.percent[it] << ",";

		out << et.VU << ","; 
		min_sum_VU = min(min_sum_VU, et.VU); max_sum_VU = max(max_sum_VU, et.VU);
		
		out << et.employ << ",";

		out << et.employ_whitout_repeat << ",";
		min_employ = min(min_employ, et.employ_whitout_repeat); max_employ = max(max_employ, et.employ_whitout_repeat);

			out << "\n";
	}


	out << "Коридор з.е," << min_sum_VU
		<< " - " << max_sum_VU << "\n";

	out << "Коридор дисциплин," << min_employ
		<< " - " << max_employ << "\n";

	out << "Коридор по каждой компетенции \n";

	for (auto it : Category::min_procent)
	{
		out << it.first << ",";
		out << it.second << " - " << Category::max_procent[it.first];
		out << "\n";
	}

}

//Загрузка псевдонимов в программу
void create_alise()
{
	ifstream al("alias.csv");
	string s, name, ch_name;
	getline(al, s);
	while (getline(al, s))
	{
		int pos = 0;
		while (true)
		{
			if (s.find('\"', pos) != string::npos) pos = s.find('\"', pos) + 1;
			else break;
		}
		name = s.substr(0, s.find(',', pos));
		ch_name = s.substr(s.find(',', pos)+1);
		Statist::alise[name] = ch_name;
		cout << name << " " << ch_name << "\n";
	}
}

//Начальная точка входа в программу
int main()
{

//#ifdef _DEBUG
	ios_base::sync_with_stdio(0); cin.tie(0); 
	FILE* IN, * OUT;
	//freopen_s(&IN, "input.txt", "r", stdin);
	freopen_s(&OUT, "LogFile.txt", "w", stdout); //Вывод данных в текстовый файл
//#endif

	create_alise();//Создание массива псевдонимов

	auto file = filesystem::current_path(); //Взятие пути директории расположения exe файла

	int numCategories = 0;
	
	ifstream in("direct of file.txt");

	in >> numCategories; in.ignore();
	vector<string> input_path(numCategories);
	vector<string> output_path(numCategories);

	for (int category = 0; category < numCategories; ++category)
		getline(in, input_path[category]);
	for (int category = 0; category < numCategories; ++category)
		getline(in, output_path[category]);

	string create;
	getline(in, create);

	bool bCreateFolder = (create.substr(create.size() - 4) == "true");

	for (int category = 0; category < numCategories; ++category)
	{
		clearingAll();

		auto input_file = file / input_path[category];
		//Код ошибки - нет файлов указанного формата, из которых ожидалось считывание
		if (!filesystem::exists(input_file)) return 3;  
		auto output_file = file / output_path[category];
		if (!filesystem::exists(output_file))
		{
			if (bCreateFolder)
			{
				filesystem::create_directory(output_file);
			}
			else
			return 4; //Код ошибки - не удаётся создать папку для вывода
		}
		//Category::outAllStatistic.open(output_path[category] + "/general.csv");

		if (bCreateFolder)
			for (auto it : filesystem::directory_iterator(input_file))
			{
				if (!it.is_directory())
				{
					int last_point = 0;
					string s = it.path().string();
					while (s[last_point] != '.') ++last_point;
					s = s.substr(0, last_point);
					filesystem::create_directory(s);
				}
			}

		for (auto it : filesystem::directory_iterator(input_file))
		{
			if (it.is_directory())
			{
				auto out = output_file / it.path().filename() / "";
				filesystem::create_directory(out);

				Category::PlanName = it.path().filename().string();

				cout << out.string() << "\n";

				//Работа с конкретным планом
				Solve(it.path().string(), out.string());
			}
		}

		//Вывод общей статистики про все планы
		TotalInfo(output_path[category]);
	}


	return 0;
}