#include <iostream> //cout
#include <cstdio> //printf
#include <cstring> //strlen
#include <string> //string
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <netdb.h> //hostent
#include <unistd.h>
#include <signal.h>

#define SERVER "192.168.0.80"
#define PORT 8887
#define BUFSIZE 1024

using namespace std;

/*
    TCP Client class
*/
class TcpClient{
private:
    int sock;
    std::string address;
    string response_data;
    int port;
    struct sockaddr_in server;

public:
    TcpClient();
    bool conn(string, int);
    bool send_data(string data) const;
    string receive();
};

TcpClient::TcpClient(){
    sock = -1;
    port = 0;
    address = "";
}

bool TcpClient::conn(string address , int port){
    // create socket if it is not already created
    if(sock == -1){
        //Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1){
            perror("Could not create socket");
        }

        printf("<INIT>\n");
    }


    // setup address structure
    if(inet_addr(address.c_str()) == -1){
        struct hostent *he;
        struct in_addr **addr_list;

        //resolve the hostname, its not an ip address
        if((he = gethostbyname( address.c_str() )) == NULL){
            //gethostbyname failed
            herror("gethostbyname");
            cout<<"Failed to resolve hostname\n";

            return false;
        }

        // Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
        addr_list = (struct in_addr **) he->h_addr_list;

        for(int i = 0; addr_list[i] != nullptr; i++)
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];

            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;

            break;
        }
    }

        //plain ip address
    else{
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }

    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    //Connect to remote server
    if( connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0 )
    {
        perror("connect failed. Error");
        return false;
    }

    cout<<"Connected\n";
    return true;
}

void parse_message(int sd, char buffer[BUFSIZE]) {
    std::string input, response;
    std::size_t found_t, startIndex, stopIndex, length;
    //char * input;

    input = buffer;
    input.erase(input.find_last_not_of(" \n\r\t") + 1); //remove white space and new line etc

    if (input != "#\\~/#") { // ignore periodic ~ messages
        printf("-->> %s\n", input.c_str());
    }
}

bool TcpClient::send_data(string data) const{
    char buffer[BUFSIZE];

    // Send some data
    if(send(sock , data.c_str() , strlen(data.c_str()) , 0) < 0){
        perror("Send failed : ");
        return false;
    }
//    printf("<Connection Terminated>\n");
    return false;
}

string TcpClient::receive(){
    char buffer[BUFSIZE], input[BUFSIZE];
    string reply;
    long int valread;

    while (sock > 0) {
        //Receive a reply from the server
        if ((valread = read(sock, buffer, BUFSIZE)) <= 0) {
            printf("<Recv failed>");
            break;
        }
        else {   //Else the message is ready for parsing
            buffer[valread] = '\0'; //null terminate buffer
            parse_message(sock, buffer);
        }


    }

    return ("END");
}


void closeSock(int num,int sock){
    int valread;
    char buffer[BUFSIZE];

    if ((valread = read(sock, buffer, BUFSIZE)) <= 0){
        close(sock);
        usleep(200);
    }
}

int main(int argc , char *argv[]){

    TcpClient c;
    //connect to host
    c.conn(SERVER , PORT);
//    signal(SIGTERM, sock, closeSock);

    //send login data
    c.send_data("F7F");

    //receive loop
    c.receive();
    printf("eee\n");


    return 0;
}
