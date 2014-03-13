#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "queue.h"

#define BUFLEN 4500
#define RED "\033[31m"
#define GREEN "\033[32m"
#define RESET_COLOR "\033[0m"
#define NUM_THREADS 15

pthread_t threads[NUM_THREADS];

// the server should listen on TCP port 8081

// returns the length of the current line and assigns a pointer at it's beginning
int getlineString(char line[], char theArray[], int flag){
  static int lastpos = 0;
  int i = 0;
  
  if( flag == 1 ){
    lastpos = 0;
    return 0;
  }
  
  if( *(theArray+lastpos) == '\0' )
    return -1;
  
  while( theArray[lastpos] != '\r' )
    line[i++] = theArray[lastpos++];
  
  line[i] = '\0';
  lastpos+=2;
  
  return i;
}// end of getlineString


// prints an array of chars on the screen
void printLine(char* line){
  
  while(*line != '\0'){
    printf("%c", *line);
    line++;
  }
  
  printf("\n");
}

// return the file name, otherwise NULL
void getRequest(char buffer[], char theFile[]){

  char line[BUFLEN];
  char type[16], val[BUFLEN];
  char* file_to_open;
  getlineString(line, buffer, 1);
  
  getlineString(line, buffer, 0);

  sscanf(line, "%s %[^\r\n]", type, val);
      
  if( strcmp(type,"GET") == 0 && strstr(val,"HTTP/1.1") != NULL ){
    file_to_open = strtok(val," ");
    ++file_to_open;
    
    strcpy(theFile, file_to_open);
    return;
  }

  theFile[0] = '\0';
}

void toLowerCase(char* line){
  char* cpy_line = line;
  
  while( *cpy_line != '\0' ){
    *cpy_line = tolower(*cpy_line);
    cpy_line++;
  }
}

// returns the host name, otherwise returns an empty array
void getCurHost(char buffer[], char cur_host[]){
  char line[BUFLEN];
  char type[16], val[BUFLEN];
  
  while( getlineString(line, buffer, 0) > 0 ){
       sscanf(line, "%s %[^\r\n]", type, val);
       toLowerCase(type);
       
       if( strcmp(type,"host:") == 0 ){
         //cpyCharArray(val, cur_host);
         strcpy(cur_host, val);
         return;
       }
  }// end of loop
  
  cur_host[0] = '\0';
}

// -----------------------	response pages	-----------------------

void returnError400(int conn){
  
  int fileSize;
  struct stat fileBuf;
  char bad_request[] = "HTTP/1.1 400 Bad Request\r\n";
  char conn_type[] = "Content-Type: text/html\r\n";
  char conn_len[32];
  char conn_close[] = "Connection: close\r\n\r\n";
  char page_line[BUFLEN];
  FILE *error400 = fopen("error400.html", "r");
  
  
  if( error400 != NULL ){
    fstat(fileno(error400), &fileBuf);
    fileSize = fileBuf.st_size;
    
    sprintf(conn_len, "Content-Length: %d\r\n", fileSize);
    
    write(conn, bad_request, strlen(bad_request));
    write(conn, conn_type, strlen(conn_type));
    write(conn, conn_len, strlen(conn_len));
    write(conn, conn_close, strlen(conn_close));
    
    while( fgets(page_line, BUFLEN, error400) != NULL ){
      write(conn, page_line, strlen(page_line));
      //printf("%s",page_line);
    }
    
    write(conn, "\r\n", strlen("\r\n"));
  }
  else
    printf("Can't find error400.html\n");
  
  fclose(error400);
}

void returnError404(int conn){
  
  int fileSize;
  struct stat fileBuf;
  char bad_request[] = "HTTP/1.1 404 Not Found\r\n";
  char conn_type[] = "Content-Type: text/html\r\n";
  char conn_len[32];
  char conn_close[] = "Connection: close\r\n\r\n";
  char page_line[BUFLEN];
  FILE *error404 = fopen("error404.html", "r");
  
  
  if( error404 != NULL ){
    fstat(fileno(error404), &fileBuf);
    fileSize = fileBuf.st_size;
    
    sprintf(conn_len, "Content-Length: %d\r\n", fileSize);
    
    write(conn, bad_request, strlen(bad_request));
    write(conn, conn_type, strlen(conn_type));
    write(conn, conn_len, strlen(conn_len));
    write(conn, conn_close, strlen(conn_close));

    
    while( fgets(page_line, BUFLEN, error404) != NULL ){
      write(conn, page_line, strlen(page_line));
      //printf("%s",page_line);
    }
  }
  else
    printf("Can't find error404.html\n");
  
  fclose(error404);
}

