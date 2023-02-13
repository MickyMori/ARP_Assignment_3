#include "./../include/processA_utilities.h"
#include <stdio.h>
#include <bmpfile.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#define SEM_PATH_WRITER "/sem_AOS_writer"
#define SEM_PATH_READER "/sem_AOS_reader"
#define SEM_PATH_START "/sem_AOS_start"

//1.5px in ncurses is equal to 30px in bmp
const int radius = 30;
// Data structure for storing the bitmap file
bmpfile_t *bmp;
// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {0, 255, 0, 0};
//define shared memory
const char *shm_name = "/AOS";
int shm_fd;
rgb_pixel_t *ptr;
/* Instantiate bitmap, passing three parameters:
*   - width of the image (in pixels)
*   - Height of the image (in pixels)
*   - Depth of the image (1 for greyscale images, 4 for colored images)
*/
const int width = 1600;
const int height = 600;
const int depth = 4;
//declare the size of the mmap
int s = 1600*600*sizeof(rgb_pixel_t);
//declare log file descriptor
int log_pa;
//declare variable to store a message
char msg[50];
//declare variable to keep track of the number of snapshots
int snap_number = 0;
//declare variable to name the snapshot
char snap_name[50];
//declare struct for server and client address
struct sockaddr_in serv_addr, cli_addr;
//declare socket file descriptor
int sockfd, newsockfd, sockfd_c;
//variable to know how to run processA
char run_as[2];
//addr and port number of companion
char addr[16], port[6];
int portno;

int cmd_received, cmd_send;

int clilen, n;
char buffer[5];

//define the host
struct hostent *server;


void draw_circle_bmp(int position_x_bmp, int position_y_bmp){

    sprintf(msg, "Drawing circle in position (%d, %d)\n", position_x_bmp, position_y_bmp);
    write(log_pa, msg, strlen(msg));
    // Code for drawing a centered circle
    for(int x = -radius; x <= radius; x++) {
        for(int y = -radius; y <= radius; y++) {
            // If distance is smaller, point is within the circle
            if(sqrt(x*x + y*y) < radius) {
                /*
                * Color the pixel at the specified (x,y) position
                * with the given pixel values
                */
                bmp_set_pixel(bmp, position_x_bmp + x, position_y_bmp + y, pixel);

            }
        }
    }

}

//this function is used to write the data of the bmp in the shared memory
void write_on_shm(){

    write(log_pa, "Writing in shared memory\n", 26);

    rgb_pixel_t *curr_pix;

    for(int i = 0; i < width; i++){
        for (int j = 0; j < height; j++){

            curr_pix = bmp_get_pixel(bmp, i, j);

            ptr[j+height*i].alpha = curr_pix->alpha;
            ptr[j+height*i].green = curr_pix->green;
            ptr[j+height*i].blue = curr_pix->blue;
            ptr[j+height*i].red = curr_pix->red;

        }
    }

}

