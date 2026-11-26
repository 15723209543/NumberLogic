#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "AI.h"
#include <string>
#include <cstdio>
// 数字校验与反馈计算。
bool isdigit(int cinnumbers,int digit);
bool iszero(int cinnumber[],int digit);
bool issimilar(int cinnumber[],int digit);
void numbertoarray(int number,int digit,int cinnumber[]);
int isA(int randnumber[],int cinnumber[],int digit);
int isB(int randnumber[],int cinnumber[],int digit);
string checknumber(const string& cinnumbers,int digit);
Feedback judgeAB(const string& randnumbers,const string& cinnumbers);
string getrandnumber(int digit);
int limitnumber(int number,int low,int high);
enum class Mode { Classic, Campaign, Duel, Practice };
enum class State { Playing, Won, Lost };
string getmodename(Mode mode);
const int historymax=1024;
struct Round {
    string randnumbers;
    int digit=4;
    int maxcount=10;
    int level=1;
    int count=0;
    State state=State::Playing;
    Mode mode=Mode::Classic;
    Attempt history[historymax];
    void start(Mode m,int d=4,int attempts=10,int stage=1,string secret="");
    string submit(const string& guess);
    int getcount() const;
    bool next();
    string reveal() const;
    Feedback evaluate(const string& guess) const;
};
struct Record {
    string date,mode;
    int digit=0,level=0,count=0,maxcount=0;
    bool won=false;
    int seconds=0;
    string secret,gethistory;
};
const int recordmax=2000;
struct Stats {
    int rounds=0,wins=0,best=0,highest=0;
    double mean=0;
};
struct Store {
    wstring root_;
    explicit Store(wstring root):root_(root) {}
    bool append(const Record& r) const;
    int load(Record records[],int maxnumber) const;
    bool exportcsv(const wstring& path) const;
    bool clear() const;
    wstring root() const { return root_; }
};
Stats countresult(const Record records[],int recordcount,string mode="");
string gethistory(const Attempt history[],int count);
string gettime();
struct Settings {
    int digit=4,attempts=10,strength=2;
    bool dark=false;
    bool load(const wstring& path);
    bool save(const wstring& path) const;
};
#ifdef _WIN32
int runUI(HINSTANCE instance,int show);
#endif
