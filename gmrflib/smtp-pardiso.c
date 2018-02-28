/* smtp-pardiso.c
 * 
 * Copyright (C) 2018 Havard Rue
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author's contact information:
 *
 *        Haavard Rue
 *        CEMSE Division
 *        King Abdullah University of Science and Technology
 *        Thuwal 23955-6900, Saudi Arabia
 *        Email: haavard.rue@kaust.edu.sa
 *        Office: +966 (0)12 808 0640
 *
 */

/*!
  \file smtp-pardiso.c
  \brief The interface to the PARDISO library
*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(__FreeBSD__)
#include <malloc.h>
#endif

#include "GMRFLib/GMRFLib.h"
#include "GMRFLib/GMRFLibP.h"

#ifndef HGVERSION
#define HGVERSION
#endif
static const char RCSId[] = "file: " __FILE__ "  " HGVERSION;

#define WARNING(_msg) fprintf(stderr, "\n\n%s:%1d: %s\n\n", __FILE__, __LINE__, _msg)

int verbose_mode = 0;

int GMRFLib_free_csr(GMRFLib_csr_tp ** csr)
{
	if (*csr) {
		Free((*csr)->ia);
		Free((*csr)->ja);
		Free((*csr)->a);
		Free(*csr);
	}
	return GMRFLib_SUCCESS;
}

int GMRFLib_duplicate_csr(GMRFLib_csr_tp ** csr_to, GMRFLib_csr_tp * csr_from)
{
	if (csr_from == NULL) {
		*csr_to = NULL;
		return GMRFLib_SUCCESS;
	}

	*csr_to = Calloc(1, GMRFLib_csr_tp);
	(*csr_to)->base = csr_from->base;
	(*csr_to)->n = csr_from->n;
	(*csr_to)->na = csr_from->na;

	(*csr_to)->ia = Calloc(csr_from->n + 1, int);
	memcpy((void *) ((*csr_to)->ia), (void *) (csr_from->ia), (size_t) (csr_from->n + 1) * sizeof(int));

	(*csr_to)->ja = Calloc(csr_from->na, int);
	memcpy((void *) ((*csr_to)->ja), (void *) (csr_from->ja), (size_t) (csr_from->na) * sizeof(int));

	(*csr_to)->a = Calloc(csr_from->na, double);
	memcpy((void *) ((*csr_to)->a), (void *) (csr_from->a), (size_t) (csr_from->na) * sizeof(double));

	return GMRFLib_SUCCESS;
}

int GMRFLib_csr_base(int base, GMRFLib_csr_tp * M)
{
	if (M->base != base) {
		GMRFLib_csr_convert(M);
	}
	return GMRFLib_SUCCESS;
}

int GMRFLib_csr_convert(GMRFLib_csr_tp * M)
{
	int i;
	if (M->base == 1) {
		M->base = 0;
		for (i = 0; i < M->n + 1; i++) {
			M->ia[i] -= 1;
		}
		for (i = 0; i < M->na; i++) {
			M->ja[i] -= 1;
		}
	} else if (M->base == 0) {
		M->base = 1;
		for (i = 0; i < M->n + 1; i++) {
			M->ia[i] += 1;
		}
		for (i = 0; i < M->na; i++) {
			M->ja[i] += 1;
		}
	} else {
		assert(0 == 1);
	}
	return GMRFLib_SUCCESS;
}

int GMRFLib_Q2csr_check(GMRFLib_csr_tp * M)
{
	int mtype = GMRFLib_PARDISO_MTYPE, error = 0;
	GMRFLib_csr_tp *Q;

	GMRFLib_duplicate_csr(&Q, M);
	GMRFLib_csr_base(1, Q);
	pardiso_chkmatrix(&mtype, &(Q->n), Q->a, Q->ia, Q->ja, &error);
	GMRFLib_free_csr(&Q);
	if (error != 0) {
		GMRFLib_ERROR(GMRFLib_ESNH);
	}
	return GMRFLib_SUCCESS;
}

int GMRFLib_Q2csr(GMRFLib_csr_tp ** csr, GMRFLib_graph_tp * graph, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg)
{
	// create a upper triangular csr matrix from Q
#define M (*csr)
	int i, j, jj, k, n, na, nnz;

	M = Calloc(1, GMRFLib_csr_tp);
	M->base = 0;
	n = graph->n;
	GMRFLib_nQelm(&nnz, graph);			       // symmetric
	na = (nnz - n) / 2 + n;				       // only upper triangular. yes, integer division
	M->na = na;
	M->n = n;
	M->a = Calloc(na, double);
	M->ja = Calloc(na, int);
	M->ia = Calloc(n + 1, int);

	M->ia[0] = 0;
	for (i = 0; i < n; i++) {
		k = 1;
		for (jj = 0; jj < graph->nnbs[i]; jj++) {
			if (graph->nbs[i][jj] > i)
				k++;
		}
		M->ia[i + 1] = M->ia[i] + k;
	}
	assert(M->ia[n] == na);

	for (i = k = 0; i < n; i++) {
		M->ja[k] = i;
		M->a[k] = Qfunc(i, i, Qfunc_arg);
		k++;
		for (jj = 0; jj < graph->nnbs[i]; jj++) {
			j = graph->nbs[i][jj];
			if (j > i) {
				M->ja[k] = j;
				M->a[k] = Qfunc(i, j, Qfunc_arg);
				k++;
			}
		}
	}
#undef M

	return GMRFLib_SUCCESS;
}

int GMRFLib_print_csr(FILE * fp, GMRFLib_csr_tp * csr)
{
	int i, j, k, jj, nnb;
	double value;

	fprintf(fp, "Q->base = %1d, Q->n = %1d, Q->na = %1d\n", csr->base, csr->n, csr->na);
	for (i = k = 0; i < csr->n; i++) {
		nnb = csr->ia[i + 1] - csr->ia[i];
		for (jj = 0; jj < nnb; jj++) {
			j = csr->ja[k];
			value = csr->a[k];
			fprintf(fp, "%sQ[ %1d , %1d ] = %.12f\n", (jj > 0 ? "\t" : ""), i + csr->base, j, value);
			k++;
		}
	}
	fflush(fp);

	return GMRFLib_SUCCESS;
}

int GMRFLib_csr2Q(GMRFLib_tabulate_Qfunc_tp ** Qtab, GMRFLib_graph_tp ** graph, GMRFLib_csr_tp * csr)
{
	int i, j, k, jj, nnb;

	int *iarr = Calloc(csr->na, int);
	int *jarr = Calloc(csr->na, int);
	double *arr = Calloc(csr->na, double);

	for (i = k = 0; i < csr->n; i++) {
		nnb = csr->ia[i + 1] - csr->ia[i];
		for (jj = 0; jj < nnb; jj++) {
			j = csr->ja[k];
			iarr[k] = i - csr->base;
			jarr[k] = j - csr->base;
			arr[k] = csr->a[k];
			k++;
		}
	}
	assert(k == csr->na);
	GMRFLib_tabulate_Qfunc_from_list(Qtab, graph, csr->na, iarr, jarr, arr, csr->n, NULL, NULL, NULL);

	Free(iarr);
	Free(jarr);
	Free(arr);

	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_init(GMRFLib_pardiso_store_tp ** store)
{
	GMRFLib_ENTER_ROUTINE;

	int error = 0, solver = 0;
	GMRFLib_pardiso_store_tp *s = Calloc(1, GMRFLib_pardiso_store_tp);

	s->mtype = GMRFLib_PARDISO_MTYPE;
	s->iparm[0] = 0;				       /* use default values */
	s->iparm[2] = GMRFLib_openmp->max_threads_inner;       /* num_proc */
	s->iparm[4] = 0;				       /* use internal reordering */
	
	pardisoinit(s->pt, &(s->mtype), &solver, s->iparm, s->dparm, &error);
	memcpy((void *) (s->iparm_default), (void *) (s->iparm), GMRFLib_PARDISO_PLEN * sizeof(int));
	if (error != 0) {
		GMRFLib_ERROR(GMRFLib_EPARDISO_INTERNAL_ERROR);
	}

	s->done_with_init = GMRFLib_TRUE;
	*store = s;

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_setparam(GMRFLib_pardiso_flag_tp flag, GMRFLib_pardiso_store_tp * store)
{
	static int check_install = GMRFLib_TRUE;
	if (check_install == GMRFLib_TRUE) {
		GMRFLib_pardiso_check_install(1);
		check_install = GMRFLib_FALSE;
	}

	assert(store->done_with_init == GMRFLib_TRUE);
	memcpy((void *) (store->iparm), (void *) (store->iparm_default), GMRFLib_PARDISO_PLEN * sizeof(int));

	store->mtype = GMRFLib_PARDISO_MTYPE;
	store->maxfct = 1;				       /* Maximum number of numerical factorizations.  */
	store->mnum = 1;				       /* Which factorization to use. */
	store->msglvl = 0;				       /* Print statistical information */
	store->nrhs = 0;
	store->err_code = 0;				       /* Initialize err_code flag */

	switch (flag) {
	case GMRFLib_PARDISO_FLAG_REORDER:
		store->phase = 11;			       // analysis
		store->iparm[4] = 0;			       /* 0 = compute the permutation */
		break;

	case GMRFLib_PARDISO_FLAG_SYMFACT:
		store->phase = 11;			       // analysis
		break;

	case GMRFLib_PARDISO_FLAG_CHOL:
		store->phase = 12;			       // Analysis, numerical factorization
		store->iparm[32] = 1;			       /* determinant */
		break;

	case GMRFLib_PARDISO_FLAG_QINV:
		store->phase = -22;
		store->iparm[35] = 1;			       /* do not overwrite internal factor L with selected inversion */
		store->iparm[36] = 0;			       /* return upper triangular Qinv */
		break;

	case GMRFLib_PARDISO_FLAG_SOLVE_L:
		store->phase = 33;			       // solve
		store->nrhs = 1;
		store->iparm[7] = 0;			       /* Max numbers of iterative refinement steps. */
		store->iparm[25] = 1;
		break;

	case GMRFLib_PARDISO_FLAG_SOLVE_LT:
		store->phase = 33;			       // solve
		store->nrhs = 1;
		store->iparm[7] = 0;			       /* Max numbers of iterative refinement steps. */
		store->iparm[25] = 2;
		break;

	case GMRFLib_PARDISO_FLAG_SOLVE_LLT:
		store->phase = 33;			       // solve
		store->nrhs = 1;
		store->iparm[7] = 0;			       /* Max numbers of iterative refinement steps. */
		store->iparm[25] = 0;
		break;

	default:
		GMRFLib_ERROR(GMRFLib_ESNH);
	}
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_check_install(int quiet)
{
	int *iparm = Calloc(GMRFLib_PARDISO_PLEN, int);
	int mtype = GMRFLib_PARDISO_MTYPE, err_code = 0, solver = 0;
	double *dparm = Calloc(GMRFLib_PARDISO_PLEN, double);
	void **pt = Calloc(GMRFLib_PARDISO_PLEN, void *);

	STDOUT_TO_DEV_NULL_START(quiet);
	pardisoinit(pt, &mtype, &solver, iparm, dparm, &err_code);
	STDOUT_TO_DEV_NULL_END;

	Free(pt);
	Free(dparm);
	Free(iparm);

	if (err_code != 0) {
		if (err_code == -10) {
			GMRFLib_ERROR(GMRFLib_EPARDISO_LICENSE_NOTFOUND);
		} else if (err_code == -11) {
			GMRFLib_ERROR(GMRFLib_EPARDISO_LICENSE_EXPIRED);
		} else if (err_code == -12) {
			GMRFLib_ERROR(GMRFLib_EPARDISO_LICENSE_ERR_USERNAME);
		} else {
			GMRFLib_ERROR(GMRFLib_ESNH);
		}
		assert(0 == 1);
	}

	return (GMRFLib_SUCCESS);
}

double GMRFLib_pardiso_Qfunc_default(int i, int j, void *arg)
{
	GMRFLib_graph_tp *g = (GMRFLib_graph_tp *) arg;
	return (i == j ? g->n + 2.0 * g->nnbs[i] : -1.0);
}


int GMRFLib_pardiso_reorder(GMRFLib_pardiso_store_tp * store, GMRFLib_graph_tp * graph, int **reordering)
{
	assert(store != NULL);
	assert(store->done_with_init == GMRFLib_TRUE);

	if (store->done_with_reorder == GMRFLib_TRUE) {
		return GMRFLib_SUCCESS;
	}

	GMRFLib_ENTER_ROUTINE;

	int i;
	GMRFLib_csr_tp *Q = NULL;

	GMRFLib_copy_graph(&(store->graph), graph);
	GMRFLib_pardiso_setparam(GMRFLib_PARDISO_FLAG_REORDER, store);
	GMRFLib_Q2csr(&Q, graph, GMRFLib_pardiso_Qfunc_default, (void *) graph);
	GMRFLib_csr_base(1, Q);
	if (verbose_mode)
		GMRFLib_print_csr(stdout, Q);
	GMRFLib_Q2csr_check(Q);

	pardiso(store->pt, &(store->maxfct), &(store->mnum), &(store->mtype), &(store->phase),
		&(Q->n), Q->a, Q->ia, Q->ja, NULL, 
		&(store->nrhs), store->iparm, &(store->msglvl), &(store->dummy), &(store->dummy), &(store->err_code), store->dparm);

	if (store->err_code) {
		GMRFLib_ERROR(GMRFLib_EPARDISO_INTERNAL_ERROR);
	}

	// I do not really know if this reordering is used for anything, so I fill it with trash in case it is
	*reordering = Calloc(Q->n, int);
	for(i = 0; i < Q->n; i++) {
		(*reordering)[i] = NAN;
	}

	store->done_with_reorder = GMRFLib_TRUE;
	store->L_nnz = store->iparm[17] - 1;
	GMRFLib_free_csr(&Q);

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_symfact(GMRFLib_pardiso_store_tp * store)
{
	assert(store->done_with_init == GMRFLib_TRUE);
	assert(store->done_with_reorder == GMRFLib_TRUE);

	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_build(GMRFLib_pardiso_store_tp * store, GMRFLib_graph_tp * graph, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg)
{
	GMRFLib_ENTER_ROUTINE;

	assert(store->done_with_init == GMRFLib_TRUE);
	assert(store->done_with_reorder == GMRFLib_TRUE);

	if (store->done_with_build == GMRFLib_TRUE && store->Q) {
		GMRFLib_free_csr(&(store->Q));
	}
	GMRFLib_Q2csr(&(store->Q), graph, Qfunc, (void *) Qfunc_arg);
	GMRFLib_csr_base(1, store->Q);
	GMRFLib_Q2csr_check(store->Q);
	if (verbose_mode)
		GMRFLib_print_csr(stdout, store->Q);

	store->done_with_build = GMRFLib_TRUE;
	GMRFLib_LEAVE_ROUTINE;

	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_chol(GMRFLib_pardiso_store_tp * store)
{
	GMRFLib_ENTER_ROUTINE;

	assert(store->done_with_init == GMRFLib_TRUE);
	assert(store->done_with_reorder == GMRFLib_TRUE);
	assert(store->done_with_build == GMRFLib_TRUE);

	GMRFLib_pardiso_setparam(GMRFLib_PARDISO_FLAG_CHOL, store);

	FIXME("call chol");
	pardiso(store->pt, &(store->maxfct), &(store->mnum), &(store->mtype), &(store->phase),
		&(store->Q->n), store->Q->a, store->Q->ia, store->Q->ja, NULL, &(store->nrhs),
		store->iparm, &(store->msglvl), NULL, NULL, &(store->err_code), store->dparm);
	FIXME("call chol done");

	if (store->err_code != 0) {
		GMRFLib_ERROR(GMRFLib_EPOSDEF);
	}

	if (store->iparm[22] > 0) {
		printf("\nERROR not pos def matrix, #neg.eigen %d", store->iparm[22]);
		GMRFLib_ERROR(GMRFLib_EPOSDEF);
	}

	store->log_det_Q = store->dparm[32];
	store->done_with_chol = GMRFLib_TRUE;
	GMRFLib_LEAVE_ROUTINE;

	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_solve_core(GMRFLib_pardiso_store_tp * store, GMRFLib_pardiso_flag_tp flag, double *x, double *b)
{
	assert(store->done_with_init == GMRFLib_TRUE);
	assert(store->done_with_reorder == GMRFLib_TRUE);
	assert(store->done_with_build == GMRFLib_TRUE);
	assert(store->done_with_chol == GMRFLib_TRUE);

	// this is so that the RHS can be overwritten
	double *xx = Calloc(store->Q->n, double);

	GMRFLib_pardiso_setparam(flag, store);
	pardiso(store->pt, &(store->maxfct), &(store->mnum), &(store->mtype), &(store->phase),
		&(store->Q->n), store->Q->a, store->Q->ia, store->Q->ja, NULL,
		&(store->nrhs), store->iparm, &(store->msglvl), b, xx, &(store->err_code), store->dparm);

	if (store->err_code != 0) {
		GMRFLib_ERROR(GMRFLib_EPARDISO_INTERNAL_ERROR);
	}

	memcpy((void *) x, (void *) xx, store->Q->n * sizeof(double));
	Free(xx);

	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_solve_L(GMRFLib_pardiso_store_tp * store, double *x, double *b)
{
	GMRFLib_ENTER_ROUTINE;
	int res = GMRFLib_pardiso_solve_core(store, GMRFLib_PARDISO_FLAG_SOLVE_L, x, b);
	GMRFLib_LEAVE_ROUTINE;

	return res;
}

int GMRFLib_pardiso_solve_LT(GMRFLib_pardiso_store_tp * store, double *x, double *b)
{
	GMRFLib_ENTER_ROUTINE;
	int res = GMRFLib_pardiso_solve_core(store, GMRFLib_PARDISO_FLAG_SOLVE_LT, x, b);
	GMRFLib_LEAVE_ROUTINE;

	return res;
}

int GMRFLib_pardiso_solve_LLT(GMRFLib_pardiso_store_tp * store, double *x, double *b)
{
	GMRFLib_ENTER_ROUTINE;
	int res = GMRFLib_pardiso_solve_core(store, GMRFLib_PARDISO_FLAG_SOLVE_LLT, x, b);
	GMRFLib_LEAVE_ROUTINE;

	return res;
}

double GMRFLib_pardiso_logdet(GMRFLib_pardiso_store_tp * store)
{
	return (store->log_det_Q);
}

int GMRFLib_pardiso_bitmap(void)
{
	/*
	 * Not available for PARDISO, USE TAUCS
	 */
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_Qinv_INLA(GMRFLib_problem_tp * problem) 
{
	if (problem == NULL) {
		return GMRFLib_SUCCESS;
	}

	GMRFLib_ENTER_ROUTINE;
	GMRFLib_pardiso_Qinv(problem->sub_sm_fact.PARDISO_fact);

	GMRFLib_csr_tp *Qi = problem->sub_sm_fact.PARDISO_fact->Qinv;
	int n = Qi->n, i, j, jj, k, kk;
	double value;
	map_id **Qinv = Calloc(n, map_id *);

	assert(Qi->base == 0);

	for (i = k = 0; i < n; i++) {
		int nnb;

		nnb = Qi->ia[i + 1] - Qi->ia[i];
		Qinv[i] = Calloc(1, map_id);
		map_id_init_hint(Qinv[i], nnb);
		for (jj = 0; jj < nnb; jj++) {
			j = Qi->ja[k];
			map_id_set(Qinv[i], j, Qi->a[k]);
			k++;
		}
	}

	if (problem->sub_constr && problem->sub_constr->nc > 0) {
#pragma omp parallel for private(i, k, j, kk, value)
		for (i = 0; i < n; i++) {
			for (k = -1; (k = (int) map_id_next(Qinv[i], k)) != -1;) {
				j = Qinv[i]->contents[k].key;
				map_id_get(Qinv[i], j, &value);
				for (kk = 0; kk < problem->sub_constr->nc; kk++) {
					value -= problem->constr_m[i + kk * n] * problem->qi_at_m[j + kk * n];
				}
				map_id_set(Qinv[i], j, value);
			}
		}
	}

	GMRFLib_Qinv_tp *subQinv = Calloc(1, GMRFLib_Qinv_tp);

	subQinv->Qinv = Qinv;
	subQinv->mapping = Calloc(1, map_ii);
	map_ii_init_hint(subQinv->mapping, n);
	for (i = 0; i < n; i++) {
		//map_ii_set(subQinv->mapping, i, i);
		//printf("Mapping %d %d\n", i, problem->sub_graph->mothergraph_idx[i]);
		map_ii_set(subQinv->mapping, problem->sub_graph->mothergraph_idx[i], problem->sub_graph->mothergraph_idx[i]);
	}

	problem->sub_inverse = subQinv;
	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_Qinv(GMRFLib_pardiso_store_tp * store)
{
	GMRFLib_ENTER_ROUTINE;

	assert(store->done_with_init == GMRFLib_TRUE);
	assert(store->done_with_reorder == GMRFLib_TRUE);
	assert(store->done_with_build == GMRFLib_TRUE);
	assert(store->done_with_chol == GMRFLib_TRUE);

	if (store->Qinv) {
		GMRFLib_free_csr(&(store->Qinv));
	}
	
	GMRFLib_duplicate_csr(&(store->Qinv), store->Q);
	GMRFLib_csr_base(1, store->Qinv);

	GMRFLib_pardiso_setparam(GMRFLib_PARDISO_FLAG_QINV, store);
	pardiso(store->pt, &(store->maxfct), &(store->mnum), &(store->mtype), &(store->phase),
		&(store->Qinv->n), store->Qinv->a, store->Qinv->ia, store->Qinv->ja,
		NULL, &(store->nrhs), store->iparm, &(store->msglvl), NULL, NULL, &(store->err_code), store->dparm);

	if (store->err_code != 0) {
		GMRFLib_ERROR(GMRFLib_EPARDISO_INTERNAL_ERROR);
	}

	GMRFLib_csr_base(0, store->Qinv);
	if (verbose_mode)
		GMRFLib_print_csr(stdout, store->Qinv);

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_pardiso_free(GMRFLib_pardiso_store_tp ** store)
{
	if (*store == NULL) {
		return GMRFLib_SUCCESS;
	}
	GMRFLib_ENTER_ROUTINE;
	(*store)->phase = -1;
	pardiso((*store)->pt, &((*store)->maxfct), &((*store)->mnum), &((*store)->mtype), &((*store)->phase),
		//
		//&((*store)->Q->n), (*store)->Q->a, (*store)->Q->ia, (*store)->Q->ja,
		&((*store)->idummy), &((*store)->dummy), &((*store)->idummy), &((*store)->idummy),
		//
		&((*store)->idummy),
		&((*store)->nrhs), (*store)->iparm, &((*store)->msglvl), NULL, NULL, &((*store)->err_code), (*store)->dparm);

	GMRFLib_free_graph((*store)->graph);
	GMRFLib_free_csr(&((*store)->Q));
	GMRFLib_free_csr(&((*store)->Qinv));
	Free((*store)->reordering);
	Free(*store);
	GMRFLib_LEAVE_ROUTINE;

	return GMRFLib_SUCCESS;
}

int GMRFLib_duplicate_pardiso_store(GMRFLib_pardiso_store_tp ** new, GMRFLib_pardiso_store_tp * old)
{
	if (old == NULL) {
		*new = NULL;
		return GMRFLib_SUCCESS;
	}
	GMRFLib_ENTER_ROUTINE;

	assert(old->done_with_init == GMRFLib_TRUE);
	GMRFLib_pardiso_init(new);

	if (old->done_with_reorder == GMRFLib_TRUE) {
		GMRFLib_pardiso_reorder(*new, old->graph, &(old->reordering));
		GMRFLib_pardiso_symfact(*new);		       // does not do anything really
	}

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

//
int my_pardiso_test(void)
{
	int err = 0;

	if (0) {
		err = GMRFLib_pardiso_check_install(1);
		if (err == GMRFLib_SUCCESS) {
			printf("PARDISO OK\n");
		} else {
			printf("PARDISO FAIL\n");
		}
	}

	GMRFLib_tabulate_Qfunc_tp *Qtab;
	GMRFLib_graph_tp *g;

	GMRFLib_tabulate_Qfunc_from_file(&Qtab, &g, "A1000.txt", -1, NULL, NULL, NULL);

	GMRFLib_csr_tp *csr, *csr2;

	GMRFLib_Q2csr(&csr, g, Qtab->Qfunc, Qtab->Qfunc_arg);
	//GMRFLib_print_csr(stdout, csr);
	P(csr->n);
	
	GMRFLib_duplicate_csr(&csr2, csr);
	// GMRFLib_print_csr(stdout, csr2);

	GMRFLib_csr2Q(&Qtab, &g, csr2);
	// GMRFLib_print_Qfunc(stdout, g, Qtab->Qfunc, Qtab->Qfunc_arg);

	int *perm = NULL;
	int i, j;

	//GMRFLib_print_graph(stdout, g);
	GMRFLib_pardiso_store_tp *store = NULL;
	GMRFLib_pardiso_init(&store);
	GMRFLib_pardiso_reorder(store, g, &perm);
	for(i=0; i<10; i++){
		P(i);
		GMRFLib_pardiso_build(store, g, Qtab->Qfunc, Qtab->Qfunc_arg);
		GMRFLib_pardiso_chol(store);
		GMRFLib_pardiso_Qinv(store);
	}
	
	//omp_set_num_threads(4);
	omp_set_nested(1);
#pragma omp parallel for private(i, j) num_threads(GMRFLib_openmp->max_threads_inner)
	for (j = 0; j < 100; j++) {
		P(j);
		GMRFLib_pardiso_store_tp *store2 = NULL;

		GMRFLib_pardiso_init(&store2);
		GMRFLib_pardiso_reorder(store2, g, &perm);
		GMRFLib_pardiso_build(store2, g, Qtab->Qfunc, Qtab->Qfunc_arg);

		GMRFLib_pardiso_chol(store2);

		int view = 0;
		double *x = Calloc(g->n, double);
		double *b = Calloc(g->n, double);

		for (i = 0; i < g->n; i++) {
			b[i] = ISQR(i + 1);
		}
		GMRFLib_pardiso_solve_LLT(store2, x, b);
		if (view) {
			printf("solve LLT\n");
			for (i = 0; i < g->n; i++) {
				printf("i %d  x= %f  b=%f\n", i, x[i], b[i]);
			}
		}

		for (i = 0; i < g->n; i++) {
			b[i] = ISQR(i + 1);
		}
		GMRFLib_pardiso_solve_LT(store2, x, b);
		if (view) {
			printf("solve LT\n");
			for (i = 0; i < g->n; i++) {
				printf("i %d  x= %f  b=%f\n", i, x[i], b[i]);
			}
		}

		for (i = 0; i < g->n; i++) {
			b[i] = ISQR(i + 1);
		}
		GMRFLib_pardiso_solve_L(store2, x, b);
		if (view) {
			printf("solve L\n");
			for (i = 0; i < g->n; i++) {
				printf("i %d  x= %f  b=%f\n", i, x[i], b[i]);
			}
		}

		GMRFLib_pardiso_Qinv(store2);
		GMRFLib_pardiso_free(&store2);
		Free(x);
		Free(b);
	}
	
	printf("call Qinv\n");
	GMRFLib_pardiso_Qinv(store);

	printf("call free\n");
	GMRFLib_free_tabulate_Qfunc(Qtab);
	GMRFLib_pardiso_free(&store);
	GMRFLib_free_csr(&csr);
	GMRFLib_free_csr(&csr2);
	GMRFLib_free_graph(g);
	Free(perm);

	exit(0);
}

#undef WARNING
