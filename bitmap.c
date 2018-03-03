/*
 * Sample program to write out a 24 bit bitmap image
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITCOUNT 24

struct bitmapHeader {
  char bfType[2];
  int fileSize; //size of file in bytes
  int reserved; //always 0
  int imageOffset; //byte offset to image data (54 bytes)
  int headerSize; //size of this info header in bytes (40)
  int width; //width of the image in pixels
  int height; //height of the image in pixels
  short planes; //number of planes in target rendering location (usually 1)
  short bitCount; //bits per pixel: since we're dealing with 24 bit images this will be 24
  int compression; //set to 0, no compression
  int imageSize; //size of image in bytes
  int xPixelsPerMeter; //resolution of pixels/meter of display width
  int yPixelsPerMeter; //resolution of pixels/meter of display height
  int colorsUsed; //number of colors in our color pallet, 0 indicates all colors the bitdepth can support
  int colorsImportant; //number of important colors, 0 for all colors to be important

  //setting packed attribute to prevent the compiler from optimizing the struct by adding extra bytes
  //We need this header to remain exactly 54 bytes for writing out to file.
} __attribute__ ((packed));


struct bitmapData {
  struct bitmapHeader header;
  int *pixelBuffer;
};


/*
 * BMP_BYTES_PER_LINE:
 *
 * Calculate how many bytes each line is going to use
 *
 * Since we don't have a datatype that stores 24 bits exactly for each color,
 * we need to use the next largest type which is 32 bits.
 *
 * So we will calculate how many bits are required per row,
 * and round it to the nearest supported number of bytes that
 * we can represent using 32 bits
 *
 *
 * (((width * bits) + 31) / 32) * 4
 * e.g.:
 *
 * 175 pixels * 24 bits = 4200 bits = 525 bytes
 *
 * Next we round it to the nearest 32 bit size by adding 31 and dividing by 32 and letting
 * integer math truncate any remainder value
 *
 * (4200 + 31)//32 = 132
 * now that's how many 32 bit integers we need to represent 1 row of 24 bit colors
 * We can multiply that by 4 to get the size in bytes since 32 bits are 4 bytes each
 *
 * 132 * 4 = 528 bytes
*/

#define BMP_BYTES_PER_LINE(width, bits) (((((width) * (bits)) + 31) / 32) * 4)


/*
 * BMP_GET_PIXEL:
 * Our bixel buffer is only a one dimensional array, so to access it as if
 * it was an x,y grid, we can do some simple math.
 */
#define GET_PIXEL_INDEX(x, y, width) ((x) + ((y)*(width)))

/*
 * Some bit math to pack in red,green, and blue values into a 32 bit integer.
 * Values range from 0 - 255 (0x00 - 0xFF)
 *
 */
#define PIXCOLOR(r,g,b) ((r) | ((g)<<8) | ((b)<<16) )



//Write the bitmap data to disk!
int writeBitmapData(FILE *output, struct bitmapData *bitmap) {

  int bytesPerRow = BMP_BYTES_PER_LINE(bitmap->header.width, BITCOUNT);
  int imageFullSize = bytesPerRow * bitmap->header.height;

  //populate the header data with what we can

  //set the magic string in the bitmap header. This is what operating systems (among many other programs)
  //look for to determine the type of file it is handling
  strncpy(bitmap->header.bfType, "BM", 2);
  //header size is always 40
  bitmap->header.headerSize = 40;
  //this should be 54...
  bitmap->header.imageOffset = sizeof(struct bitmapHeader);
  bitmap->header.planes = 1;
  bitmap->header.bitCount = BITCOUNT;
  bitmap->header.reserved = 0;
  bitmap->header.colorsImportant = 0;
  bitmap->header.colorsUsed = 0;
  bitmap->header.imageSize = imageFullSize;
  bitmap->header.fileSize = imageFullSize + bitmap->header.imageOffset;
  bitmap->header.xPixelsPerMeter = 0;
  bitmap->header.yPixelsPerMeter = 0;

  //write out the header details
  fwrite(&bitmap->header, sizeof(struct bitmapHeader), 1, output);

  //using a newer C standard, we can just allocate an array on the stack automagically!
  unsigned char lineBuffer[bytesPerRow];

  //bitmaps are stored upside down, so we'll go backwards in our loop
  //As we loop through every row, we're going to grab each pixel in each row
  //and write it to our line buffer, which will then be written to the bitmap file
  //after the row has been processed
  for (int y = bitmap->header.height - 1; y >= 0; y--) {
    for (int x = 0; x < bitmap->header.width; x++) {
      //since we've stored our image in 4 Byte integers, and we're dealing with 24 bit colors,
      //the color values are only found in the first 3 bytes of our pixel integers
      //We need to pick out these color values, and "pack" them in our line buffer.
      //The idea is we don't need to write out any extra bytes and bits that aren't used
      //to represent the actual data on disk.
      int currentPixel = bitmap->pixelBuffer[GET_PIXEL_INDEX(x, y, bitmap->header.width)];
      //Since each color is only 3 bytes, we calculate our offset for the current pixel in the row
      //by multiplying x by 3 bytes
      int lineStart = x * 3;

      //Opting to access the color data on a byte level basis rather than doing bit magic for simplicity sake
      unsigned char *pixelAsBytes = (unsigned char *)&currentPixel;
      lineBuffer[lineStart] = pixelAsBytes[0];
      lineBuffer[lineStart + 1] = pixelAsBytes[1];
      lineBuffer[lineStart + 2] = pixelAsBytes[2];

    }
    //write out the line buffer
    fwrite(lineBuffer, 1, bytesPerRow, output);
  }

  return 0;
}


