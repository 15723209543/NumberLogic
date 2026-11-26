#include "../src/UI.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
void check(bool panduan,const char* message) {
    if(!panduan) {
        cerr<<"FAIL: "<<message<<endl;
        exit(1);
    }
}
int main() {
    srand(20261126);
    int number[9];
    numbertoarray(987654321,9,number);
    check(number[0]==9 && number[8]==1,"numbertoarray");
    check((judgeAB("3579","2517")==Feedback{1,1}),"course example");
    check((judgeAB("1234","4321")==Feedback{0,4}),"all misplaced");
    check((judgeAB("1234","1234")==Feedback{4,0}),"exact");
    check((judgeAB("1234","5678")==Feedback{0,0}),"none");
    check(!checknumber("0123",4).empty(),"zero");
    check(!checknumber("1123",4).empty(),"duplicates");
    check(!checknumber("a123",4).empty(),"letters");
    check(!checknumber("12345",4).empty(),"length");
    for(int d=1;d<=9;d++) {
        for(int i=0;i<100;i++) check(checknumber(getrandnumber(d),d).empty(),"random validity");
    }
    Round r;
    r.start(Mode::Campaign,4,10,1,"1234");
    check(!r.submit("1111").empty() && r.count==0,"invalid attempt free");
    check(r.submit("4321").empty(),"valid attempt");
    check(r.submit("1234").empty() && r.state==State::Won,"win");
    check(r.getcount()==9,"original winning guess carry");
    check(r.next() && r.digit==5 && r.maxcount==19 && r.level==2,"carry");
    r.start(Mode::Campaign,9,10,6,"123456789");
    r.submit("123456789");
    check(r.next() && r.digit==9,"nine digit cap");
    r.start(Mode::Classic,4,2,1,"1234");
    r.submit("4321");
    r.submit("5678");
    check(r.state==State::Lost && r.getcount()==0,"loss");
    check(!r.submit("1234").empty(),"no postgame guess");
    static Solver solver;
    solver.reset(4);
    check(solver.size()==3024,"candidate count");
    solver.removenumber({"1234",{4,0}});
    check(solver.size()==1 && solver.guessnumber()=="1234","exact filter");
    int maxcount=0;
    for(int strength=1;strength<=3;strength++) {
        for(int trial=0;trial<30;trial++) {
            string secret=getrandnumber(4);
            solver.reset(4);
            bool solved=false;
            for(int t=0;t<30;t++) {
                string choice=solver.guessnumber(strength);
                check(checknumber(choice,4).empty(),"AI legal guess");
                Feedback f=judgeAB(secret,choice);
                solver.removenumber({choice,f});
                bool retained=false;
                for(int j=0;j<solver.size();j++) {
                    if(solver.possiblenumber[j]==atoi(secret.c_str())) retained=true;
                }
                check(retained,"secret retained");
                if(f.a==4) {
                    solved=true;
                    if(t+1>maxcount) maxcount=t+1;
                    break;
                }
            }
            check(solved,"AI eventual solution");
        }
    }
    solver.reset(9);
    check(solver.size()==362880,"nine digit enumeration");
    solver.removenumber({"123456789",{9,0}});
    check(solver.size()==1,"nine digit exact filtering");
    Store store(L".");
    check(store.clear(),"initial test history clear");
    Record record{gettime(),"经典",4,1,2,10,true,5,"1234","4321:0A4B;1234:4A0B"};
    check(store.append(record),"record append");
    Record records[2];
    check(store.load(records,2)==1 && records[0].gethistory==record.gethistory,"record reload");
    Stats stats=countresult(records,1);
    check(stats.rounds==1 && stats.wins==1 && stats.mean==2,"summary");
    check(store.exportcsv(L"test-export.csv"),"CSV export");
    Settings settings;
    settings.digit=6;
    settings.dark=true;
    check(settings.save(L"test-settings.txt"),"settings save");
    Settings loaded;
    check(loaded.load(L"test-settings.txt") && loaded.digit==6 && loaded.dark,"settings reload");
    check(store.clear() && store.load(records,2)==0,"clear history");
    cout<<"PASS: original rules, 900 random numbers, campaign, 90 randomized AI games, 9-digit enumeration/filtering, persistence, CSV, statistics, settings"<<endl;
    cout<<"Maximum guesses in 90 AI games: "<<maxcount<<endl;
    return 0;
}
