#include "syscalls.h"
#include "errno.h"

int errno;

#if CONFIG_LIBC_IO_GDB == 1

int __trap34 (int sysno,int arg1,int arg2,int arg3);

/* File IO uses trap instructions into GDB stub */
int
open ( char *name, int flag, int mode)
{
 return __trap34 (__NR_open, (int)name, flag, mode);
}

int
close ( int fd )
{
 return __trap34 (__NR_close, fd, 0, 0);
}

int
read ( int fd, void *ptr, int len)
{
  return __trap34 (__NR_read, fd, (int)ptr, len);
}

int
write ( int fd, void *ptr, int len)
{
  return __trap34 (__NR_write, fd, (int)ptr, len);
}

int
lseek ( int fd, long offset, int flag)
{
  return __trap34 (__NR_lseek, fd, offset, flag);
}

#elif (CONFIG_LIBC_IO_UART0 == 1 || CONFIG_LIBC_IO_UART1 == 1)

#if CONFIG_LIBC_IO_UART0 == 1
#define UART_NUM 0
#else
#define UART_NUM 1
#endif

/* File IO uses uarts directly */
int
open ( char *name, int flag, int mode)
{
  errno = ENOENT;
  return -1;
}

int
close ( int fd )
{
  errno = ENOENT;
  return -1;
}

int
read ( int fd, void *ptr, int len)
{
  if (fd == 0) {
    /* stdin */
    errno = EINVAL;
    return -1;
  } else {
    errno = EINVAL;
    return -1;
  }
}

void uart_tx (int dev, unsigned char data);

int
write ( int fd, void *ptr, int len)
{
  int i;
  unsigned char c;
  if (fd == 1)
    {
      /* stdout */
      for (i = 0; i < len; i++)
        {
          c = ((unsigned char*)ptr)[i];
          if (c == '\n')
            uart_tx(UART_NUM, '\r');
          uart_tx(UART_NUM, c);
        }
      return i;
    }
  else
    {
      errno = EINVAL;
      return -1;
    }
}

int
lseek ( int fd, long offset, int flag)
{
  errno = ENOENT;
  return -1;
}

#endif

char
inch ()
{
  char ret = 0;
  read(0, &ret, 1);
  return ret;
}

int
outch(char ch)
{
  return write(1, &ch, 1);
}
