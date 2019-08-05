#include "task1.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int num_threads;
int resize_factor;

void readInput(const char * fileName, image *img) {

    FILE *f = fopen(fileName, "ro");
    int colors, i;

    /* read image header: */
    fscanf(f, "%s", img->type);
    fscanf(f, "%d %d", &(img->width), &(img->height));
    fscanf(f, "%hhu\n", &(img->maxval));

    /* check if the image is color or grayscale: */
    if(img->type[1] == '5')
        colors = 1;
    else
        colors = 3;

    /* aloc space and read pixel matrix: */
    img->pixelmatrix = (unsigned char**)malloc((img->height)*sizeof(unsigned char*));
    for(i = 0  ; i < img->height ; i++) {
        (img->pixelmatrix)[i] = (unsigned char*)malloc((img->width)*sizeof(unsigned char)*colors);
        fread((void*)((img->pixelmatrix)[i]), 1, (img->width)*sizeof(unsigned char)*colors, f);
    }

    fclose(f);
}

void writeData(const char * fileName, image *img) {

    /* open file: */
    FILE *f = fopen(fileName, "w+");
    int colors, i;

    /* write image header: */
    fprintf(f, "%s\n", img->type);
    fprintf(f, "%d %d\n", img->width, img->height);
    fprintf(f, "%hhu\n", img->maxval);

    /* check if the image is color or grayscale: */
    if(img->type[1] == '5')
        colors = 1;
    else
        colors = 3;

    /* write pixel matrix: */
    for (i = 0 ; i < img->height ; i++) {
        fwrite((void*)(img->pixelmatrix[i]), 1, img->width*colors, f);
    }

    /* free space: */
    for(i = 0  ; i < img->height ; i++)
        free((img->pixelmatrix)[i]);
    free(img->pixelmatrix);

    /* close file: */
    fclose(f);

}

void* threadFunction(void *var) {

    int thread_id = ((thread_struct*)var)->thread_id;
    image *in = ((thread_struct*)var)->in;
    image *out = ((thread_struct*)var)->out;

    unsigned char gauss[3][3] = {{1,2,1},{2,4,2},{1,2,1}};
    int colors, i, j, k, l, sum, aux, aux2;

    /* check if the image is color or grayscale: */
    if(in->type[1] == '5')
        colors = 1;
    else
        colors = 3;

    /* split image: */
    int start = thread_id*(out->height/num_threads);
    int end = (thread_id+1)*(out->height/num_threads);

    if(thread_id == num_threads - 1) {
        end = out->height;
    }

    /* alloc space: */
    for (i = start ; i < end ; i++)
        (out->pixelmatrix)[i] = (unsigned char*)malloc((out->width)*sizeof(unsigned char)*colors);

    /* set resized image: */
    for (i = start ; i < end ; i++) {
        aux = i*resize_factor;
        for (j = 0 ; j < out->width*colors ; j++) {

            /* please don't ask me how did I get to this formula */
            aux2 = (j - j%colors)*resize_factor + j%colors;
            /* I don't think I know either */
		
            if (resize_factor %2 == 0) {
                sum = 0;

                /* small matrix of size res_fact*res_fact becomes one single pixel: */
                for (k = 0 ; k < resize_factor ; k++)
                    for (l = 0 ; l < resize_factor ; l++)
                        sum += in->pixelmatrix[aux+k][aux2+l*colors];
				
                /* replace pixel: */
                out->pixelmatrix[i][j] = sum/(resize_factor*resize_factor);
            }
            else /* for res_fact = 3: */
            {
                sum = 0;
                /* small matrix of size res_fact*res_fact becomes one single pixel: */
                for (k = 0 ; k < resize_factor ; k++)
                    for (l = 0 ; l < resize_factor ; l++)
                        sum += in->pixelmatrix[aux+k][aux2+l*colors] * gauss[k][l];
				
                /* replace pixel: */
                out->pixelmatrix[i][j] = sum/16;
            }
			
        }
    }
	
    return var;

}

void resize(image *in, image * out) {

    int i;
    pthread_t tid[num_threads];
    thread_struct threads[num_threads];

    /* set new header: */
    out->type[0] = in->type[0];
    out->type[1] = in->type[1];
    out->type[2] = in->type[2];

    out->width = in->width / resize_factor;
    out->height = in->height / resize_factor;
    out->maxval = in->maxval;
	
    /* split resize on threads: */	
    for(i = 0 ; i < num_threads ; i++) {
        threads[i].thread_id = i;
        threads[i].in = in;
        threads[i].out = out;
    }

    /* alloc space for matrix lines: */
    out->pixelmatrix = (unsigned char**)malloc((out->height)*sizeof(unsigned char*));
	
    for(i = 0 ; i < num_threads ; i++) {
        pthread_create(&(tid[i]), NULL, threadFunction, &(threads[i]));
    }
	
    for(i = 0 ; i < num_threads ; i++) {
        pthread_join(tid[i], NULL);
    }	
	
    /* free space for input image: */
    for(i = 0  ; i < in->height ; i++)
        free((in->pixelmatrix)[i]);
    free(in->pixelmatrix);

}
