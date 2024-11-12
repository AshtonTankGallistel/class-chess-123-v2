#include "Chess.h"

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

Chess::Chess()
{
}

Chess::~Chess()
{
}

//
// make a chess piece for the player
//
Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    // depending on playerNumber load the "x.png" or the "o.png" graphic
    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("chess/") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    //
    // we want white to be at the bottom of the screen so we need to reverse the board
    //
    char piece[2];
    piece[1] = 0;
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            ImVec2 position((float)(pieceSize * x + pieceSize), (float)(pieceSize * (_gameOptions.rowY - y) + pieceSize));
            _grid[y][x].initHolder(position, "boardsquare.png", x, y);
            _grid[y][x].setGameTag(0);
            piece[0] = bitToPieceNotation(y,x);
            _grid[y][x].setNotation(piece);
        }
    }

    //setup pieces. 0=W,1=B
    //128 is added to tag if B so that we can tell later which color the piece is
    for(int i = 0; i < 2; i++){
        //pawns
        for(int x = 0; x < 8; x++){
            Bit* bit = PieceForPlayer(i, Pawn);
            bit->setPosition(_grid[1 + 5 * i][x].getPosition()); //W=1,B=6
            bit->setParent(&_grid[1 + 5 * i][x]);
            bit->setGameTag(Pawn + i * 128);
            _grid[1 + 5 * i][x].setBit(bit);
        }
        //rooks
        for(int j = 0; j < 8;j += 7){
            Bit* bit = PieceForPlayer(i, Rook);
            bit->setPosition(_grid[0 + 7 * i][j].getPosition());
            bit->setParent(&_grid[0 + 7 * i][j]);
            bit->setGameTag(Rook + i * 128);
            _grid[0 + 7 * i][j].setBit(bit);
        }
        //knights
        for(int j = 1; j < 7;j += 5){
            Bit* bit = PieceForPlayer(i, Knight);
            bit->setPosition(_grid[0 + 7 * i][j].getPosition());
            bit->setParent(&_grid[0 + 7 * i][j]);
            bit->setGameTag(Knight + i * 128);
            _grid[0 + 7 * i][j].setBit(bit);
        }
        //bishops
        for(int j = 2; j < 6;j += 3){
            Bit* bit = PieceForPlayer(i, Bishop);
            bit->setPosition(_grid[0 + 7 * i][j].getPosition());
            bit->setParent(&_grid[0 + 7 * i][j]);
            bit->setGameTag(Bishop + i * 128);
            _grid[0 + 7 * i][j].setBit(bit);
        }
        //Queen
        Bit* bit = PieceForPlayer(i, Queen);
        bit->setPosition(_grid[0 + 7 * i][3].getPosition());
        bit->setParent(&_grid[0 + 7 * i][3]);
        bit->setGameTag(Queen + i * 128);
        _grid[0 + 7 * i][3].setBit(bit);
        //King
        bit = PieceForPlayer(i, King);
        bit->setPosition(_grid[0 + 7 * i][4].getPosition());
        bit->setParent(&_grid[0 + 7 * i][4]);
        bit->setGameTag(King + i * 128);
        _grid[0 + 7 * i][4].setBit(bit);
    }
}

//
// about the only thing we need to actually fill out for tic-tac-toe
//
bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    //can only move if it's the current player's piece
    //std::cout<<getCurrentPlayer()->playerNumber()<<","<<bit.gameTag() / 128<<std::endl;
    if(getCurrentPlayer()->playerNumber() != bit.gameTag() / 128){
        return false;
    }
    //If you're reading this, can you please let me know when the below line of code was described?
    //I spent several hours straight trying to figure out what I was supposed to do here to get the bit's location,
    //and could only find this in a single answer to an unrelated question in the discord
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    bool movable = false;
    clearHighlights(); //clear highlights from prev move
    if(getPossibleMoves(bit,src,true) > 0){
        movable= true;
    }
    return movable;
}

