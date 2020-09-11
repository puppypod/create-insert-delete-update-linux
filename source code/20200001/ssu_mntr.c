#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<dirent.h>
#include<time.h>
#include<signal.h>
#include<syslog.h>
#include"ssu_mntr.h"

extern struct myFile mfile[FILE_MAX];
extern struct checkTime checktime[FILE_MAX];

struct myFile mfile[FILE_MAX];
struct checkTime checktime[FILE_MAX];
int size = 0;
int filesize, prevsize; //데몬 프로세스 비교를 위한 변수
int file_cnt;

char trashDir[BUFLEN] = "trash";
char programDir[PATH_MAX]; //소스코드 디렉토리(학번) 경로 => 초기 세팅
char pathDir[PATH_MAX]; // checkfile(특정 디렉토리 경로) => 초기 세팅
char chName[PATH_MAX]; // trash 디렉토리 경로 
char fDir[PATH_MAX]; // files 디렉토리 경로
char iDir[PATH_MAX]; // info 디렉토리 경로
char copy_iDir[PATH_MAX];
char searchDir[PATH_MAX]; //size 디렉토리 검색에서 응용됨
char treeDir[PATH_MAX];
char aPath[PATH_MAX]; //파일 삭제시 절대경로를 입력했을 때의 값
char cmd[COMMAND_SIZE];
char info_name[PATH_MAX];
char real[BUFLEN][PATH_MAX];
time_t modify_time[FILE_MAX];
int delete_flag = true;
int isModi;
int modify_flag;
int modify_cnt;
int modify_depth;
time_t save_time;
int dir_check[PATH_MAX];
int file_check[PATH_MAX];
int prev_dir_check[PATH_MAX];
int prev_file_check[PATH_MAX];

int iOption = false;
int rOption = false;
int dOption = false;
int dNum = 0;
int lOption = false;
int just_delete = false;

void ssu_mntr()// 본격적인 프로그램 시작
{
	pid_t pid;
	char name[PATH_MAX];
	char *prompt;
	char *p;
	int select;
	int c;
	char *filename;
	char *endTime;
	char *option;
	char f[100], e[100], o[100];
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char id[PATH_MAX];


	getcwd(pathDir,PATH_MAX);
	strncpy(id,pathDir,PATH_MAX);
	strncpy(programDir,pathDir,PATH_MAX);//programDir => 프로그램 실행 디렉토리


	if((dp = opendir(pathDir))==NULL){
		fprintf(stderr,"open dir error for %s\n",pathDir);
		exit(1);
	}
	while((dirp = readdir(dp))!=NULL){ // 소스코드 내부에 특정 디렉토리의 경로를 얻어온다.
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..") || !strcmp(dirp->d_name,"trash")){
			continue;
		}
		if(stat(dirp->d_name, &statbuf)<0){
			continue;
		}
		if(S_ISDIR(statbuf.st_mode)){
			realpath(dirp->d_name, treeDir);
			sprintf(pathDir,"%s/%s",pathDir,dirp->d_name);// pathDir => checkFile 디렉토리	
			chdir(pathDir);
		}
	}
	closedir(dp);
	if((pid = fork())<0){
		fprintf(stderr,"fork error\n");
		exit(1);
	}
	else if(pid == 0){ //디몬 프로세스 별도 생성하여 작업
		if(ssu_daemon_init()<0){
			fprintf(stderr,"ssu_daemon_init failed\n");
			exit(1);
		}
		exit(0);
	}
	
	strncpy(name,id,PATH_MAX);
	p = strtok(name,"/");
	while((p = strtok(NULL,"/"))!=NULL){//프롬프트 만들기
		prompt = p;	
	}

	sprintf(prompt,"%s%s",prompt,">");
	
	while(1){//명령어 입력 및 기능 수행하는 곳
		printf("%s",prompt);
		select = select_command(filename, endTime, option);
		if(select == ENTER){
			continue; 
		}
		else if(select == EXIT){
			break; 
		}
		else if(select == HELP){
			print_help();
		}
		iOption = false;
		rOption = false;
		dOption = false;
		lOption = false;
		chdir(pathDir);
		
	}
	return;
}
int ssu_daemon_init(void)//디몬프로세스 작업
{
	pid_t pid;
	int fd, maxfd;
	FILE *fp;
	char dir[PATH_MAX];
	char *name;
	char file_collection[BUFLEN][PATH_MAX];
	char present_collection[BUFLEN][PATH_MAX];

	if((pid = fork())<0){
		fprintf(stderr,"fork error\n");
		exit(1);
	}
	else if(pid!=0){//부모 프로세스 죽이기
		exit(0);
	}

	pid = getpid();	

	setsid();//세션의 리더를 변경

	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);

	maxfd = getdtablesize();
	for(fd = 0 ; fd<maxfd ; fd++){//입력 출력 에러 fd를 close()
		close(fd);
	}
	
	umask(0);
	chdir("/");//root디렉토리로 설정
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	/////////////////////////////디몬 작업///////////////////////////////
	chdir(pathDir);
