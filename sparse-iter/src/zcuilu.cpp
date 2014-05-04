/*
    -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

       @author Hartwig Anzt 

       @precisions normal z -> s d c
*/
// includes CUDA
#include <cuda_runtime_api.h>
#include <cublas.h>
#include <cusparse_v2.h>
#include <cuda_profiler_api.h>

// project includes
#include "common_magma.h"
#include "../include/magmasparse.h"

#include <assert.h>


#define PRECISION_z






/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Prepares the ILU preconditioner via the asynchronous ILU iteration.

    Arguments
    =========

    magma_z_sparse_matrix A                   input matrix A
    magma_z_preconditioner *precond           preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zcuilusetup( magma_z_sparse_matrix A, magma_z_preconditioner *precond ){

        // copy matrix into preconditioner parameter
            magma_z_mtransfer(A, &(precond->M), A.memory_location, Magma_DEV);



            // CUSPARSE context //
            cusparseHandle_t cusparseHandle;
            cusparseStatus_t cusparseStatus;
            cusparseStatus = cusparseCreate(&cusparseHandle);
             if(cusparseStatus != 0)    printf("error in Handle.\n");


            cusparseMatDescr_t descrA;
            cusparseStatus = cusparseCreateMatDescr(&descrA);
             if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

            cusparseStatus =
            cusparseSetMatType(descrA,CUSPARSE_MATRIX_TYPE_GENERAL);
             if(cusparseStatus != 0)    printf("error in MatrType.\n");

            cusparseStatus =
            cusparseSetMatDiagType (descrA, CUSPARSE_DIAG_TYPE_NON_UNIT);
             if(cusparseStatus != 0)    printf("error in DiagType.\n");

            cusparseStatus =
            cusparseSetMatIndexBase(descrA,CUSPARSE_INDEX_BASE_ZERO);
             if(cusparseStatus != 0)    printf("error in IndexBase.\n");

            cusparseStatus =
            cusparseCreateSolveAnalysisInfo( &(precond->cuinfo) );
             if(cusparseStatus != 0)    printf("error in info.\n");

            // end CUSPARSE context //

            cusparseStatus =
            cusparseZcsrsv_analysis( cusparseHandle, 
                        CUSPARSE_OPERATION_NON_TRANSPOSE, 
                        precond->M.num_rows, precond->M.nnz, descrA,
                        precond->M.val, precond->M.row, precond->M.col, 
                        precond->cuinfo); 
             if(cusparseStatus != 0)    printf("error in analysis.\n");

            cusparseStatus =
            cusparseZcsrilu0( cusparseHandle, CUSPARSE_OPERATION_NON_TRANSPOSE, 
                              precond->M.num_rows, descrA, 
                              precond->M.val, 
                              precond->M.row, 
                              precond->M.col, 
                              precond->cuinfo);
             if(cusparseStatus != 0)    printf("error in ILU.\n");




    magma_z_sparse_matrix hA, hL, hU;

    magma_z_mtransfer( precond->M, &hA, Magma_DEV, Magma_CPU );

    hL.diagorder_type = Magma_UNITY;
    magma_z_mconvert( hA, &hL , Magma_CSR, Magma_CSRL );
    hU.diagorder_type = Magma_VALUE;
    magma_z_mconvert( hA, &hU , Magma_CSR, Magma_CSRU );
    magma_z_mtransfer( hL, &(precond->L), Magma_CPU, Magma_DEV );
    magma_z_mtransfer( hU, &(precond->U), Magma_CPU, Magma_DEV );


    cusparseMatDescr_t descrL;
    cusparseStatus = cusparseCreateMatDescr(&descrL);
     if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

    cusparseStatus =
    cusparseSetMatType(descrL,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
     if(cusparseStatus != 0)    printf("error in MatrType.\n");

    cusparseStatus =
    cusparseSetMatDiagType (descrL, CUSPARSE_DIAG_TYPE_UNIT);
     if(cusparseStatus != 0)    printf("error in DiagType.\n");

    cusparseStatus =
    cusparseSetMatIndexBase(descrL,CUSPARSE_INDEX_BASE_ZERO);
     if(cusparseStatus != 0)    printf("error in IndexBase.\n");

    cusparseStatus =
    cusparseSetMatFillMode(descrL,CUSPARSE_FILL_MODE_LOWER);
     if(cusparseStatus != 0)    printf("error in fillmode.\n");


    cusparseStatus = cusparseCreateSolveAnalysisInfo(&precond->cuinfoL); 
     if(cusparseStatus != 0)    printf("error in info.\n");

    cusparseStatus =
    cusparseZcsrsv_analysis(cusparseHandle, 
        CUSPARSE_OPERATION_NON_TRANSPOSE, precond->L.num_rows, 
        precond->L.nnz, descrL, 
        precond->L.val, precond->L.row, precond->L.col, precond->cuinfoL );
     if(cusparseStatus != 0)    printf("error in analysis.\n");

    cusparseDestroyMatDescr( descrL );

    cusparseMatDescr_t descrU;
    cusparseStatus = cusparseCreateMatDescr(&descrU);
     if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

    cusparseStatus =
    cusparseSetMatType(descrU,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
     if(cusparseStatus != 0)    printf("error in MatrType.\n");

    cusparseStatus =
    cusparseSetMatDiagType (descrU, CUSPARSE_DIAG_TYPE_NON_UNIT);
     if(cusparseStatus != 0)    printf("error in DiagType.\n");

    cusparseStatus =
    cusparseSetMatIndexBase(descrU,CUSPARSE_INDEX_BASE_ZERO);
     if(cusparseStatus != 0)    printf("error in IndexBase.\n");

    cusparseStatus =
    cusparseSetMatFillMode(descrU,CUSPARSE_FILL_MODE_UPPER);
     if(cusparseStatus != 0)    printf("error in fillmode.\n");

    cusparseStatus = cusparseCreateSolveAnalysisInfo(&precond->cuinfoU); 
     if(cusparseStatus != 0)    printf("error in info.\n");

    cusparseStatus =
    cusparseZcsrsv_analysis(cusparseHandle, 
        CUSPARSE_OPERATION_NON_TRANSPOSE, precond->U.num_rows, 
        precond->U.nnz, descrU, 
        precond->U.val, precond->U.row, precond->U.col, precond->cuinfoU );
     if(cusparseStatus != 0)    printf("error in analysis.\n");

    magma_z_mfree(&hA);
    magma_z_mfree(&hL);
    magma_z_mfree(&hU);


    cusparseDestroyMatDescr( descrA );
    cusparseDestroy( cusparseHandle );
    return MAGMA_SUCCESS;

}








/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Performs the left triangular solves using the ILU preconditioner.

    Arguments
    =========

    magma_z_vector b                        RHS
    magma_z_vector *x                       vector to precondition
    magma_z_preconditioner *precond         preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zapplycuilu_l( magma_z_vector b, magma_z_vector *x, 
                    magma_z_preconditioner *precond ){

            magmaDoubleComplex one = MAGMA_Z_MAKE( 1.0, 0.0);




            // CUSPARSE context //
            cusparseHandle_t cusparseHandle;
            cusparseStatus_t cusparseStatus;
            cusparseStatus = cusparseCreate(&cusparseHandle);
             if(cusparseStatus != 0)    printf("error in Handle.\n");


            cusparseMatDescr_t descrL;
            cusparseStatus = cusparseCreateMatDescr(&descrL);
             if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

            cusparseStatus =
            cusparseSetMatType(descrL,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
             if(cusparseStatus != 0)    printf("error in MatrType.\n");

            cusparseStatus =
            cusparseSetMatDiagType (descrL, CUSPARSE_DIAG_TYPE_UNIT);
             if(cusparseStatus != 0)    printf("error in DiagType.\n");

            cusparseStatus =
            cusparseSetMatIndexBase(descrL,CUSPARSE_INDEX_BASE_ZERO);
             if(cusparseStatus != 0)    printf("error in IndexBase.\n");

            cusparseStatus =
            cusparseSetMatFillMode(descrL,CUSPARSE_FILL_MODE_LOWER);
             if(cusparseStatus != 0)    printf("error in fillmode.\n");

            // end CUSPARSE context //

            cusparseStatus =
            cusparseZcsrsv_solve(   cusparseHandle, 
                                    CUSPARSE_OPERATION_NON_TRANSPOSE, 
                                    precond->L.num_rows, &one, 
                                    descrL,
                                    precond->L.val,
                                    precond->L.row,
                                    precond->L.col,
                                    precond->cuinfoL,
                                    b.val,
                                    x->val );
             if(cusparseStatus != 0)   printf("error in L triangular solve.\n");

    cusparseDestroyMatDescr( descrL );
    cusparseDestroy( cusparseHandle );
    magma_device_sync();
    return MAGMA_SUCCESS;

}


/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Performs the right triangular solves using the ILU preconditioner.

    Arguments
    =========

    magma_z_vector b                        RHS
    magma_z_vector *x                       vector to precondition
    magma_z_preconditioner *precond         preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zapplycuilu_r( magma_z_vector b, magma_z_vector *x, 
                    magma_z_preconditioner *precond ){

            magmaDoubleComplex one = MAGMA_Z_MAKE( 1.0, 0.0);

            // CUSPARSE context //
            cusparseHandle_t cusparseHandle;
            cusparseStatus_t cusparseStatus;
            cusparseStatus = cusparseCreate(&cusparseHandle);
             if(cusparseStatus != 0)    printf("error in Handle.\n");


            cusparseMatDescr_t descrU;
            cusparseStatus = cusparseCreateMatDescr(&descrU);
             if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

            cusparseStatus =
            cusparseSetMatType(descrU,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
             if(cusparseStatus != 0)    printf("error in MatrType.\n");

            cusparseStatus =
            cusparseSetMatDiagType (descrU, CUSPARSE_DIAG_TYPE_NON_UNIT);
             if(cusparseStatus != 0)    printf("error in DiagType.\n");

            cusparseStatus =
            cusparseSetMatIndexBase(descrU,CUSPARSE_INDEX_BASE_ZERO);
             if(cusparseStatus != 0)    printf("error in IndexBase.\n");

            cusparseStatus =
            cusparseSetMatFillMode(descrU,CUSPARSE_FILL_MODE_UPPER);
             if(cusparseStatus != 0)    printf("error in fillmode.\n");

            // end CUSPARSE context //

            cusparseStatus =
            cusparseZcsrsv_solve(   cusparseHandle, 
                                    CUSPARSE_OPERATION_NON_TRANSPOSE, 
                                    precond->U.num_rows, &one, 
                                    descrU,
                                    precond->U.val,
                                    precond->U.row,
                                    precond->U.col,
                                    precond->cuinfoU,
                                    b.val,
                                    x->val );
             if(cusparseStatus != 0)   printf("error in L triangular solve.\n");

    cusparseDestroyMatDescr( descrU );
    cusparseDestroy( cusparseHandle );
    magma_device_sync();
    return MAGMA_SUCCESS;

}




/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Prepares the IC preconditioner via the asynchronous ILU iteration.

    Arguments
    =========

    magma_z_sparse_matrix A                   input matrix A
    magma_z_preconditioner *precond           preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zcuiccsetup( magma_z_sparse_matrix A, magma_z_preconditioner *precond ){

    magma_z_sparse_matrix hA, U;
    magma_z_mtransfer( A, &hA, A.memory_location, Magma_CPU );
    U.diagorder_type = Magma_VALUE;
    magma_z_mconvert( hA, &U, Magma_CSR, Magma_CSRL);

    magma_z_mtransfer(U, &(precond->L), Magma_CPU, Magma_DEV);

    // CUSPARSE context //
    cusparseHandle_t cusparseHandle;
    cusparseStatus_t cusparseStatus;
    cusparseStatus = cusparseCreate(&cusparseHandle);
     if(cusparseStatus != 0)    printf("error in Handle.\n");


    cusparseMatDescr_t descrA;
    cusparseStatus = cusparseCreateMatDescr(&descrA);
     if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

    cusparseStatus =
    cusparseSetMatType(descrA,CUSPARSE_MATRIX_TYPE_SYMMETRIC);
     if(cusparseStatus != 0)    printf("error in MatrType.\n");

    cusparseStatus =
    cusparseSetMatDiagType (descrA, CUSPARSE_DIAG_TYPE_NON_UNIT);
     if(cusparseStatus != 0)    printf("error in DiagType.\n");

    cusparseStatus =
    cusparseSetMatIndexBase(descrA,CUSPARSE_INDEX_BASE_ZERO);
     if(cusparseStatus != 0)    printf("error in IndexBase.\n");

    cusparseStatus =
    cusparseSetMatFillMode(descrA,CUSPARSE_FILL_MODE_LOWER);
     if(cusparseStatus != 0)    printf("error in fillmode.\n");


    cusparseStatus =
    cusparseCreateSolveAnalysisInfo( &(precond->cuinfo) );
     if(cusparseStatus != 0)    printf("error in info.\n");

    // end CUSPARSE context //

    cusparseStatus =
    cusparseZcsrsv_analysis( cusparseHandle, 
                CUSPARSE_OPERATION_NON_TRANSPOSE, 
                precond->L.num_rows, precond->L.nnz, descrA,
                precond->L.val, precond->L.row, precond->L.col, 
                precond->cuinfo); 
     if(cusparseStatus != 0)    printf("error in analysis IC.\n");

    cusparseStatus =
    cusparseZcsric0( cusparseHandle, CUSPARSE_OPERATION_NON_TRANSPOSE, 
                      precond->L.num_rows, descrA, 
                      precond->L.val, 
                      precond->L.row, 
                      precond->L.col, 
                      precond->cuinfo);
     if(cusparseStatus != 0)    printf("error in ICC.\n");

    magma_z_mtransfer( precond->L, &precond->U, Magma_DEV, Magma_DEV );
    magma_z_mtransfer( precond->L, &precond->M, Magma_DEV, Magma_DEV );

    cusparseMatDescr_t descrL;
    cusparseStatus = cusparseCreateMatDescr(&descrL);
     if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

    cusparseStatus =
    cusparseSetMatType(descrL,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
     if(cusparseStatus != 0)    printf("error in MatrType.\n");

    cusparseStatus =
    cusparseSetMatDiagType (descrL, CUSPARSE_DIAG_TYPE_UNIT);
     if(cusparseStatus != 0)    printf("error in DiagType.\n");

    cusparseStatus =
    cusparseSetMatIndexBase(descrL,CUSPARSE_INDEX_BASE_ZERO);
     if(cusparseStatus != 0)    printf("error in IndexBase.\n");

    cusparseStatus =
    cusparseSetMatFillMode(descrL,CUSPARSE_FILL_MODE_LOWER);
     if(cusparseStatus != 0)    printf("error in fillmode.\n");


    cusparseStatus = cusparseCreateSolveAnalysisInfo(&precond->cuinfoL); 
     if(cusparseStatus != 0)    printf("error in info.\n");

    cusparseStatus =
    cusparseZcsrsv_analysis(cusparseHandle, 
        CUSPARSE_OPERATION_TRANSPOSE, precond->M.num_rows, 
        precond->M.nnz, descrL, 
        precond->M.val, precond->M.row, precond->M.col, precond->cuinfoL );
     if(cusparseStatus != 0)    printf("error in analysis L.\n");

    cusparseDestroyMatDescr( descrL );

    cusparseMatDescr_t descrU;
    cusparseStatus = cusparseCreateMatDescr(&descrU);
     if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

    cusparseStatus =
    cusparseSetMatType(descrU,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
     if(cusparseStatus != 0)    printf("error in MatrType.\n");

    cusparseStatus =
    cusparseSetMatDiagType (descrU, CUSPARSE_DIAG_TYPE_NON_UNIT);
     if(cusparseStatus != 0)    printf("error in DiagType.\n");

    cusparseStatus =
    cusparseSetMatIndexBase(descrU,CUSPARSE_INDEX_BASE_ZERO);
     if(cusparseStatus != 0)    printf("error in IndexBase.\n");

    cusparseStatus =
    cusparseSetMatFillMode(descrU,CUSPARSE_FILL_MODE_LOWER);
     if(cusparseStatus != 0)    printf("error in fillmode.\n");

    cusparseStatus = cusparseCreateSolveAnalysisInfo(&precond->cuinfoU); 
     if(cusparseStatus != 0)    printf("error in info.\n");

    cusparseStatus =
    cusparseZcsrsv_analysis(cusparseHandle, 
        CUSPARSE_OPERATION_NON_TRANSPOSE, precond->M.num_rows, 
        precond->M.nnz, descrU, 
        precond->M.val, precond->M.row, precond->M.col, precond->cuinfoU );
     if(cusparseStatus != 0)    printf("error in analysis U.\n");

    cusparseDestroyMatDescr( descrL );

    

    magma_z_mfree(&U);
    magma_z_mfree(&hA);

    cusparseDestroyMatDescr( descrA );
    cusparseDestroy( cusparseHandle );
    return MAGMA_SUCCESS;

}



/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Performs the left triangular solves using the ICC preconditioner.

    Arguments
    =========

    magma_z_vector b                        RHS
    magma_z_vector *x                       vector to precondition
    magma_z_preconditioner *precond         preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zapplycuicc_l( magma_z_vector b, magma_z_vector *x, 
                    magma_z_preconditioner *precond ){

            magmaDoubleComplex one = MAGMA_Z_MAKE( 1.0, 0.0);

            // CUSPARSE context //
            cusparseHandle_t cusparseHandle;
            cusparseStatus_t cusparseStatus;
            cusparseStatus = cusparseCreate(&cusparseHandle);
             if(cusparseStatus != 0)    printf("error in Handle.\n");


            cusparseMatDescr_t descrL;
            cusparseStatus = cusparseCreateMatDescr(&descrL);
             if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

            cusparseStatus =
            cusparseSetMatType(descrL,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
             if(cusparseStatus != 0)    printf("error in MatrType.\n");

            cusparseStatus =
            cusparseSetMatDiagType (descrL, CUSPARSE_DIAG_TYPE_NON_UNIT);
             if(cusparseStatus != 0)    printf("error in DiagType.\n");

            cusparseStatus =
            cusparseSetMatIndexBase(descrL,CUSPARSE_INDEX_BASE_ZERO);
             if(cusparseStatus != 0)    printf("error in IndexBase.\n");


            cusparseStatus =
            cusparseSetMatFillMode(descrL,CUSPARSE_FILL_MODE_LOWER);
             if(cusparseStatus != 0)    printf("error in fillmode.\n");

            // end CUSPARSE context //

            cusparseStatus =
            cusparseZcsrsv_solve(   cusparseHandle, 
                                    CUSPARSE_OPERATION_TRANSPOSE, 
                                    precond->M.num_rows, &one, 
                                    descrL,
                                    precond->M.val,
                                    precond->M.row,
                                    precond->M.col,
                                    precond->cuinfoL,
                                    b.val,
                                    x->val );
             if(cusparseStatus != 0)   printf("error in L triangular solve.\n");


    cusparseDestroyMatDescr( descrL );
    cusparseDestroy( cusparseHandle );
    magma_device_sync();
    return MAGMA_SUCCESS;

}


/*  -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date

    Purpose
    =======

    Performs the right triangular solves using the ICC preconditioner.

    Arguments
    =========

    magma_z_vector b                        RHS
    magma_z_vector *x                       vector to precondition
    magma_z_preconditioner *precond         preconditioner parameters

    ========================================================================  */

magma_int_t
magma_zapplycuicc_r( magma_z_vector b, magma_z_vector *x, 
                    magma_z_preconditioner *precond ){

            magmaDoubleComplex one = MAGMA_Z_MAKE( 1.0, 0.0);

            // CUSPARSE context //
            cusparseHandle_t cusparseHandle;
            cusparseStatus_t cusparseStatus;
            cusparseStatus = cusparseCreate(&cusparseHandle);
             if(cusparseStatus != 0)    printf("error in Handle.\n");


            cusparseMatDescr_t descrU;
            cusparseStatus = cusparseCreateMatDescr(&descrU);
             if(cusparseStatus != 0)    printf("error in MatrDescr.\n");

            cusparseStatus =
            cusparseSetMatType(descrU,CUSPARSE_MATRIX_TYPE_TRIANGULAR);
             if(cusparseStatus != 0)    printf("error in MatrType.\n");

            cusparseStatus =
            cusparseSetMatDiagType (descrU, CUSPARSE_DIAG_TYPE_NON_UNIT);
             if(cusparseStatus != 0)    printf("error in DiagType.\n");


            cusparseStatus =
            cusparseSetMatFillMode(descrU,CUSPARSE_FILL_MODE_LOWER);
             if(cusparseStatus != 0)    printf("error in fillmode.\n");

            cusparseStatus =
            cusparseSetMatIndexBase(descrU,CUSPARSE_INDEX_BASE_ZERO);
             if(cusparseStatus != 0)    printf("error in IndexBase.\n");


            // end CUSPARSE context //

            cusparseStatus =
            cusparseZcsrsv_solve(   cusparseHandle, 
                                    CUSPARSE_OPERATION_NON_TRANSPOSE, 
                                    precond->M.num_rows, &one, 
                                    descrU,
                                    precond->M.val,
                                    precond->M.row,
                                    precond->M.col,
                                    precond->cuinfoU,
                                    b.val,
                                    x->val );
             if(cusparseStatus != 0)   printf("error in U triangular solve.\n");

    cusparseDestroyMatDescr( descrU );
    cusparseDestroy( cusparseHandle );
    magma_device_sync();
    return MAGMA_SUCCESS;

}













