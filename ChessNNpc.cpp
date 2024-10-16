#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include "cstring";
#include <fstream>
#include <random>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#pragma comment(lib, "D3D11.lib")
#pragma warning(disable : 4996)
#pragma warning(disable : 6386)

//TODO

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

int chessBoard[8][8][9];
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

bool isBoardFlipped = false;
const int fenLength = 100;
char FENinput[fenLength] = "";
bool isHumanPlayingAgainstAI = false;
bool isAITurnedOn = false;
bool isWhitesTurn = true;
bool gameEnded = false;
bool didWhiteWin = false;
bool isItDraw = false;
bool isAnyMoveViable = true;
bool isWhiteKingCastlingPossible = true;
bool isWhiteQueenCastlingPossible = true;
bool isBlackKingCastlingPossible = true;
bool isBlackQueenCastlingPossible = true;
bool isEnPassantForWhitePossible = false;
bool isEnPassantForBlackPossible = false;
int enPassantX = 0;

int waitForFrame = 0;
bool dangerSquaresForWhiteKing[8][8];
bool isWhiteKingInCheck = false;
bool dangerSquaresForBlackKing[8][8];
bool isBlackKingInCheck = false;
int hypotheticalBoard[8][8];
int hypotheticalBoardForWhiteKing[8][8];
int hypotheticalBoardForBlackKing[8][8];
bool isMoveHypothetical = false;
int moveNumber = 0;
const int moveLimit = 99;
int lastMoves[12][2];
int maxIntValue = 2147483647;

float normalEvaluation = 0.0f;
float moreAccurateEvaluation = 0.0f;

int maxDepth = 8;
float best = 0;
int bestMoveFromX = 0;
int bestMoveFromY = 0;
int bestMoveToX = 0;
int bestMoveToY = 0;

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



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void pieceMove(int Y, int X, int whichBoard);

static void resetButton();

static bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
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

//Chess app functions
static void boardBeginWhite()
{
    for (int j = 0; j < 9; j++)
    {
        chessBoard[0][0][j] = 51, chessBoard[0][1][j] = 31, chessBoard[0][2][j] = 33, chessBoard[0][3][j] = 91;
        chessBoard[0][4][j] = 101, chessBoard[0][5][j] = 33, chessBoard[0][6][j] = 31, chessBoard[0][7][j] = 51;
        for (int i = 0; i < 8; i++)
        {
            chessBoard[1][i][j] = 11;
            chessBoard[2][i][j] = 0;
            chessBoard[3][i][j] = 0;
            chessBoard[4][i][j] = 0;
            chessBoard[5][i][j] = 0;
            chessBoard[6][i][j] = 10;
        }
        chessBoard[7][0][j] = 50, chessBoard[7][1][j] = 30, chessBoard[7][2][j] = 32, chessBoard[7][3][j] = 90;
        chessBoard[7][4][j] = 100, chessBoard[7][5][j] = 32, chessBoard[7][6][j] = 30, chessBoard[7][7][j] = 50;
        isBoardFlipped = false;
    }
}

static void boardReset()
{
    for (int i = 0; i < 9; i++)
    {
        for (int w = 0; w < 8; w++)
        {
            for (int h = 0; h < 8; h++)
            {
                chessBoard[h][w][i] = 0;
            }
        }
    }
}

static void resetIsChessSquareViableMove()
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
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 10;
            chessBoardFENX++;
            break;
        case 'p':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 11;
            chessBoardFENX++;
            break;
        case 'N':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 30;
            chessBoardFENX++;
            break;
        case 'n':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 31;
            chessBoardFENX++;
            break;
        case 'B':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 32;
            chessBoardFENX++;
            break;
        case 'b':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 33;
            chessBoardFENX++;
            break;
        case 'R':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 50;
            chessBoardFENX++;
            break;
        case 'r':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 51;
            chessBoardFENX++;
            break;
        case 'Q':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 90;
            chessBoardFENX++;
            break;
        case 'q':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 91;
            chessBoardFENX++;
            break;
        case 'K':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 100;
            chessBoardFENX++;
            break;
        case 'k':
            chessBoard[chessBoardFENY][chessBoardFENX][0] = 101;
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

static void updatePawnToQueenPromotion(int whichBoard)
{
    for (int w = 0; w < 8; w++)
    {
        if (chessBoard[0][w][whichBoard] == 10)
        {
            chessBoard[0][w][whichBoard] = 90;
        }

        if (chessBoard[7][w][whichBoard] == 11)
        {
            chessBoard[7][w][whichBoard] = 91;
        }
    }
}

