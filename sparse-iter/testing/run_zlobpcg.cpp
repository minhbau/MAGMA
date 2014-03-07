/*
    -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       November 2011

       @precisions normal z -> c d s
       @author Stan Tomov
*/

// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include "flops.h"
#include "magma.h"
#include "magma_lapack.h"
#include "../include/magmasparse.h"
#include "testings.h"

extern "C" magma_int_t
magma_zlobpcg( magma_int_t m, magma_int_t n, magma_z_sparse_matrix A,
               magmaDoubleComplex *blockX, double *evalues,
               magmaDoubleComplex *dwork, magma_int_t ldwork,
               magmaDoubleComplex *hwork, magma_int_t lwork,
               magma_z_solver_par *solver_par, magma_int_t *info );


/* ////////////////////////////////////////////////////////////////////////////
   -- Testing magma_zlobpcg
*/
int main( int argc, char** argv)
{
    TESTING_INIT();

    magma_z_solver_par solver_par;
    solver_par.epsilon = 10e-16;
    solver_par.maxiter = 1000;
    solver_par.verbose = 0;
    solver_par.num_eigenvalues = 20;
    magma_z_preconditioner precond_par;
    precond_par.solver = Magma_JACOBI;
    int precond = 0;
    int format = 0;
    int version = 0;
    
    magma_z_sparse_matrix A, B, dA;
    magma_z_vector x, b;
    B.blocksize = 8;
    B.alignment = 8;
    
    magmaDoubleComplex one = MAGMA_Z_MAKE(1.0, 0.0);
    magmaDoubleComplex zero = MAGMA_Z_MAKE(0.0, 0.0);

    B.storage_type = Magma_CSR;
    char filename[256]; 
    int i;
    for( i = 1; i < argc; ++i ) {
     if ( strcmp("--format", argv[i]) == 0 ) {
            format = atoi( argv[++i] );
            switch( format ) {
                case 0: B.storage_type = Magma_CSR; break;
                case 1: B.storage_type = Magma_ELLPACK; break;
                case 2: B.storage_type = Magma_ELLPACKT; break;
                case 3: B.storage_type = Magma_ELLPACKRT; break;
                case 4: B.storage_type = Magma_SELLC; break;
            }
        }else if ( strcmp("--precond", argv[i]) == 0 ) {
            format = atoi( argv[++i] );
            switch( precond ) {
                case 0: precond_par.solver = Magma_JACOBI; break;
            }

        }else if ( strcmp("--version", argv[i]) == 0 ) {
            version = atoi( argv[++i] );
        }else if ( strcmp("--blocksize", argv[i]) == 0 ) {
            B.blocksize = atoi( argv[++i] );
        }else if ( strcmp("--alignment", argv[i]) == 0 ) {
            B.alignment = atoi( argv[++i] );
        }else if ( strcmp("--verbose", argv[i]) == 0 ) {
            solver_par.verbose = atoi( argv[++i] );
        }  else if ( strcmp("--maxiter", argv[i]) == 0 ) {
            solver_par.maxiter = atoi( argv[++i] );
        } else if ( strcmp("--tol", argv[i]) == 0 ) {
            sscanf( argv[++i], "%lf", &solver_par.epsilon );
        } else if ( strcmp("--eigenvalues", argv[i]) == 0 ) {
            solver_par.num_eigenvalues = atoi( argv[++i] );
        } else
            break;
    }
    printf( "\n    usage: ./run_zlobpcg"
        " [ --format %d (0=CSR, 1=ELLPACK, 2=ELLPACKT, 3=ELLPACKRT, 4=SELLC)"
        " [ --blocksize %d --alignment %d ]"
        " --verbose %d (0=summary, k=details every k iterations)"
        " --maxiter %d --tol %.2e"
        " --preconditioner %d (0=Jacobi) "
        " --eigenvalues %d ]"
        " matrices \n\n", format, B.blocksize, B.alignment,
        solver_par.verbose,
        solver_par.maxiter, solver_par.epsilon, precond,  
        solver_par.num_eigenvalues);

    while(  i < argc ){

        magma_z_csr_mtx( &A,  argv[i]  ); 

        printf( "\nmatrix info: %d-by-%d with %d nonzeros\n\n"
                                    ,A.num_rows,A.num_cols,A.nnz );

        magma_zsolverinfo_init( &solver_par, &precond_par ); // inside the loop!
                           // as the matrix size has influence on the EV-length

        solver_par.ev_length = A.num_cols;

        real_Double_t  gpu_time;
        magma_opts opts;
        //parse_opts( argc, argv, &opts );
        magma_int_t ISEED[4] = {0,0,0,1}, ione = 1;
    
        magma_int_t blockSize = solver_par.num_eigenvalues;    
        magma_int_t m = A.num_rows;

        magma_z_mtransfer(A, &dA, Magma_CPU, Magma_DEV);

        // Memory allocation for the eigenvectors, eigenvalues, and workspace
        double *evalues;
        magmaDoubleComplex *evectors, *hevectors, *dwork, *hwork;
        magma_int_t info, ldwork = 8*m*blockSize;
        magma_int_t lhwork = max(2*blockSize+blockSize*magma_get_dsytrd_nb(blockSize),
                                 1 + 6*3*blockSize + 2* 3*blockSize* 3*blockSize);

        // This to be revisited - return is just blockSize but we use this for the
        // generalized eigensolver as well so we need 3X the memory
        magma_dmalloc_cpu(    &evalues ,     3*blockSize );

        magma_zmalloc(        &evectors, m * blockSize );
        magma_zmalloc_cpu(   &hevectors, m * blockSize );
        magma_zmalloc(        &dwork   ,        ldwork );
        magma_zmalloc_pinned( &hwork   ,        lhwork );

        // Solver parameters
        magma_z_solver_par solver_par;
        solver_par.epsilon = 1e-3;
        solver_par.maxiter = 360;
        
        magma_int_t n2 = m * blockSize;
        lapackf77_zlarnv( &ione, ISEED, &n2, hevectors );
        magma_zsetmatrix( m, blockSize, hevectors, m, evectors, m );

        // Find the blockSize smallest eigenvalues and corresponding eigen-vectors
        gpu_time = magma_wtime();
        magma_zlobpcg( m, blockSize, 
                       dA, evectors, evalues,
                       dwork, ldwork,
                       hwork, lhwork,
                       &solver_par, &info);
        gpu_time = magma_wtime() - gpu_time;

        printf("Time (sec) = %7.2f\n", gpu_time);

        magma_z_mfree(     &A    );
        magma_free_cpu(  evalues );
        magma_free(     evectors );
        magma_free_cpu( hevectors);
        magma_free(     dwork    );
        magma_free_pinned( hwork    );

        magma_zsolverinfo_free( &solver_par, &precond_par );

        i++;
    }

    TESTING_FINALIZE();
    return 0;
}