//	memset(dir,'\0',sizeof(dir));
	strcpy(dir,pathDir);
	prevsize = daemon_file_check(dir);

	strcpy(dir,pathDir);
	file_cnt=0;
	// file_collection : 파일 모음, file_cnt : 파일 갯수
	memset(dir_check,0,sizeof(dir_check));
	memset(file_check,0,sizeof(file_check));
	delete_flag = true; //현재값을 file_check, dir_check에 넣어두면 안됨
	modify_flag = true;
	what_creat_file(dir, file_collection, 0); // 디렉토리 내부에 존재하는 파일 및 디렉토리 배열에 초기화
	for(int i=0 ; i<prevsize ; i++){//파일을 삭제할 때 이용되는 배열들
		prev_dir_check[i] = dir_check[i];
		prev_file_check[i] = file_check[i];
	}
	while(1){
		
		chdir(programDir);
		 
		/*if(access("exit",F_OK)>=0){//디몬 종료
			break;
		}*/

		fp = fopen("log.txt","a+");
		
		chdir(pathDir);
		strcpy(dir,pathDir);
		filesize = daemon_file_check(dir);
		modify_flag = true;

		if(prevsize < filesize){ ////////////////////////////파일 생성 check
			delete_flag = true; //현재값이 이후엔 과거값이 되기에 true 세팅
			strcpy(dir,pathDir);
			file_cnt=0;
			sleep(2);
			for(int i=0 ; i<filesize ; i++){
				memset(present_collection[i],'\0',sizeof(present_collection[i]));
			}
			memset(dir_check,0,sizeof(dir_check));
			memset(file_check,0,sizeof(file_check));
			what_creat_file(dir, present_collection, 0);//현재 디렉토리 정보 저장

			for(int i=0 ; i<filesize ; i++){//파일을 삭제할 때 과거 디렉토리 정보를 의미함
				prev_dir_check[i] = dir_check[i];
				prev_file_check[i] = file_check[i];
			}

			name = what_creat_file2(file_collection,present_collection);//현재 디렉토리와 과거 디렉토리의 비교를 통해 어떤 파일이 생성되었는지 확인
			memset(info_name,'\0',sizeof(info_name));
			strcpy(info_name,name);
			strcpy(dir,pathDir);
			isModi = false;
			info_file_creat(dir, fp);//log.txt에 로그를 찍음

			for(int i=0 ; i<filesize ; i++){
				strncpy(file_collection[i] ,present_collection[i],strlen(present_collection[i]));
				file_collection[i][strlen(present_collection[i])] = '\0';
			}
			prevsize = filesize;
		}
		else if(prevsize > filesize){ //////////////////////////////파일 삭제 check
			delete_flag = false; //현재값을 file_check, dir_check에 넣어두면 안됨
			strcpy(dir,pathDir);
			file_cnt=0;
			sleep(2);
			for(int i=0 ; i<prevsize ; i++){
				memset(present_collection[i],'\0',sizeof(present_collection[i]));
			}
			memset(dir_check,0,sizeof(dir_check));
			memset(file_check,0,sizeof(file_check));
			what_creat_file(dir, present_collection, 0);//현재 디렉토리 정보 저장

			name = what_delete_file(file_collection,present_collection);//삭제한 파일이 무엇인지 알려줌
			memset(info_name,'\0',sizeof(info_name));
			strcpy(info_name,name);
			strcpy(dir,pathDir);
			isModi = false;
			info_file_delete(file_collection, fp);//어떠한 파일을 삭제했는지 log.txt에 입력

			for(int i=0 ; i<filesize ; i++){
				strncpy(file_collection[i] ,present_collection[i],strlen(present_collection[i]));
				file_collection[i][strlen(present_collection[i])] = '\0';
			}
			prevsize = filesize;
		}
		else{//파일 수정 check
			modify_cnt = 0;
			strcpy(dir,pathDir);
			modify_flag = false;
			modify_file_check(dir, 0, fp);// 어떠한 파일을 수정했는지 확인	
		}
		fclose(fp);
		sleep(1);

	}
	fp = fopen("log.txt","a+");
	fprintf(fp,"%s\n","모니터링이 종료되었습니다!!!");
	fclose(fp);
	
	return 0;
}
void info_file_creat(char *dir, FILE *fp)//어떠한 파일을 생성했는지 log.txt에 로그를 찍음
{
	struct dirent* dirp;
	DIR *dp;
	struct stat statbuf;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];
	time_t t;
	struct tm m;

	chdir(dir);

	strncpy(prePath,dir,PATH_MAX);

	dp = opendir(dir);
	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name, "..")){
			continue;
		}
		chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
		stat(dirp->d_name, &statbuf);
	
		if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
			if(!strcmp(dirp->d_name, info_name)){
				t = statbuf.st_mtime;
				m = *localtime(&t);
				fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [create_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
				isModi = true;
				break;
			}
			realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
			info_file_creat(newPath,fp);
			if(isModi){
				t = statbuf.st_mtime;
				m = *localtime(&t);
				fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [modify_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
				break;
			}
			continue;
		}
	
		////////////일반 파일////////////////
		if(!strcmp(dirp->d_name, info_name)){
			t = statbuf.st_mtime;
			m = *localtime(&t);
			fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [create_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
			isModi = true;
			break;
		}

	}
	closedir(dp);
	return;
}
void info_file_delete(char file_collection[BUFLEN][PATH_MAX], FILE *fp)//어떠한 파일을 삭제했는지 log.txt에 정보를 찍음
{
	int i;
	int findNum;
	int depth =0;
	struct stat statbuf;
	time_t t;
	struct tm m;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];
	char *p;
	char savePath[PATH_MAX];
	char cpy[PATH_MAX];
	char cpy2[PATH_MAX];

	getcwd(prePath,PATH_MAX);

	for(i=0 ; i<prevsize ; i++){
		if(!strcmp(file_collection[i], info_name)){//삭제한 파일의 정보를 찍어줌
			stat(info_name, &statbuf);
			t = time(NULL);
			m = *localtime(&t);
			fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [delete_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, info_name);
			depth = prev_file_check[i];
			findNum = i;
			break;
		}
	}

	if(depth != 0){//삭제한 파일이 하위 디렉토리에 존재한다면
		for(i=findNum ; i>=0 ; i--){
			if(depth == prev_dir_check[i]){
				memset(savePath,'\0',sizeof(savePath));
				memset(newPath,'\0',sizeof(newPath));
				strcpy(cpy,file_collection[i]);
				strcpy(newPath,real[i]);
				strcpy(cpy2,newPath);

				chdir(newPath);
				
				stat(cpy, &statbuf);
				t = statbuf.st_mtime;
				m = *localtime(&t);
				fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [modify_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, cpy);
				depth--;
				if(depth==0){
					break;
				}
			}
		}
	}
	chdir(prePath);
}
void modify_file_check(char *dir, int cnt, FILE *fp)//어떠한 파일이 수정되었는지 check하는 함수
{
	
	time_t intertime;
	struct tm m;
	struct dirent* dirp;
	struct stat statbuf;
	DIR *dp;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];

	chdir(dir);

	strncpy(prePath,dir,PATH_MAX);

	dp = opendir(dir);
	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name, "..")){
			continue;
		}
		chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
		stat(dirp->d_name, &statbuf);
	
		if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
			if(modify_time[modify_cnt] != statbuf.st_mtime){

				intertime = statbuf.st_mtime;
				save_time = intertime;
				m = *localtime(&intertime);
				modify_time[modify_cnt] = statbuf.st_mtime;

				fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [modify_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
				modify_flag = true;
				modify_depth = cnt;	
				modify_depth--;
			}
			modify_cnt++;
			realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
			modify_file_check(newPath,cnt+1, fp);
			if(modify_flag && modify_depth == cnt){

				intertime = save_time;
				m = *localtime(&intertime);

				modify_depth--;	
				fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [modify_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
			}
			continue;
		}
	
		////////////일반 파일////////////////
		if(modify_time[modify_cnt] != statbuf.st_mtime){

				intertime = statbuf.st_mtime;
				m = *localtime(&intertime);
				modify_time[modify_cnt] = statbuf.st_mtime;

			fprintf(fp,"[%.4d-%.2d-%.2d %.2d:%.2d:%.2d] [modify_%s]\n",m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec, dirp->d_name);
		}
		modify_cnt++;
	}
	closedir(dp);
	return;
}
void what_creat_file(char *dir, char file_collection[BUFLEN][PATH_MAX], int cnt)//어떠한 파일을 생성했는지 log.txt에 정보를 찍음
{
	struct dirent* dirp;
	DIR *dp;
	struct stat statbuf;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];
	char r[PATH_MAX];

	chdir(dir);

	strncpy(prePath,dir,PATH_MAX);

	dp = opendir(dir);
	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name, "..")){
			continue;
		}
		chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
		stat(dirp->d_name, &statbuf);
	
		if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
			if(delete_flag){
				dir_check[file_cnt] = cnt+1;
				file_check[file_cnt] = cnt;
				getcwd(r,PATH_MAX);
				strcpy(real[file_cnt],r); //절대경로 저장
			}
			modify_time[file_cnt] = statbuf.st_mtime;		
			strcpy(file_collection[file_cnt++], dirp->d_name);
			realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
			what_creat_file(newPath, file_collection, cnt+1);
			continue;
		}
	
		////////////일반 파일////////////////
		if(delete_flag){
			file_check[file_cnt] = cnt;
		}
		modify_time[file_cnt] = statbuf.st_mtime;		
		strcpy(file_collection[file_cnt++], dirp->d_name);
	}
	closedir(dp);
	return;
}
char *what_creat_file2(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX])//어떠한 파일이 생성되었는지 그 값을 반환해줌
{
	int i,j;
	int isTrue;
	for(i=0 ; i<filesize ; i++){
		isTrue = false;
		for(j=0 ; j<prevsize ; j++){
			if(!strcmp(prev[j], present[i])){
				isTrue = true;
			}
		}
		if(!isTrue){
			return present[i];
		}
	}
}
char *what_delete_file(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX])//어떠한 파일이 삭제되었는지 그 값을 반환해줌
{
	int i,j;
	int isTrue;
	for(i=0 ; i<prevsize ; i++){
		isTrue = false;
		for(j=0 ; j<filesize ; j++){
			if(!strcmp(prev[i], present[j])){
				isTrue = true;
			}
		}
		if(!isTrue){
			return prev[i];
		}
	}
}

