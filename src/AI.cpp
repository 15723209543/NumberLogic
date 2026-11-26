#include "UI.h"
#include <cstdlib>
using namespace std;
// 按位枚举所有不重复的数字组合。
void getallnumber(int number,int digit,int now,bool panduan[],int possiblenumber[],int &numbercount) {
    if(now==digit) {
        possiblenumber[numbercount]=number;
        numbercount++;
        return;
    }
    for(int i=1;i<=9;i++) {
        if(!panduan[i]) {
            panduan[i]=true;
            getallnumber(number*10+i,digit,now+1,panduan,possiblenumber,numbercount);
            panduan[i]=false; // 返回上一层后该数字可以重新使用
        }
    }
}
void Solver::reset(int newdigit) {
    digit=limitnumber(newdigit,1,9);
    numbercount=0;
    bool panduan[10]={false};
    getallnumber(0,digit,0,panduan,possiblenumber,numbercount);
}
// 只有与已经取得的 A/B 反馈一致的数字才保留。
void Solver::removenumber(const Attempt& attempt) {
    if(!checknumber(attempt.guess,digit).empty()) {
        return;
    }
    int cinnumber[9],randnumber[9];
    numbertoarray(atoi(attempt.guess.c_str()),digit,cinnumber);
    int count=0;
    for(int i=0;i<numbercount;i++) {
        numbertoarray(possiblenumber[i],digit,randnumber);
        if(isA(randnumber,cinnumber,digit)==attempt.result.a &&
           isB(randnumber,cinnumber,digit)==attempt.result.b) {
            possiblenumber[count]=possiblenumber[i];
            count++;
        }
    }
    numbercount=count; // 更新筛选后的数量
}
// 用普通数组记录每种反馈出现的次数。
long long countscore(int guess,const int possiblenumber[],int numbercount,int digit,int strength) {
    int ABcount[10][10]={0};
    int cinnumber[9],randnumber[9];
    numbertoarray(guess,digit,cinnumber);
    int samplecount=numbercount;
    if(samplecount>1200) {
        samplecount=1200; // 候选太多时抽样计算
    }
    for(int i=0;i<samplecount;i++) {
        int position=i*numbercount/samplecount;
        numbertoarray(possiblenumber[position],digit,randnumber);
        int A=isA(randnumber,cinnumber,digit);
        int B=isB(randnumber,cinnumber,digit);
        ABcount[A][B]++;
    }
    long long score=0;
    int maxcount=0;
    for(int i=0;i<=digit;i++) {
        for(int j=0;j<=digit;j++) {
            int count=ABcount[i][j];
            score+=(long long)count*count; // 分组越集中，分数越高
            if(count>maxcount) {
                maxcount=count;
            }
        }
    }
    if(strength==3) { // 高难度还要考虑最大的分组
        score+=(long long)maxcount*samplecount;
    }
    return score;
}
string Solver::guessnumber(int strength) const {
    if(numbercount==0) {
        return "";
    }
    if(strength==1 || numbercount<=2) {
        return to_string(possiblenumber[0]);
    }
    int guesscount=strength==3?48:20;
    if(guesscount>numbercount) {
        guesscount=numbercount;
    }
    long long minscore=-1;
    int bestnumber=possiblenumber[0];
    for(int i=0;i<guesscount;i++) {
        int number=possiblenumber[i*numbercount/guesscount];
        long long score=countscore(number,possiblenumber,numbercount,digit,strength);
        if(minscore==-1 || score<minscore) {
            minscore=score;
            bestnumber=number; // 选分数最低的数字
        }
    }
    return to_string(bestnumber);
}
int Solver::size() const {
    return numbercount;
}
