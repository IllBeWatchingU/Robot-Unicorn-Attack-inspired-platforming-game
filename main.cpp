#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH			640
#define SCREEN_HEIGHT			480
#define MAX_VELOCITY			-2
#define JUMP_VELOCITY			-700
#define GRAVITY					10
#define BONUS_JUMP_VELOCITY		-600
#define DASH_VELOCITY			MAX_VELOCITY * 2
#define DASH_RANGE				100

struct object {
	int x;
	int y;
	int length;
	int height;
};

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
                SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};


// rysowanie prostok¹ta o d³ugoœci boków l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

// czyta liczbê z pierwszej linijki pliku file
int znajdzLiczbe(FILE* file) {
	char linijka[50] = {};

	if (file == NULL) {
		printf("Nie mozna otworzyc pliku %s", "plik");
	}
	fgets(linijka, 50, file);

	char* tmp;
	if ((tmp = strchr(linijka, '\n')) != NULL)
		*tmp = '\0';

	for (int i = 0, k = strlen(linijka) - 1; i < k; i++, k--) {
		char temp = linijka[i];
		linijka[i] = linijka[k];
		linijka[k] = temp;
	}

	if (linijka[1] >= '0' && linijka[1] <= '9') {
		if (linijka[2] >= '0' && linijka[2] <= '9') {
			if (linijka[3] >= '0' && linijka[3] <= '9') {
				return (1000 * (linijka[3] - '0')) + (100 * (linijka[2] - '0')) + (10*(linijka[1] - '0')) + (linijka[0] - '0');
			}
			else {
				return (100 * (linijka[2] - '0')) + (10 * (linijka[1] - '0')) + (linijka[0] - '0');
			}
		}
		else {
			return (10 * (linijka[1] - '0')) + (linijka[0] - '0');
		}
	}
	else {
		return (int)linijka[0] - '0';
	}
}

//wpisuje do platformy k parametry z pliku 
void comparePlatformStats(char* wsk, int &k, SDL_Rect platform_start[]) {
	if (strcmp(wsk, "platform") == 0) {
		wsk = strtok(NULL, " ");
		k = atoi(wsk) - 1;
	}
	else if (strcmp(wsk, "x") == 0) {
		wsk = strtok(NULL, " ");
		platform_start[k].x = atoi(wsk);
	}
	else if (strcmp(wsk, "y") == 0) {
		wsk = strtok(NULL, " ");
		platform_start[k].y = atoi(wsk);
	}
	else if (strcmp(wsk, "length") == 0) {
		wsk = strtok(NULL, " ");
		platform_start[k].w = atoi(wsk);
	}
	else if (strcmp(wsk, "height") == 0) {
		wsk = strtok(NULL, " ");
		platform_start[k].h = atoi(wsk);
	}
}

//czyta plik, dzieli linijki na fragmenty i zapisuje je do struktur za pomoc¹ funkcji comparePlatformStats
void readFileToPlatform(FILE* file, int n, SDL_Rect platform_start[]){
	int k = 0;
	for (int i = 0; i < 5*n; i++) {
		char linijka[50] = {};
		fgets(linijka, 50, file);
		char* tmp;
		if ((tmp = strchr(linijka, '\n')) != NULL)
			*tmp = '\0';
		char* wsk;
		wsk = strtok(linijka, " ");
		comparePlatformStats(wsk, k, platform_start);
	}
}

