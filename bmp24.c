#include "bmp24.h"
#include <stdlib.h>
#include <string.h>

t_pixel ** bmp24_allocateDataPixels (int width, int height) {
    t_pixel **pixels = (t_pixel **)malloc(sizeof(t_pixel *) * height);
    if (pixels == NULL) {
        printf("Not enough memory to allocate");
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        pixels[i] = (t_pixel*)malloc(width * sizeof(t_pixel));
        if (pixels[i]==NULL) {
            printf("No enough memory to allocate");
            for (int j = 0; j < i; j++) {
                free(pixels[j]);
            }
            return NULL;
        }
    }
    return pixels;
}

void bmp24_freeDataPixels (t_pixel ** pixels, int height) {
    for (int i = 0; i < height; i++) {
        free(pixels[i]);
    }
    free(pixels);
}

t_bmp24 * bmp24_allocate (int width, int height, int colorDepth) {
    t_bmp24 *image = (t_bmp24*)malloc(sizeof(t_bmp24));
    if (image == NULL) {
        printf("Memory allocation failed");
        return NULL;
    }

    image->colorDepth = colorDepth;
    image->data = bmp24_allocateDataPixels(width , height);
    image->height = height;
    image->width = width;

    if (image->data == NULL) {
        printf("Couldn't alocate data");
        free(image);
        return NULL;
    }
    return image;
}

void bmp24_free (t_bmp24 * img) {
    bmp24_freeDataPixels(img->data, img->height);
    free(img);
}


void file_rawRead (uint32_t position, void * buffer, uint32_t size, size_t n, FILE * file) {
    fseek(file, position, SEEK_SET);
    fread(buffer, size, n, file);
}

void file_rawWrite (uint32_t position, void * buffer, uint32_t size, size_t n, FILE * file) {
    fseek(file, position, SEEK_SET);
    fwrite(buffer, size, n, file);
}

//Function that read the pixel value (calulate, move the cursor and the read and store)
void bmp24_readPixelValue(t_bmp24 *image, int x, int y, FILE *file) {
    long offset = image->header.offset + ((image->height - 1 - y) * image->width + x) * 3;
    fseek(file, offset, SEEK_SET);
    unsigned char buffer[3];
    fread(buffer, 1, 3, file);
    image->data[y][x].blue = buffer[0];
    image->data[y][x].green = buffer[1];
    image->data[y][x].red = buffer[2];
}

//basic loop that goes line by line to read the pixel data
void bmp24_readPixelData(t_bmp24 *image, FILE *file) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            bmp24_readPixelValue(image, x, y, file);
        }
    }
}

//functoin that write into the file the data (calculate, move, and write)
void bmp24_writePixelValue(t_bmp24 *image, int x, int y, FILE *file) {
    long offset = image->header.offset + ((image->height - 1 - y) * image->width + x) * 3;
    fseek(file, offset, SEEK_SET);
    unsigned char buf[3];
    buf[0] = image->data[y][x].blue;
    buf[1] = image->data[y][x].green;
    buf[2] = image->data[y][x].red;
    fwrite(buf, 1, 3, file);
}

//basic loop that goes pixel by pixel to write the pixel value at position x,y
void bmp24_writePixelData(t_bmp24 *image, FILE *file) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            bmp24_writePixelValue(image, x, y, file);
        }
    }
}

//Function taht is used to load the image
t_bmp24 *bmp24_loadImage(const char *filename) {

    FILE *file = fopen(filename, "rb");
    t_bmp24 *image = malloc(sizeof(t_bmp24));

    //temporary
    t_bmp_header header;
    t_bmp_info header_info;

    //check if the file exits
    if (file == NULL) {
        printf("Error while opening the file, the file does not exist!\n");
        return NULL;
    }

    //read the widht, height and colordept od the image
    int width = header_info.width;
    int height = header_info.height;
    int colorDepth = header_info.bits;

    //add a test to verify that this in of course a 24 color dept
    if (image->colorDepth != 24) {
        printf("The image is not 24 bits deep");
    }

    //allocate memory using previous functions
    t_bmp24 *image =  bmp24_allocate(width, height,colorDepth);

    //read teh header and header info
    file_rawRead(0, &header, sizeof(t_bmp_header), 1, file); 
    file_rawRead(sizeof(t_bmp_header), &header_info, sizeof(t_bmp_info), 1, file);
    //add them in the image header and header_info
    image->header = header;
    image->header_info = header_info;

    //read the pixel data
    bmp24_readPixelData(image, file);

    //close and return the loaded image
    fclose(file);
    return image;
}


