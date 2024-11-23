#include "Chess.h"

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

Chess::Chess()
{
    //set basic gameState. may end up unnessecary depending on how it's handled elsewhere
    //This acts moreso as a safety net in case said handling is not called.
    //update: needed to rehaul it because vs failed to display the error message for a half hour. fun!
    myState = new ChessState();
}

Chess::ChessState::ChessState(){
    for(bool i : canCastle){
        i = true;
    }
    for(bool i : canEnPassant){
        i = false;
    }
    halfMoves = 0;
    totalMoves = 0;
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
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    for(bool i : myState->canCastle){
        i = true;
    }

    //generate moves for starting player
    generateMoves();
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
    //can only move if it's the current player's piece. the check for this should have been done in generateMoves()
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    bool movable = false;
    clearHighlights(); //clear highlights from prev move
    //std::cout<< potentialMoves[srcSquare.getSquareIndex()]->size() <<std::endl;
    if(potentialMoves[srcSquare.getSquareIndex()] != nullptr){
        movable= true;
        for(int i = 0; i < potentialMoves[srcSquare.getSquareIndex()]->size(); i++){
            int spot = (*potentialMoves[srcSquare.getSquareIndex()])[i];
            _grid[spot/8][spot%8].setMoveHighlighted(true);
        }
    }
    return movable;
}

bool Chess::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst)
{
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    ChessSquare& dstSquare = static_cast<ChessSquare&>(dst);
    for (int i =0; i < potentialMoves[srcSquare.getSquareIndex()]->size(); i++){
        if((*potentialMoves[srcSquare.getSquareIndex()])[i] == dstSquare.getSquareIndex()){
            return true;
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{

    //update state/promote
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    ChessSquare& dstSquare = static_cast<ChessSquare&>(dst);
    //PROMOTE
    std::cout<<dstSquare.getRow()<<std::endl;
    if(bit.gameTag() % 128 == Pawn && (dstSquare.getRow() == 0 || dstSquare.getRow() == 7)){
        setPiece(dstSquare.getSquareIndex(), bit.gameTag() / 128, Queen);
    }
    //STATE UPDATE
    //CASTLING
    //below statements always ran, since they start true and can't become true again
    switch(srcSquare.getSquareIndex()){ //when the piece moves
        //towers
        case 0:
            myState->canCastle[0] = false; break;
        case 7:
            myState->canCastle[1] = false; break;
        case 56:
            myState->canCastle[2] = false; break;
        case 63:
            myState->canCastle[3] = false; break;
        //kings
        case 4: //W
            myState->canCastle[0] = false; break;
            myState->canCastle[1] = false; break;
        case 59: //B
            myState->canCastle[2] = false; break;
            myState->canCastle[3] = false; break;
    }
    switch(dstSquare.getSquareIndex()){ //when the piece is captured
        //towers
        case 0:
            myState->canCastle[0] = false; break;
        case 7:
            myState->canCastle[1] = false; break;
        case 56:
            myState->canCastle[2] = false; break;
        case 63:
            myState->canCastle[3] = false; break;
        //kings
        case 4: //W
            myState->canCastle[0] = false; break;
            myState->canCastle[1] = false; break;
        case 59: //B
            myState->canCastle[2] = false; break;
            myState->canCastle[3] = false; break;
    }
    //wrap up turn
    clearHighlights(); //clear highlights from move
    endTurn();
    generateMoves(); //generate moves for new current player
}

//helper function, records what moves the piece can perform into the possibleMoves variable.
//returns how many moves there are.
std::vector<int>* Chess::getPossibleMoves(Bit &bit, BitHolder &src){
    ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
    int moveCount = 0;
    std::vector<int>* moveList = new std::vector<int>;
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
                moveList->push_back(8*(srcSquare.getRow() + direction) + srcSquare.getColumn());
                moveCount += 1;
            }

            //add diagonals (done here since we know there's room vertically)
            for(int i=-1;i<=1;i+=2){
                if(0 <= srcSquare.getColumn() + i && 7 >= srcSquare.getColumn() + i){
                    if(!_grid[srcSquare.getRow() + direction][srcSquare.getColumn() + i].empty()
                    && _grid[srcSquare.getRow() + direction][srcSquare.getColumn() + i].bit()->gameTag() / 128 != bit.gameTag() / 128){
                        possibleMoves[moveCount] = 8*(srcSquare.getRow() + direction) + srcSquare.getColumn() + i;
                        moveList->push_back(8*(srcSquare.getRow() + direction) + srcSquare.getColumn() + i);
                        moveCount += 1;
                    }
                }
            }
        }
        //add the 2 move forward
        if(srcSquare.getRow() == 1 + 5 * (bit.gameTag() / 128)){ //2 when W, 6 when B
            if(0 <= srcSquare.getRow() + 2*direction && 7 >= srcSquare.getRow() + 2*direction
                && _grid[srcSquare.getRow() + 2*direction][srcSquare.getColumn()].empty()){
                    possibleMoves[moveCount] = 8*(srcSquare.getRow() + 2*direction) + srcSquare.getColumn();
                    moveList->push_back(8*(srcSquare.getRow() + 2*direction) + srcSquare.getColumn());
                    moveCount += 1;
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
                    moveList->push_back(8*(srcSquare.getRow() + x) + srcSquare.getColumn() + y);
                    moveCount += 1;
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
                        moveList->push_back(8*(row+i*x) + col+i*y);
                        moveCount += 1;
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveList->push_back(8*(row+i*x) + col+i*y);
                            moveCount += 1;
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
                        moveList->push_back(8*(row+i*x) + col+i*y);
                        moveCount += 1;
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveList->push_back(8*(row+i*x) + col+i*y);
                            moveCount += 1;
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
                        moveList->push_back(8*(row+i*x) + col+i*y);
                        moveCount += 1;
                    }
                    else{ //not empty, will break
                        //add move if the non-empty spot has an enemy piece
                        if(_grid[row+i*x][col+i*y].bit()->gameTag() / 128 != bit.gameTag() / 128){
                            possibleMoves[moveCount] = 8*(row+i*x) + col+i*y;
                            moveList->push_back(8*(row+i*x) + col+i*y);
                            moveCount += 1;
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
                    moveList->push_back(8*(srcSquare.getRow() + x) + srcSquare.getColumn() + y);
                    moveCount += 1;
                }
            }
        }
        /*
        //consider castling
        if(myState->canCastle[bit.gameTag() / 128]){ //left castle
            int i = srcSquare.getSquareIndex(); //i = index. used for readability
            //if all 3 between pieces are empty, can castle
            if(_grid[(i-1)/8][(i-1)%8].empty() && _grid[(i-2)/8][(i-2)%8].empty() && _grid[(i-3)/8][(i-3)%8].empty()){
                moveList->push_back(8*(srcSquare.getRow()) + srcSquare.getColumn() - 2);
            }
        
        }
        if(myState->canCastle[bit.gameTag() / 128 + 1]){ //right castle
            int i = srcSquare.getSquareIndex(); //i = index. used for readability
            //if all 3 between pieces are empty, can castle
            if(_grid[(i+1)/8][(i+1)%8].empty() && _grid[(i+2)/8][(i+2)%8].empty()){
                moveList->push_back(8*(srcSquare.getRow()) + srcSquare.getColumn() + 2);
            }
        }
        */
        break;
    default:
        printf("default");
        break;
    }
    possibleMoves[moveCount] = -1; // -1 signals the end of the list; this way we avoid needing to handle memory
                                   // inefficent memory use? probably. messy? definitely. Functional? enough!
    
    std::vector<int>* moveListLocation = moveList;
    return moveListLocation;
}

