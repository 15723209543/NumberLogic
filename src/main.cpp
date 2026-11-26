#include "UI.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <cstdio>
#include <cerrno>
#include <cwchar>
using namespace std;
bool isdigit(int cinnumbers,int digit) {
	return !(cinnumbers<pow(10,digit)&&cinnumbers>=pow(10,digit-1));
}

bool iszero(int cinnumber[ ],int digit) {
	bool panduan=false;
	for(int i=0; i<digit; i++) {
		if(cinnumber[i]==0)
			panduan=true;
	}
	return panduan;
}

bool issimilar(int cinnumber[ ],int digit) {
	bool panduan=false;
	for(int i=0; i<digit; i++) {
		for(int j=0; j<digit; j++) {
			if(cinnumber[i]==cinnumber[j]&&i!=j)
				panduan=true;
		}
	}
	return panduan;
}

void numbertoarray(int number,int digit,int cinnumber[1000]) {
	int i=0;
	int j;
	j=digit-1;
	while(i!=digit) {
		cinnumber[i]=number/static_cast<int>(pow(10,j));
		number=number%(static_cast<int>(pow(10,j)));
		i++;
		j--;
	}
}

int isA(int randnumber[1000],int cinnumber[1000],int digit) {
	int count=0;
	for(int i=0; i<digit; i++) {
		if(randnumber[i]==cinnumber[i])
			count++;
	}
	return count;
}

int isB(int randnumber[1000],int cinnumber[1000],int digit) {
	int count=0;
	for(int i=0; i<digit; i++) {
		for(int j=0; j<digit; j++) {
			if(randnumber[i]==cinnumber[j]&&i!=j)
				count++;
		}
	}
	return count;
}

