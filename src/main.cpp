#include "cmd/run.h"
#include "cmd/send.h"
#include "cmd/server.h"
#include <iostream>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

void help() {
  cout << "attack:\n"
          "usage: attack subcmd [...]\n"
          "  attack server\n"
          "    run a dbus service, provide a method which takes no arguments "
          "and return nothing either, listen on session fbus:\n"
          "      name: test.dbus.service\n"
          "      path: /\n"
          "      interface: test.dbus.interface\n"
          "      method: method\n"
          "  attack send\n"
          "    send a method_call dbus message with no arguments, default "
          "destination is the same as above\n"
          "  attack run binary_name\n"
          "    run this fake proc exe attack demo to pretend as "
          "/usr/bin/binary_name\n";
  return;
}

int main(int argc, char **argv) {
  int ret = -1;
  switch (argc) {
  case 1: {
    cerr << "too few args" << endl;
    help();
    break;
  }
  default: {
    struct subcmd {
      string name;
      int (*subcmd_fn)(char **argv);
    };

    auto subcmds = {
        subcmd{"run", subcmd_run},
        subcmd{"send", subcmd_send},
        subcmd{"server", subcmd_server},
    };

    string subcmd = argv[1];
    bool cmd_found = false;

    for (auto cmd : subcmds) {
      if (cmd.name == subcmd) {
        cmd_found = true;
        ret = cmd.subcmd_fn(argv + 2);
      }
    }

    if (!cmd_found) {
      cerr << "unknow sub command " << subcmd << endl;
    }
  }
  }
  return ret;
}
