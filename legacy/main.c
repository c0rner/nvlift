// MIT License
//
// Copyright (c) 2020 Martin Akesson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// Compile with:
// gcc -Wall -O2 main.c -o nvhoist && strip nvhoist

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

// Macro returning the number of elements in an array (length)
#define len(x) (sizeof(x) / sizeof(x[0]))

const char *ENV_NVIM_ADDR="NVIM_LISTEN_ADDRESS";
const char *NVIM_EXECUTABLES[]={ "nvim", "nvim.appimage" };

enum msg_type{ request, response, notification };
enum pack_type{ fixintp = 0x00, fixarray=0x90, fixstr=0xa0, str8 = 0xd9 };
typedef enum { false = 0, true } bool;

void fail_error(char *msg) {
  fprintf(stderr, "error: %s (%s)\n", msg, strerror(errno));
  exit(1);
}

bool is_unix_socket(char *file) {
  struct stat fs;
  if (file == NULL) return false;
  stat(file, &fs);
  return S_ISSOCK(fs.st_mode);
}

// Emit msgpack array of `size` onto `buf`
// NOTE: Only suports array size < 16 (fixarray)
uint8_t* mp_array(uint8_t *buf, int size) {
  if (size > 15) size = 15;
  buf[0] = fixarray + size;
  buf += 1;

  return buf;
}

// Emit msgpack integer onto `buf`
// NOTE: Only supports 7bit unsigned int (positive fixint)
uint8_t* mp_int(uint8_t *buf, int i) {
  if (i > 0x7f) i = 0x7f;
  buf[0] = fixintp + i;
  buf += 1;

  return buf;
}

// Emit msgpack string onto `buf`
// NOTE: Only supports string length < 256 (str8)
uint8_t* mp_string(uint8_t *buf, char *str) {
  int len = strlen(str);
  if (len < 32) { // fixstr
    buf[0] = fixstr + len;
    buf += 1;
  } else { // str8
    if (len > 255) len=255;
    buf[0] = str8;
    buf[1] = len;
    buf += 2;
  }
  memcpy(&buf[0], str, len);
  buf += len;

  return buf;
}

// Craft nvim rpc notification  message 
int craft_rpc(uint8_t *buf, char *cmd) {
  uint8_t *cur = buf;

  cur = mp_array(cur, 3);               // Array with 3 objects
  cur = mp_int(cur, notification);      // Obj.1 - Message type
  cur = mp_string(cur, "nvim_command"); // Obj.2 - API request
  cur = mp_array(cur, 1);               // Obj.3 - Function args
  cur = mp_string(cur, cmd);

  return cur - buf;
}

int main(int argc, char **argv) {
  char *opt_exec = NULL;
  bool opt_nested = false;
  char c;

  // Parse command line options
  while ((c = getopt (argc, argv, "e:")) != -1)
    switch (c) {
      case 'e':
        opt_exec = optarg;
        break;
    }

  argc -= optind;
  argv = &argv[optind];

  // NVIM will set environment variable `NVIM_LISTEN_ADDRESS` on start
  // verify it is a unix socket and set nested to true
  char *socket_path = getenv(ENV_NVIM_ADDR);
  if (is_unix_socket(socket_path)) opt_nested=true;

  // If running as a wrapper (ie. not nested) try any specified executable first
  if (!opt_nested) {
    if (opt_exec == NULL) {
      // Launched in wrapper mode with no ececutable set, do some guesswork
      // and try a set of wellknown executable names for Nvim
      for (int i = 0; i < len(NVIM_EXECUTABLES); i++) {
        execvp(NVIM_EXECUTABLES[i], argv);
      }
      fail_error("missing nvim executable");
    }

    // Arguments supplied will be passed on to the real Nvim application
    execvp(opt_exec, argv);
    fail_error("exec failed");
  }

  // Create unix socket and connect to named socket
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) fail_error("failed creating socket");

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  int ret = connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
  if (ret == -1) fail_error("nvim not responding");

  // Build rpc message in temporary buffer and send to Nvim
  uint8_t buf[512];
  char command[256];
  snprintf((char*) &command, sizeof(command), "split %s", argv[0]);
  int len = craft_rpc(buf, command);
  write(sock, buf, len);

  exit(0);
}
