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

using namespace std;

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

#define DELAY 250
uint x, y;
uint px = 1 * (2), py = 0;
time_t SEED = time(NULL);
time_t last_falling_time = clock();
time_t last_melt_time = clock();
time_t cmp_time = clock();
int snow_on_tick = 1;

bool fallingflag = true;
bool pauseflag = false;
bool no_snow_flag = true;

#define BIT 60
struct snow {
	vector<ll> bit;
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
	struct Map* left = this;
	struct Map* right = this;
	time_t melt_time;
	pair<snow*, snow*> line;
	Map(pair<snow*, snow*> li) :line(li) {}
	void stackup() {
		melt--;
		if (melt < 0) {
			if (top < y) {
				melt = 2;
				top++;
				flow();
			}
		}
		melt_time = clock();
		left->melt_time = clock();
		right->melt_time = clock();
	}
	void meltup() {
		if (!(melt == 2 && top == 0)) {
			melt++;
			if (melt == 3) {
				melt = 0;
				top--;
				left->flow();
				right->flow();
			}
			melt_time = clock() - (DELAY * 15 / snow_on_tick / 6 * 2);
		}
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
		ll h = (top + BIT - ((y - 1) % BIT) - 1) / BIT;
		if (h >= (y - 1) / BIT + 1) {
			fallingflag = false;
			return;
		}
		ll pos = (top <= (y - 1) % BIT) ? top : (top - ((y - 1) % BIT)) % BIT;
		int size = (top <= (y - 1) % BIT) ? (y - 1) % BIT : BIT;
		ll cmp = (((li->bit[h] << (ll)(pos - (pos % 2) + 1)) & ((1LL << size) - 1)) >> (pos - (pos % 2) + 1)) ^ li->bit[h]; // get bits on this bitfield
		while (cmp) {
			li->bit[h] ^= cmp & -cmp;
			cmp ^= cmp & -cmp;
			stackup();
		}
		for (uint i = 0; i < h; i++) { // get bits under this bitfield
			cmp = li->bit[i];
			while (cmp) {
				li->bit[h] ^= cmp & -cmp;
				cmp ^= cmp & -cmp;
				stackup();
			}
		}
	}

	bool none() {
		return !top && melt == 2;
	}
};

vector<Map*> stacked;
vector<snow> falling;

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);
	cout.tie(NULL);

	system("title snow");
	system("mode con: lines=2 cols=50");

	setup();

	while (1) {
		bool outputflag = false;
		if (_kbhit() != 0) press(_getch());
		if (cmp_time == clock()) continue; // wait for time change
		if (pauseflag) { // if paused
			last_falling_time = cmp_time - last_falling_time + clock(); // make time equal as last
			for (uint i = 0; i < x / 2; i++) stacked[i]->melt_time = cmp_time - stacked[i]->melt_time + clock();
		}
		cmp_time = clock();
		if (cmp_time >= last_falling_time + DELAY) { // if time has passed
			last_falling_time = cmp_time;
			pushsnow(fallingflag);
			for (uint i = 0; i < x / 2; i++) stacked[i]->check_stacked();
			outputflag = true;

			no_snow_flag = true; // check snow is falling
			bool floorflag = true;
			for (uint i = 0; i < x; i++) {
				if (no_snow_flag && !fallingflag) {
					int len = falling[i].bit.size();
					for (int k = 0; k < len && no_snow_flag; k++) {
						no_snow_flag = !falling[i].bit[k];
					}
				}
				if (floorflag) floorflag = stacked[i / 2]->none();
			}
			if (no_snow_flag && floorflag && !fallingflag) fallingflag = true;
		}
		if (outputflag) screen(false);
		for (uint i = 0; i < x / 2; i++) { // check melt
			if (cmp_time >= stacked[i]->melt_time + DELAY * 15 / snow_on_tick * ((x * 6 / 2 * (!no_snow_flag || fallingflag)) + 1)) {
				stacked[i]->meltup();
			}
		}
	}
}