void returnError500(int conn){
  
  int fileSize;
  struct stat fileBuf;
  char bad_request[] = "HTTP/1.1 500 Internal Server Error\r\n";
  char conn_type[] = "Content-Type: text/html\r\n";
  char conn_len[32];
  char conn_close[] = "Connection: close\r\n\r\n";
  char page_line[BUFLEN];
  FILE *error500 = fopen("error500.html", "r");
  
  if(error500 != NULL){
    fstat(fileno(error500), &fileBuf);
    fileSize = fileBuf.st_size;
    
    sprintf(conn_len, "Content-Length: %d\r\n", fileSize);
    
    write(conn, bad_request, strlen(bad_request));
    write(conn, conn_type, strlen(conn_type));
    write(conn, conn_len, strlen(conn_len));
    write(conn, conn_close, strlen(conn_close));
    
    
    while( fgets(page_line, BUFLEN, error500) != NULL ){
      write(conn, page_line, strlen(page_line));
      //printf("%s",page_line);
    }
  }
  else
    printf("Can't find error500.html\n");
  
  fclose(error500);
}

// sends the requested page and returns 0 if successful, otherwise returns 1
int returnHTMLpage(int conn, char *fileName, char* host){
  
  long int fileSize;
  struct stat fileBuf;
  char conn_request[] = "HTTP/1.1 200 OK\r\n";
  char conn_type[32] = "Content-Type: ";
  char conn_len[32];
  char conn_referer[32] = "Referer: ";
  char page_line[BUFLEN];
  char *img;
  int fileType = 0;
  
  /*
   0 is for HTML and TXT files
   1 is for JPEG and JPG
   2 is for GIF
   */
  
  //printLine(fileName);
  
  FILE *req_file;
  if( strstr(fileName, ".ico") != NULL ){
      sprintf(conn_len, "Content-Length: %lu\r\n", 0);
      strcat(conn_type, "application/octet-stream\r\n\r\n");
      
      write(conn, conn_request, strlen(conn_request));
      write(conn, conn_len, strlen(conn_len));
      write(conn, conn_type, strlen(conn_type));
      //write(conn, conn_close, strlen(conn_close));
      return 0;
  }

  if( strstr(fileName, ".jpg") != NULL || strstr(fileName, ".jpeg") != NULL){
    fileType = 1;
    strcat(conn_type, "image/jpeg\r\n\r\n");
  }
  else if( strstr(fileName, ".gif") != NULL ){
    fileType = 2;
    strcat(conn_type, "image/gif\r\n\r\n");
  }
  
  if(  fileType == 1 || fileType == 2 ){

    req_file = fopen(fileName, "rb");
    
    if( req_file != NULL ){
      fstat(fileno(req_file), &fileBuf);
      fileSize = fileBuf.st_size;
  
      sprintf(conn_len, "Content-Length: %lu\r\n", fileSize);
      
      img = (char *)malloc(fileSize+1);
      if( !img  ){
        printf("Error when allocating memory for image\r\n");
        returnError500(conn);
        fclose(req_file);
        return 1;
      }
      
      fread(img, fileSize, sizeof(unsigned char), req_file);
      
      //printf("%s", conn_len);
      
      write(conn, conn_request, strlen(conn_request));
      write(conn, conn_len, strlen(conn_len));
      write(conn, conn_type, strlen(conn_type));

      write(conn, img, fileSize);
      
      free(img);
      fclose(req_file);
      
      return 0;
    }// end of 2 level IF
    
    return 1;
  }// end of 1 level IF
  else if( fileType == 0 ){
    strcat(conn_type, "text/html\r\n\r\n");
    req_file = fopen(fileName, "r");
    
    if( req_file != NULL ){
      fstat(fileno(req_file), &fileBuf);
      fileSize = fileBuf.st_size;
    
      sprintf(conn_len, "Content-Length: %lu\r\n", fileSize);
      
      write(conn, conn_request, strlen(conn_request));
      write(conn, conn_len, strlen(conn_len));
      write(conn, conn_type, strlen(conn_type));

      while( fgets(page_line, BUFLEN, req_file) != NULL )
        write(conn, page_line, strlen(page_line));
      
      fclose(req_file);
      return 0;
    }
  }
  
  return 1;
}
// ----------------------------------------------------------------------

