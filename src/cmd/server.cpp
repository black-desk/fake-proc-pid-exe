#include "server.h"
#include <dbus-1.0/dbus/dbus.h>
#include <fmt/core.h>
#include <iostream>
#include <linux/limits.h>
#include <unistd.h>

using fmt::format;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

string get_exe(DBusConnection *conn, string sender) {
  // ref to http://www.matthew.ath.cx/articles/dbus

  DBusMessage *msg;
  msg = dbus_message_new_method_call("org.freedesktop.DBus", "/",
                                     "org.freedesktop.DBus",
                                     "GetConnectionUnixProcessID");
  if (NULL == msg) {
    cerr << "method_call message is null" << endl;
    exit(1);
  }

  DBusMessageIter args;

  auto name = strdup(sender.c_str());
  // append arguments
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &name)) {
    fprintf(stderr, "Out Of Memory!\n");
    free(name);
    exit(1);
  }

  DBusPendingCall *pending;

  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) { // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  // free message
  dbus_message_unref(msg);
  free(name);

  // block until we receive a reply
  dbus_pending_call_block(pending);

  // get the reply message
  msg = dbus_pending_call_steal_reply(pending);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  uint32_t pid;
  // read the parameters
  if (!dbus_message_iter_init(msg, &args))
    fprintf(stderr, "Message has no arguments!\n");
  else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args))
    fprintf(stderr, "Argument is not uint32!\n");
  else
    dbus_message_iter_get_basic(&args, &pid);
  char buf[PATH_MAX + 1];
  readlink(format("/proc/{}/exe", pid).c_str(), buf, PATH_MAX);
  return string(buf);
}

void reply_to_method_call(DBusMessage *msg, DBusConnection *conn) {
  dbus_uint32_t serial = 0;

  string sender(dbus_message_get_sender(msg));

  cout << format("server: get caller exe=\"{}\"", get_exe(conn, sender))
       << endl;

  // create a reply from the message
  DBusMessage *reply = dbus_message_new_method_return(msg);

  // send the reply && flush the connection
  if (!dbus_connection_send(conn, reply, &serial)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  // free the reply
  dbus_message_unref(reply);
  exit(0);
}

int subcmd_server(__attribute__((unused)) char **argv) {
  // ref to http://www.matthew.ath.cx/articles/dbus
  DBusError err;
  // initialise the errors
  dbus_error_init(&err);

  // connect to the bus
  DBusConnection *conn;
  conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err)) {
    cerr << format("Connection Error ({})", err.message) << endl;
    dbus_error_free(&err);
  }

  if (NULL == conn) {
    exit(1);
  }

  int ret = dbus_bus_request_name(conn, "test.dbus.service",
                                  DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Name Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
    exit(1);
  }

  DBusMessage *msg;
  // loop, testing for new messages
  while (true) {
    // non blocking read of the next available message
    dbus_connection_read_write(conn, 0);
    msg = dbus_connection_pop_message(conn);

    // loop again if we haven't got a message
    if (NULL == msg) {
      sleep(1);
      continue;
    }

    // check this is a method call for the right interface and method
    if (dbus_message_is_method_call(msg, "test.dbus.interface", "method"))
      reply_to_method_call(msg, conn);

    // free the message
    dbus_message_unref(msg);
  }
}