int daemon_file_check(char *dir)//파일의 개수를 출력해주는 함수
{
	struct dirent* dirp;
	DIR *dp;
	struct stat statbuf;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];
	int cnt=0;
	int filenum=0;

	chdir(dir);

	strncpy(prePath,dir,PATH_MAX);

	dp = opendir(dir);
	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name, "..")){
			continue;
		}
		chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
		stat(dirp->d_name, &statbuf);
	
		if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
			cnt++;
			realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
			filenum += daemon_file_check(newPath);
			continue;
		}
	
		////////////일반 파일////////////////
		cnt++;

	}
	closedir(dp);
	filenum += cnt;
	return filenum;
}

int select_command(char *filename, char *endTime, char *option)//어떠한 명령어를 입력했는지 알아내는 함수
{
	char copy[COMMAND_SIZE];
	int i;
	int cnt = 0;
	char *token;
	char *t[4];
	char *time;
	char space[COMMAND_SIZE] = " ";
	char *tok;
	char *p;
	FILE *fp;

	t[0]=(char*)malloc(sizeof(char)*PATH_MAX);
	
	fgets(cmd,sizeof(cmd),stdin);
	cmd[strlen(cmd)-1]='\0';
	strncpy(copy,cmd,COMMAND_SIZE);
	
	tok = strtok(copy," ");
	while((tok = strtok(NULL," "))!=NULL){
		t[cnt] = tok;
		cnt++;	
	}
	if(cnt==4){// full delete command
		filename = t[0]; //파일 이름 설정

		endTime = strcat(t[1],t[2]); // 날짜 및 시간 설정

		t[3]++;
		option = t[3]; // '-'을 제거한 옵션 설정 
		just_delete = false;
	}
	else if(cnt==3){// half delete command or size FILENAME -d NUMBER
		filename = t[0]; //파일 이름 설정
			
		if(!strncmp(t[1],"-",1)){ // size
			dOption = true;
			dNum = t[2][0]-48;
			t[1]++;
			option = t[1];
		}
		else{ // delete
			just_delete = false;
			endTime = strcat(t[1],t[2]);
			option = space;
		}

	}
	else if(cnt==2){
		just_delete = true;
		filename = t[0]; //파일 이름 설정

		t[1]++;
		option = t[1]; // 옵션 설정

		endTime = space;
	}
	else if(cnt==1){
		filename = t[0]; //파일 이름 설정

		endTime = space;

		option = space;
	}
	else{
		filename = treeDir;
		endTime = space;
		option = space;
	}
	


	if(!strncmp(cmd,"delete",6)){
		if(!strncmp(option,"i",1)){
			iOption = true;
		}
		else if(!strncmp(option,"r",1)){
			rOption = true;
		}
		play_command(filename,endTime,option,DELETE);
		return DELETE;
	}
	else if(!strncmp(cmd,"size",4)){
		if(!strncmp(option,"d",1)){
			dOption = true;
		}
		strncpy(searchDir,filename,PATH_MAX);
		play_command(filename,endTime,option,SIZE);
		return SIZE;
	}
	else if(!strncmp(cmd,"recover",7)){
		if(!strncmp(option,"l",1)){
			lOption = true;
		}
		play_command(filename,endTime,option,RECOVER);
		return RECOVER;
	}
	else if(!strncmp(cmd,"tree",4)){
		play_command(filename,endTime,option,TREE);
		return TREE;
	}
	else if(!strncmp(cmd,"exit",4)){

		/*chdir(programDir);
		fp = fopen("exit","w");
		fclose(fp);
		chdir(pathDir);*/

		return EXIT;
	}
	else if(!strncmp(cmd,"help",4)){
		return HELP;
	}
	else if(cmd[strlen(cmd)-1]=='\0'){
		return ENTER;	
	}
	else{
		return HELP;
	}
	
}