//Function to save the image 
void bmp24_saveImage (t_bmp24 * img, const char * filename){
    FILE *file = fopen(filename, "wb");

    //test to verify that the fil is correct
    if (file == NULL){
        printf("error while opening the file (doesn't exits)");
        return;
    }


    //write header and the iage data
    file_rawWrite(0, &(img->header), sizeof(t_bmp_header), 1, file);
    file_rawWrite(sizeof(t_bmp_header), &(img->header_info), sizeof(t_bmp_info), 1, file);

    //close the file
    fclose(file);
}


//Function to apply a negative filter (simple loop and just remplace)
void bmp24_negative(t_bmp24 *img) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            img->data[y][x].red   = 255 - img->data[y][x].red;
            img->data[y][x].green = 255 - img->data[y][x].green;
            img->data[y][x].blue  = 255 - img->data[y][x].blue;
        }
    }
}


//>Function to apply the grayscale filter (simple loop and just calculate the new pixels)
void bmp24_grayscale(t_bmp24 *img) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            //average computation
            unsigned char avg = (img->data[y][x].red +
                                 img->data[y][x].green +
                                 img->data[y][x].blue) / 3;
            img->data[y][x].red   = avg;
            img->data[y][x].green = avg;
            img->data[y][x].blue  = avg;
        }
    }
}


//Function that apply the brightness filter
void bmp24_brightness(t_bmp24 *img, int value) {
    int y, x;
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            int r = img->data[y][x].red + value;
            int g = img->data[y][x].green + value;
            int b = img->data[y][x].blue + value;

            //for red
            if (r > 255)
                img->data[y][x].red = 255;
            else if (r < 0)
                img->data[y][x].red = 0;
            else
                img->data[y][x].red = r;

            //for green
            if (g > 255)
                img->data[y][x].green = 255;
            else if (g < 0)
                img->data[y][x].green = 0;
            else
                img->data[y][x].green = g;

            //for blue
            if (b > 255)
                img->data[y][x].blue = 255;
            else if (b < 0)
                img->data[y][x].blue = 0;
            else
                img->data[y][x].blue = b;
        }
    }
}


//function to apply the convolution filter
t_pixel bmp24_convolution(t_bmp24 *img, int x, int y, float **kernel, int kernelSize) {
    //usefull variables
    int k, l, half = kernelSize/2;
    float sum_r = 0, sum_g = 0, sum_b = 0;

    //loop to go over the kernel
    for (k = -half; k <= half; k++) {
        for (l = -half; l <= half; l++) {
            int px = x + l;
            int py = y + k;

            //to be sure that there is no error while behind out of the image
            if (px < 0) 
                px = 0;
            if (py < 0) 
                py = 0;
            if (px >= img->width) 
                 px = img->width - 1;
            if (py >= img->height) 
                py = img->height - 1;
            
            //multiply the pixel by the kernel value
            sum_r += img->data[py][px].red * kernel[k + half][l + half];
            sum_g += img->data[py][px].green * kernel[k + half][l + half];
            sum_b += img->data[py][px].blue * kernel[k + half][l + half];
        }
    }

    t_pixel result;

    //apply the calculation of the sum to the pixel r,g,b (check if it 0 > and 255 <)
    if (sum_r < 0)
        result.red = 0;
    else if (sum_r > 255)
        result.red = 255;
    else
        result.red = (unsigned char)(sum_r + 0.5);

    if (sum_g < 0)
        result.green = 0;
    else if (sum_g > 255)
        result.green = 255;
    else
        result.green = (unsigned char)(sum_g + 0.5);

    if (sum_b < 0)
        result.blue = 0;
    else if (sum_b > 255)
        result.blue = 255;
    else
        result.blue = (unsigned char)(sum_b + 0.5);


    //return the result (obvioulsy)
    return result;
}