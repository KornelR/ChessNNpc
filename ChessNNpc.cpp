#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
//#include <wrl.h>

#pragma comment(lib, "D3D11.lib")

//D311
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int chessBoard[8][8];
bool isChessSquareWhite[8][8] = 
{
    {true,false,true,false,true,false,true,false},
    {false,true,false,true,false,true,false,true},
    {true,false,true,false,true,false,true,false},
    {false,true,false,true,false,true,false,true},
    {true,false,true,false,true,false,true,false},
    {false,true,false,true,false,true,false,true},
    {true,false,true,false,true,false,true,false},
    {false,true,false,true,false,true,false,true},
};
static int windowHeight = 700;
static int windowWidth = 830;
bool isGameStarted = false;
bool isChessSquareViableMove[8][8];
bool isChessPieceSelected = false;
int selectedPieceX = 0;
int selectedPieceY = 0;
bool isHumanWhite = true;
const int fenLength = 100;
char FENinput[fenLength] = "";
bool isHumanPlayingAgainstAI = false;
bool isAITurnedOn = false;
bool isWhitesTurn = true;
bool gameEnded = false;
bool didHumanWin = false;
bool isItDraw = false;
bool isAnyMoveViable = true;
bool isWhiteKingCastlingPossible = true;
bool isWhiteQueenCastlingPossible = true;
bool isBlackKingCastlingPossible = true;
bool isBlackQueenCastlingPossible = true;
bool isEnPassantForWhitePossible = false;
bool isEnPassantForBlackPossible = false;
int enPassantX = 0;

bool dangerSquaresForWhiteKing[8][8];
bool isWhiteKingInCheck = false;
bool dangerSquaresForBlackKing[8][8];
bool isBlackKingInCheck = false;
int hypotheticalBoard[8][8];
int hypotheticalBoardForWhiteKing[8][8];
int hypotheticalBoardForBlackKing[8][8];
bool isMoveHypothetical = false;
int moveNumber = 0;

//White 0 ;=; Black 1
//Pawn +10 +11
//Knight +30 +31
//Bishop +32 +33
//Rook +50 +51
//Queen +90 +91
//King +100 + 101
// 
//51 31 33 91 101 33 31 51
//11 11 11 11  11 11 11 11
// 0  0  0  0   0  0  0  0
// 0  0  0  0   0  0  0  0
// 0  0  0  0   0  0  0  0
// 0  0  0  0   0  0  0  0
//10 10 10 10  10 10 10 10
//50 30 32 90 100 32 30 50

//Chess app functions
static void boardBeginWhite()
{
    chessBoard[0][0] = 51 , chessBoard[0][1] = 31, chessBoard[0][2] = 33, chessBoard[0][3] = 91;
    chessBoard[0][4] = 101, chessBoard[0][5] = 33, chessBoard[0][6] = 31, chessBoard[0][7] = 51;
    for (int i = 0; i < 8; i++)
    {
        chessBoard[1][i] = 11;
        chessBoard[2][i] = 0;
        chessBoard[3][i] = 0;
        chessBoard[4][i] = 0;
        chessBoard[5][i] = 0;
        chessBoard[6][i] = 10;
    }
    chessBoard[7][0] = 50 , chessBoard[7][1] = 30, chessBoard[7][2] = 32, chessBoard[7][3] = 90;
    chessBoard[7][4] = 100, chessBoard[7][5] = 32, chessBoard[7][6] = 30, chessBoard[7][7] = 50;
    isHumanWhite = true;
}

static void boardReset()
{
    for (int w = 0; w < 8; w++)
    {
        for (int h = 0; h < 8; h++)
        {
            chessBoard[h][w] = 0;
        }
    }
}

static void resetisChessSquareViableMove()
{
    selectedPieceX = 0;
    selectedPieceY = 0;
    isChessPieceSelected = false;
    for (int w = 0; w < 8; w++)
    {
        for (int h = 0; h < 8; h++)
        {
            isChessSquareViableMove[h][w] = false;
        }
    }
}

static void boardFlip()
{
    int tempChessBoard[8][8];
    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            tempChessBoard[h][w] = chessBoard[7-h][7-w];
        }
    }

    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            chessBoard[h][w] = tempChessBoard[h][w];
        }
    }

    if (isHumanWhite == true) 
    { 
        isHumanWhite = false; 
    }
    else if (isHumanWhite == false) 
    {
        isHumanWhite = true; 
    }

    resetisChessSquareViableMove();
}

static void testingBoardOut()
{
    std::cout << "\n";
    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            std::cout << chessBoard[h][w] << "--";
        }
        std::cout << "\n";
    }
}

static void fenToBoardPosition(char FenPos[fenLength])
{
    boardReset();
    int chessBoardFENX = 0;
    int chessBoardFENY = 0;

    int i = 0;
    for (; i < fenLength && FenPos[i] != ' '; i++)
    {
        switch (FenPos[i])
        {
        default:
            chessBoardFENX++;
            break;
        case '/':
            chessBoardFENX = 0;
            chessBoardFENY++;
            break;
        case '1':
            chessBoardFENX += 1;
            break;
        case '2':
            chessBoardFENX += 2;
            break;
        case '3':
            chessBoardFENX += 3;
            break;
        case '4':
            chessBoardFENX += 4;
            break;
        case '5':
            chessBoardFENX += 5;
            break;
        case '6':
            chessBoardFENX += 6;
            break;
        case '7':
            chessBoardFENX += 7;
            break;
        case '8':
            chessBoardFENX += 8;
            break;
        case 'P':
            chessBoard[chessBoardFENY][chessBoardFENX] = 10;
            chessBoardFENX++;
            break;
        case 'p':
            chessBoard[chessBoardFENY][chessBoardFENX] = 11;
            chessBoardFENX++;
            break;
        case 'N':
            chessBoard[chessBoardFENY][chessBoardFENX] = 30;
            chessBoardFENX++;
            break;
        case 'n':
            chessBoard[chessBoardFENY][chessBoardFENX] = 31;
            chessBoardFENX++;
            break;
        case 'B':
            chessBoard[chessBoardFENY][chessBoardFENX] = 32;
            chessBoardFENX++;
            break;
        case 'b':
            chessBoard[chessBoardFENY][chessBoardFENX] = 33;
            chessBoardFENX++;
            break;
        case 'R':
            chessBoard[chessBoardFENY][chessBoardFENX] = 50;
            chessBoardFENX++;
            break;
        case 'r':
            chessBoard[chessBoardFENY][chessBoardFENX] = 51;
            chessBoardFENX++;
            break;
        case 'Q':
            chessBoard[chessBoardFENY][chessBoardFENX] = 90;
            chessBoardFENX++;
            break;
        case 'q':
            chessBoard[chessBoardFENY][chessBoardFENX] = 91;
            chessBoardFENX++;
            break;
        case 'K':
            chessBoard[chessBoardFENY][chessBoardFENX] = 100;
            chessBoardFENX++;
            break;
        case 'k':
            chessBoard[chessBoardFENY][chessBoardFENX] = 101;
            chessBoardFENX++;
            break;
        }
    }
    if (FenPos[i + 1] == 'b')
    {
        isWhitesTurn = false;
    }
    i += 3;

    for (; i < fenLength && FenPos[i] != ' '; i++)
    {
        switch (FenPos[i])
        {

        case 'K':
            isWhiteKingCastlingPossible = true;
            break;
        case 'Q':
            isWhiteQueenCastlingPossible = true;
            break;
        case 'k':
            isBlackKingCastlingPossible = true;
            break;
        case 'q':
            isBlackQueenCastlingPossible = true;
            break;
        case '-':
            isWhiteKingCastlingPossible = false;
            isWhiteQueenCastlingPossible = false;
            isBlackKingCastlingPossible = false;
            isBlackQueenCastlingPossible = false;
            break;

        }
    }
    i++;
    switch (FenPos[i])
    {
    case '-':
        break;
    case 'e':
        isEnPassantForBlackPossible = true;
        i++;
        enPassantX = FenPos[i] - '0';
        break;
    case 'c':
        isEnPassantForWhitePossible = true;
        i++;
        enPassantX = FenPos[i] - '0';
        break;
    }
}

static void updatePawnToQueenPromotion()
{
    if (isHumanWhite == true)
    {
        for (int w = 0; w < 8; w++)
        {
            if (chessBoard[0][w] == 10)
            {
                chessBoard[0][w] = 90;
            }

            if (chessBoard[7][w] == 11)
            {
                chessBoard[7][w] = 91;
            }
        }
    }
    else
    {
        for (int w = 0; w < 8; w++)
        {
            if (chessBoard[7][w] == 10)
            {
                chessBoard[7][w] = 90;
            }

            if (chessBoard[0][w] == 11)
            {
                chessBoard[0][w] = 91;
            }
        }
    }
}

static void makeHypotheticalMove(int fromY, int fromX, int toY, int toX)
{
    isMoveHypothetical = true;

    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            hypotheticalBoard[h][w] = chessBoard[h][w];
        }
    }
    hypotheticalBoard[toY][toX] = hypotheticalBoard[fromY][fromX];
    hypotheticalBoard[fromY][fromX] = 0;
}

static void resetDangerSquares()
{
    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            dangerSquaresForBlackKing[h][w] = false;
            dangerSquaresForWhiteKing[h][w] = false;
        }
    }
}

