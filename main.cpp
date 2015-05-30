/* main.cpp
 */
#include "board.hpp"
#include <iostream>
using namespace std;
const int B_SIZE    = 9;			// 碁盤の大きさ
typedef struct child {
  Move move;       // 手の場所
  int games;   // この手を探索した回数
  double rate; // この手の勝率
  int next;    // この手を打ったあとのノード
} CHILD;

const int CHILD_MAX = B_SIZE*B_SIZE+1;  // +1はPASS用

typedef struct node {
  int child_num;          // 子局面の数
  CHILD child[CHILD_MAX];
  int games_sum;// playoutの総数
} NODE;

const int NODE_MAX = 100000;
NODE node[NODE_MAX];
int node_num = 0;          // 登録ノード数
const int NODE_EMPTY = -1; // 次のノードが存在しない場合
const int ILLEGAL_Z  = -1; // ルール違反の手
double all_sum;
double sum_soft;
double temp,r;
//double sum[BOARD_MAX];
//int Playout(Board board, FastRandom& random) const;
/*return score*/
int Playout(Board board,FastRandom& random){
  while (true){
    Move move = board.RandomLightMove(random);
    if (move.GetVertex() == Vertex::Pass()) break;
    board.PlayLegal(move);
    //board.Dump();
  }
  //cout << "score = " << board.PlayoutScore() << endl;
  //cout << "winner = " << board.PlayoutWinner().ToScore() << endl;
  return board.PlayoutScore();
}
/*return color*/
int Playout2(Board board,FastRandom& random){
  while (true){
    Move move = board.RandomLightMove(random);
    if (move.GetVertex() == Vertex::Pass()) break;
    board.PlayLegal(move);
  }
  return board.PlayoutWinner().ToScore();
}
void add_child(NODE *pN,Move move)
{
  int n = pN->child_num;
  pN->child[n].move  = move;
  pN->child[n].games = 0;
  pN->child[n].rate  = 0;
  pN->child[n].next  = NODE_EMPTY;
  pN->child_num++;
}

// ノードを作成する。作成したノード番号を返す
int create_node(Board board,FastRandom& random)
{
  if ( node_num == NODE_MAX ) { cout << "node over Err\n" << endl; exit(0); }
  NODE *pN = &node[node_num];
  pN->child_num = 0;
  Player pl = board.ActPlayer();
  uint ii_start = 0;
  uint ii = ii_start;
  while (true) { // TODO separate iterator
    Vertex v = board.EmptyVertex (ii);
    Board board_copy = board;  
    if (!board_copy.IsEyelike (pl, v) && board_copy.IsLegal (pl, v)) {
      Move move = board_copy.SimpleMove(v,random);
      add_child(pN,move);
    }
    
    ii += 1;
    ii &= ~(-(ii == board.EmptyVertexCount())); // if (ii==board->empty_v_cnt) ii=0;
    if(ii==ii_start)break;
  }
  //add_child(pN, 0);  // PASSも追加

  node_num++;
  return node_num-1; 
}
int search_uct(Board board,int node_n,FastRandom& random)
{
  NODE *pN = &node[node_n];
  // UCBが一番高い手を選ぶ
  int select = -1;
  double max_ucb = -999;
  int i;
  /*j=1;
  for (i=0; i <17;i++){
    if(pw_num[i]>pN->games_sum)
      {
       break;
      }
    j++;
    }*/
  //printf("j=%d\n",j);
  //cout << pN->child_num << endl;
  for (i=0; i<pN->child_num; i++) {//上位j手に限定
    CHILD *c = &pN->child[i];
    double ucb = 0;
    if ( c->games==0 ) {
      ucb = 10000 + rand();  // 未展開
    } else {
      const double C = 0.31;
      ucb = c->rate + C * sqrt( log(pN->games_sum) / c->games );
    }
    if ( ucb > max_ucb ) {
      max_ucb = ucb;
      select = i;
    }
    //printf("in i=%d c->games=%d select=%d\n",i,c->games,select);
  }
  //printf("select=%d\n",select);
  //cout << max_ucb << endl;
  if ( select == -1 ) { cout << "Err! select\n" << endl; exit(0); }

  CHILD *c = &pN->child[select];
  Move move = c->move;
  int win;
  board.PlayLegal(move);//１手進める
  if ( c->games == 0 ) {  // 最初の1回目はplayout
    win = -Playout2(board,random);
  } else {
    if ( c->next == NODE_EMPTY ) c->next = create_node(board,random);
    win = -search_uct(board, c->next, random);
  }
  // printf("win=%d\n",win);
  // 勝率を更新
  //cout << score << endl;
  c->rate = (c->rate * c->games + win) / (c->games + 1);
  c->games++;		// この手の回数を更新
  pN->games_sum++;  // 全体の回数を更新
  //cout << "win=" << win << endl;
  return win;
  
}


int uct_loop = 50000;  // uctでplayoutを行う回数

