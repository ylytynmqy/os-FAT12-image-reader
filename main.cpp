#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

#define DATA_START 0x3e00
#define BytsPerSec 0x200
#define RootEntCnt 0x200

using namespace std;
const char *RED = "\033[31m";//5
const char *NORMAL = "\033[m";//4
extern "C" {
	void mprint(const char* c, int length);
}
vector<string> split(const string& src, string split_f)
{
	vector<string> strs;

	int separate_characterLen = split_f.size();
	int lastPosition = 0, index = -1;
	while (-1 != (index = src.find(split_f, lastPosition)))
	{
		strs.push_back(src.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;
	}
	string lastString = src.substr(lastPosition);//截取最后一个分隔符后的内容的特殊情况
	if (!lastString.empty())
		strs.push_back(lastString);
	return strs;
}

#pragma pack(push)
#pragma pack(1)
struct Fat12Header
{
	char BS_OEMName[8];//OEM字符串，必须为8个字符，不足以空格填空
	unsigned short BPB_BytsPerSec;//每扇区字节数//512
	unsigned char BPB_SecPerClus;//每簇占用的扇区数
	unsigned short BPB_RsvdSecCnt;//Boot占用的扇区数
	unsigned char BPB_NumFATs;//FAT表的记录数
	unsigned short BPB_RootEntCnt;//最大根目录文件数
	unsigned short BPB_TotSec16;//每个FAT占用扇区数
	unsigned char BPB_Media;//媒体描述符
	unsigned short BPB_FATSz16;//每个FAT占用扇区数
	unsigned short BPB_SecPerTrk;//每个磁道扇区数
	unsigned short BPB_NumHeads;//磁头数
	unsigned int BPB_HiddSec;//隐藏扇区数
	unsigned int BPB_TotSec32;//如果BPB_TotSec16是0，则在这里记录
	unsigned char BS_DrvNum;//中断13的驱动器号
	unsigned char BS_Reserved1;//未使用
	unsigned char BS_BootSig;//扩展引导标志
	unsigned int BS_VolID;//卷序列号
	char BS_VolLab[11];//卷标，必须是11个字符，不足以空格填充
	char BS_FileSysType[8];//文件系统类型，必须是8个字符，不足填充空格
};
struct Entry
{
	char DIR_Name[11];
	unsigned char DIR_Attr;
	char reserve[10];
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClus;//开始簇号
	unsigned int DIR_FileSize;//size
};
struct detailchild {
	string name;
	bool isDir;
	int directory_sum;//目录数量
	int file_sum;//文件数量
	unsigned int size;
};
struct Node {
	bool isRoot;
	unsigned short father;//父目录编号
	unsigned short start;//起始编号
	string path;
	vector<unsigned short> children;//子目录编号集
	string Name;//包括非目录的格式
	bool isDir;//是否是目录
	//是目录
	int directory_sum;//目录数量
	int file_sum;//文件数量
	vector<detailchild> resChildren;
	//若是文件
	unsigned int size;//文件大小
};
struct Filec {
	char content[512];
};
#pragma pack(pop)
vector<Node> nodeslist;
void findallentry(const vector<unsigned short> &list,const unsigned short father,vector<string> &names,vector<unsigned int> &sizes,const string fa_path) {
	//fstream fs("C:\\myfilefilefilefilefile\\window-share-files\\ref.img", ios::binary | ios::in);//windows打开img
	fstream fs("./ref.img", ios::binary | ios::in);//linux打开img
	if (!list.empty()) {
		for (int i = 0; i < list.size(); i++) {
			Node root;
			root.isRoot = 0;
			root.father = father;
			root.start = list[i];
			root.Name = names[i];
			root.size = sizes[i];
			root.isDir = 1;
			for (int d = 0; d < root.Name.size(); d++) {
				if (root.Name[d] == '.') {
					root.isDir = 0;
				}
			}
			vector<string> childrennames;
			vector<unsigned int> childrensize;
			if (root.isDir == 1) {
				root.path = fa_path + root.Name + "/";
				root.directory_sum = 0;
				root.file_sum = 0;
				for (int a = 2; i < RootEntCnt; a++)//找出所有root_directory 并且跳过那些. ..
				{
					Entry entry;
					fs.seekg(DATA_START + root.start * BytsPerSec + a * sizeof(Entry));
					fs.read(reinterpret_cast<char*>(&entry), sizeof(Entry));
					if (entry.DIR_Name[0] != 0x00)//不为空
					{
						string cname = "";
						unsigned int csize = 0;
						root.children.push_back(entry.DIR_FstClus);
						if (entry.DIR_Name[8] == 0x20 && entry.DIR_Name[9] == 0x20 && entry.DIR_Name[10] == 0x20) {
							//isdir
							if (entry.DIR_Name[0] == 0x2E) {
								//说明是.//..
							}
							else {
								root.directory_sum++;
								for (int j = 0; j < 8; j++) {
									if (entry.DIR_Name[j] == 0x00 || entry.DIR_Name[j] == 0x20) {
										break;
									}
									else {
										cname += entry.DIR_Name[j];
									}
								}
							}
						}
						else {
							//isntdir
							for (int j = 0; j < 8; j++) {
								if (entry.DIR_Name[j] == 0x00 || entry.DIR_Name[j] == 0x20) {
									break;
								}
								else {
									cname += entry.DIR_Name[j];
								}
							}
							cname += '.';
							for (int j = 8; j < 11; j++) {
								if (entry.DIR_Name[j] == 0x00 || entry.DIR_Name[j] == 0x20) {
									break;
								}
								else {
									cname += entry.DIR_Name[j];
								}
							}
							root.file_sum++;
							csize = entry.DIR_FileSize;
						}
						childrennames.push_back(cname);
						childrensize.push_back(csize);
					}
					else
					{
						break;
					}
				}
				nodeslist.push_back(root);

			}
			else {
				nodeslist.push_back(root);
			}
			if (root.isDir == 1) {
				//string yes = "yes";
				//mprint(COLOR_RED, 5);
				//mprint(yes.c_str(), yes.size());
				//mprint(COLOR_RESET, 4);
				findallentry(root.children, root.start, childrennames, childrensize,root.path);
			}
		}
	}
	fs.close();
}
void myp(const string input) {
	int len = input.size();
	mprint(input.c_str(), len);
}
void mrp(const string input) {
	mprint(RED, 5);
	int len = input.size();
	mprint(input.c_str(), len);
	mprint(NORMAL, 4);
}
void resLS(Node &now) {

	//cout << now.path << ":" << endl;
	string temp = now.path + ":"+"\n";
	myp(temp);
	if (now.isRoot == 1) {
		//root
	}
	else {
		temp = ".  ..  ";
		mrp(temp);
		//cout << "." << "  " << ".."<<"  ";//red,7
		//isnt root
	}
	for (int i = 0; i < now.resChildren.size(); i++) {
		if (now.resChildren[i].isDir == 1) {
			//dir
			//red
			temp = now.resChildren[i].name + "  ";
			mrp(temp);
			//cout << now.resChildren[i].name << "  ";
		}
		else {
			//file
			temp = now.resChildren[i].name + "  ";
			myp(temp);
			//cout << now.resChildren[i].name << "  ";
		}
	}
	temp = "\n";
	myp(temp);
	//cout << endl;
	for (int i = 0; i < now.resChildren.size(); i++) {
		if (now.resChildren[i].isDir == 1) {
			//在nodelist找到
			unsigned short temp = now.children[i];
			for (int j = 0; j < nodeslist.size(); j++) {
				if (temp == nodeslist[j].start) {
					resLS(nodeslist[j]);
					break;
				}
			}
		}
	}
}
void resLSL(Node &now) {
	string temp = "";
	temp = now.path + " " +to_string(now.directory_sum) + " " +to_string( now.file_sum )+ ":\n";
	myp(temp);
	//cout << now.path << " "<<now.directory_sum<<" "<<now.file_sum<<":" << endl;
	if (now.isRoot == 1) {
		//root
	}
	else {
		temp = ".\n..\n";
		mrp(temp);
		//cout << "." << endl;//red
		//cout << ".." << endl;//red
		//isnt root
	}
	for (int i = 0; i < now.resChildren.size(); i++) {
		if (now.resChildren[i].isDir == 1) {
			//dir
			temp = now.resChildren[i].name + "  ";
			mrp(temp);
			//cout << now.resChildren[i].name << "  ";//red
			temp = to_string(now.resChildren[i].directory_sum )+ " " +to_string(now.resChildren[i].file_sum) + "\n";
			myp(temp);
			//cout << now.resChildren[i].directory_sum << " " << now.resChildren[i].file_sum << endl;//white
		}
		else {
			//file
			temp = now.resChildren[i].name +"  "+ to_string(now.resChildren[i].size)+"\n";
			myp(temp);
			//cout << now.resChildren[i].name << "  ";
			//cout << now.resChildren[i].size << endl;
		}
	}
	temp = "\n";
	myp(temp);
	//cout << endl;
	for (int i = 0; i < now.resChildren.size(); i++) {
		if (now.resChildren[i].isDir == 1) {
			//在nodelist找到
			unsigned short temp = now.children[i];
			for (int j = 0; j < nodeslist.size(); j++) {
				if (temp == nodeslist[j].start) {
					resLSL(nodeslist[j]);
					break;
				}
			}
		}
	}
}
void catF(Node node) {
	//fstream fs("C:\\myfilefilefilefilefile\\window-share-files\\ref.img", ios::binary | ios::in);//windows打开img
	fstream fs("./ref.img", ios::binary | ios::in);//linux打开img
	int sum = node.size;
	string temp = "";
	for (int j = 0; j < 0x200; j++) {
		Filec fc;
		if (sum <= 0x200) {//小于一簇
			fs.seekg(DATA_START + (j+node.start) * BytsPerSec);//起始
			fs.read(reinterpret_cast<char*>(&fc), sizeof(Filec));
			for (unsigned int j = 0; j < sum; j++) {
				temp = fc.content[j];
				myp(temp);
				//cout << fc.content[j];
			}
			temp = "\n";
			myp(temp);
			//cout << endl;
			break;
		}
		else {
			fs.seekg(DATA_START + (j + node.start) * BytsPerSec);//起始
			fs.read(reinterpret_cast<char*>(&fc), sizeof(Filec));
			for (unsigned int j = 0; j < 0x200; j++) {
				temp = fc.content[j];
				myp(temp);
				//cout << fc.content[j];
			}
		}
	}
	fs.close();
}
int main()
{
	//打开文件
	Fat12Header f12;
	//fstream file("C:\\myfilefilefilefilefile\\window-share-files\\ref.img", ios::binary | ios::in);//windows打开img
	fstream file("./ref.img", ios::binary | ios::in);//linux打开img
	file.seekg(3);//定向
	file.read(reinterpret_cast<char *>(&f12), sizeof(Fat12Header));
	Node root;//根目录
	root.isRoot = 1;
	root.father = 0;
	root.isDir = 1;
	root.Name = "";
	root.path = "/";
	root.start = 1;
	root.size = 0;
	root.file_sum = 0;
	root.directory_sum = 0;
	vector<string> childrennames;
	vector<unsigned int> childrensize;
	for (int i = 0; i < f12.BPB_RootEntCnt; i++)//找出所有root_directory
	{
		Entry re;
		file.seekg(19 * f12.BPB_BytsPerSec + i * sizeof(Entry));
		file.read(reinterpret_cast<char*>(&re), sizeof(Entry));
		if (re.DIR_Name[0] != 0x00)//不为空
		{
			string cname="";
			unsigned int csize = 0;
			root.children.push_back(re.DIR_FstClus);
			if (re.DIR_Name[8] == 0x20 && re.DIR_Name[9] == 0x20 && re.DIR_Name[10] == 0x20) {
				//isdir
				root.directory_sum++;
				for (int j = 0; j < 8; j++) {
					if (re.DIR_Name[j] == 0x00 || re.DIR_Name[j] == 0x20) {
						break;
					}
					else {
						cname += re.DIR_Name[j];
					}
				}
			}
			else {
				//isntdir
				for (int j = 0; j < 8; j++) {
					if (re.DIR_Name[j] == 0x00 || re.DIR_Name[j] == 0x20) {
						break;
					}
					else {
						cname += re.DIR_Name[j];
					}
				}
				cname += '.';
				for (int j = 8; j < 11; j++) {
					if (re.DIR_Name[j] == 0x00|| re.DIR_Name[j] == 0x20) {
						break;
					}
					else {
						cname += re.DIR_Name[j];
					}
				}
				root.file_sum++;
				csize = re.DIR_FileSize;
			}
			childrennames.push_back(cname);
			childrensize.push_back(csize);
		}
		else
		{
			break;
		}
	}
	nodeslist.push_back(root);
	findallentry(root.children,root.start,childrennames,childrensize,root.path);
	for (int i = 1; i < nodeslist.size(); i++) {
		detailchild dc;
		dc.isDir = nodeslist[i].isDir;
		dc.name = nodeslist[i].Name;
		if (dc.isDir == 1) {
			//isdir
			dc.directory_sum = nodeslist[i].directory_sum;
			dc.file_sum = nodeslist[i].file_sum;
		}
		else {
			//file
			dc.size = nodeslist[i].size;
		}
		unsigned short father=nodeslist[i].father;
		int fa_index=-1;
		for (int j = 0; j < i; j++) {
			if (nodeslist[j].start == father) {
				fa_index = j;
				break;
			}
		}
		nodeslist[fa_index].resChildren.push_back(dc);
	}

	string command = "";
	string temp = "";
	while (true) {
		getline(cin, command);

		if(command==""){//输入的是回车
			temp = "please input a command:\n";
			myp(temp);
			//cout << "please input a command:" << endl;
		}
		else{
			command.erase(0, command.find_first_not_of(" "));
			command.erase(command.find_last_not_of(" ") + 1);//去除空格
			vector<string> list = split(command, " ");
			if (list.size()==0) {
				temp = "this command does not exit.\n";
				myp(temp);
				//cout << "this command does not exit." << endl;//敲回车专用
			}
			if (list.size() == 1 && list[0] == "exit") {//exit指令 退出
				file.close();
				return 0;
			}
			else if (list[0] == "cat") {//显示文件
				if (list.size() == 1) {
					temp = "cat lack file name.\n";
					myp(temp);
					//cout << "cat lack file name." << endl;
				}
				else if (list.size() == 2) {
					//需要读取img
					if (list[1][0] == '/') {
						//正常写法
						vector<string> pathsplit = split(command, "/");
						string filename = pathsplit[pathsplit.size() - 1];
						bool noH = 0;
						for (int p = 0; p < nodeslist.size(); p++) {
							if (filename == nodeslist[p].Name) {
								catF(nodeslist[p]);
								noH = 1;
								break;
							}
						}
						if (noH == 0) {
							temp = "cat: no such file.\n";
							myp(temp);
							//cout << "cat: no such file." << endl;
						}
					}
					else {
						string filename = list[1];
						bool noH = 0;
						for (int p = 0; p < nodeslist.size(); p++) {
							if (filename == nodeslist[p].Name) {
								catF(nodeslist[p]);
								noH = 1;
								break;
							}
						}
						if (noH == 0) {
							temp = "cat: no such file.\n";
							myp(temp);
							//cout << "cat: no such file." << endl;
						}
					}
				}
				else{
					//超过1个参数 报错
					temp = "cat has too many parameters.\n";
					myp(temp);
					//cout << "cat has too many parameters." << endl;
				}
			}
			else if (list[0] == "ls") {
				if (list.size() == 1) {
					string rootdir = "/";
					//ls 根目录 读取img
					resLS(nodeslist[0]);
				}
				else {
					bool isLS = 1;
					for (int a = 0; a < list.size(); a++) {
						if (list[a].substr(0, 1) == "-") {
							isLS = 0;
							break;
						}
					}
					if (isLS == 1) {
						//仅仅是ls
						//需要读取img
						for (int b = 0; b < list.size(); b++) {
							if (list[b][0]=='/') {
								string rootdir = "";
								if (list[b][list[b].size() - 1] == '/') {
									rootdir = list[b];
								}
								else {
									rootdir = list[b] + "/";
								}
								Node now;
								bool noH = 0;
								for (int c = 0; c < nodeslist.size(); c++) {
									if (nodeslist[c].path == rootdir) {
										now = nodeslist[c];
										noH = 1;
										resLS(now);
										break;
									}
								}
								if (noH == 0) {
									temp = "ls: no such directory.\n";
									myp(temp);
									//cout << "ls: no such directory." << endl;
								}
							}
						}
					}
					else {
						bool hasVaildPara = 1;
						int countPara = 0;//计算有几个参数
						for (int a = 0; a < list.size(); a++) {
							if (list[a].substr(0, 1) == "-") {
								//参数 需要用正则表达式匹配
								string str = list[a];
								smatch result;
								string regex_str2("-l+");
								regex pattern2(regex_str2, regex::icase);

								if (regex_match(str, result, pattern2)) {
									//匹配正确
									countPara++;
								}
								else {
									//匹配失败
									hasVaildPara = 0;
								}
							}
						}
						if (hasVaildPara == 1) {
							//有参数下面处理文件
							if (list.size() == (1 + countPara)) {
								//无文件 报错
								string rootdir = "/";
								resLSL(nodeslist[0]);
							}
							else {
								//有文件
								//需要读取img
								for (int b = 0; b < list.size(); b++) {
									if (list[b][0] == '/') {
										string rootdir = "";
										if (list[b][list[b].size() - 1] == '/') {
											rootdir = list[b];
										}
										else {
											rootdir = list[b] + "/";
										}
										Node now;
										bool noH = 0;
										for (int c = 0; c < nodeslist.size(); c++) {
											if (nodeslist[c].path == rootdir) {
												now = nodeslist[c];
												resLSL(now);
												noH = 1;
												break;
											}
										}
										if (noH == 0) {
											temp = "ls -l: no such directory.\n";
											myp(temp);
											//cout << "ls -l: no such direcotry." << endl;
										}
									}
								}
							}
						}
						else {
							temp = "ls lack valid parameter.\n";
							myp(temp);
							//cout << "ls lack valid parameter." << endl;
						}
					}
				}
			}
			else {
				temp = "this command does not exit.\n";
				myp(temp);
				//cout << "this command does not exit." << endl; 
			}
		}
	}
}