bool Chess::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst)
{
    ChessSquare& dstSquare = static_cast<ChessSquare&>(dst);
    for (int i : possibleMoves){
        if(i == -1){ //all possbile moves seen
            break;
        }
        else if(i == dstSquare.getSquareIndex()){
            return true;
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    clearHighlights(); //clear highlights from move
    endTurn();
}

//helper function, records what moves the piece can perform into the possibleMoves variable. returns how many moves there are
int Chess::getPossibleMoves(Bit &bit, BitHolder &src, bool highlight){
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    int moveCount = 0;
    switch (bit.gameTag() % 128) // get remainder after 128 to get piece num
    {
    case Pawn:
    {
        int direction = 1;
        //0=W,1=B
        if(bit.gameTag() / 128 == 1){
            direction *= -1; //B goes down, W goes up
        }
        //add the 1 move forward(s)
        if(0 <= srcSquare.getRow() + direction && 7 >= srcSquare.getRow() + direction){
            if(_grid[srcSquare.getRow() + direction][srcSquare.getColumn()].empty()){
                possibleMoves[moveCount] = 8*(srcSquare.getRow() + direction) + srcSquare.getColumn();
                moveCount += 1;
                _grid[srcSquare.getRow() + direction][srcSquare.getColumn()].setMoveHighlighted(highlight);
            }

            //add diagonals (done here since we know there's room vertically)
            for(int i=-1;i<=1;i+=2){
                if(0 <= srcSquare.getColumn() + i && 7 >= srcSquare.getColumn() + i){
                    if(!_grid[srcSquare.getRow() + direction][srcSquare.getColumn() + i].empty()
                    && _grid[srcSquare.getRow() + direction][srcSquare.getColumn() + i].bit()->gameTag() / 128 != bit.gameTag() / 128){
                        possibleMoves[moveCount] = 8*(srcSquare.getRow() + direction) + srcSquare.getColumn() + i;
                        moveCount += 1;
                        _grid[srcSquare.getRow() + direction][srcSquare.getColumn() + i].setMoveHighlighted(highlight);
                    }
                }
            }
        }
        //add the 2 move forward
        if(srcSquare.getRow() == 1 + 5 * (bit.gameTag() / 128)){ //2 when W, 6 when B
            if(0 <= srcSquare.getRow() + 2*direction && 7 >= srcSquare.getRow() + 2*direction
                && _grid[srcSquare.getRow() + 2*direction][srcSquare.getColumn()].empty()){
                    possibleMoves[moveCount] = 8*(srcSquare.getRow() + 2*direction) + srcSquare.getColumn();
                    moveCount += 1;
                    _grid[srcSquare.getRow() + 2*direction][srcSquare.getColumn()].setMoveHighlighted(highlight);
            }
        }
    }
        break;
    case Knight:
    {
        int directions[4] = {-2,-1,1,2};
        for(int x : directions){
            for(int y : directions){
                if(abs(x) == abs(y)){ //skip when two 2s or two 1s; it should always be a 2 and a 1
                    continue;
                }
                //skip when out of range
                if(0>srcSquare.getRow() + x || 7<srcSquare.getRow() + x
                    || 0>srcSquare.getColumn() + y || 7<srcSquare.getColumn() + y){
                    continue;
                }

                if(_grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].empty()
                    || _grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                    possibleMoves[moveCount] = 8*(srcSquare.getRow() + x) + srcSquare.getColumn() + y;
                    moveCount += 1;
                    _grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].setMoveHighlighted(highlight);
                }
            }
        }
    }
        break;
    case Bishop:
    {
        //variables for readability/shorter lines
        int row = srcSquare.getRow();
        int col = srcSquare.getColumn();
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                if((x == 0 && y == 0)
                    || !(x != 0 && y != 0)){ //as a bishop, skip any combos that wouldn't give diagonals
                    continue;
                }

                for(int i = 1; i < 8; i++){
                    if(0>col+i*y || 7<col+i*y || 0>row+i*x || 7<row+i*x){ //stop when out of range
                        break;
                    }
                    else if(_grid[row+i*x][col+i*y].empty()){
                        possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                        moveCount += 1;
                        _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveCount += 1;
                            _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                        }
                        break;
                    }
                }
            }
        }
    }
        break;
    case Rook:
    {
        //variables for readability/shorter lines
        int row = srcSquare.getRow();
        int col = srcSquare.getColumn();
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                if((x == 0 && y == 0)
                    || !(x == 0 || y == 0)){ //as a rook, skip any combos that would give diagonals
                    continue;
                }

                for(int i = 1; i < 8; i++){
                    if(0>col+i*y || 7<col+i*y || 0>row+i*x || 7<row+i*x){ //stop when out of range
                        break;
                    }
                    else if(_grid[row+i*x][col+i*y].empty()){
                        possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                        moveCount += 1;
                        _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveCount += 1;
                            _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                        }
                        break;
                    }
                }
            }
        }
    }
        break;
    case Queen:
    {
        //variables for readability/shorter lines
        int row = srcSquare.getRow();
        int col = srcSquare.getColumn();
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                if((x == 0 && y == 0)){ //skip the 0,0 direction, as it's not a real direction
                    continue;
                }

                for(int i = 1; i < 8; i++){
                    if(0>col+i*y || 7<col+i*y || 0>row+i*x || 7<row+i*x){ //stop when out of range
                        break;
                    }
                    else if(_grid[row+i*x][col+i*y].empty()){
                        possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                        moveCount += 1;
                        _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveCount += 1;
                            _grid[row+i*x][col+i*y].setMoveHighlighted(highlight);
                        }
                        break;
                    }
                }
            }
        }
    }
        break;
    case King:
        for(int x = -1; x <= 1; x++){
            if(0 > srcSquare.getRow() + x || 7 < srcSquare.getRow() + x){
                continue; //skip if out of board
            }
            for(int y = -1; y <= 1; y++){
                if(0 > srcSquare.getColumn() + y || 7 < srcSquare.getColumn() + y){
                    continue; //skip if out of board
                }
                //if a neightboring spot is empty, or if enemies, then it's a valid move
                if(_grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].empty()
                    || _grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                    possibleMoves[moveCount] = 8*(srcSquare.getRow() + x) + srcSquare.getColumn() + y;
                    moveCount += 1;
                    _grid[srcSquare.getRow() + x][srcSquare.getColumn() + y].setMoveHighlighted(highlight);
                }
            }
        }
        break;
    default:
        break;
    }
    possibleMoves[moveCount] = -1; // -1 signals the end of the list; this way we avoid needing to handle memory
                                   // inefficent memory use? probably. messy? definitely. Functional? enough!
    return moveCount;
}

