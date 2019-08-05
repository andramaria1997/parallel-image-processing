#ifndef HOMEWORK_H
#define HOMEWORK_H

typedef struct {
    
    char type[3];
    int width, height;
    unsigned char maxval;
    unsigned char **pixelmatrix;

}image;

/* structure for passing in image and out image
 * as parameter for threadFunction */
typedef struct {

    image *in;
    image *out;
    int thread_id;

}thread_struct;

void readInput(const char * fileName, image *img);

void writeData(const char * fileName, image *img);

void resize(image *in, image * out);

void* threadFunction(void *var);

#endif /* HOMEWORK_H */
