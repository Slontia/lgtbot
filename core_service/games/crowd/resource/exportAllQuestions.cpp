
#include <iostream>
#include <time.h>
#include <math.h>
#include <numeric>

#include "../problems.h"
#include "../problems_t.h"

using namespace std;

// formal questions
constexpr static uint32_t k_question_num = 78;
// with test1 questions
constexpr static uint32_t all_question_num = 132;
// test2 questions
constexpr static uint32_t t_question_num = 24;


static const std::array<Question*(*)(), all_question_num> create_question{
    []() -> Question* { return new Q1(); },
    []() -> Question* { return new Q2(); },
    []() -> Question* { return new Q3(); },
    []() -> Question* { return new Q4(); },
    []() -> Question* { return new Q5(); },
    []() -> Question* { return new Q6(); },
    []() -> Question* { return new Q7(); },
    []() -> Question* { return new Q8(); },
    []() -> Question* { return new Q9(); },
    []() -> Question* { return new Q10(); },
    []() -> Question* { return new Q11(); },
    []() -> Question* { return new Q12(); },
    []() -> Question* { return new Q13(); },
    []() -> Question* { return new Q14(); },
    []() -> Question* { return new Q15(); },
    []() -> Question* { return new Q16(); },
    []() -> Question* { return new Q17(); },
    []() -> Question* { return new Q18(); },
    []() -> Question* { return new Q19(); },
    []() -> Question* { return new Q20(); },
    []() -> Question* { return new Q21(); },
    []() -> Question* { return new Q22(); },
    []() -> Question* { return new Q23(); },
    []() -> Question* { return new Q24(); },
    []() -> Question* { return new Q25(); },
    []() -> Question* { return new Q26(); },
    []() -> Question* { return new Q27(); },
    []() -> Question* { return new Q28(); },
    []() -> Question* { return new Q29(); },
    []() -> Question* { return new Q30(); },
    []() -> Question* { return new Q31(); },
    []() -> Question* { return new Q32(); },
    []() -> Question* { return new Q33(); },
    []() -> Question* { return new Q34(); },
    []() -> Question* { return new Q35(); },
    []() -> Question* { return new Q36(); },
    []() -> Question* { return new Q37(); },
    []() -> Question* { return new Q38(); },
    []() -> Question* { return new Q39(); },
    []() -> Question* { return new Q40(); },
    []() -> Question* { return new Q41(); },
    []() -> Question* { return new Q42(); },
    []() -> Question* { return new Q43(); },
    []() -> Question* { return new Q44(); },
    []() -> Question* { return new Q45(); },
    []() -> Question* { return new Q46(); },
    []() -> Question* { return new Q47(); },
    []() -> Question* { return new Q48(); },
    []() -> Question* { return new Q49(); },
    []() -> Question* { return new Q50(); },
    []() -> Question* { return new Q51(); },
    []() -> Question* { return new Q52(); },
    []() -> Question* { return new Q53(); },
    []() -> Question* { return new Q54(); },
    []() -> Question* { return new Q55(); },
    []() -> Question* { return new Q56(); },
    []() -> Question* { return new Q57(); },
    []() -> Question* { return new Q58(); },
    []() -> Question* { return new Q59(); },
    []() -> Question* { return new Q60(); },
    []() -> Question* { return new Q61(); },
    []() -> Question* { return new Q62(); },
    []() -> Question* { return new Q63(); },
    []() -> Question* { return new Q64(); },
    []() -> Question* { return new Q65(); },
    []() -> Question* { return new Q66(); },
    []() -> Question* { return new Q67(); },
    []() -> Question* { return new Q68(); },
    []() -> Question* { return new Q69(); },
    []() -> Question* { return new Q70(); },
    []() -> Question* { return new Q71(); },
    []() -> Question* { return new Q72(); },
    []() -> Question* { return new Q73(); },
    []() -> Question* { return new Q74(); },
    []() -> Question* { return new Q75(); },
    []() -> Question* { return new Q76(); },
    []() -> Question* { return new Q77(); },
    []() -> Question* { return new Q78(); },
    // test questions
    []() -> Question* { return new Q79(); },
    []() -> Question* { return new Q80(); },
    []() -> Question* { return new Q81(); },
    []() -> Question* { return new Q82(); },
    []() -> Question* { return new Q83(); },
    []() -> Question* { return new Q84(); },
    []() -> Question* { return new Q85(); },
    []() -> Question* { return new Q86(); },
    []() -> Question* { return new Q87(); },
    []() -> Question* { return new Q88(); },
    []() -> Question* { return new Q89(); },
    []() -> Question* { return new Q90(); },
    []() -> Question* { return new Q91(); },
    []() -> Question* { return new Q92(); },
    []() -> Question* { return new Q93(); },
    []() -> Question* { return new Q94(); },
    []() -> Question* { return new Q95(); },
    []() -> Question* { return new Q96(); },
    []() -> Question* { return new Q97(); },
    []() -> Question* { return new Q98(); },
    []() -> Question* { return new Q99(); },
    []() -> Question* { return new Q100(); },
    []() -> Question* { return new Q101(); },
    []() -> Question* { return new Q102(); },
    []() -> Question* { return new Q103(); },
    []() -> Question* { return new Q104(); },
    []() -> Question* { return new Q105(); },
    []() -> Question* { return new Q106(); },
    []() -> Question* { return new Q107(); },
    []() -> Question* { return new Q108(); },
    []() -> Question* { return new Q109(); },
    []() -> Question* { return new Q110(); },
    []() -> Question* { return new Q111(); },
    []() -> Question* { return new Q112(); },
    []() -> Question* { return new Q113(); },
    []() -> Question* { return new Q114(); },
    []() -> Question* { return new Q115(); },
    []() -> Question* { return new Q116(); },
    []() -> Question* { return new Q117(); },
    []() -> Question* { return new Q118(); },
    []() -> Question* { return new Q119(); },
    []() -> Question* { return new Q120(); },
    []() -> Question* { return new Q121(); },
    []() -> Question* { return new Q122(); },
    []() -> Question* { return new Q123(); },
    []() -> Question* { return new Q124(); },
    []() -> Question* { return new Q125(); },
    []() -> Question* { return new Q126(); },
    []() -> Question* { return new Q127(); },
    []() -> Question* { return new Q128(); },
    []() -> Question* { return new Q129(); },
    []() -> Question* { return new Q130(); },
    []() -> Question* { return new Q131(); },
    []() -> Question* { return new Q132(); },
};

