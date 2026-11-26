/*开发设计一个游戏，并在此基础上提交设计报告一份。
猜数字游戏：
计算机随机生成一个四位数（每个数字都在1-9之间，并且四个数字互不相同）
然后让玩家猜测是什么。玩家输入的四位数如果不符合规则（比如包含0，或者中间有数字相同）则给出输入不正确提示。
否则，如果猜的数学中有数字相同但位置不同的记为1个B，如果猜的数学中有数字相同且位置也相同的记为1个A，并且把结果提示给玩家。
例如系统生成的随机数是3579，玩家猜2517，则给出提示1A1B。
玩家根据提示继续猜测，系统继续给出提示，直到猜满10次不对显示你输了，或者猜对了显示你赢了。*/

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
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
	int i;
	int j;
	j=digit-1;
	while(i!=digit) {
		cinnumber[i]=number/pow(10,j);
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

int main() {
	srand(time(0)) ;
	int proceed=0;
	int digit=4;
	int level=1;
	int count=0;
	int continueproceed=1;
	int randnumber[1000],cinnumber[1000];
	int cinnumbers;
	cout<<"你好，我们来玩一个游戏"<<endl;
	cout<<"计算机随机生成一个4位数（每个数字都在1-9之间，并且四个数字互不相同）"<<endl;
	cout<<"你需要猜测是什么，并输入它"<<endl;
	cout<<"如果猜的数学中有数字相同但位置不同的记为1个B"<<endl;
	cout<<"如果猜的数学中有数字相同且位置也相同的记为1个A"<<endl;
	cout<<"玩家根据提示继续猜测，系统继续给出提示"<<endl;
	cout<<"直到猜满10次不对,你输了"<<endl;
	cout<<"猜对了显示你赢了,并进入下一关"<<endl;
	cout<<"特别提示：下一关会在上一关基础上增加一位数字，同时上一关没有用完的机会加入下一关"<<endl;
	cout<<"输入数字1开始：";
	cin>>proceed;
	while(proceed!=1) {
		cout<<"游戏等待开始，输入数字1开始：";
		cin>>proceed;
	}
	while(proceed) {
		if(level>6) {
			digit=9;
		}
		count=count+10;
		cout<<"\n当前是第"<<level<<"关，本轮的数字是"<<digit<<"位数"<<endl;
		cout<<"你还剩余"<<count<<"次机会"<<endl;
		for(int i=0; i<digit; i++) {
			randnumber[i]=(rand()%9+1);
		}
		while(issimilar(randnumber,digit)) {
			for(int i=0; i<digit; i++) {
				randnumber[i]=(rand()%9+1);
			}
		}
		bool panduan=true;
		while(panduan) {
			cout<<"请输入你选择的数字：";
			cin>> cinnumbers;
			numbertoarray(cinnumbers,digit,cinnumber);
			while(isdigit(cinnumbers,digit)||iszero(cinnumber,digit)||issimilar(cinnumber,digit)) {
				if(isdigit(cinnumbers,digit))
					cout<<"你的数字位数不正确，请重新输入：";
				else if(iszero(cinnumber,digit))
					cout<<"你的数字位数上含有0，请重新输入：";
				else if(issimilar(cinnumber,digit))
					cout<<"你的数字位数上有相同的，请重新输入：";
				cin>> cinnumbers;
				numbertoarray(cinnumbers,digit,cinnumber);
			}
			if(isA(randnumber,cinnumber,digit)==digit) {
				cout<<"恭喜你，猜对了数字"<<cinnumbers<<endl;
				cout<<"当前你已完成"<<level<<"关卡"<<endl;
				digit++;
				level++;
				panduan=false;
			} else {
				cout<<"你的目前输入数字的结论"<<isA(randnumber,cinnumber,digit)<<"A"<<isB(randnumber,cinnumber,digit)<<"B"<<endl;
				count--;
				cout<<"你还剩余"<<count<<"次机会，请再次输入"<<endl;
				if(count==0) {
					cout<<"对不起，次数已经用完，你输了"<<endl;
					cout<<"本轮的数字是：";
					for(int i=0; i<digit; i++) {
						cout<<randnumber[i];
					}
					cout<<"\n";
					cout<<"当前你已完成"<<level-1<<"关卡"<<endl;
					cout<<"输入0退出游戏;输入1继续游戏："<<endl;
					cin>> proceed;
					while(continueproceed) {
						if (proceed==1) {
							digit=4;
							level=1;
							panduan=false;
							continueproceed=0;
						} else if (proceed==0) {
							cout<<"\n游戏已退出";
							panduan=false;
							continueproceed=0;
						} else {
							cout<<"你的数字有误，请重新输入：";
							cout<<"输入0退出游戏;输入1继续游戏："<<endl;
							cin>> proceed;
						}
					}
				}
			}
		}
	}
	return 0;
}