static void updateDangerSquares()
{
    int temporaryChessBoard[8][8];

    if (isMoveHypothetical == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = chessBoard[h][w];
            }
        }
    }
    else
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = hypotheticalBoard[h][w];
            }
        }
    }

    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            if (temporaryChessBoard[h][w] != 0)
            {
//Danger Squares for Black (white pieces)
                    switch (temporaryChessBoard[h][w])
                    {
                    case 10:
                        if (isHumanWhite == true)
                        {
                            if (w != 0 && h != 0)
                            {
                                dangerSquaresForBlackKing[h - 1][w - 1] = true;
                            }
                            if (w != 7 && h != 0)
                            {
                                dangerSquaresForBlackKing[h - 1][w + 1] = true;
                            }
                        }
                        else
                        {
                            if (w != 0 && h != 7)
                            {
                                dangerSquaresForBlackKing[h + 1][w - 1] = true;
                            }

                            if (w != 7 && h != 7)
                            {                         
                                dangerSquaresForBlackKing[h + 1][w + 1] = true;
                            }
                        }
                        break;

                    case 30:
                        if (h > 1 && w < 7)
                        {
                            dangerSquaresForBlackKing[h - 2][w + 1] = true;
                        }
                        if (h > 1 && w > 0)
                        {
                            dangerSquaresForBlackKing[h - 2][w - 1] = true;
                        }
                        if (h > 0 && w > 1)
                        {
                            dangerSquaresForBlackKing[h - 1][w - 2] = true;
                        }
                        if (h < 7 && w > 1)
                        {
                            dangerSquaresForBlackKing[h + 1][w - 2] = true;
                        }
                        if (h < 6 && w > 0)
                        {
                            dangerSquaresForBlackKing[h + 2][w - 1] = true;
                        }
                        if (h < 6 && w < 7)
                        {
                            dangerSquaresForBlackKing[h + 2][w + 1] = true;
                        }
                        if (h < 7 && w < 6)
                        {
                            dangerSquaresForBlackKing[h + 1][w + 2] = true;
                        }
                        if (h > 0 && w < 6)
                        {
                            dangerSquaresForBlackKing[h - 1][w + 2] = true;
                        }
                        break;

                    case 32:
                    {
                        int tempX = w + 1;
                        int tempY = h - 1;
                        while (tempY >= 0 && tempX <= 7)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h - 1;
                        while (tempY >= 0 && tempX >= 0)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX >= 0)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY++;
                        }
                        tempX = w + 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX <= 7)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY++;
                        }
                        break;
                    }

                    case 90:
                    {
                        int tempX = w + 1;
                        int tempY = h - 1;
                        while (tempY >= 0 && tempX <= 7)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h - 1;
                        while (tempY >= 0 && tempX >= 0)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX >= 0)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY++;
                        }
                        tempX = w + 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX <= 7)
                        {
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY++;
                        }

                        {
                            int tempY = h - 1;
                            while (tempY >= 0)
                            {
                                if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                    break;
                                }
                                if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                    break;
                                }

                                if (temporaryChessBoard[tempY][w] == 0)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                }
                                tempY--;
                            }
                            tempY = h + 1;

                            while (tempY <= 7)
                            {
                                if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                    break;
                                }
                                if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                    break;
                                }

                                if (temporaryChessBoard[tempY][w] == 0)
                                {
                                    dangerSquaresForBlackKing[tempY][w] = true;
                                }
                                tempY++;
                            }

                            int tempX = w - 1;
                            while (tempX >= 0)
                            {
                                if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                    break;
                                }
                                if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                    break;
                                }

                                if (temporaryChessBoard[h][tempX] == 0)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                }
                                tempX--;
                            }
                            tempX = w + 1;

                            while (tempX <= 7)
                            {
                                if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                    break;
                                }
                                if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                    break;
                                }

                                if (temporaryChessBoard[h][tempX] == 0)
                                {
                                    dangerSquaresForBlackKing[h][tempX] = true;
                                }
                                tempX++;
                            }
                        }
                        break;
                    }

                    case 50:
                    {
                        int tempY = h - 1;
                        while (tempY >= 0)
                        {
                            if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][w] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                            }
                            tempY--;
                        }
                        tempY = h + 1;

                        while (tempY <= 7)
                        {
                            if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                                break;
                            }
                            if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][w] == 0)
                            {
                                dangerSquaresForBlackKing[tempY][w] = true;
                            }
                            tempY++;
                        }

                        int tempX = w - 1;
                        while (tempX >= 0)
                        {
                            if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[h][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                            }
                            tempX--;
                        }
                        tempX = w + 1;

                        while (tempX <= 7)
                        {
                            if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                                break;
                            }
                            if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[h][tempX] == 0)
                            {
                                dangerSquaresForBlackKing[h][tempX] = true;
                            }
                            tempX++;
                        }
                        break;
                    }

                    case 100:
                    {
                        if (h > 0 && w < 7)
                        {
                            dangerSquaresForBlackKing[h - 1][w + 1] = true;
                        }
                        if (h > 0)
                        {
                            dangerSquaresForBlackKing[h - 1][w] = true;
                        }
                        if (h > 0 && w > 0)
                        {
                            dangerSquaresForBlackKing[h - 1][w - 1] = true;
                        }
                        if (w > 0)
                        {
                            dangerSquaresForBlackKing[h][w - 1] = true;
                        }
                        if (h < 7 && w > 0)
                        {
                            dangerSquaresForBlackKing[h + 1][w - 1] = true;
                        }
                        if (h < 7)
                        {
                            dangerSquaresForBlackKing[h + 1][w] = true;
                        }
                        if (h < 7 && w < 7)
                        {
                            dangerSquaresForBlackKing[h + 1][w + 1] = true;
                        }
                        if (w < 7)
                        {
                            dangerSquaresForBlackKing[h][w + 1] = true;
                        }
                        break;
                    }
//Danger Squares for White (black pieces)
                    case 11:
                        if (isHumanWhite == false)
                        {
                            if (w != 0 && h != 0)
                            {
                                dangerSquaresForWhiteKing[h - 1][w - 1] = true;
                            }
                            if (w != 7 && h != 0) {
                                dangerSquaresForWhiteKing[h - 1][w + 1] = true;
                            }
                        }
                        else
                        {
                            if (w != 0 && h != 7)
                            {
                                dangerSquaresForWhiteKing[h + 1][w - 1] = true;
                            }
                            if (w != 7 && h != 7)
                            {
                                dangerSquaresForWhiteKing[h + 1][w + 1] = true;
                            }
                        }
                        break;

                    case 31:
                        if (h > 1 && w < 7)
                        {
                            dangerSquaresForWhiteKing[h - 2][w + 1] = true;
                        }
                        if (h > 1 && w > 0)
                        {
                            dangerSquaresForWhiteKing[h - 2][w - 1] = true;
                        }
                        if (h > 0 && w > 1)
                        {
                            dangerSquaresForWhiteKing[h - 1][w - 2] = true;
                        }
                        if (h < 7 && w > 1)
                        {
                            dangerSquaresForWhiteKing[h + 1][w - 2] = true;
                        }
                        if (h < 6 && w > 0)
                        {
                            dangerSquaresForWhiteKing[h + 2][w - 1] = true;
                        }
                        if (h < 6 && w < 7)
                        {
                            dangerSquaresForWhiteKing[h + 2][w + 1] = true;
                        }
                        if (h < 7 && w < 6)
                        {
                            dangerSquaresForWhiteKing[h + 1][w + 2] = true;
                        }
                        if (h > 0 && w < 6)
                        {
                            dangerSquaresForWhiteKing[h - 1][w + 2] = true;
                        }
                        break;

                    case 33:
                    {
                        int tempX = w + 1;
                        int tempY = h - 1;
                        while (tempY >= 0 && tempX <= 7)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h - 1;
                        while (tempY >= 0 && tempX >= 0)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX >= 0)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY++;
                        }
                        tempX = w + 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX <= 7)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY++;
                        }
                        break;
                    }
                    case 91:
                    {
                        int tempX = w + 1;
                        int tempY = h - 1;
                        while (tempY >= 0 && tempX <= 7)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h - 1;
                        while (tempY >= 0 && tempX >= 0)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY--;
                        }
                        tempX = w - 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX >= 0)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX--;
                            tempY++;
                        }
                        tempX = w + 1;
                        tempY = h + 1;
                        while (tempY <= 7 && tempX <= 7)
                        {
                            if (((temporaryChessBoard[tempY][tempX] % 10) == 0 && temporaryChessBoard[tempY][tempX] != 0) || (temporaryChessBoard[tempY][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][tempX] % 10) == 1 || (temporaryChessBoard[tempY][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][tempX] = true;
                            }
                            tempX++;
                            tempY++;
                        }

                        {
                            int tempY = h - 1;
                            while (tempY >= 0)
                            {
                                if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                    break;
                                }
                                if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                    break;
                                }

                                if (temporaryChessBoard[tempY][w] == 0)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                }
                                tempY--;
                            }
                            tempY = h + 1;
                            while (tempY <= 7)
                            {
                                if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                    break;
                                }
                                if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                    break;
                                }

                                if (temporaryChessBoard[tempY][w] == 0)
                                {
                                    dangerSquaresForWhiteKing[tempY][w] = true;
                                }
                                tempY++;
                            }
                            int tempX = w - 1;
                            while (tempX >= 0)
                            {
                                if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                    break;
                                }
                                if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                    break;
                                }

                                if (temporaryChessBoard[h][tempX] == 0)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                }
                                tempX--;
                            }
                            tempX = w + 1;
                            while (tempX <= 7)
                            {
                                if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                    break;
                                }
                                if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                    break;
                                }

                                if (temporaryChessBoard[h][tempX] == 0)
                                {
                                    dangerSquaresForWhiteKing[h][tempX] = true;
                                }
                                tempX++;
                            }
                        }
                        break;
                    }

                    case 51:
                    {
                        int tempY = h - 1;
                        while (tempY >= 0)
                        {
                            if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][w] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                            }
                            tempY--;
                        }
                        tempY = h + 1;
                        while (tempY <= 7)
                        {
                            if (((temporaryChessBoard[tempY][w] % 10) == 0 && temporaryChessBoard[tempY][w] != 0) || (temporaryChessBoard[tempY][w] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                                break;
                            }
                            if ((temporaryChessBoard[tempY][w] % 10) == 1 || (temporaryChessBoard[tempY][w] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                                break;
                            }

                            if (temporaryChessBoard[tempY][w] == 0)
                            {
                                dangerSquaresForWhiteKing[tempY][w] = true;
                            }
                            tempY++;
                        }
                        int tempX = w - 1;
                        while (tempX >= 0)
                        {
                            if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[h][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                            }
                            tempX--;
                        }
                        tempX = w + 1;
                        while (tempX <= 7)
                        {
                            if (((temporaryChessBoard[h][tempX] % 10) == 0 && temporaryChessBoard[h][tempX] != 0) || (temporaryChessBoard[h][tempX] % 10) == 2)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                                break;
                            }
                            if ((temporaryChessBoard[h][tempX] % 10) == 1 || (temporaryChessBoard[h][tempX] % 10) == 3)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                                break;
                            }

                            if (temporaryChessBoard[h][tempX] == 0)
                            {
                                dangerSquaresForWhiteKing[h][tempX] = true;
                            }
                            tempX++;
                        }
                        break;
                    }


                    case 101:
                    {
                        if (h > 0 && w < 7)
                        {
                            dangerSquaresForWhiteKing[h - 1][w + 1] = true;
                        }
                        if (h > 0)
                        {
                            dangerSquaresForWhiteKing[h - 1][w] = true;
                        }
                        if (h > 0 && w > 0)
                        {
                            dangerSquaresForWhiteKing[h - 1][w - 1] = true;
                        }
                        if (w > 0)
                        {
                            dangerSquaresForWhiteKing[h][w - 1] = true;
                        }
                        if (h < 7 && w > 0)
                        {
                            dangerSquaresForWhiteKing[h + 1][w - 1] = true;
                        }
                        if (h < 7)
                        {
                            dangerSquaresForWhiteKing[h + 1][w] = true;
                        }
                        if (h < 7 && w < 7)
                        {
                            dangerSquaresForWhiteKing[h + 1][w + 1] = true;
                        }
                        if (w < 7)
                        {
                            dangerSquaresForWhiteKing[h][w + 1] = true;
                        }
                        break;
                    }
                }
            }
        }
    }
}

static void updateIsKingInCheck()
{
    resetDangerSquares();
    updateDangerSquares();

    int temporaryChessBoard[8][8];

    if (isMoveHypothetical == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = chessBoard[h][w];
            }
        }
    }
    else
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = hypotheticalBoard[h][w];
            }
        }
    }

    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            if (temporaryChessBoard[h][w] == 100)
            {
                if (dangerSquaresForWhiteKing[h][w] == true)
                {
                    isWhiteKingInCheck = true;
                }
                else
                {
                    isWhiteKingInCheck = false;
                }
            }

            if (temporaryChessBoard[h][w] == 101)
            {
                if (dangerSquaresForBlackKing[h][w] == true)
                {
                    isBlackKingInCheck = true;
                }
                else
                {
                    isBlackKingInCheck = false;
                }
            }
        }
    }
}

static void updateCastlingWhenRookIsCaptured()
{
    if (isWhiteKingCastlingPossible == true)
    {
        if (isHumanWhite == true)
        {
            if (chessBoard[7][7] != 50)
            {
                isWhiteKingCastlingPossible = false;
            }
        }
        else
        {
            if (chessBoard[0][0] != 50)
            {
                isWhiteKingCastlingPossible = false;
            }
        }
    }
    if (isWhiteQueenCastlingPossible == true)
    {
        if (isHumanWhite == true)
        {
            if (chessBoard[7][0] != 50)
            {
                isWhiteQueenCastlingPossible = false;
            }
        }
        else
        {
            if (chessBoard[0][7] != 50)
            {
                isWhiteQueenCastlingPossible = false;
            }
        }
    }

    if (isBlackKingCastlingPossible == true)
    {
        if (isHumanWhite == true)
        {
            if (chessBoard[0][7] != 51)
            {
                isBlackKingCastlingPossible = false;
            }
        }
        else
        {
            if (chessBoard[7][0] != 51)
            {
                isBlackKingCastlingPossible = false;
            }
        }
    }
    if (isBlackQueenCastlingPossible == true)
    {
        if (isHumanWhite == true)
        {
            if (chessBoard[0][0] != 51)
            {
                isBlackQueenCastlingPossible = false;
            }
        }
        else
        {
            if (chessBoard[7][7] != 51)
            {
                isBlackQueenCastlingPossible = false;
            }
        }
    }
}