Move select_best_uct(Board board,FastRandom& random)
{
  node_num = 0;
  Board board_copy = board;
  int next = create_node(board_copy,random);
  
  int i;
  //int *board_copy = new int[BOARD_MAX]; 
  
  for (i=0; i<uct_loop; i++) {
    Board board_copy = board;
    search_uct(board_copy, next,random);
    
  }
  //delete [] board_copy;
  int best_i = -1;
  int max = -999;
  NODE *pN = &node[next];
  for (i=0; i<pN->child_num; i++) {
    CHILD *c = &pN->child[i];
    //  cout << c->games << endl;
    if ( c->games > max ) {
      best_i = i;
      max = c->games;
    }
    //printf("%3d:z=%2d,games=%5d,rate=%.4f\n",i,get81(c->z),c->games,c->rate);
  }
  Move ret_move = pN->child[best_i].move;
  
  
  //fprintf(stderr,"z=%2d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n",get81(ret_z),pN->child[best_i].rate,max,all_playouts,node_num);
  return ret_move;
}
/*Simple MonteCarlo*/
const int playout_num = 200;
Vertex select_best_move (Board board, FastRandom& random) {
  //uint ii_start = random.GetNextUint (board.EmptyVertexCount()); 
  Player pl = board.ActPlayer();
  uint ii_start = 0;
  uint ii = ii_start;
  int max_score = 0;
  Vertex max_vertex = Vertex::Pass();
  
  while (true) { // TODO separate iterator
    Vertex v = board.EmptyVertex (ii);
    Board board_copy = board;  
    if (!board_copy.IsEyelike (pl, v) && board_copy.IsLegal (pl, v)) {
      Move move = board_copy.SimpleMove(v,random);
      board_copy.PlayLegal(move);//create Node
      int count_score = 0;
      if(pl.ToScore()==1){
	for(int i = 0; i < playout_num; i++){//playout per Node
	  Board board_playout = board_copy;
	  if(Playout(board_playout,random)>0){
	    count_score ++;
	  }
	}
      }
      else {
	for(int i = 0; i < playout_num; i++){//playout per Node
	  Board board_playout = board_copy;
	  if(Playout(board_playout,random)<0){
	    count_score ++;
	  }
	}
      }
      if(max_score<count_score){
	max_vertex = v;
	max_score = count_score;
      }
    }
    ii += 1;
    ii &= ~(-(ii == board.EmptyVertexCount())); // if (ii==board->empty_v_cnt) ii=0;
    if (ii == ii_start) return max_vertex;
  }
}
int gtprun() {
    FastRandom random(80);
    Board board, empty; 
    string line, word;
    while (getline(cin, line)) {
	stringstream ss(line);
	if (! (ss >> word)) {
	    cerr << "error " << line << endl;
	    continue;
	}
	if (word == "name") {
	    cout << "= libego+kaneko" << endl << endl;
	    continue;
	}
	if (word == "protocol_version") {
	    cout << "= 2" << endl << endl;
	    continue;
	}
	if (word == "version") {
	    cout << "= test" << endl << endl;
	    continue;
	}
	if (word == "list_commands") {
	    cout << "= genmove" << endl
		 << "boardsize" << endl
		 << "clear_board" << endl
		 << "komi" << endl
		 << "play" << endl
		 << "quit" << endl
		 << endl;
	}
	if (word == "boardsize") {
	    int size = 9;
	    ss >> size;
	    if (board.Size() != size) {
		cerr << "not supported size " << line << endl;
		return 1;
	    }
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "clear_board") {
	    board.Load(empty);
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "show_board") {
	  board.Dump();
	  continue;
	}
	if (word == "komi") {
	    double value;
	    ss >> value;
	    board.SetKomi(value);
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "play") {
	    string player, location;
	    cout << "mark" << player << endl;
	    ss >> player >> location;
	    board.PlayLegal(player == "B" ? Player::Black() : Player::White(),
			    Vertex::OfGtpString(location));
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "genmove") {
	    string player;
	    ss >> player;
	    //Move move = board.SimpleMove(select_best_move(board,random),random);/*RandomLightMove(random);*/
	    Move move = select_best_uct(board,random);
	    board.PlayLegal(move);
	    cout << "= " << move.GetVertex().ToGtpString() << endl << endl;
	    continue;
	}
	if (word == "quit") {
	    cout << "=" << endl << endl;
	    return 0;
	}
	cerr << "unknown " << line << endl << endl;
    }    
    return 0;
}

void test()
{
    Board board; 
    //board.Clear();
    cerr << board.ToAsciiArt() << endl;

    FastRandom random(80);
    for (int i=0; i<16; ++i)
	cerr << random.GetNextUint(81) << endl;
    //int tesuu = 0;
    while (true) {
      //   Move move = board.RandomLightMove(random);
      //Move move = board.SimpleMove(select_best_move(board,random),random);
      Move move = select_best_uct(board,random);
	//cout << board.BothPlayerPass() << endl;
	//cerr << move << endl;
	if (move.GetVertex() == Vertex::Pass()) break;
	board.PlayLegal(move);
	board.Dump();
	//tesuu ++;
	//cout << tesuu << endl;
    }
}
int main() {
  test();
  //gtprun();
}
