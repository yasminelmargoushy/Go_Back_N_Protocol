#include "packet.hpp"

int main() {
  int drop_prob;
  drop_prob = 20;
  int in, out;
  char *type;
  mkfifo(clin, 0777);
  mkfifo(crout, 0777);
  mkfifo(clout, 0777);
  mkfifo(crin, 0777);

  if ( fork() ) {
    in = open(clin, O_RDONLY) ;
    out = open(crout, O_WRONLY);
    type = "[DATA]";
  } else {
    in = open(crin, O_RDONLY);
    out = open(clout, O_WRONLY);
    type = "[ACK]";
  }

  Frame f;

  while ( true ) {
    int ret = read(in, &f, sizeof(f));
    if ( ret == 0 ) {
      break;
    } else if ( ret < 0 ) {
      perror(type);
      exit(-1);
    }

    printf("Received %s Frame Seq no = %d.\n", type, f.seq);

    if ( (random() % 100 < drop_prob) ) {
      f.valid = false;
    }
    write(out, &f, sizeof(f));
    if (f.valid)
    {
      printf("Transmitted %s Frame. Seq no = %d.\n", type, f.seq);
    }
    else
    {
      printf("[X] Corrupted %s Frame. Seq no = %d.\n", type, f.seq);
    }
  }
}