static void updateViableMoves()
{
    updateIsKingInCheck();
    updateCastlingWhenRookIsCaptured();

    if (isChessPieceSelected == true)
    {
        switch (chessBoard[selectedPieceY][selectedPieceX])
        {
//White Pawn Movement:
        case 10:
            if (isHumanWhite == true)
            {
                //Double move
                if (selectedPieceY == 6 && chessBoard[selectedPieceY - 2][selectedPieceX] == 0 && chessBoard[selectedPieceY - 1][selectedPieceX] == 0)
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Single move
                if (chessBoard[selectedPieceY - 1][selectedPieceX] == 0) 
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false; 
                }
                //Attacking
                if (selectedPieceX != 0)
                {
                    if (chessBoard[selectedPieceY - 1][selectedPieceX - 1] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) != 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForWhitePossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
                if (selectedPieceX != 7) {
                    if (chessBoard[selectedPieceY - 1][selectedPieceX + 1] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) != 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForWhitePossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }

            }
            else
            {
                //Double move
                if (selectedPieceY == 1 && chessBoard[selectedPieceY + 2][selectedPieceX] == 0 && chessBoard[selectedPieceY + 1][selectedPieceX] == 0)
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Single move
                if (chessBoard[selectedPieceY + 1][selectedPieceX] == 0) 
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Attacking
                if (selectedPieceX != 0)
                {
                    if (chessBoard[selectedPieceY + 1][selectedPieceX - 1] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) != 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForWhitePossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 4)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
                if (selectedPieceX != 7) {
                    if (chessBoard[selectedPieceY + 1][selectedPieceX + 1] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) != 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForWhitePossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 4)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
            }
            break;
//Black Pawn Movement
        case 11:
            if (isHumanWhite == false)
            {
                //Double move
                if (selectedPieceY == 6 && chessBoard[selectedPieceY - 2][selectedPieceX] == 0 && chessBoard[selectedPieceY - 1][selectedPieceX] == 0)
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Single move
                if (chessBoard[selectedPieceY - 1][selectedPieceX] == 0) 
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Attacking
                if (selectedPieceX != 0)
                {
                    if (chessBoard[selectedPieceY - 1][selectedPieceX - 1] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) != 1 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) != 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForBlackPossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
                if (selectedPieceX != 7) {
                    if (chessBoard[selectedPieceY - 1][selectedPieceX + 1] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) != 1 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) != 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForBlackPossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
            }
            else
            {
                //Double move
                if (selectedPieceY == 1 && chessBoard[selectedPieceY + 2][selectedPieceX] == 0 && chessBoard[selectedPieceY + 1][selectedPieceX] == 0 )
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Single move
                if (chessBoard[selectedPieceY + 1][selectedPieceX] == 0) 
                { 
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                //Attacking
                if (selectedPieceX != 0)
                {
                    if (chessBoard[selectedPieceY + 1][selectedPieceX - 1] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) != 1 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) != 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForBlackPossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 4)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
                if (selectedPieceX != 7) {
                    if (chessBoard[selectedPieceY + 1][selectedPieceX + 1] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) != 1 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) != 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    //EnPassant
                    if (isEnPassantForBlackPossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 4)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                        }
                        isMoveHypothetical = false;
                    }
                }
            }
            break;
//White Knight Movement from Upper Right Corner Counterclockwise
        case 30: 
            if (selectedPieceY > 1 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1] % 10) == 1 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 1 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1] % 10) == 1 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX - 2] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 2);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX - 2] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 2);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1] % 10) == 1 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1] % 10) == 1 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX + 2] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 2);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX + 2] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 2);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            break;
//Black Knight Movement from Upper Right Corner Counterclockwise
        case 31:
            if (selectedPieceY > 1 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1] % 10) == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 1 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1] % 10) == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX - 2] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 2);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX - 2] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 2);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1] % 10) == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1] % 10) == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX + 2] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 2);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX + 2] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 2);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            break;
//White Bishop Movement from Upper Right Corner Counterclockwise
        case 32:
        {
            int tempX = selectedPieceX + 1;
            int tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX <= 7)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX >= 0)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX >= 0)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY++;
            }
            tempX = selectedPieceX + 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX <= 7)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY++;
            }
            break;
        }
//Black Bishop Movement from Upper Right Corner Counterclockwise
        case 33:
        {
            int tempX = selectedPieceX + 1;
            int tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX <= 7)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX >= 0)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX >= 0)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY++;
            }
            tempX = selectedPieceX + 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX <= 7)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY++;
            }
            break;
        }

//White Queen Bishop
        case 90:
        {
            int tempX = selectedPieceX + 1;
            int tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX <= 7)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX >= 0)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX >= 0)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY++;
            }
            tempX = selectedPieceX + 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX <= 7)
            {
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY++;
            }
//White Queen Rook
            {   //up
                int tempY = selectedPieceY - 1;
                while (tempY >= 0)
                {
                    if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempY--;
                }
                tempY = selectedPieceY + 1;
                //down
                while (tempY <= 7)
                {
                    if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempY++;
                }
                //left
                int tempX = selectedPieceX - 1;
                while (tempX >= 0)
                {
                    if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempX--;
                }
                tempX = selectedPieceX + 1;
                //right
                while (tempX <= 7)
                {
                    if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempX++;
                }
            }
            break;
        }
