//LR(1)语法分析 
using namespace std;
#include<iostream>
#include<string>
#include<cstring>
#include<fstream>
#include<algorithm>
#include<iomanip>
#define max 100000
//ACTION-GOTO表
//每一个文法符号的first集
int ruleCount[200]={0};
struct FIRST_X{//X[0-199]:Vn(Vn[i]) X[200-399]:Vt(Vt[i+200])
	char X[400];
	int set[400][40];//set[i][j]=对应的index -1:$ ,0-199:VtIndex
	int count[400];//记录每一个first集中元素的数量 
};
struct FRIST_Y{
	char Y[20];
	int set[30];
	int count;
}; 
struct FIRST_T{
	int set[40];
	int count;
};
struct FOLLOW_X{//X[0-199]:Vn(下标与Vn数组中相同)
	char X[200];
	int set[200][40];//set[i][j]=对应的终结符的下标 -1:# ,0-199:VtIndex
	int count[200];
};
struct VnAttribute{
	int index[200][20];//在计算时存储是否还在列表中 (index:0 ~ VnNum-1)(index[i][0]:产生式个数)(index[i][1 ~ 个数]:产生式的序号，从1开始)(index[i]一共有个数+1个)
	int is2empty[200];//是否可推导出空 (未定：-1；不可以：1；可以：0)
	int initNum[200];//index[i][0]:产生式的初始个数
}; 
struct initItem{//初始项目总和 0:S'->·Start,# ;1: S'->Start·,# 
	int ruleIndex[500];//产生式序号 0:人工提取出来的产生式 >=1:输入的产生式 
	int potIndex[500];//·的位置 
	int INum;//初始项目数量 
};
struct Item{//项目 
	int initIndex;//哪一个初始项目 
	int set[40];//$-closure(I)的内容 -1:# ;0-VtNum:Vt
	int setCount;//$-closure(I)的数量 
	int attr[7];
	//attr[0]:所属项目集序号
	//attr[1]:在项目集中的序号 
	//attr[2]:项目状态-1:未标记;0:移进项目(·之后是Vt); 1:待约项目(·之后是Vn); 2:归约项目(·在最后); 3:acc项;
	//attr[3]:Vindex 跨过的元素的下标 Vt[200,399],Vn[0,199] 
	//attr[4]:跨元素后下一个项目集序号 
	//attr[5]:跨元素后下一个项目集中的序号 
	//attr[6]:该条项目是否已经向后检查过了 0:初始、没有检查过；1:检查过了 
};
struct ItemSet{//项目集 
	Item item[40];
	int itemNum;//一个项目集中项目数量
	bool isCheck;//是否已经检查过(所有项目的检查标志位都为1)
	bool isFilter;//是否已经过滤过(所有项目都已经知道了状态) 
	char array[40];
	int arrayNum;
};
struct ItemSets{//项目集族 
	ItemSet ISet[200];//项目集列表 
	initItem initI;//初始项目  
	int ISetNum;//项目集数量 
	int changeMap[400][3];//转换函数 I_i--V-->I_j
	int changeNum;//转换函数数量 
};
bool isAllChecked(ItemSet ISet){
	bool ok=true;
	for(int i=0;i<ISet.itemNum;i++){
		if(ISet.item[i].attr[6]==0){
			ok=false;
			break;
		}
	}
	return ok;
}
bool isAllFiltered(ItemSet ISet){
	bool ok=true;
	for(int i=0;i<ISet.itemNum;i++){
		if(ISet.item[i].attr[2]==-1){
			ok=false;
			break;
		}
	}
	return ok;
}
//堆栈
struct wrongMsg{
	int ID;
	char wrongChar;
	int row;
	int col;
};
struct Token{//记录Token 
	string content;
	int type;
};
struct GOTO{
	int array[200][200];
	int StatusNum;
	int VnNum;
};
int getVnIndex(char a,char Vn[200],int VnNum){
	int index = -1;
	for(int i=0;i<VnNum;i++){
		if(Vn[i]==a){
			index = i;
			break;
		}
	}
	return index;
}
bool isRepeat(int set[40],int VtIndex,int count){
	bool flag = false;
	for(int i=0;i<count;i++){
		if(set[i]==VtIndex){
			flag = true;
			break;
		}
	}
	return flag;
}
bool isRepeatinISet(int type,int initIndex,ItemSet ISet){
	for(int i=0;i<ISet.itemNum;i++){
		if(ISet.item[i].initIndex==initIndex){
			return true;
		}
	}
	return false;
}
int getRepeatinISetIndex(int type,int initIndex,ItemSet ISet){
	for(int i=0;i<ISet.itemNum;i++){
		if(ISet.item[i].initIndex==initIndex){
			return i;
		}
	}
	return -1;
}
int getRuleLength(char array[100]){
	int result = 0;
	for(int i=0;i<100;i++){
		if(array[i]!='\0'){
			result++;
		}
		else{
			break;
		}
	}
	return result;
}
bool isVt(char a,char Vt[200],int VtNum){
	bool ok = false;
	for(int i=0;i<VtNum;i++){
		if(a==Vt[i]){
			ok = true;
			break;
		}
	}
	return ok;	
}
VnAttribute isEmpty(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum){
	VnAttribute attr;
	int VnIndex = -1;
	int array[number];
	int id = -1;
	int last2empty[VnNum];
	for(int i=0;i<number;i++){
		array[i]=1;
	}
	for(int i=0;i<VnNum;i++){
		attr.is2empty[i]=-1;
	}
	array[0]=0;
	for(int i=0;i<VnNum;i++){
		memset(attr.index[i],-1,20);
		attr.index[i][0] = 0;
		attr.is2empty[i] = -1;
	}
	for(int i=1;i<number;i++){//先标记所有的非终结符所在产生式的个数和序号 
		VnIndex=getVnIndex(rules[i][0],Vn,VnNum);
		attr.index[VnIndex][0]++;
		id = attr.index[VnIndex][0];
		attr.index[VnIndex][id]=i;
	}
	for(int i=0;i<VnNum;i++){
		attr.initNum[i] = attr.index[i][0];
		if(attr.initNum[i]==0){
			attr.is2empty[i] = -2;	
		}
	}
	//first round 
	for(int i=1;i<number;i++){//扫描每个产生式 
		if(array[i]==0){
			continue;
		}
		for(int j=1;j<getRuleLength(rules[i]);j++){//扫描产生式右部 ，产看是否含有终结符Vt
			if(isVt(rules[i][j],Vt,VtNum)){//如果有 
				VnIndex = getVnIndex(rules[i][0],Vn,VnNum);
				for(int k=1;k<attr.initNum[VnIndex]+1;k++){
					if(attr.index[VnIndex][k]==i){//则删除这行产生式
						attr.index[VnIndex][k]=-1;
						int newCount = 0;
						for(int s=1;s<attr.initNum[VnIndex]+1;s++){
							if(attr.index[VnIndex][s]!=-1){//重新计算剩余产生式数量 
								newCount++;
							}
						}
						attr.index[VnIndex][0]=newCount;
					}
				}
				array[i]=0;
				break;
			}
		}
		if(rules[i][1]=='$'&&getRuleLength(rules[i])==2){//扫描产生式右部 ，是否是空->是，则删除以该产生式的左部为左部的所有产生式，并置该非终结符为可推导出空
			array[i]=0;
			VnIndex = getVnIndex(rules[i][0],Vn,VnNum);
			attr.is2empty[VnIndex]=0;
			for(int k=1;k<attr.initNum[VnIndex]+1;k++){
				if(attr.index[VnIndex][k]!=-1){
					array[attr.index[VnIndex][k]]=0;
					attr.index[VnIndex][k]=-1;
				}
			}
			attr.index[VnIndex][0]=0;
		} 	
	}
	for(int j=0;j<VnNum;j++){ 
		if(attr.is2empty[j]!=-2){//如果该非终结符为左部的产生式均被删除，说明不可以推出空
			if(attr.index[j][0]==0&&attr.is2empty[j]!=0){
				attr.is2empty[j] = 1;
			}
		}
	} 
	
	
	for(int i=0;i<VnNum;i++){
		last2empty[i] = attr.is2empty[i];
	}
	//second round
	bool isSame =true;
	bool isLast = false;
	int count = 0; 
	int Ccount = 0;
	while(1){
		isSame = true;
		Ccount++;
		for(int i=0;i<number;i++){
			if(array[i]==1){//对留下来的产生式的右部进行扫描 
				count = 0;
				for(int j=1;j<getRuleLength(rules[i]);j++){
					VnIndex = getVnIndex(rules[i][j],Vn,VnNum);
					if(attr.is2empty[VnIndex]==1){//该右部非终结符不可推导出 ，删除该行产生式 
						VnIndex = getVnIndex(rules[i][0],Vn,VnNum);
						for(int k=1;k<attr.initNum[VnIndex]+1;k++){
							if(attr.index[VnIndex][k]==i){
								attr.index[VnIndex][k]=-1;
								int newCount = 0;
								for(int s=1;s<attr.initNum[VnIndex]+1;s++){
									if(attr.index[VnIndex][s]!=-1){
										newCount++;
									}
								}
								attr.index[VnIndex][0]=newCount;
							}
						}
						array[i]=0;
						//若导致该非终结符为左部的产生式全部被删除 ,说明不可以推出空 
						attr.is2empty[VnIndex]=1;
					}
					else if(attr.is2empty[VnIndex]==0){//该非终结符可以推导出空 
						count++;
					} 
				}
				if(count==getRuleLength(rules[i])-1){//导致该产生式右部可以推导出空 ，该产生式可以推出空，删除所有以该产生式左部为左部的产生式 
					array[i]=0;
					VnIndex = getVnIndex(rules[i][0],Vn,VnNum);
					attr.index[VnIndex][0]=0;
					attr.is2empty[VnIndex]=0;//改为可推导出空 
					for(int k=1;k<attr.initNum[VnIndex]+1;k++){//删除以其为左部的产生式 
						if(attr.index[VnIndex][k]!=-1){
							array[attr.index[VnIndex][k]]=0;
							attr.index[VnIndex][k]=-1;
						}
					}
				}
			}
		}
		for(int i=0;i<VnNum;i++){
			if(last2empty[i]!=attr.is2empty[i]){
				isSame = false;
			}
		}
		if(isSame&&!isLast){
			isLast = true;
		}
		else if(isSame&&isLast){
			break;
		}
		for(int i=0;i<VnNum;i++){
			last2empty[i]=attr.is2empty[i];
		}
	}
	return attr;
}
FIRST_X getFirstSet(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,VnAttribute attr){
	FIRST_X FIRST;
	for(int i=0;i<VnNum;i++){
		FIRST.X[i] = Vn[i];
		FIRST.set[i][0] = -2;
		FIRST.count[i] = 0;
	}
	for(int i=200;i<VtNum+200;i++){
		FIRST.X[i] = Vt[i-200];
		FIRST.set[i][0] = i;
		FIRST.count[i] = 1;
	}
	int Ccount = 0;
	int rightEmptyNum = 0;
	int lastSet[400][40];
	for(int i=0;i<VnNum;i++){
		for(int j=0;j<VtNum+1;j++){
			lastSet[i][j]=-2;
		}
	} 
	while(1){
		Ccount++;
		for(int i=0;i<VnNum;i++){
			rightEmptyNum=0;
			for(int j=1;j<number;j++){//对每个产生式进行查询 
				if(rules[j][0]==FIRST.X[i]&&getRuleLength(rules[j])==2&&rules[j][1]=='$'&&!isRepeat(FIRST.set[i],-1,FIRST.count[i])){
					FIRST.set[i][FIRST.count[i]]=-1;
					FIRST.count[i]++;
				}
				else if(rules[j][0]==FIRST.X[i]&&getRuleLength(rules[j])==2&&rules[j][1]=='$'&&isRepeat(FIRST.set[i],-1,FIRST.count[i])){
				}
				else if(rules[j][0]==FIRST.X[i]&&getRuleLength(rules[j])>=2&&isVt(rules[j][1],Vt,VtNum)&&!isRepeat(FIRST.set[i],getVnIndex(rules[j][1],Vt,VtNum),FIRST.count[i])){
					FIRST.set[i][FIRST.count[i]]=getVnIndex(rules[j][1],Vt,VtNum);//右部第一个是终结符 
					FIRST.count[i]++;
				}
				else if(rules[j][0]==FIRST.X[i]&&getRuleLength(rules[j])>=2&&isVt(rules[j][1],Vt,VtNum)&&isRepeat(FIRST.set[i],getVnIndex(rules[j][1],Vt,VtNum),FIRST.count[i])){

				}
				else if(rules[j][0]==FIRST.X[i]&&getRuleLength(rules[j])>2&&isVt(rules[j][1],Vn,VnNum)){//右部一个是非终结符 
					int VnNumber = getRuleLength(rules[j])-1;
					int VnIndex = getVnIndex(rules[j][1],Vn,VnNum);
					if(attr.is2empty[VnIndex]==0){//如果该非终结符可以推导出空 
						rightEmptyNum++;
						for(int k=0;k<FIRST.count[VnIndex];k++){
							if(FIRST.set[VnIndex][k]>-1&&!isRepeat(FIRST.set[i],FIRST.set[VnIndex][k],FIRST.count[i])){//不是空且没有重复的加入 
								FIRST.set[i][FIRST.count[i]]=FIRST.set[VnIndex][k];
								FIRST.count[i]++;
							}
							else{
							} 
						} 
					}
					else{//第一个非终结符但是不可推出空,把这个非终结符的first集加入 
						for(int k=0;k<FIRST.count[VnIndex];k++){
							if(FIRST.set[VnIndex][k]>-1&&!isRepeat(FIRST.set[i],FIRST.set[VnIndex][k],FIRST.count[i])){//不是空且没有重复的加入 
								FIRST.set[i][FIRST.count[i]]=FIRST.set[VnIndex][k];
								FIRST.count[i]++;
							}
							else{
							} 
						} 
						continue;
					} 
					for(int s=2;s<getRuleLength(rules[j]);s++){
						if(isVt(rules[j][s],Vt,VtNum)){//右边是终结符 ，可以结束对该行产生式的处理了 
							VnIndex = getVnIndex(rules[j][s],Vt,VtNum);
							if(!isRepeat(FIRST.set[i],VnIndex,FIRST.count[i])){
								FIRST.set[i][FIRST.count[i]]=VnIndex;
								FIRST.count[i]++;
							}
							else{
							}
							break; 
						}
						else{//是非终结符 
							VnIndex = getVnIndex(rules[j][s],Vn,VnNum);
							if(attr.is2empty[VnIndex]==0){//该后继非终结符可以推导出空 
								rightEmptyNum++;
								if(rightEmptyNum==s){
									for(int k=0;k<FIRST.count[VnIndex];k++){
										if(FIRST.set[VnIndex][k]>-1&&!isRepeat(FIRST.set[i],FIRST.set[VnIndex][k],FIRST.count[i])){//不是空且没有重复的加入 
											FIRST.set[i][FIRST.count[i]]=FIRST.set[VnIndex][k];
											FIRST.count[i]++;
										}else{
										} 
									} 
								}
							}
							else{//将下一个不能推出空的非终结符的first集中不重复的元素加入 
								for(int k=0;k<FIRST.count[VnIndex];k++){
									if(FIRST.set[VnIndex][k]>-1&&!isRepeat(FIRST.set[i],FIRST.set[VnIndex][k],FIRST.count[i])){//不是空且没有重复的加入 
										FIRST.set[i][FIRST.count[i]]=FIRST.set[VnIndex][k];
										FIRST.count[i]++;
									}
									else{
									} 
								} 
								break;
							}
						}
						if(rightEmptyNum==getRuleLength(rules[j])-1){
							if(!isRepeat(FIRST.set[i],-1,FIRST.count[i])){
								FIRST.set[i][FIRST.count[i]]=-1;
								FIRST.count[i]++;
							}
							else{
							}
						}
					}
				}
				else if(rules[j][0]==FIRST.X[i]){
				}
			}
		}
		bool flag = true;
		for(int i=0;i<VnNum;i++){
			for(int j=0;j<FIRST.count[i];j++){
				if(lastSet[i][j]!=FIRST.set[i][j]){
					flag = false;
					break;
				}
			}
			if(flag == false){
				break;
			}
		}
		if(flag){
			break;
		}
		else{
			for(int i=0;i<VnNum;i++){
				for(int j=0;j<FIRST.count[i];j++){
					lastSet[i][j]=FIRST.set[i][j];
				}
			}
		}
	}
	return FIRST;
}
FRIST_Y getCovFirst(FIRST_X first,int addArray[20],int addCount,char Vn[200],char Vt[200],int VnNum,int VtNum,int type){
	FRIST_Y FIRST;
	//初始化
	char V;
	int Xcount=0; 
	int Vindex=0;
	FIRST.count = 0;
	for(int i=0;i<addCount;i++){
		FIRST.Y[i] = '\0';
	} 
	for(int i=0;i<30;i++){
		FIRST.set[i]=-2;
	}
	for(int i=0;i<addCount;i++){
		if(addArray[i]!=-1){
			V = first.X[addArray[i]];
			if(isVt(V,Vt,VtNum)){//遇上终结符则结束 
				if(!isRepeat(FIRST.set,getVnIndex(V,Vt,VtNum),FIRST.count)){//是终结符，且没有在first集中重复
					FIRST.set[FIRST.count]=getVnIndex(V,Vt,VtNum);
					FIRST.count++;
				}
				else{//是终结符，但是在first集中重复了
				}
				break;
			}
			if(isVt(V,Vn,VnNum)){//非终结符 需要继续 
				Vindex = getVnIndex(V,Vn,VnNum);//是非终结符
				for(int j=0;j<first.count[Vindex];j++){//查找最初first集合中的第j个first集 
					if(first.set[Vindex][j]>-1&&!isRepeat(FIRST.set,first.set[Vindex][j],FIRST.count)){
						FIRST.set[FIRST.count]=first.set[Vindex][j];
						FIRST.count++;
					}
					else if(first.set[Vindex][j]>-1){
					}
				}
				bool emflag = false;
				for(int k=0;k<first.count[Vindex];k++){
					if(first.set[Vindex][k]==-1){
						Xcount++;
						emflag = true;
						break;
					}
				}
				if(!emflag){//当前非终结符不可 推导出空，则退出结束 
					break;
				}
			}
		}
		else{
			Xcount=addCount;
			break;
		}
	}
	if(Xcount==addCount){
		FIRST.set[FIRST.count]=-1;
		FIRST.count++;
	}
	return FIRST;
}
FOLLOW_X getFollowSet(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,char Start,FIRST_X FIRST){
	FOLLOW_X FOLLOW;
	int addArray[20]={-1};
	int addCount=0;
	int Ccount = 0;
	int lastSet[400][40];
	//初始化
	for(int i=0;i<VnNum;i++){
		FOLLOW.X[i]=Vn[i]; 
		FOLLOW.count[i]=0;
		for(int j=0;j<VtNum+1;j++){
			FOLLOW.set[i][j]=-2;
		}
	}
	for(int i=0;i<VnNum;i++){
		if(Vn[i]==Start){
			FOLLOW.count[i]++;
			FOLLOW.set[i][0]=-1;
		}
	}
	for(int i=0;i<VnNum;i++){
		for(int j=0;j<VtNum+1;j++){
			lastSet[i][j]=-2;
		}
	} 
	while(1){
		Ccount++;
		int VnIndex = -1;
		int leftIndex= -1;
		bool is2Empty=false;
		for(int i=1;i<number;i++){//在每个产生式中寻找 
			for(int j=1;j<getRuleLength(rules[i]);j++){
				addCount=0;
				//右部含有非终结符的
				if(isVt(rules[i][j],Vn,VnNum)){
					VnIndex = getVnIndex(rules[i][j],Vn,VnNum);
					if(j+1>=getRuleLength(rules[i])&&addCount==0&&is2Empty){//之后没有跟着的文法符号 
						is2Empty = true;
					}
					else{
						addCount=0;
						is2Empty=false;
						//列出first集
						for(int k=j+1;k<getRuleLength(rules[i]);k++){
							if(isVt(rules[i][k],Vn,VnNum)){//是非终结符 
								addArray[addCount]=getVnIndex(rules[i][k],Vn,VnNum);
								addCount++;
							}
							else if(isVt(rules[i][k],Vt,VtNum)){//是终结符 
								addArray[addCount]=getVnIndex(rules[i][k],Vt,VtNum)+200;
								addCount++;
							}
						}
						FRIST_Y first_y = getCovFirst(FIRST,addArray,addCount,Vn,Vt,VnNum,VtNum,-1);
						is2Empty=false;
						for(int k=0;k<first_y.count;k++){
							if(first_y.set[k]==-1){
								is2Empty=true;
								break;
							}
						}
						for(int k=0;k<first_y.count;k++){
							if(first_y.set[k]>-1&&!isRepeat(FOLLOW.set[VnIndex],first_y.set[k],FOLLOW.count[VnIndex])){
								FOLLOW.set[VnIndex][FOLLOW.count[VnIndex]]=first_y.set[k];
								FOLLOW.count[VnIndex]++;
							}
							else if(first_y.set[k]>-1){
							} 
							else if(first_y.set[k]==-1){//含有空的first集 
							}
						}
					}
					
					if(is2Empty){//后面的文法符号->可推导出空或者是后面没有文法符号 ，还需要额外加上FOLLOW(左部) 
						leftIndex = getVnIndex(rules[i][0],Vn,VnNum);
						for(int k=0;k<FOLLOW.count[leftIndex];k++){
							if(!isRepeat(FOLLOW.set[VnIndex],FOLLOW.set[leftIndex][k],FOLLOW.count[VnIndex])){
								FOLLOW.set[VnIndex][FOLLOW.count[VnIndex]]=FOLLOW.set[leftIndex][k];
								FOLLOW.count[VnIndex]++;
							}
							else{
							}
						}				
					}
					else{//后面的文法符号->不可推导出空 ，把当前复合的first集加入即可 
					}
				} 
				else{
				} 
			}
		}
		bool reflag = true;
		for(int i=0;i<VnNum;i++){
			for(int j=0;j<FOLLOW.count[i];j++){
				if(lastSet[i][j]!=FOLLOW.set[i][j]){
					reflag = false;
					break;
				}
			}
			if(reflag == false){
				break;
			}
		}
		if(reflag){
			break;
		}
		else{
			for(int i=0;i<VnNum;i++){
				for(int j=0;j<FOLLOW.count[i];j++){
					lastSet[i][j]=FOLLOW.set[i][j];
				}
			}
		}
		if(Ccount>=5){
			cout<<"被迫结束"<<endl;
			break;
		}
	}
	return FOLLOW;
}
initItem getInitTtems(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,char Start){
	initItem init_I;
	//初始化
	init_I.ruleIndex[0]=0;
	init_I.potIndex[0]=0;
	//I0:S'->·Start
	init_I.ruleIndex[1]=0;
	init_I.potIndex[1]=1;
	//I1:S'->Start· 
	init_I.INum=2;
	for(int i=1;i<number;i++){//注意$的初始化 
		for(int j=0;j<getRuleLength(rules[i]);j++){
			if(getRuleLength(rules[i])==2&&rules[i][1]=='$'){
				init_I.ruleIndex[init_I.INum]=i;
				init_I.potIndex[init_I.INum]=0;
				init_I.INum++;
				break;
			}
			else{
				init_I.ruleIndex[init_I.INum]=i;
				init_I.potIndex[init_I.INum]=j;
				init_I.INum++;
			}
		}
	}
	//输出初始化检查 
	cout<<"一共产生了"<<init_I.INum<<"条产生式"<<endl;
	return init_I;
}
Item setSets(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,char Start,FIRST_X FIRST,ItemSet ISet,Item I_j,initItem INIT_I,int leftIndex,int ruleIndex,int potIndex){
	Item tempNode;
	int addArray[20]={-1};
	int addCount=0;
	int betaNum=0;
	for(int i=potIndex+2;i<getRuleLength(rules[ruleIndex]);i++){
		if(isVt(rules[ruleIndex][i],Vt,VtNum)){
			addArray[addCount]=getVnIndex(rules[ruleIndex][i],Vt,VtNum)+200;
			addCount++;
		}
		else if(isVt(rules[ruleIndex][i],Vn,VnNum)){
			addArray[addCount]=getVnIndex(rules[ruleIndex][i],Vn,VnNum);
			addCount++;
		}
	}
	betaNum=addCount;
	FIRST_T first_temp;
	first_temp.count=0; 
	for(int i=0;i<I_j.setCount;i++){
		//每个单独计算closure中的 a
		addCount=betaNum;
		addArray[addCount]=I_j.set[i];
		if(I_j.set[i]!=-1){
			addArray[addCount]+=200;
		}
		addCount++;
		FRIST_Y first_y = getCovFirst(FIRST,addArray,addCount,Vn,Vt,VnNum,VtNum,betaNum);
		for(int s=0;s<first_y.count;s++){
			if(!isRepeat(first_temp.set,first_y.set[s],first_temp.count)){
				first_temp.set[first_temp.count]=first_y.set[s];
				first_temp.count++;
			}
		}
	}
	
	for(int j=0;j<first_temp.count;j++){
		tempNode.set[j]=first_temp.set[j];
	}
	tempNode.setCount=first_temp.count;
	return tempNode;
}
ItemSet getNewItemSet(char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,char Start,FIRST_X FIRST,ItemSet ISet,Item I_j,initItem INIT_I,int leftIndex,int ruleIndex,int potIndex){
	Item tempNode ;
	int repeatIIndex;
	bool isNewinSet=false;
	//寻找Vn[leftIndex]开头的、potIndex=0的产生式加入
	for(int i=1;i<INIT_I.INum;i++){
		if(rules[INIT_I.ruleIndex[i]][0]==Vn[leftIndex]&&INIT_I.potIndex[i]==0){
			tempNode= setSets(rules,number,Vn,Vt,VnNum,VtNum,Start,FIRST,ISet,I_j,INIT_I,leftIndex,ruleIndex,potIndex);
			if(isRepeatinISet(1,i,ISet)){
				repeatIIndex=getRepeatinISetIndex(1,i,ISet);
				for(int j=0;j<tempNode.setCount;j++){
					bool flag=true;
					for(int k=0;k<ISet.item[repeatIIndex].setCount;k++){
						if(ISet.item[repeatIIndex].set[k]==tempNode.set[j]){
							flag=false;
						}
					}
					if(flag==false){
						continue;
					}
					else{
						isNewinSet=true;
						break;
					}
				}
				if(isNewinSet){
					for(int j=0;j<tempNode.setCount;j++){
						bool flag=true;
						for(int k=0;k<ISet.item[repeatIIndex].setCount;k++){
							if(ISet.item[repeatIIndex].set[k]==tempNode.set[j]){
								flag=false;
							}
						}
						if(flag==false){
							continue;
						}
						else{
							ISet.item[repeatIIndex].set[ISet.item[repeatIIndex].setCount]=tempNode.set[j];
							ISet.item[repeatIIndex].setCount++;
						}
					}
				}
			}
			else{
				ISet.item[ISet.itemNum].initIndex=i;
				ISet.item[ISet.itemNum].attr[0]=I_j.attr[0];
				ISet.item[ISet.itemNum].attr[1]=ISet.itemNum;
				ISet.item[ISet.itemNum].attr[2]=-1;
				ISet.item[ISet.itemNum].attr[3]=-1;
				ISet.item[ISet.itemNum].attr[4]=-1;
				ISet.item[ISet.itemNum].attr[5]=-1;
				ISet.item[ISet.itemNum].attr[6]=0;
				//复制first集到closure中			 
				
				for(int j=0;j<tempNode.setCount;j++){
					ISet.item[ISet.itemNum].set[j]=tempNode.set[j];
				}
				ISet.item[ISet.itemNum].setCount=tempNode.setCount;
				ISet.itemNum++;
			}
		}
	} 
	return ISet;
}
bool isRepeatinISet(ItemSet ISet,Item I_j,int ruleIndex,int potIndex,initItem INIT_I){
	bool ok = false;
	for(int i=0;i<ISet.itemNum;i++){
		if(INIT_I.ruleIndex[ISet.item[i].initIndex]==ruleIndex&&INIT_I.potIndex[ISet.item[i].initIndex]==potIndex+1&&I_j.setCount==ISet.item[i].setCount){
			bool isrepeatinISet=true;
			for(int k=0;k<ISet.item[i].setCount;k++){
				if(ISet.item[i].set[k]!=I_j.set[k]){
					isrepeatinISet=false;
					break;
				}
			}
			if(isrepeatinISet==true){
				ok = true;
				break;
			}
		}
	}
	return ok;
}
int getInitIndex(initItem INIT_I,int ruleIndex,int potIndex){
	int InitIndex=-1;
	for(int i=0;i<INIT_I.INum;i++){
		if(INIT_I.potIndex[i]==potIndex&&INIT_I.ruleIndex[i]==ruleIndex){
			InitIndex=i;
			break;
		}
	}
	return InitIndex;
}

