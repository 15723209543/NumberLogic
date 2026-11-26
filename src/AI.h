#pragma once
#include <string>
using namespace std;
struct Feedback {
    int a=0;
    int b=0;
    bool operator==(const Feedback& r) const { return a==r.a && b==r.b; }
    string str() const;
};
struct Attempt {
    string guess;
    Feedback result;
};
// AI 只接收猜测及反馈，不读取谜底。
struct Solver {
    int possiblenumber[362880];
    int numbercount=0;
    int digit=4;
    void reset(int newdigit);
    void removenumber(const Attempt& attempt);
    string guessnumber(int strength=2) const;
    int size() const;
};
