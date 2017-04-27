#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <vector>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <array>


using namespace std;

void enforce_background();
void sig_handler(int);
void set_sig_handler(int, sighandler_t);
void close_pipe(int pipefd [2]);
vector<char *> mk_cstrvec(vector<string> & strvec);
void dl_cstrvec(vector<char *> & cstrvec);
void nice_exec(vector<string> args);
void help();
void s_help(string);
inline void nope_out(const string & sc_name); 
vector<vector<string>> mk_vvs(vector<string> commands);
void pipeCommands(vector<vector<string>> commands, int pipenum, vector<string> iofiles, bool ioredir);
void status_change(int *, pid_t);
void ioredirect(vector<string> iofiles);

int  status = 0;

int main() {

  cout.setf(ios_base::unitbuf);

  signal(SIGINT,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);
  signal(SIGCHLD,SIG_IGN);

  string input;  

  while(true) {
    char * cwd;
    char * home;

    //    string stdin = "STDIN_FILENO";
    //string stdout = "STDOUT_FILENO";
    //string stderr = "STDERR_FILENO";
    
    stringstream ss;
    string ssInput;
 
    bool ioredir = false;
    bool invalidarg = false;    
    bool bgjobs = false;

    vector<string> argv;
    vector<string>::iterator it;

    vector<string> iofiles;

    int pipenum = 0;
    // int argnum = 0;

    home = getenv("HOME");
    
    cout << "1730sh:";
    if((cwd = get_current_dir_name()) != NULL) {
      if(strlen(cwd) > strlen(home)) {
	cout << "~";
	//	cout << cwd;
	int dif = (strlen(cwd) - strlen(home));
	for(int i=(strlen(cwd)-dif);i<(int)strlen(cwd);i++) {
	    cout << cwd[i];
	}
      }//if
      else {
	cout << "~";
      }
      cout << "/$ ";
      free(cwd);
    }//if
    else {
      perror("cwd");
      return EXIT_FAILURE;
    }
    
    getline(cin,input);
    ss << input;
    
    while(ss >> ssInput) {
      if(ssInput == "&") {
	bgjobs = true;
	if(ss >> ssInput) {
	  cout << "syntax error near unexpected token `" << ssInput << "'" << endl;
	  invalidarg = true;
	}
      }
      if(ssInput == "<") {
	ioredir = true;
	iofiles.push_back(ssInput);
	if(ss >> ssInput) {
	  iofiles.push_back(ssInput);
	} else {
	  cout << "syntax error near unexpected token `newline'" << endl;
	  invalidarg = true;
	}
	continue;
      }
      if(ssInput == ">") {
	ioredir = true;
	iofiles.push_back(ssInput);
	if(ss >> ssInput) {
	  iofiles.push_back(ssInput);
	} else {
	  cout << "syntax error near unexpected token `newline'" << endl;
	  invalidarg = true;
	}
	continue;
      }
      if(ssInput == ">>") {
        ioredir = true;
	iofiles.push_back(ssInput);
	if(ss >> ssInput) {
	  iofiles.push_back(ssInput);
	} else {
	  cout << "syntax error near unexpected token `newline'" << endl;
	  invalidarg = true;
	}
	continue;
      }
      if(ssInput == "e>") {
	ioredir = true;
	iofiles.push_back(ssInput);
	if(ss >> ssInput) {
	  iofiles.push_back(ssInput);
	} else {
	  cout << "syntax error near unexpected token `newline'" << endl;
	  invalidarg = true;
	}
	continue;
      }
      if(ssInput == "e>>") {
	ioredir = true;
	iofiles.push_back(ssInput);
	if(ss >> ssInput) {
	  iofiles.push_back(ssInput);
	} else {
	  cout << "syntax error near unexpected token `newline'" << endl;
	  invalidarg = true;
	}
	continue;
      }
      if(ssInput == "|") pipenum++;
      argv.push_back(ssInput);
    }//while

    if(argv.size() == 0) continue;
    //prints error msg when io redirection is used but no file is stated
    if(invalidarg) {
      continue;
    }        
    it = argv.begin(); //it is set to beginning of argv

    //need to fix exit, doesn't handle signals and upon no N specified, must get last process exit status
    if(*it == "exit") {
      if(argv.size() == 1) {
	exit(status);
      }
      else {
	it++;
	exit(stoi(*it));
      }
    } 
    else if(*it == "cd") {
      if(argv.size() != 1) {
	it++;
	if(*it == "~") {
	  if(chdir(home) == -1) perror("cd");
	} 
	else {
	  if(chdir((*it).c_str()) == -1) perror("cd");
	}
      }
      else {
	if(chdir(home) == -1) perror("cd");
      }
    }//else if
    else if(*it == "help") {
      if(argv.size() != 1) {
	it++;
	s_help(*it);
	} else {
	help();
      }
    }
    else if(*it == "|") {
      cout << "syntax error near unexpected token `|'" << endl;
    }
    else if(bgjobs) {
      
    }
    else {
	vector<vector<string>> commands = mk_vvs(argv);
	pipeCommands(commands,pipenum,iofiles,ioredir);
    }
    
  }//while
}

