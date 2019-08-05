#ifndef HOMEWORK_H1
#define HOMEWORK_H1

/* The same structure as in the first task
 * for concistency reasons */
typedef struct {

    char type[3];
    int width, height;
    unsigned char maxval;
    unsigned char **pixelmatrix;

}image;

/* structure for passing out image
 * as parameter for threadFunction */
typedef struct {

    image *img;
    int thread_id;

}thread_struct;

void initialize(image *im);
void render(image *im);
void writeData(const char * fileName, image *img);
void* threadFunction(void *var);

#endif /* HOMEWORK_H1 */
