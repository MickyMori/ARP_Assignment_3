#include "./../include/processB_utilities.h"
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
int width = 1600;
int height = 600;
int depth = 4;
//declare the size of the mmap
int s = 1600*600*sizeof(rgb_pixel_t);
//declare log file descriptor
int log_pb;
//declare variable to store a message
char msg[50];


//this function is used to build the bmp having all the pixel values inside ptr
void build_bmp(){

    write(log_pb, "Read from shared memory and update the bmp\n", 44);
    for (int i = 0; i < width; i++){
        for (int j = 0; j < height; j++){
            bmp_set_pixel(bmp, i, j, ptr[j+height*i]);
        }
    }

}

//this function has the purpose to retrieve the center of the circle from the bmp knowing its radius
center find_center(){
    int counter;
    rgb_pixel_t *curr_pix;
    center c;
    for (int i = 0; i < width; i++){
        counter = 0;
        for (int j = 0; j < height; j++){

            curr_pix = bmp_get_pixel(bmp, i, j);

            if(curr_pix->green == pixel.green && curr_pix->red == pixel.red){
                counter++;
            }
            
            if(counter == 59){
                c.x = i;
                c.y = j-30;
                sprintf(msg, "Center found in position (%d, %d)\n", c.x, c.y);
                write(log_pb, msg, strlen(msg));
                return c;
            }
        }
    }
    return c;
}


int main(int argc, char const *argv[])
{
    //open log file
    log_pb = open("logFiles/processB.log", O_WRONLY | O_APPEND | O_CREAT, 0666);  
	ftruncate(log_pb, 0);
    center c;

    //the sleep function is used to make sure that process A creates the shared memory before 
    //process B tries to open it
    sleep(1);

    //define semaphores
    sem_t *sem_id_writer;
    sem_t *sem_id_reader;
    sem_t *sem_id_start;

    sem_id_start = sem_open(SEM_PATH_START, 0644, 1);
    if(sem_id_start== (void*)-1){
        write(log_pb, "start sem_open failure", 23);
        exit(1);
    }

    sem_wait(sem_id_start);

    //open shared memory
    shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd == 1)
    {
        write(log_pb, "Shared memory segment failed\n", 30);
        exit(1);
    }

    //create mapping of the shared memory
    ptr = (rgb_pixel_t *)mmap(0, s, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        write(log_pb, "Map failed\n", 12);
        return 1;
    }

    close(shm_fd);

    //open writer semaphore
    sem_id_writer = sem_open(SEM_PATH_WRITER, 0);
    if(sem_id_writer== (void*)-1){
        write(log_pb, "writer sem_open failure", 24);
        exit(1);
    }

    //open writer semaphore
    sem_id_reader = sem_open(SEM_PATH_READER, 0);
    if(sem_id_reader== (void*)-1){
        write(log_pb, "reader sem_open failure", 24);
        exit(1);
    }

    //create the bmp
    bmp = bmp_create(width, height, depth);


    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Infinite loop
    while (TRUE) {

        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        else {
            sem_wait(sem_id_reader);

            // destroy previous bmp and create a blank new one
            bmp_destroy(bmp);
            bmp = bmp_create(width, height, depth);

            build_bmp();

            c = find_center();

            c.x = c.x/20;
            c.y = c.y/20;

            draw_circle(c);

            sem_post(sem_id_writer);
            
        }

    }

    //close semaphores
    sem_close(sem_id_reader);
    sem_close(sem_id_writer);

    //unlink semaphores
    sem_unlink(SEM_PATH_READER);
    sem_unlink(SEM_PATH_WRITER);

    endwin();
    return 0;
}