int main(int argc, char *argv[])
{
    //open log file
    log_pa = open("logFiles/processA.log", O_WRONLY | O_APPEND | O_CREAT, 0666);  
	ftruncate(log_pa, 0);

    write(log_pa, "Process A starting\n", 20);

    sem_t *sem_id_start;

    sem_id_start = sem_open(SEM_PATH_START, O_CREAT, 0644, 1);
    if(sem_id_start== (void*)-1){
        write(log_pa, "reader sem_open failure", 24);
        exit(1);
    }

    sem_init(sem_id_start, 1, 1);

    sem_wait(sem_id_start);

    //choose how to run processA
    printf("Select how to run processA (n normal, c client, s server): \n");
    fflush(stdout);
    fgets(run_as, 2 , stdin);
    while(strcmp(run_as, "n") != 0 && strcmp(run_as, "c") != 0 && strcmp(run_as, "s") != 0){

        printf("Invalid input! Insert again (n normal, c client, s server): \n");
        fflush(stdout);
        fgets(run_as, 2 , stdin);
 
    }

    if(strcmp(run_as, "c") == 0){

        printf("Insert the address of the companion application: \n");
        fflush(stdout);
        scanf("%s", addr);
            
    }

    if(strcmp(run_as, "c") == 0 || strcmp(run_as, "s") == 0){
    
        printf("Insert the port number of the companion application: \n");
        fflush(stdout);
        scanf("%s", port);

    }

    if(strcmp(run_as, "n") == 0){
        write(log_pa, "Normal mode selected\n", 22);
    }else if(strcmp(run_as, "c") == 0){
        write(log_pa, "Client mode selected\n", 22);
    }else if(strcmp(run_as, "s") == 0){
        write(log_pa, "Server mode selected\n", 22);
    }



    //define semaphores
    sem_t *sem_id_writer;
    sem_t *sem_id_reader;

    //if the program is running in server mode opens the connection
    if(strcmp(run_as, "s") == 0){
        //open socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            write(log_pa, "ERROR opening socket\n", 22);

        bzero((char *) &serv_addr, sizeof(serv_addr));

        portno = atoi(port);

        serv_addr.sin_family = AF_INET;

        serv_addr.sin_port = htons(portno);

        serv_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            write(log_pa, "ERROR on binding\n", 18);

        listen(sockfd,5); 

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            write(log_pa, "ERROR on accept\n", 17);
    }

    //if the program is running in client mode open the connection
    if(strcmp(run_as, "c") == 0){

        portno = atoi(port);

        

        sockfd_c = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_c < 0)
            write(log_pa, "ERROR opening socket\n", 22);

        

        serv_addr.sin_family = AF_INET;

        serv_addr.sin_port = htons(portno);

        inet_aton(addr, &serv_addr.sin_addr);

        if (connect(sockfd_c,&serv_addr,sizeof(serv_addr)) < 0)
            write(log_pa, "ERROR connecting\n", 18);

    }

    //open shared memory
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == 1)
    {
        write(log_pa, "Shared memory segment failed\n", 30);
        exit(1);
    }

    ftruncate(shm_fd, s);
    
    //create mapping
    ptr = (rgb_pixel_t *)mmap(0, s, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        write(log_pa, "Map failed\n", 12);
        return 1;
    }

    close(shm_fd);

    //open writer semaphore
    sem_id_writer = sem_open(SEM_PATH_WRITER, O_CREAT, 0644, 1);
    if(sem_id_writer== (void*)-1){
        write(log_pa, "writer sem_open failure", 24);
        exit(1);
    }

    //open  reader semaphore
    sem_id_reader = sem_open(SEM_PATH_READER, O_CREAT, 0644, 1);
    if(sem_id_reader== (void*)-1){
        write(log_pa, "reader sem_open failure", 24);
        exit(1);
    }

    //initialize semaphores
    sem_init(sem_id_writer, 1, 1);
    sem_init(sem_id_reader, 1, 0);

    sem_post(sem_id_start);

    //create the bmp
    bmp = bmp_create(width, height, depth);


    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();
    
    //initilalize the position of the center of the circle in the middle of the bmp itself
    int position_x_bmp = width/2;
    int position_y_bmp = height/2;

    draw_circle_bmp(position_x_bmp, position_y_bmp);

    //this semaphore is used to make sure that while process A writes on the shared memory
    //process B doesn't try to read from it
    sem_wait(sem_id_writer);

    write_on_shm();

    //this semaphore is used to make sure that process A doesn't procede
    //untill process B has finished reading
    sem_post(sem_id_reader);

    // Infinite loop
    while (TRUE)
    {

        if(strcmp(run_as, "s") == 0){

            //receive an integer value as a string corresponding to the key that has been pressed
            n = read(newsockfd,buffer,5);
                if (n < 0) error("ERROR reading from socket");


            //convert the bytes into an integer
            cmd_received = atoi(buffer); 

            if(cmd_received == KEY_RESIZE) {
                if(first_resize) {
                    first_resize = FALSE;
                }
                else {
                    reset_console_ui();

                    write(log_pa, "Resizing window\n", 17);
                    // destroy previous bmp and create a blank new one
                    bmp_destroy(bmp);
                    bmp = bmp_create(width, height, depth);

                    position_x_bmp = width/2;
                    position_y_bmp = height/2;

                    draw_circle_bmp(position_x_bmp, position_y_bmp);
                    
                    //this semaphore is used to make sure that while process A writes on the shared memory
                    //process B doesn't try to read from it
                    sem_wait(sem_id_writer);

                    write_on_shm();

                    //this semaphore is used to make sure that process A doesn't procede
                    //untill process B has finished reading
                    sem_post(sem_id_reader);

                }
            }

            // Else, if user presses print button...
            else if(cmd_received == 1111) {

                mvprintw(LINES - 1, 1, "Print button pressed");
                refresh();

                write(log_pa, "Print button pressed\n", 22);

                //increment the number of snapshots
                snap_number++;
                //name the current snap
                sprintf(snap_name, "out/snapshot%d.bmp", snap_number);
                // Save image as .bmp file
                bmp_save(bmp, snap_name);

                sleep(1);
                for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                    mvaddch(LINES - 1, j, ' ');
                }

            }

            // If input is an arrow key, move circle accordingly...
            else if(cmd_received == KEY_LEFT || cmd_received == KEY_RIGHT || cmd_received == KEY_UP || cmd_received == KEY_DOWN) {

                // destroy previous bmp and create a blank new one
                bmp_destroy(bmp);
                bmp = bmp_create(width, height, depth);

                move_circle(cmd_received);
                draw_circle();

                //compute the position of the circle in the bmp
                position_x_bmp = circle.x*20;
                position_y_bmp = circle.y*20;

                if(position_y_bmp < 30)
                    position_y_bmp = 30;

                draw_circle_bmp(position_x_bmp, position_y_bmp);

                //this semaphore is used to make sure that while process A writes on the shared memory
                //process B doesn't try to read from it
                sem_wait(sem_id_writer);

                write_on_shm();

                //this semaphore is used to make sure that process A doesn't procede
                //untill process B has finished reading
                sem_post(sem_id_reader);


            }

            bzero(buffer, 5);
        
        }
        else if(strcmp(run_as, "n") == 0 || strcmp(run_as, "c") == 0){
            // Get input in non-blocking mode
            int cmd = getch();

            // If user resizes screen, re-draw UI...
            if(cmd == KEY_RESIZE) {
                if(first_resize) {
                    first_resize = FALSE;
                }
                else {
                    reset_console_ui();

                    write(log_pa, "Resizing window\n", 17);
                    // destroy previous bmp and create a blank new one
                    bmp_destroy(bmp);
                    bmp = bmp_create(width, height, depth);

                    position_x_bmp = width/2;
                    position_y_bmp = height/2;

                    draw_circle_bmp(position_x_bmp, position_y_bmp);
                    
                    //this semaphore is used to make sure that while process A writes on the shared memory
                    //process B doesn't try to read from it
                    sem_wait(sem_id_writer);

                    write_on_shm();

                    //this semaphore is used to make sure that process A doesn't procede
                    //untill process B has finished reading
                    sem_post(sem_id_reader);

                }
            }

            // Else, if user presses print button...
            else if(cmd == KEY_MOUSE) {
                if(getmouse(&event) == OK) {
                    if(check_button_pressed(print_btn, &event)) {
                        mvprintw(LINES - 1, 1, "Print button pressed");
                        refresh();

                        write(log_pa, "Print button pressed\n", 22);

                        if(strcmp(run_as, "c") == 0){

                            sprintf(buffer,"%d", 1111);

                            n = write(sockfd_c,buffer,strlen(buffer));
                            if (n < 0)
                                write(log_pa, "ERROR writing to socket\n", 25);
                            bzero(buffer,5);

                        }

                        //increment the number of snapshots
                        snap_number++;
                        //name the current snap
                        sprintf(snap_name, "out/snapshot%d.bmp", snap_number);
                        // Save image as .bmp file
                        bmp_save(bmp, snap_name);

                        sleep(1);
                        for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                            mvaddch(LINES - 1, j, ' ');
                        }
                    }
                }
            }

            // If input is an arrow key, move circle accordingly...
            else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

                if(strcmp(run_as, "c") == 0){

                    sprintf(buffer,"%d", cmd);

                    n = write(sockfd_c,buffer,strlen(buffer));
                    if (n < 0)
                        error("ERROR writing to socket");
                    bzero(buffer,5);

                }

                // destroy previous bmp and create a blank new one
                bmp_destroy(bmp);
                bmp = bmp_create(width, height, depth);

                move_circle(cmd);
                draw_circle();

                //compute the position of the circle in the bmp
                position_x_bmp = circle.x*20;
                position_y_bmp = circle.y*20;

                if(position_y_bmp < 30)
                    position_y_bmp = 30;

                draw_circle_bmp(position_x_bmp, position_y_bmp);

                //this semaphore is used to make sure that while process A writes on the shared memory
                //process B doesn't try to read from it
                sem_wait(sem_id_writer);

                write_on_shm();

                //this semaphore is used to make sure that process A doesn't procede
                //untill process B has finished reading
                sem_post(sem_id_reader);


            }
        }
    }

    //close semaphores
    sem_close(sem_id_reader);
    sem_close(sem_id_writer);

    //unlink semaphores
    sem_unlink(SEM_PATH_READER);
    sem_unlink(SEM_PATH_WRITER);

    //delete mapping
    munmap(ptr, s);

    // Free resources before termination
    bmp_destroy(bmp);

    //close sockets
    if(strcmp(run_as, "s") == 0){
        close(sockfd);
        close(newsockfd);
    }else if(strcmp(run_as, "c") == 0){
        close(sockfd_c);
    }

    endwin();
    return 0;
}