//helper function, clears all highlights on the board
void Chess::clearHighlights(){
    for(int i = 0; i < 64; i++){
        _grid[i/8][i%8].setMoveHighlighted(false);
    }
}

//The below function generates all possible moves that can be chosen by the player this turn
//The moves are stored as an std::vector of vectors. Each spot is tied to a spot on the board (starting from spot 0 to 64),
//And the value stored is a (pointer to) std::vector of ints listing all possible spots it can move to.
//If there's no spots, value=nullptr. This is all stored in class var potentialMoves
void Chess::generateMoves(){
    //potentialMoves setup in Chess constructor. All spots will be replaced, so no need to call a clear func
    for(int i = 0;i<64;i++){
        if(_grid[i/8][i%8].empty()){ //if empty, no moves
            potentialMoves[i] = nullptr;
        } //if not player's piece, no moves
        else if(getCurrentPlayer()->playerNumber() != _grid[i/8][i%8].bit()->gameTag() / 128){
            potentialMoves[i] = nullptr;
        } //if neither, then it's the player's piece, and could make a move
        else{
            std::vector<int>* myMoveList = nullptr;
            myMoveList = getPossibleMoves(*_grid[i/8][i%8].bit(),_grid[i/8][i%8]);
            if(myMoveList->size() == 0){ //if piece cannot move, nullptr
                potentialMoves[i] = nullptr;
            }
            else{ //if piece can move, add the list for that spot
                //std::cout<<i<<std::endl;
                potentialMoves[i] = myMoveList;
            }
        }
    }
}