static void makeHypotheticalMove(int fromY, int fromX, int toY, int toX, int whichBoard)
{
    isMoveHypothetical = true;

    for (int h = 0; h < 8; h++)
    {
        for (int w = 0; w < 8; w++)
        {
            hypotheticalBoard[h][w] = chessBoard[h][w][whichBoard];
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

static void updateDangerSquares(int whichBoard)
{
    int temporaryChessBoard[8][8];

    if (isMoveHypothetical == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = chessBoard[h][w][whichBoard];
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
                        if (w != 0 && h != 0)
                        {
                            dangerSquaresForBlackKing[h - 1][w - 1] = true;
                        }
                        if (w != 7 && h != 0)
                        {
                            dangerSquaresForBlackKing[h - 1][w + 1] = true;
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
                        if (w != 0 && h != 7)
                        {
                            dangerSquaresForWhiteKing[h + 1][w - 1] = true;
                        }
                        if (w != 7 && h != 7)
                        {
                            dangerSquaresForWhiteKing[h + 1][w + 1] = true;
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

static void updateIsKingInCheck(int whichBoard)
{
    resetDangerSquares();
    updateDangerSquares(whichBoard);

    int temporaryChessBoard[8][8];

    if (isMoveHypothetical == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                temporaryChessBoard[h][w] = chessBoard[h][w][whichBoard];
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

static void updateCastlingWhenRookIsCaptured(int whichBoard)
{
    if (isWhiteKingCastlingPossible == true)
    {
        if (chessBoard[7][7][whichBoard] != 50)
        {
            isWhiteKingCastlingPossible = false;
        }
    }
    if (isWhiteQueenCastlingPossible == true)
    {
        if (chessBoard[7][0][whichBoard] != 50)
        {
            isWhiteQueenCastlingPossible = false;
        }
    }

    if (isBlackKingCastlingPossible == true)
    {
        if (chessBoard[0][7][whichBoard] != 51)
        {
            isBlackKingCastlingPossible = false;
        }
    }
    if (isBlackQueenCastlingPossible == true)
    {
        if (chessBoard[0][0][whichBoard] != 51)
        {
            isBlackQueenCastlingPossible = false;
        }
    }
}

static void updateViableMoves(int whichBoard)
{
    updateIsKingInCheck(whichBoard);
    updateCastlingWhenRookIsCaptured(whichBoard);

    if (isChessPieceSelected == true)
    {
        switch (chessBoard[selectedPieceY][selectedPieceX][whichBoard])
        {
//White Pawn Movement:
        case 10:
            //Double move
            if (selectedPieceY == 6 && chessBoard[selectedPieceY - 2][selectedPieceX][whichBoard] == 0 && chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] == 0)
            { 
                makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX, whichBoard);
                updateIsKingInCheck(whichBoard);
                if (isWhiteKingInCheck == false)
                {
                    isChessSquareViableMove[selectedPieceY - 2][selectedPieceX] = true;
                }
                isMoveHypothetical = false;
            }
            //Single move
            if (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] == 0)
            {
                makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX, whichBoard);
                updateIsKingInCheck(whichBoard);
                if (isWhiteKingInCheck == false)
                {
                    isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                }
                isMoveHypothetical = false; 
            }
            //Attacking
            if (selectedPieceX != 0)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) != 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1,whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
                //EnPassant
                if (isEnPassantForWhitePossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX != 7) {
                if (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) != 0 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) != 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
                //EnPassant
                if (isEnPassantForWhitePossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            break;
//Black Pawn Movement
        case 11:
            //Double move
            if (selectedPieceY == 1 && chessBoard[selectedPieceY + 2][selectedPieceX][whichBoard] == 0 && chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] == 0)
            { 
                makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX, whichBoard);
                updateIsKingInCheck(whichBoard);
                if (isBlackKingInCheck == false)
                {
                    isChessSquareViableMove[selectedPieceY + 2][selectedPieceX] = true;
                }
                isMoveHypothetical = false;
            }
            //Single move
            if (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] == 0)
            { 
                makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX, whichBoard);
                updateIsKingInCheck(whichBoard);
                if (isBlackKingInCheck == false)
                {
                    isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                }
                isMoveHypothetical = false;
            }
            //Attacking
            if (selectedPieceX != 0)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) != 1 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) != 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
                //EnPassant
                if (isEnPassantForBlackPossible == true && (selectedPieceX - 1) == enPassantX && selectedPieceY == 4)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX != 7) {
                if (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] != 0 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) != 1 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) != 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
                //EnPassant
                if (isEnPassantForBlackPossible == true && (selectedPieceX + 1) == enPassantX && selectedPieceY == 4)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            break;
