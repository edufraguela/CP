/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1

#define          X_RESN  1024  /* x resolution */
#define          Y_RESN  1024 /* y resolution */

/* Boundaries of the mandelbrot set */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* More iterations -> more detailed image & higher computational cost */
#define   maxIterations  1000

typedef struct complextype
{
  float real, imag;
} Compl;

static inline double get_seconds(struct timeval t_ini, struct timeval t_end){
  return (t_end.tv_usec - t_ini.tv_usec) / 1E6 +
         (t_end.tv_sec - t_ini.tv_sec);
}

int main (int argc, char** argv){

	int numprocs, rank, flops=0, flops_total=0;
  //Cálculo de los incrementos de X e Y
	float pasoX = (X_MAX - X_MIN)/X_RESN;
	float pasoY = (Y_MAX - Y_MIN)/Y_RESN;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* Mandelbrot variables */
	int i, j, k;
  Compl z, c;
  float lengthsq, temp;
  int *vres, *res[Y_RESN];

	int filas_por_proceso = Y_RESN/numprocs;
  //En esta matriz guardamos las porciones de imagen que cada proceso calcula
	int res_parcial[filas_por_proceso+1][X_RESN];
  //Vector que almacena el nº de flops realizadas por cada proceso
	int flops_por_proceso[numprocs];

	int filas[numprocs], filaInicial[numprocs], count[numprocs], displ[numprocs];

	for(i=0; i<numprocs; i++){
		if( i < Y_RESN%numprocs ){
			filas[i] = Y_RESN/numprocs + 1;
			filaInicial[i] = i*filas[i];
		}else{
			filas[i] = Y_RESN/numprocs;
			filaInicial[i] = filaInicial[i-1] + filas[i-1];
		}
		count[i] = filas[i] * X_RESN;
		displ[i] = filaInicial[i] * X_RESN;
	}

  /* Timestamp variables */
	struct timeval ti, tf;

	if(rank==0){
		/* Allocate result matrix of Y_RESN x X_RESN */
		vres = (int *) malloc( Y_RESN * X_RESN * sizeof(int));
		if (!vres)
    {
			fprintf(stderr, "Error allocating memory\n");
			return 1;
		}
		for(i=0; i<Y_RESN; i++){
			res[i] = vres + i*X_RESN;
		}
	}

	/* Start measuring time */
	gettimeofday(&ti, NULL);

	/* Calculate and draw points */
	for(i=filaInicial[rank]; i<filaInicial[rank]+filas[rank]; i++)
  {
		for(j=0; j < X_RESN; j++)
    {
      z.real = z.imag = 0.0;
    	c.real = X_MIN + j * pasoX;
    	c.imag = Y_MAX - i * pasoY;
    	k = 0;
      flops += 4;

    	do
      { /* iterate for pixel color */
    		temp = z.real*z.real - z.imag*z.imag + c.real;
    		z.imag = 2.0*z.real*z.imag + c.imag;
    		z.real = temp;
    		lengthsq = z.real*z.real+z.imag*z.imag;
    		k++;
    		flops += 10;
    	} while (lengthsq < 4.0 && k < maxIterations);

			if (k >= maxIterations) res_parcial[i-filaInicial[rank]][j] = 0;
			else res_parcial[i-filaInicial[rank]][j] = k;
		}
	}

	/* End measuring time */
	gettimeofday(&tf, NULL);
	fprintf (stderr, "Rank:%d (PERF) Computation Time (seconds) = %lf\n", rank, get_seconds(ti,tf));

  //Los procesos terminan sus cálculos
	MPI_Barrier(MPI_COMM_WORLD);

	/* Start communication time */
	gettimeofday(&ti, NULL);
  //Recopilamos los trozos de imagen en vres
	MPI_Gatherv(res_parcial, count[rank], MPI_INT, vres, count, displ, MPI_INT, 0, MPI_COMM_WORLD);

	/* End communication time */
	gettimeofday(&tf, NULL);
	fprintf (stderr, "Rank:%d (PERF) Communication Time (seconds) = %lf\n", rank, get_seconds(ti,tf));

	//Cálculo del número total de flops
	MPI_Gather(&flops, 1, MPI_INT, flops_por_proceso, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank == 0)
  {
		for(i = 0; i < numprocs; i++)
    {
			flops_total += flops_por_proceso[i];
		}
		for(i = 0; i < numprocs; i++){
			fprintf(stderr, "b%d = %5f  flops = %d\n", i, (flops_total*1.0) / (flops_por_proceso[i]*1.0*numprocs), flops_por_proceso[i]);
		}
	}

  /* Print result out */
	if( DEBUG && (rank == 0)) {
		for(i=0;i<Y_RESN;i++) {
		  for(j=0;j<X_RESN;j++)
				  printf("%3d ", res[i][j]);
		  printf("\n");
		}
	}

	MPI_Finalize();
}