//Black Queen Bishop
        case 91:
        {
            int tempX = selectedPieceX + 1;
            int tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX <= 7)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY - 1;
            while (tempY >= 0 && tempX >= 0)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY--;
            }
            tempX = selectedPieceX - 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX >= 0)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
                tempY++;
            }
            tempX = selectedPieceX + 1;
            tempY = selectedPieceY + 1;
            while (tempY <= 7 && tempX <= 7)
            {
                if (((chessBoard[tempY][tempX] % 10) == 0 && chessBoard[tempY][tempX] != 0) || (chessBoard[tempY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX] % 10) == 1 || (chessBoard[tempY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
                tempY++;
            }
//Black Queen Rook
            {   //up
                int tempY = selectedPieceY - 1;
                while (tempY >= 0)
                {
                    if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempY--;
                }
                tempY = selectedPieceY + 1;
                //down
                while (tempY <= 7)
                {
                    if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempY++;
                }
                //left
                int tempX = selectedPieceX - 1;
                while (tempX >= 0)
                {
                    if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempX--;
                }
                tempX = selectedPieceX + 1;
                //right
                while (tempX <= 7)
                {
                    if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                        updateIsKingInCheck();
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                    }
                    tempX++;
                }
            }
        break;
        }
//White Rook
        case 50:
        {   //up
            int tempY = selectedPieceY - 1;
            while (tempY >= 0)
            {
                if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempY--;
            }
            tempY = selectedPieceY + 1;
            //down
            while (tempY <= 7)
            {
                if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempY++;
            }
            //left
            int tempX = selectedPieceX - 1;
            while (tempX >= 0)
            {
                if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
            }
            tempX = selectedPieceX + 1;
            //right
            while (tempX <= 7)
            {
                if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
            }
        }
        break;
//Black Rook
        case 51:
        {   //up
            int tempY = selectedPieceY - 1;
            while (tempY >= 0)
            {
                if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempY--;
            }
            tempY = selectedPieceY + 1;
            //down
            while (tempY <= 7)
            {
                if (((chessBoard[tempY][selectedPieceX] % 10) == 0 && chessBoard[tempY][selectedPieceX] != 0) || (chessBoard[tempY][selectedPieceX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][selectedPieceX] % 10) == 1 || (chessBoard[tempY][selectedPieceX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempY++;
            }
            //left
            int tempX = selectedPieceX - 1;
            while (tempX >= 0)
            {
                if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX--;
            }
            tempX = selectedPieceX + 1;
            //right
            while (tempX <= 7)
            {
                if (((chessBoard[selectedPieceY][tempX] % 10) == 0 && chessBoard[selectedPieceY][tempX] != 0) || (chessBoard[selectedPieceY][tempX] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[selectedPieceY][tempX] % 10) == 1 || (chessBoard[selectedPieceY][tempX] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                }
                tempX++;
            }
        }
        break;
//White King from Upper Right Counterclockwise
        case 100:
        {
            if (selectedPieceY > 0 && selectedPieceX < 7 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX > 0 && (chessBoard[selectedPieceY][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1] % 10) == 1 || (chessBoard[selectedPieceY][selectedPieceX - 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX < 7 && (chessBoard[selectedPieceY][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1] % 10) == 1 || (chessBoard[selectedPieceY][selectedPieceX + 1] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }

            //Castling
            if (isWhiteKingInCheck == false)
            {
                if (isHumanWhite == true)
                {
                    if (isWhiteKingCastlingPossible == true && chessBoard[7][5] == 0 && chessBoard[7][6] == 0)
                    {
                        if (dangerSquaresForWhiteKing[7][5] == false && dangerSquaresForWhiteKing[7][6] == false)
                        {
                            isChessSquareViableMove[7][6] = true;
                        }
                    }
                    if (isWhiteQueenCastlingPossible == true && chessBoard[7][1] == 0 && chessBoard[7][2] == 0 && chessBoard[7][3] == 0)
                    {
                        if (dangerSquaresForWhiteKing[7][2] == false && dangerSquaresForWhiteKing[7][3] == false)
                        {
                            isChessSquareViableMove[7][2] = true;
                        }
                    }
                }
                else
                {
                    if (isWhiteKingCastlingPossible == true && chessBoard[0][1] == 0 && chessBoard[0][2] == 0)
                    {
                        if (dangerSquaresForWhiteKing[0][1] == false && dangerSquaresForWhiteKing[0][2] == false)
                        {
                            isChessSquareViableMove[0][1] = true;
                        }
                    }
                    if (isWhiteQueenCastlingPossible == true && chessBoard[0][4] == 0 && chessBoard[0][5] == 0 && chessBoard[0][6] == 0)
                    {
                        if (dangerSquaresForWhiteKing[0][4] == false && dangerSquaresForWhiteKing[0][5] == false)
                        {
                            isChessSquareViableMove[0][5] = true;
                        }
                    }
                }
            }
        }
        break;
//Black King from Upper Right Counterclockwise
        case 101:
        {
            if (selectedPieceY > 0 && selectedPieceX < 7 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX > 0 && (chessBoard[selectedPieceY][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1] % 10) == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX < 7 && (chessBoard[selectedPieceY][selectedPieceX + 1] == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1] % 10) == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX + 1);
                    updateIsKingInCheck();
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }

            //Castling
            if (isBlackKingInCheck == false)
            {
                if (isHumanWhite == false)
                {
                    if (isBlackKingCastlingPossible == true && chessBoard[7][1] == 0 && chessBoard[7][2] == 0)
                    {
                        if (dangerSquaresForBlackKing[7][1] == false && dangerSquaresForBlackKing[7][2] == false)
                        {
                            isChessSquareViableMove[7][1] = true;
                        }
                    }
                    if (isBlackQueenCastlingPossible == true && chessBoard[7][4] == 0 && chessBoard[7][5] == 0 && chessBoard[7][6] == 0)
                    {
                        if (dangerSquaresForBlackKing[7][4] == false && dangerSquaresForBlackKing[7][5] == false)
                        {
                            isChessSquareViableMove[7][5] = true;
                        }
                    }
                }
                else
                {
                    if (isBlackKingCastlingPossible == true && chessBoard[0][5] == 0 && chessBoard[0][6] == 0)
                    {
                        if (dangerSquaresForBlackKing[0][5] == false && dangerSquaresForBlackKing[0][6] == false)
                        {
                            isChessSquareViableMove[0][6] = true;
                        }
                    }
                    if (isBlackQueenCastlingPossible == true && chessBoard[0][1] == 0 && chessBoard[0][2] == 0 && chessBoard[0][3] == 0)
                    {
                        if (dangerSquaresForBlackKing[0][2] == false && dangerSquaresForBlackKing[0][3] == false)
                        {
                            isChessSquareViableMove[0][2] = true;
                        }
                    }
                }
            }
        }
        break;
        }
    }
    updateIsKingInCheck();
}

static void isGameEnded()
{
    isAnyMoveViable = false;
    isChessPieceSelected = true;
    if (moveNumber >= 100)
    {
        gameEnded = true;
    }
    if (gameEnded == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (isWhitesTurn == true)
                {
                    if (chessBoard[h][w] != 0 && ((chessBoard[h][w] % 10) == 0 || (chessBoard[h][w] % 10) == 2))
                    {
                        selectedPieceY = h;
                        selectedPieceX = w;
                        updateViableMoves();
                        for (int i = 0; i < 8; i++)
                        {
                            for (int j = 0; j < 8; j++)
                            {
                                if (isChessSquareViableMove[i][j] == true)
                                {
                                    isAnyMoveViable = true;
                                }
                            }
                        }
                    }
                }
                if (isWhitesTurn == false)
                {
                    if (chessBoard[h][w] != 0 && ((chessBoard[h][w] % 10) == 1 || (chessBoard[h][w] % 10) == 3))
                    {
                        selectedPieceY = h;
                        selectedPieceX = w;
                        updateViableMoves();
                        for (int i = 0; i < 8; i++)
                        {
                            for (int j = 0; j < 8; j++)
                            {
                                if (isChessSquareViableMove[i][j] == true)
                                {
                                    isAnyMoveViable = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (isAnyMoveViable == false)
    {
        gameEnded = true;
        if (isWhitesTurn == true)
        {
            if (isWhiteKingInCheck == true)
            {
                if (isHumanWhite == true)
                {
                    didHumanWin = false;
                }
                else
                {
                    didHumanWin = true;
                }
            }
            else
            {
                isItDraw = true;
            }
        }
        else
        {
            if (isBlackKingInCheck == true)
            {
                if (isHumanWhite == true)
                {
                    didHumanWin = true;
                }
                else
                {
                    didHumanWin = false;
                }
            }
            else
            {
                isItDraw = true;
            }
        }
    }

    isChessPieceSelected = false;
    resetisChessSquareViableMove();
}


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "cstring";

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

//Neural network functions and variables

const int neuronsCount[10] =
{
    387, 1024, 4096, 4096, 4096,
    4096, 2048, 1024, 512, 128
};

float weights[4096][4096][10];

float neurons[4096][10];

int biases[4096][10];

float nodeValues[4096][10];

int chessPositionsData[100][66];
int increment = 0;
bool wasHumanFirstInData = true;
float learningRate = 0.0001f;
bool isBackPropagationRunning = false;
int tempHowItsFrom = 0;
int tempHowItsTo = 0;

#include <fstream>
#include <random>
#include <string>

int temporaryChessPositionsData[66];
//chessPositionsData[0][0] = 0 - Draw, 10 - Do like first, -10 - Do like second
#pragma warning(disable : 4996)

static void resetButton()
{
    isGameStarted = false;
    boardReset();
    boardBeginWhite();
    isHumanWhite = true;
    isHumanPlayingAgainstAI = false;
    isAITurnedOn = false;
    resetisChessSquareViableMove();
    isWhitesTurn = true;
    updateIsKingInCheck();
    gameEnded = false;
    didHumanWin = false;
    isItDraw = false;
    isAnyMoveViable = true;
    isWhiteKingCastlingPossible = true;
    isWhiteQueenCastlingPossible = true;
    isBlackKingCastlingPossible = true;
    isBlackQueenCastlingPossible = true;
    isEnPassantForWhitePossible = false;
    isEnPassantForBlackPossible = false;
    enPassantX = 0;
    moveNumber = 0;
    wasHumanFirstInData = false;
}

//Cost
/*
static void nnCalculateCost(bool draw,bool isBestMoveKnown, int howItsFrom, int howItsTo, int howShouldBeFrom, int howShouldBeTo)
{
    if (draw == true)
    {
        for (int i = 0; i < 64; i++)
        {
            if (i == howItsFrom)
            {
                nodeValues9[i] = -neuronsLayer9[i] - 127;
            }
            else
            {
                nodeValues9[i] = -neuronsLayer9[i];
            }
        }

        for (int i = 64; i < 128; i++)
        {
            if (i == howItsTo+64)
            {
                nodeValues9[i] = -neuronsLayer9[i] - 127;
            }
            else
            {
                nodeValues9[i] = -neuronsLayer9[i];
            }
        }
    }
    else
    {
        if (isBestMoveKnown == true)
        {
            for (int i = 0; i < 64; i++)
            {
                if (i == howItsFrom && i == howShouldBeFrom)
                {
                    nodeValues9[i] = 0;
                }
                else if (i == howShouldBeFrom)
                {
                    nodeValues9[i] = -neuronsLayer9[i] + 127;
                }
                else
                {
                    nodeValues9[i] = -neuronsLayer9[i] - 127;
                }
            }

            for (int i = 64; i < 128; i++)
            {
                if (i == howItsTo+64 && i == howShouldBeTo+64)
                {
                    nodeValues9[i] = 0;
                }
                else if (i == howShouldBeTo+64)
                {
                    nodeValues9[i] = -neuronsLayer9[i] + 127;
                }
                else
                {
                    nodeValues9[i] = -neuronsLayer9[i] - 127;
                }
            }

        }
        else
        {
            for (int i = 0; i < 64; i++)
            {
                if (i == howItsFrom)
                {
                    nodeValues9[i] = -neuronsLayer9[i] - 127;
                }
                else
                {
                    nodeValues9[i] = -neuronsLayer9[i];
                }
            }
            for (int i = 64; i < 128; i++)
            {
                if (i == howItsTo+64)
                {
                    nodeValues9[i] = -neuronsLayer9[i] - 127;
                }
                else
                {
                    nodeValues9[i] = -neuronsLayer9[i];
                }
            }
        }
    }

    if (isTestingON == true)
    {
        std::cout << "\nNode Values9: ";
        for (int i = 0; i < 128; i++)
        {
            std::cout << nodeValues9[i] << ".";
        }
    }

    //Multiplying by 2; partial derivative
    for (int i = 0; i < 128; i++)
    {
        nodeValues9[i] *= 2;
    }

    if (isTestingON == true)
    {
        std::cout << "\nAfter derivative: ";
        for (int i = 0; i < 128; i++)
        {
            std::cout << nodeValues9[i] << ".";
        }
    }

}
*/

static void nnLoad()
{
    char weight[] = "NeuralNetwork/weightsLayer";
    char bias[] = "NeuralNetwork/biasesLayer";
    char number[] = "0";
    char txt[] = ".txt";

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(weight) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, weight);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ifstream w(name);

        for (int j = 0; j < neuronsCount[i - 1]; j++)
        {
            for (int k = 0; k < neuronsCount[i]; k++)
            {
                w >> weights[j][k][i];

            }
        }
        w.close();
    }

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(bias) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, bias);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ifstream b(name);

        for (int j = 0; j < neuronsCount[i]; j++)
        {
            b >> biases[j][i];
        }
        b.close();
    }
}

static void nnWriteWithRandomValues()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrWeight(0, 10000);
    std::uniform_int_distribution<> distrBias(0, 20);
    char weight[] = "NeuralNetwork/weightsLayer";
    char bias[] = "NeuralNetwork/biasesLayer";
    char number[] = "0";
    char txt[] = ".txt";

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(weight) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, weight);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ofstream w(name);

        for (int j = 0; j < neuronsCount[i-1]; j++)
        {
            for (int k = 0; k < neuronsCount[i]; k++)
            {
                w << distrWeight(gen) << " ";
            }
            w << std::endl;
        }
        w.close();
    }

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(bias) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, bias);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ofstream b(name);

        for (int j = 0; j < neuronsCount[i]; j++)
        {
            b << distrBias(gen) << " ";
        }
        b.close();
    }
}

static void nnWrite()
{
    char weight[] = "NeuralNetwork/weightsLayer";
    char bias[] = "NeuralNetwork/biasesLayer";
    char number[] = "0";
    char txt[] = ".txt";

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(weight) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, weight);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ofstream w(name);

        for (int j = 0; j < neuronsCount[i - 1]; j++)
        {
            for (int k = 0; k < neuronsCount[i]; k++)
            {
                w << weights[j][k][i] << " ";
            }
            w << std::endl;
        }
        w.close();
    }

    for (int i = 1; i < 10; i++)
    {
        char* name = new char[std::strlen(bias) + std::strlen(number) + std::strlen(txt) + 1];
        std::strcpy(name, bias);
        number[0] = i + '0';
        std::strcat(name, number);
        std::strcat(name, txt);
        std::ofstream b(name);

        for (int j = 0; j < neuronsCount[i]; j++)
        {
            b << biases[j][i] << " ";
        }
        b.close();
    }
}

//BoardToinput
/*
static void nnBoardToInput()
{
    for (int i = 0; i < neuronsInLayer0; i++)
    {
        neuronsLayer0[i] = 0;
    }
    //387 = 64 + 64 + 64 + 64 + 64 + 64 + 3
    //      P    N    B    R    Q    K    castle, castle, -1 no enpassant, 0-7 collumn in which pawn moved
    //bot pieces = +; opponent pieces = -
    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            switch (chessBoard[h][w])
            {
            case 10:
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w] = 1;
                }
                break;
            case 11:
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w] = 1;
                }
                break;
            case 30:
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w + 64] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 64] = 1;
                }
                break;
            case 31: 
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w + 64] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 64] = 1;
                }
                break;
            case 32: 
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w + 128] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 128] = 1;
                }
                break;
            case 33: 
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w + 128] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 128] = 1;
                }
                break;
            case 50:
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w + 192] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 192] = 1;
                }
                break;
            case 51:
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w + 192] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 192] = 1;
                }
                break;
            case 90:
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w + 256] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 256] = 1;
                }
                break;
            case 91:
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w + 256] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 256] = 1;
                }
                break;
            case 100:
                if (isHumanWhite == true)
                {
                    neuronsLayer0[(h * 8) + w + 320] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 320] = 1;
                }
                break;
            case 101:
                if (isHumanWhite == false)
                {
                    neuronsLayer0[(h * 8) + w + 320] = -1;
                }
                else
                {
                    neuronsLayer0[(h * 8) + w + 320] = 1;
                }
                break;
            }

            if (isHumanWhite == true)
            {
                if (isBlackKingCastlingPossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 3] = 10;
                }
                if (isBlackQueenCastlingPossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 2] = 10;
                }
                if (isEnPassantForBlackPossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 1] = enPassantX;
                }
            }
            else
            {
                if (isWhiteKingCastlingPossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 3] = 10;
                }
                if (isWhiteQueenCastlingPossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 2] = 10;
                }
                if (isEnPassantForWhitePossible == true)
                {
                    neuronsLayer0[neuronsInLayer0 - 1] = enPassantX;
                }
            }
        }
    }
}
*/