int limitnumber(int number,int low,int high) {
    if(number<low) return low;
    if(number>high) return high;
    return number;
}
string Feedback::str() const {
    return to_string(a)+"A"+to_string(b)+"B";
}
string checknumber(const string& cinnumbers,int digit) {
    if(digit<1 || digit>9 || (int)cinnumbers.size()!=digit) {
        return "位数不正确";
    }
    for(int i=0;i<digit;i++) {
        if(cinnumbers[i]<'0' || cinnumbers[i]>'9') {
            return "只能输入数字";
        }
    }
    int number=atoi(cinnumbers.c_str());
    int cinnumber[9];
    numbertoarray(number,digit,cinnumber);
    if(iszero(cinnumber,digit)) return "数字不能含有0";
    if(isdigit(number,digit)) return "位数不正确";
    if(issimilar(cinnumber,digit)) return "数字不能重复";
    return "";
}
Feedback judgeAB(const string& randnumbers,const string& cinnumbers) {
    Feedback panduan;
    int digit=(int)randnumbers.size();
    if(!checknumber(randnumbers,digit).empty() || !checknumber(cinnumbers,digit).empty()) {
        return panduan;
    }
    int randnumber[9],cinnumber[9];
    numbertoarray(atoi(randnumbers.c_str()),digit,randnumber);
    numbertoarray(atoi(cinnumbers.c_str()),digit,cinnumber);
    panduan.a=isA(randnumber,cinnumber,digit);
    panduan.b=isB(randnumber,cinnumber,digit);
    return panduan;
}
string getrandnumber(int digit) {
    digit=limitnumber(digit,1,9);
    int randnumber[9];
    // 数字重复时重新抽取该位。
    for(int i=0;i<digit;i++) {
        do {
            randnumber[i]=rand()%9+1;
        } while(issimilar(randnumber,i+1));
    }
    string number;
    for(int i=0;i<digit;i++) {
        number+=char('0'+randnumber[i]);
    }
    return number;
}
string getmodename(Mode mode) {
    switch (mode) {
        case Mode::Classic: return "经典";
        case Mode::Campaign: return "闯关";
        case Mode::Duel: return "AI对战";
        case Mode::Practice: return "训练";
    }
    return "未知";
}
void Round::start(Mode m, int d, int attempts, int stage, string secret) {
    mode = m;
    digit = limitnumber(d,1,9);
    maxcount = limitnumber(attempts,1,historymax);
    level = stage;
    count = 0;
    
    state = State::Playing;
    randnumbers = checknumber(secret, digit).empty() ? secret : getrandnumber(digit);
}
string Round::submit(const string& guess) {
    if (state != State::Playing) {
        return "本轮已经结束";
    }
    string error = checknumber(guess, digit);
    if (!error.empty()) {
        return error;
    }
    Feedback f = judgeAB(randnumbers, guess);
    history[count].guess=guess;
    history[count].result=f;
    ++count;
    if (f.a == digit) {
        state = State::Won;
    } else if (count >= maxcount) {
        state = State::Lost;
    }
    return {};
}
int Round::getcount() const {
    if(mode==Mode::Campaign && state==State::Won) {
        return maxcount-count+1;
    }
    return maxcount-count;
}
bool Round::next() {
    if (mode != Mode::Campaign || state != State::Won) {
        return false;
    }
    int carry = getcount(); // 猜中时不扣除继承机会
    start(mode, limitnumber(digit+1,1,9), carry + 10, level + 1);
    return true;
}
string Round::reveal() const {
    return state == State::Playing ? string("") : randnumbers;
}
Feedback Round::evaluate(const string& guess) const {
    return judgeAB(randnumbers, guess);
}
// 文件采用普通 C 文件接口，Windows 下支持中文路径。
FILE* openfile(const wstring& path,const wchar_t* mode) {
#ifdef _WIN32
    FILE* file=NULL;
    if(_wfopen_s(&file,path.c_str(),mode)!=0) return NULL;
    return file;
#else
    string filename(path.begin(),path.end());
    wstring mode1=mode;
    string mode2(mode1.begin(),mode1.end());
    return fopen(filename.c_str(),mode2.c_str());
#endif
}
string gettime() {
    time_t now=time(NULL);
    tm local;
#ifdef _WIN32
    localtime_s(&local,&now);
#else
    localtime_r(&now,&local);
#endif
    char date[40];
    strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",&local);
    return date;
}
string gethistory(const Attempt history[],int count) {
    string result;
    for(int i=0;i<count;i++) {
        if(i!=0) result+=';';
        result+=history[i].guess+":"+history[i].result.str();
    }
    return result;
}
bool Store::append(const Record& r) const {
    FILE* file=openfile(root_+L"/history.tsv",L"ab");
    if(file==NULL) return false;
    int result=fprintf(file,"%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
        r.date.c_str(),r.mode.c_str(),r.digit,r.level,r.count,r.maxcount,
        (int)r.won,r.seconds,r.secret.c_str(),r.gethistory.c_str());
    int closed=fclose(file);
    return result>=0 && closed==0;
}
// 读取一行记录，字段采用制表符分隔。
bool readrecord(const string& line,Record& r) {
    string field[10];
    int count=0;
    for(size_t i=0;i<line.size();i++) {
        if(line[i]=='\t') {
            count++;
            if(count>=10) return false;
        } else if(line[i]!='\r' && line[i]!='\n') {
            field[count]+=line[i];
        }
    }
    if(count!=9) return false;
    for(int i=2;i<=7;i++) {
        if(field[i].empty() || field[i].size()>9) return false;
        for(size_t j=0;j<field[i].size();j++) {
            if(field[i][j]<'0' || field[i][j]>'9') return false;
        }
    }
    r.date=field[0];
    r.mode=field[1];
    r.digit=atoi(field[2].c_str());
    r.level=atoi(field[3].c_str());
    r.count=atoi(field[4].c_str());
    r.maxcount=atoi(field[5].c_str());
    r.won=field[6]=="1";
    r.seconds=atoi(field[7].c_str());
    r.secret=field[8];
    r.gethistory=field[9];
    return r.count>0 && r.count<=r.maxcount && r.maxcount<=historymax &&
        r.level>0 && checknumber(r.secret,r.digit).empty();
}
int Store::load(Record records[],int maxnumber) const {
    FILE* file=openfile(root_+L"/history.tsv",L"rb");
    if(file==NULL || maxnumber<=0) {
        if(file!=NULL) fclose(file);
        return 0;
    }
    const int linesize=32768;
    char* line=new char[linesize];
    int count=0;
    int start=0;
    while(fgets(line,linesize,file)!=NULL) {
        Record r;
        if(!readrecord(line,r)) continue;
        if(count<maxnumber) {
            records[count]=r;
            count++;
        } else {
            records[start]=r;
            start=(start+1)%maxnumber;
        }
    }
    delete[] line;
    fclose(file);
    // 超过容量时保留最近记录，恢复时间顺序。
    if(start!=0) {
        Record* temp=new Record[count];
        for(int i=0;i<count;i++) temp[i]=records[(start+i)%count];
        for(int i=0;i<count;i++) records[i]=temp[i];
        delete[] temp;
    }
    return count;
}
string csv(const string& s) {
    string result="\"";
    for(size_t i=0;i<s.size();i++) {
        if(s[i]=='\"') result+='\"';
        result+=s[i];
    }
    return result+'\"';
}
bool Store::exportcsv(const wstring& path) const {
    FILE* in=openfile(root_+L"/history.tsv",L"rb");
    FILE* out=openfile(path,L"wb");
    if(out==NULL) {
        if(in!=NULL) fclose(in);
        return false;
    }
    fprintf(out,"\xEF\xBB\xBF日期,模式,位数,关卡,次数,上限,获胜,秒数,答案,过程\r\n");
    const int linesize=32768;
    char* line=new char[linesize];
    while(in!=NULL && fgets(line,linesize,in)!=NULL) {
        Record r;
        if(!readrecord(line,r)) continue;
        fprintf(out,"%s,%s,%d,%d,%d,%d,%d,%d,%s,%s\r\n",
            csv(r.date).c_str(),csv(r.mode).c_str(),r.digit,r.level,r.count,
            r.maxcount,(int)r.won,r.seconds,csv(r.secret).c_str(),csv(r.gethistory).c_str());
    }
    delete[] line;
    bool panduan=!ferror(out) && (in==NULL || !ferror(in));
    if(in!=NULL) fclose(in);
    return fclose(out)==0 && panduan;
}
bool Store::clear() const {
    wstring path=root_+L"/history.tsv";
#ifdef _WIN32
    return DeleteFileW(path.c_str()) || GetLastError()==ERROR_FILE_NOT_FOUND;
#else
    string name(path.begin(),path.end());
    return remove(name.c_str())==0 || errno==ENOENT;
#endif
}
Stats countresult(const Record records[],int recordcount,string mode) {
    Stats result;
    int total=0;
    for(int i=0;i<recordcount;i++) {
        const Record& r=records[i];
        if(!mode.empty() && r.mode!=mode) continue;
        result.rounds++;
        if(r.won) {
            result.wins++;
            total+=r.count;
            if(result.best==0 || r.count<result.best) result.best=r.count;
            if(r.level>result.highest) result.highest=r.level;
        }
    }
    if(result.wins!=0) result.mean=(double)total/result.wins;
    return result;
}
bool Settings::load(const wstring& path) {
    FILE* file=openfile(path,L"rb");
    if(file==NULL) return false;
    int d,a,s,theme;
#ifdef _WIN32
    int count=fscanf_s(file,"%d%d%d%d",&d,&a,&s,&theme);
#else
    int count=fscanf(file,"%d%d%d%d",&d,&a,&s,&theme);
#endif
    fclose(file);
    if(count!=4) return false;
    digit=limitnumber(d,3,6);
    attempts=limitnumber(a,5,20);
    strength=limitnumber(s,1,3);
    dark=theme!=0;
    return true;
}
bool Settings::save(const wstring& path) const {
    FILE* file=openfile(path,L"wb");
    if(file==NULL) return false;
    int result=fprintf(file,"%d %d %d %d\n",digit,attempts,strength,(int)dark);
    return fclose(file)==0 && result>=0;
}
// 控制台游戏流程。
// 正常启动软件时使用图形界面；传入 --console 时运行这个流程。
int consolegame() {
    int proceed=0;
    int digit=4;
    int level=1;
    int count=0;
    int continueproceed=1;
    int randnumber[1000],cinnumber[1000];
    int cinnumbers;
    string input;
    cout<<"你好，我们来玩一个游戏"<<endl;
    cout<<"计算机随机生成一个4位数（每个数字都在1-9之间，并且四个数字互不相同）"<<endl;
    cout<<"数字相同但位置不同记为1B，数字与位置均相同记为1A"<<endl;
    cout<<"猜满10次不对就输了，猜对进入下一关"<<endl;
    cout<<"下一关增加一位数字，最高9位，同时继承上一关没有用完的机会"<<endl;
    cout<<"输入数字1开始：";
    while(cin>>input) {
        if(input=="1") {
            proceed=1;
            break;
        }
        cout<<"游戏等待开始，输入数字1开始：";
    }
    while(proceed) {
        if(level>6) {
            digit=9;
        }
        count=limitnumber(count+10,1,historymax);
        cout<<"\n当前是第"<<level<<"关，本轮的数字是"<<digit<<"位数"<<endl;
        cout<<"你还剩余"<<count<<"次机会"<<endl;
        for(int i=0;i<digit;i++) {
            randnumber[i]=rand()%9+1;
        }
        while(issimilar(randnumber,digit)) {
            // 9位时逐位去重，避免等待过长。
            for(int i=0;i<digit;i++) {
                do {
                    randnumber[i]=rand()%9+1;
                } while(issimilar(randnumber,i+1));
            }
        }
        bool panduan=true;
        while(panduan) {
            cout<<"请输入你选择的数字：";
            if(!(cin>>input)) {
                return 0;
            }
            string error=checknumber(input,digit);
            while(!error.empty()) {
                cout<<error<<"，请重新输入：";
                if(!(cin>>input)) {
                    return 0;
                }
                error=checknumber(input,digit);
            }
            cinnumbers=atoi(input.c_str());
            numbertoarray(cinnumbers,digit,cinnumber);
            if(isA(randnumber,cinnumber,digit)==digit) {
                cout<<"恭喜你，猜对了数字"<<cinnumbers<<endl;
                cout<<"当前你已完成"<<level<<"关卡"<<endl;
                digit=limitnumber(digit+1,4,9);
                level++;
                panduan=false;
            } else {
                cout<<"你的目前输入数字的结论"<<isA(randnumber,cinnumber,digit)
                    <<"A"<<isB(randnumber,cinnumber,digit)<<"B"<<endl;
                count--;
                cout<<"你还剩余"<<count<<"次机会，请再次输入"<<endl;
                if(count==0) {
                    cout<<"对不起，次数已经用完，你输了"<<endl;
                    cout<<"本轮的数字是：";
                    for(int i=0;i<digit;i++) {
                        cout<<randnumber[i];
                    }
                    cout<<"\n当前你已完成"<<level-1<<"关卡"<<endl;
                    continueproceed=1;
                    while(continueproceed) {
                        cout<<"输入0退出游戏；输入1继续游戏：";
                        if(!(cin>>input)) {
                            return 0;
                        }
                        if(input=="1") {
                            proceed=1;
                            digit=4;
                            level=1;
                            panduan=false;
                            continueproceed=0;
                        } else if(input=="0") {
                            proceed=0;
                            panduan=false;
                            continueproceed=0;
                            cout<<"游戏已退出"<<endl;
                        } else {
                            cout<<"你的数字有误，请重新输入"<<endl;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
#ifdef _WIN32
#ifndef NUMBERLOGIC_TEST
int main(int argc,char* argv[]) {
    srand((unsigned int)time(NULL));
    if(argc>1 && string(argv[1])=="--console") {
        if(!AttachConsole(ATTACH_PARENT_PROCESS)) {
            AllocConsole();
        }
        FILE* inputfile=NULL;
        FILE* outputfile=NULL;
        freopen_s(&inputfile,"CONIN$","r",stdin);
        freopen_s(&outputfile,"CONOUT$","w",stdout);
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        cin.clear();
        cout.clear();
        return consolegame();
    }
    return runUI(GetModuleHandleW(NULL),SW_SHOWDEFAULT);
}
#endif
#endif