// test mode 2
static const std::array<Question*(*)(), t_question_num> test_question{
    []() -> Question* { return new Qt1(); },
    []() -> Question* { return new Qt2(); },
    []() -> Question* { return new Qt3(); },
    []() -> Question* { return new Qt4(); },
    []() -> Question* { return new Qt5(); },
    []() -> Question* { return new Qt6(); },
    []() -> Question* { return new Qt7(); },
    []() -> Question* { return new Qt8(); },
    []() -> Question* { return new Qt9(); },
    []() -> Question* { return new Qt10(); },
    []() -> Question* { return new Qt11(); },
    []() -> Question* { return new Qt12(); },
    []() -> Question* { return new Qt13(); },
    []() -> Question* { return new Qt14(); },
    []() -> Question* { return new Qt15(); },
    []() -> Question* { return new Qt16(); },
    []() -> Question* { return new Qt17(); },
    []() -> Question* { return new Qt18(); },
    []() -> Question* { return new Qt19(); },
    []() -> Question* { return new Qt20(); },
    []() -> Question* { return new Qt21(); },
    []() -> Question* { return new Qt22(); },
    []() -> Question* { return new Qt23(); },
    []() -> Question* { return new Qt24(); },
};

string init_question(const int id, const int mode)
{
    vector<Player> players;
    Player tempP;
    for (int i = 0; i < 10; i++) {
        players.push_back(tempP);
    }
    Question* question;
    if (mode != 2) {
        question = create_question[id - 1]();
    } else {
        question = test_question[id - 1]();
    }
    question -> init(players);
    question -> initTexts(players);
    question -> initOptions();
    thread_local static string str;
    str = "";
    str += string("题号：#") + (mode == 2 ? "t" : "") + to_string(question -> id) + "\n";
    str += "出题者：" + (question -> author) + "\n";
    if (mode == 1) {
        str += "[测试] ";
    } else if (mode == 2) {
        str += "[其他/废弃] ";
    }
    str += "题目：" + (question -> title) + "\n\n";
    str += question -> String();
    delete question;
    return str;
}

string export_questions()
{
    thread_local static string questions_str;
    for (int i = 1; i <= all_question_num; i++) {
        questions_str += "------------------------------------------------------------\n";
        if (i == k_question_num + 1) {
            questions_str += "\n**********************************************************************";
            questions_str += "\n++下方为测试题库，平衡性尚未测试完全，不会出现在正式游戏中++";
            questions_str += "\n**********************************************************************\n\n";
        }
        questions_str += init_question(i, i > k_question_num);
    }
    questions_str += "\n**********************************************************************";
    questions_str += "\n++下方为其他/废弃题库，可能不再考虑修改或更新，不会出现在正式游戏中++";
    questions_str += "\n**********************************************************************\n\n";
    for (int i = 1; i <= t_question_num; i++) {
        questions_str += "------------------------------------------------------------\n";
        questions_str += init_question(i, 2);
    }
    return questions_str;
}

int main()
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    char file[50];

    sprintf(file, "乌合之众题库_%d.%d.%d_%d-%d-%d.txt", 1900 + p->tm_year, 1+ p->tm_mon, p->tm_mday, k_question_num, all_question_num, t_question_num);
    freopen(file, "w", stdout);

    cout << "乌合之众题库" << endl << endl;
    cout << "本记录导出时间：" << ctime(&timep) << endl;
    cout << "总题目数：" << (all_question_num + t_question_num) << "（包含 " << t_question_num << " 个废弃题目）" << endl;
    cout << "正式题库题目数：" << k_question_num << endl << endl;
    cout << "注：部分题目数字会根据参与游戏的人数和当前分数发生变化，此展示题库默认10人参与游戏且所有人分数为0" << endl << endl;

    cout << export_questions();
    return 0;
}