void play_command(char *filename, char *endTime, char *option, int select)//본격적으로 명령어 함수를 진행하는 함수
{
	int depth = 1;
	switch(select){
		case DELETE:
			command_delete(filename, endTime, option);
			break;
		case SIZE:
			command_size(filename, option);
			break;
		case RECOVER:
			command_recover(filename, option);
			break;
		case TREE:
			command_tree(filename,depth);
			break;
	}
	return;
}
void command_delete(char *filename, char *endTime, char *option)//delete 명령어를 수행
{
	DIR *dp;
	struct dirent *dirp;
	int isExist = false;
	char *p;
	char cpy[PATH_MAX];
	int i;
	int cnt=0;

	if(access(trashDir,F_OK)<0){ // trash 디렉토리 생성
		chdir(programDir);
		mkdir(trashDir,0755);
		sprintf(chName,"%s/%s",programDir,trashDir);

		chdir(chName);

		mkdir("files",0755);
		sprintf(fDir,"%s/%s",chName,"files"); // ./trash/files 
		mkdir("info",0755);
		sprintf(iDir,"%s/%s",chName,"info"); // ./trash/info
		sprintf(copy_iDir,"%s/%s",chName,"info"); // ./trash/info
		
		chdir(pathDir); // checkFile 경로

	}

	strcpy(cpy,filename);
	for(i=0 ; i<strlen(cpy) ; i++){
		if(cpy[i] == '/'){
			cnt++;	
		}
	}

	if(strstr(cpy,"/")!=NULL){//삭제할 파일을 절대경로로 입력 받았다는 것임
		cnt--;
		strncat(aPath,"/",1);
		p = strtok(cpy,"/");
		strcat(aPath,p);
		if(chdir(aPath)<0){
			fprintf(stderr,"%s 라는 경로는 존재하지 않습니다.\n",aPath);
			return;
		}
		cnt--;
		strncat(aPath,"/",1);
		while((p = strtok(NULL,"/"))!=NULL){
		
			if(cnt!=0){
				strcat(aPath,p);
				if(chdir(aPath)<0){
					fprintf(stderr,"%s 라는 경로는 존재하지 않습니다.\n",aPath);
					return;
				}
				strncat(aPath,"/",1);
			}
			else{
				filename = p;
			}
			cnt--;
		}
		aPath[strlen(aPath)-1] = '\0';
	}
	
	if(!strcmp(filename," ")){// 파일이름을 입력하지 않은 경우 에러 출력
		fprintf(stderr,"삭제할 파일을 입력하지 않았습니다. 파일 이름을 입력해주세요.\n");	
		return;
	}
	if(!strcmp(aPath,"")){
		if((dp = opendir(pathDir))==NULL){
			fprintf(stderr,"opendir error for %s\n",pathDir);
			return;
		}
		while((dirp = readdir(dp))!=NULL){
			if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
				continue;	
			}
			if(!strcmp(filename,dirp->d_name)){ // 삭제할 파일명이 존재한다면
				isExist = true;
				break;
			}
		}
		closedir(dp);
		
	}
	else{
		if((dp = opendir(aPath))==NULL){
			fprintf(stderr,"opendir error for %s\n",pathDir);
			return;
		}
		while((dirp = readdir(dp))!=NULL){
			if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
				continue;	
			}
			if(!strcmp(filename,dirp->d_name)){ // 삭제할 파일명이 존재한다면
				isExist = true;
				break;
			}
		}
		closedir(dp);
	}
	if(isExist){ // 삭제할 파일명이 존재
		if(strcmp(endTime," ")){ // 시간 존재
			do_timeOption(filename, endTime);
		}
		else{ // 삭제할 파일 또는 옵션 존재
			do_delete(filename);
		}
			
	}
	else{// 삭제할 파일명이 존재하지 않는다면
		fprintf(stderr,"삭제할 파일이 존재하지 않습니다. 다시 입력해주세요.\n");
		return;
	}


	return;
}
void command_size(char *filename, char *option)//size 명령어를 수행
{
	DIR *dp;
	struct dirent *dirp;
	off_t fsize;
	FILE *fp;	
	int all_size = 0;
	int isExist = false;
	char searchPath[PATH_MAX];
	struct stat statbuf;
	char cName[PATH_MAX];
	char print_path[PATH_MAX];

	chdir(programDir);

	if(dNum==0 && dOption){
		printf("size [FILENAME] [OPTION] [NUMBER]를 입력하셔야합니다.\n");
		return;
	}
		
	if((dp = opendir(programDir))==NULL){
		fprintf(stderr,"opendir error for %s\n",programDir);
		exit(1);
	}

	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		if(!strcmp(dirp->d_name,filename)){
			isExist = true;
			break;
		}
	}
	closedir(dp);

	if(!isExist){ //검색 파일 및 디렉토리가 존재하지 않으면 재입력 요구
		fprintf(stderr,"검색한 파일 또는 디렉토리가 존재하지않습니다. 다시 입력해주세요.\n");
		return;
	}

	strncpy(cName,filename,PATH_MAX);

	if(stat(cName, &statbuf)<0){
		fprintf(stderr,"stat error for %s\n",filename);
		exit(1);
	}	

	if(S_ISDIR(statbuf.st_mode)){ //검색 파일이 디렉토리라면
		realpath(cName,searchPath);

		all_size = check_dir_size(searchPath, dNum);
		if(!dOption){
			sprintf(print_path,"./%s",filename);
			printf("%-10d%s\n",all_size,print_path);
		}
		else{
			sort_file();	
		}
	}
	else{ //검색 파일이 일반 파일이라면
		if((fp = fopen(cName,"r"))==NULL){
			fprintf(stderr,"fopen error for %s\n",cName);
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		all_size = ftell(fp);
		printf("%-10d%s\n",all_size,filename);
		fclose(fp);
	}

	chdir(pathDir); // 원래 디렉토리 작업파일로 돌려놓음
	return;
}

