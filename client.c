#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/wait.h>
#include <netinet/in.h>
#define MAX_COMMAND_SIZE 1024

#define BUFF_SIZE 4096

int main(int argc, int argv[])
{
    sqlite3 *db;
    int rc = sqlite3_open("schema.sql", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
  
    int firstInteraction = 1;
    int PORT;
    char port_txt[7];
    FILE * portfd=fopen("PORT.txt", "r");
    fgets(port_txt, sizeof(port_txt), portfd);
    PORT=atoi(port_txt);
    fflush(stdout);
    int network_socket;
    if ((network_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        return -1;
    }
    int reuse = 1;
    if (setsockopt(network_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    int con_status;
    if (con_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Error at connectin to socket");
        return -1;
    }
    else
        printf("Connected to port %d \n", PORT);
    char server_response[BUFF_SIZE];
    int quit=0;
    while(1)
    {
        //Enter a command
        fflush(stdout);
        if(firstInteraction)
        {
             printf("Hi User! Welcome to FoodWasteReducer! Please specify your meaning for this application (donor/receiver) and your name \n");
             firstInteraction=0;
        }else printf("Input: \n");
        char command[MAX_COMMAND_SIZE], response[MAX_COMMAND_SIZE];
        fgets(command, sizeof(command),stdin);
        if(strcmp(command,"quit")==0)
        {
            return 0;
        }
    
        send(network_socket, command, sizeof(command), 0);
        fflush(stdin);
        recv(network_socket, response, sizeof(response) - 1, 0);
        response[sizeof(response) - 1] = '\0';  // Null-terminate the string
        printf("%s\n", response);
        memset(response, 0, sizeof(response));  // Clear the buffer
        
    }
    return 0;
}
/*
Available commands:
(not signed)
donor / receiver [Name] - selects the type of client and its name

(signed)
[Donor]
list new food - starts the process of listing a new food 
remove item - will print the list of items you have published and its corresponding value. 
                you will have to type the number which corresponds to the item you want to remove. 
list my items - lists your current items  

[Receiver] , [Donor]
display [name]
show categories - displays a list of food categories 
display items from (name) - displays items from a category by its name (if it exists)   
quit
[Receiver]
add to cart (id) (number of servings) - adds the food with the specific id to the cart and the number of servings   
remove from cart (id) (number of servings) - adds the food with the specific id to the cart and the number of servings  
finish order - if there is something in the cart, for every product in the cart 
show cart - displays all items in your cart
*/