//Think
/*
static void nnThink()
{

    //Calculate 1st layer
    for (int i = 0; i < neuronsInLayer1; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer0; j++)
        {
            temporaryNeuronValue = (neuronsLayer0[j] * weightsLayer1[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer1[i];
        temporaryNeuronValue /= neuronsInLayer0;
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer1[i] = temporaryNeuronValue;
    }
    //Calculate 2nd layer
    for (int i = 0; i < neuronsInLayer2; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer1; j++)
        {
            temporaryNeuronValue = (neuronsLayer1[j] * weightsLayer2[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer2[i];
        temporaryNeuronValue /= (neuronsInLayer1 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer2[i] = temporaryNeuronValue;
    }
    //Calculate 3rd layer
    for (int i = 0; i < neuronsInLayer3; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer2; j++)
        {
            temporaryNeuronValue = (neuronsLayer2[j] * weightsLayer3[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer3[i];
        temporaryNeuronValue /= (neuronsInLayer2 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer3[i] = temporaryNeuronValue;
    }
    //Calculate 4th layer
    for (int i = 0; i < neuronsInLayer4; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer3; j++)
        {
            temporaryNeuronValue = (neuronsLayer3[j] * weightsLayer4[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer4[i];
        temporaryNeuronValue /= (neuronsInLayer3 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer4[i] = temporaryNeuronValue;
    }
    //Calculate 5th layer
    for (int i = 0; i < neuronsInLayer5; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer4; j++)
        {
            temporaryNeuronValue = (neuronsLayer4[j] * weightsLayer5[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer5[i];
        temporaryNeuronValue /= (neuronsInLayer4 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer5[i] = temporaryNeuronValue;
    }
    //Calculate 6th layer
    for (int i = 0; i < neuronsInLayer6; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer5; j++)
        {
            temporaryNeuronValue = (neuronsLayer5[j] * weightsLayer6[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer6[i];
        temporaryNeuronValue /= (neuronsInLayer5 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer6[i] = temporaryNeuronValue;
    }
    //Calculate 7th layer
    for (int i = 0; i < neuronsInLayer7; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer6; j++)
        {
            temporaryNeuronValue = (neuronsLayer6[j] * weightsLayer7[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer7[i];
        temporaryNeuronValue /= (neuronsInLayer6 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer7[i] = temporaryNeuronValue;
    }
    //Calculate 8th layer
    for (int i = 0; i < neuronsInLayer8; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer7; j++)
        {
            temporaryNeuronValue = (neuronsLayer7[j] * weightsLayer8[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer8[i];
        temporaryNeuronValue /= (neuronsInLayer7 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer8[i] = temporaryNeuronValue;      
    }
    //Calculate 9th layer
    for (int i = 0; i < neuronsInLayer9; i++)
    {
        int temporaryNeuronValue = 0;
        for (int j = 0; j < neuronsInLayer8; j++)
        {
            temporaryNeuronValue = (neuronsLayer8[j] * weightsLayer9[j][i]) + temporaryNeuronValue;
        }
        temporaryNeuronValue -= biasesLayer9[i];
        temporaryNeuronValue /= (neuronsInLayer8 * 127);
        if (temporaryNeuronValue > 127)
        {
            temporaryNeuronValue = 127;
        }
        if (temporaryNeuronValue < -127)
        {
            temporaryNeuronValue = -127;
        }
        neuronsLayer9[i] = temporaryNeuronValue;
    }
}
*/

//WhatMoveToPlay
/*
static void nnWhatMoveToPlay()
{
    if (isTestingON == true)
    {
        std::cout << "Last Layer Neurons: ";
        for (int i = 0; i < neuronsInLayer9; i++)
        {
            std::cout << neuronsLayer9[i] << ".";
        }
        std::cout << "\n";
    }
    if (isBackPropagationRunning == false)
    {
        moveNumber++;
        if (moveNumber == 1)
        {
            wasHumanFirstInData = false;
        }
        if (isTestingON == true)
        {
            std::cout << "ChessData: ";
        }
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                chessPositionsData[moveNumber][h * 8 + w] = chessBoard[h][w];
                if (isTestingON == true)
                {
                    std::cout << chessBoard[h][w] << ".";
                }
            }
        }
    }
    //Delete OutputFrom which is empty square or opponnent square
    if (isHumanWhite == true)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (chessBoard[h][w] == 0 || chessBoard[h][w] % 10 == 0 || chessBoard[h][w] % 10 == 2)
                {
                    neuronsLayer9[h * 8 + w] = -128;
                }
            }
        }
    }
    else
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (chessBoard[h][w] == 0 || chessBoard[h][w] % 10 == 1 || chessBoard[h][w] % 10 == 3)
                {
                    neuronsLayer9[h * 8 + w] = -128;
                }
            }
        }
    }

    //Delete non viable moves
    if (isHumanWhite == true)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (chessBoard[h][w] != 0 && (chessBoard[h][w] % 10 == 1 || chessBoard[h][w] % 10 == 3))
                {
                    isChessPieceSelected = true;
                    selectedPieceX = w;
                    selectedPieceY = h;
                    updateViableMoves();
                }
            }
        }
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (isChessSquareViableMove[h][w] == false)
                {
                    neuronsLayer9[h * 8 + w + 64] = -128;
                }
            }
        }
        resetisChessSquareViableMove();
    }
    else
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (chessBoard[h][w] != 0 && (chessBoard[h][w] % 10 == 0 || chessBoard[h][w] % 10 == 2))
                {
                    isChessPieceSelected = true;
                    selectedPieceX = w;
                    selectedPieceY = h;
                    updateViableMoves();
                }
            }
        }
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (isChessSquareViableMove[h][w] == false)
                {
                    neuronsLayer9[h * 8 + w + 64] = -128;
                }
            }
        }
        resetisChessSquareViableMove();
    }

    bool doesThisPieceHaveMoves = false;
    bool isAnyMoveLeft = true;

    int valueFrom = -128;
    int placeFrom = 0;

    int toX = 0;
    int toY = 0;

    while (doesThisPieceHaveMoves == false)
    {
        isAnyMoveLeft = false;
        valueFrom = -128;
        placeFrom = 0;
        for (int i = 0; i < 64; i++)
        {
            if (neuronsLayer9[i] > valueFrom)
            {
                valueFrom = neuronsLayer9[i];
                placeFrom = i;
            }
        }
        selectedPieceX = placeFrom % 8;
        placeFrom -= selectedPieceX;
        selectedPieceY = placeFrom / 8;
        isChessPieceSelected = true;
        updateViableMoves();
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (isChessSquareViableMove[h][w] == true)
                {
                    doesThisPieceHaveMoves = true;
                }
            }
        }
        if (doesThisPieceHaveMoves == false)
        {
            neuronsLayer9[selectedPieceY * 8 + selectedPieceX] = -128;
            resetisChessSquareViableMove();
        }
        for (int i = 0; i < 64; i++)
        {
            if (neuronsLayer9[i] > -128)
            {
                isAnyMoveLeft = true;
            }
        }
        if (isAnyMoveLeft == false)
        {
            isGameEnded();
        }
    }
    int valueTo = -128;
    int placeTo = 0;
    if (gameEnded == false)
    {

        for (int i = 0; i < 64; i++)
        {
            if (neuronsLayer9[i + 64] > valueTo)
            {
                valueTo = neuronsLayer9[i + 64];
                placeTo = i;
            }
        }
        toX = placeTo % 8;
        placeTo -= toX;
        toY = placeTo / 8;
        while (isChessSquareViableMove[toY][toX] == false)
        {
            neuronsLayer9[toY * 8 + toX + 64] = -256;
            valueTo = -128;
            placeTo = 0;
            for (int i = 0; i < 64; i++)
            {
                if (neuronsLayer9[i + 64] > valueTo)
                {
                    valueTo = neuronsLayer9[i + 64];
                    placeTo = i;
                }
            }
            toX = placeTo % 8;
            placeTo -= toX;
            toY = placeTo / 8;
        }
        //for NN
        if (isBackPropagationRunning == false)
        {
            chessPositionsData[moveNumber][64] = (selectedPieceY * 8) + selectedPieceX;
            chessPositionsData[moveNumber][65] = (toY * 8) + toX;
            if (isTestingON == true)
            {
                std::cout << "\nMoves: " << chessPositionsData[moveNumber][64] << "." << chessPositionsData[moveNumber][65];
            }
        }
        //Moving piece
        int whatPieceToMove = chessBoard[selectedPieceY][selectedPieceX];
        chessBoard[selectedPieceY][selectedPieceX] = 0;
        chessBoard[toY][toX] = whatPieceToMove;
        if (isGameStarted == false) { isGameStarted = true; }

        //Rook move while castling
        if (whatPieceToMove == 100)
        {
            if (isHumanWhite == true)
            {
                if (isWhiteKingCastlingPossible == true && toY == 7 && toX == 6)
                {
                    chessBoard[7][5] = 50;
                    chessBoard[7][7] = 0;
                }
                if (isWhiteQueenCastlingPossible == true && toY == 7 && toX == 2)
                {
                    chessBoard[7][3] = 50;
                    chessBoard[7][0] = 0;
                }
            }
            else
            {
                if (isWhiteKingCastlingPossible == true && toY == 0 && toX == 1)
                {
                    chessBoard[0][2] = 50;
                    chessBoard[0][0] = 0;
                }
                if (isWhiteQueenCastlingPossible == true && toY == 0 && toX == 5)
                {
                    chessBoard[0][4] = 50;
                    chessBoard[0][7] = 0;
                }
            }
        }
        if (whatPieceToMove == 101)
        {
            if (isHumanWhite == true)
            {
                if (isBlackKingCastlingPossible == true && toY == 0 && toX == 6)
                {
                    chessBoard[0][5] = 51;
                    chessBoard[0][7] = 0;
                }
                if (isBlackQueenCastlingPossible == true && toY == 0 && toX == 2)
                {
                    chessBoard[0][3] = 51;
                    chessBoard[0][0] = 0;
                }
            }
            else
            {
                if (isBlackKingCastlingPossible == true && toY == 7 && toX == 1)
                {
                    chessBoard[7][2] = 51;
                    chessBoard[7][0] = 0;
                }
                if (isBlackQueenCastlingPossible == true && toY == 7 && toX == 5)
                {
                    chessBoard[7][4] = 51;
                    chessBoard[7][7] = 0;
                }
            }
        }

        //Remove possibility of castling
        switch (whatPieceToMove)
        {
        default:
            break;
        case 100:
            isWhiteKingCastlingPossible = false;
            isWhiteQueenCastlingPossible = false;
            break;
        case 101:
            isBlackKingCastlingPossible = false;
            isBlackQueenCastlingPossible = false;
            break;
        case 50:
            if (isHumanWhite == true)
            {
                if (selectedPieceX == 7) { isWhiteKingCastlingPossible = false; }
                if (selectedPieceX == 0) { isWhiteQueenCastlingPossible = false; }
            }
            if (isHumanWhite == false)
            {
                if (selectedPieceX == 0) { isWhiteKingCastlingPossible = false; }
                if (selectedPieceX == 7) { isWhiteQueenCastlingPossible = false; }
            }
            break;
        case 51:
            if (isHumanWhite == false)
            {
                if (selectedPieceX == 0) { isBlackKingCastlingPossible = false; }
                if (selectedPieceX == 7) { isBlackQueenCastlingPossible = false; }
            }
            if (isHumanWhite == true)
            {
                if (selectedPieceX == 7) { isBlackKingCastlingPossible = false; }
                if (selectedPieceX == 0) { isBlackQueenCastlingPossible = false; }
            }
            break;
        }

        //En Passant pawn capture
        if (whatPieceToMove == 10)
        {
            if (isHumanWhite == true)
            {
                if (isEnPassantForWhitePossible == true && toY == 2)
                {
                    chessBoard[toY + 1][toX] = 0;
                }
            }
            else
            {
                if (isEnPassantForWhitePossible == true && toY == 5)
                {
                    chessBoard[toY - 1][toX] = 0;
                }
            }
        }
        if (whatPieceToMove == 11)
        {
            if (isHumanWhite == true)
            {
                if (isEnPassantForBlackPossible == true && toY == 5)
                {
                    chessBoard[toY - 1][toX] = 0;
                }
            }
            else
            {
                if (isEnPassantForBlackPossible == true && toY == 2)
                {
                    chessBoard[toY + 1][toX] = 0;
                }
            }
        }

        //isEnPassantPossible
        if (whatPieceToMove == 10)
        {
            if (isHumanWhite == true)
            {
                if (selectedPieceY == 6 && toY == 4)
                {
                    isEnPassantForBlackPossible = true;
                    enPassantX = toX;
                }
            }
            else
            {
                if (selectedPieceY == 1 && toY == 3)
                {
                    isEnPassantForBlackPossible = true;
                    enPassantX = toX;
                }
            }
        }
        else
        {
            isEnPassantForBlackPossible = false;
        }
        if (whatPieceToMove == 11)
        {
            if (isHumanWhite == true)
            {
                if (selectedPieceY == 1 && toY == 3)
                {
                    isEnPassantForWhitePossible = true;
                    enPassantX = toX;
                }
            }
            else
            {
                if (selectedPieceY == 6 && toY == 4)
                {
                    isEnPassantForWhitePossible = true;
                    enPassantX = toX;
                }
            }
        }
        else
        {
            isEnPassantForWhitePossible = false;
        }

        if (isWhitesTurn == true) { isWhitesTurn = false; }
        else { isWhitesTurn = true; }

        resetisChessSquareViableMove();
        updatePawnToQueenPromotion();
        updateIsKingInCheck();
        isGameEnded();
    }
    if (isBackPropagationRunning == true)
    {
        tempHowItsFrom = placeFrom;
        tempHowItsTo = placeTo;
    }
}
*/