void sort_file()//오름차순으로 정렬해 주는 bubble sort 함수
{
	struct myFile tmp;
	int i,j,k;
	char print_name[FILE_MAX];

	for(i=0 ; i<size-1 ; i++){
		for(j = 0 ; j<size-1-i ; j++){
			if(strcmp(mfile[j].fname,mfile[j+1].fname)>0){
				memcpy(tmp.fname,mfile[j].fname,FILE_MAX);
				tmp.flen = mfile[j].flen;

				memcpy(mfile[j].fname,mfile[j+1].fname,FILE_MAX);
				mfile[j].flen = mfile[j+1].flen;

				memcpy(mfile[j+1].fname,tmp.fname,FILE_MAX);
				mfile[j+1].flen = tmp.flen;
			}
		}
	}
	for(i=0 ; i<size ; i++){
		printf("%-10d",mfile[i].flen);
		printf("./%s\n",mfile[i].fname);
	}
	size=0;
}

int check_dir_size(char *dir, int num)//디렉토리의 크기를 측정해주는 함수
{
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	int fsize = 0;
	FILE *fp;
	char newPath[PATH_MAX];
	char prePath[PATH_MAX];
	int cntDir;
	char print_dir[PATH_MAX];
	char *p, *p2;
	char *name, *name2;
	int fsize2;
	char relativeDir[PATH_MAX];
	char savePath[PATH_MAX];

	chdir(dir);
	cntDir = num;

	if(!dOption){ // d 옵션이 아닐때
		strncpy(prePath,dir,PATH_MAX);
	
		if((dp = opendir(dir)) == NULL){
			fprintf(stderr,"open dir error for %s\n",dir);
			exit(1);
		}
	
		while((dirp = readdir(dp)) != NULL){
			if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
				continue;
			}
	
			chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
			stat(dirp->d_name, &statbuf);
	
			if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
				realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
				fsize += check_dir_size(newPath, cntDir);
				continue;
			}
	
			if((fp = fopen(dirp->d_name,"r")) == NULL){
				fprintf(stderr,"fopen error for %s\n",dirp->d_name);
				exit(1);
			}
			fseek(fp, 0, SEEK_END);
			fsize += ftell(fp);
	
			fclose(fp);
		}
		closedir(dp);
	
		return fsize;
	}
	else{ // d 옵션일 때
		strncpy(prePath,dir,PATH_MAX);
		strncpy(relativeDir,dir,PATH_MAX);
			
		p = strtok(relativeDir,"/");
		while((p = strtok(NULL,"/"))!=NULL){
			name = p;		
			name2 = p;
		}
		

	
		if((dp = opendir(dir)) == NULL){
			fprintf(stderr,"open dir error for %s\n",dir);
			exit(1);
		}
		cntDir--;
	
		while((dirp = readdir(dp)) != NULL){
			if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
				continue;
			}
	
			chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
	
			stat(dirp->d_name, &statbuf);
	
			if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면

				realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
				fsize += check_dir_size(newPath,cntDir);


				continue;
			}
	
			if((fp = fopen(dirp->d_name,"r")) == NULL){
				fprintf(stderr,"fopen error for %s\n",dirp->d_name);
				exit(1);
			}
			fseek(fp, 0, SEEK_END);
			fsize2 = ftell(fp);
			fsize += fsize2;
			if(cntDir>0){
				realpath(dirp->d_name, savePath);
				p2 = strstr(savePath,searchDir);
				name = p2;
				strcpy(mfile[size].fname,name);
				mfile[size].flen = fsize2;
				size++;
			}
	
			fclose(fp);
		}
		closedir(dp);
		
		if(cntDir==0){
			p2 = strstr(prePath,searchDir);
			name = p2;
			strcpy(mfile[size].fname,name);
			mfile[size].flen = fsize;
			size++;
		}
		return fsize;
	}
}
void command_recover(char *filename, char *option)//recover 명령어를 진행하는 함수
{
	DIR *dp;
	struct dirent *dirp;
	int isExist = false;
	int isTrue = false;
	int isTrue2 = false;
	int isTrue3 = false;
	int isTrue4 = false;
	int origin_t = false;
	int copy_t = false;
	char inputPath[PATH_MAX];
	FILE *fp;
	int fd;
	char *p, *p2 ,*copy;
	char rBuf[FILE_MAX], tBuf[FILE_MAX];
	int length;
	char *collection[FILE_MAX];
	char cTime[FILE_MAX][FILE_MAX];
	int collection_cnt = 0;
	int i,j;
	char *fullname[FILE_MAX];
	char copyname[BUFLEN][FILE_MAX];
	char name[BUFLEN], ename[BUFLEN];
	int namesize, cnt = 0;
	char cpy[BUFLEN], cpy2[BUFLEN];
	int arrCnt=0;
	int saveCnt;
	char lastName[BUFLEN];
	char plus_lastName[BUFLEN];
	char newPath[FILE_MAX];
	int sCnt, originCnt;

	chdir(programDir);
	if((dp = opendir(programDir))==NULL){
		fprintf(stderr,"open dir error for %s\n",programDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		if(!strcmp(dirp->d_name, "trash")){
			realpath(dirp->d_name, inputPath); // trash 디렉토리 절대경로 생성
			
			isExist = true;
			break;
		}
	}
	closedir(dp);
	if(!isExist){
		fprintf(stderr,"복구할 파일이 존재하지 않습니다.\n");
		return;
	}
	isExist = false;
	chdir(inputPath);
	sprintf(iDir,"%s/%s",inputPath,"info");
	sprintf(fDir,"%s/%s",inputPath,"files");
	
	chdir(iDir); //info 디렉토리로 작업 환경 변경

	
	if((dp = opendir(iDir))==NULL){
		fprintf(stderr,"open dir error for %s\n",iDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		if(!strcmp(dirp->d_name, filename)){
			isExist = true;
		}
		collection[collection_cnt++] = dirp->d_name;
	}
	closedir(dp);

	
	if(!isExist){
		fprintf(stderr,"trash 디렉토리에는 %s 파일이 존재하지 않습니다. 다시 입력해주세요\n",filename);
		return;
	}

	if(lOption){
		printf("-l 옵션 기능입니다.\n");
		for(i=0 ; i<collection_cnt ; i++){
			if((fd = open(collection[i],O_RDONLY))<0){
				fprintf(stderr,"open error for %s\n",collection[i]);
				exit(1);
			}
			length = read(fd,tBuf,FILE_MAX);

			p2 = strtok(tBuf,"\n");
			p2 = strtok(NULL,"\n");//절대경로
			p2 = strtok(NULL,"\n");//삭제시간
			p2 += 4; // 삭제 시간만을 출력하기 위한 작업
			copy = p2;
			strcpy(cTime[i],copy);
			close(fd);
		}

		sort_time(collection,cTime,collection_cnt);
	}
	////////////////////////////////////////////////////////////////
	strcpy(cpy,filename);
	if(strstr(cpy,".")!=NULL){
		p = strtok(cpy,".");
		strcpy(name,p);
		if(strstr(filename,"_")!=NULL){
			origin_t = true;
		}
	}
	else{
		strcpy(name,filename);
		isTrue4 = true;
	}
	namesize = strlen(name);

	if(strstr(filename,".")!=NULL){
		strcpy(ename,strstr(filename,"."));	
	}

	if((dp = opendir(iDir))==NULL){
		fprintf(stderr,"open dir error for %s\n",iDir);
		exit(1);
	}
	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		if(!isTrue4){
			if(!strncmp(dirp->d_name,name,namesize)){
				if(strstr(dirp->d_name,"_")!=NULL){
					if(!origin_t){
						continue;
					}
				}
				if(strstr(dirp->d_name,"-")==NULL){
					saveCnt = cnt+1;
				}
				fullname[cnt++] = dirp->d_name;
			}
		}
		else{
			if(!strcmp(dirp->d_name,name)){
				if(strstr(dirp->d_name,"-")==NULL){
					saveCnt = cnt+1;
				}
				fullname[cnt++] = dirp->d_name;
			}
		}

	}
	closedir(dp);


	sCnt = cnt;
	if(cnt!=1 && cnt!=arrCnt){ // 중복되는 파일이 있다면
		printf("파일이 중복됩니다. 선택해주세요.\n");
	}
	while(cnt!=1 && cnt!=arrCnt){ // 중복되는 파일이 있다면
		arrCnt++;
		printf("%d. %-15s",arrCnt,filename);
		
		if((fd = open(fullname[arrCnt-1],O_RDONLY))<0){
			fprintf(stderr,"open error for %s\n",filename);
			exit(1);
		}
		
		length = read(fd,rBuf,PATH_MAX);
		p = strtok(rBuf,"\n");
		p = strtok(NULL,"\n");
		p = strtok(NULL,"\n"); // 삭제 시간
		printf("%s\t",p);
		p = strtok(NULL,"\n"); // 수정 시간
		printf("%s\n",p);
		close(fd);
		isTrue3 = true;
	}
	if(isTrue3){
		printf("Choose : ");
		scanf("%d",&cnt);
		if(cnt == saveCnt){
			isTrue2 = true;
		}
		getchar();
	}
	if(isTrue2){
		originCnt = saveCnt;
		if(saveCnt < sCnt){ 
			saveCnt++;
		}
		else{
			saveCnt--;	
		}

	}

	strcpy(filename,fullname[cnt-1]);


	if((fd = open(filename, O_RDONLY))<0){
		fprintf(stderr,"open error for %s\n",filename);
		exit(1);
	}
	length = read(fd,rBuf,PATH_MAX);
	p = strtok(rBuf,"\n");
	p = strtok(NULL,"\n"); // p => 삭제했던 디렉토리 정보

	cnt = 0;
	strcpy(cpy2,p);
	p2 = strtok(cpy2,"/");
	while((p2 = strtok(NULL,"/"))!=NULL){
		memset(lastName,'\0',sizeof(lastName));
		strcpy(lastName,p2);
	}
	strcpy(plus_lastName,lastName);
	for(int i=0 ; i<sCnt ; i++){
		strcpy(copyname[i],fullname[i]);
	}


	while(1){
		isTrue = false;
		if((dp = opendir(pathDir))==NULL){
			fprintf(stderr,"open dir error for %s\n",pathDir);
			exit(1);
		}
		while((dirp = readdir(dp))!=NULL){
			if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
				continue;
			}
			if(!strcmp(dirp->d_name,plus_lastName)){ //복구하려는 파일에 같은 이름의 파일의 이름이 존재한다면
				memset(plus_lastName,'\0',sizeof(plus_lastName));
				isTrue = true;
				cnt++;	
				sprintf(plus_lastName,"%d_%s",cnt,lastName);
				break;
			}
			
	
		}
		closedir(dp);
		if(cnt==0 || !isTrue){
			break;
		}
	}


	unlink(filename); // info 디렉토리 정보 삭제
	chdir(fDir); // files 디렉토리로 작업 환경 변경
	if(cnt!=0){
		sprintf(newPath,"%s/%s",pathDir,plus_lastName);
		rename(filename,newPath); //복구
	}
	else{
		rename(filename,p); //복구
	}
	chdir(iDir);
	if(isTrue2){
		rename(copyname[saveCnt-1],copyname[originCnt-1]);
	}
	chdir(fDir);
	if(isTrue2){
		rename(copyname[saveCnt-1],copyname[originCnt-1]);
	}

	close(fd);


	

	chdir(pathDir);//checkFile 디렉토리로 작업환경 변경

	return;
}
void sort_time(char *c[FILE_MAX], char t[FILE_MAX][FILE_MAX], int size)//삭제된 시간이 오래된 순으로 정렬해 주는 함수
{
	int i,j;
	char *cTmp;
	char tTmp[FILE_MAX];
	int cnt=0;
	char name[BUFLEN];
	char ename[BUFLEN];
	char copy[BUFLEN];
	char copy2[BUFLEN];
	char *p;
	
	for(i=0 ; i<size-1 ; i++){
		for(j=0 ; j<size-1-i ; j++){
			if(strcmp(t[j],t[j+1]) > 0){
				cTmp = c[j];
				strncpy(tTmp, t[j], FILE_MAX);

				c[j] = c[j+1];
				strncpy(t[j], t[j+1], FILE_MAX);

				c[j+1] = cTmp;
				strncpy(t[j+1], tTmp, FILE_MAX);
			}
		}
	}
	for(i=0 ; i<size ; i++){
		if(strstr(c[i],"-")!=NULL){
			strcpy(copy,c[i]);
			strcpy(copy2,c[i]);
			p = strtok(copy,"-");	
			strcpy(name,p);

			p = strstr(copy2,".");
			strcpy(ename,p);

			printf("%d. %s%-15s",i+1,name,ename);
			printf("%s\n",t[i]);
			continue;
		}
		printf("%d. %-15s%s\n",i+1,c[i],t[i]);
	}
}
void command_tree(char *filename, int depth)//tree명령어를 진행하는 함수
{
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	FILE *fp;
	char prePath[PATH_MAX];
	char newPath[PATH_MAX];
	char name[PATH_MAX];
	char *print_name;
	char *p;
	int i;
	int cnt = depth;
	int preCnt;

	strcpy(name,filename);
	p = strtok(name,"/");
	while((p = strtok(NULL,"/"))!=NULL){
		print_name = p;	
	}
	printf("%s\n",print_name);

	chdir(filename);

	strncpy(prePath,filename,PATH_MAX);

	if((dp = opendir(filename)) == NULL){
		fprintf(stderr,"open dir error for %s\n",filename);
		exit(1);
	}
	
	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		chdir(prePath);// 이전 디렉토리를 유지하기 위한 작업
		preCnt = cnt;
		for(i=0 ; i<preCnt ; i++){
			printf("\t");
		}
		printf("|--");

		stat(dirp->d_name, &statbuf);
		// 일반 파일이라면
		if(!S_ISDIR(statbuf.st_mode)){
			printf("%s\n",dirp->d_name);
		}
	
		if(S_ISDIR(statbuf.st_mode)){ // 만약 디렉토리라면
			realpath(dirp->d_name,newPath); // 상대경로를 절대경로로 변경
			cnt++;
			command_tree(newPath, cnt);
			cnt = preCnt;
			continue;
		}
	
	
	}
	closedir(dp);
	
	return;

}

