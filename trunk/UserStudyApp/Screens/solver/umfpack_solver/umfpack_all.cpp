//#define WIN32
//#define __cplusplus
#define NBLAS
#define NCHOLMOD
//#define NDEBUG
#define NPOSIX
#define UMF_WINDOWS

#include "umfpack.h"

#include	"UFconfig.hpp"

#include	"amd_1.hpp"
#include	"amd_2.hpp"
#include	"amd_aat.hpp"
#include	"amd_control.hpp"
#include	"amd_defaults.hpp"
#include	"amd_dump.hpp"
#include	"amd_global.hpp"
#include	"amd_info.hpp"
#include	"amd_order.hpp"
#include	"amd_postorder.hpp"
#include	"amd_post_tree.hpp"
#include	"amd_preprocess.hpp"
#include	"amd_valid.hpp"

#include	"umfpack_timer.hpp"
#include	"umfpack_tictoc.hpp"
#include	"umfpack_global.hpp"

#define DINT
#include	"umf_analyze.hpp"
#include	"umf_apply_order.hpp"
#include	"umf_colamd.hpp"
#include	"umf_free.hpp"

#include	"umf_triplet.hpp"		// for umfdi_triplet_nomap_nox
#include	"umf_assemble.hpp"		// for umfdi_assemble
#include	"umf_store_lu.hpp"		// for umfdi_store_lu

#include	"umf_multicompile.hpp"	// for multiple stuff

#include	"umf_blas3_update.hpp"
#include	"umf_build_tuples.hpp"
#include	"umf_cholmod.hpp"
#include	"umf_create_element.hpp"
#include	"umf_dump.hpp"
#include	"umf_extend_front.hpp"
#include	"umf_fsize.hpp"
#include	"umf_garbage_collection.hpp"
#include	"umf_get_memory.hpp"
#include	"umf_grow_front.hpp"
#include	"umf_init_front.hpp"
#include	"umf_is_permutation.hpp"
#include	"umf_kernel.hpp"
#include	"umf_kernel_init.hpp"
#include	"umf_kernel_wrapup.hpp"
#include	"umf_local_search.hpp"
#include	"umf_lsolve.hpp"
#include	"umf_malloc.hpp"
#include	"umf_mem_alloc_element.hpp"
#include	"umf_mem_alloc_head_block.hpp"
#include	"umf_mem_alloc_tail_block.hpp"
#include	"umf_mem_free_tail_block.hpp"
#include	"umf_mem_init_memoryspace.hpp"

#include	"umf_realloc.hpp"
#include	"umf_report_perm.hpp"
#include	"umf_report_vector.hpp"
#include	"umf_row_search.hpp"
#include	"umf_scale.hpp"
#include	"umf_scale_column.hpp"
#include	"umf_set_stats.hpp"
#include	"umf_singletons.hpp"
#include	"umf_solve.hpp"
#include	"umf_start_front.hpp"
#include	"umf_symbolic_usage.hpp"
#include	"umf_transpose.hpp"
#include	"umf_tuple_lengths.hpp"
#include	"umf_usolve.hpp"
#include	"umf_valid_numeric.hpp"
#include	"umf_valid_symbolic.hpp"

#include	"umfpack_defaults.hpp"
#include	"umfpack_free_numeric.hpp"
#include	"umfpack_free_symbolic.hpp"
#include	"umfpack_get_determinant.hpp"
#include	"umfpack_get_lunz.hpp"
#include	"umfpack_get_numeric.hpp"
#include	"umfpack_get_symbolic.hpp"
#include	"umfpack_load_numeric.hpp"
#include	"umfpack_load_symbolic.hpp"
#include	"umfpack_numeric.hpp"
#include	"umfpack_qsymbolic.hpp"
#include	"umfpack_report_control.hpp"
#include	"umfpack_report_info.hpp"
#include	"umfpack_report_matrix.hpp"
#include	"umfpack_report_numeric.hpp"
#include	"umfpack_report_perm.hpp"
#include	"umfpack_report_status.hpp"
#include	"umfpack_report_symbolic.hpp"
#include	"umfpack_report_triplet.hpp"
#include	"umfpack_report_vector.hpp"
#include	"umfpack_save_numeric.hpp"
#include	"umfpack_save_symbolic.hpp"
#include	"umfpack_scale.hpp"
#include	"umfpack_symbolic.hpp"
#include	"umfpack_transpose.hpp"
#include	"umfpack_col_to_triplet.hpp"
#include	"umfpack_triplet_to_col.hpp"

#undef NCHOLMOD
#undef WIN32
//#undef __cplusplus
