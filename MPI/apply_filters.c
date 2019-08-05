#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* pnm image structure: */
typedef struct {

    char type[3];
    int width, height;
    int start, end;
    unsigned char **pixelmatrix;

} image;

/* apply one filter: */
void applyFilter(image *img1, image *img2, float filter[3][3]) {

    int i, j, k, l, colors;
    float accumulator;
    
    if (img1->type[1] == '5')
        colors = 1;
    else
        colors = 3;

    for (i = 0 ; i < img1->end ; i++) {
    
        for (j = 0 ; j < img1->width*colors ; j++) {
        
            /* leave border untouched: */
            if ((i == 0) || (i == img1->end-1) ||
                (j < colors) || (j >= (img1->width-1)*colors)) {

                img2->pixelmatrix[i][j] = img1->pixelmatrix[i][j];
            } else { 
                accumulator = 0;
            
                /* apply filter on pixel: */
                for (k = 0 ; k < 3 ; k++)
                    for (l = 0 ; l < 3 ; l++)
                        accumulator += filter[k][l]*img1->pixelmatrix[i - 1 + k][j - colors + l*colors];
                
                /* replace pixel: */
                img2->pixelmatrix[i][j] = (unsigned char)(accumulator);
            
            }
        }
    }
}

int main(int argc, char** argv) {
    
    int rank, nProcesses;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    /* images: */
    image *original, *modified;
    original = (image*)malloc(sizeof(image));
    modified = (image*)malloc(sizeof(image));

    /* auxiliary pointer used to exchange pixel matrices after applying filter: */
    void *aux;

    char type[3];
    int width, height;
    char maxval; 
    int i, j, colors, start, end;

    /* filter kernels: */
    float smooth[3][3] = {{1.0f/9, 1.0f/9, 1.0f/9},{1.0f/9, 1.0f/9, 1.0f/9},{1.0f/9, 1.0f/9, 1.0f/9}};
    float blur[3][3] = {{1.0f/16, 2.0f/16, 1.0f/16}, {2.0f/16, 4.0f/16, 2.0f/16}, {1.0f/16, 2.0f/16, 1.0f/16}};
    float sharpen[3][3] = {{0, -2.0f/3, 0}, {-2.0f/3, 11.0f/3, -2.0f/3}, {0, -2.0f/3, 0}};
    float mean[3][3] = {{-1.0f, -1.0f, -1.0f}, {-1.0f, 9.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}};
    float emboss[3][3] = {{0, 1.0f, 0}, {0, 0, 0}, {0, -1.0f, 0}};

    /* the first process reads the image from file and sends it to
     * all of the other processes: */
    if (rank == 0) {

        /* open input file: */
        FILE *f = fopen(argv[1], "r+");

        if (f == NULL)
            return -1;

        /* read image header: */
        fscanf(f, "%s\n", type);
        fscanf(f, "%d %d\n", &width, &height);
        fscanf(f, "%hhu\n", &maxval);

        /* broadcast image header: */
        MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&type, 3, MPI_CHAR, 0, MPI_COMM_WORLD);
	
        /* check if image is color or grayscale: */
        if (type[1] == '5')
            colors = 1;
        else
            colors = 3;

        /* alloc space for initial pixel matrix: */
        original->pixelmatrix = (unsigned char**)malloc(height*sizeof(unsigned char*));
        for(i = 0  ; i < height ; i++) {
            original->pixelmatrix[i] = (unsigned char*)malloc(width*colors*sizeof(unsigned char));
        }

        /* read matrix: */
        for (i = 0 ; i < height ; i++)
            fread((void*)(original->pixelmatrix[i]), 1, colors*width*sizeof(unsigned char), f);

        /* close input file: */
        fclose(f);

        /* split image between processes and send it line by line: */
        for (i = 1 ; i < nProcesses ; i++) {
            
            start = i*(height/nProcesses) - 1;
            end = (i+1)*(height/nProcesses) + 1;

            if (i == nProcesses-1)
                end = height;

            for (j = start ; j < end ; j++)
                MPI_Send(original->pixelmatrix[j], width*colors, MPI_CHAR, i, j-start, MPI_COMM_WORLD);

        }

        /* set start and end for applying filter: */
        original->start = 0;
        original->end = (height/nProcesses) + 1;

        if (nProcesses == 1)
            original->end = height;

        /* alloc space for modified pixel matrix: */
        modified->pixelmatrix = (unsigned char**)malloc(height*sizeof(unsigned char*));
        for(i = 0  ; i < height ; i++) {
            modified->pixelmatrix[i] = (unsigned char*)malloc(width*colors*sizeof(unsigned char));
        }


    } else { /* if the process rank is not 0, receive the header: */

        MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&type, 3, MPI_CHAR, 0, MPI_COMM_WORLD);

    }

    original->type[0] = type[0];
    original->type[1] = type[1];
    original->type[2] = type[2];

    original->width = width;
    original->height = height;

    if (type[1] == '5')
        colors = 1;
    else
        colors = 3;

    if (rank != 0) { /* if the process rank is not 0, receive part of the image: */

        end = (height/nProcesses + 2);

        if (rank == nProcesses-1)
            end = height - (rank)*(height/nProcesses) + 1;

       /*  alloc space for received matrix & receive matrix from process 0 line by line: */
        original->pixelmatrix = (unsigned char**)malloc(end*sizeof(unsigned char*));

        for (i = 0 ; i < end ; i++) {

            original->pixelmatrix[i] = (unsigned char*)malloc(width*colors*sizeof(unsigned char));
            MPI_Recv(original->pixelmatrix[i], width*colors, MPI_CHAR, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        }

        /* alloc space for modified matrix */
        modified->pixelmatrix = (unsigned char**)malloc(end*sizeof(unsigned char*));
        for (i = 0 ; i < end ; i++) {

            modified->pixelmatrix[i] = (unsigned char*)malloc(width*colors*sizeof(unsigned char));

        }

        original->end = end;

    }

    /* apply all filters: */
    for (i = 2 ; i < argc ; i++) {

        if (strcmp(argv[i], "smooth") == 0) {

            /* apply filter: */
            applyFilter(original, modified, smooth);

            /* replace pixel matrix with the filtered one: */
            aux = original->pixelmatrix;
            original->pixelmatrix = modified->pixelmatrix;
            modified->pixelmatrix = aux;
        }

        if (strcmp(argv[i], "blur") == 0) {

            /* apply filter: */
            applyFilter(original, modified, blur);

            /* replace pixel matrix with the filtered one: */
            aux = original->pixelmatrix;
            original->pixelmatrix = modified->pixelmatrix;
            modified->pixelmatrix = aux;
        }

        if (strcmp(argv[i], "sharpen") == 0) {

            /* apply filter: */
            applyFilter(original, modified, sharpen);

            /* replace pixel matrix with the filtered one: */
            aux = original->pixelmatrix;
            original->pixelmatrix = modified->pixelmatrix;
            modified->pixelmatrix = aux;
        }

        if (strcmp(argv[i], "mean") == 0) {

            /* apply filter: */
            applyFilter(original, modified, mean);

            /* replace pixel matrix with the filtered one: */
            aux = original->pixelmatrix;
            original->pixelmatrix = modified->pixelmatrix;
            modified->pixelmatrix = aux;
        }

        if (strcmp(argv[i], "emboss") == 0) {

            /* apply filter: */
            applyFilter(original, modified, emboss);

            /* replace pixel matrix with the filtered one: */
            aux = original->pixelmatrix;
            original->pixelmatrix = modified->pixelmatrix;
            modified->pixelmatrix = aux;
        }

	    /* send and receive recomputed lines: */
        if (nProcesses > 1) {

            if (rank == 0) {

                /* exchange bottom lines for first process: */
                MPI_Sendrecv(original->pixelmatrix[height/nProcesses-1], width*colors, MPI_CHAR, 1, 0,
                             original->pixelmatrix[height/nProcesses], width*colors, MPI_CHAR, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            } else if ((rank == nProcesses-1) && (rank != 0)) {

                /* exchange first lines for last process: */
                MPI_Sendrecv(original->pixelmatrix[1], width*colors, MPI_CHAR, rank-1, 1,
                             original->pixelmatrix[0], width*colors, MPI_CHAR, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            } else {

                /* exchange first and bottom lines for middle processes: */
                MPI_Sendrecv(original->pixelmatrix[1], width*colors, MPI_CHAR, rank-1, 1,
                             original->pixelmatrix[0], width*colors, MPI_CHAR, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Sendrecv(original->pixelmatrix[height/nProcesses], width*colors, MPI_CHAR, rank+1, 0,
                             original->pixelmatrix[height/nProcesses+1], width*colors, MPI_CHAR, rank+1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            }

        }

    }

    if (rank == 0) {

        /* open output file: */
        FILE *g = fopen(argv[2], "w+");

        if (g == NULL) {
            return -1;
        }

        /* receive all other parts of image */
        for (i = 1 ; i < nProcesses ; i++) {
            
            start = i*(height/nProcesses);
            end = (i+1)*(height/nProcesses);

            if (i == nProcesses-1)
                end = height;

            for (j = start ; j < end ; j++) {

                MPI_Recv(original->pixelmatrix[j], width*colors, MPI_CHAR, i, j-start, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            }
        }

        /* print header: */
        fprintf(g, "%s\n", type);
        fprintf(g, "%d %d\n", width, height);
        fprintf(g, "%hhu\n", maxval);

        /* print matrix: */
        for(i = 0 ; i < height ; i++)
            fwrite((void*)(original->pixelmatrix[i]), 1, width*colors*sizeof(unsigned char), g);

        /* free space for both images: */
        for(i = 0  ; i < height ; i++) {
            free(original->pixelmatrix[i]);
            free(modified->pixelmatrix[i]);
        }
        free(original->pixelmatrix);
        free(modified->pixelmatrix);

        /* close file: */
        fclose(g);

    }
    else {

        end = (height/nProcesses + 1);

        if (rank == nProcesses-1)
            end = height - (rank)*(height/nProcesses) + 1;

        /* send modified pixel matrix to process 0: */
        for (i = 1 ; i < end ; i++) {

            MPI_Send(original->pixelmatrix[i], width*colors, MPI_CHAR, 0, i-1, MPI_COMM_WORLD);

        }

        /* free space for both images (received and modified): */
        for (i = 0 ; i < end ; i++) {
            free(original->pixelmatrix[i]);
            free(modified->pixelmatrix[i]);
        }
        free(original->pixelmatrix);
        free(modified->pixelmatrix);

    }

    free(original);
    free(modified);

    MPI_Finalize();

    return 0;

}