//rysuje platformê na ekranie screen wed³ug danych podanych w strukturze platform 
//kolory tej platformy to ostatnie dwa argumenty
void drawPlatform(SDL_Surface* screen, SDL_Rect platform, Uint32 outlineColor, Uint32 fillColor) {
	int delX = platform.w;
	int delY = platform.h;
	int leftX = platform.x;
	int topY = platform.y;
	int rightX = platform.x + platform.w;
	int bottomY = platform.y + platform.h;

	if (platform.x > SCREEN_WIDTH || rightX < 0) { //sprawdza, czy jest poza ekranem w osi X
		DrawRectangle(screen, 0, 0, 0, 0, outlineColor, fillColor);
	}
	else {
		if (platform.y > SCREEN_HEIGHT || bottomY < 0) { //sprawdza, czy jest poza ekranem w osi Y
			DrawRectangle(screen, 0, 0, 0, 0, outlineColor, fillColor);
		}
		else {
			if (rightX > SCREEN_WIDTH) { // jeœli platforma wystaje w prawo w osi X
				delX = SCREEN_WIDTH - platform.x;
				
			}
			else if (platform.x < 0 && rightX <= SCREEN_WIDTH) { //jeœli platforma wystaje w lewo w osi X
				leftX = 0;
				delX = platform.w + platform.x;
			}
			else if (platform.x < 0 && rightX > SCREEN_WIDTH) { //jeœli wystaje z obu stron
				leftX = 0;
				delX = SCREEN_WIDTH;
			}

			if (bottomY > SCREEN_HEIGHT) { //jeœli platforma wystaje w dole w osi Y
				delY = SCREEN_HEIGHT - platform.y;
			}
			else if (platform.y < 0 && bottomY <= SCREEN_HEIGHT) { //jeœli platforma wystaje u góry w osi Y
				topY = 0;
				delY = platform.h + platform.y;
			}
			
			DrawRectangle(screen, leftX, topY, delX, delY, outlineColor, fillColor);
		}
	}
}

//resetuje platformê do stanu pocz¹tkowego
void resetPlatform(int n, SDL_Rect platform_start[], SDL_Rect platform[]) {
		platform[n].x = platform_start[n].x;
		platform[n].y = platform_start[n].y;
		platform[n].h = platform_start[n].h;
		platform[n].w = platform_start[n].w;
}

//przywraca grê do stanu pocz¹tkowego
void resetGame(double &worldTime, int n, int &velX, SDL_Rect platform_start[], SDL_Rect platform[], SDL_Rect &playerHitbox, object &player, const int levelWidth, bool &standing) {
	worldTime = 0;
	velX = 0;
	for (int i = 0; i < n; i++) {
		resetPlatform(i, platform_start, platform);
	}
	//player.y = platform[0].y - player.height;
	//playerHitbox.y = platform[0].y - playerHitbox.h;
	standing = true;
	player.y = 385;
	playerHitbox.y = 370;
	player.x = 40;
	playerHitbox.x = 5;
}

//sprawdza, czy gracz stoi na platformie
bool checkStanding(SDL_Rect playerHitbox, SDL_Rect* platform) {
		SDL_Rect bottom;
		bottom.x = playerHitbox.x;
		bottom.y = playerHitbox.y + playerHitbox.h;
		bottom.h = 1;
		bottom.w = playerHitbox.w - 1;
		if (SDL_HasIntersection(platform, &bottom) == SDL_TRUE) {
			return true;
		}
	return false;
}

//sprawdza, czy gracz styka siê z przeszkod¹
bool checkCollisionRight(SDL_Rect playerHitbox, SDL_Rect* platform) {
		SDL_Rect pixel;
		pixel.x = playerHitbox.x + playerHitbox.w;
		pixel.y = playerHitbox.y;
		pixel.h = playerHitbox.h - 1;
		pixel.w = playerHitbox.w / 2;
		if (SDL_HasIntersection(platform, &pixel) == SDL_TRUE) {
			return true;
		}
	return false;
}

//sprawdza, czy gracz styka siê z przeszkod¹ na suficie
bool checkCollisionTop(SDL_Rect playerHitbox, SDL_Rect* platform) {
		SDL_Rect pixel;
		pixel.x = playerHitbox.x;
		pixel.y = playerHitbox.y;
		pixel.h = 1;
		pixel.w = playerHitbox.w - 1;
		if (SDL_HasIntersection(platform, &pixel) == SDL_TRUE) {
			return true;
		}
	return false;
}

bool checkCollisionStandingAll(object &player, SDL_Rect playerHitbox, SDL_Rect platform[], const int n, bool& standing, bool& jump) {
	for (int i = 0; i < n; i++) {
		if (checkStanding(playerHitbox, &platform[i]) == true) {
			player.y = platform[i].y - player.height;
			return true;
		}
	}
	return false;
}

