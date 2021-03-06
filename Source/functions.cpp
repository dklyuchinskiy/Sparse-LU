#include "Header.h"


void print(int m, int n, double *u, int ldu)
{
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			printf("%4.1lf ", u[i + ldu*j]);
			if (j % N == N - 1) printf("|");
		}
		printf("\n");
		if (i % N == N - 1) printf("\n");
	}
	
}

// Функция выделения памяти под массив
double* alloc_arr(int n)
{
	double *f = (double*)malloc(n * sizeof(double));

	return f;
}

// Функция освобождения памяти 
void free_arr(double* * arr)
{
	free(*arr);
}

// Инициализация значений массивов. Заданий правой части и краевых условий
void Init_matrix(int nbl, int n2, int n3, double *A, int ldA)
{
	// n2 = nbl * nbl - size of small matrix
	// [n*n] x [3*n] of big matrix A

	int size = n2 * n3;
	double h = 1.0 / (N);
	int lda = nbl;

	A[0:size] = 0;

	for (int j = 0; j < nbl; j++)
	{
		if (j == 0)
		{
			fill_middle(nbl, j, h, &A[j * nbl + ldA * 0], ldA);
			fill_right(nbl, j, h, &A[j * nbl + ldA * nbl], ldA);
		}
		else if (j == nbl - 1)
		{
			fill_left(nbl, j, h, &A[j * nbl + ldA * nbl], ldA);
			fill_middle(nbl, j, h, &A[j * nbl + ldA * 2 * nbl], ldA);
		}
		else
		{
			fill_left(nbl, j, h, &A[j * nbl + ldA * 0], ldA);
			fill_middle(nbl, j, h, &A[j * nbl + ldA * nbl], ldA);
			fill_right(nbl, j, h, &A[j * nbl + ldA * 2 * nbl], ldA);
		}
	}

}

void fill_left(int nbl, int j, double h, double *A, int ldA)
{
	for (int i = 0; i < nbl; i++)
		A[i + ldA*i] = 1.0 / h;
}

void fill_right(int nbl, int j, double h, double *A, int ldA)
{
	for (int i = 0; i < nbl; i++)
		A[i + ldA*i] = 1.0 / h;
}

void fill_middle(int nbl, int j, double h, double *A, int ldA)
{

	A[0 + ldA * 0] = -1.0 / h;
	A[0 + ldA * 1] = 1.0 / h;

	for (int i = 1; i < nbl - 1; i++)
	{
		A[i + ldA*(i - 1)] = 1.0 / h;
		A[i + ldA*(i)] = -4.0 / h;
		A[i + ldA*(i + 1)] = 1.0 / h;
	}

	A[nbl - 1 + ldA * (nbl - 2)] = 1.0 / h;
	A[nbl - 1 + ldA * (nbl - 1)] = -1.0 / h;

}

