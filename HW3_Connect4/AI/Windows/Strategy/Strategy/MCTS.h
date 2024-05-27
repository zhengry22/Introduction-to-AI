#include <iostream>
#include <ctime>
#include <cstring>
#include <cmath>
#include <random>
#include "Point.h"

using namespace std;

#define MAX_LINE_NUM  12
#define MAX_LINE_WIDTH 12
#define A_WIN 0
#define TIE 1
#define B_WIN 2
#define C_COEFF 2
#define UNUSED_POS			0
#define USER_USED_POS		1
#define COMPUTER_USED_POS	2
#define INVALID_NODE (STEP*)-1

struct SIM_STEP {
	unsigned char is_usera;		//1 is user A,0 is user B
	unsigned char is_terminated = 0; //1表示当前是胜利节点（已经有4个点相连）， 0表示普通节点
	char c_top[MAX_LINE_NUM];
	unsigned short Board_A[MAX_LINE_NUM];	//机器棋局
	unsigned short Board_B[MAX_LINE_NUM];	//用户（AI)棋局
};

struct STEP {
	int trycount;		//尝试次数
	int wincount;		//获胜次数
	unsigned char X;	 //该步骤的坐标 X
	unsigned char Y;	 //该步骤的坐标 Y
	unsigned char SubNodeNum;  //子节点数
	STEP* child[MAX_LINE_NUM];	//下一步棋局的指针， 未扩展前为nullptr
	STEP* parent;			   //指向父节点的指针
	SIM_STEP info;		 //将simulation阶段需要的信息放在此结构中
};


class MCTS {
private:
	int M;						// Number of rows
	int N;						// Number of cols
	int forbidden_X;			// NoX
	int forbidden_Y;			// NoY
	STEP* root_step;			// The array for storing all the nodes
	SIM_STEP simulate_buf;	    //蒙特卡罗模拟用的buffer，因为每次都清理，无需开辟多个
	int suffle_list[MAX_LINE_WIDTH]; //存放0~N的数据，然后每一轮用shuffle函数打乱，增加选择子节点的随机性
	int buf_top;  //未使用的buffer的顶部，新扩展节点放在此处。
	clock_t startTime;           //程序执行的起始时间
	const double timelimit = CLOCKS_PER_SEC * 2;	//每次允许模拟时间
	//打乱一个数组中的数据
	void shuffle(int* buf, int len) {
		int tmpval, index;
		for (int i = 0; i < N; i++) {
			index = rand() % N;
			tmpval = buf[i];
			buf[i] = index;
			buf[index] = tmpval;
		}
	}
	//判断当前棋局是否胜利（横竖对角线有4子相连）
	bool checkwin(unsigned short* board, int x, int y) {
		//横向检测
		int i, j;
		int count = 0;
		for (i = y; i >= 0; i--)
			if (!((board[x] >> i) & 1))
				break;
		count += (y - i);
		for (i = y; i < N; i++)
			if (!((board[x] >> i) & 1))
				break;
		count += (i - y - 1);
		if (count >= 4) return true;

		//纵向检测
		count = 0;
		for (i = x; i < M; i++)
			if (!((board[i] >> y) & 1))
				break;
		count += (i - x);
		if (count >= 4) return true;

		//左下-右上
		count = 0;
		for (i = x, j = y; i < M && j >= 0; i++, j--)
			if (!((board[i] >> j) & 1))
				break;
		count += (y - j);
		for (i = x, j = y; i >= 0 && j < N; i--, j++)
			if (!((board[i] >> j) & 1))
				break;
		count += (j - y - 1);
		if (count >= 4) return true;

		//左上-右下
		count = 0;
		for (i = x, j = y; i >= 0 && j >= 0; i--, j--)
			if (!((board[i] >> j) & 1))
				break;
		count += (y - j);
		for (i = x, j = y; i < M && j < N; i++, j++)
			if (!((board[i] >> j) & 1))
				break;
		count += (j - y - 1);
		if (count >= 4) return true;

		return false;
	}
public:
	MCTS(const int* bd, const int M, const int N, const int* tp, const int noX, const int noY) {
		srand((unsigned int)time(NULL));
		startTime = clock();
		root_step = new STEP[10000000]; // Array for storing all the nodes
		buf_top = 0;
		this->M = M;
		this->N = N;
		buf_top++; //扩展根 节点
		memset(root_step, 0, sizeof(STEP));
		for (int i = 0; i < M; i++) {
			//压缩board的格式，A，B分别用不同的变量保存。
			for (int j = 0; j < N; j++) {
				if (bd[i * N + j] == 1)	 //用户（AI）
					root_step[0].info.Board_A[i] |= 1 << j;
				if (bd[i * N + j] == 2) //机器
					root_step[0].info.Board_B[i] |= 1 << j;
			}
		}
		for (int i = 0; i < N; i++) {
			root_step[0].info.c_top[i] = (unsigned char)tp[i];
			root_step[0].child[i] = nullptr;
		}
		root_step[0].info.is_usera = 1; // In the beginning, we set ourselves as User a.
		root_step[0].parent = nullptr;
		forbidden_X = noX;
		forbidden_Y = noY;
		for (int i = 0; i < N; i++)
			suffle_list[i] = i;
		//shuffle(suffle_list, N);
	}

