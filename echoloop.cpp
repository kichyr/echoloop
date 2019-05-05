#include <unistd.h>
#include <stdio.h>
#include<stdlib.h>
#include <fcntl.h>
#include<errno.h>
#include <sys/stat.h>
#include<signal.h>
#include<vector>
#include<string>
#include<iostream>
#include <string.h>
#include <iterator>
#include <thread>
#include <mutex>


//overlord <<
template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    if ( !v.empty() ) {
        out << '[';
        std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "\b\b]";
    }
    return out;
}
//
std::mutex m;
int fd;
int SIZE = 4096;
char serverFIFO[10] = "fifo.serv";


//handler на обработку останвки сервера
void new_handler(int a) {
    //close(a);
    //close(rd);
    close(fd);
    unlink(serverFIFO);
    exit(0);
}

void print_thread_f(std::vector<std::string>* all_args) {
    while(1) {
        sleep(1);
        //критическая секция
        m.lock();
        for(auto i : *all_args)
            std::cout << i << std::endl;
        std::cout << std::endl;
        fflush(stdout);
        m.unlock();
        //
    }
}

int main(int argc, char* argv[]) {
    int fd;
    int rd;
    char buff[4096];
    int read_byte = 0;
    

    ///-----------------------------------если его не существует-------------------------------------   
    // пытаемся открыть fifo.serv   
    if((fd = open(serverFIFO, O_WRONLY|O_NONBLOCK)) < 0) {
        signal(SIGINT, new_handler);
        //Создаем фифо (неблокирующий режим)
        if(mknod(serverFIFO, S_IFIFO | 0666, 0) < 0) {
            printf("can't create ServerFIFO, seems like fifo file exist, pls delete it");
            exit(errno);
        }

        std::vector<std::string> all_args;
        for(int i = 1; i < argc; i++)
            all_args.push_back(argv[i]);

        std::thread thread_obj(print_thread_f, &all_args); 
        
        if((fd = open(serverFIFO, O_RDONLY)) < 0) {
            perror("can't open fifo");
            exit(errno);
        }
        
        //вектор аргументов
        

        while(1) {
            if((rd = read(fd, buff, SIZE)) < 0) {
                perror("can't read from ServerFIFO");
                exit(errno);
            }
            if(rd > 0) {
                for(int i = 0; i < rd; i++) {
                    if(buff[i] != ' ')
                        all_args.push_back(*(new std::string));
                    //критическая секция
                    m.lock();
                    while(buff[i] != ' ' && i < rd)
                        all_args.back().push_back(buff[i++]);
                    m.unlock();
                    //
                }
            }
            
        }
    }
    //---------------------------------если он существует----------------------------------------
    else {
        //пишем данные в fifo.serv по одному аргументу
        for(int i = 1; i < argc; i++) {
            if(rd = (write(fd, argv[i], strlen(argv[i]))) < 0) {
                perror("can't write in ServerFIFO");
                exit(errno);
            }
            //раздделитель
            if(rd = write(fd, " ", 1) < 0) {
                perror("can't write in ServerFIFO");
                exit(errno);
            }
        }
        std::cout << "ok I'm end";
    }
    
    return 0;
}