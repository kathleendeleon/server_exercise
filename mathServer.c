//Compile with:
//$ gcc mathServer.c -o mathServer -lpthread

//---Header file inclusion---//

#include "mathClientServer.h"
#include <errno.h> // For perror()
#include <pthread.h> // For pthread_create()


//---Definition of constants:---//

#define STD_OKAY_MSG "Okay"

#define STD_ERROR_MSG "Error doing operation"

#define STD_BYE_MSG "Good bye!"

#define THIS_PROGRAM_NAME "mathServer"

#define FILENAME_EXTENSION ".bc"

#define OUTPUT_FILENAME "out.txt"

#define ERROR_FILENAME "err.txt"

#define CALC_PROGNAME "/usr/bin/bc"

extern void* handleClient(void* vPtr);
extern void* dirCommand(int fd);
extern void* readCommand(int clientFd, int fileNum);
extern void* writeCommand(int clientFd, int fileNum, void* text);
extern void*   deleteCommand(int clientFd, int fileNum);
extern void*  calcCommand(int clientFd, int fileNum);

const int ERROR_FD= -1;


//---Definition of functions:---//

//  PURPOSE:  To run the server by 'accept()'-ing client requests from
//'listenFd' and doing them.
void doServer(int listenFd) {
  //  I.  Application validiity check:

  //  II.  Server clients:
  pthread_t threadId;
  pthread_attr_t threadAttr;
  int threadCount= 0;
  int* iPtr;

  // YOUR CODE HERE

  listen(listenFd,5);  

  pthread_attr_init(&threadAttr);
  while (1)  {
    printf("pre connectDesc \n");
    int  fd = accept(listenFd,NULL,NULL);    
    if (fd < 0) {
      perror("Error on accept attempt\n");
      exit(EXIT_FAILURE);
    }      

    iPtr = (int*)calloc(2,sizeof(int*));
    iPtr[0] = fd;
    iPtr[1] = getpid();
    threadCount++;

    pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
    pthread_create(&threadId,&threadAttr,handleClient,(void*)iPtr);
    
  }
  pthread_attr_destroy(&threadAttr);    
}

void* handleClient(void* vPtr) {
  int* iPtr  = (int*)vPtr;
  int fd  = iPtr[0];
  int* threadId = &iPtr[1];
  free(vPtr);

  //  II.B.  Read command:
  char  buffer[BUFFER_LEN];
  char  command;
  int  fileNum;
  char text[BUFFER_LEN];
  int shouldContinue= 1;
  char* textPtr;
  
  while  (shouldContinue)
    {
      memset(buffer,'\0',BUFFER_LEN);
      memset(text  ,'\0',BUFFER_LEN);
      read(fd,buffer,BUFFER_LEN);
      printf("Thread %d received: %s\n",*threadId,buffer);
      sscanf(buffer,"%c %d \"%[^\"]\"",&command,&fileNum,text);

      // YOUR CODE HERE    
      if (command == DIR_CMD_CHAR) {
        dirCommand(fd);
      } else if (command == READ_CMD_CHAR) {
        readCommand(fd,fileNum);
      } else if (command == WRITE_CMD_CHAR) {
        textPtr = (char*)malloc(sizeof(char)*strlen(text));
        strncpy(textPtr,text,strlen(text));
        writeCommand(fd,fileNum,(void*)textPtr);
      } else if (command == DELETE_CMD_CHAR) {
        deleteCommand(fd,fileNum);
      } else if (command == CALC_CMD_CHAR) {
        calcCommand(fd,fileNum);
      } else if (command == QUIT_CMD_CHAR) {
        write(fd,STD_BYE_MSG,strlen(STD_BYE_MSG));
        shouldContinue = 0;
        close(fd);
        fflush(stdout);
        return(EXIT_SUCCESS);
      }
    }
  printf("Thread %d quitting. \n",*threadId);
  return(NULL); 
}