// reads the data from the current connection
int readData(int connfd){
     ssize_t nbytes;
     int cpy_nbytes;
     int bufLen = 0;
     int bufSize = 4500;
     char *nextBufPos;
     char curMessage[64];
     char host_name[32];
     char cur_host[32];
     char host_dcs[64];
     char requested_file[32];
     char *buf = (char *)malloc( sizeof(char)*(bufSize + 1) );
     char *tempBuf = NULL;
     
     buf[0] = '\0';
     
     while ( (nbytes = read(connfd, curMessage, sizeof(curMessage))) > 0 ){
      cpy_nbytes = nbytes;
      curMessage[cpy_nbytes] = '\0';
      
        if( bufLen + nbytes >= bufSize )
          tempBuf = (char *)realloc(buf, sizeof(char)*bufSize*2);
        
        if( tempBuf != NULL ){
          bufSize *= 2;
          buf = tempBuf;
	tempBuf = NULL;
        }
       
       strcat(buf, curMessage);
       bufLen = strlen(buf);
       
        if( (nextBufPos = strstr(buf, "\r\n\r\n")) != NULL ){
          
	host_name[0] = '\0';
	cur_host[0] = '\0';
	host_dcs[0] = '\0';
	requested_file[0] = '\0';
        
	gethostname(host_name, 32);
	toLowerCase(host_name);
	 //printLine(host_name);
 
         getRequest(buf, requested_file);
         //printLine(requested_file);
 	  
         getCurHost(buf, cur_host);
         //printLine(cur_host);
	 //bo720-3-02.dcs.gla.ac.uk:8081
         
         strcpy(host_dcs, host_name);
         
         // ------------------- Applies for the University computers
         //strcat(host_dcs, ".dcs.gla.ac.uk:8081");
         
         // ------------------- On localhost append only the port
         strcat(host_dcs, ":8081");
         
         if( requested_file[0] != '\0'){
 	if( cur_host[0] != '\0' && (!strcmp(cur_host, host_name) || !strcmp(host_dcs, cur_host)) ){
 	        
 	  if( returnHTMLpage(connfd,requested_file, cur_host) != 0 )
 	    returnError404(connfd);
 	  }
 	  else
 	    returnError400(connfd);
         }
         else
 	returnError400(connfd);
        
         buf[0] = '\0';
         curMessage[0] = '\0';
         bufLen = 0;
       }
    }

    free(buf);
    close(connfd);
    return nbytes;
}

void *handleMessages(void *queue){
  int connfd;
  Queue *q = (Queue *)queue;
  
  pthread_mutex_lock(getMutex(q));
    while( isEmpty(q) == 1 )
      pthread_cond_wait(getCond(q), getMutex(q));
    
    connfd = *((int *)q_remove(q));
  pthread_mutex_unlock(getMutex(q));
  
  readData(connfd);
}

int main(){
  
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  int sockoptON = 1;
  
  if( fd < 0 ){
    printf(RED "An error occurred while creating socket...\n" RESET_COLOR);
    close(fd);
    return 1;
  }
  
  if (setsockopt(fd, 0, SO_REUSEADDR, &sockoptON, sizeof(int)) < 0)
  {
    printf(RED "Error occured while setting socket options...\n" RESET_COLOR);
    return 1;
  }
  
  struct sockaddr_in addr;
  
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8081);
  
  printf("---------------------------------------------------\n");
  
  printf(GREEN "Binding socket...\n" RESET_COLOR);
  
  if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1){
    printf(RED "An error occurred while binding...\n" RESET_COLOR);
    close(fd);
    return 1;
  }
  
  int backlog = 12;
  
  printf(GREEN "Listening for connections...\n" RESET_COLOR);
  
  if(listen(fd, backlog) == -1){
    printf(RED "An error occurred while listening...\n" RESET_COLOR);
    close(fd);
    return 1;
  }
  
  int connfd;
  struct sockaddr_in cliaddr;
  socklen_t cliaddr_len = sizeof(cliaddr);
  
  printf(GREEN "Accepting connection...\n" RESET_COLOR);
  
  printf(GREEN "Reading data...\n" RESET_COLOR);
  
  Queue *q = q_create();
  int i;
  
  for( i = 0; i < NUM_THREADS; i++){
    pthread_create(&threads[i], NULL, handleMessages, (void *)q);
  }
  
  char example_host[32];
  
  gethostname(example_host, 32);
  
  printf("Try accessing the following page: %s:8081/pics.html\n", example_host);
  
  while(1){

    connfd = accept(fd, (struct sockaddr *) &cliaddr, &cliaddr_len);
     
    if(connfd == -1){
      printf(RED "An error occurred while accepting a connection...\n" RESET_COLOR);
      close(fd);
      return 1;
    }
    
    pthread_mutex_lock(getMutex(q));
      i = q_add(q, (void *)&connfd);
      
      if( i == 0 ){
        returnError500(connfd);
        i = *((int *)q_remove(q));
      }
      else
        pthread_cond_signal(getCond(q));
    pthread_mutex_unlock(getMutex(q));
  }
  
  // closing the per-connection fd because I have finished with that connection
   printf("Closing the connection...\n");
  
  //close(connfd);
  q_destroy(q);
  close(fd);
  
  printf("---------------------------------------------------\n");
  
  return 0;
}// end of main