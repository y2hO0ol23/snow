#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <ctime>
#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <conio.h>

#include <algorithm>
#include <vector>
#include <stack>

typedef long long int ll;
typedef unsigned int uint;

#define BLACK 0
#define RED 4
#define LIGHTGRAY 7
#define GRAY 8
#define WHITE 15 
void textcolor(int foreground, int background) {
	int color = foreground + background * 16;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
void gotoxy(int x, int y) {
	COORD pos = { x,y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
void Remove_cursor(void) {
	CONSOLE_CURSOR_INFO curInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curInfo.bVisible = 0;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

void setup();
bool inp(char* t);
void clear();
void press(char chr);
void pushsnow(bool pushflag);
void screen(bool reload);

void footer();

void set_console(int fy, int fx);

#define DELAY 250
uint x, y;
uint px = 1 * (2), py = 0;
time_t SEED = time(NULL);
time_t last_falling_time;
time_t last_melt_time;
time_t cmp_time;
time_t lclock = clock();
int snow_freq = 1;

bool fallingflag = true;
bool pauseflag = false;
bool no_snow_flag = true;

#define BIT 60
struct snow {
	std::vector<ll> bit;
	void push(bool val) {
		bit[0] <<= 1;
		bit[0] &= (1LL << ((y - 1) % BIT + 1)) - 1;

		int len = bit.size();
		for (int i = 1; i < bit.size(); i++) {
			bit[i - 1] |= (bit[i] & (1LL << (BIT - 1))) >> (BIT - 1); // push bit to next bitfield
			bit[i] <<= 1;
			bit[i] &= (1LL << BIT) - 1;
		}
		bit[len - 1] |= (ll)val; // push input to top of bitfield
	}
};
struct Map {
	ll top = 0; // value of height on this line of snow
	int melt = 2; // value of snow melted
	struct Map* left;
	struct Map* right;
	time_t melt_time;
	std::pair<snow*, snow*> line;
	Map(std::pair<snow*, snow*> li) :line(li) {}
	void stackup() {
		if (top < y) melt--;
		if (melt < 0) {
			melt = 2;
			top++;
			flow();
		}
		melt_time = lclock;
	}
	void meltup() {
		if (!this->floor()) {
			melt++;
			if (melt == 3) {
				melt = 0;
				top--;
				left->flow();
				right->flow();
			}
			melt_time = lclock - (DELAY * 15 / snow_freq / 6 * 2);
		}
	}
	void set_melt_ratio(int to){
		melt_time = lclock + (melt_time - lclock) / snow_freq * to;
	}
	void flow() {
		if (top > left->top + 2 && top > right->top + 2) { // if both of snows can flow down
			if (rand() % 2) { // flow to random place
				left->stackup();
				meltup();
			}
			else {
				right->stackup();
				meltup();
			}
		}
		else {
			if (top > left->top + 2) {
				left->stackup();
				meltup();
			}
			if (top > right->top + 2) {
				right->stackup();
				meltup();
			}
		}
	}
	void link_to_right(struct Map* point) {
		right = point;
		point->left = this;
	}

	void check_stacked() {
		check_stacked(line.first);
		check_stacked(line.second);
	}
	void check_stacked(struct snow* li) {
		ll h = (top + BIT - ((y - 1) % BIT + 1)) / BIT;
		for (uint i = 0; i < h; i++) { // get bits under this bitfield
			ll cmp = li->bit[i];
			while (cmp) {
				li->bit[i] ^= cmp & -cmp;
				cmp ^= cmp & -cmp;
				stackup();
			}
		}
		if (!(top < y)) {
			fallingflag = false;
			return;
		}
		ll pos = (h == 0) ? top : (top - ((y - 1) % BIT + 1)) % BIT;
		int size = (h == 0) ? (y - 1) % BIT : BIT;
		ll cmp = (((li->bit[h] << (ll)(pos - (pos % 2) + 1)) & ((1LL << size) - 1)) >> (pos - (pos % 2) + 1)) ^ li->bit[h]; // get bits on this bitfield
		while (cmp) {
			li->bit[h] ^= cmp & -cmp;
			cmp ^= cmp & -cmp;
			stackup();
		}
	}

	bool floor() {
		return (top == 0 && melt == 2);
	}
};

std::vector<Map*> stacked;
std::vector<snow> falling;

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);
	std::cout.tie(NULL);

	std::system("title snow");
	set_console(2, 50);

	setup();

	while (1) {
		lclock = clock();
		bool outputflag = false;
		if (_kbhit() != 0) press(_getch());
		if (cmp_time == lclock) continue; // wait for time change
		if (pauseflag) { // if paused
			last_falling_time = cmp_time - last_falling_time + lclock; // make time equal as last
			for (uint i = 0; i < x / 2; i++) stacked[i]->melt_time = cmp_time - stacked[i]->melt_time + lclock;
		}
		cmp_time = lclock;
		if (cmp_time >= last_falling_time + DELAY) { // if time has passed
			outputflag = true;
			last_falling_time = cmp_time;
			pushsnow(fallingflag);

			no_snow_flag = true; // check snow is falling
			bool floorflag = true;
			for (uint i = 0; i < x / 2; i++) {
				stacked[i]->check_stacked();
				if (floorflag) floorflag = stacked[i]->floor();
			}
			for (uint i = 0; i < x && no_snow_flag; i++) {
				int len = falling[i].bit.size();
				for (int j = 0; j < len; j++) {
					if (no_snow_flag) no_snow_flag = (falling[i].bit[j] == 0);
				}
			}
			if (no_snow_flag && floorflag && !fallingflag) fallingflag = true;
		}
		for (uint i = 0; i < x / 2; i++) { // check melt
			if (cmp_time >= stacked[i]->melt_time + DELAY * 15 / snow_freq * ((x * 3 / 2 * (!no_snow_flag || fallingflag)) + 1)) {
				outputflag = true;
				stacked[i]->meltup();
			}
		}
		if (outputflag) screen(false);
	}
}

void setup() {
	std::cout << "input // length width : ";
	char t[1000];
	std::cin.getline(t, 1000);
	SEED = time(NULL);
	snow_freq = 1;
	if (inp(t)) { // change string, if input style is not correct, call input again
		system("cls");
		std::cout << "Style : [length] [width] <snow freq> <seed>\n";
		setup();
		return;
	}
	// if value is under than 2, has error so call input again
	if (min(x, y) == 0) {
		std::cout << "Minimum value : 1\n";
		setup();
		return;
	}

	// settings //
	x *= 2;
	y *= 2;
	stacked.clear();
	falling.clear();
	px = 1 * (2), py = 0;
	last_falling_time = lclock;
	last_melt_time = lclock;
	cmp_time = lclock;
	pauseflag = false;
	if (snow_freq < 1) snow_freq = 1;
	if (snow_freq > x) snow_freq = x;
	srand(SEED);
	set_console(y / 2 + 3, max(x * 2 / 2, 50) + 4);
	if (max(x, 50) != x) px = 26 - (x / 2);
	system("cls");
	// settings //

	//set sizes of vectors
	falling.resize(x);
	for (uint i = 0; i < x; i++) falling[i].bit.resize((y - 1) / BIT + 1); // falling
	for (uint i = 0; i < x; i += 2) stacked.push_back(new Map({ &falling[i], &falling[i + 1] })); // stacked => with falling on this line

	stacked[0]->left = stacked[0];
	for (uint i = 0; i < x / 2 - 1; i++) stacked[i]->link_to_right(stacked[i + 1]); // connect to left and right (to flow near)
	stacked[x / 2 - 1]->right = stacked[x / 2 - 1];

	clear();
	screen(true);
	footer();
}
bool inp(char* t) { // change string from input to numbers
	int pivot = 0;
	int len = strlen(t);
	while (pivot < len && t[pivot] == ' ') pivot++; // erase space
	y = 0;
	while (pivot < len && t[pivot] != ' ') { // get number
		y *= 10;
		if (t[pivot] < '0' || '9' < t[pivot]) return 1; // if not number, return
		y += t[pivot] - '0';
		pivot++;
	}
	while (pivot < len && t[pivot] == ' ') pivot++;
	if (pivot >= len) return 1;
	x = 0;
	while (pivot < len && t[pivot] != ' ') {
		x *= 10;
		if (t[pivot] < '0' || '9' < t[pivot]) return 1;
		x += t[pivot] - '0';
		pivot++;
	}
	while (pivot < len && t[pivot] == ' ') pivot++;
	if (pivot >= len) return 0;
	snow_freq = 0;
	while (pivot < len && t[pivot] != ' ') {
		snow_freq *= 10;
		if (t[pivot] < '0' || '9' < t[pivot]) {
			x = -1;
			return 1;
		}
		snow_freq += t[pivot] - '0';
		pivot++;
	}
	while (pivot < len && t[pivot] == ' ') pivot++;
	if (pivot >= len) return 0;
	SEED = 0;
	while (pivot < len && t[pivot] != ' ') {
		SEED *= 10;
		if (t[pivot] < '0' || '9' < t[pivot]) {
			x = -1;
			return 1;
		}
		SEED += static_cast<__int64>(t[pivot]) - '0';
		pivot++;
	}
	while (pivot < len && t[pivot] == ' ') pivot++;
	if (pivot >= len) return 0;
	x = -1;
	return 1;
}
void clear() { // fill black on console in range 
	textcolor(WHITE, BLACK);
	for (uint i = 0; i < (y / 2) + 4; i++) {
		gotoxy(px - 2, py + i);
		for (uint j = 0; j < x + 4; j++) std::cout << " ";
	}
	gotoxy(0, 0);
}
void press(char chr) { // catch key pressed
	if (!pauseflag) {
		if ((chr == '-' || chr == '_') && snow_freq > 1) {
			for (int i = 0; i < x / 2; i++) stacked[i]->set_melt_ratio(snow_freq - 1);
			snow_freq--;
		}
		if ((chr == '+' || chr == '=') && snow_freq < x) {
			for (int i = 0; i < x / 2; i++) stacked[i]->set_melt_ratio(snow_freq - 1);
			snow_freq++;
		}
		if (chr == 'c' || chr == 'C') {
			if (fallingflag) fallingflag = false;
			else fallingflag = true;
		}
		if (chr == 'r' || chr == 'R') {
			set_console(y / 2 + 3, max(x * 2 / 2, 50) + 4);
			screen(true);
		}
		if (chr == 27) {
			main();
			exit(1);
		}
		footer();
	}
	if (chr == ' ') {
		if (pauseflag) pauseflag = false;
		else pauseflag = true;
		screen(false);
		footer();
	}
}
void pushsnow(bool pushflag) { // make snow
	std::vector<bool> pos(x, false);
	if (pushflag) {
		int cnt = 0;
		for (int i = 0; i < x; i++) {
			pos[i] = ((rand() % (x - i)) < snow_freq - cnt);
			cnt += pos[i];
		}
	}
	int h = (y - 1) / BIT; // get top of bitfield
	for (uint i = 0; i < x; i++) falling[i].push(pos[i]); // push
}
void screen(bool reload) {
	int half[3] = { WHITE,LIGHTGRAY,BLACK };
	int full[3] = { LIGHTGRAY,GRAY,GRAY };
	std::string outputs[7] = {
		"__", "▒ ", " ", "'", ".",":"
	};

	for (uint i = 0; i < x; i += 2) {
		for (uint j = 0; j < y; j += 2) {
			ll top = stacked[i / 2]->top;
			int h = (y - 1) / BIT - (j / BIT);
			int pos = j % BIT;
			if (!reload && !(falling[i].bit[h] >> pos) && !(falling[i + 1].bit[h] >> pos) && top <= (ll)y - ((j / BIT + 1) * BIT) - 4) { // no bit on bitfield, skip
				if (h == 0) break;

				if (!(falling[i].bit[h - 1] & 1) && !(falling[i + 1].bit[h - 1] & 1)) {
					j = (j / BIT + 1) * BIT - 2;
					continue;
				}
				else if (j != (j / BIT + 1) * BIT - 2) {
					j = (j / BIT + 1) * BIT - 4;
					continue;
				}
			}
			if (top == y - j - 2) { // snow level 1
				textcolor(half[stacked[i / 2]->melt], (half[stacked[i / 2]->melt] == WHITE) ? GRAY : BLACK);
				gotoxy(px + i, py + j / 2);
				std::cout << outputs[0];
			}
			else if (top == y - j - 1) { // snow level 2
				textcolor((stacked[i / 2]->melt == 2) ? LIGHTGRAY : WHITE, full[stacked[i / 2]->melt]);
				gotoxy(px + i, py + j / 2);
				std::cout << outputs[1];
			}
			else if (top >= y - j) { // full snow
				textcolor(WHITE, WHITE);
				gotoxy(px + i, py + j / 2);
				std::cout << outputs[2] << outputs[2]; 
				if (!reload && top > y - j + 3) break;
			}
			else {
				int val = 0;
				val += (bool)(falling[i].bit[h] & (1LL << pos)) << 0; // left top
				val += (bool)(falling[i].bit[h] & (1LL << (pos + 1))) << 1; // left bot
				val += (bool)(falling[i + 1].bit[h] & (1LL << pos)) << 2; // right top
				val += (bool)(falling[i + 1].bit[h] & (1LL << (pos + 1))) << 3; // right bot
				textcolor(WHITE, BLACK);
				if (val & 3) {
					if ((val & 3) == 3) {
						if ((h > 0 && pos == BIT - 2 && falling[i].bit[h - 1] & 1) || (falling[i].bit[h] & (1LL << (pos + 2)))) {
							gotoxy(px + i, py + j / 2);
							std::cout << outputs[5];
						}
					}
					else {
						gotoxy(px + i, py + j / 2);
						std::cout << ((val & 1) ? outputs[3] : outputs[4]);
					}
				}
				else if ((h > 0 && pos == BIT - 2 && falling[i].bit[h - 1] & 1) || // next bitfield
					(falling[i].bit[h] & (1LL << (pos + 2))) || top >= (ll)y - j - 4 ||
					(stacked[i / 2]->melt_time == lclock - (DELAY * 15 / snow_freq / 6 * 2) && y - j - 1 < top)) {
					
					gotoxy(px + i, py + j / 2);
					std::cout << outputs[2];
				}
				if (val & 12) {
					if ((val & 12) == 12) {
						if ((h > 0 && pos == BIT - 2 && !(falling[i + 1].bit[h - 1] & 1)) || !(falling[i + 1].bit[h] & (1LL << (pos + 2)))) {
							gotoxy(px + i + 1, py + j / 2);
							std::cout << outputs[5];
						}
					}
					else {
						gotoxy(px + i + 1, py + j / 2);
						std::cout << ((val & 4) ? outputs[3] : outputs[4]);
					}
				}
				else if ((h > 0 && pos == BIT - 2 && falling[i + 1].bit[h - 1] & 1) ||  // next bitfield
					(falling[i + 1].bit[h] & (1LL << (pos + 2))) || top >= (ll)y - j - 4 ||
					(stacked[i / 2]->melt_time == lclock - (DELAY * 15 / snow_freq / 6 * 2) && y - j - 1 < top)) {
					gotoxy(px + i + 1, py + j / 2);
					std::cout << outputs[2];
				}
			}
		}
	}

	textcolor(GRAY, WHITE);
	for (uint i = 0; i < x; i += 2) {
		gotoxy(px + i, py + (y / 2));
		std::cout << outputs[2] << outputs[2]; // underline
	}
	for (uint i = 0; i < y / 2 + 1; i++) { // sideline
		gotoxy(px - 2, i + py);
		std::cout << outputs[2] << outputs[2];
		gotoxy(px + x, i + py);
		std::cout << outputs[2] << outputs[2];
	}
}

void footer() {
	textcolor(WHITE, BLACK);
	if (pauseflag) {
		textcolor(WHITE, RED);
		gotoxy(0, py + y / 2 + 1);
		std::cout << "Paused";

		gotoxy(6, py + y / 2 + 1);
		textcolor(WHITE, BLACK);
		for (int i = 6; i < 50; i++) std::cout << " ";
		gotoxy(0, py + y / 2 + 2);
		for (int i = 0; i < 50; i++) std::cout << " ";
	}
	else {
		gotoxy(0, py + y / 2 + 1);
		std::cout << "Size : " << y / 2 << ", " << x / 2;

		gotoxy(37, py + y / 2 + 1);
		for (int i = 37; i < 50; i++) std::cout << " ";
		gotoxy(25, py + y / 2 + 1);
		std::cout << "Snow freq : " << snow_freq;

		gotoxy(0, py + y / 2 + 2);
		std::cout << "Seed : " << SEED;
	}
}

void set_console(int fy, int fx) {
	Remove_cursor();
	char cmd[256];
	sprintf(cmd, "mode con: lines=%d cols=%d", fy, fx);
	system(cmd);
}
