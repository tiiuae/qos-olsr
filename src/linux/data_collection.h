/*
General format of data collection module for the ML integration part

(Neighbors Linked list)
head->1st_neighbor->2nd_neighbor->...->nth_neighbor->head

head is not used for anything but as an anchor searching and parsing

Every neighbor node has the following entries:

adress: holds the address of the neighbor
size: has the size of the collected data points
data_node: linked list of data points
prediction: results of the prediction
next*: the next neighbor node (holds the address of the address if the last node)


(data_node linked list)

head->1st_data_point->2nd_data_point->...->size_data_point->head

head is not used for anything but as an anchor searching and parsing

Every data_point node has the following entries:

LQ
NLQ
RSSI
next*
(any additional features for ML can be added here)

*/
#include "olsr_types.h"
#include "ipcalc.h"
#include "link_set.h"
#include "../keras2c/model_lstm.h"



#define data_size 12  /*the window on the data*/
#define features 5
#define prediction_window 7
#define probthreshold 0.9

struct data_node{                   /* data_node entry */
  float link_lq;                  /* data_node entry linkquality */
  float link_Nlq;                 /* data_node entry neighbor link quality */
  int8_t rssi;                      /* data_node entry rssi*/
  int trend;                        /* data_node entry trend*/
  float tauf;                       /* data_node entry data*/
  struct data_node *next;           /* the next data_node entry */
};


struct link_data{                   /* link_data entry */
  union olsr_ip_addr *link_addr;     /* link_data address */
  uint16_t size;                    /*size of the data node*/
  uint32_t prediction;              /* value of the prediction (the type can be adjusted accordingly) */
  int8_t max_rssi;
  int8_t min_rssi;
  int8_t prediction_holder[7];
  int8_t prediction_pointer;
  struct data_node *data_head;
  struct link_data *next;           /* the next link data */
};



bool link_data_add_link(union olsr_ip_addr *addreess, struct link_data *last_pointer);
bool link_data_add_link_data(struct data_node *data_node_entry, union olsr_ip_addr *);
bool link_data_remove(union olsr_ip_addr *);
struct link_data* get_link_node(union olsr_ip_addr *address);
void init_link_data_head(void);

void add_and_predict(void);
bool link_data_get_data(float array[], union olsr_ip_addr *address);
int get_prediction(union olsr_ip_addr *address);

bool data_node_clear(struct link_data *link_node);
bool data_node_add(struct link_data *link_node, struct data_node *node_entry);
void print_data(void);