//Backpropagation
/*
static void nnBackPropagation()
{
    //W9 update
    for (int h = 0; h < neuronsInLayer8; h++)
    {
        for (int w = 0; w < neuronsInLayer9; w++)
        {
            weightsLayer9[h][w] += (nodeValues9[w] * neuronsLayer8[h] * learningRate * ((increment/10) + 1));
            if (weightsLayer9[h][w] > 127)
            {
                weightsLayer9[h][w] = 127;
            }
            if (weightsLayer9[h][w] < -127)
            {
                weightsLayer9[h][w] = -127;
            }
            if (isTestingON == true)
            {
                std::cout << "Weight:" << weightsLayer9[h][w];
                std::cout << " . Node: " << nodeValues9[w] << " . NL8: " << neuronsLayer8[h] << " . LR: " << learningRate << " . Inc: " << (increment / 10) + 1;
            }
            std::cout << "\n";
        }
    }
    //B9 update
    for (int i = 0; i < neuronsInLayer9; i++)
    {
        biasesLayer9[i] += (nodeValues9[i]);
        if (biasesLayer9[i] > 1000)
        {
            biasesLayer9[i] = 1000;
        }
        if (biasesLayer9[i] < -1000)
        {
            biasesLayer9[i] = -1000;
        }
    }
    //Node8 update
    if (isTestingON == true)
    {
        std::cout << "\n";
    }
    for (int i = 0; i < neuronsInLayer8; i++)
    {
        for (int j = 0; j < neuronsInLayer9; j++)
        {
            nodeValues8[i] += weightsLayer9[i][j] * nodeValues9[j];

        }
        nodeValues8[i] /= (neuronsInLayer9 * neuronsInLayer9);
        if (nodeValues8[i] > 127)
        {
            nodeValues8[i] = 127;
        }
        if (nodeValues8[i] < - 127)
        {
            nodeValues8[i] = -127;
        }
        if (isTestingON == true)
        {
            std::cout << "NV8: " << nodeValues8[i] << ".WL:" << weightsLayer9[i][0] << ". NV9:" << nodeValues9[0] << ".";
        }
        std::cout << "\n";
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer8; i++)
    {
        nodeValues8[i] *= 2;
    }
    //////////////////////////////////////
    //W8 update
    for (int h = 0; h < neuronsInLayer7; h++)
    {
        for (int w = 0; w < neuronsInLayer8; w++)
        {
            weightsLayer8[h][w] += (nodeValues8[w] * neuronsLayer7[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer8[h][w] > 127)
            {
                weightsLayer8[h][w] = 127;
            }
            if (weightsLayer8[h][w] < -127)
            {
                weightsLayer8[h][w] = -127;
            }
        }
    }
    //B8 update
    for (int i = 0; i < neuronsInLayer8; i++)
    {
        biasesLayer8[i] += (nodeValues8[i]);
        if (biasesLayer8[i] > 1000)
        {
            biasesLayer8[i] = 1000;
        }
        if (biasesLayer8[i] < -1000)
        {
            biasesLayer8[i] = -1000;
        }
    }
    //Node7 update
    for (int i = 0; i < neuronsInLayer7; i++)
    {
        for (int j = 0; j < neuronsInLayer8; j++)
        {
            nodeValues7[i] += weightsLayer8[i][j] * nodeValues8[j];
        }
        nodeValues7[i] /= (neuronsInLayer8 * neuronsInLayer8);
        if (nodeValues7[i] > 127)
        {
            nodeValues7[i] = 127;
        }
        if (nodeValues7[i] < -127)
        {
            nodeValues7[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer7; i++)
    {
        nodeValues7[i] *= 2;
    }
    //////////////////////////////////////
    //W7 update
    for (int h = 0; h < neuronsInLayer6; h++)
    {
        for (int w = 0; w < neuronsInLayer7; w++)
        {
            weightsLayer7[h][w] += (nodeValues7[w] * neuronsLayer6[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer7[h][w] > 127)
            {
                weightsLayer7[h][w] = 127;
            }
            if (weightsLayer7[h][w] < -127)
            {
                weightsLayer7[h][w] = -127;
            }
        }
    }
    //B7 update
    for (int i = 0; i < neuronsInLayer7; i++)
    {
        biasesLayer7[i] += (nodeValues7[i]);
        if (biasesLayer7[i] > 1000)
        {
            biasesLayer7[i] = 1000;
        }
        if (biasesLayer7[i] < -1000)
        {
            biasesLayer7[i] = -1000;
        }
    }
    //Node6 update
    for (int i = 0; i < neuronsInLayer6; i++)
    {
        for (int j = 0; j < neuronsInLayer7; j++)
        {
            nodeValues6[i] += weightsLayer7[i][j] * nodeValues7[j];
        }
        nodeValues6[i] /= (neuronsInLayer7 * neuronsInLayer7);
        if (nodeValues6[i] > 127)
        {
            nodeValues6[i] = 127;
        }
        if (nodeValues6[i] < -127)
        {
            nodeValues6[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer6; i++)
    {
        nodeValues6[i] *= 2;
    }
    //////////////////////////////////////
    //W6 update
    for (int h = 0; h < neuronsInLayer5; h++)
    {
        for (int w = 0; w < neuronsInLayer6; w++)
        {
            weightsLayer6[h][w] += (nodeValues6[w] * neuronsLayer5[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer6[h][w] > 127)
            {
                weightsLayer6[h][w] = 127;
            }
            if (weightsLayer6[h][w] < -127)
            {
                weightsLayer6[h][w] = -127;
            }
        }
    }
    //B6 update
    for (int i = 0; i < neuronsInLayer6; i++)
    {
        biasesLayer6[i] += (nodeValues6[i]);
        if (biasesLayer6[i] > 1000)
        {
            biasesLayer6[i] = 1000;
        }
        if (biasesLayer6[i] < -1000)
        {
            biasesLayer6[i] = -1000;
        }
    }
    //Node5 update
    for (int i = 0; i < neuronsInLayer5; i++)
    {
        for (int j = 0; j < neuronsInLayer6; j++)
        {
            nodeValues5[i] += weightsLayer6[i][j] * nodeValues6[j];
        }
        nodeValues5[i] /= (neuronsInLayer6 * neuronsInLayer6);
        if (nodeValues5[i] > 127)
        {
            nodeValues5[i] = 127;
        }
        if (nodeValues5[i] < -127)
        {
            nodeValues5[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer5; i++)
    {
        nodeValues5[i] *= 2;
    }
    //////////////////////////////////////
    //W5 update
    for (int h = 0; h < neuronsInLayer4; h++)
    {
        for (int w = 0; w < neuronsInLayer5; w++)
        {
            weightsLayer5[h][w] += (nodeValues5[w] * neuronsLayer4[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer5[h][w] > 127)
            {
                weightsLayer5[h][w] = 127;
            }
            if (weightsLayer5[h][w] < -127)
            {
                weightsLayer5[h][w] = -127;
            }
        }
    }
    //B5 update
    for (int i = 0; i < neuronsInLayer5; i++)
    {
        biasesLayer5[i] += (nodeValues5[i]);
        if (biasesLayer5[i] > 1000)
        {
            biasesLayer5[i] = 1000;
        }
        if (biasesLayer5[i] < -1000)
        {
            biasesLayer5[i] = -1000;
        }
    }
    //Node4 update
    for (int i = 0; i < neuronsInLayer4; i++)
    {
        for (int j = 0; j < neuronsInLayer5; j++)
        {
            nodeValues4[i] += weightsLayer5[i][j] * nodeValues5[j];
        }
        nodeValues4[i] /= (neuronsInLayer5 * neuronsInLayer5);
        if (nodeValues4[i] > 127)
        {
            nodeValues4[i] = 127;
        }
        if (nodeValues4[i] < -127)
        {
            nodeValues4[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer4; i++)
    {
        nodeValues4[i] *= 2;
    }
    //////////////////////////////////////
    //W4 update
    for (int h = 0; h < neuronsInLayer3; h++)
    {
        for (int w = 0; w < neuronsInLayer4; w++)
        {
            weightsLayer4[h][w] += (nodeValues4[w] * neuronsLayer3[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer4[h][w] > 127)
            {
                weightsLayer4[h][w] = 127;
            }
            if (weightsLayer4[h][w] < -127)
            {
                weightsLayer4[h][w] = -127;
            }
        }
    }
    //B4 update
    for (int i = 0; i < neuronsInLayer4; i++)
    {
        biasesLayer4[i] += (nodeValues4[i]);
        if (biasesLayer4[i] > 1000)
        {
            biasesLayer4[i] = 1000;
        }
        if (biasesLayer4[i] < -1000)
        {
            biasesLayer4[i] = -1000;
        }
    }
    //Node3 update
    for (int i = 0; i < neuronsInLayer3; i++)
    {
        for (int j = 0; j < neuronsInLayer4; j++)
        {
            nodeValues3[i] += weightsLayer4[i][j] * nodeValues4[j];
        }
        nodeValues3[i] /= (neuronsInLayer4 * neuronsInLayer4);
        if (nodeValues3[i] > 127)
        {
            nodeValues3[i] = 127;
        }
        if (nodeValues3[i] < -127)
        {
            nodeValues3[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer3; i++)
    {
        nodeValues3[i] *= 2;
    }
    //////////////////////////////////////
    //W3 update
    for (int h = 0; h < neuronsInLayer2; h++)
    {
        for (int w = 0; w < neuronsInLayer3; w++)
        {
            weightsLayer3[h][w] += (nodeValues3[w] * neuronsLayer2[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer3[h][w] > 127)
            {
                weightsLayer3[h][w] = 127;
            }
            if (weightsLayer3[h][w] < -127)
            {
                weightsLayer3[h][w] = -127;
            }
        }
    }
    //B3 update
    for (int i = 0; i < neuronsInLayer3; i++)
    {
        biasesLayer3[i] += (nodeValues3[i]);
        if (biasesLayer3[i] > 1000)
        {
            biasesLayer3[i] = 1000;
        }
        if (biasesLayer3[i] < -1000)
        {
            biasesLayer3[i] = -1000;
        }
    }
    //Node2 update
    for (int i = 0; i < neuronsInLayer2; i++)
    {
        for (int j = 0; j < neuronsInLayer3; j++)
        {
            nodeValues2[i] += weightsLayer3[i][j] * nodeValues3[j];
        }
        nodeValues2[i] /= (neuronsInLayer3 * neuronsInLayer3);
        if (nodeValues2[i] > 127)
        {
            nodeValues2[i] = 127;
        }
        if (nodeValues2[i] < -127)
        {
            nodeValues2[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer2; i++)
    {
        nodeValues2[i] *= 2;
    }
    //////////////////////////////////////
    //W2 update
    for (int h = 0; h < neuronsInLayer1; h++)
    {
        for (int w = 0; w < neuronsInLayer2; w++)
        {
            weightsLayer2[h][w] += (nodeValues2[w] * neuronsLayer1[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer2[h][w] > 127)
            {
                weightsLayer2[h][w] = 127;
            }
            if (weightsLayer2[h][w] < -127)
            {
                weightsLayer2[h][w] = -127;
            }
        }
    }
    //B2 update
    for (int i = 0; i < neuronsInLayer2; i++)
    {
        biasesLayer2[i] += (nodeValues2[i]);
        if (biasesLayer2[i] > 1000)
        {
            biasesLayer2[i] = 1000;
        }
        if (biasesLayer2[i] < -1000)
        {
            biasesLayer2[i] = -1000;
        }
    }
    //Node1 update
    for (int i = 0; i < neuronsInLayer1; i++)
    {
        for (int j = 0; j < neuronsInLayer2; j++)
        {
            nodeValues1[i] += weightsLayer2[i][j] * nodeValues2[j];
        }
        nodeValues1[i] /= (neuronsInLayer2 * neuronsInLayer2);
        if (nodeValues1[i] > 127)
        {
            nodeValues1[i] = 127;
        }
        if (nodeValues1[i] < -127)
        {
            nodeValues1[i] = -127;
        }
    }
    //Multiplying by 2; partial derivative
    for (int i = 0; i < neuronsInLayer1; i++)
    {
        nodeValues1[i] *= 2;
    }
    //////////////////////////////////////
    //W1 update
    for (int h = 0; h < neuronsInLayer0; h++)
    {
        for (int w = 0; w < neuronsInLayer1; w++)
        {
            weightsLayer1[h][w] += (nodeValues1[w] * neuronsLayer0[h] * learningRate * (increment / 10 + 1));
            if (weightsLayer1[h][w] > 127)
            {
                weightsLayer1[h][w] = 127;
            }
            if (weightsLayer1[h][w] < -127)
            {
                weightsLayer1[h][w] = -127;
            }
        }
    }
    //B1 update
    for (int i = 0; i < neuronsInLayer1; i++)
    {
        biasesLayer1[i] += (nodeValues1[i]);
        if (biasesLayer1[i] > 1000)
        {
            biasesLayer1[i] = 1000;
        }
        if (biasesLayer1[i] < -1000)
        {
            biasesLayer1[i] = -1000;
        }
    }
}
*/

