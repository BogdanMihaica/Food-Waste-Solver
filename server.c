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
#include <time.h>
#include <netinet/in.h>
#include <pthread.h> // Include the pthread library

#define MAX_COMMAND_SIZE 1024

#define BUFF_SIZE 4096
#define MESSAGE "Hey! This is the message from the server you were waiting for!"
#define MAX_ITEMS 1024
int numDonors = 0, numReceivers = 0, numFoods = 0;
sqlite3 *db;
struct Donors
{
    char name[50];
};

struct Receivers
{
    char name[50];
};

struct Food
{
    int id;
    char name[50];
    char category[50];
    char nameOfRestaurant[50];
    int servings;
    int gramsPerServing;
};
struct Cart
{
    int nrItems;
    struct
    {
        char name[50];
        int id;
        int servings;
    } item[60];
};
struct Donors donors[MAX_ITEMS];
struct Receivers receivers[MAX_ITEMS];
struct Food foods[MAX_ITEMS];
int updateItem(sqlite3 *db, int ID, int numberOfServings)
{
    char *errMsg = 0;
    if (numberOfServings == 0)
    {
        char deleteQuery[100];
        sprintf(deleteQuery, "DELETE FROM Food WHERE id = %d;", ID);

        int rc = sqlite3_exec(db, deleteQuery, 0, 0, &errMsg);

        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
            return 0;
        }
    }
    else
    {
        char updateQuery[100];
        sprintf(updateQuery, "UPDATE Food SET servings = %d WHERE id = %d;", numberOfServings, ID);

        int rc = sqlite3_exec(db, updateQuery, 0, 0, &errMsg);

        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
            return 0;
        }
    }

    return 1;
}
void removeItemByIndex(struct Cart *cart, int indexToRemove)
{

    for (int i = indexToRemove; i < cart->nrItems - 1; ++i)
    {
        cart->item[i] = cart->item[i + 1];
    }
    cart->nrItems--;
}
int generateUniqueId()
{
    time_t current_time;
    time(&current_time);

    static int counter = 1;
    int unique_id = (int)current_time + counter;
    counter++;

    return unique_id;
}
int addFoodToDatabase(struct Food newFood)
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;
    rc = sqlite3_open("data.db", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return rc;
    }
    char sql[1000];
    snprintf(sql, sizeof(sql),
             "INSERT INTO Food (id, name, category, nameOfRestaurant, servings, gramsPerServing) VALUES (%d, '%s', '%s', '%s', %d, %d);",
             newFood.id, newFood.name, newFood.category, newFood.nameOfRestaurant, newFood.servings, newFood.gramsPerServing);

    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return rc;
    }
    return SQLITE_OK;
}
char *tolower_string(const char *str)
{

    int length = 0;
    const char *temp = str;
    while (*temp != '\0')
    {
        length++;
        temp++;
    }
    char *result = (char *)malloc(length + 1);

    int i;
    for (i = 0; i < length; i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
        {

            result[i] = str[i] + ('a' - 'A');
        }
        else
        {
            result[i] = str[i];
        }
    }
    result[length] = '\0';

    return result;
}
int remove_item_from_food_table(sqlite3 *db, int item_id, const char *item_name)
{
    char sql[200];
    snprintf(sql, sizeof(sql), "DELETE FROM Food WHERE id = %d AND nameofrestaurant = '%s';", item_id, item_name);

    int rc = sqlite3_exec(db, sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    return SQLITE_OK;
}
int fetchDonorsCallback(void *data, int argc, char **argv, char **azColName)
{
    int *numDonors = (int *)data;
    if (*numDonors < MAX_ITEMS)
    {
        strcpy(donors[*numDonors].name, argv[0]);
        (*numDonors)++;
    }
    else
    {
        fprintf(stderr, "Exceeded maximum number of donors.\n");
    }
    return 0;
}

int fetchReceiversCallback(void *data, int argc, char **argv, char **azColName)
{
    int *numReceivers = (int *)data;
    if (*numReceivers < MAX_ITEMS)
    {
        strcpy(receivers[*numReceivers].name, argv[0]);
        (*numReceivers)++;
    }
    else
    {
        fprintf(stderr, "Exceeded maximum number of receivers.\n");
    }
    return 0;
}

int fetchFoodCallback(void *data, int argc, char **argv, char **azColName)
{
    int *numFoods = (int *)data;
    if (*numFoods < MAX_ITEMS)
    {
        struct Food *food = &foods[*numFoods];
        food->id = atoi(argv[0]);
        strcpy(food->name, argv[1]);
        strcpy(food->category, argv[2]);
        strcpy(food->nameOfRestaurant, argv[3]);
        food->servings = atoi(argv[4]);
        food->gramsPerServing = atoi(argv[5]);
        (*numFoods)++;
    }
    else
    {
        fprintf(stderr, "Exceeded maximum number of foods.\n");
    }
    return 0;
}

void fetchDonors(sqlite3 *db, int *numDonors)
{
    char *sql = "SELECT Name FROM Donors";
    char *errMsg = 0;

    int rc = sqlite3_exec(db, sql, fetchDonorsCallback, numDonors, &errMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

void fetchReceivers(sqlite3 *db, int *numReceivers)
{
    char *sql = "SELECT Name FROM Receivers";
    char *errMsg = 0;

    int rc = sqlite3_exec(db, sql, fetchReceiversCallback, numReceivers, &errMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

void fetchFood(sqlite3 *db, int *numFoods)
{
    char *sql = "SELECT * FROM Food";
    char *errMsg = 0;

    int rc = sqlite3_exec(db, sql, fetchFoodCallback, numFoods, &errMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}
void fetchAll(sqlite3 *db)
{
    numDonors = 0;
    fetchDonors(db, &numDonors);
    numReceivers = 0;
    fetchReceivers(db, &numReceivers);
    numFoods = 0;
    fetchFood(db, &numFoods);
}
void roleHandle(const char *role, int sockfd, char *loggedName, int *found, int *choice)
{
    fetchAll(db);
    *choice = 0;
    int j = 0;
    if (role[0] == 'd' &&
        role[1] == 'o' &&
        role[2] == 'n' &&
        role[3] == 'o' &&
        role[4] == 'r')
    {
        int i;
        for (i = 6; role[i] && role[i] != '\n'; i++)
        {
            loggedName[j++] = role[i];
        }
        loggedName[j] = '\0';
        *choice = 1;
    }
    else if (role[0] == 'r' &&
             role[1] == 'e' &&
             role[2] == 'c' &&
             role[3] == 'e' &&
             role[4] == 'i' &&
             role[5] == 'v' &&
             role[6] == 'e' &&
             role[7] == 'r')
    {
        int i;
        for (i = 9; role[i]; i++)
        {
            loggedName[j++] = role[i];
        }
        loggedName[j] = '\0';
        *choice = 2;
    }
 
    if (*choice == 1)
    {
        for (int i = 0; i < numDonors; i++)
        {
            if (strcmp(donors[i].name, loggedName) == 0)
            {
                *found = 1;
                break;
            }
        }
    }
    else if (*choice == 2)
    {
        for (int i = 0; i < numReceivers; i++)
        {
            if (strcmp(receivers[i].name, loggedName) == 0)
            {
                *found = 1;

                break;
            }
        }
    }
}
void remove_item(const char *comm, const char *name, int sockfd)
{
    char idString[20];
    strcpy(idString, comm + 12);
    int id = atoi(idString);
    if (id)
    {
        remove_item_from_food_table(db, id, name);
        char response_message[BUFF_SIZE];
        sprintf(response_message, "Item with ID %d successfully removed from our database.\n", id);
        fetchAll(db);
        if (send(sockfd, response_message, strlen(response_message), 0) == -1)
        {
            perror("Error sending response");
            return;
        }
    }
    else
    {
        char response_message[BUFF_SIZE] = "No id was parsed. Try again.";
        if (send(sockfd, response_message, strlen(response_message), 0) == -1)
        {
            perror("Error sending response");
            return;
        }
    }
}
void list_new_food(const char *role, int sockfd, const char *name)
{
    struct Food newFood;
    char response_message[BUFF_SIZE];
    char client_command[MAX_COMMAND_SIZE];
    sprintf(response_message, "Let's start the process of adding a new food\n Tell us the name of the food you want to add\n");
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
    ssize_t bytes_received = recv(sockfd, client_command, sizeof(client_command), 0);
    if (bytes_received <= 0)
    {
        perror("Error receiving command");
    }
    for (int i = 0; client_command[i]; i++)
    {
        if (client_command[i] == '\n')
            client_command[i] = '\0';
    }
    strcpy(newFood.name, client_command);
    memset(response_message, 0, sizeof(response_message));
    memset(client_command, 0, sizeof(client_command));
    sprintf(response_message, "Great! Now what's its category?\n");
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
    bytes_received = recv(sockfd, client_command, sizeof(client_command), 0);
    if (bytes_received <= 0)
    {
        perror("Error receiving command");
    }
    for (int i = 0; client_command[i]; i++)
    {
        if (client_command[i] == '\n')
            client_command[i] = '\0';
    }
    strcpy(newFood.category, client_command);
    memset(response_message, 0, sizeof(response_message));
    memset(client_command, 0, sizeof(client_command));
    sprintf(response_message, "How many servings do you want to list of that food?\n");
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
    bytes_received = recv(sockfd, client_command, sizeof(client_command), 0);
    if (bytes_received <= 0)
    {
        perror("Error receiving command");
    }
    for (int i = 0; client_command[i]; i++)
    {
        if (client_command[i] == '\n')
            client_command[i] = '\0';
    }
    newFood.servings = atoi(client_command);
    memset(response_message, 0, sizeof(response_message));
    memset(client_command, 0, sizeof(client_command));
    sprintf(response_message, "One last detail. How many grams are per one serving?\n");
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
    bytes_received = recv(sockfd, client_command, sizeof(client_command), 0);
    if (bytes_received <= 0)
    {
        perror("Error receiving command");
    }
    for (int i = 0; client_command[i]; i++)
    {
        if (client_command[i] == '\n')
            client_command[i] = '\0';
    }
    newFood.gramsPerServing = atoi(client_command);
    memset(response_message, 0, sizeof(response_message));
    memset(client_command, 0, sizeof(client_command));
    newFood.id = generateUniqueId();
    strcpy(newFood.nameOfRestaurant, name);
    sprintf(response_message, "Thank you for your contribution to save the world hunger! This is the item you listed:\nID:%d    NAME:%s   SERVINGS AVAILABLE:%d   GRAMS/SERVING:%d\n",
            newFood.id, newFood.name, newFood.servings, newFood.gramsPerServing);
    addFoodToDatabase(newFood);
    fetchAll(db);
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
void list_my_items(const char *name, int sockfd)
{
    fetchAll(db);
    char response_message[BUFF_SIZE];
    sprintf(response_message, "These are your items: \n");
    for (int k = 0; k < numFoods; k++)
    {
        if (strcmp(foods[k].nameOfRestaurant, name) == 0)
        {
            char info[200];
            sprintf(info, "ID:%d    NAME:%s   SERVINGS AVAILABLE:%d   GRAMS/SERVING:%d\n",
                    foods[k].id, foods[k].name, foods[k].servings, foods[k].gramsPerServing);
            strcat(response_message, info);
        }
    }
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
char *getNameById(int id)
{
    for (int i = 0; i < numFoods; i++)
    {
        if (id == foods[i].id)
        {
            return foods[i].name;
        }
    }
    return NULL;
}
void showCart(struct Cart *cart, int sockfd)
{
    char response_message[BUFF_SIZE] = "These are the items in your cart:\n";
    for (int i = 0; i < cart->nrItems; i++)
    {
        char info[200];
        sprintf(info, "ID:%d   NAME:%s  SERVINGS:%d\n", cart->item[i].id, cart->item[i].name, cart->item[i].servings);
        strcat(response_message, info);
    }
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
void add_to_cart(struct Cart *cart, const char *comm, int sockfd)
{
    char idString[20];
    char serv[20];
    int j = 0;
    int i;
    for (i = 12; comm[i] != ' '; i++)
    {
        idString[j++] = comm[i];
    }
    idString[j] = '\0';
    j = 0;
    for (; comm[i]; i++)
    {
        if (comm[i] != '\n')
        {
            serv[j++] = comm[i];
        }
    }
    serv[j] = '\0';
    int found = 0;
    int index = -1;
    int id = atoi(idString);
    int nrServings = atoi(serv);
    int forInfo = nrServings;
    fetchAll(db);
    // Lets see if we added it in the past
    for (int i = 0; i < cart->nrItems; i++)
    {
        if (cart->item[i].id == id)
        {
            index = i;
            break;
        }
    }
    if (index >= 0)
    {
        nrServings += cart->item[index].servings;
        for (int i = 0; i < numFoods; i++)
        {
            if (foods[i].id == id)
            {
                if (nrServings <= foods[i].servings)
                {
                    found = 1;
                }
                break;
            }
        }
        if (found)
        {

            cart->item[index].servings = nrServings;
            char response_message[BUFF_SIZE];
            sprintf(response_message, "Succesfully added %d more servings to item %d", forInfo, id);
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
        else
        {
            char response_message[BUFF_SIZE] = "Could not find product or servings exceed the limit";
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
    }
    else
    {
        for (int i = 0; i < numFoods; i++)
        {
            if (foods[i].id == id)
            {
                if (nrServings <= foods[i].servings)
                {
                    found = 1;
                }
                break;
            }
        }
        if (found)
        {

            cart->item[cart->nrItems].id = id;
            cart->item[cart->nrItems].servings = nrServings;
            strcpy(cart->item[cart->nrItems].name, getNameById(id));
            char response_message[BUFF_SIZE];
            cart->nrItems++;
            sprintf(response_message, "Succesfully added %d servings from item %d", nrServings, id);
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
        else
        {
            char response_message[BUFF_SIZE] = "Could not find product or servings exceed the limit";
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
    }
}
void remove_from_cart(struct Cart *cart, const char *comm, int sockfd)
{
    char idString[20];
    char serv[20];
    int j = 0;
    int i;
    for (i = 17; comm[i] != ' ' && comm[i]; i++)
    {
        idString[j++] = comm[i];
    }
    idString[j] = '\0';
    j = 0;
    for (; comm[i]; i++)
    {
        if (comm[i] != '\n')
        {
            serv[j++] = comm[i];
        }
    }
    serv[j] = '\0';
    int found = 0;
    int index = -1;
    int removeAll = 0;
    int id = atoi(idString);
    int nrServings = 0;
    if (serv[0])
        nrServings = atoi(serv);
    int forInfo = nrServings;
    fetchAll(db);
    for (int i = 0; i < cart->nrItems; i++)
    {
        if (id == cart->item[i].id)
        {
            found = 1;
            index = i;
        }
    }
    if (nrServings)
    {
        if (found)
        {
            if (nrServings < cart->item[index].servings)
            {
                cart->item[index].servings -= nrServings;
                char response_message[BUFF_SIZE];
                sprintf(response_message, "Succesfully removed %d servings from item %d", nrServings, id);
                if (send(sockfd, response_message, strlen(response_message), 0) == -1)
                {
                    perror("Error sending response");
                    return;
                }
            }
            else
                removeAll = 1;
        }
        else
        {
            char response_message[BUFF_SIZE] = "Could not find product or servings exceed the limit";
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
    }
    if (!nrServings || removeAll)
    {
        int start = index + 1;
        if (found)
        {
            for (int j = start; j < cart->nrItems; j++)
            {
                cart->item[j - 1] = cart->item[j];
            }
            cart->item[cart->nrItems - 1].id = 0;
            cart->item[cart->nrItems - 1].servings = 0;
            cart->nrItems--;
            char response_message[BUFF_SIZE];
            sprintf(response_message, "Succesfully removed all servings from item %d", id);
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
        else
        {
            char response_message[BUFF_SIZE] = "Could not find product or servings exceed the limit";
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
    }
}
int getServingsById(int id)
{
    for (int i = 0; i < numFoods; i++)
    {
        if (id == foods[i].id)
        {
            return foods[i].servings;
        }
    }
    return 0;
}
void finish_order(struct Cart *cart, int sockfd)
{
    fetchAll(db);
    int foundNot = 0;
    for (int i = 0; i < cart->nrItems; i++)
    {
        int index;
        int found = 0;
        for (int j = 0; j < numFoods; j++)
        {
            if (cart->item[i].id == foods[j].id && cart->item[i].servings <= foods[j].servings)
            {
                found = 1;
                index = j;
                break;
            }
        }
        if (found == 0)
        {
            foundNot = 1;
            char response_message[BUFF_SIZE];
            sprintf(response_message, "Item %d is no longer available. It will be removed automatically from your cart.", cart->item[i].id);
            removeItemByIndex(cart, i);
            i--; // Go back a step because the item that has to be verified next sits on the position that has been removed
            if (send(sockfd, response_message, strlen(response_message), 0) == -1)
            {
                perror("Error sending response");
                return;
            }
        }
    }
    if (foundNot == 0)
    {
        for (int i = 0; i < cart->nrItems; i++)
        {
            int newServings = getServingsById(cart->item[i].id) - cart->item[i].servings;
            updateItem(db, cart->item[i].id, newServings);
        }
        char response_message[BUFF_SIZE] = "Order has been placed!";
        if (send(sockfd, response_message, strlen(response_message), 0) == -1)
        {
            perror("Error sending response");
            return;
        }
        cart->nrItems = 0;
        fetchAll(db);
    }
}
void display_by_name(const char *comm, int sockfd)
{
    char prefix[50] = "display items from ";
    fetchAll(db);
    int i = sizeof(prefix);
    char category[20];
    int j = 0;
    strcpy(category, comm + 19);

 
    char response_message[BUFF_SIZE * 2] = "These are the items for your category: \n";
    for (int k = 0; k < numFoods; k++)
    {
        if (strcmp(tolower_string(foods[k].category), tolower_string(category)) == 0)
        {
            char info[200];
            sprintf(info, "ID:%d    NAME:%s   SERVINGS AVAILABLE:%d   GRAMS/SERVING:%d\n",
                    foods[k].id, foods[k].name, foods[k].servings, foods[k].gramsPerServing);
            strcat(response_message, info);
        }
    }

    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
void display(const char *comm, int sockfd)
{
    char prefix[50] = "display ";
    fetchAll(db);
    int i = sizeof(prefix);
    char category[20];
    int j = 0;
    strcpy(category, comm + 8);


    char response_message[BUFF_SIZE * 2] = "These are the items including your search: \n";
    for (int k = 0; k < numFoods; k++)
    {
        if (strstr(tolower_string(foods[k].name), tolower_string(category)))
        {
            char info[200];
            sprintf(info, "ID:%d    NAME:%s   SERVINGS AVAILABLE:%d   GRAMS/SERVING:%d\n",
                    foods[k].id, foods[k].name, foods[k].servings, foods[k].gramsPerServing);
            strcat(response_message, info);
        }
    }

    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
void show_categories(const char *role, int sockfd)
{
    char response_message[BUFF_SIZE];
    sprintf(response_message, "Categories:\n\nFast-food\nItalian\nAsian\nWorldwide\nRomanian\nVegan\nVegetarian\nPizza\nSoups\nSalads\n");
    if (send(sockfd, response_message, strlen(response_message), 0) == -1)
    {
        perror("Error sending response");
        return;
    }
}
void addName(sqlite3 *db, const char *name, const char *tableName)
{

    const char *sql = "INSERT INTO %s (name) VALUES (?)";
    char query[100];

    snprintf(query, sizeof(query), sql, tableName);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}
void *handle_client(void *arg)
{

    struct Cart cart;
    char table[10];
    int choice = 0;
    int found = 0;
    char logged_name[60];
    int client_sock = *((int *)arg);
    char client_command[MAX_COMMAND_SIZE];
    while (1)
    {

        ssize_t bytes_received = recv(client_sock, client_command, sizeof(client_command), 0);
        if (bytes_received <= 0)
        {
            perror("Error receiving command");
            break;
        }
        for (int i = 0; client_command[i]; i++)
        {
            if (client_command[i] == '\n')
                client_command[i] = '\0';
        }
        if ((strstr(client_command, "donor") || strstr(client_command, "receiver")))
        {
            if (!found)
            {

                roleHandle(client_command, client_sock, logged_name, &found, &choice);
                if (found == 0)
                {
                    char response_message[BUFF_SIZE];
                    sprintf(response_message, "No user was found with this name. Do you want to add it to our database? (y/n)\n");
                    if (send(client_sock, response_message, strlen(response_message), 0) == -1)
                    {
                        perror("Error sending response");
                    }
                    ssize_t bytes_received_yn = recv(client_sock, client_command, sizeof(client_command), 0);
                    for (int i = 0; client_command[i]; i++)
                    {
                        if (client_command[i] == '\n')
                            client_command[i] = '\0';
                    }
                    if (strcmp(client_command, "y") == 0 || strcmp(client_command, "Y") == 0)
                    {

                        if (choice == 1)
                            strcpy(table, "donors");
                        else if (choice == 2)
                            strcpy(table, "receivers");
                        addName(db, logged_name, table);
                        fetchAll(db);
                        char response_message[BUFF_SIZE];
                        sprintf(response_message, "Added your name to the database!\n");
                        if (send(client_sock, response_message, strlen(response_message), 0) == -1)
                        {
                            perror("Error sending response");
                        }
                    }
                    else
                    {
                        strcpy(response_message, "");
                        strcpy(logged_name, "");
                        sprintf(response_message, "Ok, aborting\n");
                        if (send(client_sock, response_message, strlen(response_message), 0) == -1)
                        {
                            perror("Error sending response");
                        }
                    }
                }
                else
                {
                    char response_message[BUFF_SIZE];
                    sprintf(response_message, "Succesfully logged as %s !\n", logged_name);
                    if (send(client_sock, response_message, strlen(response_message), 0) == -1)
                    {
                        perror("Error sending response");
                    }
                }
            }

            else
            {
                char response_message[BUFF_SIZE];
                sprintf(response_message, "Already logged as %s\n", logged_name);
                if (send(client_sock, response_message, strlen(response_message), 0) == -1)
                {
                    perror("Error sending response");
                }
            }
        }
        else if (strstr(client_command, "remove item") && choice == 1)
            remove_item(client_command, logged_name, client_sock);
        else if ((strcmp(client_command, "show categories") == 0))
            show_categories(client_command, client_sock);
        else if ((strcmp(client_command, "list my items") == 0) && choice == 1)
            list_my_items(logged_name, client_sock);
        else if (strstr(client_command, "display items from"))
            display_by_name(client_command, client_sock);
        else if (strstr(client_command, "list new food") && choice == 1)
            list_new_food(client_command, client_sock, logged_name);
        else if (strstr(client_command, "add to cart") && choice == 2)
            add_to_cart(&cart, client_command, client_sock);
        else if (strstr(client_command, "remove from cart") && choice == 2)
            remove_from_cart(&cart, client_command, client_sock);
        else if (strcmp(client_command, "finish order") == 0 && choice == 2)
            finish_order(&cart, client_sock);
        else if (strcmp(client_command, "show cart") == 0 && choice == 2)
            showCart(&cart, client_sock);
        else if (strstr(client_command, "display"))
            display(client_command, client_sock);
        else if (strcmp(client_command, "quit") == 0)
        {
            pthread_exit(NULL);
            char res[BUFF_SIZE];
            sprintf(res, "Bye!");
            if (send(client_sock, res, strlen(res), 0) == -1)
            {
                perror("Error sending response");
            }
        }
        else
        {
            char res[BUFF_SIZE];
            sprintf(res, "Unknown command");
            if (send(client_sock, res, strlen(res), 0) == -1)
            {
                perror("Error sending response");
            }
        }
    }
    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, int argv[])
{

    int rc = sqlite3_open("data.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    fetchAll(db); // Repopulates structs

    // for (int k = 0; k < numFoods; k++)
    // {

    //     printf("ID:%d    NAME:%s   SERVINGS AVAILABLE:%d   GRAMS/SERVING:%d\n",
    //            foods[k].id, foods[k].name, foods[k].servings, foods[k].gramsPerServing);
    // }

    // }
    // printf("Fetched receivers (%d):\n", numDonors);
    // for (int i = 0; i < numReceivers; i++)
    // {
    //     printf("%s  %d\n ", receivers[i].name, (int)strlen(receivers[i].name));
    // }
    int PORT;
    char port_txt[7];
    FILE *portfd = fopen("PORT.txt", "r");
    fgets(port_txt, sizeof(port_txt), portfd);
    PORT = atoi(port_txt);
    fflush(stdout);
    int network_socket;
    if ((network_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(network_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(network_socket); // Close the socket before returning
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_status;
    if ((bind_status = bind(network_socket, (struct sockaddr *)&server_address, sizeof(server_address))) == -1)
    {
        perror("Error at binding to the socket");
        close(network_socket); // Close the socket before returning
        return -1;
    }
    else
    {
        printf("Successful bind on port %d \n", PORT);
    }

    listen(network_socket, 100);

    while (1)
    {
        int client_sock;
        client_sock = accept(network_socket, NULL, NULL);

        if (client_sock == -1)
        {
            perror("Error accepting connection");
            continue;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sock) != 0)
        {
            perror("Error creating thread");
            close(client_sock);
        }
        else
        {
            pthread_detach(thread_id);
        }
    }
    close(network_socket);

    return 0;
}