//helper function, clears all highlights on the board
void Chess::clearHighlights(){
    for(int i = 0; i < 64; i++){
        _grid[i/8][i%8].setMoveHighlighted(false);
    }
}

//
// free all the memory used by the game on the heap
//
void Chess::stopGame()
{
}

Player* Chess::checkForWinner()
{
    // check to see if either player has won
    return nullptr;
}

bool Chess::checkForDraw()
{
    // check to see if the board is full
    return false;
}

//
// add a helper to Square so it returns out FEN chess notation in the form p for white pawn, K for black king, etc.
// this version is used from the top level board to record moves
//
const char Chess::bitToPieceNotation(int row, int column) const {
    if (row < 0 || row >= 8 || column < 0 || column >= 8) {
        return '0';
    }

    const char* wpieces = { "?PNBRQK" };
    const char* bpieces = { "?pnbrqk" };
    unsigned char notation = '0';
    Bit* bit = _grid[row][column].bit();
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() & 127];
    } else {
        notation = '0';
    }
    return notation;
}

//
// state strings
//
std::string Chess::initialStateString()
{
    return stateString();
}

//
// this still needs to be tied into imguis init and shutdown
// we will read the state string and store it in each turn object
//
std::string Chess::stateString()
{
    std::string s;
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            s += bitToPieceNotation(y, x);
        }
    }
    return s;
}

//
// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini file and set the game state to the last saved state
//
void Chess::setStateString(const std::string &s)
{
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            int index = y * _gameOptions.rowX + x;
            int playerNumber = s[index] - '0';
            if (playerNumber) {
                _grid[y][x].setBit(PieceForPlayer(playerNumber - 1, Pawn));
            } else {
                _grid[y][x].setBit(nullptr);
            }
        }
    }
}


//
// this is the function that will be called by the AI
//
void Chess::updateAI() 
{
}

