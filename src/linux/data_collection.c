/*
General format of data collection module for the ML integration part

(link Linked list)
head->1st_link->2nd_link->...->nth_link->head

head is not used for anything but as an anchor searching and parsing

Every link node has the following entries:

adress: holds the address of the link
size: has the size of the collected data points
data_node: linked list of data points
prediction: results of the prediction
next*: the next link node (holds the address of the address if the last node)


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

/*to do

void link_data_get_data()
*/

#include "data_collection.h"
#include "lq_plugin_ffeth_nl80211.h"
#include "fpm.h"
#include "../keras2c/model_lstm.h"



struct link_data *head = NULL;

bool link_data_add_link(union olsr_ip_addr *address, struct link_data *last_pointer){
    init_link_data_head();
    struct link_data *parser;
    parser = head;
    while (parser->next!=head){
        parser = parser->next;
    }

    parser->next = (struct link_data*) malloc(sizeof(struct link_data));
    parser = parser->next;

    parser->next = head;
    parser->link_addr = address;
    parser->size = 0;
    parser->prediction = 2;
    parser->prediction_pointer = 10;
    for(int i =0;i<prediction_window;i++)
      parser->prediction_holder[i] = 2;

    parser->data_head = (struct data_node*) malloc (sizeof(struct data_node));
    parser->data_head->next = parser->data_head;
    parser->max_rssi = -100;
    parser->min_rssi = 0;

    last_pointer = parser;
    return true;
}

void init_link_data_head(){
    if(head==NULL){
        OLSR_PRINTF(1,"Init head\n");
        head = (struct link_data*) malloc(sizeof(struct link_data));
        head->next = (struct link_data*) malloc(sizeof(struct link_data));
        head->next = head;
    }
}

bool link_data_add_link_data(struct data_node *data_node_entry, union olsr_ip_addr *address){
    struct link_data *link_node;
    link_node = get_link_node(address);
    if(link_node == NULL)
        if(!link_data_add_link(address, link_node)){
            OLSR_PRINTF(1,"Could not add a new link to the link data\n");
        }

    if(!data_node_add(link_node, data_node_entry))
         OLSR_PRINTF(1,"Could not add a new data node to the link node\n");

}


struct link_data *get_link_node(union olsr_ip_addr *address){
    OLSR_PRINTF(1,"Inside get link node\n");
    struct link_data *parser;
    init_link_data_head();
    parser = head;
    
    while (parser->next!=head){
        parser = parser->next;
        OLSR_PRINTF(1,"Going through the addresses");
        OLSR_PRINTF(1,"\n %d\n",address);
        if (ipequal(address,parser->link_addr)){
            OLSR_PRINTF(1,"\nFound a link");
            return parser;
        }
    }
    OLSR_PRINTF(1,"Couldnt get the addres");
    return NULL;
}

bool data_node_add(struct link_data *link_node, struct data_node *node_entry){
    OLSR_PRINTF(1,"Inside data node add\n");
    if (link_node==NULL)
        return false;
    struct data_node *parser;
    parser = link_node->data_head;
    while(parser->next!=link_node->data_head){
        parser=parser->next;
    }

    parser->next = (struct data_node*)malloc(sizeof(struct data_node));
    parser->next = node_entry;
    parser = parser->next;
    parser->next = link_node->data_head;
    
    

    if (link_node->size == data_size){
        OLSR_PRINTF(1,"data size more than the max \n");
        struct data_node *temp;
        temp = link_node->data_head->next;
        link_node->data_head->next = temp->next;
        free(temp);
    }
    else{
        link_node->size++;
    }
    return true;
}

bool data_node_clear(struct link_data *link_node){
    OLSR_PRINTF(1,"Inside the data_node_clear\n");
    struct data_node *parser;
    struct data_node *temp;
    parser = link_node->data_head;
    temp = parser->next;
    while(temp!=link_node->data_head){
        parser = temp->next;
        free(temp);
        temp = parser; 
    }

    link_node->data_head->next = link_node->data_head;
    return true;
}

