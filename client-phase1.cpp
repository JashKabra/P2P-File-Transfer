#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <string>
using namespace std;

int main(int argc, char** argv){

    if(argc != 3) {
        cout << "Enter 2 arguments - Config File and Directory Path\n";
        exit(0);
    }

    string client_no;
    string port;
    string unique_id;

    int no_of_neighbors;
    int no_of_neighbors_lesser = 0;
    int no_of_neighbors_greater = 0;
    vector <pair <string, string> > neighbors;

    vector <tuple <int, string, string> > neighbor_initial_data;
    int no_of_connected_neighbors = 0;
    int no_of_connected_neighbors_lesser = 0;
    int no_of_connected_neighbors_greater = 0;

    int no_of_files;
    vector <string> files;

    string config_file = argv[1];
    char* dir_path = argv[2];

    ifstream fin(config_file);

    fin >> client_no >> port >> unique_id;
    fin >> no_of_neighbors;

    for ( int neighbor = 1; neighbor <= no_of_neighbors; neighbor ++ ) {
        string neighbor_no;
        string neighbor_port;

        fin >> neighbor_no >> neighbor_port;
        if(stoi(neighbor_no) < stoi(client_no)) no_of_neighbors_lesser++;
        else no_of_neighbors_greater++;
        neighbors.push_back({neighbor_no, neighbor_port});
    }
    
    fin >> no_of_files;
    for ( int file = 1; file <= no_of_files; file ++ ) {
        string file_name;
        fin >> file_name;
        files.push_back(file_name);
    }

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_path)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            string file_name = ent -> d_name;
            if (file_name != "." && file_name != ".." && file_name != "Downloaded") {
                cout << file_name << "\n";
            }
        }
        closedir (dir);
    }
    else {
        perror ("");
        return EXIT_FAILURE;
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    //char* port1=&port[0];
    getaddrinfo(NULL, &port[0], &hints, &res);

    // make a socket:

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd == 0) {
        cout << "Error in creating socket\n";
    }

    // bind it to the port we passed in to getaddrinfo():

    if(bind(sockfd, res->ai_addr, res->ai_addrlen)<0){
        cout<<"Binding Error"<<endl;
    }

    if(listen(sockfd, 10)<0){
        cout<<"Error in listening"<<endl;
    }

    while (no_of_connected_neighbors_greater < no_of_neighbors_greater) {
    
        // now accept an incoming connection:

        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd < 0){
            cout<<"Error in accepting"<<endl;
        }
        no_of_connected_neighbors ++;
        no_of_connected_neighbors_greater ++;
        char buffer[3][100] = {0};
        read(new_fd, buffer[0], 100);
        send(new_fd, &client_no[0], client_no.length(), 0);
        read(new_fd, buffer[1], 100);
        send(new_fd, &port[0], port.length(), 0);
        read(new_fd, buffer[2], 100);
        send(new_fd, &unique_id[0], unique_id.length(), 0);

        close(new_fd);
        neighbor_initial_data.push_back(make_tuple(strtol(buffer[0], nullptr, 10), (string) buffer[1], (string) buffer[2]));
    }

    for (int neigh = 0; neigh < no_of_neighbors; neigh ++ ) {
        string neighbor_id = neighbors[neigh].first;
        string neighbor_port = neighbors[neigh].second;
        if(stoi(neighbor_id) > stoi(client_no)) continue;

        
        int neighbor_sockfd;
        struct addrinfo neighbor_hints, *neighbor_res;
        memset(&neighbor_hints, 0, sizeof neighbor_hints);
        neighbor_hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
        neighbor_hints.ai_socktype = SOCK_STREAM;
        // neighbor_hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
        
        getaddrinfo("127.0.0.1", &neighbor_port[0], &neighbor_hints, &neighbor_res);
        neighbor_sockfd = socket(neighbor_res->ai_family, neighbor_res->ai_socktype, neighbor_res->ai_protocol);
        
        if(neighbor_sockfd == 0){
            cout<<"Error in creating neighbour socket"<<endl;
        }

        int connected =-1;

        while(connected ==-1){
            connected = connect(neighbor_sockfd, neighbor_res -> ai_addr, neighbor_res -> ai_addrlen);
        }

        no_of_connected_neighbors ++;
        no_of_connected_neighbors_lesser ++;
        char buffer[3][1024] = {0};
        send(neighbor_sockfd, &client_no[0], client_no.length(), 0);
        read(neighbor_sockfd, buffer[0], 100);
        send(neighbor_sockfd, &port[0], port.length(), 0);
        read(neighbor_sockfd, buffer[1], 100);
        send(neighbor_sockfd, &unique_id[0], unique_id.length(), 0);
        read(neighbor_sockfd, buffer[2], 100);

        close(neighbor_sockfd);
        neighbor_initial_data.push_back(make_tuple(strtol(buffer[0], nullptr, 10), (string) buffer[1], (string) buffer[2]));
    }

    sort(neighbor_initial_data.begin(), neighbor_initial_data.end());
    for (auto elem: neighbor_initial_data) {
        cout << "Connected to " << get<0>(elem) << " with unique-ID " << get<2>(elem) << " on port " << get<1>(elem) << "\n";
    }

    close(sockfd);

}