void printItem(initItem INIT_I,char rules[200][100],Item item,char Vt[200],char Start='S'){
	int index = item.initIndex;
	int ruleIndex = INIT_I.ruleIndex[index];
	int potIndex= INIT_I.potIndex[index];
	for(int j=0;j<ruleCount[ruleIndex];j++){
		if(ruleIndex==0&&j==0){
			cout<<"S'";
		}
		else if(ruleIndex==0&&j!=0){
			cout<<Start;
		}
		else{
			cout<<rules[ruleIndex][j];
		}
		if(j==0){
			cout<<"->";
		}
		if(potIndex==j){
			cout<<"·";
		}
	}
	cout<<",";
	int i=0;
	for(i=0;i<item.setCount-1;i++){
		if(item.set[i]==-1){
			cout<<"#,";
		}
		else{
			cout<<Vt[item.set[i]]<<",";
		}
	}
	if(item.set[i]==-1){
			cout<<"#";
		}
		else{
			cout<<Vt[item.set[i]];
		}
	cout<<"\n";
}
void printRule(char rules[200][100],int ruleIndex,int potIndex){
	for(int j=0;j<ruleCount[ruleIndex];j++){
		if(potIndex==j&&j!=0){
			cout<<"·";
		}
		cout<<rules[ruleIndex][j];
		if(j==0){
			cout<<"->";
		}
		if(potIndex==j&&j==0){
			cout<<"·";
		}
	}
	cout<<"\n";
}
ItemSets getItemSets(initItem INIT_I,char rules[200][100],int number,char Vn[200],char Vt[200],int VnNum,int VtNum,char Start,FIRST_X FIRST,int ruleCount[200]){
	ItemSets ISets;
	int Ccount=0;
	int FilterNum=0;
	//初始化项目集
	ISets.initI=INIT_I;
	ISets.ISet[0].isCheck=false;
	ISets.ISet[0].isFilter=false;
	//I_0[0]
	ISets.ISet[0].item[0].initIndex=0;
	ISets.ISet[0].item[0].set[0]=-1;//set[0]='#'
	ISets.ISet[0].item[0].setCount=1;
	ISets.ISet[0].item[0].attr[0]=0;//I_0
	ISets.ISet[0].item[0].attr[1]=0;//I_0[0]
	ISets.ISet[0].item[0].attr[2]=1;//状态未知 
	ISets.ISet[0].item[0].attr[3]=getVnIndex(Start,Vn,VnNum);//Vindex:未知 
	ISets.ISet[0].item[0].attr[4]=1;//I_j 未知或不需要 
	ISets.ISet[0].item[0].attr[5]=0;//I_j[k] 未知或不需要
	ISets.ISet[0].item[0].attr[6]=0;//尚未向后检查
	ISets.ISet[0].arrayNum=1;
	ISets.ISet[0].array[0]=Start;
	ISets.ISet[0].itemNum=1;
	//I_1[0]
	ISets.ISet[1].item[0].initIndex=1;
	ISets.ISet[1].item[0].set[0]=-1;//set[0]='#'
	ISets.ISet[1].item[0].setCount=1;
	ISets.ISet[1].item[0].attr[0]=1;//I_1
	ISets.ISet[1].item[0].attr[1]=0;//I_1[0]
	ISets.ISet[1].item[0].attr[2]=3;//状态：acc 
	ISets.ISet[1].item[0].attr[3]=-2;//Vindex:未知或不需要(attr[2]==2||attr[2]==3) 
	ISets.ISet[1].item[0].attr[4]=-1;//I_j 未知或不需要
	ISets.ISet[1].item[0].attr[5]=-1;//I_j[k] 未知或不需要
	ISets.ISet[1].item[0].attr[6]=1;//已经向后检查
	ISets.ISet[1].itemNum=1;
	ISets.ISet[1].arrayNum=1;
	ISets.ISet[1].array[0]='$'; 
	ISets.ISetNum=2;
	ISets.changeMap[0][0]=0;
	ISets.changeMap[0][1]=getVnIndex(Start,Vn,VnNum);
	ISets.changeMap[0][2]=1;
	ISets.changeNum=1;
	do{
		//先向后检查,将没有检查的项目集检查完 
		for(int i=0;i<ISets.ISetNum;i++){
			if(!isAllChecked(ISets.ISet[i])){
				//检查该项目
				for(int j=0;j<ISets.ISet[i].itemNum;j++){
					if(ISets.ISet[i].item[j].attr[6]==0){
						int initIndex = ISets.ISet[i].item[j].initIndex;
						int ruleIndex = ISets.initI.ruleIndex[initIndex];
						int potIndex = ISets.initI.potIndex[initIndex];
						int leftIndex;
						if(ruleIndex!=0&&rules[ruleIndex][potIndex+1]=='$'){
							ISets.ISet[i].item[j].attr[6]=1;
							ISets.ISet[i].item[j].attr[2]=2;//顺便处理状态 
						}
						else if(ruleIndex!=0&&isVt(rules[ruleIndex][potIndex+1],Vt,VtNum)){
							ISets.ISet[i].item[j].attr[6]=1;
						}
						else if(ruleIndex!=0&&isVt(rules[ruleIndex][potIndex+1],Vn,VnNum)||ruleIndex==0){
							if(ruleIndex==0)
								leftIndex = getVnIndex(Start,Vn,VnNum);
							else
								leftIndex = getVnIndex(rules[ruleIndex][potIndex+1],Vn,VnNum);
							//ADD-1
							ISets.ISet[i]=getNewItemSet(rules,number,Vn,Vt,VnNum,VtNum,Start,FIRST,ISets.ISet[i],ISets.ISet[i].item[j],ISets.initI,leftIndex,ruleIndex,potIndex);
							ISets.ISet[i].item[j].attr[6]=1;
						}
						else if(ruleIndex!=0&&potIndex+1>=getRuleLength(rules[ruleIndex])){
							ISets.ISet[i].item[j].attr[2]=2;
							ISets.ISet[i].item[j].attr[6]=1;
						}
						else{
						}
					}
				}
			}
			else{
				ISets.ISet[i].isCheck=true;
			} 
		}
		bool toFiltering = true;
		for(int i=0;i<ISets.ISetNum;i++){
			if(!isAllChecked(ISets.ISet[i])){
				toFiltering=false;
			}
		}
		
		if(toFiltering){
			int lastISetsCount=ISets.ISetNum;
			bool isNewISet=false;
			for(int i=0;i<lastISetsCount;i++){
				if(!isAllFiltered(ISets.ISet[i])){
					for(int j=0;j<ISets.ISet[i].itemNum;j++){
						int initIndex = ISets.ISet[i].item[j].initIndex;
						int ruleIndex = ISets.initI.ruleIndex[initIndex];
						int potIndex = ISets.initI.potIndex[initIndex];
						int Vindex = -2;
						int repeatSet = -1;
						if(ISets.ISet[i].item[j].attr[2]==-1){//过滤目标 
							if(rules[ruleIndex][potIndex+1]=='$'){//状态：归约项目 
								ISets.ISet[i].item[j].attr[2]=2;
								ISets.ISet[i].array[ISets.ISet[i].arrayNum]++; 
								ISets.ISet[i].arrayNum++;
								ISets.ISet[i].item[j].attr[4] = -1;
								ISets.ISet[i].item[j].attr[5] = -1;
							}
							else{
								if(isVt(rules[ruleIndex][potIndex+1],Vt,VtNum)){//状态：移进项目
									ISets.ISet[i].item[j].attr[2]=0; 
									Vindex = getVnIndex(rules[ruleIndex][potIndex+1],Vt,VtNum)+200;
								}
								else if(isVt(rules[ruleIndex][potIndex+1],Vn,VnNum)){//状态：待约项目 
									ISets.ISet[i].item[j].attr[2]=1;
									Vindex = getVnIndex(rules[ruleIndex][potIndex+1],Vn,VnNum);
								}
								else{
									continue;
								} 
								ISets.ISet[i].item[j].attr[3]=Vindex;//Vindex
								int nextISetIndex=-1;
								int nextIIndex=-1; 
								//查找看有没有已经重复了的
								bool ok1=false,ok2=false,ok3=false,ok4=false;
								//ok1:同一个集合中，属于同一个产生式，同样的位置
								//ok2:转化函数是否已存在 
								//ok3:
								//ok4:新产生的项目是否已经在其他集合中
								ok1=isRepeatinISet(ISets.ISet[i],ISets.ISet[i].item[j],ruleIndex,potIndex,INIT_I);
								int k1,k2,k3;
								int repeatIsetIndex=-1;
								int repeatIIndex=-1;
								for(k1=0;k1<ISets.ISetNum;k1++){
									for(k2=0;k2<ISets.ISet[k1].itemNum;k2++){
										//新的initIndex(i,j之后)==遍历的某个结果
										if(getInitIndex(INIT_I,ruleIndex,potIndex+1)==ISets.ISet[k1].item[k2].initIndex){
											if(ISets.ISet[i].item[j].setCount==ISets.ISet[k1].item[k2].setCount){
												ok4=true;
												for(k3=0;k3<ISets.ISet[k1].item[k2].setCount;k3++){
													if(ISets.ISet[k1].item[k2].set[k3]!=ISets.ISet[i].item[j].set[k3]){
														ok4=false;
														break;
													}
												}
												if(ok4==true){
													repeatIsetIndex=k1;
													repeatIIndex=k2;
													break;
												}
											}
										}
									}
									if(ok4==true){
										break;
									}
								} 
								for(int k1=0;k1<ISets.changeNum;k1++){
									if(Vindex==ISets.changeMap[k1][1]&&ISets.changeMap[k1][0]==i){//fromID相等且Vindex与转换相等 
										ok2=true;
										repeatSet=ISets.changeMap[k1][2];
										break;
									}
								}
								if(!ok1&&!ok4&&!ok2){
									int initIndex=-1;
									nextISetIndex=ISets.ISetNum;
									nextIIndex=0;
									initIndex=getInitIndex(INIT_I,ruleIndex,potIndex+1);
									ISets.ISet[nextISetIndex].item[0].initIndex=initIndex;
									ISets.ISet[nextISetIndex].item[0].setCount=0;
									ISets.ISet[nextISetIndex].item[0].attr[0]=nextISetIndex;
									ISets.ISet[nextISetIndex].item[0].attr[1]=0;
									ISets.ISet[nextISetIndex].item[0].attr[2]=-1;
									ISets.ISet[nextISetIndex].item[0].attr[6]=0;
									ISets.ISet[nextISetIndex].itemNum=1;
									ISets.ISet[nextISetIndex].isCheck=false;
									ISets.ISet[nextISetIndex].isFilter=false;
									ISets.ISet[nextISetIndex].arrayNum=0;
									for(int k1=0;k1<ISets.ISet[i].item[j].setCount;k1++){
										ISets.ISet[nextISetIndex].item[0].set[k1]=ISets.ISet[i].item[j].set[k1];
										ISets.ISet[nextISetIndex].item[0].setCount++;
									}
									ISets.ISetNum++;
								}
								else if(!ok1&&!ok4&&ok2){
									int initIndex=-1;
									nextISetIndex=repeatSet;
									nextIIndex=ISets.ISet[nextISetIndex].itemNum;
									initIndex=getInitIndex(INIT_I,ruleIndex,potIndex+1);
									ISets.ISet[nextISetIndex].item[nextIIndex].initIndex=initIndex;
									ISets.ISet[nextISetIndex].item[nextIIndex].setCount=0;
									ISets.ISet[nextISetIndex].item[nextIIndex].attr[0]=nextISetIndex;
									ISets.ISet[nextISetIndex].item[nextIIndex].attr[1]=0;
									ISets.ISet[nextISetIndex].item[nextIIndex].attr[2]=-1;
									ISets.ISet[nextISetIndex].item[nextIIndex].attr[6]=0;
									ISets.ISet[nextISetIndex].itemNum++;
									ISets.ISet[nextISetIndex].isCheck=false;
									ISets.ISet[nextISetIndex].isFilter=false;
									ISets.ISet[nextISetIndex].arrayNum=0;
									for(int k1=0;k1<ISets.ISet[i].item[j].setCount;k1++){
										ISets.ISet[nextISetIndex].item[nextIIndex].set[k1]=ISets.ISet[i].item[j].set[k1];
										ISets.ISet[nextISetIndex].item[nextIIndex].setCount++;
									}
								}
								else if(ok1&&!ok2){
									nextISetIndex=i;
									for(int k1=0;k1<ISets.ISet[i].itemNum;k1++){
										if(ISets.initI.ruleIndex[ISets.ISet[i].item[k1].initIndex]==ruleIndex&&ISets.initI.potIndex[ISets.ISet[i].item[k1].initIndex]==potIndex+1){
											nextIIndex=k1;
											break;
										}
									}
								}
								else if(ok4&&!ok2){
									nextISetIndex=repeatIsetIndex;
									nextIIndex=repeatIIndex;
								}
								else if(ok2){
									nextISetIndex=repeatSet;
									ok3=true;
									nextIIndex=-1;
									for(k2=0;k2<ISets.ISet[nextISetIndex].itemNum;k2++){
										//新的initIndex(i,j之后)==遍历的某个结果
										if(getInitIndex(INIT_I,ruleIndex,potIndex+1)==ISets.ISet[nextISetIndex].item[k2].initIndex){
											if(ISets.ISet[i].item[j].setCount==ISets.ISet[nextISetIndex].item[k2].setCount){
												ok3=true;
												for(k3=0;k3<ISets.ISet[nextISetIndex].item[k2].setCount;k3++){
													if(ISets.ISet[nextISetIndex].item[k2].set[k3]!=ISets.ISet[i].item[j].set[k3]){
														ok3=false;
														break;
													}
												}
												if(ok3==true){
													repeatIIndex=k2;
													break;
												}
											}
										}
									}
									if(ok3){
										nextIIndex=ISets.ISet[nextISetIndex].itemNum;
										int initIndex=-1;
										initIndex=getInitIndex(INIT_I,ruleIndex,potIndex+1);
										ok1=isRepeatinISet(ISets.ISet[nextISetIndex],ISets.ISet[i].item[j],ruleIndex,potIndex,INIT_I);
										if(ok1){
										}
										else{
											ISets.ISet[nextISetIndex].item[nextIIndex].initIndex=initIndex;
											ISets.ISet[nextISetIndex].item[nextIIndex].setCount=0;
											ISets.ISet[nextISetIndex].item[nextIIndex].attr[0]=nextISetIndex;
											ISets.ISet[nextISetIndex].item[nextIIndex].attr[1]=0;
											ISets.ISet[nextISetIndex].item[nextIIndex].attr[2]=-1;
											ISets.ISet[nextISetIndex].item[nextIIndex].attr[6]=0;
											ISets.ISet[nextISetIndex].itemNum++;
											ISets.ISet[nextISetIndex].isCheck=false;
											ISets.ISet[nextISetIndex].isFilter=false;
											ISets.ISet[nextISetIndex].arrayNum=0;
											for(int k1=0;k1<ISets.ISet[i].item[j].setCount;k1++){
												ISets.ISet[nextISetIndex].item[nextIIndex].set[k1]=ISets.ISet[i].item[j].set[k1];
												ISets.ISet[nextISetIndex].item[nextIIndex].setCount++;
											}
										}
									}
									else{
										nextIIndex=repeatIIndex;
									}
								} 
								ISets.ISet[i].item[j].attr[4]=repeatSet;//to I_j 
								ISets.ISet[i].item[j].attr[5]=ISets.ISet[nextISetIndex].itemNum-1;//to I_j[k] 
								ISets.ISet[i].item[j].attr[6]=1;//已经检查了 
								if(ok2){
									
								} 
								else{
									ISets.ISet[i].item[j].attr[4]=nextISetIndex;//to I_j 
									ISets.ISet[i].item[j].attr[5]=nextIIndex;//to I_j[k] 
									ISets.ISet[i].item[j].attr[6]=1;//已经检查了 
									ISets.changeMap[ISets.changeNum][0]=i;
									ISets.changeMap[ISets.changeNum][1]=Vindex;
									ISets.changeMap[ISets.changeNum][2]=nextISetIndex;
									ISets.changeNum++;
								}
							}
						}
						else{
						}
					}
				}
				else{
					ISets.ISet[i].isFilter=true;
				}
			}
		}
		else{
		}
		
		FilterNum=0;
		for(int i=0;i<ISets.ISetNum;i++){
			if(isAllFiltered(ISets.ISet[i])){
				FilterNum++;
			}
		}
		Ccount++;
		/*if(Ccount>=6){
			break;
		}
		*/
	}while(FilterNum!=ISets.ISetNum);
	return ISets;
}
string to_String(int n){
    int m = n;
    char s[max];
    char ss[max];
    int i=0,j=0;
    if(n==0){
    	return "0";
	}
    if (n < 0)// 处理负数
    {
        m = 0 - m;
        j = 1;
        ss[0] = '-';
    }    
    while (m>0)
    {
        s[i++] = m % 10 + '0';
        m /= 10;
    }
    s[i] = '\0';
    i = i - 1;
    while (i >= 0)
    {
        ss[j++] = s[i--];
    }    
    ss[j] = '\0';    
    return ss;
}

