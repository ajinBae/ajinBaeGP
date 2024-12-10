#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <Windows.h>
#include "common.h"
#include "io.h"
#include "display.h"

#define DOUBLE_TAP 300 //2번 더 움직이기

void init(void);
void intro(void);
void outro(void);
void cursor_move(DIRECTION dir);
void sample_obj_move(void);
POSITION sample_obj_next_position(void);
DIRECTION previousDir = d_stay;
clock_t previousTime = 0;


/* ================= control =================== */
int sys_clock = 0;      // system-wide clock(ms)
CURSOR cursor = { { 1, 1 }, {1, 1} };


/* ================= game data =================== */
char map[N_LAYER][MAP_HEIGHT][MAP_WIDTH] = { 0 };



/*int color_map[N_LAYER][MAP_HEIGHT][MAP_WIDTH]; //1번 초기상태

void textcolor(int colorNum) {
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorNum);
}

enum ColorType { BLUE = 9, RED = 12, DARK_YELLOW = 6, BLACK = 0, YELLOW = 14, GRAY = 8 } COLOR;

void print_colored_char(char ch, int color) {
   textcolor(color);
   printf("%c", ch);
   textcolor(7);
}

void display_textcolors() {
   for (int i = 0; i < MAP_HEIGHT; i++) {
     for (int j = 0; j < MAP_WIDTH; j++) {
       int color = color_map[0][i][j];
       char ch = map[0][i][j];
       print_colored_char(ch, color);
     }
   }
}*/



RESOURCE resource = {
   .spice = 0,
   .spice_max = 0,
   .population = 0,
   .population_max = 0
};

OBJECT_SAMPLE obj = {
   .pos = {1, 1},
   .dest = {MAP_HEIGHT - 2, MAP_WIDTH - 2},
   .repr = 'o',
   .speed = 300,
   .next_move_time = 300
};

/* ================= main() =================== */
int main(void) {
    srand((unsigned int)time(NULL));

    init();
    intro();
    display(resource, map, cursor);

    while (1) {
        // loop 돌 때마다(즉, TICK==10ms마다) 키 입력 확인
        KEY key = get_key();

        // 키 입력이 있으면 처리
        if (is_arrow_key(key)) {
            cursor_move(ktod(key));
        }
        else {
            // 방향키 외의 입력
            switch (key) {
            case k_quit: outro();
            case k_none:
            case k_undef:
            default: break;
            }
        }

        // 샘플 오브젝트 동작
        sample_obj_move();

        // 화면 출력
        display(resource, map, cursor);
        Sleep(TICK);
        sys_clock += 10;
    }
}

/* ================= subfunctions =================== */
void intro(void) {
    printf("DUNE 1.5\n");
    Sleep(2000);
    system("cls");
}

void outro(void) {
    printf("exiting...\n");
    exit(0);
}

