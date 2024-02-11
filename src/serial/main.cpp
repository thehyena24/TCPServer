/* 
 * tcpserver.c - A multithreaded TCP echo server 
 * usage: tcpserver <port>
 * 
 * Testing : 
 * nc localhost <port> < input.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <unistd.h>

using namespace std;

class stopConnection
{
private:
  int server_socket;

public:
  stopConnection(int sock)
  {
    server_socket = sock;
  }

  ~stopConnection()
  {
    close(server_socket);
  }
};

int main(int argc, char ** argv) 
{
  int portno; 
  map <string, string> KV_DATASTORE;

  if (argc != 2) 
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  portno = atoi(argv[1]);

  int server_socket = socket(AF_INET, SOCK_STREAM, 0);  

  stopConnection x(server_socket);

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(portno);
  server_address.sin_addr.s_addr = INADDR_ANY; 
  bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));

  while(true)
  {
    listen(server_socket, 100);

    int client_socket = accept(server_socket, NULL, NULL);

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
        // cout << val << endl;

        i += 1;
      }

      else if(temp[i] == "WRITE")
      {
        string key = temp[i + 1];
        string value = temp[i + 2];

        value.erase(0, 1);

        KV_DATASTORE[key] = value;

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

        if(KV_DATASTORE.find(key) == KV_DATASTORE.end())
        {
          string message = "NULL\n";
          const char* val = message.c_str();

          send(client_socket, val, strlen(val), 0);
        }

        else
        {
          KV_DATASTORE.erase(key);

          string message = "FIN\n";
          const char* val = message.c_str();

          send(client_socket, val, strlen(val), 0);

          i += 1;
        }
      }
    }
  }

  close(server_socket);

  return 0;
}