//sprawdza, czy nie ma kolizji miêdzy przeszkodami a graczem
//w przypadku kiedy gracz zderza siê z przeszkod¹ od góry, odbija siê od niej
//jeœli zderza siê z przeszkod¹ poziomo, to gra siê resetuje
void checkCollisionAll(const int n, SDL_Rect& playerHitbox, SDL_Rect* platform, bool& death, object& player, int tempPosY, int tempPosHY, double& jumpVel, int& game, int& menu)
{
	for (int i = 0; i < n; i++) {
		if (checkCollisionTop(playerHitbox, &platform[i]) == true && death == false) {
			player.y = tempPosY;
			playerHitbox.y = tempPosHY;
			jumpVel += GRAVITY;
		}
		if (checkCollisionRight(playerHitbox, &platform[i]) == true && death == false) {
			death = true;
			game = 1;
			menu = 0;
		}
	}
}

//jeden ruch w uproszczonym sterowaniu
void simpleMove(object& player, SDL_Rect& playerHitbox, const int n, SDL_Rect* platform, int velX, bool& standing, double& jumpVel, double jumpDistance, 
	bool& jump, bool &death, int &game, int &menu, const int levelHeight)
{
	//tymczasowa zmienna, w której jest przechowywana pozycja gracza przed wykonaniem ruchu
	int tempPosY = player.y;
	int tempPosHY = playerHitbox.y;

	for (int i = 0; i < n; i++) { //ruch gracza w kierunku poziomym
		platform[i].x += velX;
	}
	if (standing == true) { //sprawdzanie czy stoi, czy skacze/spada
		jumpVel = 0;
	}
	else if(standing == false && death == false) { //je¿eli nie stoi, to spada
		player.y += jumpDistance;
		playerHitbox.y += jumpDistance;
		jumpVel += GRAVITY;
	}

	//jeœli gracz w tym ruchu wypada poza planszê, to wykonuj¹ siê odpowiednie komendy (z góry siê "odbija", z do³u nastêpuje reset gry)
	if ((player.y - player.height) <= 0) {
		player.y = tempPosY;
		playerHitbox.y = tempPosHY;
		jumpVel += GRAVITY;
	}
	else if ((player.y + player.height) >= levelHeight) {
		if (death == true) {
			return;
		}
		death = true;
		standing = true;
		game = 1;
		menu = 0;
		return;
	}

	//sprawdza, czy gracz nie stoi na jakiejkolwiek platformie
	if (checkCollisionStandingAll(player, playerHitbox, platform, n, standing, jump) == true) {
		playerHitbox.y = player.y - player.height;
		standing = true;
		jump = false;
	}
	else {
		standing = false;
	}

	checkCollisionAll(n, playerHitbox, platform, death, player, tempPosY, tempPosHY, jumpVel, game, menu);
}

void altControlsMove(object& player, SDL_Rect& playerHitbox, int& velX, double worldTime, const int n, SDL_Rect* platform, bool& standing, 
	bool& dash, double& jumpVel, double jumpDistance, int& dashRange, double dashVel, const int levelHeight, bool& death, int& game, 
	int& menu, bool& jump, bool& doublejump)
{
	//tymczasowa zmienna, w której jest przechowywana pozycja gracza przed wykonaniem ruchu
	int tempPosY = player.y;
	int tempPosHY = playerHitbox.y;

	//zwiêkszanie prêdkoœci gracza po 10/20 sekundach
	if (velX <= MAX_VELOCITY) {
		velX = MAX_VELOCITY;
	}
	else {
		if (worldTime <= 10) {
			velX = -1;
		}
		if (worldTime >= 10 && worldTime <= 20) {
			velX = -2;
		}
		else if (worldTime >= 20) {
			velX = -3;
		}
	}

	//ruch gracza
	for (int i = 0; i < n; i++) {
		platform[i].x += velX;
	}

	//jeœli stoi na platformie albo jest w zrywie, to nie skacze
	if (standing == true || dash == true) {
		jumpVel = 0;
	}
	else { //inaczej spada z platformy albo skacze do góry, jeœli jest w skoku
		player.y += jumpDistance;
		playerHitbox.y += jumpDistance;
		jumpVel += GRAVITY;
	}

	//wykonanie zrywu
	if (dash == true) {
		if (dashRange > 0) {
			dashRange += dashVel;
			for (int i = 0; i < n; i++) {
				platform[i].x += dashVel;
			}
		}
		else {
			dash = false;
		}
	}

	//jeœli gracz w tym ruchu wypada poza planszê, to wykonuj¹ siê odpowiednie komendy (z góry siê "odbija", z do³u nastêpuje reset gry)
	if ((player.y - player.height) <= 0) {
		player.y = tempPosY;
		playerHitbox.y = tempPosHY;
		jumpVel += GRAVITY;
	}
	else if ((player.y + player.height) >= levelHeight) {
		death = true;
		game = 1;
		menu = 0;
	}

	if (checkCollisionStandingAll(player, playerHitbox, platform, n, standing, jump) == true) {
		playerHitbox.y = player.y - player.height;
		standing = true;
		jump = false;
		doublejump = false;
	}
	else {
		standing = false;
	}

	checkCollisionAll(n, playerHitbox, platform, death, player, tempPosY, tempPosHY, jumpVel, game, menu);
}

