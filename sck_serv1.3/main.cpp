#include <cstdio>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <sstream>

#define TRUE 1
#define FALSE 0
#define PORT 8887

/*
 * TODO:
 * Parallel threads, especially including ping
 * split all connections into new threads
 * create thread pool
 */



bool G_ackReceived = false;
std:: string G_settingsFile = "Settings.txt";
std::vector<std::string> G_enrolled(255); //Array to hold G_enrolled devices min 255 deep
std::vector<std::string> G_online(255); //Array to hold list of units connected and online
int max_clients = 255;
fd_set readfds;

void loadSettings(){
    printf("<Loading Settings>\n");
    std::string item_name, input;
    std::ifstream nameFileout;
    std::size_t found_t, startIndex, stopIndex, length;

    nameFileout.open(G_settingsFile);

    while(std::getline(nameFileout, input)){
        //std::cout << "input: " << input << std::endl;
        found_t = input.find("Enrolled");
        if (found_t!=std::string::npos) { // Found reference string
            startIndex = input.find('[');
            if (startIndex != std::string::npos) { // Found reference point [
                stopIndex = input.find(']');
                length = (stopIndex - startIndex) - 1;
                if (stopIndex != std::string::npos) { // Found reference point ]
                    input = input.substr(startIndex + 1, length);
                    input.erase(input.find_last_not_of(" \"\'\n\r\t")+1); //remove white space and new line etc
                    std::stringstream ss(input);
                    while(ss.good()){
                        std::string substr;
                        getline( ss, substr, ',' );
                        if(substr != ",") {
                            G_enrolled.push_back(substr);
                        }
                    }
                    printf("Loaded Enrolled units: [");
                    int a = 0;
                    int firstprint = 0;
                    for(auto & i : G_enrolled) {
                        if (a == 0){
                            if(!i.empty()) {
                                std::cout << i;
                                firstprint ++;
                            }
                        }
                        else {
                            if(!i.empty()) {
                                if (firstprint >= 1) {
                                    std::cout << ", " << i;
                                } else {
                                    std::cout << i;
                                }
                                firstprint ++;
                            }
                        }

                        a++;
                    }
                    printf("]\n");


                }
            }
        }
    }
    nameFileout.close();

}

void parse_message(int sd, int valread, char buffer[1025]){
    std::string input, response;
    std::size_t found_t, startIndex, stopIndex, length;

    //set the string terminating NULL byte on the end of the data read
    buffer[valread] = '\0';

    if (G_ackReceived){ //if acknowledge response on respond with ack ok
        char message[8] = "Ack_ok";
        send(sd , message , strlen(message) , 0 ); //Return message received confirmation
    }


    input = buffer;
    input.erase(input.find_last_not_of(" \n\r\t")+1); //remove white space and new line etc


    if (input != "~") { // ignore periodic ~ messages
        printf("-->> %s\n", input.c_str());
    }

    if (input == "s.clients"){
        response+= "-Clients online:----\n";
        for(auto & a : G_online) {
            if (a.length()>0){
                response += "  |--"+ a +"\n";
//              printf("%s\n",a.c_str());
            }
        }
        response += "--------------------\n";
        //printf("%s\n",response.c_str());

        char * cstr = new char [response.length()+1];
        std::strcpy (cstr, response.c_str());
        send(sd, cstr, strlen(cstr), 0);
        response ="";
    }


    found_t = input.find("s.enrol");
    if (found_t!=std::string::npos){ // Found reference string
        startIndex = input.find('[');
        if (startIndex!=std::string::npos){ // Found reference point [
            stopIndex = input.find(']');
            length = (stopIndex - startIndex)-1;
            if (stopIndex!=std::string::npos){ // Found reference point ]
                input = input.substr(startIndex+1, length);
                bool newEnrolment = false;
                for (auto &e : G_enrolled){
                    if (e == input) {
                        newEnrolment = true;
                    }
                    else {
                        newEnrolment = false;
                    }
                }
                if (!newEnrolment){
                    G_enrolled.push_back(input);
                    std::cout << "New unit enrolled: "<< input << "\n";
                    //Save G_enrolled list to file
                }
                else{
                    std::cout << "Unit already enrolled: "<< input << "\n";
                }
            }
        }
    }
    found_t = input.find("s.enrolled");
    if (found_t!=std::string::npos){ // Found reference string
        response = "[";
        int index = 0;
        for (auto &e : G_enrolled) {
            if (e != "") {
                if (index != 0) {
                    response += ", " + e;
                    //std::cout <<", " << e ;
                } else {
                    response += e;
                    //std::cout << "[" << e ;
                }
            }
            index ++;
        }
        //std::cout << "]\n";
        response += "]\n";
        char * cstr = new char [response.length()+1];
        std::strcpy (cstr, response.c_str());
        send(sd, cstr, strlen(cstr), 0);
        response = "";
    }




    }