//White Knight Movement from Upper Right Corner Counterclockwise
        case 30: 
            if (selectedPieceY > 1 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 1 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX + 1][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 1 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 2][selectedPieceX - 1][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 2, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 2][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 1)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 2][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX > 0)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX - 1][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 6 && selectedPieceX < 7)
            {
                if (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 2][selectedPieceX + 1][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 2, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 2][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 2][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 2] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX < 6)
            {
                if (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 2][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 2, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                    if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isWhiteKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][tempX][whichBoard] % 10) == 0 && chessBoard[tempY][tempX][whichBoard] != 0) || (chessBoard[tempY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][tempX][whichBoard] % 10) == 1 || (chessBoard[tempY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                    if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[tempY][selectedPieceX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                    if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
                        if (isBlackKingInCheck == false)
                        {
                            isChessSquareViableMove[selectedPieceY][tempX] = true;
                        }
                        isMoveHypothetical = false;
                        break;
                    }
                    if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                    {
                        break;
                    }

                    if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                    {
                        makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                        updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 0 && chessBoard[tempY][selectedPieceX][whichBoard] != 0) || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[tempY][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[tempY][selectedPieceX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[tempY][selectedPieceX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, tempY, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
                if (((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 0 && chessBoard[selectedPieceY][tempX][whichBoard] != 0) || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 2)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][tempX] = true;
                    }
                    isMoveHypothetical = false;
                    break;
                }
                if ((chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][tempX][whichBoard] % 10) == 3)
                {
                    break;
                }

                if (chessBoard[selectedPieceY][tempX][whichBoard] == 0)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, tempX, whichBoard);
                    updateIsKingInCheck(whichBoard);
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
            if (selectedPieceY > 0 && selectedPieceX < 7 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY - 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX > 0 && (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY + 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX < 7 && (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] % 10) == 1 || (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] % 10) == 3))
            {
                if (dangerSquaresForWhiteKing[selectedPieceY][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isWhiteKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }

            //Castling
            updateIsKingInCheck(whichBoard);
            if (isWhiteKingInCheck == false)
            {
                if (isWhiteKingCastlingPossible == true && chessBoard[7][5][whichBoard] == 0 && chessBoard[7][6][whichBoard] == 0)
                {
                    if (dangerSquaresForWhiteKing[7][5] == false && dangerSquaresForWhiteKing[7][6] == false)
                    {
                        isChessSquareViableMove[7][6] = true;
                    }
                }
                if (isWhiteQueenCastlingPossible == true && chessBoard[7][1][whichBoard] == 0 && chessBoard[7][2][whichBoard] == 0 && chessBoard[7][3][whichBoard] == 0)
                {
                    if (dangerSquaresForWhiteKing[7][2] == false && dangerSquaresForWhiteKing[7][3] == false)
                    {
                        isChessSquareViableMove[7][2] = true;
                    }
                }
            }
        }
        break;
//Black King from Upper Right Counterclockwise
        case 101:
        {
            if (selectedPieceY > 0 && selectedPieceX < 7 && (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX + 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY > 0 && selectedPieceX > 0 && (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY - 1][selectedPieceX - 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY - 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY - 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY - 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX > 0 && (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY][selectedPieceX - 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX > 0 && (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX - 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX - 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX - 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX - 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceY < 7 && selectedPieceX < 7 && (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY + 1][selectedPieceX + 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY + 1][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY + 1, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY + 1][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }
            if (selectedPieceX < 7 && (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] % 10) == 0 || (chessBoard[selectedPieceY][selectedPieceX + 1][whichBoard] % 10) == 2))
            {
                if (dangerSquaresForBlackKing[selectedPieceY][selectedPieceX + 1] == false)
                {
                    makeHypotheticalMove(selectedPieceY, selectedPieceX, selectedPieceY, selectedPieceX + 1, whichBoard);
                    updateIsKingInCheck(whichBoard);
                    if (isBlackKingInCheck == false)
                    {
                        isChessSquareViableMove[selectedPieceY][selectedPieceX + 1] = true;
                    }
                    isMoveHypothetical = false;
                }
            }

            //Castling
            updateIsKingInCheck(whichBoard);
            if (isBlackKingInCheck == false)
            {
                if (isBlackKingCastlingPossible == true && chessBoard[0][5][whichBoard] == 0 && chessBoard[0][6][whichBoard] == 0)
                {
                    if (dangerSquaresForBlackKing[0][5] == false && dangerSquaresForBlackKing[0][6] == false)
                    {
                        isChessSquareViableMove[0][6] = true;
                    }
                }
                if (isBlackQueenCastlingPossible == true && chessBoard[0][1][whichBoard] == 0 && chessBoard[0][2][whichBoard] == 0 && chessBoard[0][3][whichBoard] == 0)
                {
                    if (dangerSquaresForBlackKing[0][2] == false && dangerSquaresForBlackKing[0][3] == false)
                    {
                        isChessSquareViableMove[0][2] = true;
                    }
                }
            }
        }
        break;
        }
    }
    isMoveHypothetical = false;
    updateIsKingInCheck(whichBoard);
}

static void isGameEnded(int whichBoard)
{
    isAnyMoveViable = false;
    isChessPieceSelected = true;
    if (moveNumber >= moveLimit)
    {
        gameEnded = true;
    }

    //repetition move draw
    if (lastMoves[0][0] == lastMoves[4][0] && lastMoves[4][0] == lastMoves[8][0] && lastMoves[0][1] == lastMoves[4][1] && lastMoves[4][1] == lastMoves[8][1])
    {
        if (lastMoves[1][0] == lastMoves[5][0] && lastMoves[5][0] == lastMoves[9][0] && lastMoves[1][1] == lastMoves[5][1] && lastMoves[5][1] == lastMoves[9][1])
        {
            gameEnded = true;
            isItDraw = true;
        }
    }


    if (gameEnded == false)
    {
        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                if (isWhitesTurn == true)
                {
                    if (chessBoard[h][w][whichBoard] != 0 && ((chessBoard[h][w][whichBoard] % 10) == 0 || (chessBoard[h][w][whichBoard] % 10) == 2))
                    {
                        selectedPieceY = h;
                        selectedPieceX = w;
                        updateViableMoves(whichBoard);
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
                    if (chessBoard[h][w][whichBoard] != 0 && ((chessBoard[h][w][whichBoard] % 10) == 1 || (chessBoard[h][w][whichBoard] % 10) == 3))
                    {
                        selectedPieceY = h;
                        selectedPieceX = w;
                        updateViableMoves(whichBoard);
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
        waitForFrame = 0;
        if (isWhitesTurn == true)
        {
            if (isWhiteKingInCheck == true)
            {
                didWhiteWin = false;
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
                didWhiteWin = true;
            }
            else
            {
                isItDraw = true;
            }
        }
    }

    isChessPieceSelected = false;
    resetIsChessSquareViableMove();
}

static void evaluation(int whichBoard)
{
    normalEvaluation = 0.0f;
    moreAccurateEvaluation = 0.0f;
    if (gameEnded == true)
    {
        if (isItDraw == true)
        {
            normalEvaluation = 0.0f;
            moreAccurateEvaluation = 0.0f;
        }
        else
        {
            if (didWhiteWin == true)
            {
                normalEvaluation = 1000.0f;
                moreAccurateEvaluation = 1000.0f;
            }
            else
            {
                normalEvaluation = -1000.0f;
                moreAccurateEvaluation = -1000.0f;
            }
        }
    }
    else
    {
        float whiteKingValues[8][8] =
        {
            { -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 },
            { -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 },
            { -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 },
            { -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 },
            { -2.0, -3.0, -3.0, -4.0, -4.0, -3.0, -3.0, -3.0 },
            { -1.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -1.0 },
            {  2.0,  2.0,  0.0,  0.0,  0.0,  0.0,  2.0,  2.0 },
            {  2.0,  3.0,  1.0,  0.0,  0.0,  1.0,  3.0,  2.0 }
        };
        float blackKingValues[8][8] =
        {
            { -2.0, -3.0, -1.0,  0.0,  0.0, -1.0, -3.0, -2.0 },
            { -2.0, -2.0,  0.0,  0.0,  0.0,  0.0, -2.0, -2.0 },
            {  1.0,  2.0,  2.0,  2.0,  2.0,  2.0,  2.0,  1.0 },
            {  2.0,  3.0,  3.0,  4.0,  4.0,  3.0,  3.0,  3.0 },
            {  3.0,  4.0,  4.0,  5.0,  5.0,  4.0,  4.0,  3.0 },
            {  3.0,  4.0,  4.0,  5.0,  5.0,  4.0,  4.0,  3.0 },
            {  3.0,  4.0,  4.0,  5.0,  5.0,  4.0,  4.0,  3.0 },
            {  3.0,  4.0,  4.0,  5.0,  5.0,  4.0,  4.0,  3.0 }
        };
        float whiteQueenValues[8][8] =
        {
            {-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0, },
            {-1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0, },
            {-1.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0, },
            {-0.5,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5, },
            {-0.5,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5, },
            {-1.0,  0.5,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0, },
            {-1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0, },
            { -2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0}
        };
        float blackQueenValues[8][8] =
        {
            {  2.0,  1.0,  1.0,  0.5,  0.5,  1.0,  1.0,  2.0 },
            {  1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0 },
            {  1.0,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  1.0 },
            {  0.5,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  0.5 },
            {  0.5,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  0.5 },
            {  1.0, -0.5, -0.5, -0.5, -0.5, -0.5,  0.0,  1.0 },
            {  1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0 },
            {  2.0,  1.0,  1.0,  0.5,  0.5,  1.0,  1.0,  2.0 }
        };
        float whiteRookValues[8][8] =
        {
            {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 },
            {0.5,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  0.5 },
            {-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 },
            {-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 },
            {-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 },
            {-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 },
            {-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 },
            {0.0,  0.0,  0.0,  0.5,  0.5,  0.0,  0.0,  0.0 }
        };
        float blackRookValues[8][8] =
        {
            {  0.0,  0.0,  0.0, -0.5, -0.5,  0.0,  0.0,  0.0 },
            {  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5 },
            {  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5 },
            {  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5 },
            {  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5 },
            {  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5 },
            { -0.5, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -0.5 },
            {  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 }
        };
        float whiteBishopValues[8][8] =
        {
            {-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0 },
            {-1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0},
            {-1.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0},
            {-0.5,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5},
            { 0.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5},
            {-1.0,  0.5,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0},
            {-1.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.0, -1.0},
            {-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0}
        };
        float blackBishopValues[8][8] =
        {
            { 2.0,  1.0,  1.0,  0.5,  0.5,  1.0,  1.0,  2.0 },
            { 1.0,  0.0, -0.5,  0.0,  0.0,  0.0,  0.0,  1.0 },
            { 1.0, -0.5, -0.5, -0.5, -0.5, -0.5,  0.0,  1.0 },
            { 0.0,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  0.5 },
            { 0.5,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  0.5 },
            { 1.0,  0.0, -0.5, -0.5, -0.5, -0.5,  0.0,  1.0 },
            { 1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0 },
            { 2.0,  1.0,  1.0,  0.5,  0.5,  1.0,  1.0,  2.0 }
        };
        float whiteKnightValues[8][8] =
        {
            {-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0},
            {-4.0, -2.0,  0.0,  0.0,  0.0,  0.0, -2.0, -4.0},
            {-3.0,  0.0,  1.0,  1.5,  1.5,  1.0,  0.0, -3.0},
            {-3.0,  0.5,  1.5,  2.0,  2.0,  1.5,  0.5, -3.0},
            {-3.0,  0.0,  1.5,  2.0,  2.0,  1.5,  0.0, -3.0},
            {-3.0,  0.5,  1.0,  1.5,  1.5,  1.0,  0.5, -3.0},
            {-4.0, -2.0,  0.0,  0.5,  0.5,  0.0, -2.0, -4.0},
            {-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0}
        };
        float blackKnightValues[8][8] =
        {
            {  5.0,  4.0,  3.0,  3.0,  3.0,  3.0,  4.0,  5.0 },
            {  4.0,  2.0,  0.0, -0.5, -0.5,  0.0,  2.0,  4.0 },
            {  3.0, -0.5, -1.0, -1.5, -1.5, -1.0, -0.5,  3.0 },
            {  3.0,  0.0, -1.5, -2.0, -2.0, -1.5,  0.0,  3.0 },
            {  3.0, -0.5, -1.5, -2.0, -2.0, -1.5, -0.5,  3.0 },
            {  3.0,  0.0, -1.0, -1.5, -1.5, -1.0,  0.0,  3.0 },
            {  4.0,  2.0,  0.0,  0.0,  0.0,  0.0,  2.0,  4.0 },
            {  5.0,  4.0,  3.0,  3.0,  3.0,  3.0,  4.0,  5.0 },
        };
        float whitePawnValues[8][8] =
        {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
            {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0 },
            {1.0, 1.0, 2.0, 3.0, 3.0, 2.0, 1.0, 1.0 },
            {0.5, 0.5, 1.0, 2.5, 2.5, 1.0, 0.5, 0.5 },
            {0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 0.0 },
            {0.5, -0.5, -1.0, 0.0, 0.0, -1.0, -0.5, 0.5 },
            {0.5, 1.0, 1.0, -2.0, -2.0, 1.0, 1.0, 0.5 },
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
        };
        float blackPawnValues[8][8] =
        {
            { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 },
            { -0.5, -1.0, -1.0,  2.0,  2.0, -1.0, -1.0, -0.5 },
            { -0.5,  0.5,  1.0,  0.0,  0.0,  1.0,  0.5, -0.5 },
            { 0.0,  0.0,  0.0, -2.0, -2.0,  0.0,  0.0,  0.0 },
            { -0.5, -0.5, -1.0, -2.5, -2.5, -1.0, -0.5, -0.5 },
            { -1.0, -1.0, -2.0, -3.0, -3.0, -2.0, -1.0, -1.0 },
            { -5.0, -5.0, -5.0, -5.0, -5.0, -5.0, -5.0, -5.0 },
            { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 },
        };

        for (int h = 0; h < 8; h++)
        {
            for (int w = 0; w < 8; w++)
            {
                switch (chessBoard[h][w][whichBoard])
                {
                case 10:
                    normalEvaluation += 1;
                    moreAccurateEvaluation += 1;
                    moreAccurateEvaluation += whitePawnValues[h][w];
                    break;
                case 11:
                    normalEvaluation += -1;
                    moreAccurateEvaluation += -1;
                    moreAccurateEvaluation += blackPawnValues[h][w];
                    break;
                case 30:
                    normalEvaluation += 3;
                    moreAccurateEvaluation += 3;
                    moreAccurateEvaluation += whiteKnightValues[h][w];
                    break;
                case 31:
                    normalEvaluation += -3;
                    moreAccurateEvaluation += -3;
                    moreAccurateEvaluation += blackKnightValues[h][w];
                    break;
                case 32:
                    normalEvaluation += 3;
                    moreAccurateEvaluation += 3;
                    moreAccurateEvaluation += whiteBishopValues[h][w];
                    break;
                case 33:
                    normalEvaluation += -3;
                    moreAccurateEvaluation += -3;
                    moreAccurateEvaluation += blackBishopValues[h][w];
                    break;
                case 50:
                    normalEvaluation += 5;
                    moreAccurateEvaluation += 5;
                    moreAccurateEvaluation += whiteRookValues[h][w];
                    break;
                case 51:
                    normalEvaluation += -5;
                    moreAccurateEvaluation += -5;
                    moreAccurateEvaluation += blackRookValues[h][w];
                    break;
                case 90:
                    normalEvaluation += 9;
                    moreAccurateEvaluation += 9;
                    moreAccurateEvaluation += whiteQueenValues[h][w];
                    break;
                case 91:
                    normalEvaluation += -9;
                    moreAccurateEvaluation += -9;
                    moreAccurateEvaluation += blackQueenValues[h][w];
                    break;
                case 100:
                    normalEvaluation += 10;
                    moreAccurateEvaluation += 10;
                    moreAccurateEvaluation += whiteKingValues[h][w];
                    break;
                case 101:
                    normalEvaluation += -10;
                    moreAccurateEvaluation += -10;
                    moreAccurateEvaluation += blackKingValues[h][w];
                    break;
                default:
                    break;
                }
            }
        }
    }
}

static float moveSearch(int depth)
{
    if (gameEnded == true || depth == 0)
    {
        return moreAccurateEvaluation;
    }
    if (isWhitesTurn == true)
    {
        best = -10000;
    }
    else
    {
        best = 10000;
    }
}

static void resetButton()
{
    isGameStarted = false;
    boardReset();
    boardBeginWhite();
    isBoardFlipped = false;
    isHumanPlayingAgainstAI = false;
    isAITurnedOn = false;
    resetIsChessSquareViableMove();
    isWhitesTurn = true;
    updateIsKingInCheck(0);
    gameEnded = false;
    didWhiteWin = false;
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
    waitForFrame = 0;

    for (int i = 0; i < 12; i++)
    {
        lastMoves[i][0] = i;
        lastMoves[i][1] = i;
    }
    evaluation(0);
}

static void pieceMove(int Y, int X, int whichBoard)
{
    moveNumber++;
    int whatPieceToMove = chessBoard[selectedPieceY][selectedPieceX][whichBoard];
    chessBoard[selectedPieceY][selectedPieceX][whichBoard] = 0;
    chessBoard[Y][X][whichBoard] = whatPieceToMove;

    //Last moves update
    for (int i = 11; i > 0; i--)
    {
        lastMoves[i][0] = lastMoves[i - 1][0];
        lastMoves[i][1] = lastMoves[i - 1][1];
    }
    lastMoves[0][0] = (selectedPieceY * 8) + selectedPieceX;
    lastMoves[0][1] = (Y * 8) + X;

    if (isGameStarted == false) { isGameStarted = true; }

    //Rook move while castling
    if (whatPieceToMove == 100)
    {
        if (isWhiteKingCastlingPossible == true && Y == 7 && X == 6)
        {
            chessBoard[7][5][whichBoard] = 50;
            chessBoard[7][7][whichBoard] = 0;
        }
        if (isWhiteQueenCastlingPossible == true && Y == 7 && X == 2)
        {
            chessBoard[7][3][whichBoard] = 50;
            chessBoard[7][0][whichBoard] = 0;
        }
    }
    if (whatPieceToMove == 101)
    {
        if (isBlackKingCastlingPossible == true && Y == 0 && X == 6)
        {
            chessBoard[0][5][whichBoard] = 51;
            chessBoard[0][7][whichBoard] = 0;
        }
        if (isBlackQueenCastlingPossible == true && Y == 0 && X == 2)
        {
            chessBoard[0][3][whichBoard] = 51;
            chessBoard[0][0][whichBoard] = 0;
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
        if (selectedPieceX == 7) { isWhiteKingCastlingPossible = false; }
        if (selectedPieceX == 0) { isWhiteQueenCastlingPossible = false; }
        break;
    case 51:
        if (selectedPieceX == 7) { isBlackKingCastlingPossible = false; }
        if (selectedPieceX == 0) { isBlackQueenCastlingPossible = false; }
        break;
    }

    //En Passant pawn capture
    if (whatPieceToMove == 10)
    {
        if (isEnPassantForWhitePossible == true && Y == 2)
        {
            chessBoard[Y + 1][X][whichBoard] = 0;
        }
    }
    if (whatPieceToMove == 11)
    {
        if (isEnPassantForBlackPossible == true && Y == 5)
        {
            chessBoard[Y - 1][X][whichBoard] = 0;
        }
    }

    //isEnPassantPossible
    if (whatPieceToMove == 10)
    {
        if (selectedPieceY == 6 && Y == 4)
        {
            isEnPassantForBlackPossible = true;
            enPassantX = X;
        }
    }
    else
    {
        isEnPassantForBlackPossible = false;
    }
    if (whatPieceToMove == 11)
    {
        if (selectedPieceY == 1 && Y == 3)
        {
            isEnPassantForWhitePossible = true;
            enPassantX = X;
        }
    }
    else
    {
        isEnPassantForWhitePossible = false;
    }

    if (isWhitesTurn == true) { isWhitesTurn = false; }
    else { isWhitesTurn = true; }

    resetIsChessSquareViableMove();
    updatePawnToQueenPromotion(whichBoard);
    updateIsKingInCheck(whichBoard);
    isGameEnded(whichBoard);
    evaluation(whichBoard);
}

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
  
    resetButton();

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

        int starterBoardPosX = 80;
        int starterBoardPosY = 100;
        //Graphics
        if (isBoardFlipped == true)
        {
            //Drawing Black and White Squares
            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    ImGui::SetCursorPos(ImVec2(((7-w) * 60) + starterBoardPosX, ((7-h) * 60) + starterBoardPosY));
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
                    ImGui::SetCursorPos(ImVec2(((7-w) * 60) + starterBoardPosX, ((7-h) * 60) + starterBoardPosY));
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
                        if (chessBoard[h][w][0] == 100)
                        {
                            ImGui::SetCursorPos(ImVec2(((7-w) * 60) + starterBoardPosX, ((7-h) * 60) + starterBoardPosY));
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
                        if (chessBoard[h][w][0] == 101)
                        {
                            ImGui::SetCursorPos(ImVec2(((7-w) * 60) + starterBoardPosX, ((7-h) * 60) + starterBoardPosY));
                            ImGui::Image((void*)squarer, ImVec2(my_image_width, my_image_height));
                        }
                    }
                }
            }

            //Drawing Selected Square
            if (isChessPieceSelected == true)
            {
                ImGui::SetCursorPos(ImVec2(((7-selectedPieceX) * 60) + starterBoardPosX, ((7-selectedPieceY) * 60) + starterBoardPosY));
                ImGui::Image((void*)squarec, ImVec2(my_image_width, my_image_height));
            }

            //Drawing Chess Pieces
            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    ImGui::SetCursorPos(ImVec2(((7-w) * 60) + starterBoardPosX, ((7-h) * 60) + starterBoardPosY));
                    switch (chessBoard[h][w][0])
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
        }
        else
        {

            //Drawing Black and White Squares
            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
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

            for (int w = 0; w < 8; w++)
            {
                for (int h = 0; h < 8; h++)
                {
                    ImGui::SetCursorPos(ImVec2((w * 60) + starterBoardPosX, (h * 60) + starterBoardPosY));
                    if (dangerSquaresForWhiteKing[h][w] == true)
                    {
                        ImGui::Image((void*)squarer, ImVec2(my_image_width, my_image_height));
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
                        if (chessBoard[h][w][0] == 100)
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
                        if (chessBoard[h][w][0] == 101)
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
                    switch (chessBoard[h][w][0])
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
        }

        //Buttons and Text

        if (isGameStarted == false)
        {
            ImGui::SetCursorPos(ImVec2(my_image_width * 8 + starterBoardPosX + 30, my_image_height * 4 + starterBoardPosY - 30));
            if (ImGui::Button("Flip Board", ImVec2(100, 60)))
            {
                resetIsChessSquareViableMove();
                isChessPieceSelected = false;
                if (isBoardFlipped == true)
                {
                    isBoardFlipped = false;
                }
                else if (isBoardFlipped == false)
                {
                    isBoardFlipped = true;
                }
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

        ImGui::SetCursorPos(ImVec2(starterBoardPosX + 480 + 10, starterBoardPosY + 90));
        ImGui::Checkbox("Start AI from here", &isAITurnedOn);

        if (isAITurnedOn == true)
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 180));
            ImGui::Text("You are playing against AI.");
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 210));
            ImGui::Text("Good Luck.");
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(starterBoardPosX + 510, starterBoardPosY + 180));
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

        ImGui::SetCursorPos(ImVec2(starterBoardPosX -50, starterBoardPosY + 190));
        ImGui::Text("%.3g",normalEvaluation);
        ImGui::SetCursorPos(ImVec2(starterBoardPosX - 50, starterBoardPosY + 220));
        ImGui::Text("%.3g", moreAccurateEvaluation);

        if (isBoardFlipped == false)
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
            if (didWhiteWin == true)
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
        if (gameEnded == false && isAITurnedOn == true)
        {
            if (isWhitesTurn == true && isBoardFlipped == true)
            {
                //thinkAndPlayMove();
            }
            if (isWhitesTurn == false && isBoardFlipped == false)
            {
                //thinkAndPlayMove();
            }
        }
        
        if (gameEnded == false)
        {
            //Moving Chess Pieces
            ImVec2 mousePosition = ImGui::GetMousePos();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mousePosition.x > 80 && mousePosition.x < 560 && mousePosition.y > 100 && mousePosition.y < 580)
            {
                int X = (mousePosition.x - 80) / 60;
                int Y = (mousePosition.y - 100) / 60;
                if (isBoardFlipped == true)
                {
                    X = 7 - X;
                    Y = 7 - Y;
                }


                if (isChessPieceSelected == true && isChessSquareViableMove[Y][X] == true)
                {
                    pieceMove(Y, X, 0);
                }

                //Piece Deselection
                if (isChessPieceSelected == true && isChessSquareViableMove[Y][X] == false)
                {
                    isChessPieceSelected = false;
                    resetIsChessSquareViableMove();
                }

                //Piece Selection
                if (isChessPieceSelected == false)
                {
                    //White
                    if (isWhitesTurn == true && chessBoard[Y][X][0] != 0 && (chessBoard[Y][X][0] % 10 == 0 || chessBoard[Y][X][0] % 10 == 2))
                    {
                        isChessPieceSelected = true;
                        selectedPieceX = X;
                        selectedPieceY = Y;
                        updateViableMoves(0);
                    }
                    //Black
                    if (isWhitesTurn == false && chessBoard[Y][X][0] != 0 && (chessBoard[Y][X][0] % 10 == 1 || chessBoard[Y][X][0] % 10 == 3))
                    {
                        isChessPieceSelected = true;
                        selectedPieceX = X;
                        selectedPieceY = Y;
                        updateViableMoves(0);
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