//BackpropagationHandler
/*
static void nnBackPropagationHandler()
{
    bool isDataDraw = false;
    bool isFirstDataWin = false;
    if (chessPositionsData[0][0] == 0)
    {
        isDataDraw = true;
    }
    else if (chessPositionsData[0][0] == 10)
    {
        isFirstDataWin = true;
    }
    increment = 1;
    while (increment < 99 && increment < moveNumber)
    {
        if (isDataDraw == true)
        {
            for (int h = 0; h < 8; h++)
            {
                for (int w = 0; w < 8; w++)
                {
                    chessBoard[h][w] = chessPositionsData[increment][(h * 8 + w)];
                }
            }
            nnBoardToInput();
            nnThink();
            nnWhatMoveToPlay();
            nnCalculateCost(true, false, 0, 0, 0, 0);
            nnBackPropagation();
            increment++;
        }
        else
        {
            if (isFirstDataWin == true)
            {
                for (int h = 0; h < 8; h++)
                {
                    for (int w = 0; w < 8; w++)
                    {
                        chessBoard[h][w] = chessPositionsData[increment][(h * 8 + w)];
                    }
                }
                nnBoardToInput();
                nnThink();
                nnWhatMoveToPlay();
                nnCalculateCost(false, true, tempHowItsFrom, tempHowItsTo, chessPositionsData[increment][64], chessPositionsData[increment][65]);
                nnBackPropagation();
                increment++;

                for (int h = 0; h < 8; h++)
                {
                    for (int w = 0; w < 8; w++)
                    {
                        chessBoard[h][w] = chessPositionsData[increment][(h * 8 + w)];
                    }
                }

                nnBoardToInput();
                nnThink();
                nnWhatMoveToPlay();
                nnCalculateCost(false, false, tempHowItsFrom, tempHowItsTo, 0, 0);
                nnBackPropagation();
                increment++;
            }
            else
            {
                for (int h = 0; h < 8; h++)
                {
                    for (int w = 0; w < 8; w++)
                    {
                        chessBoard[h][w] = chessPositionsData[increment][(h * 8 + w)];
                    }
                }
                nnBoardToInput();
                nnThink();
                nnWhatMoveToPlay();
                nnCalculateCost(false, false, tempHowItsFrom, tempHowItsTo, 0, 0);
                nnBackPropagation();
                increment++;

                for (int h = 0; h < 8; h++)
                {
                    for (int w = 0; w < 8; w++)
                    {
                        chessBoard[h][w] = chessPositionsData[increment][(h * 8 + w)];
                    }
                }
                nnBoardToInput();
                nnThink();
                nnWhatMoveToPlay();
                nnCalculateCost(false, true, tempHowItsFrom, tempHowItsTo, chessPositionsData[increment][64], chessPositionsData[increment][65]);
                nnBackPropagation();
                increment++;

            }
        }      
    }
    resetButton();
}
*/


// Warstwa 9 Neuronow = 0; nie wiem czemu nie dziala
// Cost jest albo -127 albo 1 
// Node Values sa w pizde wysokie
//Cale NNThink do przerobienia
//Jedyne co troche dziala to Warstwa 1, wartosci sa bliskie zeru +- 5
//Wszystkie kolejne to same zera