int saveBitmap(char *filename, struct bitmapData *bitmap) {
  FILE *output = fopen(filename, "w");
  if (!output) {
    printf("Failed to open file: %s\n", filename);
    return -1;
  }

  writeBitmapData(output, bitmap);
  fclose(output);
  return 0;
}

struct bitmapData *newBitmap(int width, int height) {

  struct bitmapData *bitmap = calloc(1, sizeof(struct bitmapData));
  if (!bitmap) {
    printf("Error allocating new bitmap struct\n");
    return NULL;
  }

  bitmap->header.width = width;
  bitmap->header.height = height;

  //allocate enough space to draw on!
  int numberOfPixels = bitmap->header.width * bitmap->header.height;
  bitmap->pixelBuffer = calloc(numberOfPixels, sizeof(int));

  if (!bitmap->pixelBuffer) {
    printf("Failed to allocate memory for pixel buffer :(\n");
    free(bitmap);
    return NULL;
  }

  return bitmap;
}

void freeBitmap(struct bitmapData *bitmap) {
  if (!bitmap) return;

  if (bitmap->pixelBuffer) free(bitmap->pixelBuffer);
  free(bitmap);
}

void putPixel(struct bitmapData *bitmap, int x, int y, int red, int green, int blue) {
  bitmap->pixelBuffer[GET_PIXEL_INDEX(x, y, bitmap->header.width)] = PIXCOLOR(red, green, blue);
}


//Bresenham’s circle drawing algorithm
//source: https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
void drawCircle(struct bitmapData *bitmap, int xc, int yc, int x, int y)
{

  int red = (x % 255);
  int green = (y % 255);
  int blue = (255 - (x % 255));

  putPixel(bitmap, xc+x, yc+y, red, green, blue);
  putPixel(bitmap, xc-x, yc+y, red, green, blue);
  putPixel(bitmap, xc+x, yc-y, red, green, blue);
  putPixel(bitmap, xc-x, yc-y, red, green, blue);
  putPixel(bitmap, xc+y, yc+x, red, green, blue);
  putPixel(bitmap, xc-y, yc+x, red, green, blue);
  putPixel(bitmap, xc+y, yc-x, red, green, blue);
  putPixel(bitmap, xc-y, yc-x, red, green, blue);
}

// Function for circle-generation
// using Bresenham's algorithm
void circleBres(struct bitmapData *bitmap, int xc, int yc, int r)
{
  int x = 0, y = r;
  int d = 3 - 2 * r;
  while (y >= x)
  {
    // for each pixel we will
    // draw all eight pixels
    drawCircle(bitmap, xc, yc, x, y);
    x++;

    // check for decision parameter
    // and correspondingly
    // update d, x, y
    if (d > 0)
    {
      y--;
      d = d + 4 * (x - y) + 10;
    }
    else
      d = d + 4 * x + 6;
    drawCircle(bitmap, xc, yc, x, y);
  }
}


int main() {
  int prgmStatus = 0;
  int imgWidth = 1024;
  int imgHeight = 1024;

  struct bitmapData *myPicture = newBitmap(imgWidth, imgHeight);
  if (!myPicture) {
    return -1;
  }

  int radiusStart = (imgHeight > imgWidth) ? imgHeight/2 : imgWidth/2;
  while (--radiusStart > 0) {
    circleBres(myPicture, imgWidth/2, imgHeight/2, radiusStart);
  }

  prgmStatus = saveBitmap("myBitmap.bmp", myPicture);
  freeBitmap(myPicture);
  return prgmStatus;
}
