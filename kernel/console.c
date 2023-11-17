//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

#define MAX_HISTORY 16

uint ARROW_FLAG = 0;

//
// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  
  // input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

struct {
  char bufferArray[MAX_HISTORY][INPUT_BUF_SIZE];
  uint lengthsArray[MAX_HISTORY];
  uint lastCommandIndex;
  int numOfCommandsInMemory;
  int currentHistory;
} historyBufferArray;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(c);
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

void
history_reset_if_full(){
  if((historyBufferArray.lastCommandIndex) == MAX_HISTORY){
      historyBufferArray.lastCommandIndex = 0;
  }
}

void
history_add(){
  historyBufferArray.currentHistory = -1;
  int char_index = 0;
  history_reset_if_full();
  for(int index = (cons.r);index < ((cons.w) - 1);index++)
      historyBufferArray.bufferArray[historyBufferArray.lastCommandIndex][char_index++] = cons.buf[index];
  historyBufferArray.lengthsArray[historyBufferArray.lastCommandIndex] = char_index;
  historyBufferArray.lastCommandIndex++;
  if ((historyBufferArray.numOfCommandsInMemory) != MAX_HISTORY){
    historyBufferArray.numOfCommandsInMemory++;
  }
}

uint
history_add_condition(){
  char history_command[7] = "history";
  int temp_index = 0;
  for (int index = (cons.r);index < (cons.r) + 7 ;index++){
      if(history_command[temp_index] != cons.buf[index])
          return 1;
      temp_index++;
  }
  return 0;
}

uint
history_get(uint historyarg){
  if (historyarg > historyBufferArray.lastCommandIndex){
    printf("Invalid argument for history funciton!\n");
    return 1;
  }
  for (uint i = 0; i <= historyBufferArray.lastCommandIndex; i++)
  {
    printf("%s\n", historyBufferArray.bufferArray[i]);
  }  
  printf("Your requested function is %s \n", historyBufferArray.bufferArray[historyarg]);
  return 0;
}

void
arrow_command(){
  release(&cons.lock);
  if((historyBufferArray.currentHistory >= 0) && (historyBufferArray.currentHistory <= historyBufferArray.lastCommandIndex)){
      for(int index = 0;index <= INPUT_BUF_SIZE;index++)
          consoleintr(127);
      for(int index = 0;index < historyBufferArray.lengthsArray[historyBufferArray.currentHistory];index++)
          consoleintr(historyBufferArray.bufferArray[historyBufferArray.currentHistory][index]);
  }
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c) {
  acquire(&cons.lock);

  switch (c) {
      case C('P'):  // Print process list.
          procdump();
          break;
      case C('U'):  // Kill line.
          while (cons.e != cons.w &&
                  cons.buf[(cons.e - 1) % INPUT_BUF_SIZE] != '\n') {
              cons.e--;
              consputc(BACKSPACE);
          }
          break;
      case C('H'): // Backspace
      case '\x7f': // Delete key
          if (cons.e != cons.w) {
              cons.e--;
              consputc(BACKSPACE);
          }
          break;
      case 27:
          ARROW_FLAG = 1;
          break;
      default:
          if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE) {
            c = (c == '\r') ? '\n' : c;

            if((c == 65) && (ARROW_FLAG)){
              if(historyBufferArray.currentHistory == -1)
                  historyBufferArray.currentHistory = historyBufferArray.lastCommandIndex;
              else if(historyBufferArray.currentHistory != 0){
                      historyBufferArray.currentHistory--;
              }
              arrow_command();
              ARROW_FLAG = 0;
              return;
            }
            else if((c == 66) && (ARROW_FLAG)){
              if(historyBufferArray.currentHistory == -1){
                historyBufferArray.currentHistory = 0;
              }
              else{
                if(historyBufferArray.currentHistory != historyBufferArray.lastCommandIndex) {
                    historyBufferArray.currentHistory++;
                }
              }
              arrow_command();
              ARROW_FLAG = 0;
              return;
            }

            else if (!((c == 91) && (ARROW_FLAG))){
                // echo back to the user.
                consputc(c);

                // store for consumption by consoleread().
                cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

                if (c == '\n' || c == C('D') || cons.e - cons.r == INPUT_BUF_SIZE) {
                    // wake up consoleread() if a whole line (or end-of-file)
                    // has arrived.
                    cons.w = cons.e;
                    wakeup(&cons.r);
                    if(history_add_condition())
                        history_add();
                }
            }
          }
          break;
  }
  release(&cons.lock);
}


void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