void do_delete(char *filename){//files, info 디렉토리에 넣기
	char newFile[PATH_MAX];
	char abPath[PATH_MAX];
	struct stat statbuf;
	time_t t, dT;
	struct tm mTm, dTm;
	FILE* fp;
	int check_size;
	char iPath[PATH_MAX];
	char buf[PATH_MAX];
	int size = 2001;
	DIR *dp;
	struct dirent *dirp;
	int fd;
	int cnt = 0;
	char *p, *p2;
	int isTrue;
	int number=1;
	char cat[2];
	char name[BUFLEN];
	char ename[BUFLEN];
	char copy[BUFLEN];
	char temp[BUFLEN];
	char *point;
	char prevname[BUFLEN];
	char cpy[BUFLEN];
	int real_delete = false;
	char input_d;

	cat[1] = '\0';
	strcpy(temp,filename);

	if(iOption){
		if(!strcmp(aPath,"")){//상대경로
			unlink(filename);	
		}
		else{//절대경로
			chdir(aPath);
			unlink(filename);	
		}
	}
	else{
		if(rOption && just_delete){
			printf("Delete [y/n]? ");
			scanf("%c",&input_d);
			getchar();
			if(input_d == 'n'){
				return;
			}
		}
		if(strstr(aPath,"/")==NULL){
			realpath(filename,abPath); // 상대경로 -> 절대경로
		}
		else{ //절대경로로 입력 받았다면
			strcpy(cpy,aPath);
			strcat(cpy,"/");
			strcat(cpy,filename);//hs
			strcpy(abPath,cpy);
		}

		chdir(iDir);

		while(1){//삭제 시 같은 파일이 존재하면 작업 진행
			isTrue = is_samefile(filename);

			if(isTrue){

				strcpy(copy,filename);

				if(strstr(copy,".")!=NULL){
					strcpy(prevname,filename);
					p2 = strtok(copy,".");
					strcpy(name,p2);
				}
				else{
					strcpy(name,temp);
				}
				if(number == 1){
					if(strstr(prevname,".")!=NULL){
						strcpy(ename,strstr(prevname,"."));
					}
				}
	
				cat[0] = number + 48;
				if((point = strstr(name,"-"))==NULL){
					strcat(name,"-");
					strcat(name,cat);
				}
				else{
					point++;
					point[0] = cat[0];
				}
				if(strstr(prevname,".")!=NULL){
					filename = strcat(name,ename);
				}
				else{
					filename = name;
				}
				number++;
			}
			else{
				break;
			}

		}
		if((fp = fopen(filename,"w"))==NULL){
			fprintf(stderr,"fopen error for %s\n",filename);
			exit(1);
		}
		fputs("[Trash info]\n",fp);
		strncat(abPath,"\n",1);
		fputs(abPath,fp);

		if(!strcmp(aPath,"")){
			chdir(pathDir);//check file의 삭제 및 수정 시간을 알아야함
		}
		else{//절대 경로일 경우
			chdir(aPath);
		}
		if(stat(temp,&statbuf)<0){
			fprintf(stderr,"stat error for %s\n",temp);
			exit(1);
		}
		t = statbuf.st_mtime;
		mTm = *localtime(&t); //수정시간
	
		dT = time(NULL);
		dTm = *localtime(&dT);//삭제시간
	
	
		fprintf(fp,"D : %04d-%02d-%02d %02d:%02d:%02d\n",dTm.tm_year+1900,dTm.tm_mon+1,dTm.tm_mday,dTm.tm_hour,dTm.tm_min,dTm.tm_sec);// 삭제시간 넣기
	
		fprintf(fp,"M : %04d-%02d-%02d %02d:%02d:%02d\n",mTm.tm_year+1900,mTm.tm_mon+1,mTm.tm_mday,mTm.tm_hour,mTm.tm_min,mTm.tm_sec);// 수정시간 넣기
		
		fclose(fp);//파일 닫기
		
		rename(temp,filename);
		sprintf(newFile,"%s/%s/%s",chName,"files",filename);
		rename(filename,newFile);
	}

	while(size>2000){//info파일에 파일들 크기가 2KB가 넘는다면 오래된 파일부터 삭제해주는 기능을 수행함
		cnt = 0;
		size = check_dir_size(copy_iDir,0);
		chdir(iDir);
		if(size >2000){//hs, 2kb가 넘을 때 오래된 파일부터 삭제
			if((dp = opendir(iDir))==NULL){
				fprintf(stderr,"open dir error for %s\n",programDir);
				exit(1);
			}
		
			while((dirp = readdir(dp)) != NULL){
				if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
					continue;
				}
				if((fd = open(dirp->d_name, O_RDONLY))<0){
					fprintf(stderr,"open error for %s\n",dirp->d_name);
					exit(1);
				}
				read(fd,buf,PATH_MAX);
				p = strtok(buf,"\n");
				p = strtok(NULL,"\n");
				p = strtok(NULL,"\n"); // 삭제 시간 
				p += 4; // real 삭제 시간
				strcpy(checktime[cnt].fname, dirp->d_name);
				strcpy(checktime[cnt].ftime, p);
				cnt++;
	
			}
			closedir(dp);	
			old_time_delete(cnt);
		}
		size = check_dir_size(copy_iDir,0);
	}
	chdir(pathDir);

	return;
}
int is_samefile(char *filename)//삭제 시 같은 파일일 때 수행해주는 함수
{
	DIR *dp;
	struct dirent *dirp;

	if((dp = opendir(iDir))==NULL){
		fprintf(stderr,"open dir error for %s\n",iDir);
		exit(1);
	}

	while((dirp = readdir(dp))!=NULL){
		if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
			continue;
		}
		if(!strcmp(dirp->d_name,filename)){
			return true;
		}
	}

	closedir(dp);
	return false;
}
void old_time_delete(int size)//삭제한지 오래된 파일을 삭제함
{
	int i,j;
	struct checkTime tmp;
	for(i=0 ; i<size-1 ; i++){
		for(j=0 ; j<size-1-i ; j++){
			if(strcmp(checktime[j].ftime,checktime[j+1].ftime)>0){
				strcpy(tmp.fname,checktime[j].fname);
				strcpy(tmp.ftime,checktime[j].ftime);

				strcpy(checktime[j].fname, checktime[j+1].fname);
				strcpy(checktime[j].ftime, checktime[j+1].ftime);

				strcpy(checktime[j+1].fname, tmp.fname);
				strcpy(checktime[j+1].ftime, tmp.ftime);
			}
		}
	}
	// 삭제 작업, 위 작업을 통해 0번방의 구조체가 가장 오래된 파일임을 구할수 있다.
	chdir(iDir);
	unlink(checktime[0].fname);
	chdir(fDir);
	unlink(checktime[0].fname);

	return;
}