bool link_data_remove(union olsr_ip_addr *address){
    struct link_data *parser;
    struct link_data *previous;
    OLSR_PRINTF(1,"deleting a node\n");
    parser = head;
    struct ipaddr_str buf;
    printf("The given address: %s\n",olsr_ip_to_string(&buf, address));
    

    while (parser->next!=head){
        previous = parser;
        parser = parser->next;
        printf("Checking address: %s\n",olsr_ip_to_string(&buf, parser->link_addr));
        if (ipequal(address,parser->link_addr)){
            printf("Found the IP\n");
            previous->next = parser->next;
            data_node_clear(parser);
            free(parser);
            return true;
        }
    }
    return false;
}


void add_and_predict(){

    //goes through all links and adds the required feature and adds the prediction too
    OLSR_PRINTF(1,"Into the add and predict function\n");
    struct link_entry *link;
    init_link_data_head();
    OLSR_FOR_ALL_LINK_ENTRIES(link) {
        
    OLSR_PRINTF(1,"Going through the links\n");
    struct data_node *new_data = (struct data_node*)malloc(sizeof(struct data_node));
    // if (head==NULL){
    //     OLSR_PRINTF(1,"NULL head\n");
    //     init_link_data_head();
    // }
    const struct lq_ffeth *lq = (struct lq_ffeth*) link->linkquality;  /*Something wierd here i can read the lq and nlq using lq_ffeth*/
    new_data->link_lq = (lq->valueLq + 1)/255.0;
    // printf("The lq here: %d\n",lq->valueLq);
    new_data->link_Nlq = (lq->valueNlq + 1)/255.0;
    new_data->rssi = 12; //totally random number
    #ifdef LINUX_NL80211
        new_data->rssi = lq->valueRSSI;
    #endif
    
    
    #ifdef LINUX_NL80211
      //Adding tau and trend
    // extract the value and sign of tau of
    uint8_t sign, atau;
    float tauf;
    int trend;
    
    if ((lq->valueBandwidth & 0xF8)== 0)
    {
      new_data->tauf = 0;
      new_data->trend = 0;

    }
    else
    {
      sign = lq->valueBandwidth & 0x04;
      atau = (lq->valueBandwidth & 0xF8) >> 3;
      new_data->tauf = (float)(1.0 * atau / 32.0);
      if (sign == 0)
      {
        new_data->tauf = new_data->tauf * -1.0;
        new_data->trend  = -1;
      }
      else{
        new_data->trend = 1;
      }
//
//
    }
    trend = new_data->trend;
    #endif
    if(new_data->rssi > 0)
        new_data->rssi = link->rssi;

    link_data_add_link_data(new_data,&(link->neighbor_iface_addr));


    // add the ML model here
    float data[60];
    if(link_data_get_data(data, &(link->neighbor_iface_addr))){

        k2c_tensor lstm_input_tensor = {data,2,(data_size*features),{data_size, features, 1, 1, 1}};
        float c_dense[3] = {0,0,0}; 
        k2c_tensor c_dense_tensor = {&c_dense[0],1,1,{1,1,1,1,1}};
        model_lstm_initialize(); 
        model_lstm(&lstm_input_tensor,&c_dense_tensor);
        // model_lstm();
        struct link_data *link_node;
        link_node = get_link_node(&(link->neighbor_iface_addr));
        //link_node->prediction = 1;
        int out = 0;
        
        
        if (c_dense_tensor.array[0]>c_dense_tensor.array[1] && c_dense_tensor.array[0]>c_dense_tensor.array[2] )
          out = 1;
        else
          if(c_dense_tensor.array[1] >= probthreshold || c_dense_tensor.array[2] >= probthreshold)
            if (c_dense_tensor.array[1] >= probthreshold)
              out = 2;
            else
              out = 0;
          else
            out = link_node->prediction_holder[(link_node->prediction_pointer-1)%prediction_window];
        //-1 static
        //0 away
        //1 towards
        
        //-1 - towards    array[0]
          
        //0 - static      array[1]
        
        //1 - away       array[2]
        
        printf("The prediction of 0: %f\n",c_dense_tensor.array[0]);//static
        printf("The prediction of 1: %f\n",c_dense_tensor.array[1]);//away
        printf("The prediction of -1: %f",c_dense_tensor.array[2]);//towards
        
        //Add the prediction to the window
        link_node->prediction_holder[link_node->prediction_pointer%prediction_window] = out;
        link_node->prediction_pointer = link_node->prediction_pointer%prediction_window + 1;
        
        //get the max out of the window (the max has to be more than 3 window inputs)
        int8_t max0 = 0;
        int8_t max1 = 0;
        int8_t max2 = 0;
        for(int i = 0;i<prediction_window;i++){
          if (link_node->prediction_holder[i] == 0)
            max0++;
          else if (link_node->prediction_holder[i] == 1)
            max1++;
          else
            max2++;
        }
        
        if (max0 >= 4 || max1>=4 || max2>=4){
          if (max0>max1 && max0>max2)
            link_node->prediction = 0;
          else if (max1>max2)
            link_node->prediction = 1;
          else 
            link_node->prediction = 2;
        }
        
        //link_node->prediction = out;
        OLSR_PRINTF(1,"Prediction: %d\n",out);
        // ((struct lq_ffeth*) link->linkquality)->prediction = out;
        //lq->prediction = (uint8_t) out;


    }


    }OLSR_FOR_ALL_LINK_ENTRIES_END(link)

    //print all the links here to see how its working
    print_data();

}


