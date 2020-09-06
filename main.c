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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

const char *ENV_NVIM_ADDR="NVIM_LISTEN_ADDRESS";
enum msg_type{ request, response, notification };
enum pack_type{ fixintp = 0x00, fixarray=0x90, fixstr=0xa0, str8 = 0xd9 };

void fail_error(char *msg) {
  fprintf(stderr, "error: %s (%s)\n", msg, strerror(errno));
  exit(1);
}

void fallback(char **argv) {
  execv(argv[0], argv);
  // Getting here means exec failed
  fail_error("exec failed");
}

int is_unix_socket(char *file) {
  struct stat fs;
  stat(file, &fs);
  return S_ISSOCK(fs.st_mode);
}

// for debugging
void dump(void *buf, int len) {
  for (int i = 0; i < len; i++) {
    printf("%x ", ((unsigned char*) buf)[i]);
  }
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
  if (argc<2) {
    printf("executable path missing");
    exit(1);
  }

  // Drop executable from arglist
  argc--;
  argv++;

  // Check env for NVIM_LISTEN_ADDRESS, if we are in a nvim
  // session it will be pointing to a RPC unix socket.
  char *sock_name = getenv(ENV_NVIM_ADDR);
  if (NULL == sock_name || !is_unix_socket(sock_name)) fallback(argv);

  // Create unix socket and connect to named socket
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) fail_error("failed creating socket");

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_name, sizeof(addr.sun_path) - 1);

  int ret = connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
  if (ret == -1) fail_error("nvim not responding");

  // Build rpc message in temporary buffer
  uint8_t buf[256];
  char command[128];
  snprintf((char*) &command, sizeof(command), "split %s", argv[1]);
  int len = craft_rpc(buf, command);
  write(sock, buf, len);

  exit(0);
}