void printChange(int changeMap[400][3],int changeNum,char Vn[200],char Vt[200],int VnNum,int VtNum){
	char a;
	for(int i=0;i<changeNum;i++){
		if(changeMap[i][1]>=200){
			a=Vt[changeMap[i][1]-200];
		}
		else{
			a=Vn[changeMap[i][1]];
		}
		cout<<"f("<<changeMap[i][0]<<","<<a<<")="<<changeMap[i][2]<<endl;
	}
	
}
wrongMsg test(string line,int action[50][50],int gotoT[50][50],char Vn[200],char Vt[200],int VnNum,int VtNum,char rules[200][100],int maxStatus){
	wrongMsg result;
	result.ID=-1;
	int solvedNum=0;
	int lineLength;
	string outline;
	int statusStack[50];
	char signStack[50];
	char getinStack[50];
	int getinNum=0;
	int signNum=0;
	int statusNum=0;
	int StackCount=0;
	int actionStack=0;
	int gotoStack=-1;
	int now=0;
	char nowChar;
	for(int i=0;i<line.length();i++){
		if(line[i]=='('){
			getinStack[i]='j';
		}
		else if(line[i]==')'){
			getinStack[i]='k';
		}
		else{
			getinStack[i]=line[i];
		}
		getinNum++;
	}
	lineLength=getinNum;
	statusStack[0]=0;
	statusNum++;
	getinStack[getinNum]='#';
	getinNum++;
	signStack[0]='#';
	signNum++;
	int VtIndex;
	int VnIndex;
	int row,col;
	int lastCount=0;
	char lastleft;
	bool isExit=false;
	bool mustBreak=false;
	cout<<setw(5)<<"步骤"<<setw(12)<<"状态栈"<<setw(12)<<"符号栈"<<setw(12)<<"输入串"<<setw(12)<<"ACTION"<<setw(12)<<"GOTO"<<endl;
	while(1){
		StackCount++;
		cout<<setw(5)<<StackCount;
		nowChar=getinStack[now];
		if(actionStack>0){
			statusStack[statusNum]=actionStack;
			statusNum++;
			signStack[signNum]=nowChar;
			signNum++;
			for(int i=0;i<getinNum-1;i++){
				getinStack[i]=getinStack[i+1];
			}
			getinNum--;
			solvedNum=lineLength-getinNum;
			now=0;
			nowChar=getinStack[now];
		}
		else if(actionStack<0&&gotoStack>0){
			statusStack[statusNum-lastCount]=gotoStack;
			statusNum-=lastCount-1;
			signStack[signNum-lastCount]=lastleft;
			signNum-=lastCount-1;
			now=0;
			nowChar=getinStack[now];
		}
		if(isVt(nowChar,Vt,VtNum)){
			VtIndex=getVnIndex(nowChar,Vt,VtNum);
			col=VtIndex;
			row=statusStack[statusNum-1];
			cout<<"当前状态"<<row<<",当前符号"<<nowChar<<endl;
			actionStack=action[row][col];
		}
		else if(nowChar=='#'){
			row=statusStack[statusNum-1];
			col=VtNum;
			cout<<"当前状态"<<row<<",当前符号"<<nowChar<<endl;
			actionStack=action[row][col];
		}
		if(actionStack>maxStatus){
			result.ID=1;
			result.wrongChar=signStack[signNum-1];
			result.row=0;
			result.col=solvedNum;
			mustBreak=true;
		}
		if(actionStack<0){
			lastCount=ruleCount[-actionStack]-1;
			lastleft=rules[-actionStack][0];
			cout<<"产生式"<<-actionStack<<"一共有"<<lastCount<<"个右部，左部是"<<lastleft<<endl;
			if(isVt(lastleft,Vn,VnNum)){
				col = getVnIndex(lastleft,Vn,VnNum);
				row = statusStack[statusNum-lastCount-1];
				cout<<"前一个状态是"<<row<<endl;
				gotoStack=gotoT[row][col];
			}
			else{
				cout<<"左部不是Vn"<<endl;
			}
		}
		else if(actionStack==0){
			isExit=true;
			result.ID=0;
		}
		else{
			gotoStack=-1;
		}
		outline="\0";
		for(int i=0;i<statusNum;i++){
			outline+=to_String(statusStack[i]);
		}
		cout<<setw(12)<<outline;
		outline="\0";
		for(int i=0;i<signNum;i++){
			outline+=signStack[i];
		}
		cout<<setw(12)<<outline;
		outline="\0";
		for(int i=0;i<getinNum;i++){
			outline+=getinStack[i];
		}
		cout<<setw(12)<<outline;
		if(isExit){
			cout<<setw(12)<<"acc";
		}
		else{
			cout<<setw(12)<<actionStack;
		}
		if(gotoStack==-1){
			cout<<setw(12)<<" "<<endl;
		}
		else{
			cout<<setw(12)<<gotoStack<<endl;
		}
		cout<<"已处理"<<solvedNum<<"个"<<endl;
		if(StackCount>=300||isExit||mustBreak){
			break;
		}
	}
	return result;
}
int main(){
	fstream tokenFile;
	fstream ruleFile;
	fstream results;
	string tokenLine[300],ruleLine[200];
	int n=0,m=0;//n:tokens'number;m:rules'number
	Token tokenArray[200];
	char rules[200][100];
	char Vn[200],Vt[200],Start,Rule;
	int VnNum=0,VtNum=0;
	int action[50][50]={200};
	int gotoT[50][50]={0};
	bool _isVn=false,_isVt=false,_isStart=false,_isRule=false;
	//读取token表 
	tokenFile.open("TokenResult.txt",ios::in|ios::out);
	if(!tokenFile){
		cout<<"Fail to open the file 'TokenResult.txt' !"<<endl;
		exit(1);
	} 
	else{
		while(!tokenFile.eof()&&getline(tokenFile,tokenLine[n])){
			n++;
		}
		for(int i=0;i<n;i++){
			bool type = false;
			string token = "";
			int number = 0;
			for(int j=0;j<tokenLine[i].length();j++){
				if(tokenLine[i][j]==':'){
					type = true;
					tokenArray[i].content=token;
					continue;
				}
				if(type == false&&tokenLine[i][j]!=' '){
					token+=tokenLine[i][j];
				}
				else if(type&&tokenLine[i][j]!=' '){
					number = number*10+tokenLine[i][j]-'0';
				}
			}
			tokenArray[i].type = number;
			type = false;
		}
		/*
		//输出token 
		for(int i=0;i<n;i++){
			cout<<tokenArray[i].content<<":"<<tokenArray[i].type<<endl;
		}
		*/
		tokenFile.close();
	}
	//读取新的规则（产生式） 
	ruleFile.open("rule2.txt",ios::in);
	if(!ruleFile){
		cout<<"Fail to open the file 'rule2.txt' !"<<endl;
		exit(1);
	}
	else{
		while(!ruleFile.eof()&&getline(ruleFile,ruleLine[m])){
			m++;
		}
		ruleCount[0]=2;
		int count=1;
		for(int i=1;i<m;i++){
			memset(rules[i],'\0',100);
			rules[i][0] = ruleLine[i][0];
			count=1;
			for(int j=3;j<ruleLine[i].length();j++){
				if(ruleLine[i][j]!=' '&&ruleLine[i][j]!=','){
					rules[i][count] = ruleLine[i][j];
					count++;
				}
			}
			ruleCount[i]=count;
		} 
		ruleFile.close();
	}
	
	for(int i=0;i<ruleLine[0].length();i++){//处理产生式
		if(ruleLine[0][i]=='{'){
			if(!_isVn&&!_isVt){
				_isVn=true;
			}
			else if(_isVn&&!_isVt){
				_isVt=true;
			}	
			continue;
		}
		else if(ruleLine[0][i]=='}'){
			if(_isVt){
				_isVn=false;
				_isVt=false;
				_isRule=true;
			}
			continue;
		}
		if(_isRule&&!_isStart&&ruleLine[0][i]!=','){
			Rule=ruleLine[0][i];
			_isStart=true;
			_isRule=false;
		}
		else if(_isStart&&ruleLine[0][i]!=','){
			Start=ruleLine[0][i];
			_isStart=false;
		}
		if(_isVn&&!_isVt&&ruleLine[0][i]!=','){
			Vn[VnNum] = ruleLine[0][i];
			VnNum++;
		}
		else if(_isVt&&ruleLine[0][i]!=','){
			Vt[VtNum] = ruleLine[0][i];
			VtNum++;
		}
	}
	rules[0][1]=Start;
	//判断非终结符是否可到达空
	VnAttribute Vnattr = isEmpty(rules,m,Vn,Vt,VnNum,VtNum);
	//构造每一个文法符号的first集 
	cout<<"\n----first----"<<endl;
	FIRST_X FIRST = getFirstSet(rules,m,Vn,Vt,VnNum,VtNum,Vnattr);
	for(int i=0;i<VnNum;i++){
		cout<<"FIRST("<<Vn[i]<<")=";
		for(int j=0;j<FIRST.count[i];j++){
			if(FIRST.set[i][j]!=-1)
				cout<<Vt[FIRST.set[i][j]]<<",";
			else
				cout<<"$,";
		}
		cout<<endl;
	}
	for(int i=200;i<200+VtNum;i++){
		cout<<"FIRST("<<Vt[i-200]<<")=";
		for(int j=0;j<FIRST.count[i];j++){
			if(FIRST.set[i][j]!=-1)
				cout<<Vt[FIRST.set[i][j]-200]<<",";
			else
				cout<<"$,";
		}
		cout<<endl;
	}
	//测试随机构造first集 
	int addArray[20]={-1};
	int addCount=0;
	addArray[0]=getVnIndex('S',Vn,VnNum);
	addArray[1]=getVnIndex('a',Vt,VtNum)+200;
	FRIST_Y first_y = getCovFirst(FIRST,addArray,2,Vn,Vt,VnNum,VtNum,-1);
	//构造每一个非终结符的follow集
	cout<<"\n----follow----"<<endl;
	FOLLOW_X FOLLOW = getFollowSet(rules,m,Vn,Vt,VnNum,VtNum,Start,FIRST);
	for(int i=0;i<VnNum;i++){
		cout<<"FOLLOW("<<Vn[i]<<")=";
		for(int j=0;j<FOLLOW.count[i];j++){
			if(FOLLOW.set[i][j]!=-1)
				cout<<Vt[FOLLOW.set[i][j]]<<",";
			else
				cout<<"#,";
		}
		cout<<endl;
	}
	
	//新规则构造 LR_1项目
	initItem INIT_I = getInitTtems(rules,m,Vn,Vt,VnNum,VtNum,Start);
	ItemSets ISets = getItemSets(INIT_I,rules,m,Vn,Vt,VnNum,VtNum,Start,FIRST,ruleCount);
	//ItemSet ISet=getISet(rules,m,Vn,Vt,VnNum,VtNum,Start,FIRST);
	//输送出来看一下
	for(int i=0;i<ISets.ISetNum;i++){
		cout<<"I["<<i<<"]------------------------\n";
		for(int k2 =0;k2<ISets.ISet[i].itemNum;k2++){
			cout<<"        ";
			printItem(INIT_I,rules,ISets.ISet[i].item[k2],Vt,Start);
		}
	} 
	printChange(ISets.changeMap,ISets.changeNum,Vn,Vt,VnNum,VtNum);
	//处理ACTION-GOTO表 
	for(int i=0;i<ISets.ISetNum;i++){
		for(int j=0;j<VtNum+1;j++){
			action[i][j]=200;
		}
	}
	for(int i=0;i<ISets.ISetNum;i++){
		for(int j=0;j<VnNum+1;j++){
			gotoT[i][j]=200;
		}
	}
	for(int i=0;i<ISets.changeNum;i++){
		if(ISets.changeMap[i][1]>=200){
			action[ISets.changeMap[i][0]][ISets.changeMap[i][1]-200]=ISets.changeMap[i][2];
		}
	}
	bool isEnded=false;
	int EndRuleIndex;
	int EndSetIndex;
	int EndSet[40];
	int EndSetNum=0;
	for(int i=0;i<ISets.ISetNum;i++){
		isEnded=false;
		for(int j=0;j<ISets.ISet[i].itemNum;j++){
			if(ISets.ISet[i].item[j].attr[2]==2){
				isEnded=true;
				EndRuleIndex=ISets.ISet[i].item[j].initIndex;
				EndSetIndex=i;
				for(int k=0;k<ISets.ISet[i].item[j].setCount;k++){
					EndSet[k] = ISets.ISet[i].item[j].set[k];
				}
				EndSetNum = ISets.ISet[i].item[j].setCount;
				break;
			}
			else if(ISets.ISet[i].item[j].attr[2]==3){
				EndRuleIndex=ISets.ISet[i].item[j].initIndex;
				EndSetIndex=i;
				action[EndSetIndex][VtNum]=0;
			}
		}
		if(isEnded){
			for(int k=0;k<EndSetNum;k++){
				if(EndSet[k]==-1){
					action[EndSetIndex][VtNum]=-ISets.initI.ruleIndex[EndRuleIndex];
				}
				else{
					action[EndSetIndex][EndSet[k]]=-ISets.initI.ruleIndex[EndRuleIndex];
				}
			}
		}
	}
	for(int i=0;i<ISets.changeNum;i++){
		if(ISets.changeMap[i][1]<200){
			gotoT[ISets.changeMap[i][0]][ISets.changeMap[i][1]]=ISets.changeMap[i][2];
		}
	}
	//输出查看ACTION-GOTO表 
	cout<<setw(3)<<" ";
	for(int i=0;i<VnNum+VtNum+1;i++){
		if(i<VtNum){
			cout<<setw(3)<<Vt[i];
		}
		else if(i==VtNum){
			cout<<setw(3)<<"#";
		}
		else {
			cout<<setw(3)<<Vn[i-VtNum-1];
		}
	}
	cout<<endl;
	for(int i=0;i<ISets.ISetNum;i++){
		cout<<setw(3)<<i;
		for(int j=0;j<VtNum+1;j++){
			if(action[i][j]>=200){
				cout<<setw(3)<<" ";
			}
			else{
				cout<<setw(3)<<action[i][j];
			}
		}
		for(int j=0;j<VnNum;j++){
			if(gotoT[i][j]>=200){
				cout<<setw(3)<<" ";
			}
			else{
				cout<<setw(3)<<gotoT[i][j];
			}
		}
		cout<<endl;
	}
	//开始语法分析
	string line;
	cin>>line;
	wrongMsg test1=test(line,action,gotoT,Vn,Vt,VnNum,VtNum,rules,ISets.ISetNum); 
	if(test1.ID==0){
		cout<<"YES"<<endl;
	}
	else{
		cout<<"NO"<<endl;
		cout<<"WRONG MESSAGE:位于第"<<test1.row<<"行、第"<<test1.col+1<<"个,字符'"<<test1.wrongChar<<"'多余"<<endl; 
	} 
	return 0;
}

