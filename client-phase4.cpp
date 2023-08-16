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
#include <set>
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

    string no_of_files;
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
    for ( int file = 1; file <= stoi(no_of_files); file ++ ) {
        string file_name;

        fin >> file_name;
        files.push_back(file_name);
    }

    sort(files.begin(), files.end());
    vector<int> file_unique_id (stoi(no_of_files), -1);
    vector <string> filenames;
    set <string> files_not_found;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_path)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            string file_name = ent -> d_name;
            if (file_name != "." && file_name != ".." && file_name != "Downloaded") {
                filenames.push_back(file_name);
                cout << file_name << "\n";
            }
        }
        closedir (dir);
    }
   
    else {
        perror ("");
        return EXIT_FAILURE;
    }

    sort(filenames.begin(), filenames.end());

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

    string random_send = "Wasting a turn";
    char random_read[100] = {0};

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
        char num_neighbor_files[100]={0};
        char yes_or_no[100] = {0};
        
        read(new_fd, buffer[0], 100);
        send(new_fd, &client_no[0], client_no.length(), 0);
        read(new_fd, buffer[1], 100);
        send(new_fd, &port[0], port.length(), 0);
        read(new_fd, buffer[2], 100);
        send(new_fd, &unique_id[0], unique_id.length(), 0);
        read(new_fd, num_neighbor_files, 100);
        send(new_fd, &no_of_files[0], no_of_files.length(), 0);
        
        for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(new_fd, neighbor_filename, 100);
            bool found = binary_search(filenames.begin(), filenames.end(), (string) neighbor_filename);
            if (found) send(new_fd, "Y", 1, 0);
            else {
                send(new_fd, "N", 1, 0);
                files_not_found.insert(neighbor_filename);
            }
        }

        read(new_fd, random_read, 100);
        for(int nf = 0; nf < stoi(no_of_files); nf ++) {
            send(new_fd, &(files[nf])[0], files[nf].length(), 0);
            read(new_fd, yes_or_no, 100);
            if((string) yes_or_no == "Y"){
                int unique_id_neigh = strtol(buffer[2], nullptr, 10);
                if(file_unique_id[nf] < 0) file_unique_id[nf] = unique_id_neigh;
                else if(file_unique_id[nf] > unique_id_neigh) file_unique_id[nf] = unique_id_neigh;
            }
        }

        close(new_fd);
        neighbor_initial_data.push_back(make_tuple(strtol(buffer[0], nullptr, 10), (string) buffer[1], (string) buffer[2]));
    }

    for (int neigh = 0; neigh < no_of_neighbors; neigh ++ ) {
        string neighbor_id = neighbors[neigh].first;
        string neighbor_port = neighbors[neigh].second;
        if(stoi(neighbor_id) > stoi(client_no)){continue;}

        
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
        char num_neighbor_files[100]={0};
       
        char yes_or_no[100] = {0};

        send(neighbor_sockfd, &client_no[0], client_no.length(), 0);
        read(neighbor_sockfd, buffer[0], 100);
        send(neighbor_sockfd, &port[0], port.length(), 0);
        read(neighbor_sockfd, buffer[1], 100);
        send(neighbor_sockfd, &unique_id[0], unique_id.length(), 0);
        read(neighbor_sockfd, buffer[2], 100);
        send(neighbor_sockfd, &no_of_files[0], no_of_files.length(), 0);
        read(neighbor_sockfd, num_neighbor_files, 100);

        for(int nf = 0; nf < stoi(no_of_files); nf ++) {
            send(neighbor_sockfd, &(files[nf])[0], files[nf].length(), 0);
            read(neighbor_sockfd, yes_or_no, 100);
            if((string) yes_or_no == "Y"){
                int unique_id_neigh = strtol(buffer[2], nullptr, 10);
                if(file_unique_id[nf] < 0) file_unique_id[nf] = unique_id_neigh;
                else if(file_unique_id[nf] > unique_id_neigh) file_unique_id[nf] = unique_id_neigh;
            }
        }

        send(neighbor_sockfd, &random_send[0], random_send.length(), 0);
        for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(neighbor_sockfd, neighbor_filename, 100);
            bool found = binary_search(filenames.begin(), filenames.end(), (string) neighbor_filename);
            if (found) send(neighbor_sockfd, "Y", 1, 0);
            else {
                send(neighbor_sockfd, "N", 1, 0);
                files_not_found.insert(neighbor_filename);
            }
        }

        close(neighbor_sockfd);
        neighbor_initial_data.push_back(make_tuple(strtol(buffer[0], nullptr, 10), (string) buffer[1], (string) buffer[2]));
    }

    //round 1 over
    no_of_connected_neighbors = 0;
    no_of_connected_neighbors_lesser = 0;
    no_of_connected_neighbors_greater = 0;
    vector<string> files_not_found_vec;
    for(auto s:files_not_found){
        files_not_found_vec.push_back(s);
    }
    vector<int> file_not_found_id(files_not_found_vec.size(),-1); 

    while (no_of_connected_neighbors_lesser < no_of_neighbors_lesser) {
    
        // now accept an incoming connection:

        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd < 0){
            cout<<"Error in accepting"<<endl;
        }
        no_of_connected_neighbors ++;
        no_of_connected_neighbors_lesser ++;
        char buffer[100] = {0};
        char num_neighbor_files[100]={0};
        char yes_or_no[100] = {0};
        string num_files_2=to_string(files_not_found.size());
        
        read(new_fd, buffer, 100);
        send(new_fd, &unique_id[0], unique_id.length(), 0);
        read(new_fd, num_neighbor_files, 100);
        send(new_fd, &num_files_2[0], num_files_2.length(), 0);
        
        for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(new_fd, neighbor_filename, 100);
            bool found = binary_search(filenames.begin(), filenames.end(), (string) neighbor_filename);
            if (found) send(new_fd, "Y", 1, 0);
            else send(new_fd, "N", 1, 0);
        }

        read(new_fd, random_read, 100);
        int iter=0;
        for(auto s: files_not_found) {
            send(new_fd, &s[0], s.length(), 0);
            read(new_fd, yes_or_no, 100);
            if((string) yes_or_no == "Y"){
                int unique_id_neigh = strtol(buffer, nullptr, 10);
                if(file_not_found_id[iter]<0)file_not_found_id[iter]=unique_id_neigh;
                else if(file_not_found_id[iter]>unique_id_neigh)file_not_found_id[iter]=unique_id_neigh;
            }
            iter++;
        }

        close(new_fd);
        
    }

    for (int neigh = 0; neigh < no_of_neighbors; neigh ++ ) {
        string neighbor_id = neighbors[neigh].first;
        string neighbor_port = neighbors[neigh].second;
        if(stoi(neighbor_id) < stoi(client_no)){continue;}

        
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
        no_of_connected_neighbors_greater ++;
        char buffer[1024] = {0};
        char num_neighbor_files[100]={0};
        char yes_or_no[100] = {0};

        string num_files_2=to_string(files_not_found.size());

        send(neighbor_sockfd, &unique_id[0], unique_id.length(), 0);
        read(neighbor_sockfd, buffer, 100);
        send(neighbor_sockfd, &num_files_2[0], num_files_2.length(), 0);
        read(neighbor_sockfd, num_neighbor_files, 100);

        int iter=0;
        for(auto s: files_not_found) {
            send(neighbor_sockfd, &s[0], s.length(), 0);
            read(neighbor_sockfd, yes_or_no, 100);
            if((string) yes_or_no == "Y"){
                int unique_id_neigh = strtol(buffer, nullptr, 10);
                if(file_not_found_id[iter]<0)file_not_found_id[iter]=unique_id_neigh;
                else if(file_not_found_id[iter]>unique_id_neigh)file_not_found_id[iter]=unique_id_neigh;
            }
            iter++;
        }       

        send(neighbor_sockfd, &random_send[0], random_send.length(), 0);
        
        for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(neighbor_sockfd, neighbor_filename, 100);
            bool found = binary_search(filenames.begin(), filenames.end(), (string) neighbor_filename);
            if (found) send(neighbor_sockfd, "Y", 1, 0);
            else send(neighbor_sockfd, "N", 1, 0);
        }

        close(neighbor_sockfd);
    }

    //round 2 over
    vector<string> filenames_2;
    for(int nf=0;nf<stoi(no_of_files);nf++){
        if(file_unique_id[nf]<0)filenames_2.push_back(filenames[nf]);
    }
    vector<int> filenames_2_id(filenames_2.size(),-1); 

    while (no_of_connected_neighbors_greater < no_of_neighbors_greater) {
    
        // now accept an incoming connection:

        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd < 0){
            cout<<"Error in accepting"<<endl;
        }
        no_of_connected_neighbors ++;
        no_of_connected_neighbors_greater ++;
        char buffer[100] = {0};
        char num_neighbor_files[100]={0};
        string num_files_2=to_string(filenames_2.size());
        
        read(new_fd, num_neighbor_files, 100);
        send(new_fd, &num_files_2[0], num_files_2.length(), 0);
        
        for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(new_fd, neighbor_filename, 100);
            int index=find(files_not_found_vec.begin(),files_not_found_vec.end(),neighbor_filename)-files_not_found_vec.begin();
            string unique_id_tosend=to_string(file_not_found_id[index]);
            send(new_fd, &unique_id_tosend[0], unique_id_tosend.size(), 0);
        }

        read(new_fd, random_read, 100);
        for(int nf = 0; nf < stoi(num_files_2); nf ++) {
            send(new_fd, &(filenames_2[nf])[0], filenames_2[nf].length(), 0);
            read(new_fd, buffer, 100);
            int unique_id_neigh = strtol(buffer, nullptr, 10);
            if(filenames_2_id[nf]<0)filenames_2_id[nf]=unique_id_neigh;
            else if(filenames_2_id[nf] > unique_id_neigh) filenames_2_id[nf] = unique_id_neigh;
        }
        close(new_fd);
    }

    for (int neigh = 0; neigh < no_of_neighbors; neigh ++ ) {
        string neighbor_id = neighbors[neigh].first;
        string neighbor_port = neighbors[neigh].second;
        if(stoi(neighbor_id) > stoi(client_no)){continue;}

        
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
        char buffer[1024] = {0};
        char num_neighbor_files[100]={0};
        string num_files_2=to_string(filenames_2.size());
       
        send(neighbor_sockfd, &num_files_2[0], num_files_2.length(), 0);
        read(neighbor_sockfd, num_neighbor_files, 100);

        for(int nf = 0; nf < stoi(num_files_2); nf ++) {
            send(neighbor_sockfd, &(filenames_2[nf])[0], filenames_2[nf].length(), 0);
            read(neighbor_sockfd, buffer, 100);
            int unique_id_neigh = strtol(buffer, nullptr, 10);
            if(filenames_2_id[nf]<0)filenames_2_id[nf]=unique_id_neigh;
            else if(filenames_2_id[nf] > unique_id_neigh) filenames_2_id[nf] = unique_id_neigh;
        }

        send(neighbor_sockfd, &random_send[0], random_send.length(), 0);
         for(int nf = 0; nf < strtol(num_neighbor_files, nullptr, 10); nf ++) {
            char neighbor_filename[100]={0};
            read(neighbor_sockfd, neighbor_filename, 100);
            int index=find(files_not_found_vec.begin(),files_not_found_vec.end(),neighbor_filename)-files_not_found_vec.begin();
            string unique_id_tosend=to_string(file_not_found_id[index]);
            send(neighbor_sockfd, &unique_id_tosend[0], unique_id_tosend.size(), 0);
        }
        close(neighbor_sockfd);
    }


    sort(neighbor_initial_data.begin(), neighbor_initial_data.end());
    for (auto elem: neighbor_initial_data) {
        cout << "Connected to " << get<0>(elem) << " with unique-ID " << get<2>(elem) << " on port " << get<1>(elem) << "\n";
    }

    vector<int> depth(stoi(no_of_files),0);
    vector<int> final_unique_id(stoi(no_of_files),0);

    for(int nf = 0; nf < stoi(no_of_files); nf ++) {
        int unique_id_neigh = file_unique_id[nf];
        if(unique_id_neigh>0){
            depth[nf]=1;
            final_unique_id[nf]=unique_id_neigh;
        }
    }

    for(int nf=0;nf<filenames_2.size();nf++){
        string f=filenames_2[nf];
        int index=find(files.begin(),files.end(),f)-files.begin();
        int unique_id_neigh = filenames_2_id[nf];
        if(unique_id_neigh>0){
            depth[index]=2;
            final_unique_id[index]=unique_id_neigh;
        }
    }
    for(int nf = 0; nf < stoi(no_of_files); nf ++) {
        cout << "Found " << files[nf] << " at " << final_unique_id[nf] << " with MD5 0 at depth " << depth[nf] << "\n";
    }
    close(sockfd);
}