	~MCTS() {
		delete[] root_step;
	}

	Point Execute() {
		// What does execute do ?
		int rnd = 0;
		STEP* selected_node = nullptr;
		while (true) {
			if (rnd % 10000 == 0) {
				// Calc time
				clock_t endTime = clock();
				if (endTime - startTime >= timelimit) {
					break;
				}
			}
			selected_node = Select();
			//cout << "selected node: " << endl;
			//Debug(&selected_node->info);
			if (selected_node == INVALID_NODE) {
				// The next step can't be done
				continue;
			}
			if (selected_node->info.is_terminated == 1) {
				int winner = (selected_node->info.is_usera == 1) ? A_WIN : B_WIN;
				/*cout << "The node we select that terminated is:" << endl;
				Debug(&selected_node->info);*/
				Update(selected_node, winner);
				continue;
			}
			int winner = Rollout(&selected_node->info);
			Update(selected_node, winner);
			/*cout << "The node we selected is :" << endl;
			Debug(&selected_node->info);*/
			rnd++;
		}
		// Select the child of root that has the maximum access rate
		STEP* Max_Visit = get_subnode_by_maxvisit(root_step);
		// STEP* Max_Visit = get_subnode_by_winrate(root_step);
		Point ret(Max_Visit->X, Max_Visit->Y);
		return ret;
	}

	void Debug(SIM_STEP* nd) {
		for (int i = 0; i < 12; i++) {
			cout << endl;
			for (int j = 0; j < 12; j++) {
				if ((nd->Board_A[i] >> j) & 0x01)
					cout << "1 ";
				else  if ((nd->Board_B[i] >> j) & 0x01)
					cout << "* ";
				else
					cout << "0 ";
			}
		}
		cout << endl;
	}

	STEP* get_subnode_by_maxvisit(STEP* nd) {
		int i;
		int maxindex = 0;
		int maxvisit = 0;
		for (i = 0; i < this->N; i++) {
			if (nd->child[i] != nullptr && nd->child[i] != INVALID_NODE) {
				if (nd->child[i]->trycount > maxvisit) {
					maxvisit = nd->child[i]->trycount;
					maxindex = i;
				}
			}
		}
		return nd->child[maxindex];
	}

	STEP* get_subnode_by_winrate(STEP* nd) {
		int i;
		int maxindex = 0;
		int winrate = 0;
		for (i = 0; i < this->N; i++) {
			if (nd->child[i] != nullptr && nd->child[i] != INVALID_NODE) {
				if (nd->child[i]->wincount / nd->child[i]->trycount > winrate) {
					winrate = nd->child[i]->wincount / nd->child[i]->trycount;
					maxindex = i;
				}
			}
		}
		return nd->child[maxindex];
	}

	STEP* get_subnode_by_ucb(STEP* nd) {
		//计算公式：UCB=u/n+C*sqrt(ln(n(parent)/n)
		char max_id = -1;
		double max_UCB = -1;
		for (char i = 0; i < N; i++) {
			if (nd->child[i] != INVALID_NODE && nd->child[i] != nullptr) {
				// Calculate UCB
				double this_UCB = ((double)(nd->child[i]->wincount) / (double)(nd->child[i]->trycount))
					+ sqrt((double)(((double)C_COEFF * log(nd->child[i]->parent->trycount)) / (double)(nd->child[i]->trycount)));
				// Update max
				if (this_UCB > max_UCB) {
					max_UCB = this_UCB;
					max_id = i;
				}
			}
		}
		if (max_id == -1) {
			// All nodes are invalid
			return INVALID_NODE;
		}
		return nd->child[max_id];
	}

	char HasUnvisitedChild(STEP* current) {
		for (char i = 0; i < N; i++) {
			if (current->child[i] == nullptr)
				return i;
		}
		return -1; // -1 means all children have been visited
	}

	STEP* Select() {
		// First find the leaf that has unexpanded sub nodes
		STEP* state = root_step;
		char id = -1;
		while ((state != INVALID_NODE) && (state != nullptr) && (id = HasUnvisitedChild(state)) == -1) {
			// If the node's children are all visited, then set state as the one that has the largest UCB
			state = get_subnode_by_ucb(state);
		}
		// Find a child of state that hasn't been expanded yet
		if (state == INVALID_NODE) return INVALID_NODE;
		return Expand(state, (int)id);
	}