void close_pipe(int pipefd[2]) {
  if (close(pipefd[0]) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  } // if
  if (close(pipefd[1]) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  } // if
}

vector<char *> mk_cstrvec(vector<string> & strvec) {
  vector<char *> cstrvec;
  for (unsigned int i = 0; i < strvec.size(); ++i) {
    cstrvec.push_back(new char [strvec.at(i).size() + 1]);
    strcpy(cstrvec.at(i), strvec.at(i).c_str());
  } // for
  cstrvec.push_back(nullptr);
  return cstrvec;
} // mk_cstrvec

void dl_cstrvec(vector<char *> & cstrvec) {
  for (unsigned int i = 0; i < cstrvec.size(); ++i) {
    delete[] cstrvec.at(i);
  } // for
}

void nice_exec(vector<string> strargs) {
  vector<char *> cstrargs = mk_cstrvec(strargs);
  execvp(cstrargs.at(0), &cstrargs.at(0));
  perror("execvp");
  dl_cstrvec(cstrargs);
  exit(EXIT_FAILURE);
} // nice_exec

void help() {
  cout << "bg JID - Resume the stopped job JID in the background, as if it had been started with&." << endl;
  cout << "cd [PATH] - Change the current directory to PATH. The environmental variable HOME is the default PATH." << endl;
  cout << "exit [N] - Cause the shell to exit with a status of N. If N is omitted, the exit status is that of the last job executed." << endl;
  cout << "export NAME[=WORD] - NAME is automatically included in the environment of subsequently executed jobs" << endl;
  cout << "fg JID - Resume job JID in the foreground, and make it the current job." << endl;
  cout << "help - Display helpful information about builtin commands." << endl;
  cout << "jobs - List current jobs." << endl;  
}

void s_help(string opt) {
  if(opt == "cd") {
    cout << "cd [PATH] - Change the current directory to PATH. The environmental variable HOME is the default PATH." << endl;
  } else if(opt == "exit") {
    cout << "exit [N] - Cause the shell to exit with a status of N. If N is omitted, the exit status is that of the last job executed." << endl;
  } else if(opt == "help") {
    cout << "help - Display helpful information about builtin commands." << endl;
  } else if(opt == "bg") {
    cout << "bg JID - Resume the stopped job JID in the background, as if it had been started with&." << endl;
  } else if(opt == "export") {
    cout << "export NAME[=WORD] - NAME is automatically included in the environment of subsequently executed jobs" << endl;
  } else if(opt == "fg") {
    cout << "fg JID - Resume job JID in the foreground, and make it the current job." << endl;
  } else if(opt == "jobs") {
    cout << "jobs - List current jobs." << endl;
  } else {
    cout << "help: no help topics match `" << opt << "'.  Try `help help'" << endl;
  }
}

inline void nope_out(const string & sc_name) {
  perror(sc_name.c_str());
  exit(EXIT_FAILURE);
} // nope_out

vector<vector<string>> mk_vvs(vector<string> commands) {
  vector<vector<string>> vvs;
  
  vector<string>::iterator it;
  vector<string> temp;

  it = commands.begin();
  for(;it<commands.end();it++) {
    if(*it == "|") {
      vvs.push_back(temp);
      temp.clear();
    } else {
    temp.push_back(*it);
    }
  }

  vvs.push_back(temp);
  return vvs;
}

