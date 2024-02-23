#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <vector>
#include <stdio.h>
#include <ctime>
using namespace std;


//----------------------------------------------//
// constant variables (parameters)

const int startingSpeed = 6;
const int speedLimit = 20;
const int speedupWait = 200;   // frames between adding speed
const int scoreWait = 15;      // pixels traveled before adding 1 score
const int runWait = 2;         // frames between changing running sprites

const int jumpPower = 21;
const int gravityPower = 4;
const int colliderMargin = 4;  // bigger margin means smaller colliders

const int scoreTopMargin = 8;
const int scoreRightMargin = 10;
const int scoreDigitDistance = 2;

const int cactusChance = 3;   // chance to spawn a cactus when limit is not reached (smaller value = bigger chance, 1 means guaranteed)
const int cactusLimit = 3;
const int cactusInitialDistance = 80;
const int cloudChance = 3;    // chance to spawn a cloud when limit is not reached (smaller value = bigger chance, 1 means guaranteed)
const int cloudInitialDistance = 50;
const int cloudSpeed = 1;

const int paletteTotal = 6; // amount of available color palettes


//----------------------------------------------//
// functions

void Start(), GetInput(), Logic(), SpawnSprites(), DrawSprites(), GameOver(), GameOverWait(), Restart();
void SetColorPalette(int palette), SwitchColorPalette(bool forwards), Draw(int sprite, int startX, int startY), PrintScreen(), ResetScreen(), PrepareScreen(), PrepareSprites();
void SpawnCactus(int cactusType), DeleteCactus(int cactusIndex), SpawnCloud(int cloudType, int cloudY), DeleteCloud(int cloudIndex);


//----------------------------------------------//
// global variables

// ------ sprites
CHAR_INFO screenBuffer[44110]; // y[110]x[401 (2*200 + 1)] screen is 200x110 with last additional black column
vector<vector<vector<int>>> sprites;
vector<vector<int>> sprite_sizes;

// ------ game
bool isPlaying;
bool gameEnded;
bool hasChangedColor;

int dinoY;
int dinoYPower;
int dinoSprite;
int runCounter;

int speed;
int speedupCounter;

int score;
int highscore;
int scoreCounter;

int cactusDistance;
int cloudDistance;

int currentPalette;

// ------ console
CONSOLE_SCREEN_BUFFER_INFOEX screenInfo;
static CONSOLE_FONT_INFOEX  fontInfo;
CONSOLE_CURSOR_INFO cursorInfo;
HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
COORD screenStart;
COORD screenSize;
SMALL_RECT screenWriteRect;
WORD colorWords[4];
char wait;


//----------------------------------------------//
// structs

struct spriteSize {
    int X;
    int Y;
};

struct collider {
    int startX;
    int endX;
    int startY;
    int endY;
};


//----------------------------------------------//
// classes

class cactus {
public:

    // variables
    int type;
    int positionX;
    spriteSize size;

    // constructor
    cactus(int chosenType) {
        type = chosenType;
        positionX = 200;
        size.X = sprite_sizes[type + 6][0];
        size.Y = sprite_sizes[type + 6][1];
    }

    collider getCollider() {
        return collider{ 
            positionX,        // start X
            positionX+size.X, // end X
            93-size.Y,        // start Y
            93                // end Y
        };
    }

};

class cloud {
public:

    // variables
    int type;
    int positionX;
    int positionY;
    spriteSize size;

    //constructor
    cloud(int chosenType, int chosenY) {
        type = chosenType;
        positionX = 200;
        positionY = chosenY;
        size.X = sprite_sizes[type + 32][0];
        size.Y = sprite_sizes[type + 32][1];
    }


};

//----------------------------------------------//
// global variables that use classes

vector<cactus> cacti;
vector<cloud> clouds;

//----------------------------------------------//