//function for setting the board based on FEN string
void Chess::FENtoBoard(std::string FEN){
    int gameState = -1; //tracks if we need to handle gameState info from FEN.
                       //-1 means we don't, otherwise it correlates to the pos in the string with gameState
    int boardPos = 0; //ties to the position on the board of the next piece.
    for(int i = 0; i < FEN.length(); i++){
        //EMPTY SPACE HANDLER
        if(48 <= FEN[i] && FEN[i] <=57){
            boardPos += FEN[i] - 48; // FEN - 48 so as to change the char into it's correlated int value
            continue;
        }
        if(FEN[i] == 47){ // 47 = / . Represents a transition to the next line. not needed here
            continue;
        }
        if(FEN[i] == 32){ // 32 = space. Represents when board position starts.
            gameState = i;
            break; //break so as to have seperate loop handle it.
        }
        
        //upper = white = 0. all upper chars have val < 95, so remainder = player num
        ChessPiece myPiece;
        switch(FEN[i] % 32){ //char's num value. doing overcomplicated math to get both upper/lowercase
            case 16: // p = 80, 112
                myPiece = Pawn;
                break;
            case 18: // r = 82, 114
                myPiece = Rook;
                break;
            case 14: // n = 78, 110
                myPiece = Knight;
                break;
            case 2: // b = 66, 98
                myPiece = Bishop;
                break;
            case 17: // q = 81, 113
                myPiece = Queen;
                break;
            case 11: // k = 75, 107
                myPiece = King;
                break;
            default:
                std::cout <<"error: improper Fen input" <<std::endl;
                break;
        }
        //equation needed since FEN starts at top left, but ours starts at bottom left.
        int position = (8 - boardPos/8)*8 - (8 - boardPos%8);
        // FEN / 95 since < 95 means 0 meaning W, and >95 means 1 meaning B
        Chess::setPiece(position, (int)FEN[i] / 95, myPiece);
        boardPos += 1; //increment for next pos
    }

    //set default game state. some may be overriden, and we can't fully tell which will/won't as we go.
    //As such, all are set to false to be safe. These can be overriden manually after calling this if needed
    for(bool i : myState->canCastle){
        i = false;
    }
    for(bool i : myState->canEnPassant){
        i = false;
    }
    myState->halfMoves = 0;
    myState->totalMoves = 0;

    //handle board pos if needed (sorry this is so long...)
    if(gameState != -1){
        gameState++; //move from space to player
        //CURRENT PLAYER
        //ok ok. hear me out. there's no dedicated way to manually set the player.
        //but b always goes second. and this should be called at the start of a game. so...
        if(gameState == 'b'){
            endTurn(); //if there's a better way to do this, i apologize.
        }
        gameState += 2; //skip to castle info.
        //CASTLING
        while(FEN[gameState] != ' '){
            switch(FEN[gameState]){
                case 'Q':
                    myState->canCastle[0] = true; break;
                case 'K':
                    myState->canCastle[1] = true; break;
                case 'q':
                    myState->canCastle[2] = true; break;
                case 'k':
                    myState->canCastle[3] = true; break;
                default: break;
            }
            gameState++;
        }
        gameState++;
        //EN PASSANT
        if(FEN[gameState] != '-'){
            while(FEN[gameState] != ' '){
                //for each pair (pair= char and number) get the position in enPas array
                int enPasPosition = FEN[gameState] - 97; //get pos in 8 spots
                //determine if w or b
                if(FEN[gameState] == 54){ //54 = lane 6 = black piece
                    enPasPosition += 8; //set to latter half since
                }
                myState->canEnPassant[enPasPosition] = true;
                gameState += 2;
            }
        }
        else{
            gameState += 2;
        }
        //MOVE COUNTERS
        while(FEN[gameState] != '-' && FEN[gameState] != ' '){
            myState->halfMoves *= 10;
            myState->halfMoves = FEN[gameState] - 48;
            gameState++;
        }
        gameState++;
        if(FEN[gameState] != ' '){ //if last was a -, you'd still be on a _, so skip forward 1
            gameState++;
        }
        myState->totalMoves = FEN[gameState] - 48;
        while(gameState < FEN.length()){
            myState->halfMoves *= 10;
            myState->halfMoves = FEN[gameState] - 48;
            gameState++;
        }
        //ok so you know what i said about setting turns earlier? how there's no dedicated way to set them?
        //I was going to do a loop to add the num of turns accounted for above.
        //but, that would likely result in a lot of loading time. Additionally, afaik, other than player...
        //nothing actually needs the num of turns, outside of it being tracked in gamestate.
        //as such, I'm not adding a loop here, unless it turns out I need to later. sry for the wall of text
    }
}

//helper funciton. runs the needed code to place a piece on the board.
void Chess::setPiece(int pos, int player, ChessPiece piece){
    Bit* bit = PieceForPlayer(player, piece);
    bit->setPosition(_grid[pos / 8][pos % 8].getPosition());
    bit->setParent(&_grid[pos / 8][pos % 8]);
    bit->setGameTag(piece + player * 128);
    _grid[pos / 8][pos % 8].setBit(bit);
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