//obs³uga zdarzeñ w menu
void handleMenuEvents(SDL_Event& event, bool& death, int& lives, double& worldTime, const int n, 
	int& velX, SDL_Rect* platform_start, SDL_Rect* platform, SDL_Rect& playerHitbox, object& player, 
	const int levelWidth, bool& standing, int& menu, int& game, int& quit){
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_RETURN: {
				if (death == false || lives == 0) {
					lives = 3;
				}
				else if (death == true) {
					lives--;
				}
				resetGame(worldTime, n, velX, platform_start, platform, playerHitbox, player, levelWidth, standing);
				death = false;
				menu = 1;
				game = 0;
				break;
			}
			case SDLK_ESCAPE:
				menu = 1;
				quit = 1;
				break;
			}
		}
	}
}

//obs³uga zdarzeñ dla domyœlnego sterowania
void handleEventsDefaultControls(SDL_Event& event, int& velX, bool& jump, double& jumpVel, bool& standing, int& game, int& quit, double& worldTime, const int& n, SDL_Rect* platform_start, SDL_Rect* platform, SDL_Rect& playerHitbox, object& player, const int& levelWidth, bool& altControls)
{
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_RIGHT: {
			velX = MAX_VELOCITY;
			break;
		}
		case SDLK_UP: {
			if (jump == false) {
				jumpVel = JUMP_VELOCITY;
				jump = true;
				standing = false;
			}
			break;
		}
		case SDLK_ESCAPE: {
			game = 1;
			quit = 1;
			break;
		}
		case SDLK_n: {
			resetGame(worldTime, n, velX, platform_start, platform, playerHitbox, player, levelWidth, standing);
			break;
		}
		case SDLK_d: {
			altControls = true;
			break;
		}
		}
	}
	else if (event.type == SDL_KEYUP) {
		if (event.key.keysym.sym == SDLK_RIGHT) {
			velX = 0;
		}
	}
	else if (event.type == SDL_QUIT) {
		game = 1;
		quit = 1;
	}
}

