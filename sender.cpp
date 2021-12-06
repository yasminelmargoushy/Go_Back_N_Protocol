#include "packet.hpp"

// Configurations
#define NETWORK_BUFFER_SIZE 20
#define DATALINK_BUFFER_SIZE WINDOW_SIZE
#define TIME_OUT_INTERVAL 15

// Global Variables
int read_end, write_end;
uint32_t Next_Send_Seq = 0;
uint32_t Expected_Ack_Seq = DATALINK_BUFFER_SIZE - 1;
uint8_t DLinkBuffer_Av_Slots = DATALINK_BUFFER_SIZE;
uint8_t NextFill_DLinkBuffer = 0;
bool sent [DATALINK_BUFFER_SIZE];
bool Final_Send = false;


// Macros
#define inc(var)  var = ((var + 1) % DATALINK_BUFFER_SIZE)

/************************ PROTOTYPES ************************/
void Network_Layer_Init(void);
void enable_network_layer(void);
void disable_network_layer(void);
bool From_Network_Layer(Packet * P);
void To_Network_Layer(Packet p);
void send_data_packet(Packet p, bool last);
bool get_ack_packet();
void Timeout_Handler(uint8_t Seq);
void To_Physical_Layer(Frame f);
int8_t From_Physical_Layer(Frame * f);

bool between(uint8_t a, uint8_t b, uint8_t c)
{
	if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return true;
	else
		return false;
}

/************************ Timer *************************/
typedef struct{
int TimerCount[DATALINK_BUFFER_SIZE];
bool TimerFlag[DATALINK_BUFFER_SIZE];
}TIMER;
TIMER Timers;

void start_timer(uint8_t k)
{
  Timers.TimerFlag[k] = true;
}
void stop_timer(uint8_t k)
{
  Timers.TimerFlag[k] = false;
  Timers.TimerCount[k] = 0;
}

/************************ NETWORK LAYER ************************/
// Global Variables Related To Network Layer
Packet Network_Layer_Buffer [NETWORK_BUFFER_SIZE];
uint32_t current = 0;
bool network_layer_enabled = true;

// Network Layer Initialization Function
void Network_Layer_Init(void)
{
  Packet P;
  P.data = 'A';

  for(uint8_t i = 0; i < NETWORK_BUFFER_SIZE; ++i)
  {
    Network_Layer_Buffer[i] = P;
    ++P.data;
    if (P.data > 'Z')
    {
      P.data = 'A';
    }
  }
}
// Network Layer Control Functions
void enable_network_layer(void)
{
	network_layer_enabled = true;
}
void disable_network_layer(void)
{
	network_layer_enabled = false;
}
// Network Layer Access Layer
bool From_Network_Layer(Packet * P)
{
  bool Last = false;
  if (current >= NETWORK_BUFFER_SIZE)
  {
    P = nullptr;
    return Last;
  }
  else if (current == (NETWORK_BUFFER_SIZE - 1))
  {
    Last = true;
  }
  else if (current < (NETWORK_BUFFER_SIZE - 1))
  {
    Last = false;
  }
  (*P) = Network_Layer_Buffer[current];
  ++current;
  --DLinkBuffer_Av_Slots;
  inc(NextFill_DLinkBuffer);
  if (DLinkBuffer_Av_Slots == 0)
  {
    disable_network_layer();
  }
  return Last;
}
void To_Network_Layer(Packet p)
{
	cout << "[NETWORK-LAYER] " << p.data << endl;
}

/************************ DATALINK LAYER ************************/
// Global Variables Related To Datalink Layer
Packet Datalink_Layer_Buffer [DATALINK_BUFFER_SIZE];


void send_data_packet(Packet p, bool last) {
  // Arrange Frame
  Frame f;
  f.fr_type = fr_data;
  f.info = p;
  f.seq = Next_Send_Seq;
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

  // Start Timer
  start_timer(Next_Send_Seq);

  // Mark as Sent
  sent[Next_Send_Seq] = true;

  // Print Log
  printf("Sent Data packet %d whose Data is: %c\n", Next_Send_Seq, f.info.data);

  // Increament Next to send
  inc(Next_Send_Seq);

  usleep(500);
}

bool get_ack_packet(void)
{
  Frame f;
  // Read from Physical Layer
  int8_t Retint = From_Physical_Layer(&f);

  if ((Retint != -1) && (f.fr_type == ack) && (f.valid))
  {
    uint8_t temp_Ack = Expected_Ack_Seq;
    while (between(Expected_Ack_Seq, temp_Ack, Next_Send_Seq))
    {
      // Stop Timer
      stop_timer(temp_Ack);

      // Mark as unsent
      sent[temp_Ack] = false;

      ++DLinkBuffer_Av_Slots;
      if (temp_Ack == f.seq)
      {
        break;
      }
      inc (temp_Ack);
    }

    if (DLinkBuffer_Av_Slots > 0)
    {
      enable_network_layer();
    }
    printf("Receive Ack packet %d\n", f.seq);
    Expected_Ack_Seq = f.seq;
    inc(Expected_Ack_Seq);
    if(f.last)
    {
      exit(EXIT_SUCCESS);
    }
  }
  else if ((Retint != -1) && (f.seq == Expected_Ack_Seq) && (f.valid))
  {

  }
  return (Retint == -1)?false:true ;
}

void Timeout_Handler(uint8_t Seq)
{
  uint8_t temp = Expected_Ack_Seq;
  do
  {
    sent[temp] = false;
    Timers.TimerCount[temp] = 0;
    Timers.TimerFlag[temp] = false;
    inc(temp);
  }while ((temp != Next_Send_Seq));
  Final_Send = false;
  Next_Send_Seq = Expected_Ack_Seq;
  printf("Timeout Event on Ack of Frame %d\n", Seq);
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
  Network_Layer_Init();
  write_end = open(clin, O_WRONLY);
  read_end = open(clout, O_RDONLY | O_NONBLOCK);
  bool Last = false;
  bool Not_Last_in_NL = true;
  uint8_t Last_Seq = DATALINK_BUFFER_SIZE;
  bool first_Time = true;

  while(true)
  {
    /***************** Get From Network *****************/
    if (network_layer_enabled && Not_Last_in_NL)
    {
      uint8_t temp = NextFill_DLinkBuffer ;
      Last = From_Network_Layer( &(Datalink_Layer_Buffer[NextFill_DLinkBuffer]) );
      if (Last)
      {
        Not_Last_in_NL = false;
        Last_Seq = temp;
      }
    }

    /***************** Send Data Packet *****************/
    if((!Not_Last_in_NL) && (Next_Send_Seq == Last_Seq) && (!Final_Send))
    {
      send_data_packet(Datalink_Layer_Buffer[Next_Send_Seq], true);
      Final_Send = true;
    }
    else if (sent[Next_Send_Seq] == true)
    {
      // Pause Sending
    }
    else if (!Final_Send)
    {
      send_data_packet(Datalink_Layer_Buffer[Next_Send_Seq], false);
      if (first_Time)
      {
        first_Time = false;
        inc(Expected_Ack_Seq);
      }
    }

    /***************** Get Ack Packet *****************/
    get_ack_packet();
    for(uint8_t i=0; i < DATALINK_BUFFER_SIZE; ++i)
    {
      if(Timers.TimerFlag[i]==true)
      {
        Timers.TimerCount[i]++;
        if (Timers.TimerCount[i] == TIME_OUT_INTERVAL)
        {
          Timeout_Handler(i);
        }
      }
    }
  }
}