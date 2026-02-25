#include "judge/code_runner.hpp"
#include "core/temp_file.hpp"
#include "net/blocking_io.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::vector <std::string> code_runner::normalize_output(const std::string& output){
    std::vector <std::string> ret;
    std::string tmp;
    for(char i : output){
        if(i == '\n'){
            while(!tmp.empty() && is_blank(tmp.back())) tmp.pop_back();
            ret.push_back(tmp);
            tmp.clear();
        }
        else{
            tmp.push_back(i);
        }
    }

    if(!output.empty() && output.back() != '\n'){
        while(!tmp.empty() && is_blank(tmp.back())) tmp.pop_back();
        ret.push_back(tmp);
    }

    while(!ret.empty() && ret.back().empty()){
        ret.pop_back();
    }

    return ret;
}

bool code_runner::is_blank(char c){
    return (c == ' ' || c == '\t' || c == '\r');
}

std::expected <std::vector <std::string>, error_code> code_runner::run_cpp(path binary_path, path input_path){
    auto output_temp = temp_file::create("/tmp/oj_output_XXXXXX");
    if(!output_temp){
        return std::unexpected(output_temp.error());
    }

    unique_fd input_fd = unique_fd(open(input_path.c_str(), O_RDONLY));
    if(!input_fd){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    pid_t pid = fork();
    if(pid < 0){
        return std::unexpected(error_code::create(fork_failed));
    }
    
    // child process
    if(pid == 0){
        dup2(input_fd.get(), STDIN_FILENO);
        dup2(output_temp->get_fd(), STDERR_FILENO);
        dup2(output_temp->get_fd(), STDOUT_FILENO);
        input_fd.close();

        std::vector <char*> argv = {
            const_cast<char*>(binary_path.c_str()),
            nullptr
        };

        execv(binary_path.c_str(), argv.data());
        _exit(127);
    }

    // parent process
    input_fd.close();

    int status = 0;
    while(true){
        if(waitpid(pid, &status, 0) == pid) break;
        int ec = errno;
        if(ec == EINTR) continue;
        return std::unexpected(error_code::create(waitpid_failed));
    }

    if(lseek(output_temp->get_fd(), 0, SEEK_SET) < 0){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    auto output_result = blocking_io::read_all(output_temp->get_fd());
    if(!output_result){
        return std::unexpected(output_result.error());
    }

    if(WIFEXITED(status)){
        int exit_code = WEXITSTATUS(status);
        if(exit_code == 0) return normalize_output(*output_result);
        return std::unexpected(error_code::create(runtime_error));
    }
    
    if(WIFSIGNALED(status)){
        return std::unexpected(error_code::create(runtime_error));
    }

    return std::unexpected(error_code::create(waitpid_failed));
}
