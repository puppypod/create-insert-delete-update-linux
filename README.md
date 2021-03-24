# 숭실대학교 _ 리눅스 시스템 프로그래밍 개인프로젝트 P2 _ CRUD 구현

## 설계 및 구현

-  ssu_mntr 프로그램 기본 사항

  - ssu_mntr 프로그램은 지정한 특정 디렉토리의 파일의 변경 상태 모니터링
    
    -  지정한 디렉토리는 제출할 소스코드 디렉토리 밑에 서브디렉토리로 생성
    -  모니터링 프로그램은 별도 생성한 디몬 프로세스가 담당
    -  파일이 생성, 삭제, 수정될 경우, 학번 서브디렉토리 밑에 있는 “log.txt”파일에 변경 사항을 추가

    ![1](https://user-images.githubusercontent.com/47939832/112284811-fa91ac80-8ccc-11eb-9790-23d873a1e832.png)
    
    -  log.txt 파일 구조
      - [생성 or 삭제 or 수정 시간] [수행내용_파일이름] 형태로 작성
      - 새로운 파일 변경 사항은 “log.txt”파일 밑에 추가
      - 파일 이름에는 한글 지원하지 않아도 됨
    - 모니터링이 종료될 경우, 종료 메시지(학생이 원하는 종료 메시지 마음대로)를 출력한 후 종료
- 설계 및 구현
  
  - 가) ssu_mntr 실행 후 프롬프트 출력

    - 프롬프트 모양 : “학번>” 문자 출력. ex) 20201234>
    - 프롬프트에서는 delete, size, recover, tree, exit, help 명령어만 수행
    - 이외의 명령어 수행 시 자동으로 help를 실행시킨 것과 동일하게 표준 출력 후 프롬프트 출력
    - 엔터만 입력 시 프롬프트 재출력
  
  - 나) DELETE [FILENAME] [END_TIME] [OPTION]
  
    - 지정한 삭제 시간에 자동으로 파일을 삭제해주는 명령어
    - 삭제한 파일은 제출할 소스코드 디렉토리 밑에 있는 'trash' 디렉토리로 이동
    - “trash” 디렉토리가 없을 경우 생성해야 함.
    - trash 디렉토리
      
      - trash 디렉토리 밑에 두 개의 서브디렉토리, files, info 디렉토리를 생성
        
        - “files” 디렉토리 : DELETE 명령어로 지운 파일 자체 저장 (rename으로 경로 변경)
        - “info” 디렉토리 : DELETE 명령어로 지운 파일의 정보(절대경로, 삭제 시간, 최종 수정시간(mtime))를 저장. 단 저장될 파일 이름은 절대경로를 제외하고 최종 파일 이름만 포함됨. 
      
      - info 디렉토리의 크기가 정해진 크기(2KB)를 초과할 경우 오래된 파일부터 files 디렉토리의 원본 파일과 함께 info 디렉토리 파일 정보까지 삭제
        ![2](https://user-images.githubusercontent.com/47939832/112284814-fbc2d980-8ccc-11eb-842e-498f5e819a00.png)
        
    - FILENAME
      
      - 파일의 경로(절대경로와 상대경로 모두 가능해야함)
      - FILENAME 입력이 없거나 존재하지 않을 경우, 에러처리 후 프롬프트로 출력
      - 삭제할 파일은 지정한 특정 디렉토리 안에 있는 파일들로 한정함
    
    - END_TIME
      
      - 삭제할 파일의 삭제 예정 시간 ex) 20201234> DELETE FILELNAME 2020-05-05 10:00 (5월 5일 10시 삭제)
      - 단, 제출 실행 예제는 테스트가 쉽게 가능하면 3분 이내 시간을 사용하기 바람
      - END_TIME을 주어주지 않을 경우 바로 삭제함
    
    - OPTION
      
      - -i : 삭제 시 'trash' 디렉토리로 삭제 파일과 정보를 이동시키지 않고 파일 삭제
      - -r : 지정한 시간에 삭제 시 삭제 여부 재확인
      
      ![3](https://user-images.githubusercontent.com/47939832/112284815-fc5b7000-8ccc-11eb-94f0-04b942f69b33.png)
  
  - 다) SIZE [FILENAME] [OPTION]
  
    - 파일경로(상대경로), 파일 크기 출력하는 명령어
    - 기본 크기는 byte단위임
    - 출력하는 파일 및 디렉토리는 문자열 오름차순으로 출력해야함
    - FILENAME
      
      - 디렉토리와 파일이 모두 가능하며 FILENAME으로 입력 시 크기가 모두 표시되어야함
      - 파일경로는 상대경로로 출력해야함
      
      ![4](https://user-images.githubusercontent.com/47939832/112284818-fc5b7000-8ccc-11eb-9044-82f3a33d12df.png)
      
    - OPTION
      
      - -d NUMBER : NUMBER 단계만큼의 하위 디렉토리까지 출력
        
        - NUMBER의 수보다 하위 디렉토리의 수가 적을 경우, 최대 하위 디렉토리까지 출력
        
        ![5](https://user-images.githubusercontent.com/47939832/112284820-fcf40680-8ccc-11eb-92ab-284b59fdb023.png)
  
  - 라) RECOVER [FILENAME] [OPTION]
      
    - “trash”디렉토리 안에 있는 파일을 원래 경로로 복구하는 명령어
    - 동일한 이름의 파일이 “trash”디렉토리에 여러 개 있을 경우 이를 표준 출력으로 파일 이름, 삭제 시간, 수정 시간을 보여줌
    - 복구 시 이름이 중복된다면 파일의 처음에 “숫자_”를 추가
      
      ![6](https://user-images.githubusercontent.com/47939832/112284823-fcf40680-8ccc-11eb-9d99-08445032364e.png)
      
    - FILENAME
      
      - 해당 파일이 없거나 파일의 복구 경로가 없다면 에러처리 후 프롬프트 전환
        
        ![7](https://user-images.githubusercontent.com/47939832/112284824-fd8c9d00-8ccc-11eb-8226-07b4237b2fbd.png)
        
    - OPTION
      
      - -l : ‘trash’디렉토리 밑에 있는 파일과 삭제 시간들을 삭제 시간이 오래된 순으로 출력 후, 명령어 진행
        
        ![8](https://user-images.githubusercontent.com/47939832/112284826-fd8c9d00-8ccc-11eb-8278-a8d80047fd49.png)
  
  - 마) TREE
  
    - “check”디렉토리의 구조를 tree 형태로 보여주는 명령어
    - 꼭 이 형태를 따르지 않아도 되나 지정한 디렉토리의 구조를 그림으로 보여주어야 함 
    - 아래 예시에서는 ~lsp/test/20162495/check 디렉토리가 파일의 변경 상태를 모니터링을 하기 위해 지정한 특정 디렉토리임
    
      ![9](https://user-images.githubusercontent.com/47939832/112284828-ff566080-8ccc-11eb-9cb5-506af5ee8ec3.png)
  
  - 바) EXIT
  
    - 프로그램 종료시키는 명령어
    - EXIT 명령어 실행 시 명령어를 실행시키는 프로세스와 지정한 디렉토리를 모니터링하는 프로세스 모두 종료되어야함.
  
  - 사) HELP

    - 명령어 사용법을 출력하는 명령어

## 소스코드 분석

- 설계
  
  - main.c
    
    ![10](https://user-images.githubusercontent.com/47939832/112298491-6b8b9100-8cda-11eb-96d3-a436f28554b0.png)
    
  - ssu_mntr.c
    
    ▶ ssu_mntr()
      
      ![11](https://user-images.githubusercontent.com/47939832/112298496-6cbcbe00-8cda-11eb-894f-469697a177c2.png)
      
    ▶ select_command()
    
      ![12](https://user-images.githubusercontent.com/47939832/112298498-6d555480-8cda-11eb-801f-7a62b096836f.png)
      
    ▶ play_command()
    
      ![13](https://user-images.githubusercontent.com/47939832/112298500-6d555480-8cda-11eb-8528-a87971627646.png)
      
    ▶ command_delete()
    
      ![14](https://user-images.githubusercontent.com/47939832/112298503-6dedeb00-8cda-11eb-9157-ef4f73d6828b.png)
      
    ▶ do_delete()
    
      ![15](https://user-images.githubusercontent.com/47939832/112298504-6dedeb00-8cda-11eb-9e52-0ca5f2566661.png)
      
    ▶ do_timeOption()
    
      ![16](https://user-images.githubusercontent.com/47939832/112298506-6e868180-8cda-11eb-989c-ad9a41a26279.png)
      
    ▶ command_size()
    
      ![17](https://user-images.githubusercontent.com/47939832/112298510-6e868180-8cda-11eb-803a-e733147ec0ce.png)
      
    ▶ check_dir_size()
    
      ![18](https://user-images.githubusercontent.com/47939832/112298512-6f1f1800-8cda-11eb-8591-2b6e55e75406.png)
      
    ▶ command_recover()
    
      ![19](https://user-images.githubusercontent.com/47939832/112298513-6f1f1800-8cda-11eb-98bd-f852f15973a7.png)
      
    ▶ command_tree()
    
      ![20](https://user-images.githubusercontent.com/47939832/112298514-6fb7ae80-8cda-11eb-8b92-4c4b2c1d952f.png)
      
    ▶ ssu_daemon_init()
    
      ![21](https://user-images.githubusercontent.com/47939832/112298516-70504500-8cda-11eb-8e1f-6e38a094364a.png)
      
      ![22](https://user-images.githubusercontent.com/47939832/112298518-70504500-8cda-11eb-8ea7-98dd8f3babc6.png)
    
- 구현
  
  - int ssu_daemon_init(void);//디몬 프로세스 작업을 진행하는 함수
    
    -> 디몬프로세스를 별도로 돌려 log.txt 파일에 로그를 찍어줌
    
  - int daemon_file_check(char *dir);//check파일의 파일 개수를 알려주는 함수
    
    -> 이 함수를 통해 파일이 생성되었는지, 삭제되었는지, 수정되었는지 flag를 알 수 있음
    
  - void what_creat_file(char *dir, char file_collection[BUFLEN][PATH_MAX], int cnt);//디렉토리에 존재하는 파일들을 2차원 배열에 초기화시킴
    
    -> check파일 디렉토리에 어떠한 파일들이 존재하는지 배열에 초기화 시킴
    
  - char *what_creat_file2(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX]);//어떠한 파일을 생성했는지 알려주는 함수
    
    -> 이전 디렉토리 배열과 현재 디렉토리 배열의 비교를 통해 어떤 파일이 생성되었는지 반환해주는 함수임
  
  - char *what_delete_file(char prev[BUFLEN][PATH_MAX], char present[BUFLEN][PATH_MAX]);//어떠한 파일을 삭제했는지 알려주는 함수
    
    -> 이전 디렉토리 배열과 현재 디렉토리 배열의 비교를 통해 어떤 파일이 삭제되었는지 반환해주는 함수임
    
  - void info_file_creat(char *dir,FILE *fp);//어떠한 파일을 생성했는지 log.txt에 찍어주는 함수
    
    -> 생성된 파일을 log.txt에 찍어줌.
    
  - void info_file_delete(char file_collection[BUFLEN][PATH_MAX], FILE *fp);//어떠한 파일을 삭제했는지 log.txt에 찍어주는 함수
    
    -> 어떠한 파일을 삭제했는지 log.txt에 찍어주는 함수
    
  - void modify_file_check(char *dir, int cnt, FILE *fp);//어떠한 파일을 수정했는지 log.txt에 찍어주는 함수
    
    -> 수정된 시간을 전역 배열에 주기적으로 갱신시켜주며 어떤 파일이 수정되었다면 수정된 시간을 업로드하며 log.txt파일에 수정된 정보를 찍어줌
    
  - void ssu_mntr();//디몬프로세스와 명령어 기능 수행하는 ssu_mntr프로그램
    
    -> delete, size, recover. exit, help, tree 등 여러 명령어들을 입력받아 명세서가 요구한대로 수행하는 함수
    
  - int select_command(char *filename, char *endTime, char *option);//명령어를 선택하는 함수
    
    -> 입력한 정보를 token으로 분할하여 delete인지, size인지, recover인지, tree인지, help인지, exit인지 분별해줌
    
  - void play_command(char *filename, char *endTime, char *option, int select);//본격적인 명령을 수행하는 함수
    
    -> 본격적인 명령을 수행하는 함수
    
  - void print_help();//help 명령어
    
    -> 프로그램을 사용할 방법을 출력함
    
  - void command_delete(char *filename, char *endTime, char *option);//delete 명령어 수행
    
    -> delete 명령어를 수행하며 잘못된 입력 시, 에러 메시지를 출력해줌
    
  - void command_size(char *filename, char *option);//size 명령어 수행
    
    -> size 명령어를 수행하며 잘못된 입력 시, 에러 메시지를 출력해줌
    
  - void command_recover(char *filename, char *option);//recover 명령어 수행
    
    -> recover 명령어를 수행하며 잘못된 입력 시, 에러 메시지를 출력해줌
    
  - void command_tree(char *filename,int depth);//tree 명령어 수행
    
    -> tree 명령어를 수행하며 잘못된 입력 시, 에러 메시지를 출력해줌
    
  - void do_delete(char *filename);//파일을 삭제하는 함수
    
    -> 파일을 삭제해주는 기능을 하며 –i옵션 및 –r옵션 기능을 수행해주기도 함
    
  - int is_samefile(char *filename);//삭제 시 trash디렉토리에 같은 이름의 파일이 존재할 때 수행하는 함수
    
    -> info파일이 2KB가 넘는다면 명세서의 요구사항대로 파일을 삭제 해주는 함수
    
  - void old_time_delete(int size); //2kb 넘어간다면 오래된 파일 삭제
    
    -> 2kb 넘어간다면 오래된 파일부터 삭제해줌
    
  - void do_timeOption(char *filename, char *endTime);//endtime이 존재한다면 수행하는 함수
    
    -> endtime이 존재한다면 수행하며, fork()를 통해 부모 프로세스는 이 기능을 수행하지 않고 프롬프트를 입력받을 준비를 하고, 자식 프로세스는 endtime에 삭제함
    
  - char do_rOption(); // 정말로 삭제할지 되묻는 함수 
    
    -> r옵션 수행
    
  - int check_dir_size(char *dir, int num);//디렉토리의 size를 출력해주는 함수
    
    -> check 디렉토리 size를 출력해주는 함수
    
  - void sort_file();//size 명령어 수행 시, 오름차순으로 정렬하여 출력
    
    -> size 명령어 수행시, 오름차순으로 정렬하여 출력
    
  - void sort_time(char *c[FILE_MAX], char t[FILE_MAX][FILE_MAX], int size);//시간 순으로 정렬하는 함수
    
    -> 시간 순으로 정렬하는 함수
    