void sparse_lu(int nbl, int n2, int n3, double *A, int ldA, int **ipiv_mat)
{
	int *ipiv = new int[nbl];
	int info;
	char low = 'L';
	char up = 'U';
	char unit = 'U';
	char no = 'N';
	char all = 'A';
	char left = 'L';
	char right = 'R';
	int ldL = nbl;
	int ldU = nbl;
	int start, start_l, start_r;
	double zero = 0.0;
	double one = 1.0;
	double mone = -1.0;
	int ione = 1;

	double *U = alloc_arr(nbl * nbl);
	double *L = alloc_arr(nbl * nbl);

	U[0:nbl*nbl] = 0;
	L[0:nbl*nbl] = 0;
	int mione = -1;

	for (int j = 0; j < nbl; j++)
	{
		if (j == 0) start = 0 + ldA * 0;
		else if (j == nbl - 1) start = n2 - nbl + ldA * 2 * nbl;
		else start = j * nbl + ldA * nbl;

		// LU of the A11 and copy factors

		dgetrf(&nbl, &nbl, &A[start], &ldA, ipiv, &info);

		// Copy factors
		dlacpy(&low, &nbl, &nbl, &A[start], &ldA, L, &ldL);
		dlacpy(&up, &nbl, &nbl, &A[start], &ldA, U, &ldU);
	
		for (int i = 0; i < nbl; i++)
			if (ipiv[i] != i + 1) printf("Iter: %d, row %d interchanged with row %d\n", j + 1, ipiv[i], i + 1);
	

		for (int i = 0; i < nbl; i++)
		{
			ipiv_mat[j][i] = ipiv[i];
		}
		
		if (j == nbl - 1) break;
		else
		{
			// Apply A21 * U^(-1)

			if (j == nbl - 2) start_l = n2 - nbl + ldA * nbl;
			else start_l = (j + 1) * nbl + ldA * 0;

			// Inversion U

			dtrtri(&up, &no, &nbl, U, &ldU, &info);

			// Apply A21 * U^(-1)

			dtrmm(&right, &up, &no, &no, &nbl, &nbl, &one, U, &ldU, &A[start_l], &ldA);

			// Apply L^(-1) * P ^ (-1) * A12

			if (j == 0) start_r = 0 + ldA * nbl;
			else start_r = (j)* nbl + ldA * 2 * nbl;

			// Inversion of L
			
			dtrtri(&low, &unit, &nbl, L, &ldL, &info);

			// Swap rows of A12 due to P^(-1) * A12

			dlaswp(&nbl, &A[start_r], &ldA, &ione, &nbl, ipiv, &ione);

			// Apply L^(-1) to P^(-1) * A12

			dtrmm(&left, &low, &no, &unit, &nbl, &nbl, &one, L, &ldL, &A[start_r], &ldA);

			// Compute Schur component

			dgemm(&no, &no, &nbl, &nbl, &nbl, &mone, &A[start_l], &ldA, &A[start_r], &ldA, &one, &A[start_l + ldA * nbl], &ldA);
		}

	}

	free_arr(&U);
	free_arr(&L);
}

void change(char right, int nbl, double *L, int ldL, int *ipiv)
{
	if (right == 'R' || right == 'r') // switch columns
	{
		for (int i = 0; i < nbl; i++)
			if (ipiv[i] != (i + 1)) swap_col(nbl, i, ipiv[i] - 1, L, ldL, ipiv);
	}

}

void swap_col(int nbl, int k, int m, double *L, int ldL, int *ipiv)
{
	double *c = alloc_arr(nbl);
	for (int i = 0; i < nbl; i++)
	{
		c[i] = L[i + ldL * m];
		L[i + ldL * m] = L[i + ldL * k];
		L[i + ldL * k] = c[i];
	}
	free_arr(&c);
}

void test(int nbl, int n2, int n3, double *A_f, int ldaf, double *U_f, int lduf, double *L_f, int ldlf, int **ipiv_mat)
{
	char low = 'L';
	char up = 'U';
	char unit = 'U';
	char no = 'N';
	char all = 'A';
	char left = 'L';
	char right = 'R';
	int start, start_l, start_r;
	double zero = 0.0;
	double one = 1.0;
	double mone = -1.0;
	int ione = 1;
	int mione = -1;
	double norm = 0;
	char frob = 'F';

	double *A_res = alloc_arr(n2*n2);
	A_res[0:n2*n2] = 0;

	// Switch rows L = P * L
	for (int j = 0; j < nbl; j++)
	{
		dlaswp(&nbl, &L_f[j * nbl + ldlf * (j) * nbl], &ldlf, &ione, &nbl, ipiv_mat[j], &mione);
	}

	// L = L * U , L is overwritten
	//dtrmm(&right, &up, &no, &no, &n2, &n2, &one, U_f, &lduf, L_f, &ldlf);

	// A = L * U , A - new matrix
	dgemm(&no, &no, &n2, &n2, &n2, &one, L_f, &ldlf, U_f, &lduf, &zero, A_res, &ldaf);

#ifdef DEBUG
	printf("matrix A result: A = L * U\n");
	print(n2, n2, A_f, ldaf);
#endif

	// Norm of residual || A - L * U ||
	for (int j = 0; j < n2; j++)
		for (int i = 0; i < n2; i++)
			A_res[i + ldaf * j] = A_res[i + ldaf * j] - A_f[i + ldaf * j];

	norm = dlange(&frob, &n2, &n2, A_res, &ldaf, NULL);
	if (norm < eps) printf("Norm %12.10lf : PASSED\n", norm);
	else printf("Norm %12.10lf : FAILED\n", norm);

	free_arr(&A_res);
}

