#include <zlib.h>
#include <png.h>
#include <string.h>
#include <stdlib.h>

void error_exit(char *msg)
{
    perror(msg);
    exit(1);
}

void Usage(char * exe)
{
    printf("Usage: %s input colorgrade output\nApplies a 16x256 colorgrade to an input image and creates a new file with the colorgraded image\n", exe);
    exit(1);
}

void create_read_png(png_structp *png_ptrp, png_infop *info_ptrp, png_infop *end_infop)
{
    *png_ptrp = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*png_ptrp) error_exit("png create read struct fail");

    *info_ptrp = png_create_info_struct(*png_ptrp);
    if (!*info_ptrp)
    {
        png_destroy_read_struct(png_ptrp, NULL, NULL);
        error_exit("png create info struct fail");
    }

    *end_infop = png_create_info_struct(*png_ptrp);
    if (!*end_infop)
    {
        png_destroy_read_struct(png_ptrp, info_ptrp, NULL);
        error_exit("png create info struct fail");
    }
}

void create_write_png(png_structp *png_ptrp, png_infop *info_ptrp, png_infop *end_infop)
{
    *png_ptrp = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*png_ptrp) error_exit("png create write struct fail");

    *info_ptrp = png_create_info_struct(*png_ptrp);
    if (!*info_ptrp)
    {
        png_destroy_write_struct(png_ptrp, NULL);
        error_exit("png create info struct fail");
    }
}

int main(int argc, char* argv[])
{
    if (argc != 4) Usage(argv[0]);

    png_structp png_ptr1;
    png_infop info_ptr1;
    png_infop end_info1;
    create_read_png(&png_ptr1, &info_ptr1, &end_info1);

    FILE *fp1 = fopen(argv[1], "rb");

    png_init_io(png_ptr1, fp1);
    
    png_read_png(png_ptr1, info_ptr1, 0, NULL);
    
    png_uint_32 width1, height1;
    int bit_depth1, color_type1, interlace_type1, compression_type1, filter_method1;
    png_get_IHDR(png_ptr1, info_ptr1, &width1, &height1,
        &bit_depth1, &color_type1, &interlace_type1,
        &compression_type1, &filter_method1);
        
    png_bytep *row_pointers1 = png_get_rows(png_ptr1, info_ptr1);

    png_structp png_ptr2;
    png_infop info_ptr2;
    png_infop end_info2;
    create_read_png(&png_ptr2, &info_ptr2, &end_info2);

    FILE *fp2 = fopen(argv[2], "rb");

    png_init_io(png_ptr2, fp2);
    
    png_read_png(png_ptr2, info_ptr2, 0, NULL);
    
    png_uint_32 width2, height2;
    int color_type2;
    png_get_IHDR(png_ptr2, info_ptr2, &width2, &height2, NULL, &color_type2, NULL, NULL, NULL);

    if (width2 != 256 || height2 != 16)
    {
        png_destroy_read_struct(&png_ptr1, &info_ptr1, &end_info1);
        png_destroy_read_struct(&png_ptr2, &info_ptr2, &end_info2);
        error_exit("Not a colorgrade! Wtf");
    }
        
    png_bytep *row_pointers2 = png_get_rows(png_ptr2, info_ptr2);

    int pixel_size1, pixel_size2;
    if (color_type1 == PNG_COLOR_TYPE_RGB) pixel_size1 = 3;
    else if (color_type1 == PNG_COLOR_TYPE_RGB_ALPHA) pixel_size1 = 4;
    else
    {
        png_destroy_read_struct(&png_ptr1, &info_ptr1, &end_info1);
        png_destroy_read_struct(&png_ptr2, &info_ptr2, &end_info2);
        error_exit("Invalid color type for input image");
    }
    if (color_type2 == PNG_COLOR_TYPE_RGB) pixel_size2 = 3;
    else if (color_type2 == PNG_COLOR_TYPE_RGB_ALPHA) pixel_size2 = 4;
    else
    {
        png_destroy_read_struct(&png_ptr1, &info_ptr1, &end_info1);
        png_destroy_read_struct(&png_ptr2, &info_ptr2, &end_info2);
        error_exit("Invalid color type for colorgrade image");
    }

    for (int i = 0; i < height1; i++)
    {
        for (int j = 0; j < pixel_size1*width1; j += pixel_size1)
        {
            int r_tile = row_pointers1[i][j] / 17;
            double r_over = (double)(row_pointers1[i][j] % 17) / 17.;
            if (r_tile == 15)
            {
                r_tile = 14;
                r_over = 1.;
            }
            int g_tile = row_pointers1[i][j+1] / 17;
            double g_over = (double)(row_pointers1[i][j+1] % 17) / 17.;
            if (g_tile == 15)
            {
                g_tile = 14;
                g_over = 1.;
            }
            int b_tile = row_pointers1[i][j+2] / 17;
            double b_over = (double)(row_pointers1[i][j+2] % 17) / 17.;
            if (b_tile == 15)
            {
                b_tile = 14;
                b_over = 1.;
            }

            for (int k = 0; k < 3; k++)
            {
                //printf("%4.3f, %4.3f, %4.3f, %d\n", r_over, g_over, b_over, (int)row_pointers2[g_tile  ][4*(16*b_tile    +r_tile  )+k]);
                row_pointers1[i][j+k] = (png_byte)(
                    (1 - r_over) * (1 - g_over) * (1 - b_over) * row_pointers2[g_tile  ][pixel_size2*(16*b_tile    +r_tile  )+k] +
                    (    r_over) * (1 - g_over) * (1 - b_over) * row_pointers2[g_tile  ][pixel_size2*(16*b_tile    +r_tile+1)+k] +
                    (1 - r_over) * (    g_over) * (1 - b_over) * row_pointers2[g_tile+1][pixel_size2*(16*b_tile    +r_tile  )+k] +
                    (    r_over) * (    g_over) * (1 - b_over) * row_pointers2[g_tile+1][pixel_size2*(16*b_tile    +r_tile+1)+k] +
                    (1 - r_over) * (1 - g_over) * (    b_over) * row_pointers2[g_tile  ][pixel_size2*(16*(b_tile+1)+r_tile  )+k] +
                    (    r_over) * (1 - g_over) * (    b_over) * row_pointers2[g_tile  ][pixel_size2*(16*(b_tile+1)+r_tile+1)+k] +
                    (1 - r_over) * (    g_over) * (    b_over) * row_pointers2[g_tile+1][pixel_size2*(16*(b_tile+1)+r_tile  )+k] +
                    (    r_over) * (    g_over) * (    b_over) * row_pointers2[g_tile+1][pixel_size2*(16*(b_tile+1)+r_tile+1)+k]
                );
            }
        }
    }
    
    png_structp png_ptr3;
    png_infop info_ptr3;
    png_infop end_info3;
    create_write_png(&png_ptr3, &info_ptr3, &end_info3);

    FILE *fp3 = fopen(argv[3], "wb");

    png_init_io(png_ptr3, fp3);

    png_set_IHDR(png_ptr3, info_ptr3, width1, height1,
        bit_depth1, color_type1, interlace_type1,
        compression_type1, filter_method1);

    png_set_rows(png_ptr3, info_ptr3, row_pointers1);
    png_write_png(png_ptr3, info_ptr3, 0, NULL);
}