int login(int new_socket) {
//send new connection greeting message
    char buffer[20]; //data buffer of 1K
    char message[19] = "<Login Required>";
    std::string passUsed;
    std::size_t found;
    bool enrolled_flag = false;

    if (send(new_socket, message, strlen(message), 0) != strlen(message)){
        perror("send");
    }
    if (read( new_socket , buffer, 20) != 0) {
        passUsed = buffer;
        passUsed.erase(passUsed.find_last_not_of(" \n\r\t") + 1); //remove white space and new line etc
        printf("  |---->Password used: [%s]\n", passUsed.c_str());

//        found = passUsed.find("");
        if (passUsed.empty()){
            printf("<Http Login Denied>\n");
            close(new_socket);
            return(-1);
        }

        found = passUsed.find("GET / HTTP");
        if (found!=std::string::npos){
            printf("<Http Login Denied>\n");
            close(new_socket);
            return(-1);
        }

        found = passUsed.find("GET / HTTPS");
        if (found!=std::string::npos){
            printf("<Http Login Denied>\n");
            close(new_socket);
            return(-1);
        }


        for (auto &e : G_enrolled) {
            if (e == passUsed) {
                enrolled_flag = true;
            } else {
                enrolled_flag = false;
            }

        }
        if (enrolled_flag) {
            printf("<Login Accepted>\n");
            char accepted[18] = "<Login Accepted>\n";
            send(new_socket, accepted, strlen(accepted), 0);
            G_online.push_back(passUsed);
        }
        else {
            printf("<Login Denied>\n");
            close(new_socket);
            return(-1);
        }
    }
    return (new_socket);
}

void manage_clients(int client_socket[max_clients], sockaddr_in address,int sd){
    char buffer[1024];
    int valread, addrlen;
    addrlen= sizeof(address);

    for (int i = 0; i < max_clients; i++){
        sd = client_socket[i];
        if (FD_ISSET( sd , &readfds)){
            //Check if it was for closing , and also read the incoming message
            if ((valread = read( sd , buffer, 1024)) == 0){
                //Somebody disconnected , get the ex-client's details and print
                getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
                printf("<Client disconnected , ip %s , port %d>\n" ,
                       inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                //Close the socket and mark as 0 in list for reuse
                close(sd);
                client_socket[i] = 0;
            }


            else{   //Else the message is ready for parsing
                parse_message(sd, valread, buffer);
            }
        }
    }
}


void new_conn(int master_socket, sockaddr_in address,
              int client_socket[max_clients], int sd) {
    //If something happened on the master socket ,
    int addrlen, new_socket;
    addrlen= sizeof(address);

    //then its an incoming connection
    if (FD_ISSET(master_socket, &readfds)) {
        if ((new_socket = accept(master_socket,
                                 (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //inform user of socket number - used in send and receive commands
        printf("<New connection [%d], IP: %s , pipe: %d>\n" ,
               new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        new_socket = login(new_socket); //LOGIN function

        //add new socket to array of sockets
        if (new_socket != -1){
            for (int i = 0; i < max_clients; i++) {
                //if position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    //printf("Added to socket: %d\n" , i);
                    break;
                }
            }
        }
    }

    manage_clients(client_socket, address, sd); //Manage Clients function

}


int main(int argc , char *argv[]){
    //G_enrolled = {"111","333","7F7","F1F", "F7F"};
    int opt = TRUE;
    int master_socket , addrlen , client_socket[max_clients] ,
            activity, i , valread , sd, max_sd;
    struct sockaddr_in address{};
    char buffer[1025]; //data buffer of 1K

    printf("<Init>\n");
    loadSettings();


    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++){
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 ){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    //bind the socket to localhost port [abcd]
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("<Server online on port %d> \n", PORT);

    //try to specify maximum of pending connections for the master socket
    if (listen(master_socket, max_clients) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    printf("<Waiting for connections ...>\n");


    while(TRUE){
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++){
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select(max_sd + 1 , &readfds , nullptr , nullptr , nullptr);

        if ((activity < 0) && (errno!=EINTR)){
            printf("select error");
        }

        new_conn(master_socket, address, client_socket, sd); //Threads go here


    }

    return 0;
}