void construct_A(int nbl, int n2, int n3, double *A, int ldA, double *A_f, int ldaf)
{
	char all = 'A';
	char low = 'L';
	char up = 'U';
	int nbl2 = 2 * nbl;
	int nbl3 = 3 * nbl;

	// Copy block rows 1 and 2
	dlacpy(&all, &nbl2, &nbl3, &A[0 + ldA * 0], &ldA, &A_f[0 + ldaf * 0], &ldaf);

	for (int j = 2; j < nbl - 1; j++)
	{
		// Copy row j
		dlacpy(&all, &nbl, &nbl3, &A[j * nbl + ldA * 0], &ldA, &A_f[j * nbl + ldaf * (j - 1) * nbl], &ldaf);
	}

	// Copy left
	dlacpy(&all, &nbl, &nbl2, &A[(nbl - 1) * nbl + ldA * nbl], &ldA, &A_f[(nbl - 1) * nbl + ldaf * (nbl - 2) * nbl], &ldaf);

#ifdef DEBUG
	printf("A_init\n");
	print(n2, n2, A_f, ldaf);
#endif
}

void construct_L(int nbl, int n2, int n3, double *A, int ldA, double *L_f, int ldlf)
{
	char all = 'A';
	char low = 'L';
	char up = 'U';
	int nbl2 = 2 * nbl;
	int nbl3 = 3 * nbl;

	// Copy j = 0 and j = 1 block rows
	dlacpy(&low, &nbl2, &nbl3, &A[0 + ldA * 0], &ldA, &L_f[0 + ldlf * 0], &ldlf);

	for (int j = 2; j < nbl - 1; j++)
	{
		// Copy left
		dlacpy(&all, &nbl, &nbl, &A[j * nbl + ldA * 0], &ldA, &L_f[j * nbl + ldlf * (j - 1) * nbl], &ldlf);
		// Copy middle
		dlacpy(&low, &nbl, &nbl, &A[j * nbl + ldA * nbl], &ldA, &L_f[j * nbl + ldlf * j * nbl], &ldlf);
	}

	// Copy left
	dlacpy(&all, &nbl, &nbl, &A[(nbl - 1) * nbl + ldA * nbl], &ldA, &L_f[(nbl - 1) * nbl + ldlf * (nbl - 2) * nbl], &ldlf);
	// Copy middle
	dlacpy(&low, &nbl, &nbl, &A[(nbl - 1) * nbl + ldA * 2 * nbl], &ldA, &L_f[(nbl - 1) * nbl + ldlf * (nbl - 1) * nbl], &ldlf);

	for (int i = 0; i < n2; i++)
		L_f[i + ldlf * i] = 1.0;

#ifdef DEBUG
	printf("L_full\n");
	print(n2, n2, L_f, ldlf);
#endif
}

void construct_U(int nbl, int n2, int n3, double *A, int ldA, double *U_f, int lduf)
{
	char all = 'A';
	char low = 'L';
	char up = 'U';
	int nbl2 = 2 * nbl;
	int nbl3 = 3 * nbl;

	// Copy j = 0 and j = 1 block rows
	dlacpy(&up, &nbl2, &nbl3, &A[0 + ldA * 0], &ldA, &U_f[0 + lduf * 0], &lduf);

	for (int j = 2; j < nbl - 1; j++)
	{
		// Copy right
		dlacpy(&all, &nbl, &nbl, &A[j * nbl + ldA * 2 * nbl], &ldA, &U_f[j * nbl + lduf * (j + 1) * nbl], &lduf);
		// Copy middle
		dlacpy(&up, &nbl, &nbl, &A[j * nbl + ldA * nbl], &ldA, &U_f[j * nbl + lduf * j * nbl], &lduf);
	}

	// Copy middle
	dlacpy(&up, &nbl, &nbl, &A[(nbl - 1) * nbl + ldA * 2 * nbl], &ldA, &U_f[(nbl - 1) * nbl + lduf * (nbl - 1) * nbl], &lduf);

#ifdef DEBUG
	printf("U_full\n");
	print(n2, n2, U_f, lduf);
#endif
}