	STEP* Expand(STEP* nd, int index) {
		// Some situations that can't be expanded 
		if (nd == INVALID_NODE) return INVALID_NODE;
		if ((nd->info.c_top[index] == 0) || ((nd->info.c_top[index] == 1) && (forbidden_X == 0) && (forbidden_Y == index))) {
			nd->child[index] = INVALID_NODE;
			return INVALID_NODE;
		}

		// The connection made between parent and child
		nd->child[index] = &root_step[buf_top]; // The node is actually stored in root_step
		root_step[buf_top].parent = nd;
		root_step[buf_top].trycount = 0;
		root_step[buf_top].wincount = 0;
		nd->child[index]->X = nd->info.c_top[index] - 1;
		nd->child[index]->Y = index;

		// Be careful that initially, the child ptrs are not set to be null. 
		for (int i = 0; i < N; i++) {
			nd->child[index]->child[i] = nullptr;
		}

		// In case of forbidden
		if ((nd->child[index]->X == forbidden_X) && (nd->child[index]->Y == forbidden_Y)) {
			nd->child[index]->X--;
		}

		// Copy info
		memcpy(&nd->child[index]->info, &nd->info, sizeof(SIM_STEP));

		// Adjustments
		nd->child[index]->info.is_usera = (nd->child[index]->info.is_usera == 0) ? 1 : 0;
		nd->child[index]->info.c_top[index]--;

		// In case that the upper node is the forbidden node
		if ((nd->child[index]->info.c_top[index] - 1 == forbidden_X) && (index == forbidden_Y)) {
			nd->child[index]->info.c_top[index]--;
		}

		buf_top++;

		// bit operations 
		unsigned short bitop = 0x0001;
		bitop <<= index;
		if (nd->child[index]->info.is_usera == 1) {
			// operate Board a
			nd->child[index]->info.Board_A[nd->child[index]->X] |= bitop;
			// Check whether a wins
			if (checkwin(nd->child[index]->info.Board_A, nd->child[index]->X, nd->child[index]->Y)) {
				nd->child[index]->info.is_terminated = 1;
			}
		}
		else {
			// operate Board b
			nd->child[index]->info.Board_B[nd->child[index]->X] |= bitop;
			// Check whether a wins
			if (checkwin(nd->child[index]->info.Board_B, nd->child[index]->X, nd->child[index]->Y)) {
				nd->child[index]->info.is_terminated = 1;
			}
		}
		// cout << "Expanded node is: " << endl;
		// Debug(&nd->child[index]->info);
		return nd->child[index];
	}

	int Rollout(SIM_STEP* ss) {
		// copy ss to simulate buf, so that other children won't be affected
		simulate_buf = *ss;
		// ss stands for the current layout. The next step we should reverse is_usera

		unsigned char X, Y;
		unsigned short* bd;
		unsigned short bitop;

		// Use do-while to refactor
		do {
			// Reverse the user
			simulate_buf.is_usera = (simulate_buf.is_usera == 1) ? 0 : 1;

			// generate random number bewteen 0 - 11 as the next step
			Y = rand() % N;
			X = simulate_buf.c_top[Y] - 1;
			if (X == forbidden_X && Y == forbidden_Y) {
				X--;
			}
			if (X < 0) {
				bool flag = 0; // There might be a chance that all the places are taken
				for (int i = 1; i < N; i++) {
					Y = (Y + i) % N;
					X = simulate_buf.c_top[Y] - 1;
					if (X == forbidden_X && Y == forbidden_Y) {
						X--;
					}
					if (X >= 0) {
						flag = 1;
						break;
					}
				}
				if (flag == 0) {
					// Full, and it's a tie
					return TIE;
				}
			}

			simulate_buf.c_top[Y]--;
			if (simulate_buf.c_top[Y] - 1 == forbidden_X && Y == forbidden_Y) {
				simulate_buf.c_top[Y]--;
			}

			bd = (simulate_buf.is_usera == 1) ? simulate_buf.Board_A : simulate_buf.Board_B;
			bitop = 0x0001;
			bitop <<= Y;


			if (simulate_buf.is_usera == 1) {
				// operate Board a
				simulate_buf.Board_A[X] |= bitop;
			}
			else {
				// operate Board b
				simulate_buf.Board_B[X] |= bitop;
			}
			//cout << "The current layout is: " << endl;
			//Debug(&simulate_buf);
		} while (!checkwin(bd, X, Y));

		// Now there should be a winner already
		return (simulate_buf.is_usera == 1) ? A_WIN : B_WIN;
	}

	void Update(STEP* ss, int val) {
		STEP* cur_node = ss;
		while (cur_node != nullptr) {
			// if cur_node == nullptr, it means that this is the root of MCT
			cur_node->trycount++;
			if ((cur_node->info.is_usera == 1 && val == A_WIN) || (cur_node->info.is_usera == 0 && val == B_WIN)) {
				cur_node->wincount++;
			}
			cur_node = cur_node->parent;
		}
	}
};