void* dirCommand(int fd) {
  DIR* dirPtr = opendir(".");

  if (dirPtr == NULL) {
    write(fd,STD_ERROR_MSG,strlen(STD_ERROR_MSG));
  }

  struct    dirent*   entryPtr;
  char buffer[BUFFER_LEN];
  char *filename;
  while ( (entryPtr = readdir(dirPtr)) != NULL ) 
    {
      filename = entryPtr->d_name;
      strncat(buffer,filename,BUFFER_LEN);
      strncat(buffer,"\n",BUFFER_LEN);  
    }
  closedir(dirPtr);
  write(fd,buffer,BUFFER_LEN);
  return(EXIT_SUCCESS);
}

void* readCommand(int clientFd, 
		  int fileNum) {
  char fileName[BUFFER_LEN];
  snprintf(fileName,BUFFER_LEN,"%d%s",fileNum,FILENAME_EXTENSION);

  char buffer[BUFFER_LEN];
  int fileFd = open(fileName,O_RDONLY,0440); //

  if (fileFd == -1) {
    write(clientFd,STD_ERROR_MSG,strlen(STD_ERROR_MSG));
  }

  read(fileFd,buffer,BUFFER_LEN);
  strcat(buffer,"\0");
  write(clientFd,buffer,strlen(buffer));
  close(fileFd);
  return(EXIT_SUCCESS);
}


void* writeCommand(int clientFd,
		   int  fileNum,
		   void* textPtr) {
  char* tPtr = (char*)textPtr;
  char fileName[BUFFER_LEN];
  snprintf(fileName,BUFFER_LEN,"%d%s",fileNum,FILENAME_EXTENSION);
  int textLen = strlen(tPtr);
  int numWritten;

  int fileFd = open(fileName,O_WRONLY|O_CREAT, 0660);
  if (textLen <= BUFFER_LEN) {
    printf("writeCmd: entered textLen <= buffLen cond, textLen = %d \n", textLen);
    numWritten = write(fileFd,tPtr,textLen);
  } else {
    printf("writeCmd: entered textLen > buffLen cond, textLen = %d \n", textLen);
    numWritten = write(fileFd,tPtr,BUFFER_LEN);
  }
  if (numWritten != -1 && fileFd != -1) {
    printf("writeCmd: no errors \n");
    fprintf(stdout,STD_OKAY_MSG);
    write(clientFd,STD_OKAY_MSG,strlen(STD_OKAY_MSG));
  } else {
    printf("writeCmd: there was an error");
    fprintf(stderr,STD_ERROR_MSG);
    write(clientFd,STD_ERROR_MSG,strlen(STD_ERROR_MSG));
  }
  free(textPtr);
  close(fileFd);
  return(EXIT_SUCCESS);
}

void* deleteCommand(int clientFd,
		    int fileNum  ) {
  char fileName[BUFFER_LEN];
  int status;
  snprintf(fileName,BUFFER_LEN,"%d%s",fileNum,FILENAME_EXTENSION);
  status = unlink(fileName);
  if (status != -1) {
    printf("deleteCmd: unlink executed properly\n");
    write(clientFd,STD_OKAY_MSG,strlen(STD_OKAY_MSG));
  } else {
    printf("deleteCmd: unlink ended abnormally \n");
    write(clientFd,STD_ERROR_MSG,strlen(STD_ERROR_MSG));
  }
  return(EXIT_SUCCESS);
}