void print_data(void){
    struct link_data *parser;
    
    parser = head;
    struct data_node *temp;
    struct ipaddr_str buf;
    while (parser->next!=head){
        parser = parser->next;
        if(parser->size == 0)
            continue;
        OLSR_PRINTF(1,"\nPrinting data nodes for: %s\n",olsr_ip_to_string(&buf, parser->link_addr));
            temp = parser->data_head;
            while(temp->next!=parser->data_head){
                temp = temp->next;
               // OLSR_PRINTF(1,"LQ: %f NLQ: %f RSSI: %d trend: %d tau: %f Prediction: %d",temp->link_lq,temp->link_Nlq,temp->rssi,temp->trend,temp->tauf,parser->prediction);
                OLSR_PRINTF(1,"LQ: %f NLQ: %f RSSI: %d Trend: %d Prediction: %d",temp->link_lq,temp->link_Nlq,temp->rssi,temp->trend,parser->prediction);

                float norm = ((temp->rssi*1.0+100)/(100.0));
                OLSR_PRINTF(1," Normalized RSSI: %f \n", norm);

            }

        OLSR_PRINTF(1,"-------------------\n");
    }    
}

bool link_data_get_data(float array[], union olsr_ip_addr *address){
    struct link_data *parser;
    parser = head->next;
    struct data_node *data;
    while (parser->next){
        //Get the link node
        
        if (ipequal(address,parser->link_addr)){
            printf("Found the Node\n");
            if(parser->size < data_size){
                // printf("Too few data: %d\n",parser->size );
                return false;

            }
            int i = 0;
            data = parser->data_head->next;
            parser->max_rssi = -100;
            parser->min_rssi = 0;
            
            while (data!= parser->data_head){ //Get the max and min RSSI values for normalization
                if(parser->max_rssi < data->rssi)
                    parser->max_rssi = data->rssi;
                if(parser->min_rssi> data->rssi)
                    parser->min_rssi = data->rssi;
                data = data->next;
            }
            if (parser->min_rssi == parser->max_rssi){
                parser->max_rssi++;
            }
            data = parser->data_head->next;
            while (data != parser->data_head){
                
                array[i++] = data->link_lq;
                array[i++] = data->link_Nlq;
                if(data->trend<0){
                  array[i++] = 0;
                  array[i++] = data->tauf*-0.1;
                }
                else if(data->trend == 0){
                  array[i++] = 0.5;
                  array[i++] = data->tauf;
                }
                else{
                  array[i++] = data->trend;
                  array[i++] = data->tauf;
                }
                array[i++] = ((data->rssi*1.0+100)/(100.0));
                //array[i++] = ((data->rssi*1.0-parser->min_rssi)/(parser->max_rssi - parser->min_rssi));             
                //array[i++] = (data->rssi);
                //printf("LQ: The normalized RSSI: %f", array[i-1]);
                
                // printf("%d\n",i);
                data = data->next;
            }


        
        // parser->prediction = c_dense_tensor.array[0] > 0.75;


            return true;
        }
        parser = parser->next;
    }
    if (parser == head){
        return false;
    }
    return false;

}

int get_prediction(union olsr_ip_addr *address){
        struct link_data *link_node;
        link_node = get_link_node(address);
        if (link_node == NULL){
            //OLSR_PRINTF(1,"The node is none: %d\n", 2);
            return 2;
            }
        return link_node->prediction;
}