// this will happen first
void Start() {

    screenInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    cursorInfo.dwSize = sizeof(CONSOLE_CURSOR_INFO);
    GetConsoleScreenBufferInfoEx(h, &screenInfo);
    GetCurrentConsoleFontEx(h, 0, &fontInfo);
    GetConsoleCursorInfo(h, &cursorInfo);
    

    // set start of screen
    screenStart.X = 0;
    screenStart.Y = 0;

    // set size of screen in pixels
    screenSize.X = 401;
    screenSize.Y = 110;

    // destination rectangle
    screenWriteRect.Top = 0;
    screenWriteRect.Left = 0;
    screenWriteRect.Bottom = 109;
    screenWriteRect.Right = 400;
    

    //set "pixel" size
    fontInfo.dwFontSize.Y = 7;
    SetCurrentConsoleFontEx(h, NULL, &fontInfo);

    // set console size
    const int width = 402, height = 120;
    COORD c;
    c.X = width;
    c.Y = height;
    screenInfo.dwSize = c;
    screenInfo.dwMaximumWindowSize = c;

    screenInfo.srWindow.Left = 0;
    screenInfo.srWindow.Right = width;
    screenInfo.srWindow.Top = 0;
    screenInfo.srWindow.Bottom = height;

    SetConsoleScreenBufferInfoEx(h, &screenInfo);

    // prepare sprites
    PrepareSprites();


    // set colors
    SetColorPalette(0);

    colorWords[0] = WORD(0x0000);
    colorWords[1] = WORD(0x0010);
    colorWords[2] = WORD(0x0020);
    colorWords[3] = WORD(0x0030);


    // hides the cursor entirely
    cursorInfo.bVisible = false; 
    SetConsoleCursorInfo(h, &cursorInfo);

    // prepares the screen buffer
    PrepareScreen();


    // set random seed
    srand(time(NULL));


    // set highscore
    highscore = 0;

    // reset arrow message
    hasChangedColor = false;

    // start the game
    gameEnded = false;

    Restart();

}



int main()
{
    Start();

    while (!gameEnded) {

        while (isPlaying) {

            GetInput();

            Logic();

            SpawnSprites();

            DrawSprites();
            PrintScreen();

            Sleep(30);

        }

        GameOver();

        Sleep(500);
        
        GameOverWait();

    }

    return 0;
}




void GetInput() {

    // if keyboard is hit
    if (_kbhit()) {

        switch (_getch()) {

        case ' ':
            // jump
            if (dinoY == 0) {

                dinoYPower += jumpPower;

            }
            break;

        // left arrow key
        case 75:
            SwitchColorPalette(false);
            break;

        // right arrow key
        case 77:
            SwitchColorPalette(true);
            break;

        }

    }

}


void Logic() {

    // jump
    dinoY += dinoYPower;

    if (dinoY == 0) {

        // running sprite swap
        runCounter--;
        if (runCounter <= 0) {

            runCounter = runWait;

            if (dinoSprite == 3) { dinoSprite = 2; }
            else { dinoSprite = 3; }

        }
        

    }
    else {

        // gravity
        dinoYPower -= gravityPower;

    }

    if (dinoY < 0) {

        dinoY = 0; 
        dinoYPower = 0;

    }

    // cactus logic (loop through every cactus)
    for (int i = 0; i < cacti.size(); i++) {

        cactus* currentCactus = &(cacti[i]);

        // check if cactus is out of the screen
        if ((*currentCactus).positionX < 0-(*currentCactus).size.X) {

            DeleteCactus(i);

        }
        // if not
        else {

            // move the cactus
            (*currentCactus).positionX -= speed;

            // check for collision
            collider currentCollider = (*currentCactus).getCollider();

            if ((currentCollider.startX <= (42 - colliderMargin) && currentCollider.endX >= (20 + colliderMargin)) && (currentCollider.startY <= (92 - dinoY - colliderMargin) && currentCollider.endY >= (69 + colliderMargin))) {

                //GameOver();
                isPlaying = false;
            }
        }

    }
        


    // cloud logic (loop through every cloud)
    for (int i = 0; i < clouds.size(); i++) {

        cloud* currentCloud = &(clouds[i]);

        // check if cloud is out of the screen
        if ((*currentCloud).positionX < 0 - (*currentCloud).size.X) {

            DeleteCloud(i);

        }
        // if not
        else {

            // move the cloud
            (*currentCloud).positionX -= cloudSpeed;

        }

    }

    

    // add score
    scoreCounter -= speed;
    if (scoreCounter <= 0) {
        score++;
        scoreCounter = scoreWait;
    }

    //increase speed
    if (speed < speedLimit) {

        speedupCounter--;
        if (speedupCounter <= 0) {
            speed++;
            speedupCounter = speedupWait;
        }

    }
    

}


