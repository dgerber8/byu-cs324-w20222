/*
  This program is an adaptation of the Mandelbrot program
  from the Programming Rosetta Stone, see
  http://rosettacode.org/wiki/Mandelbrot_set

  Compile the program with:

  gcc -o mandelbrot -O4 mandelbrot.c

  Usage:
 
  ./mandelbrot <xmin> <xmax> <ymin> <ymax> <maxiter> <xres> <out.ppm>

  Example:

  ./mandelbrot 0.27085 0.27100 0.004640 0.004810 1000 1024 pic.ppm

  The interior of Mandelbrot set is black, the levels are gray.
  If you have very many levels, the picture is likely going to be quite
  dark. You can postprocess it to fix the palette. For instance,
  with ImageMagick you can do (assuming the picture was saved to pic.ppm):

  convert -normalize pic.ppm pic.png

  The resulting pic.png is still gray, but the levels will be nicer. You
  can also add colors, for instance:

  convert -negate -normalize -fill blue -tint 100 pic.ppm pic.png

  See http://www.imagemagick.org/Usage/color_mods/ for what ImageMagick
  can do. It can do a lot.


OMP_NUM_THREADS		elapsed time for only the parallel region	elapsed time for the entire program execution
1					29.661490					35.27
2					16.161815					24.28
4					8.917262					15.00
8					4.891248					11.00
16					3.361341					9.51
32					2.465898					8.54

1. How many cores are available for computation on your machine?

20


2. What happens to the time associated with computation of only the parallel region (i.e., the first for loop) as the number of threads doubles?

It roughly halves in the beginning but as cores increase, the difference trends less substantial


3. What is the speedup (α) of the only the parallel region (i.e., the first for loop) when four threads are used? Hint: calculate Sp for just the parallel region, with p = 4.

29.661490/8.917262 = 3.3263


4. At what point (i.e., how many threads) did you stop observing the expected performance gain in the parallel region of the code?

Between 16 and 32


5. At the point you indicated in #4, what was the reason for the lack of additional performance gain?

The performance gain is much less significant between 16 and 32 because there are only 20 available cores on the machine.


6. What is the overall speedup when four threads ae3re used? Hint: calculate Sp for the overall execution time of the program (i.e., using the "elapsed time" output by time), with 
   p = 4. Show the steps you used to calculate it.

35.27/15.00 = 2.3513


7. Using the result from #6, compute the efficiency (Ep) of using four cores (i.e., p = 4) to parallelize with respect to overall execution time. Show the steps you used to calculate it.

2.3513//4 = 0.588


8. Briefly explain why the efficiency calculated in #7 is less than 1.

For the efficiency to be equal to or greater than 1, the runtime would need at least halve each time we double our cores which isn't the case. Partially because we have to also consider
our non-parallelizable runtime which doesn't change with the number of cores. It's also partially b/c of Amdahls Law.


9. Consider Amdahl's Law: Tα = pT/α + (1-p)T
In this case α is speedup of the parallel region only, and p is the fraction of original run time that is parallelizable.
Find the fraction of parallelizable code, p, by using:
the answer to #3, α (speedup of parallel region);
the "elapsed time" output by time for 4 threads as Tα; and
the "elapsed time" output by time for 1 thread as T.

15.00 = p*(35.27)/3.3263+(1-(p))*(35.27)
15/35.27 = p/3.3263+(1-p)
0.435 = 1 - (2.3263*p)/3.3263
-0.565 * -3.3263/2.3263 = p
p = 0.807


10. Using the result from #9, as the number of threads grows indefinitely (α approaches infinity), what does Tα approach?

Tα approaches (1-p)*T because as α approaches infinity, the (pT/α) half of the equation approaches 0.


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <omp.h>

int main(int argc, char* argv[])
{
  /* Parse the command line arguments. */
  if (argc != 8) {
    printf("Usage:   %s <xmin> <xmax> <ymin> <ymax> <maxiter> <xres> <out.ppm>\n", argv[0]);
    printf("Example: %s 0.27085 0.27100 0.004640 0.004810 1000 1024 pic.ppm\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* The window in the plane. */
  const double xmin = atof(argv[1]);
  const double xmax = atof(argv[2]);
  const double ymin = atof(argv[3]);
  const double ymax = atof(argv[4]);

  /* Maximum number of iterations, at most 65535. */
  const uint16_t maxiter = (unsigned short)atoi(argv[5]);

  /* Image size, width is given, height is computed. */
  const int xres = atoi(argv[6]);
  const int yres = (xres*(ymax-ymin))/(xmax-xmin);

  /* The output file name */
  const char* filename = argv[7];

  /* Open the file and write the header. */
  FILE * fp = fopen(filename,"wb");
  char *comment="# Mandelbrot set";/* comment should start with # */

  /*write ASCII header to the file*/
  fprintf(fp,
          "P6\n# Mandelbrot, xmin=%lf, xmax=%lf, ymin=%lf, ymax=%lf, maxiter=%d\n%d\n%d\n%d\n",
          xmin, xmax, ymin, ymax, maxiter, xres, yres, (maxiter < 256 ? 256 : maxiter));

  /* Precompute pixel width and height. */
  double dx=(xmax-xmin)/xres;
  double dy=(ymax-ymin)/yres;

  double x, y; /* Coordinates of the current point in the complex plane. */
  double u, v; /* Coordinates of the iterated point. */
  int i,j; /* Pixel counters */
  int k; /* Iteration counter */
  int *saved = malloc(sizeof(int)*yres*xres);

  double start;
  double end;
  start = omp_get_wtime();

#pragma omp parallel for private(i, j, k, x, y) firstprivate(saved)
  for (j = 0; j < yres; j++) {
    y = ymax - j * dy;
    for(i = 0; i < xres; i++) {
      double u = 0.0;
      double v= 0.0;
      double u2 = u * u;
      double v2 = v*v;
      x = xmin + i * dx;
      /* iterate the point */
      for (k = 1; k < maxiter && (u2 + v2 < 4.0); k++) {
            v = 2 * u * v + y;
            u = u2 - v2 + x;
            u2 = u * u;
            v2 = v * v;
      };
      saved[xres * j + i] = k;
    }
  }

  end = omp_get_wtime();
  printf("The for loop took %f (%f) seconds\n", end - start, omp_get_wtime());

  for (j = 0; j < yres; j++) {
    for(i = 0; i < xres; i++) {
      /* compute  pixel color and write it to file */
      k = saved[xres * j + i];
      if (k >= maxiter) {
        /* interior */
        const unsigned char black[] = {0, 0, 0, 0, 0, 0};
        fwrite (black, 6, 1, fp);
      }
      else {
        /* exterior */
        unsigned char color[6];
        color[0] = k >> 8;
        color[1] = k & 255;
        color[2] = k >> 8;
        color[3] = k & 255;
        color[4] = k >> 8;
        color[5] = k & 255;
        fwrite(color, 6, 1, fp);
      };
    }
  }
  fclose(fp);
  free(saved);
  return 0;
}