void setup() {
	Remove_cursor();
	std::cout << "input // length width : ";
	char t[1000];
	cin.getline(t, 1000);
	if (inp(t)) { // change string, if input style is not correct, call input again
		system("cls");
		std::cout << "Style : [length] [width] <seed>\n";
		setup();
		return;
	}
	// if value is under than 2, has error so call input again
	if (min(x, y) < 4) { // in inp() function multiplied 2 so compare with 4
		std::cout << "Minimum value : 2\n";
		setup();
		return;
	}

	// settings //
	stacked.clear();
	falling.clear();
	px = 1 * (2), py = 0;
	last_falling_time = clock();
	last_melt_time = clock();
	cmp_time = clock();
	pauseflag = false;
	snow_on_tick = 1;
	srand(SEED);
	char cmd[256];
	sprintf(cmd, "mode con: lines=%d cols=%d", y / 2 + 3, max(x * 2 / 2, 50) + 4);
	if (max(x, 50) != x) px = 26 - (x / 2);
	system(cmd);
	system("cls");
	// settings //

	//set sizes of vectors
	falling.resize(x);
	for (uint i = 0; i < x; i++) falling[i].bit.resize((y - 1) / BIT + 1); // falling
	for (uint i = 0; i < x; i += 2) stacked.push_back(new Map({ &falling[i], &falling[i + 1] })); // stacked => with falling on this line

	stacked[0]->left = stacked[0];
	for (uint i = 0; i < x / 2 - 1; i++) stacked[i]->link_to_right(stacked[i + 1]); // connect to left and right (to flow near)
	stacked[x / 2 - 1]->right = stacked[x / 2 - 2];

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
	x *= 2;
	y *= 2;
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
		if ((chr == '-' || chr == '_') && snow_on_tick > 1) snow_on_tick--;
		if ((chr == '+' || chr == '=') && snow_on_tick < x / 2) snow_on_tick++;
		if (chr == 'c' || chr == 'C') {
			if (fallingflag) fallingflag = false;
			else fallingflag = true;
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
	textcolor(WHITE, BLACK);
}
void pushsnow(bool pushflag) { // make snow
	vector<bool> pos(x, false);
	if (pushflag) {
		int loop = snow_on_tick;
		while (loop--) {
			pos[rand() % x] = true;
		}
	}
	int h = (y - 1) / BIT; // get top of bitfield
	for (uint i = 0; i < x; i++) { // push
		bool val =
			pos[i] &&
			(0 == (falling[i].bit[h] & 1)) &&
			(0 == (falling[i + 1].bit[h] & 1));
		falling[i].push(val);
		i++;
		falling[i].push(
			pos[i] &&
			(0 == (falling[i].bit[h] & 1)) &&
			(0 == val));
	}
}
void screen(bool reload) {
	int half[3] = { WHITE,LIGHTGRAY,BLACK };
	int full[3] = { LIGHTGRAY,GRAY,GRAY };
	string outputs[6] = {
		"__", "▒ ", " ", "'", ".", "■"
	};

	for (uint i = 0; i < x; i += 2) {
		for (uint j = 0; j < y; j += 2) {
			ll top = stacked[i / 2]->top;
			int h = (y - 1) / BIT - (j / BIT);
			int pos = j % BIT;
			if (!reload && h != 0 && falling[i].bit[h] >> pos == 0 && falling[i + 1].bit[h] >> pos == 0 &&
				!(top >= y - ((j / BIT + 1) * BIT) - 2) && !(falling[i].bit[h - 1] & 1) && !(falling[i + 1].bit[h - 1] & 1)) { // no bit on bitfield, skip
				j = (j / BIT + 1) * BIT - 2;
				continue;
			}
			if (top == y - (ll)j - 2) { // snow level 1
				gotoxy(px + i, py + j / 2);
				textcolor(half[stacked[i / 2]->melt], (half[stacked[i / 2]->melt] == WHITE) ? GRAY : BLACK);
				std::cout << outputs[0];
			}
			else if (top == y - (ll)j - 1) { // snow level 2
				gotoxy(px + i, py + j / 2);
				textcolor((stacked[i / 2]->melt == 2) ? LIGHTGRAY : WHITE, full[stacked[i / 2]->melt]);
				std::cout << outputs[1];
			}
			else if (top > y - (ll)j - 1) { // full snow
				gotoxy(px + i, py + j / 2);
				textcolor(WHITE, WHITE);
				std::cout << outputs[2] << outputs[2];
				if (!reload && top > y - (ll)j) break;
			}
			else {
				int val = 0;
				val += (bool)(falling[i].bit[h] & (1LL << pos)) << 0; // left top
				val += (bool)(falling[i].bit[h] & (1LL << (pos + 1))) << 1; // left bot
				val += (bool)(falling[i + 1].bit[h] & (1LL << pos)) << 2; // right top
				val += (bool)(falling[i + 1].bit[h] & (1LL << (pos + 1))) << 3; // right bot
				textcolor(WHITE, BLACK);
				if (val & 3) {
					gotoxy(px + i, py + j / 2);
					std::cout << (((val & 3) == 1) ? outputs[3] : outputs[4]); // left
				}
				else if ((h > 0 && pos == BIT - 2 && falling[i].bit[h - 1] & 1) || // next bitfield
					falling[i].bit[h] & (1LL << (pos + 2)) || top >= y - ((ll)j + 2) - 2) { // when snow is under it
					gotoxy(px + i, py + j / 2);
					std::cout << outputs[2];
				}
				if (val & 12) {
					gotoxy(px + i + 1, py + j / 2);
					std::cout << (((val & 12) == 4) ? outputs[3] : outputs[4]); // right
				}
				else if ((h > 0 && pos == BIT - 2 && falling[i + 1].bit[h - 1] & 1) ||  // next bitfield
					falling[i + 1].bit[h] & (1LL << (pos + 2)) || top >= y - ((ll)j + 2) - 2) { // when snow is under it
					gotoxy(px + i + 1, py + j / 2);
					std::cout << outputs[2];
				}
			}
		}
	}

	gotoxy(px, py + y / 2);
	textcolor(WHITE, BLACK);
	for (uint i = 0; i < x; i += 2) std::cout << outputs[5]; // underline
	for (uint i = 0; i < y / 2 + 1; i++) { // sideline
		gotoxy(px - 2, i + py);
		std::cout << outputs[5];
		gotoxy(px + x, i + py);
		std::cout << outputs[5];
	}
}

void footer() {
	if (pauseflag) {
		gotoxy(0, py + y / 2 + 1);
		textcolor(WHITE, RED);
		std::cout << "Paused";

		textcolor(WHITE, BLACK);
		gotoxy(6, py + y / 2 + 1);
		for (int i = 6; i < 50; i++) std::cout << " ";
		gotoxy(0, py + y / 2 + 2);
		for (int i = 0; i < 50; i++) std::cout << " ";
	}
	else {
		gotoxy(0, py + y / 2 + 1);
		std::cout << "Size : " << y / 2 << ", " << x / 2;

		gotoxy(40, py + y / 2 + 1);
		for (int i = 40; i < 50; i++) std::cout << " ";
		gotoxy(25, py + y / 2 + 1);
		std::cout << "Snow on tick : " << snow_on_tick;

		gotoxy(0, py + y / 2 + 2);
		std::cout << "Seed : " << SEED;
	}
}