void DrawSprites() {


    ResetScreen();

    Draw(5, 0, 90); // floor

    // loop through every cloud and draw it
    for (int i = 0; i < clouds.size(); i++) {

        Draw((clouds[i].type + 32), clouds[i].positionX, clouds[i].positionY);

    }

    // loop through every cactus and draw it
    for (int i = 0; i < cacti.size(); i++) {

        Draw((cacti[i].type + 6), cacti[i].positionX, 93-cacti[i].size.Y);

    }

    Draw(dinoSprite, 20, 69 - dinoY); // dino

    // score
    Draw(30, 166 - (6 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin); // SCORE text

    Draw(((score % 100000) / 10000) + 10, 185 - (4 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin); // first digit
    Draw(((score % 10000) / 1000) + 10, 188 - (3 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);
    Draw(((score % 1000) / 100) + 10, 191 - (2 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);
    Draw(((score % 100) / 10) + 10, 194 - scoreDigitDistance - scoreRightMargin, scoreTopMargin);
    Draw((score % 10) + 10, 197 - scoreRightMargin, scoreTopMargin);                                         // last digit

    // highscore
    Draw(31, 143 - (14 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin); // SCORE text

    Draw(((highscore % 100000) / 10000) + 20, 151 - (12 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin); // first digit
    Draw(((highscore % 10000) / 1000) + 20, 154 - (11 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);
    Draw(((highscore % 1000) / 100) + 20, 157 - (10 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);
    Draw(((highscore % 100) / 10) + 20, 160 - (9 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);
    Draw((highscore % 10) + 20, 163 - (8 * scoreDigitDistance) - scoreRightMargin, scoreTopMargin);                // last digit

}


void GameOver() {

    dinoSprite = 4;
    DrawSprites();
    Draw(9, 49, 36);
    PrintScreen();
    
}


void GameOverWait() {

    // change color message
    if (!hasChangedColor) {
        Draw(35, 75, 86);
        PrintScreen();
    }

    // ESC TO EXIT message
    Draw(36, scoreRightMargin - 1, scoreTopMargin - 1);
    PrintScreen();

    wait = _getch();

    if (wait == ' ') { Restart(); }
    else { 

        // left arrow
        if (wait == 75) { SwitchColorPalette(false); PrintScreen(); }
        // right arrow
        else if (wait == 77) { SwitchColorPalette(true); PrintScreen(); }
        // ESC
        else if (wait == 27) { gameEnded = true; }

        else { GameOverWait(); }

    }

}


void Restart() {

    // delete all cacti and clouds
    cacti.clear();
    clouds.clear();

    // position the dino
    dinoY = 0;
    dinoYPower = 0;
    dinoSprite = 1;
    runCounter = runWait;

    // set beginning speed
    speed = startingSpeed;
    speedupCounter = speedupWait;

    // reset cactus and cloud distance
    cactusDistance = 0;
    cloudDistance = 0;

    // reset score
    if (score > highscore) { highscore = score; }
    score = 0;
    scoreCounter - scoreWait;

    // enable playing
    isPlaying = true;

}


void SpawnSprites() {

    // attempt to spawn a random cactus if available
    cactusDistance -= speed;
    if (cactusDistance <= 0) {

        cactusDistance = cactusInitialDistance + (3 * speed);   // gaps will be wider as speed increases

        if (cacti.size() < cactusLimit) {

            if ((rand() % cactusChance) == 0 || cacti.size() == 0) {

                SpawnCactus(rand() % 3);
            }

        }

    }

    // attempt to spawn a random cloud if available
    cloudDistance -= cloudSpeed;
    if (cloudDistance <= 0) {

        cloudDistance = cloudInitialDistance;

        if ((rand() % cloudChance) == 0 || clouds.size() == 0) {

            SpawnCloud(rand() % 3, 18 + (rand() % 45));
        }

    }

}


// ------------------------ SPAWNING FUNCTIONS -------------------------------

void SpawnCactus(int cactusType) {

    cacti.push_back(cactus(cactusType));

}

void DeleteCactus(int cactusIndex) {

    cacti.erase(cacti.begin() + cactusIndex);

}

void SpawnCloud(int cloudType, int cloudY) {

    clouds.push_back(cloud(cloudType, cloudY));

}

void DeleteCloud(int cloudIndex) {

    clouds.erase(clouds.begin() + cloudIndex);

}


// --------------------- SCREEN DRAWING FUNCTIONS ----------------------------

void Draw(int sprite, int startX, int startY) {

    vector<vector<int>> currentSprite = sprites[sprite];
    vector<int> currentSpriteSize = sprite_sizes[sprite];

    for (int row = 0; row < currentSpriteSize[1]; row++) {
        for (int col = 0; col < currentSpriteSize[0]; col++) {

            // if current pixel isn't transparent and is not outside of screen bounds, then add it to screen buffer.
            if (currentSprite[row][col] != 0 && (row + startY) >= 0 && (col + startX) >= 0 && (row + startY) < 110 && (col + startX) < 200) {

                screenBuffer[ 2 * ((row+startY) * 200 + (col+startX)) + (row + startY)].Attributes = colorWords[ currentSprite[row][col] ];
                screenBuffer[ 2 * ((row+startY) * 200 + (col+startX)) + 1 + (row + startY)].Attributes = colorWords[currentSprite[row][col]];

            }

        }
    }

}


void PrintScreen() {

    WriteConsoleOutput(
        h, // screen buffer to write to
        screenBuffer,        // buffer to copy from
        screenSize,     // col-row size of screenBuffer
        screenStart,    // top left src cell in screenBuffer
        &screenWriteRect);  // dest. screen buffer rectangle

}


void ResetScreen() {

    for (int row = 0; row < 110; row++) {
        for (int col = 0; col < 401; col++) {

            // if last pixel in a row, set to black. Otherwise background color
            if (col == 400) {
                screenBuffer[row * 401 + col].Attributes = colorWords[0];
            }
            else {
                screenBuffer[row * 401 + col].Attributes = colorWords[1];
            } 

        }
    }

}


void PrepareScreen() {

    for (int row = 0; row < 110; row++) {
        for (int col = 0; col < 401; col++) {

            // set all screen characters to be blank
            screenBuffer[row * 401 + col].Char.UnicodeChar = CHAR(' ');

        }
    }

}


void SwitchColorPalette(bool forwards) {

    if (forwards) {

        if (currentPalette < paletteTotal - 1) {
            SetColorPalette(currentPalette + 1);
        }
        else {
            SetColorPalette(0);
        }

    }
    else {

        if (currentPalette > 0) {
            SetColorPalette(currentPalette - 1);
        }
        else {
            SetColorPalette(paletteTotal - 1);
        }

    }

}


void SetColorPalette(int palette) {

    switch (palette) {

        // default
    case 0:
        screenInfo.ColorTable[0] = RGB(0, 0, 0);
        screenInfo.ColorTable[1] = RGB(242, 242, 242);
        screenInfo.ColorTable[2] = RGB(192, 192, 192);
        screenInfo.ColorTable[3] = RGB(90, 90, 90);
        break;

        // warm
    case 1:
        screenInfo.ColorTable[0] = RGB(48, 30, 10);
        screenInfo.ColorTable[1] = RGB(255, 227, 186);
        screenInfo.ColorTable[2] = RGB(227, 172, 100);
        screenInfo.ColorTable[3] = RGB(191, 94, 61);
        break;

        // darkmode
    case 2:
        screenInfo.ColorTable[0] = RGB(0, 0, 0);
        screenInfo.ColorTable[1] = RGB(30, 30, 30);
        screenInfo.ColorTable[2] = RGB(70, 70, 70);
        screenInfo.ColorTable[3] = RGB(190, 190, 190);
        break;

        // cool
    case 3:
        screenInfo.ColorTable[0] = RGB(0, 0, 0);
        screenInfo.ColorTable[1] = RGB(37, 33, 74);
        screenInfo.ColorTable[2] = RGB(48, 73, 171);
        screenInfo.ColorTable[3] = RGB(76, 135, 230);
        break;

        // gameboy
    case 4:
        screenInfo.ColorTable[0] = RGB(15, 56, 15);
        screenInfo.ColorTable[1] = RGB(202, 220, 159);
        screenInfo.ColorTable[2] = RGB(139, 172, 15);
        screenInfo.ColorTable[3] = RGB(48, 98, 48);
        break;

        // pink
    case 5:
        screenInfo.ColorTable[0] = RGB(48, 0, 46);
        screenInfo.ColorTable[1] = RGB(255, 170, 255);
        screenInfo.ColorTable[2] = RGB(230, 100, 230);
        screenInfo.ColorTable[3] = RGB(192, 0, 181);
        break;

    }

    SetConsoleScreenBufferInfoEx(h, &screenInfo);
    currentPalette = palette;
    hasChangedColor = true;

}


void PrepareSprites() {

    vector<vector<int>> sprite;
    vector<int> sprite_size;

    // [0] test
    sprite_size = { 16, 16 };
    sprite = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [1] dino (neutral)
    sprite_size = { 22, 23 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0},
    {1, 3, 1, 0, 0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0},
    {1, 3, 1, 1, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [2] dino (run 1)
    sprite_size = { 22, 23 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0},
    {1, 3, 1, 0, 0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0},
    {1, 3, 1, 1, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [3] dino (run 2)
    sprite_size = { 22, 23 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0},
    {1, 3, 1, 0, 0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0},
    {1, 3, 1, 1, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 3, 3, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0}

    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [4] dino (shock)
    sprite_size = { 22, 23 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 3, 1, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1},
    {1, 3, 1, 0, 0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 0},
    {1, 3, 1, 1, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [5] floor
    sprite_size = { 200, 1 };
    sprite = {
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [6] cactus (small)
    sprite_size = { 13, 23 };
    sprite = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 3, 3, 3, 1, 1, 1, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0},
    {0, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [7] cactus (medium)
    sprite_size = { 20, 17 };
    sprite = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 1, 0, 0, 1, 1, 1, 3, 3, 1, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 3, 1, 1, 3, 3, 1, 3, 3, 1, 1, 0, 0},
    {0, 0, 1, 1, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 1, 0},
    {0, 1, 3, 1, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1},
    {1, 3, 3, 1, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1},
    {1, 3, 3, 1, 3, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 1, 3, 3, 1},
    {1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 1, 3, 3, 1},
    {1, 3, 3, 1, 3, 3, 3, 3, 3, 1, 0, 0, 1, 1, 3, 3, 3, 3, 3, 1},
    {0, 1, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0},
    {0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0},
    {0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [8] cactus (big)
    sprite_size = { 38, 25 };
    sprite = {
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 3, 1, 1, 0, 0, 1, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 1, 1, 1, 1, 1},
    {0, 1, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 1, 1, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 1, 1, 1, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 1, 1, 1, 3, 3, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 3, 1, 3, 1, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1},
    {0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 1, 3, 1, 3, 3, 1, 3, 1, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0},
    {0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 1, 3, 3, 3, 3, 3, 1, 3, 1, 3, 3, 1, 3, 1, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0},
    {0, 0, 0, 1, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 3, 1, 3, 3, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 3, 1, 1, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 1, 3, 3, 3, 1, 0, 0, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [9] game over
    sprite_size = { 102, 37 };
    sprite = {
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1},
    {1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1},
    {1, 3, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 1, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1},
    {1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 3, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 1, 1},
    {1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 1},
    {1, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 1, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 0, 1, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 1, 1, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 1, 3, 3, 1, 3, 3, 3, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 1, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 1, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 1, 1, 1, 1, 1, 3, 3, 1, 1, 1, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [10] 0 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {3, 0, 3},
    {3, 0, 3},
    {3, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [11] 1 digit
    sprite_size = { 3, 5 };
    sprite = {
    {0, 3, 0},
    {3, 3, 0},
    {0, 3, 0},
    {0, 3, 0},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [12] 2 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {0, 0, 3},
    {3, 3, 3},
    {3, 0, 0},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [13] 3 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {0, 0, 3},
    {0, 3, 3},
    {0, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [14] 4 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 0, 0},
    {3, 0, 3},
    {3, 3, 3},
    {0, 0, 3},
    {0, 0, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [15] 5 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {3, 0, 0},
    {3, 3, 3},
    {0, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [16] 6 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {3, 0, 0},
    {3, 3, 3},
    {3, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [17] 7 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {0, 0, 3},
    {0, 3, 0},
    {3, 0, 0},
    {3, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [18] 8 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 0},
    {3, 3, 0},
    {3, 3, 3},
    {3, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [19] 9 digit
    sprite_size = { 3, 5 };
    sprite = {
    {3, 3, 3},
    {3, 0, 3},
    {3, 3, 3},
    {0, 0, 3},
    {3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [20] 0 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {2, 0, 2},
    {2, 0, 2},
    {2, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [21] 1 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {0, 2, 0},
    {2, 2, 0},
    {0, 2, 0},
    {0, 2, 0},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [22] 2 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {0, 0, 2},
    {2, 2, 2},
    {2, 0, 0},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [23] 3 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {0, 0, 2},
    {0, 2, 2},
    {0, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [24] 4 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 0, 0},
    {2, 0, 2},
    {2, 2, 2},
    {0, 0, 2},
    {0, 0, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [25] 5 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {2, 0, 0},
    {2, 2, 2},
    {0, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [26] 6 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {2, 0, 0},
    {2, 2, 2},
    {2, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [27] 7 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {0, 0, 2},
    {0, 2, 0},
    {2, 0, 0},
    {2, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [28] 8 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 0},
    {2, 2, 0},
    {2, 2, 2},
    {2, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [29] 9 lighter digit
    sprite_size = { 3, 5 };
    sprite = {
    {2, 2, 2},
    {2, 0, 2},
    {2, 2, 2},
    {0, 0, 2},
    {2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [30] SCORE text
    sprite_size = { 19, 5 };
    sprite = {
    {3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3},
    {3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 0, 0},
    {3, 3, 3, 0, 3, 0, 0, 0, 3, 0, 3, 0, 3, 3, 0, 0, 3, 3, 0},
    {0, 0, 3, 0, 3, 0, 0, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 0, 0},
    {3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3, 0, 3, 0, 3, 0, 3, 3, 3}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [31] HI light text
    sprite_size = { 8, 5 };
    sprite = {
    {2, 0, 0, 2, 0, 2, 2, 2},
    {2, 0, 0, 2, 0, 0, 2, 0},
    {2, 2, 2, 2, 0, 0, 2, 0},
    {2, 0, 0, 2, 0, 0, 2, 0},
    {2, 0, 0, 2, 0, 2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [32] cloud (small)
    sprite_size = { 16, 6 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 2, 0, 0, 0},
    {0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0},
    {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0},
    {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [33] cloud (medium)
    sprite_size = { 23, 7 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0},
    {0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0},
    {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [34] cloud (big)
    sprite_size = { 39, 10 };
    sprite = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0},
    {0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0},
    {0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0},
    {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [35] CHANGE COLOR text
    sprite_size = { 51, 16 };
    sprite = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 3, 3, 3, 1, 3, 1, 1, 3, 1, 1, 3, 3, 1, 1, 3, 1, 1, 3, 1, 3, 3, 3, 3, 1, 3, 3, 3, 1, 1, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 1, 1, 1, 3, 3, 3, 1, 3, 3, 3, 1},
    {1, 3, 1, 1, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 3, 1, 3, 1, 3, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 3, 1},
    {1, 3, 1, 1, 1, 3, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 1, 3, 3, 1, 3, 1, 3, 3, 1, 3, 3, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 3, 1, 1},
    {1, 3, 1, 1, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 3, 1},
    {1, 3, 3, 3, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 1, 1, 3, 1, 3, 3, 3, 3, 1, 3, 3, 3, 1, 1, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 3, 3, 1, 3, 1, 3, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 3, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);


    // [36] ESC TO EXIT text
    sprite_size = { 39, 7 };
    sprite = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 2, 1},
    {1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2, 1, 1, 2, 1, 1},
    {1, 2, 2, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 2, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1},
    {1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2, 1, 1, 2, 1, 1},
    {1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 1, 2, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    sprite_sizes.push_back(sprite_size);
    sprites.push_back(sprite);

}

// ---------------------------------------------------------------------------


