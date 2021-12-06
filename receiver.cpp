#include "packet.hpp"

// Configurations
#define DATALINK_BUFFER_SIZE WINDOW_SIZE
#define PROBABILITY 10

// Global Variables
int read_end, write_end;
uint32_t Expected_Data_Seq = 0;
Frame Received_Frame[100];
uint8_t current = 0;
uint8_t Next_Ack = 0;

/************************ PROTOTYPES ************************/
void To_Network_Layer(Packet p);
void send_Ack_packet(uint8_t Sequ, bool last);
bool get_data_Packet(void);
void To_Physical_Layer(Frame f);
int8_t From_Physical_Layer(Frame * f);

// Macros
#define inc(var)  var = ((var + 1) % DATALINK_BUFFER_SIZE)

/************************ NETWORK LAYER ************************/
void To_Network_Layer(Packet p)
{
	ofstream out("Receiver_OUTPUT.txt",fstream::app);
  out << "[NETWORK-LAYER] " << p.data << endl;
  out.close();
}

/************************ DATALINK LAYER ************************/
void send_Ack_packet(uint8_t Sequ, bool last)
 {
  // Arrange Frame
  Frame f;
  f.fr_type = ack;
  f.seq = Sequ;
  f.valid = true;
  if (last)
  {
    f.last = true;
  }
  else
  {
    f.last = false;
  }
  
  // Send to Physical Layer 
  To_Physical_Layer(f);

  // Print Log
  printf("Sent Ack packet %d\n", Sequ);
  if (last)
  {
    exit(EXIT_SUCCESS);
  }
}

bool get_data_Packet(void)
{
  Frame f;
  // Read from Physical Layer
  int8_t Retint = From_Physical_Layer(&f);
  bool Ret = false;

  if ((Retint != -1) && (f.seq == Expected_Data_Seq) && (f.valid))
  {
    printf("Receive Data packet %d whose Data is: %c\n", Expected_Data_Seq, f.info.data);
    inc(Expected_Data_Seq);
    To_Network_Layer (f.info);
    Received_Frame[current] = f;
    ++current;
    Ret = true;
  }
  else if ((Retint != -1) && (f.seq == Expected_Data_Seq) && (!f.valid))
  {
    printf("Discarded Corrupted Data packet %d\n", Expected_Data_Seq);
    Ret = true;
  }
  else if ((Retint != -1) && (f.seq != Expected_Data_Seq) && (f.seq != WINDOW_SIZE))
  {
    printf("Discarded Data packet %d whose Data is: %c\n", f.seq, f.info.data);
    Ret = true;
  }
  return Ret;
}

/************************ PHYSICAL LAYER ************************/
void To_Physical_Layer(Frame f)
{
  write(write_end, &f, sizeof(f));
}
int8_t From_Physical_Layer(Frame * f)
{
  int8_t Ret = read(read_end, f, sizeof((*f)));
  return Ret;
}

int main() 
{
  write_end = open(crin, O_WRONLY);
  read_end = open(crout, O_RDONLY);
  ofstream out("Receiver_OUTPUT.txt");
  out << "{Recieved Packets}" << endl;
  out.close();
  srand(time(0));
  while(true)
  {
    if(get_data_Packet())
    {
      if(current > Next_Ack)
      {
        if ((rand()%100 > PROBABILITY))
        {
          uint8_t sequ = Received_Frame[Next_Ack].seq;
          bool last = Received_Frame[Next_Ack].last;
          send_Ack_packet(sequ, last);
          ++Next_Ack;
        }
      }
      if(Received_Frame[Next_Ack].last)
      {
          uint8_t sequ = Received_Frame[Next_Ack].seq;
          bool last = Received_Frame[Next_Ack].last;
          send_Ack_packet(sequ, last);
          exit(EXIT_SUCCESS);
      }
    }
  }
}