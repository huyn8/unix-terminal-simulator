/*
* Authors: Huy Nguyen
* Unix Shell 
* Version 1
* 7/16/2021
*/
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <stdio.h>
#include <fcntl.h>


/*
* Executes command parsed from the user given that the commands are valid
* and in correct formatting
*
* args:
* char* arguments[] : 
* bool skipWait : for determining if parent process needs to wait for child to complete
* bool sleep : for determining parent needs to sleep for child to complete
*
* return: none
*/
void execute (char* arguments[], bool skipWait, bool sleep){
    int child_pid_ = fork();
    //fail
    if(child_pid_ < 0){
        std::cout << "Fork Failed" << std::endl;
        return;
    }
    //child process
    else if(child_pid_ == 0){ 
         if(execvp(arguments[0], arguments) == -1){
            perror("Execvp Error");
        }   
        exit(0);
    }
    //parent process
    else{
        if(!skipWait){
            if(wait(NULL) == -1){
                perror("Wait Error");
            }
        }
        if(sleep){
            if(wait(0) == -1){
                perror("Wait Error");
            }
        }
    }
}

/*
* Redirect command from standard output and input to a file or from 
* file respectively. Given that correct commands and formatting still hold
*
* args:
* char* arguments[]: the commands to be excuted
* char* file[] : file name
* int mode: 1 for output redirection and 0 for input redirection
*
* return: none
*/
void redirect(char* arguments[], char* file[], int mode){
    int child_pid_ = fork();
    int redirect_ = 0;

    //fail
    if(child_pid_ < 0){
        std::cout << "Fork Failed" << std::endl;
        return;
    }
    //child process
    else if(child_pid_ == 0){
        if(mode == 1){//stdout redirection
            redirect_ = open(file[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        else if (mode == 0){//stdin redirection
            redirect_ = open(file[1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        if(redirect_ < 0){
            perror("Redirection Error");
        }
        else {
            dup2(redirect_, mode);
            close(redirect_);

            //error checking
            arguments[1] = NULL;
            if (execvp(arguments[0], arguments) == -1) {
                perror("Execvp Error");
            }
            exit(1);
        }
    }
    //parent process
    else {
        waitpid(child_pid_, 0, 2);
    }
}

/*
* Creates a pipe to pass data between two processes given that correct commands and formatting still hold
*
* args:
* char* left[]: left side of the pipe (first process)
* char* right[] : right side of the pipe (second process)
* int moreArgs: in special cases there will be more arguments on the right side of the pipe, 
* (like redirection), if this is true, run redirect(...) for addtional processing
*
* return: none
*/
void openPipe(char* left[], char* right[], bool moreArgs){
    int fd_[2];

    if(pipe(fd_) == -1){ 
        perror("Pipe Error");
        return;
    }

    //first process on one end of the pipe
    int child_pid1_ = fork();
    if(child_pid1_ < 0){
        std::cout << "Fork Failed" << std::endl;
        return;
    }
    else if(child_pid1_ == 0){
        close(fd_[0]);//read end of first process (unused)
        dup2(fd_[1],1);   
        close(fd_[1]);//write end of first process (closed after used) 

        if(execvp(left[0], left) == -1){
            perror("Execvp 1 Error on Pipe 1");            
        }
        exit(1);
    }

    //second process on  the other end of the pipe
    int child_pid2_ = fork();
    if(child_pid2_ < 0){
        std::cout << "Fork Failed" << std::endl;
        return;
    }
    else if (child_pid2_ == 0)  
    {
        close(fd_[1]); //write end of second process (unused)
        dup2(fd_[0],0);   
        close(fd_[0]); //read end of second process (closed after used) 
       
        //if more arguements are present in the piping process
        if (moreArgs){
            redirect(right, right, 1);
        }

        if(execvp(right[0], right) == -1){
            perror("Execvp 2 Error on Pipe 2");            
        }
        exit(1);
    }

    close(fd_[0]);
    close(fd_[1]);

    waitpid(child_pid1_, 0, 2);   
    waitpid(child_pid2_, 0, 2);
}

/*
* This main method will parse, transform and process user input commands
*
* args: (the below parameters are not actually used)
* none 
*
* return: a random integer
*/
//int mainImpl(int argc, const char* argv[]) //for compiling via make file
int main(int argc, const char* argv[]){// int argc, const char* argv[] in mainImpl are not actually used (for compiling via g++)
    int should_run_ = 1;

    std::cout << "Enter a command or type exit to terminate" << std::endl;
    while(should_run_ == 1){
        std::cout << "osh> "; //prompting for user input
        std::cout.flush(); //flusing for fun
        std::string user_command_ = "";  //variable for storing user command 
        std::getline(std::cin, user_command_); //get user input and storing it in user_command_
        std::vector<std::vector<std::string>> commands_;  //vector to store tokenized commands
        std::vector<std::string> temp_; // for temporary commands processing
        std::istringstream iss_(user_command_); //string stream object for spliting up the command
        std::string command_; //temp string var to store individual command
        bool is_amp_ = false; //for signaling (in later processing)
        bool sleep_ = false; //for signaling (in later processing)

        //parse user input and store in 2d vector commands_
        bool nulled_ = false;
        while (true){
            if(iss_ >> command_){
                if(command_ == "&" || command_ == ";" || command_ == "|"){
                    temp_.push_back(command_);
                    temp_.push_back("NULL");
                    commands_.push_back(temp_);
                    temp_.clear();
                    nulled_ = true;
                    continue;
                }
                else{
                    temp_.push_back(command_);
                    nulled_ = false;
                }
            }
            else{
                if(!nulled_){
                    temp_.push_back("NULL");
                    commands_.push_back(temp_);
                    break;
                }
                else{
                    break;
                }
            }
        }

        //put commands from 2d vector commands_ to 2d char* [][] to match execvp arguments requirement
        //non-command characters will now be excluded
        char* args_[commands_.size()][10] = {NULL};
        int i2 = 0;
        int j2 = 0;
        for(int i = 0; i < commands_.size(); i++){//row
            for(int j = 0; j < commands_[i].size(); j++){//column
                if(commands_[i][j] == "&" || commands_[i][j] == ";" ){
                    args_[i2][j2] = NULL;
                    i2++;
                    j2 = 0;
                    break;
                }
                else if(commands_[i][j] == ">" || commands_[i][j] == "<"){
                    continue;
                }
                else if(commands_[i][j] == "|"){
                    args_[i2][j2] = NULL;
                    i2++;
                    j2 = 0;
                    break;
                }
                else{
                    if(commands_[i][j] != "NULL"){
                        args_[i2][j2] = (char*)commands_[i][j].c_str();
                        j2++;
                    }
                    else{
                        args_[i2][j2] = NULL;
                        i2++;
                        j2 = 0;
                    }
                }
            }
        }

        //actual commands processing with many different cases 
        for(int r = 0; r < commands_.size(); r++){
            bool executed_ = false;
            for(int c = 0; c < commands_[r].size(); c++){
                if(commands_[r][c] == "ascii"){
                    std::cout << "  |\\_/|        ****************************     (\\_/)\n";
                    std::cout << " / @ @ \\       *  \"Purrrfectly pleasant\"  *    (='.'=)\n";
                    std::cout << "( > º < )      *       Poppy Prinz        *    (\")_(\")\n";
                    std::cout << " `>>x<<´       *   (pprinz@example.com)   *\n";
                    std::cout << " /  O  \\       ****************************\n";
                    executed_ = true;
                }
                else if (commands_[r][c] == "exit"){
                    should_run_ = 0;
                    executed_ = true;
                    std::cout << "Shell terminated, have a good day!" << std::endl;
                }
                else if(commands_[r][c] == "&"){
                    is_amp_ = true; 
                    execute(args_[r], is_amp_, sleep_);
                    is_amp_ = false;
                    sleep_ = true;
                    executed_ = true;
                    break;
                } 
                else if(commands_[r][c] == ";"){
                    execute(args_[r], is_amp_, sleep_);
                    executed_ = true;
                    break;
                }
                else if(commands_[r][c] == ">"){
                    redirect(args_[r], args_[r], 1);
                    executed_ = true;
                }
                else if(commands_[r][c] == "<"){
                    redirect(args_[r], args_[r], 0);
                    executed_ = true;
                }
                else if(commands_[r][c] == "|"){
                    if(commands_.size() <= 2){
                        openPipe(args_[r], args_[r+1], false);
                        r++;
                        executed_ = true;
                        break;
                    }
                    else if(commands_.size() > 2){
                        openPipe(args_[r], args_[r+1], true);
                        r+=2;
                        executed_ = true;
                        break;
                    }
                }
            }
            if(!executed_){
                execute(args_[r], false, false);
            }
        }
    }  
    return 0;
}
