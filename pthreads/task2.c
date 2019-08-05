#include "task2.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

int num_threads;
int resolution;

void initialize(image *im) {

    int i;

    /* initialize header: */

    im->type[0] = 'P';
    im->type[1] = '5';
    im->type[2] = 0;

    im->width = resolution;
    im->height = resolution;
    im->maxval = 255;

    /* alloc space: */

    im->pixelmatrix = (unsigned char**)malloc(resolution*sizeof(unsigned char*));

    for(i = 0  ; i < resolution ; i++) {
        (im->pixelmatrix)[i] = (unsigned char*)malloc(resolution*sizeof(unsigned char));
    }
}


void* threadFunction(void *var) {

    int i, j;
    float x, y, distance;

	int thread_id = ((thread_struct*)var)->thread_id;
	image *img = ((thread_struct*)var)->img;

    /* split image: */
    int start = thread_id*(resolution/num_threads);
    int end = (thread_id+1)*(resolution/num_threads);

    if(thread_id == num_threads - 1) {
        end = resolution;
    }

    for (i = start ; i < end ; i++){
        for (j = 0 ; j < resolution ; j++) {

            /* x and y coords: */
            x = (float)j + 0.5;
            y = (float)(resolution - i - 0.5);

            /* distance = |a*x + b*y + c| / sqrt(a^2 + b^2) */
            distance = abs(-x*100/resolution + 2*y*100/resolution)/sqrt(5);

            if (distance < 3)
                img->pixelmatrix[i][j] = 0;    /* black pixel */
            else
                img->pixelmatrix[i][j] = 255;  /* white pixel */
        }
    }

    return var;
}

void render(image *im) {

    pthread_t tid[num_threads];
    thread_struct threads[num_threads];
    int i;

    /* split render on threads: */	
    for(i = 0 ; i < num_threads ; i++) {
        threads[i].thread_id = i;
        threads[i].img = im;
    }

    /* create threads: */
    for(i = 0 ; i < num_threads ; i++) {
        pthread_create(&(tid[i]), NULL, threadFunction, &(threads[i]));
    }
	
    /* join threads: */
    for(i = 0 ; i < num_threads ; i++) {
        pthread_join(tid[i], NULL);
    }
}

void writeData(const char * fileName, image *img) {

    /* open file: */
    FILE *f = fopen(fileName, "w+");
    int i;

    /* write image header: */
    fprintf(f, "%s\n", img->type);
    fprintf(f, "%d %d\n", img->width, img->height);
    fprintf(f, "%hhu\n", img->maxval);

    /* write pixel matrix: */
    for (i = 0 ; i < img->height ; i++) {
        fwrite((void*)(img->pixelmatrix[i]), 1, img->width, f);
    }

    /* free space: */
    for(i = 0  ; i < img->height ; i++)
        free((img->pixelmatrix)[i]);
    free(img->pixelmatrix);

    /* close file: */
    fclose(f);
}

