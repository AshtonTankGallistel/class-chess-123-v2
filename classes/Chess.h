#pragma once
#include "Game.h"
#include "ChessSquare.h"

const int pieceSize = 64;

enum ChessPiece {
    NoPiece = 0,
    Pawn = 1,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

//
// the main game class
//
class Chess : public Game
{
public:
    Chess();
    ~Chess();

    // set up the board
    void        setUpBoard() override;

    Player*     checkForWinner() override;
    bool        checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() override;
    void        setStateString(const std::string &s) override;
    bool        actionForEmptyHolder(BitHolder& holder) override;
    bool        canBitMoveFrom(Bit& bit, BitHolder& src) override;
    bool        canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
    void        bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    //HELPER FUNCTION FOR ABOVE 4 FUNCS, FINDS OUT WHICH MOVES A PIECE CAN PERFORM
    std::vector<int>*   getPossibleMoves(Bit &bit, BitHolder &src);
    //HELPER FUNCTION, CLEARS ALL HIGHLIGHTS
    void        clearHighlights();
    //MOVE GENERATOR FUNCTION. GETS ALL POSSIBLE MOVES FOR THE CURRENT TURN
    void        generateMoves();
    //BOARD SETUP FUNCTION. SETS UP BOARD BASED ON FEN STRING
    void        FENtoBoard(std::string FEN);
    //HELPER FUNCTION, SETS A PIECE ONTO THE BOARD
    void        setPiece(int pos, int player, ChessPiece piece);


    void        stopGame() override;
    BitHolder& getHolderAt(const int x, const int y) override { return _grid[y][x]; }

	void        updateAI() override;
    bool        gameHasAI() override { return true; }
private:
    Bit *       PieceForPlayer(const int playerNumber, ChessPiece piece);
    const char  bitToPieceNotation(int row, int column) const;

    ChessSquare      _grid[8][8];

    //ADDED VAR, TRACKS WHICH INDEXES THE PIECE CAN BE MOVED TO. MAX OF 28 (A piece can only have 27 spots total, so that+1)
    //ENDS AT -1.
    int         possibleMoves[28];
    std::vector<int>* potentialMoves[64];
    //GAMESTATE STRUCT. USED FOR MAIN GAMESTATE, AND AI SEARCHING GAMESTATES
    struct ChessState{
        //note: i'm going to try and change the arrays to bytes later. putting this comment to remind myself
        bool canCastle[4]; //{leftWhite,RightWhite,LeftBlack,RightBlack}
        bool canEnPassant[16]; //White 0-7, Black 0-7
        int halfMoves; //num of moves since the last time a pawn moved or a piece was captured (1 per player)
        int totalMoves; //num of moves total (1 per both players)
        ChessState();
    };

    //MAIN GAMESTATE VAR. TRACKS THE CURRENT GAMESTATE.
    ChessState* myState;
};