//obs³uga zdarzeñ dla alternatywnego sterowania
void handleEventsAltControls(SDL_Event& event, bool& jump, int& t3, double& jumpVel, bool& standing, bool& doublejump, bool& dash, int& dashRange, int& game, 
	int& quit, double& worldTime, const int n, int& velX, SDL_Rect* platform_start, SDL_Rect* platform, SDL_Rect& playerHitbox, object& player, const int levelWidth, 
	bool& altControls, int& t4, double& jumpTime, double& bonusVel)
{
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_z: {
			if (jump == false) {
				t3 = SDL_GetTicks();
				jumpVel = JUMP_VELOCITY;
				jump = true;
				standing = false;
			}
			else if (jump == true && doublejump == false) {
				jumpVel = JUMP_VELOCITY;
				doublejump = true;
			}
			break;
		}
		case SDLK_x: {
			if (dash == false) {
				dashRange = DASH_RANGE;
				if (jump == true) {
					if (doublejump == true) {
						doublejump = false;
						dash = true;
					}
					else {
						dash = true;
					}
				}
				else {
					dash = true;
				}
			}
			break;
		}
		case SDLK_ESCAPE: {
			game = 1;
			quit = 1;
			break;
		}
		case SDLK_n: {
			resetGame(worldTime, n, velX, platform_start, platform, playerHitbox, player, levelWidth, standing);
			break;
		}
		case SDLK_d: {
			altControls = false;
			break;
		}
		}
	}
	else if (event.type == SDL_KEYUP) {
		if (event.key.keysym.sym == SDLK_z) {
			t4 = SDL_GetTicks();
			jumpTime = (t4 - t3) * 0.001;
			if (jumpTime > 0.3) {
				jumpTime = 0.3;
			}
			bonusVel = jumpTime * BONUS_JUMP_VELOCITY;
			jumpVel += bonusVel;
		}
	}
	else if (event.type == SDL_QUIT) {
		game = 1;
		quit = 1;
	}
}