void init(void) {
    srand((unsigned int)time(NULL));


    // layer 0(map[0])에 지형 생성
    for (int j = 0; j < MAP_WIDTH; j++) {
        map[0][0][j] = '#';
        map[0][MAP_HEIGHT - 1][j] = '#';
    }

    for (int i = 1; i < MAP_HEIGHT - 1; i++) {
        map[0][i][0] = '#';
        map[0][i][MAP_WIDTH - 1] = '#';
        for (int j = 1; j < MAP_WIDTH - 1; j++) {
            map[0][i][j] = ' ';
        }
    }

    // layer 1(map[1])은 비워 두기(-1로 채움)
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            map[1][i][j] = -1;
        }
    }

    // object sample
    //아트레이디스, 하베스터, 스파이스
    map[0][MAP_HEIGHT - 2][1] = 'B'; //color_map[0][MAP_HEIGHT - 2][1] = 9;
    map[0][MAP_HEIGHT - 2][2] = 'B'; //color_map[0][MAP_HEIGHT - 2][2] = 9;
    map[0][MAP_HEIGHT - 3][1] = 'B'; map[0][MAP_HEIGHT - 3][2] = 'B';
    map[0][MAP_HEIGHT - 2][3] = 'P'; map[0][MAP_HEIGHT - 2][4] = 'P';
    map[0][MAP_HEIGHT - 3][3] = 'P'; map[0][MAP_HEIGHT - 3][4] = 'P';
    map[1][MAP_HEIGHT - 4][1] = 'H';
    map[0][MAP_HEIGHT - 6][1] = '0' + (rand() % 9 + 1);

    //하코넨, 하베스터, 스파이스
    map[0][1][MAP_WIDTH - 2] = 'B'; map[0][1][MAP_WIDTH - 3] = 'B';
    map[0][2][MAP_WIDTH - 2] = 'B';   map[0][2][MAP_WIDTH - 3] = 'B';
    map[0][1][MAP_WIDTH - 4] = 'P';   map[0][1][MAP_WIDTH - 5] = 'P';
    map[0][2][MAP_WIDTH - 4] = 'P';   map[0][2][MAP_WIDTH - 5] = 'P';
    map[1][MAP_HEIGHT - 4][MAP_WIDTH - 2] = 'H';

    // 게임 시작 시 초기화할 다른 값들 설정
    resource.spice = 50;         // 초반 스파이스
    resource.spice_max = 100;    // 최대 스파이스
    resource.population = 20;    // 초반 인구
    resource.population_max = 100; // 최대 인구

    obj.pos = (POSITION){ 2, 2 }; // 샘플 객체의 초기 위치 설정
    obj.dest = (POSITION){ MAP_HEIGHT - 2, MAP_WIDTH - 2 }; // 샘플 객체의 목표 위치 설정
    obj.repr = 'o'; // 샘플 객체의 대표 문자
    obj.speed = 300; // 샘플 객체의 이동 속도 설정
    obj.next_move_time = 300; // 샘플 객체의 다음 이동 시간 설정

    // 게임 시스템의 기본 설정
    previousDir = d_stay;   // 이전 방향 초기화
    previousTime = 0;        // 이전 시간 초기화
}

// (가능하다면) 지정한 방향으로 커서 이동
void cursor_move(DIRECTION dir) {
    POSITION curr = cursor.current;
    POSITION new_pos = pmove(curr, dir);

    // validation check
    if (1 <= new_pos.row && new_pos.row <= MAP_HEIGHT - 2 && \
        1 <= new_pos.column && new_pos.column <= MAP_WIDTH - 2) {

        cursor.previous = cursor.current;
        cursor.current = new_pos;
    }
}

/* ================= sample object movement =================== */
POSITION sample_obj_next_position(void) {
    // 현재 위치와 목적지를 비교해서 이동 방향 결정   
    POSITION diff = psub(obj.dest, obj.pos);
    DIRECTION dir;

    // 목적지 도착. 지금은 단순히 원래 자리로 왕복
    if (diff.row == 0 && diff.column == 0) {
        if (obj.dest.row == 1 && obj.dest.column == 1) {
            // topleft --> bottomright로 목적지 설정
            POSITION new_dest = { MAP_HEIGHT - 2, MAP_WIDTH - 2 };
            obj.dest = new_dest;
        }
        else {
            // bottomright --> topleft로 목적지 설정
            POSITION new_dest = { 1, 1 };
            obj.dest = new_dest;
        }
        return obj.pos;
    }

    // 가로축, 세로축 거리를 비교해서 더 먼 쪽 축으로 이동
    if (abs(diff.row) >= abs(diff.column)) {
        dir = (diff.row >= 0) ? d_down : d_up;
    }
    else {
        dir = (diff.column >= 0) ? d_right : d_left;
    }

    // validation check
    // next_pos가 맵을 벗어나지 않고, (지금은 없지만)장애물에 부딪히지 않으면 다음 위치로 이동
    // 지금은 충돌 시 아무것도 안 하는데, 나중에는 장애물을 피해가거나 적과 전투를 하거나... 등등
    POSITION next_pos = pmove(obj.pos, dir);
    if (1 <= next_pos.row && next_pos.row <= MAP_HEIGHT - 2 && \
        1 <= next_pos.column && next_pos.column <= MAP_WIDTH - 2 && \
        map[1][next_pos.row][next_pos.column] < 0) {

        return next_pos;
    }
    else {
        return obj.pos;  // 제자리
    }
}

void sample_obj_move(void) {
    if (sys_clock <= obj.next_move_time) {
        // 아직 시간이 안 됐음
        return;
    }

    // 오브젝트(건물, 유닛 등)은 layer1(map[1])에 저장
    map[1][obj.pos.row][obj.pos.column] = -1;
    obj.pos = sample_obj_next_position();
    map[1][obj.pos.row][obj.pos.column] = obj.repr;

    obj.next_move_time = sys_clock + obj.speed;
}