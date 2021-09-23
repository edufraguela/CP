/* Práctica 2 - Paralelismo -> Implementación con funciones estándar de MPI
   Authors:
            Fernando Seara Romera   f.searar@udc.es
            Eduardo Pérez Fraguela  eduardo.perez.fraguela@udc.es
*/

#include <stdio.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int i, done = 0, n;
    double PI25DT = 3.141592653589793238462643;
    double pi, h, sum, x, aux;
    int numprocs, rank;

    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while (!done)
    {
        if(rank == 0) {
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d",&n);
        }

        //Envío del número de intervalos desde root al resto de procesos
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0) break;

        h   = 1.0 / (double) n;
        sum = 0.0;
        //Cada proceso realiza el cálculo de su intervalo
        for (i = rank+1; i <= n; i+=numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = h * sum;

        //Recolección de cálculos y suma de estos en el root
        MPI_Reduce(&pi, &aux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if(rank == 0) {  
          printf("pi is approximately %.16f, Error is %.16f\n", aux, fabs(aux - PI25DT));
        }
    }
    MPI_Finalize();
}