void pipeCommands(vector<vector<string>> commands, int pipenum, vector<string> iofiles, bool ioredir) {
  pid_t pid;//,wpid;

  vector<array<int, 2>> pipes;

  for (unsigned int i = 0; i < commands.size(); ++i) {

    if (i != commands.size() - 1) {
      int pipefd [2];
      if (pipe(pipefd) == -1) {
	perror("pipe");
	exit(EXIT_FAILURE);
      } // if
      pipes.push_back({pipefd[0], pipefd[1]});
    } // if

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if ((pid = fork()) == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {

      if (i != 0) {
	if (dup2(pipes.at(i-1)[0], STDIN_FILENO) == -1) {
	  perror("dup2");
	  exit(EXIT_FAILURE);
	} // if
      } // if

      if (i != commands.size() - 1) {
	if (dup2(pipes.at(i)[1], STDOUT_FILENO) == -1) {
	  perror("dup2");
	  exit(EXIT_FAILURE);
	} // if
      } // if
      
      // close all pipes created so far
      for (unsigned int i = 0; i < pipes.size(); ++i) {
	close_pipe(pipes.at(i).data());
      } // for
      
      if(ioredir && (i==commands.size()-1)) {
	ioredirect(iofiles);
      } 
      nice_exec(commands.at(i));
      
    } // if

  } // for

  // close all pipes
  for (unsigned int i = 0; i < pipes.size(); ++i) {
    close_pipe(pipes.at(i).data());
  } // for
  // wpid = waitpid(-1,&status,0);
  //waitpid(pid,&status, WUNTRACED | WCONTINUED | WNOHANG);
  waitpid(pid,&status, 0);
  cout << "pid: " << pid << endl;
  cout << "status: "<< status << endl;
  status_change(&status, pid);
}

void status_change(int * status, pid_t pid) {
  //while((wpid = waitpid(pid,status, WUNTRACED | WCONTINUED | WNOHANG)) > 0) {
    // while((pid = waitpid(-1,status, 0)) > 0) {
    if(WIFEXITED(status)) {
      //      int exitStatus = WEXITSTATUS(status);
      cout << endl << getpgid(pid) << " Exited (" << WEXITSTATUS(status) << ")\t" << endl;
      cout << strerror(errno) << endl;
    }
    if(WIFSIGNALED(status)) {
      int signal = WTERMSIG(status);
      cout << endl << getpgid(pid) << " Exited (" << strsignal(signal) << ")\t" << endl;
    }
    if(WIFSTOPPED(status)) {
      //    int stopped = WSTOPSIG(status);
      cout << endl << getpgid(pid) << "Stopped " << "\t" << endl;
      // kill(pid, SIGCONT);
    }
    if(WIFCONTINUED(status)) {
      cout << endl << getpgid(pid) << " Continued " << "\t" << endl;
    }
    //}
}

void enforce_background() {
  int pgid;
  if ((pgid = tcgetpgrp(STDOUT_FILENO)) == getpgrp()) {
    cerr << "need to be in background" << endl;
    exit(EXIT_FAILURE);
  } else if (pgid == -1) {
    perror("tcgetpgrp");
    exit(EXIT_FAILURE);
  } // if
} // enforce_background

void sig_handler(int signo) {
  cout << "Caught signal " 
       << signo << " (" << strsignal(signo) << ") "
       << "[" << "use \"jobs -l\" to inspect" << "]"
       << endl;
  // if (signo == SIGCONT) kill(getpid(), SIGSTOP);
} // sig_handler

// Why not just use the signal function? (see signal(2))
void set_sig_handler(int signo, sighandler_t handler) {
  struct sigaction sa;     // sigaction struct object
  sa.sa_handler = handler; // set disposition
  if (sigaction(signo, &sa, nullptr) == -1) perror("sigaction");
} // setup_sig_handler

void ioredirect(vector<string> iofiles) {
  int fd;

 vector<string>::iterator it;
  it = iofiles.begin();
 
  for(;it<iofiles.end();it++) {
    if(*it == "<") {
      it++;
      if((fd = open((*it).c_str(), O_RDONLY)) == -1) {
	//perror((*it).c_str());
	//continue;
	nope_out((*it).c_str());
      }
      dup2(fd,STDIN_FILENO);
      close(fd);
    } else if(*it == ">") {
      it++;
      if((fd = open((*it).c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0777)) == -1) {
	//nope_out("open");
	perror((*it).c_str());
	continue;
      }
      dup2(fd,STDOUT_FILENO);
      close(fd);
    } else if(*it == ">>") {
      it++;
      if((fd = open((*it).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0777)) == -1) {
	perror("open");
	continue;
	//nope_out((*it).c_str());
      }
      dup2(fd,STDOUT_FILENO);
      close(fd);
    } else if(*it == "e>") {
      it++;
      if((fd = open((*it).c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0777)) == -1) {
	perror("open");
	continue;
	//nope_out((*it).c_str());
      }
      dup2(fd,STDERR_FILENO);
      close(fd);
    } else if(*it == "e>>") {
      it++;
      if((fd = open((*it).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0777)) == -1) {
	//perror("open");
	//continue;
	nope_out((*it).c_str());
      }
      dup2(fd,STDERR_FILENO);
      close(fd);
    }    
  }
}