// main
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	//zmienne
	int t1, t2, t3, t4, quit, rc, dx, dy, velX = 0;
	double delta, worldTime, jumpDistance = 0, jumpTime = 0, bonusVel = 0;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* yasuo, * bcg, * heart;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	object player;
	SDL_Rect playerHitbox;
	SDL_Rect* platform_start;
	SDL_Rect* platform;
	bool jump = false, altControls = false, doublejump = false, dash = false, standing = true;
	double jumpVel = JUMP_VELOCITY, dashVel = DASH_VELOCITY;
	int dashRange = DASH_RANGE;
	//koniec zmiennych

	player.x = 40;
	player.y = 285;
	player.height = 15;
	player.length = 15;
	playerHitbox.x = 5;
	playerHitbox.y = 270;
	playerHitbox.w = 30;
	playerHitbox.h = 30;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Robot Unicorn Attack");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// wy³¹czenie widocznoœci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);


	bcg = SDL_LoadBMP("./bcggreen.bmp");
	if (bcg == NULL) {
		printf("SDL_LoadBMP(bcggreen.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	yasuo = SDL_LoadBMP("./malyjasiek.bmp");
	if (yasuo == NULL) {
		printf("SDL_LoadBMP(malyjasiek.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	heart = SDL_LoadBMP("./heart.bmp");
	if (heart == NULL) {
		printf("SDL_LoadBMP(heart.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	FILE* file = fopen("gamedata.txt", "r");

	//czytanie liczb z pierwszych trzech linijek pliku
	const int levelWidth = znajdzLiczbe(file);
	const int levelHeight = znajdzLiczbe(file);
	const int n = znajdzLiczbe(file);

	//przydzielanie pamiêci do platform oraz przeszkód i wczytywanie ich parametrów z pliku
	platform_start = new SDL_Rect[n];
	platform = new SDL_Rect[n];

	readFileToPlatform(file, n, platform_start);

	fclose(file);

	for (int i = 0; i < n; i++) {
		resetPlatform(i, platform_start, platform);
	}

	player.y = platform[0].y - player.height;
	playerHitbox.y = platform[0].y - playerHitbox.h;

	dx = levelWidth / 4;
	dy = levelHeight / 2;

	//kolory rgb 
	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x76, 0xC9, 0x87);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int greenscreen = SDL_MapRGB(screen->format, 0xFF, 0x7F, 0x27);

	SDL_SetColorKey(yasuo, SDL_TRUE, greenscreen);
	SDL_SetColorKey(heart, SDL_TRUE, greenscreen);

	quit = 0;
	worldTime = 0;

	int game = 1, menu = 0;
	bool death = false;
	int lives = 0;

	t1 = SDL_GetTicks();

	while (!quit) {

		//
		while (!menu) {
			SDL_FillRect(screen, NULL, czarny);

			if (death == false || lives == 0) {
				sprintf(text, "Nacisnij ENTER, by rozpoczac rozgrywke");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_WIDTH / 2, text, charset);
				sprintf(text, "ESC - wyjscie");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_WIDTH / 2 + 20, text, charset);
			}
			else if (death == true) {
				sprintf(text, "Pozostala liczba zyc - %d", (lives - 1));
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_WIDTH / 2 - 20, text, charset);
				sprintf(text, "Czy kontynuowac rozgrywke?");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_WIDTH / 2, text, charset);
				sprintf(text, "ENTER - tak, ESC - wyjscie z programu");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_WIDTH / 2 + 20, text, charset);
			}

			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			handleMenuEvents(event, death, lives, worldTime, n, velX, platform_start, platform, playerHitbox, player, levelWidth, standing, menu, game, quit);
		}

		while (!game) {

			t2 = SDL_GetTicks();

			// w tym momencie t2-t1 to czas w milisekundach,
			// jaki uplyna³ od ostatniego narysowania ekranu
			// delta to ten sam czas w sekundach
			delta = (t2 - t1) * 0.001;
			t1 = t2;

			worldTime += delta;

			jumpDistance = delta * jumpVel;

			if (altControls == false) { //instrukcje dla uproszczonego sterowania

				simpleMove(player, playerHitbox, n, platform, velX, standing, jumpVel, jumpDistance, jump, death, game, menu, levelHeight);

				if (game == 1) {
					break;
				}
			}
			else { //instrukcje dla domyœlnego sterowania
				altControlsMove(player, playerHitbox, velX, worldTime, n, platform, standing, dash, jumpVel, jumpDistance, 
					dashRange, dashVel, levelHeight, death, game, menu, jump, doublejump);
				if (game == 1) {
					break;
				}
			}

			//zawijanie siê platform
			if (platform[0].x <= -levelWidth) {
				for (int i = 0; i < n; i++) {
					resetPlatform(i, platform_start, platform);
				}
			}

			//rysowanie t³a, platform i gracza
			DrawSurface(screen, bcg, dx, dy);

			for (int i = 0; i < n; i++) {
				drawPlatform(screen, platform[i], czerwony, czerwony);
			}

			DrawSurface(screen, yasuo, player.x, player.y);

			// tekst informacyjny / info text
			DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 55, czerwony, czarny);
			sprintf(text, "Czas rozgrywki = %.1lf s / s", worldTime);
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

			if (altControls == true) {
				sprintf(text, "Esc - wyjscie, Z - skok, X - zryw, N - nowa rozgrywka");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
				sprintf(text, "D - inny tryb sterowania");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);
			}
			else {
				sprintf(text, "Esc - wyjscie, \033 - przyspieszenie, \030 - skok, N - nowa rozgrywka");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
				sprintf(text, "D - inny tryb sterowania");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);
			}

			//rysowanie sprite'ów ¿ycia na ekranie
			if (lives == 3) {
				DrawSurface(screen, heart, 10, SCREEN_HEIGHT - 10);
				DrawSurface(screen, heart, 25, SCREEN_HEIGHT - 10);
				DrawSurface(screen, heart, 40, SCREEN_HEIGHT - 10);
			}
			else if (lives == 2) {
				DrawSurface(screen, heart, 10, SCREEN_HEIGHT - 10);
				DrawSurface(screen, heart, 25, SCREEN_HEIGHT - 10);
			}
			else if (lives == 1) {
				DrawSurface(screen, heart, 10, SCREEN_HEIGHT - 10);
			}

			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			// obs³uga zdarzeñ (o ile jakieœ zasz³y) / handling of events (if there were any)
			while (SDL_PollEvent(&event)) {
				if (altControls == false) {
					handleEventsDefaultControls(event, velX, jump, jumpVel, standing, game, quit, worldTime, n, 
						platform_start, platform, playerHitbox, player, levelWidth, altControls);
				}
				else {
					handleEventsAltControls(event, jump, t3, jumpVel, standing, doublejump, dash, dashRange, game, quit, worldTime, 
						n, velX, platform_start, platform, playerHitbox, player, levelWidth, altControls, t4, jumpTime, bonusVel);
				}
			}
		}
	};

		// zwolnienie powierzchni / freeing all surfaces
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);

		delete[] platform;
		delete[] platform_start;

		SDL_Quit();
		return 0;
	}