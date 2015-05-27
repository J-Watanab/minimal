/* main.cpp
 */
#include "board.hpp"
#include <iostream>
using namespace std;
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
	if (word == "komi") {
	    double value;
	    ss >> value;
	    board.SetKomi(value);
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "play") {
	    string player, location;
	    ss >> player >> location;
	    board.PlayLegal(player == "B" ? Player::Black() : Player::White(),
			    Vertex::OfGtpString(location));
	    cout << "=" << endl << endl;
	    continue;
	}
	if (word == "genmove") {
	    string player;
	    ss >> player;
	    Move move = board.RandomLightMove(random);
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
//int Playout(Board board, FastRandom& random) const;
int Playout(Board board,FastRandom& random){
  while (true){
    Move move = board.RandomLightMove(random);
    if (move.GetVertex() == Vertex::Pass()) break;
    board.PlayLegal(move);
    //board.Dump();
  }
  return board.PlayoutScore();
}
/*Simple MonteCarlo*/
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
	for(int i = 0; i < 500; i++){//playout per Node
	  Board board_playout = board_copy;
	  if(Playout(board_playout,random)>0){
	    count_score ++;
	  }
	}
      }
      else {
	for(int i = 0; i < 500; i++){//playout per Node
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
      Move move = board.SimpleMove(select_best_move(board,random),random);
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
  // gtprun();
}
