#ifndef MNTR_H
#define MNTR_H

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef DELETE
	#define DELETE 1
#endif
#ifndef SIZE
	#define SIZE 2
#endif
#ifndef RECOVER
	#define RECOVER 3
#endif
#ifndef TREE
	#define TREE 4
#endif
#ifndef EXIT
	#define EXIT 5
#endif
#ifndef HELP
	#define HELP 6
#endif
#ifndef ENTER
	#define ENTER 7
#endif
#ifndef PATH_MAX 
	#define PATH_MAX 1024
#endif
#ifndef BUFLEN
	#define BUFLEN 64
#endif
#ifndef COMMAND_SIZE 
	#define COMMAND_SIZE 64
#endif
#ifndef FILE_MAX
	#define FILE_MAX 2000
#endif

struct checkTime{
	char fname[FILE_MAX];
	char ftime[BUFLEN];
};
struct myFile{
	char fname[FILE_MAX];
	int flen;
};

int ssu_daemon_init(void);//디몬 프로세스 작업을 진행하는 함수
int daemon_file_check(char *dir);//check파일의 파일 개수를 알려주는 함수
void what_creat_file(char *dir, char file_collection[BUFLEN][PATH_MAX], int cnt);//디렉토리에 존재하는 파일들을 2차원 배열에 초기화시킴
char *what_creat_file2(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX]);//어떠한 파일을 생성했는지 알려주는 함수
char *what_delete_file(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX]);//어떠한 파일을 삭제했는지 알려주는 함수
void info_file_creat(char *dir,FILE *fp);//어떠한 파일을 생성했는지 log.txt에 찍어주는 함수
void info_file_delete(char file_collection[BUFLEN][PATH_MAX], FILE *fp);//어떠한 파일을 삭제했는지 log.txt에 찍어주는 함수
void modify_file_check(char *dir, int cnt, FILE *fp);//어떠한 파일을 수정했는지 log.txt에 찍어주는 함수

void ssu_mntr();//디몬프로세스와 명령어 기능 수행하는 ssu_mntr프로그램
int select_command(char *filename, char *endTime, char *option);//명령어를 선택하는 함수
void play_command(char *filename, char *endTime, char *option, int select);//본격적인 명령을 수행하는 함수
void print_help();//help 명령어

void command_delete(char *filename, char *endTime, char *option);//delete 명령어 수행
void command_size(char *filename, char *option);//size 명령어 수행
void command_recover(char *filename, char *option);//recover 명령어 수행
void command_tree(char *filename,int depth);//tree 명령어 수행

void do_delete(char *filename);//파일을 삭제하는 함수
int is_samefile(char *filename);//삭제 시 trash디렉토리에 같은 이름의 파일이 존재할 때 수행하는 함수
void old_time_delete(int size); //2kb 넘어간다면 오래된 파일 삭제
void do_timeOption(char *filename, char *endTime);//endtime이 존재한다면 수행하는 함수
char do_rOption(); // 정말로 삭제할지 되묻는 함수 
int check_dir_size(char *dir, int num);//디렉토리의 size를 출력해주는 함수
void sort_file();//size 명령어 수행 시, 오름차순으로 정렬하여 출력
void sort_time(char *c[FILE_MAX], char t[FILE_MAX][FILE_MAX], int size);//시간 순으로 정렬하는 함수

#endif
