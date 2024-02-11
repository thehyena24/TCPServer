#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <sstream>
#include <sys/socket.h>
#include <regex>
#include <unistd.h>
#include <arpa/inet.h>
#include <queue>
using namespace std;

#define maxCon 100
#define SOCKERROR (-1)
#define THREADPOOL 20

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

pthread_t thread_pool[THREADPOOL];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

char client_message[1024];
map<string,string> KV_DATASTORE;
queue <int> clients;

int check(int code , const char* msg);
void handle_connection(int p_client_socket);
void* thread_function(void*);

int main(int argc, char ** argv) 
{
  int portno; /* port to listen on */
  
  /* 
   * check command line arguments 
   */
  if (argc != 2) 
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // DONE: Server port number taken as command line argument
  portno = atoi(argv[1]);

  SA_IN server_addr , client_addr ;
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(portno);

  for(int i = 0 ; i < THREADPOOL ; i++)
  {
    pthread_create(&thread_pool[i],NULL, thread_function, NULL);
  }

  int server_sock = check(socket(AF_INET, SOCK_STREAM, 0), "Socket faild to exist");
  int bindStatus = check(bind(server_sock, (SA *) &server_addr, sizeof(server_addr)), "Error binding socket to local address");

  check(listen(server_sock, maxCon),"Listening failed");

  while(true)
  {
    int client_socket;
    int addr_size = sizeof(SA_IN);
    check(client_socket = accept(server_sock,(SA *)&client_addr,(socklen_t *)&addr_size), "accept failed");

    pthread_mutex_lock(&mutex);
    clients.push(client_socket);
    pthread_cond_signal(&cond_var);
    pthread_mutex_unlock(&mutex);
  }

  close(server_sock);
  return 0;
}

void* thread_function(void*)
{   
  while(true)
  {
    int pclient;

    pthread_mutex_lock(&mutex);

    if(clients.empty())
    {
      pthread_cond_wait(&cond_var, &mutex);
      pclient = clients.front();
      clients.pop();
    }

    pthread_mutex_unlock(&mutex);
    
    handle_connection(pclient);
  }
}

int check(int code , const char *msg)
{
  if(code == SOCKERROR)
  {
    perror(msg);
    exit(0);
  }

  return code;
}

void handle_connection(int p_client_socket)
{
  int client_socket = p_client_socket;

  char buffer[1024] = {0};
  read(client_socket, buffer, sizeof(buffer));

  char* token = strtok(buffer, "\n");
  vector <string> temp;

  while(token != NULL)
  {
    temp.push_back(token);
    token = strtok(NULL, "\n");
  }

  for(int i = 0; i < temp.size(); i++)
  {
    if(temp[i] == "END")
    {
      string lol = "\n";
      const char* val = lol.c_str();
      send(client_socket, val, strlen(val), 0);
      close(client_socket);
      break;
    }

    else if(temp[i] == "READ")
    {
      string key = temp[i + 1];

      if(KV_DATASTORE.count(key) == 0)
      {
        string message = "NULL\n";
        const char* val = message.c_str();

        send(client_socket, val, strlen(val), 0);
      }

      else
      { 
        string value = KV_DATASTORE[key];
        value += "\n";

        const char* val = value.c_str();

        send(client_socket, val, strlen(val), 0);
      }

      i += 1;
    }

    else if(temp[i] == "WRITE")
    {
      string key = temp[i + 1];
      string value = temp[i + 2];

      value.erase(0, 1);

      pthread_mutex_lock(&mutex);
      KV_DATASTORE[key] = value;
      pthread_mutex_unlock(&mutex);

      string message = "FIN\n";
      const char* val = message.c_str();

      send(client_socket, val, strlen(val), 0);

      i += 2;
    }

    else if(temp[i] == "COUNT")
    {
      // cout << "Count: " << KV_DATASTORE.size() << endl;
      int count = KV_DATASTORE.size();

      string str_count = to_string(count);
      // str_count += "\n";

      const char* char_count = str_count.c_str();
      send(client_socket, char_count, strlen(char_count), 0);
    }

    else if(temp[i] == "DELETE")
    {
      string key = temp[i + 1];

      pthread_mutex_lock(&mutex);
      if(KV_DATASTORE.find(key) == KV_DATASTORE.end())
      {
        pthread_mutex_unlock(&mutex);

        string message = "NULL\n";
        const char* val = message.c_str();

        send(client_socket, val, strlen(val), 0);
      }

      else
      {
        KV_DATASTORE.erase(key);
        pthread_mutex_unlock(&mutex);

        string message = "FIN\n";
        const char* val = message.c_str();

        send(client_socket, val, strlen(val), 0);

        i += 1;
      }
    }
  }
}