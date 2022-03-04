#include "send.h"
#include <dbus-1.0/dbus/dbus.h>
#include <fmt/core.h>
#include <iostream>

using fmt::format;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

int subcmd_send(__attribute__((unused)) char **argv) {
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

  DBusMessage *msg;
  msg = dbus_message_new_method_call("test.dbus.service", "/",
                                     "test.dbus.interface", "method");
  if (NULL == msg) {
    cerr << "method_call message is null" << endl;
    exit(1);
  }

  cout << "dbus call start" << endl;

  // send message and get a handle for a reply
  DBusPendingCall *pending;
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) { // -1 is default timeout
    cerr << "out of mem" << endl;
    exit(1);
  }
  if (NULL == pending) {
    cerr << "pending call is null" << endl;
    exit(1);
  }

  dbus_connection_flush(conn);

  // free message
  dbus_message_unref(msg);

  // block until we receive a reply
  dbus_pending_call_block(pending);

  cout << "dbus call end" << endl;

  // free the pending message handle
  dbus_pending_call_unref(pending);
  return 0;
}