void* calcCommand(int clientFd,
		  int fileNum  ) {
  pid_t childId = fork();
  int status;
  char buffer[BUFFER_LEN];

  if (childId < 0) {
    write(clientFd,STD_ERROR_MSG,strlen(STD_ERROR_MSG));
  } else if (childId == 0) {
    char fileName[BUFFER_LEN];
    snprintf(fileName,BUFFER_LEN,"%d%s",fileNum,FILENAME_EXTENSION);

    int inFd= open(fileName,O_RDONLY,0);
    int outFd= open(OUTPUT_FILENAME,O_WRONLY|O_CREAT|O_TRUNC,0660);
    int errFd= open(ERROR_FILENAME, O_WRONLY|O_CREAT|O_TRUNC,0660);

    if  ( (inFd < 0) || (outFd < 0) || (errFd < 0) )
      {
	fprintf(stderr,"Could not open one or more files\n");
	exit(EXIT_FAILURE);
      }
 
    close(0);
    dup(inFd);
    close(1);
    dup(outFd);
    close(2);
    dup(errFd);
    execl(CALC_PROGNAME,CALC_PROGNAME,NULL);
    fprintf(stderr,"CALC_PROGNAME failed to run \n");
    exit(EXIT_FAILURE);
  }

  childId = wait(&status);
  int        fileFd = open(OUTPUT_FILENAME,O_RDONLY,0440);
  int         errFd= open(ERROR_FILENAME, O_WRONLY|O_CREAT|O_TRUNC,0660);
  char errBuffer[BUFFER_LEN];

  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
      write(clientFd, STD_ERROR_MSG,strlen(STD_ERROR_MSG));
    } else {
      read(fileFd,buffer,BUFFER_LEN);
      if (strlen(buffer) <BUFFER_LEN) {
	read(errFd,errBuffer,BUFFER_LEN);
	strncat(buffer,errBuffer,BUFFER_LEN);           
      }
    }
    write(clientFd,buffer,BUFFER_LEN);
  }
  close(fileFd);
  close(errFd); 
  return(EXIT_SUCCESS);
}


//  PURPOSE:  To decide a port number, either from the command line arguments
//'argc' and 'argv[]', or by asking the user.  Returns port number.
int getPortNum(int argc,
	      char*argv[]
	      )
{
  //  I.  Application validity check:

  //  II.  Get listening socket:
  int portNum;

  if  (argc >= 2)
    portNum= strtol(argv[1],NULL,0);
  else
    {
      char buffer[BUFFER_LEN];

      printf("Port number to monopolize? ");
      fgets(buffer,BUFFER_LEN,stdin);
      portNum = strtol(buffer,NULL,0);
    }

  //  III.  Finished:  
  return(portNum);
}


//  PURPOSE:  To attempt to create and return a file-descriptor for listening
//to the OS telling this server when a client process has connect()-ed
//to 'port'.  Returns that file-descriptor, or 'ERROR_FD' on failure.
int getServerFileDescriptor (int port
			     )
{
  //  I.  Application validity check:

  //  II.  Attempt to get socket file descriptor and bind it to 'port':
  //  II.A.  Create a socket
  int socketDescriptor = socket(AF_INET, // AF_INET domain
				SOCK_STREAM, // Reliable TCP
				0);

  if  (socketDescriptor < 0)
    {
      perror(THIS_PROGRAM_NAME);
      return(ERROR_FD);
    }

  //  II.B.  Attempt to bind 'socketDescriptor' to 'port':
  //  II.B.1.  We'll fill in this datastruct
  struct sockaddr_in socketInfo;

  //  II.B.2.  Fill socketInfo with 0's
  memset(&socketInfo,'\0',sizeof(socketInfo));

  //  II.B.3.  Use TCP/IP:
  socketInfo.sin_family = AF_INET;

  //  II.B.4.  Tell port in network endian with htons()
  socketInfo.sin_port = htons(port);

  //  II.B.5.  Allow machine to connect to this service
  socketInfo.sin_addr.s_addr = INADDR_ANY;

  //  II.B.6.  Try to bind socket with port and other specifications
  int status = bind(socketDescriptor, // from socket()
		    (struct sockaddr*)&socketInfo,
		    sizeof(socketInfo)
		    );

  if  (status < 0)
    {
      perror(THIS_PROGRAM_NAME);
      return(ERROR_FD);
    }

  //  II.B.6.  Set OS queue length:
  listen(socketDescriptor,5);

  //  III.  Finished:
  return(socketDescriptor);
}


int  main (int argc,
	   char*argv[]
	   )
{
  //  I.  Application validity check:

  //  II.  Do server:
  int      port= getPortNum(argc,argv);
  int      listenFd= getServerFileDescriptor(port);
  int      status= EXIT_FAILURE;

  if  (listenFd >= 0)
    {
      doServer(listenFd);
      close(listenFd);
    }

  //  III.  Finished:
  return(EXIT_SUCCESS);
}