//Main
int main(int, char**)
{
    //Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Chesster v1", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Chesster v1", WS_OVERLAPPEDWINDOW, 100, 100, windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    //Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    //Disabling Host Window Resizing
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    SetWindowLong(hwnd, GWL_STYLE, style); 

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    
    //Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     //Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      //Enable Gamepad Controls

    //Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
   
    //Color
    ImVec4 clear_color = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);

    //Loading assets
    int my_image_width = 60;
    int my_image_height = 60;
    ID3D11ShaderResourceView* squareb = NULL; LoadTextureFromFile("assets/squareb.png", &squareb, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* squarec = NULL; LoadTextureFromFile("assets/squarec.png", &squarec, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* squared = NULL; LoadTextureFromFile("assets/squared.png", &squared, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* squarel = NULL; LoadTextureFromFile("assets/squarel.png", &squarel, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* squarer = NULL; LoadTextureFromFile("assets/squarer.png", &squarer, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* bb = NULL; LoadTextureFromFile("assets/bb.png", &bb, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* bk = NULL; LoadTextureFromFile("assets/bk.png", &bk, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* bn = NULL; LoadTextureFromFile("assets/bn.png", &bn, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* bp = NULL; LoadTextureFromFile("assets/bp.png", &bp, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* bq = NULL; LoadTextureFromFile("assets/bq.png", &bq, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* br = NULL; LoadTextureFromFile("assets/br.png", &br, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wb = NULL; LoadTextureFromFile("assets/wb.png", &wb, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wk = NULL; LoadTextureFromFile("assets/wk.png", &wk, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wn = NULL; LoadTextureFromFile("assets/wn.png", &wn, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wp = NULL; LoadTextureFromFile("assets/wp.png", &wp, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wq = NULL; LoadTextureFromFile("assets/wq.png", &wq, &my_image_width, &my_image_height);
    ID3D11ShaderResourceView* wr = NULL; LoadTextureFromFile("assets/wr.png", &wr, &my_image_width, &my_image_height);
    
    boardBeginWhite();
    nnWriteWithRandomValues();
    //nnLoad();

    int waitForFrame = 0;
    //Main loop
    bool done = false;
    while (!done)
    {
        //Required
        
        //Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }
        if (done)
        {
            //nnWrite();
            break;
        }
        //Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        //Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        //Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Chess", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        //Drawing Black and White Squares
        int starterBoardPosX = 80;
        int starterBoardPosY = 100;
        for (int w = 0; w < 8; w++)
        {
            for (int h = 0; h < 8; h++)
            {
                ImGui::SetCursorPos(ImVec2((w*60)+starterBoardPosX, (h*60)+starterBoardPosY));
                if (isChessSquareWhite[h][w] == true)
                {
                    ImGui::Image((void*)squarel, ImVec2(my_image_width, my_image_height));
                }
                else
                {
                    ImGui::Image((void*)squared, ImVec2(my_image_width, my_image_height));
                }
            }
        }

        //Drawing Viable Moves
        for (int w = 0; w < 8; w++)
        {
            for (int h = 0; h < 8; h++)
            {
                ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
                if (isChessSquareViableMove[h][w] == true)
                {
                    ImGui::Image((void*)squareb, ImVec2(my_image_width, my_image_height));
                }
            }
        }

        //Drawing Check Indicator
        if (isWhiteKingInCheck == true)
        {
            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    if (chessBoard[h][w] == 100)
                    {
                        ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
                        ImGui::Image((void*)squarer, ImVec2(my_image_width, my_image_height));
                    }
                }
            }
        }
        if (isBlackKingInCheck == true)
        {
            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    if (chessBoard[h][w] == 101)
                    {
                        ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
                        ImGui::Image((void*)squarer, ImVec2(my_image_width, my_image_height));
                    }
                }
            }
        }

        //Drawing Selected Square
        if (isChessPieceSelected == true)
        {
            ImGui::SetCursorPos(ImVec2((selectedPieceX * 60) + starterBoardPosX, (selectedPieceY * 60) + starterBoardPosY));
            ImGui::Image((void*)squarec, ImVec2(my_image_width, my_image_height));
        }

        //Drawing Chess Pieces
        for (int w = 0; w < 8; w++)
        {
            for (int h = 0; h < 8; h++)
            {
                ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
                switch (chessBoard[h][w])
                {
                default:
                    break;
                case 0:
                    break;
                case 10:
                    ImGui::Image((void*)wp, ImVec2(my_image_width, my_image_height));
                    break;
                case 11:
                    ImGui::Image((void*)bp, ImVec2(my_image_width, my_image_height));
                    break;
                case 30:
                    ImGui::Image((void*)wn, ImVec2(my_image_width, my_image_height));
                    break;
                case 31:
                    ImGui::Image((void*)bn, ImVec2(my_image_width, my_image_height));
                    break;
                case 32:
                    ImGui::Image((void*)wb, ImVec2(my_image_width, my_image_height));
                    break;
                case 33:
                    ImGui::Image((void*)bb, ImVec2(my_image_width, my_image_height));
                    break;
                case 50:
                    ImGui::Image((void*)wr, ImVec2(my_image_width, my_image_height));
                    break;
                case 51:
                    ImGui::Image((void*)br, ImVec2(my_image_width, my_image_height));
                    break;
                case 90:
                    ImGui::Image((void*)wq, ImVec2(my_image_width, my_image_height));
                    break;
                case 91:
                    ImGui::Image((void*)bq, ImVec2(my_image_width, my_image_height));
                    break;
                case 100:
                    ImGui::Image((void*)wk, ImVec2(my_image_width, my_image_height));
                    break;
                case 101:
                    ImGui::Image((void*)bk, ImVec2(my_image_width, my_image_height));
                    break; 
                }
            }
        }

        //Buttons
        if (isGameStarted == false)
        {
            ImGui::SetCursorPos(ImVec2(my_image_width * 8 + starterBoardPosX + 30, my_image_height * 4 + starterBoardPosY - 30));
            if (ImGui::Button("Flip Board", ImVec2(100, 60)))
            {
                boardFlip();
            }
        }
        ImGui::SetCursorPos(ImVec2(my_image_width * 8 + starterBoardPosX + 160, my_image_height * 4 + starterBoardPosY - 30));
        if (ImGui::Button("Reset", ImVec2(60, 60)))
        {
            resetButton();
        }

        ImGui::SetCursorPos(ImVec2(starterBoardPosX, starterBoardPosY - 50));
        ImGui::Text("Input new FEN position:");
        ImGui::SetCursorPos(ImVec2(starterBoardPosX, starterBoardPosY - 30));
        ImGui::SetNextItemWidth(480);
        ImGui::InputText(" ", FENinput, fenLength);
        
        ImGui::SetCursorPos(ImVec2(starterBoardPosX + 480 + 10, starterBoardPosY - 30));
        if (ImGui::Button("Confirm FEN", ImVec2(100, 20)))
        {
            if (isGameStarted == false)
            {
                fenToBoardPosition(FENinput);
                for (int i = 0; i < 100; i++)
                {
                    FENinput[i] = 0;
                }
            }
        }

       
        ImGui::SetCursorPos(ImVec2(starterBoardPosX +510, starterBoardPosY+120));
        ImGui::Checkbox("Start AI from here", &isAITurnedOn);

        if (isAITurnedOn == true || isHumanPlayingAgainstAI == true)
        {
            isAITurnedOn = true;
            isHumanPlayingAgainstAI = true;
            if (isGameStarted == false && isHumanWhite == false)
            {
                isChessPieceSelected = false;
            }
        }
      
        if (isAITurnedOn == true)
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 160));
            ImGui::Text("You are playing against AI.");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 180));
            ImGui::Text("Good Luck.");
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 160));
            ImGui::Text("You are playing against You.");
        }
//Text
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 300));
            ImGui::Text("Game starts after first move.");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 320));
            ImGui::Text("Once the game is started you");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 340));
            ImGui::Text("can't switch sides but you");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 360));
            ImGui::Text("can at any time turn on AI.");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 380));
            ImGui::Text("After that you play against it");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 400));
            ImGui::Text("to the end of the game.");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 505, starterBoardPosY + 495));
            ImGui::Text("Disclaimer: Pawn promotion is");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 505, starterBoardPosY + 515));
            ImGui::Text("always to the Queen.");
            ImGui::Text("%d", moveNumber);

        if (isHumanWhite == true)
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX, starterBoardPosY + 485));
            ImGui::Text("You are playing with white.");
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX, starterBoardPosY + 485));
            ImGui::Text("You are playing with black.");
        }

        if (gameEnded == true)
        {
            ImGui::SetCursorPos(ImVec2(my_image_width * 8 + starterBoardPosX + 30, my_image_height * 4 + starterBoardPosY - 7));
            if (didHumanWin == true)
            {
                ImGui::Text("It's a Win :)");
            }
            else
            {
                if (isItDraw == true)
                {
                    ImGui::Text("It's a Draw :|");
                }
                else
                {
                    ImGui::Text("It's a Loss :( ");
                }
            }
        }

        //Backpropagation after game's end
        if (gameEnded == true)
        {
            if (isItDraw == true)
            {
                chessPositionsData[0][0] = 0;
            }
            else if (wasHumanFirstInData == true)
            {
                if (didHumanWin == true)
                {
                    chessPositionsData[0][0] = 10;
                }
                else
                {
                    chessPositionsData[0][0] = -10;
                }
            }
            else if (wasHumanFirstInData == false)
            {
                if (didHumanWin == true)
                {
                    chessPositionsData[0][0] = -10;
                }
                else
                {
                    chessPositionsData[0][0] = 10;
                }
            }

            //isBackPropagationRunning = true;
            //nnBackPropagationHandler();
            //isBackPropagationRunning = false;
        }

        if (gameEnded == false)
        {
            if (isAITurnedOn == true && waitForFrame > 5)
            {
                if (isHumanWhite == true)
                {
                    if (isWhitesTurn == false)
                    {
                        //nnBoardToInput();
                        //nnThink();
                        //nnWhatMoveToPlay();
                    }   
                }
                else
                {
                    if (isWhitesTurn == true)
                    {
                        //nnBoardToInput();
                        //nnThink();
                        //nnWhatMoveToPlay();
                    }
                }
            }
            //Moving Chess Pieces
            ImVec2 mousePosition = ImGui::GetMousePos();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mousePosition.x > 80 && mousePosition.x < 560 && mousePosition.y > 100 && mousePosition.y < 580)
            {
                int X = (mousePosition.x - 80) / 60;
                int Y = (mousePosition.y - 100) / 60;
                //std::cout << "Click: " << Y << "," << X << "\n";

                //Piece Move for both colors
                if (isChessPieceSelected == true && isChessSquareViableMove[Y][X] == true)
                {   //chess Positions data for nn
                    if (isAITurnedOn == true)
                    {
                        moveNumber++;
                        if (moveNumber == 1)
                        {
                            wasHumanFirstInData = true;
                        }
                        if (isHumanWhite == false)
                        {
                            boardFlip();
                            for (int h = 0; h < 8; h++)
                            {
                                for (int w = 0; w < 8; w++)
                                {
                                    chessPositionsData[moveNumber][h * 8 + w] = chessBoard[h][w];
                                }
                            }
                            boardFlip();
                        }
                        else
                        {
                            for (int h = 0; h < 8; h++)
                            {
                                for (int w = 0; w < 8; w++)
                                {
                                    chessPositionsData[moveNumber][h * 8 + w] = chessBoard[h][w];
                                }
                            }
                        }
                    }
                    int whatPieceToMove = chessBoard[selectedPieceY][selectedPieceX];
                    chessBoard[selectedPieceY][selectedPieceX] = 0;
                    chessBoard[Y][X] = whatPieceToMove;
                    if (isGameStarted == false) { isGameStarted = true; }
                    if (isAITurnedOn == true)
                    {
                        if (isHumanWhite == false)
                        {
                            boardFlip();
                            chessPositionsData[moveNumber][64] = selectedPieceY * 8 + selectedPieceX;
                            chessPositionsData[moveNumber][65] = Y * 8 + X;
                            boardFlip();
                        }
                        else
                        {
                            chessPositionsData[moveNumber][64] = selectedPieceY * 8 + selectedPieceX;
                            chessPositionsData[moveNumber][65] = Y * 8 + X;
                        }
                    }

                    //Rook move while castling
                    if (whatPieceToMove == 100)
                    {
                        if (isHumanWhite == true)
                        {
                            if (isWhiteKingCastlingPossible == true && Y == 7 && X == 6)
                            {
                                chessBoard[7][5] = 50;
                                chessBoard[7][7] = 0;
                            }
                            if (isWhiteQueenCastlingPossible == true && Y == 7 && X == 2)
                            {
                                chessBoard[7][3] = 50;
                                chessBoard[7][0] = 0;
                            }
                        }
                        else
                        {
                            if (isWhiteKingCastlingPossible == true && Y == 0 && X == 1)
                            {
                                chessBoard[0][2] = 50;
                                chessBoard[0][0] = 0;
                            }
                            if (isWhiteQueenCastlingPossible == true && Y == 0 && X == 5)
                            {
                                chessBoard[0][4] = 50;
                                chessBoard[0][7] = 0;
                            }
                        }
                    }
                    if (whatPieceToMove == 101)
                    {
                        if (isHumanWhite == true)
                        {
                            if (isBlackKingCastlingPossible == true && Y == 0 && X == 6)
                            {
                                chessBoard[0][5] = 51;
                                chessBoard[0][7] = 0;
                            }
                            if (isBlackQueenCastlingPossible == true && Y == 0 && X == 2)
                            {
                                chessBoard[0][3] = 51;
                                chessBoard[0][0] = 0;
                            }
                        }
                        else
                        {
                            if (isBlackKingCastlingPossible == true && Y == 7 && X == 1)
                            {
                                chessBoard[7][2] = 51;
                                chessBoard[7][0] = 0;
                            }
                            if (isBlackQueenCastlingPossible == true && Y == 7 && X == 5)
                            {
                                chessBoard[7][4] = 51;
                                chessBoard[7][7] = 0;
                            }
                        }
                    }

                    //Remove possibility of castling
                    switch (whatPieceToMove)
                    {
                    default:
                        break;
                    case 100:
                        isWhiteKingCastlingPossible = false;
                        isWhiteQueenCastlingPossible = false;
                        break;
                    case 101:
                        isBlackKingCastlingPossible = false;
                        isBlackQueenCastlingPossible = false;
                        break;
                    case 50:
                        if (isHumanWhite == true)
                        {
                            if (selectedPieceX == 7) { isWhiteKingCastlingPossible = false;}
                            if (selectedPieceX == 0) { isWhiteQueenCastlingPossible = false; }
                        }
                        if (isHumanWhite == false)
                        {
                            if (selectedPieceX == 0) { isWhiteKingCastlingPossible = false; }
                            if (selectedPieceX == 7) { isWhiteQueenCastlingPossible = false; }
                        }
                        break;
                    case 51:
                        if (isHumanWhite == false)
                        {
                            if (selectedPieceX == 0) { isBlackKingCastlingPossible = false; }
                            if (selectedPieceX == 7) { isBlackQueenCastlingPossible = false; }
                        }
                        if (isHumanWhite == true)
                        {
                            if (selectedPieceX == 7) { isBlackKingCastlingPossible = false; }
                            if (selectedPieceX == 0) { isBlackQueenCastlingPossible = false; }
                        }
                        break;
                    }

                    //En Passant pawn capture
                    if (whatPieceToMove == 10)
                    {
                        if (isHumanWhite == true)
                        {
                            if (isEnPassantForWhitePossible == true && Y == 2)
                            {
                                chessBoard[Y + 1][X] = 0;
                            }
                        }
                        else
                        {
                            if (isEnPassantForWhitePossible == true && Y == 5)
                            {
                                chessBoard[Y - 1][X] = 0;
                            }
                        }
                    }
                    if (whatPieceToMove == 11)
                    {
                        if (isHumanWhite == true)
                        {
                            if (isEnPassantForBlackPossible == true && Y == 5)
                            {
                                chessBoard[Y - 1][X] = 0;
                            }
                        }
                        else
                        {
                            if (isEnPassantForBlackPossible == true && Y == 2)
                            {
                                chessBoard[Y + 1][X] = 0;
                            }
                        }
                    }

                    //isEnPassantPossible
                    if (whatPieceToMove == 10)
                    {
                        if (isHumanWhite == true)
                        {
                            if (selectedPieceY == 6 && Y == 4)
                            {
                                isEnPassantForBlackPossible = true;
                                enPassantX = X;
                            }
                        }
                        else
                        {
                            if (selectedPieceY == 1 && Y == 3)
                            {
                                isEnPassantForBlackPossible = true;
                                enPassantX = X;
                            }
                        }
                    }
                    else
                    {
                        isEnPassantForBlackPossible = false;
                    }
                    if (whatPieceToMove == 11)
                    {
                        if (isHumanWhite == true)
                        {
                            if (selectedPieceY == 1 && Y == 3)
                            {
                                isEnPassantForWhitePossible = true;
                                enPassantX = X;
                            }
                        }
                        else
                        {
                            if (selectedPieceY == 6 && Y == 4)
                            {
                                isEnPassantForWhitePossible = true;
                                enPassantX = X;
                            }
                        }
                    }
                    else
                    {
                        isEnPassantForWhitePossible = false;
                    }

                    if (isWhitesTurn == true) { isWhitesTurn = false; }
                    else { isWhitesTurn = true; }

                    resetisChessSquareViableMove();
                    updatePawnToQueenPromotion();
                    updateIsKingInCheck();
                    isGameEnded();
                }

                //Piece Deselection
                if (isChessPieceSelected == true && isChessSquareViableMove[Y][X] == false)
                {
                    isChessPieceSelected = false;
                    resetisChessSquareViableMove();
                    //std::cout << "Deselection\n";
                }

                //Piece Selection
                if (isChessPieceSelected == false)
                {
                    //White
                    if (isWhitesTurn == true && chessBoard[Y][X] != 0 && (chessBoard[Y][X] % 10 == 0 || chessBoard[Y][X] % 10 == 2))
                    {
                        isChessPieceSelected = true;
                        selectedPieceX = X;
                        selectedPieceY = Y;
                        updateViableMoves();
                        //std::cout << "White Selection\n";
                    }
                    //Black
                    if (isWhitesTurn == false && chessBoard[Y][X] != 0 && (chessBoard[Y][X] % 10 == 1 || chessBoard[Y][X] % 10 == 3))
                    {
                        isChessPieceSelected = true;
                        selectedPieceX = X;
                        selectedPieceY = Y;
                        updateViableMoves();
                        //std::cout << "Black Selection\n";
                    }
                }
                waitForFrame = 0;
            }
        }

        ImGui::End();
        //Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        //Present
        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        waitForFrame++;
    }
    //Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

//Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

//Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); //Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) //Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