char do_rOption(){ //지정한 시간에 삭제할지 결정
	char ans;
	printf("Delete [y/n]? ");
	scanf("%c",&ans);
	getchar();
	return ans;
}

void do_timeOption(char *filename, char *endTime){//삭제 시 endtime 값이 존재한다면 진행하는 함수
	time_t t, t2;
	struct tm dt, test;
	int year,mon,day,hour,min;
	char m=48;
	char isY;
	pid_t pid;

	year = (endTime[0]-m)*1000 + (endTime[1]-m)*100 + (endTime[2]-m)*10 + endTime[3]-m;
	mon = (endTime[5]-m)*10 + endTime[6]-m;
	day = (endTime[8]-m)*10 + endTime[9]-m;
	hour = (endTime[10]-m)*10 + endTime[11]-m;
	min = (endTime[13]-m)*10 + endTime[14]-m;

	t2 = time(NULL);
	test = *localtime(&t2);
	if(year<test.tm_year+1900){
		fprintf(stderr,"error : 지울 시간을 잘못입력하셨습니다.\n");
		return;
	}
	else if(mon < test.tm_mon+1){
		fprintf(stderr,"error : 지울 시간을 잘못입력하셨습니다.\n");
		return;
	}
	else if(day < test.tm_mday){
		fprintf(stderr,"error : 지울 시간을 잘못입력하셨습니다.\n");
		return;
	}
	else if(hour < test.tm_hour){
		fprintf(stderr,"error : 지울 시간을 잘못입력하셨습니다.\n");
		return;
	}
	else if(min < test.tm_min){
		fprintf(stderr,"error : 지울 시간을 잘못입력하셨습니다.\n");
		return;
	}

	if(rOption){
		isY = do_rOption();
		if(isY=='y'){
			;
		}
		else if(isY == 'n'){
			return;
		}
		else{
			printf("y 또는 n을 입력해주세요.\n");
			return;
		}
	}
	
	if((pid = fork())<0){
		fprintf(stderr,"fork error\n");
	}
	if(pid == 0){ //자식프로세스
		while(1){
			t =	time(NULL); 
			dt = *localtime(&t);
			if((dt.tm_min == min) && (dt.tm_hour == hour)){
				if((dt.tm_mday == day) && (dt.tm_mon+1 == mon) && (dt.tm_year+1900 == year)){
					do_delete(filename);
					break;
				}
			}
			
		}
		exit(0);
	}
	else{ //부모 프로세스
		;
	}
	return;
}

void print_help()
{
	printf("delete [FILENAME] [END_TIME] [OPTION]\n");
	printf("option >> \n");
	printf("-i : 삭제 시 \'trash\' 디렉토리로 삭제 파일과 정보를 이동시키지 않고 파일 삭제\n");
	printf("-r : 지정한 시간에 삭제 시 삭제 여부 재확인\n\n");
	printf("size [FILENAME] [OPTION]\n");
	printf("option >> \n");
	printf("-d NUMBER : NUMBER 단계만큼의 하위 디렉토리까지 출력\n\n");
	printf("recover [FILENAME] [OPTION]\n");
	printf("option >> \n");
	printf("-l : \'trash\' 디렉토리 밑에 있는 파일과 삭제 시간들을 삭제 시간이 오래된 순으로 출력 후, 명령어 진행\n\n");
	printf("tree\n");
	printf(">> \"check\" 디렉토리의 구조를 tree 형태로 보여줌\n\n");
	printf("exit\n");
	printf(">> 프로그램 종료\n\n");
	printf("help\n");
	printf(">> 명령어 사용법 출력\n\n");
}
