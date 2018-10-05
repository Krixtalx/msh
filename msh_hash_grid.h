/************************
 * TO COMPILE : g++ -O3 -lflann_cpp -fopenmp -I../dev msh_hash_grid_benchmark.cpp -o test.exe
 * TODOs:
 * [x] Fix issue when _init function cannot be used if no implementation is declared.
 * [x] Optimization - in both knn and radius I need a better way to determine whether I can early out
 * [x] Optimization - see if I can simplify the radius search function for small search radii.
 *        --> Very small gains given the increase in complexity.
 * [x] Optimization - spatial locality - sort linear data on bin idx or morton curves
 *        --> Does not seem to produce improvement. Something else must be dominating the times
 *        --> Maybe morton curves will be better
 * [x] Fix knn search
 *     [x] Multithread knn
 * [x] Multithreading
 *     [x] API for supplying more then a single point
 *     [x] OpenMP optional support ( if -fopenmp was supplied, sequential otherwise)
 *     [ ] Replace openMP with c11 threads?
 * [x] Params struct for searching
 *    [ ] Compatibility function?
 * [ ] Clean the API, prepare examples.
 * [ ] Add 2d support on API level
 * [ ] Docs
 *     NOTE(maciej): Remember to tell the user about sensitivity to max_n_neigh, as it is essentially
 *                   spreading out the memory. Should some functions to query point density be added?
 * [ ] Assert proof 
 **********************/

#ifndef MSH_HASH_GRID_H
#define MSH_HASH_GRID_H

#if defined(MSH_HASH_GRID_INCLUDE_HEADERS)
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#endif

#ifndef MSH_HG_MALLOC
#define MSH_HG_MALLOC(x) malloc((x))
#endif

#ifndef MSH_HG_MEMSET
#define MSH_HG_MEMSET(x,y,z) memset((x), (y), (z))
#endif


#ifndef MSH_HG_CALLOC
#define MSH_HG_CALLOC(x,y) calloc((x), (y))
#endif

#ifndef MSH_HG_REALLOC
#define MSH_HG_REALLOC(x,y) realloc((x), (y))
#endif

#ifndef MSH_HG_FREE
#define MSH_HG_FREE(x) free((x))
#endif

#if defined(_OPENMP)
#include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct msh_hash_grid msh_hash_grid_t;

typedef struct msh_hash_grid_search_desc
{
  float* query_pts;
  size_t n_query_pts;

  float* distances_sq;
  int32_t* indices;
  size_t* n_neighbors;

  union
  {
    float radius;
    size_t k;
  };
  size_t max_n_neigh;
  int sort;
} msh_hash_grid_search_desc_t;

void msh_hash_grid_init( msh_hash_grid_t* hg, const float* pts, const int n_pts, const float radius );
void msh_hash_grid_term( msh_hash_grid_t* hg );
int msh_hash_grid_radius_search_mt( const msh_hash_grid_t* hg,
                                    msh_hash_grid_search_desc_t* search_desc );
int msh_hash_grid_knn_search_mt( const msh_hash_grid_t* hg, 
                                 msh_hash_grid_search_desc_t* search_desc );

// We we'd like to deprecate these functions in the future
int msh_hash_grid_radius_search( const msh_hash_grid_t* hg, const float* query_pt, const float radius, 
                                 float* dists_sq, int32_t* indices, size_t max_n_results, int sort );
int msh_hash_grid_knn_search( const msh_hash_grid_t* hg, const float* query_pt, const size_t k,
                              float* dists_sq, int* indices, int sort );




typedef struct msh_hg_v3 
{ 
  float x, y, z; 
} msh_hg_v3_t;

typedef struct msh_hg_v3i
{
  float x, y, z;
  int32_t i;
} msh_hg_v3i_t;

typedef struct msh_hg_bin_data msh_hg__bin_data_t;
typedef struct msh_hg_bin_info msh_hg__bin_info_t;
typedef struct msh_hg_map msh_hg_map_t;

typedef struct msh_hash_grid
{
  size_t width;
  size_t height;
  size_t depth;
  float cell_size;

  msh_hg_v3_t min_pt;
  msh_hg_v3_t max_pt;

  msh_hg_map_t* bin_table;
  msh_hg_v3i_t* data_buffer;
  msh_hg__bin_info_t* offsets;

  int32_t   _slab_size;
  float _inv_cell_size;
} msh_hash_grid_t;


#ifdef __cplusplus
}
#endif

#endif /* MSH_HASH_GRID_H */

#ifdef MSH_HASH_GRID_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy of msh_array
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct msh_hg_array_header
{
  size_t len;
  size_t cap;
} msh_hg_array_hdr_t;

#define msh_hg_array(T) T*

void* msh_hg__array_grow(const void *array, size_t new_len, size_t elem_size);

#define msh_hg_array__grow_formula(x)    ((2*(x)+5))
#define msh_hg_array__hdr(a)             ((msh_hg_array_hdr_t *)((char *)(a) - sizeof(msh_hg_array_hdr_t)))

#define msh_hg_array_len(a)              ((a) ? (msh_hg_array__hdr((a))->len) : 0)
#define msh_hg_array_cap(a)              ((a) ? (msh_hg_array__hdr((a))->cap) : 0)
#define msh_hg_array_front(a)            ((a) ? (a) : NULL)
#define msh_hg_array_back(a)             (msh_hg_array_len((a)) ? ((a) + msh_hg_array_len((a)) - 1 ) : NULL)

#define msh_hg_array_free(a)             ((a) ? (MSH_HG_FREE(msh_hg_array__hdr(a)), (a) = NULL) : 0 )                                  
#define msh_hg_array_fit(a, n)           ((n) <= msh_hg_array_cap(a) ? (0) : ( *(void**)&(a) = msh_hg__array_grow((a), (n), sizeof(*(a))) )) 
#define msh_hg_array_push(a, ...)        (msh_hg_array_fit((a), 1 + msh_hg_array_len((a))), (a)[msh_hg_array__hdr(a)->len++] = (__VA_ARGS__))

#define MSH_HG_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MSH_HG_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MSH_HG_MAX3(a, b, c) MSH_HG_MAX(MSH_HG_MAX(a,b), MSH_HG_MAX(b,c))

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy of msh_map
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct msh_hg_map 
{
  uint64_t* keys;
  uint64_t* vals;
  size_t _len;
  size_t _cap;
} msh_hg_map_t;

uint64_t  msh_hg_hash_uint64( uint64_t x );
void      msh_hg_map_init( msh_hg_map_t* map, uint32_t cap );
void      msh_hg_map_free( msh_hg_map_t* map );
size_t    msh_hg_map_len( msh_hg_map_t* map );
size_t    msh_hg_map_cap( msh_hg_map_t* map ); 
void      msh_hg_map_insert( msh_hg_map_t* map, uint64_t key, uint64_t val );
uint64_t* msh_hg_map_get( const msh_hg_map_t* map, uint64_t key );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual start of implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#define MSH_HG_INLINE __forceinline
#else
#define MSH_HG_INLINE __attribute__((always_inline, unused)) inline
#endif


MSH_HG_INLINE msh_hg_v3_t
msh_hg__vec3_add( msh_hg_v3_t a, msh_hg_v3_t b )
{
  return (msh_hg_v3_t) { a.x + b.x, a.y + b.y, a.z + b.z };
}

MSH_HG_INLINE msh_hg_v3_t
msh_hg__vec3_sub( msh_hg_v3_t a, msh_hg_v3_t b )
{
  return (msh_hg_v3_t) { a.x - b.x, a.y - b.y, a.z - b.z };
}

typedef struct msh_hg_bin_data
{ 
  int32_t n_pts;
  msh_hg_v3i_t* data;
} msh_hg__bin_data_t;

typedef struct msh_hg_bin_info
{ 
  int32_t offset;
  int32_t length;
} msh_hg__bin_info_t;

MSH_HG_INLINE uint64_t
msh_hash_grid__bin_pt( const msh_hash_grid_t* hg, uint64_t ix, uint64_t iy, uint64_t iz )
{
  uint64_t bin_idx = iz * hg->_slab_size + iy * hg->width + ix;
  return bin_idx;
}

int uint64_compare( const void * a, const void * b )
{
  return ( *(uint64_t*)a - *(uint64_t*)b );
}

void
msh_hash_grid_init( msh_hash_grid_t* hg, const float* pts, const int n_pts, const float radius )
{
  // Compute bbox
  hg->min_pt = (msh_hg_v3_t){ .x =  1e9, .y =  1e9, .z =  1e9 };
  hg->max_pt = (msh_hg_v3_t){ .x = -1e9, .y = -1e9, .z = -1e9 };

  for( int i = 0; i < n_pts; ++i )
  {
    const float* pt_ptr = &pts[3*i];
    msh_hg_v3_t pt = { .x = pt_ptr[0], .y = pt_ptr[1], .z = pt_ptr[2] };
    hg->min_pt.x = (hg->min_pt.x > pt.x) ? pt.x : hg->min_pt.x;
    hg->min_pt.y = (hg->min_pt.y > pt.y) ? pt.y : hg->min_pt.y;
    hg->min_pt.z = (hg->min_pt.z > pt.z) ? pt.z : hg->min_pt.z;

    hg->max_pt.x = (hg->max_pt.x < pt.x) ? pt.x : hg->max_pt.x;
    hg->max_pt.y = (hg->max_pt.y < pt.y) ? pt.y : hg->max_pt.y;
    hg->max_pt.z = (hg->max_pt.z < pt.z) ? pt.z : hg->max_pt.z;
  }

  // Calculate dimensions
  float dim_x = (hg->max_pt.x - hg->min_pt.x);
  float dim_y = (hg->max_pt.y - hg->min_pt.y);
  float dim_z = (hg->max_pt.z - hg->min_pt.z);
  float dim = MSH_HG_MAX3( dim_x, dim_y, dim_z );
  
  // Calculate cell size
  if( radius > 0.0 ) { hg->cell_size = 2.0f * radius; }
  else               { hg->cell_size = dim / (32 * sqrtf(3.0f)); }

  hg->width     = (int)(dim_x / hg->cell_size + 1.0);
  hg->height    = (int)(dim_y / hg->cell_size + 1.0) ;
  hg->depth     = (int)(dim_z / hg->cell_size + 1.0) ;
  hg->_inv_cell_size = 1.0f / hg->cell_size;
  hg->_slab_size = hg->height * hg->width;

  // Create hash table
  hg->bin_table = (msh_hg_map_t*)MSH_HG_CALLOC( 1, sizeof(msh_hg_map_t) );
  msh_hg_map_init( hg->bin_table, 1024 );
  msh_hg_array( msh_hg__bin_data_t ) bin_table_data = 0;
  uint64_t n_bins = 0;
  for( int i = 0 ; i < n_pts; ++i )
  {
    msh_hg_v3i_t pt_data = (msh_hg_v3i_t){ .x = pts[3*i+0], 
                                           .y = pts[3*i+1], 
                                           .z = pts[3*i+2], 
                                           .i = i };
 
    uint64_t ix = (uint64_t)((pt_data.x - hg->min_pt.x ) * hg->_inv_cell_size);
    uint64_t iy = (uint64_t)((pt_data.y - hg->min_pt.y ) * hg->_inv_cell_size);
    uint64_t iz = (uint64_t)((pt_data.z - hg->min_pt.z ) * hg->_inv_cell_size);

    uint64_t bin_idx = msh_hash_grid__bin_pt( hg, ix, iy, iz );

    // NOTE(maciej): In msh_map we can't have 0 as key
    uint64_t* bin_table_idx = msh_hg_map_get( hg->bin_table, bin_idx );
    
    if( bin_table_idx ) 
    { 
      bin_table_data[*bin_table_idx].n_pts += 1;
      msh_hg_array_push( bin_table_data[*bin_table_idx].data, pt_data );
    }
    else 
    {
      msh_hg_map_insert( hg->bin_table, bin_idx, n_bins );
      
      msh_hg__bin_data_t new_bin = {0};
      new_bin.n_pts = 1;
      msh_hg_array_push( new_bin.data, pt_data );
      msh_hg_array_push( bin_table_data, new_bin );
      n_bins++;
    }
  }

  // Prepare storage for linear data
  hg->offsets     = (msh_hg__bin_info_t*)MSH_HG_MALLOC( n_bins * sizeof(msh_hg__bin_info_t) );
  hg->data_buffer = (msh_hg_v3i_t*)MSH_HG_MALLOC( n_pts * sizeof( msh_hg_v3i_t ) );
  MSH_HG_MEMSET( hg->offsets, 0, n_bins * sizeof(msh_hg__bin_info_t) );


  // Gather indices of bins that have data in them from hash table
  msh_array( uint64_t ) filled_bin_indices = {0};
  for( size_t i = 0; i < msh_hg_map_cap(hg->bin_table); ++i )
  {
    // Remember that msh_hg_map internally increments the index, so we need to decrement it here.
    if( hg->bin_table->keys[i] ) 
    { 
      msh_array_push( filled_bin_indices, hg->bin_table->keys[i] - 1); 
    }
  }
  qsort( filled_bin_indices, msh_array_len( filled_bin_indices ), sizeof(uint64_t), uint64_compare );

  // Now lay the data into an array based on the sorted keys (following fill order)
  // TODO(maciej): Morton ordering?
  int offset = 0;
  for( size_t i = 0; i < msh_array_len(filled_bin_indices); ++i )
  {
    uint64_t* bin_index = msh_hg_map_get( hg->bin_table, filled_bin_indices[i] );
    assert( bin_index );
    msh_hg__bin_data_t* bin = &bin_table_data[ *bin_index ];
    assert( bin );
    int n_bin_pts = bin->n_pts;
    for( int j = 0; j < n_bin_pts; ++j )
    {
      hg->data_buffer[ offset + j ] = bin->data[j] ;
    }
    hg->offsets[ *bin_index ] = (msh_hg__bin_info_t) { .offset = offset, .length = n_bin_pts };
    offset += n_bin_pts;
  }


  // Clean-up temporary data
  for( size_t i = 0; i < n_bins; ++i )
  {
    msh_hg_array_free( bin_table_data[i].data );
  }
  msh_hg_array_free( bin_table_data );
}

void
msh_hash_grid_term( msh_hash_grid_t* hg )
{
  hg->width          = 0;
  hg->height         = 0;
  hg->depth          = 0;
  hg->cell_size      = 0.0f;
  hg->min_pt         = (msh_hg_v3_t){ 0.0f, 0.0f, 0.0f };
  hg->max_pt         = (msh_hg_v3_t){ 0.0f, 0.0f, 0.0f };
  hg->_slab_size     = 0.0f;
  hg->_inv_cell_size = 0.0f;

  MSH_HG_FREE( hg->data_buffer ); hg->data_buffer = NULL;
  MSH_HG_FREE( hg->offsets );     hg->offsets = NULL;
  MSH_HG_FREE( hg->bin_table );   hg->bin_table = NULL;
}


// NOTE(maciej): This implementation is a special case modification of a templated
// sort by Sean T. Barret from stb.h. We simply want to allow sorting both the indices
// and distances if user requested returning sorted results.
void 
msh_hash_grid__ins_sort( float *dists, int32_t* indices, int n )
{
   int i = 0;
   int j = 0;
   for( i = 1; i < n; ++i )
   {
      float da   = dists[i];
      int32_t ia = indices[i];
      j = i;
      while( j > 0 )
      {
        float db = dists[j-1];
        if( da >= db ) { break; }
        dists[j] = dists[j-1];
        indices[j] = indices[j-1];
        --j;
      }
      if (i != j)
      {
        dists[j] = da;
        indices[j] = ia;
      }
   }
}

void 
msh_hash_grid__quick_sort( float *dists, int32_t* indices, int n )
{
   // threshold for transitioning to insertion sort
   while( n > 12 ) 
   {
      float da, db, dt;
      int32_t it = 0;
      int32_t c01, c12, c, m, i, j;

      // compute median of three
      m = n >> 1;
      da = dists[0];
      db = dists[m];
      c = da < db;
      c01 = c;
      da = dists[m];
      db = dists[n-1];
      c = da < db;
      c12 = c;
      // if 0 >= mid >= end, or 0 < mid < end, then use mid
      if( c01 != c12 ) 
      {
         // otherwise, we'll need to swap something else to middle
         int32_t z;
         da = dists[0];
         db = dists[n-1];
         c = da < db;
         // 0>mid && mid<n:  0>n => n; 0<n => 0
         // 0<mid && mid>n:  0>n => 0; 0<n => n
         z = (c == c12) ? 0 : n-1;
         dt = dists[z];
         dists[z] = dists[m];
         dists[m] = dt;
         it = indices[z];
         indices[z] = indices[m];
         indices[m] = it;
      }
      // now dists[m] is the median-of-three  swap it to the beginning so it won't move around 
      dt = dists[0];
      dists[0] = dists[m];
      dists[m] = dt;
      it = indices[0];
      indices[0] = indices[m];
      indices[m] = it; 

      // partition loop 
      i=1;
      j=n-1;
      for(;;)
      {
         // handling of equality is crucial here for sentinels & efficiency with duplicates
         db = dists[0];
         for( ;;++i ) 
         {
            da = dists[i];
            c = da < db;
            if (!c) break;
         }
         da = dists[0];
         for( ;;--j ) {
            db = dists[j];
            c = da < db;
            if (!c) break;
         }
         // make sure we haven't crossed
         if( i >= j ) { break; }
         dt = dists[i];
         dists[i] = dists[j];
         dists[j] = dt;
         it = indices[i];
         indices[i] = indices[j];
         indices[j] = it;

         ++i;
         --j;
      }
      // recurse on smaller side, iterate on larger
      if( j < (n-i) ) 
      {
         msh_hash_grid__quick_sort( dists, indices, j );
         dists = dists + i;
         indices = indices + i;
         n = n - i;
      } 
      else 
      {
         msh_hash_grid__quick_sort( dists + i, indices + i, n - i );
         n = j;
      }
   }
}

void msh_hash_grid__sort( float* dists, int32_t* indices, int n )
{
  msh_hash_grid__quick_sort( dists, indices, n );
  msh_hash_grid__ins_sort( dists, indices, n );
}

typedef struct msh_hash_grid_dist_storage
{
  size_t cap;
  size_t len;
  int32_t max_dist_idx;
  float* dists;
  int32_t* indices;
} msh_hash_grid_dist_storage_t;

MSH_HG_INLINE void 
msh_hash_grid_dist_storage_init( msh_hash_grid_dist_storage_t* q,
                                 const int k, float* dists, int32_t* indices )
{
  q->cap          = k;
  q->len          = 0;
  q->max_dist_idx = -1;
  q->dists        = dists;
  q->indices      = indices;
}

MSH_HG_INLINE void
msh_hash_grid_dist_storage_push( msh_hash_grid_dist_storage_t* q, 
                                 const float dist, const int32_t idx )
{
  // We have storage left
  if( q->len < q->cap )
  {
    // Push new element
    q->dists[q->len]   = dist;
    q->indices[q->len] = idx;

    // book keep the index of max dist
    if( q->max_dist_idx != -1 )
    {
      if( q->dists[q->max_dist_idx] < dist ) { q->max_dist_idx = q->len; }
    }
    else
    {
      q->max_dist_idx = q->len;
    }
    q->len++;
  }
  // We are at capacity. Only add if new is smaller than maximal dist.
  else
  {
    if( q->dists[q->max_dist_idx] > dist )
    {
      q->dists[q->max_dist_idx]   = dist;
      q->indices[q->max_dist_idx] = idx;

      // Make sure we are really looking at the highest element after replacement
      for( size_t i = 0; i < q->len; ++i )
      {
        if( q->dists[i] > q->dists[q->max_dist_idx] ) { q->max_dist_idx = i; }
      }
    }
  }
}

MSH_HG_INLINE void
msh_hash_grid__find_neighbors_in_bin( const msh_hash_grid_t* hg, const uint64_t bin_idx, 
                                      const float radius_sq, const msh_hg_v3_t* pt, 
                                      msh_hash_grid_dist_storage_t* s )
{
  // issue this whole things stops working if we use doubles.
  uint64_t* bin_table_idx = msh_hg_map_get( hg->bin_table, bin_idx );
  if( !bin_table_idx ) { return; }

  msh_hg__bin_info_t bi = hg->offsets[ *bin_table_idx ];
  int n_pts = bi.length;
  const msh_hg_v3i_t* data = &hg->data_buffer[bi.offset];

  float px = pt->x;
  float py = pt->y;
  float pz = pt->z;

  for( int32_t i = 0; i < n_pts; ++i )
  {
    // TODO(maciej): Maybe SSE?
    float   dix = data[i].x;
    float   diy = data[i].y;
    float   diz = data[i].z;
    int32_t dii = data[i].i;

    float vx = dix - px;
    float vy = diy - py;
    float vz = diz - pz;
    float dist_sq = vx * vx + vy * vy + vz * vz;

    if( dist_sq < radius_sq )
    {
      msh_hash_grid_dist_storage_push( s, dist_sq, dii );
    }
  }
}

int msh_hash_grid_radius_search_mt( const msh_hash_grid_t* hg, 
                                    msh_hash_grid_search_desc_t* hg_sd )
{
  assert( hg_sd->query_pts );
  assert( hg_sd->distances_sq );
  assert( hg_sd->indices );
  assert( hg_sd->radius > 0.0 );
  assert( hg_sd->n_query_pts > 0 );
  assert( hg_sd->max_n_neigh > 0 );

  // Unpack the some useful data from structs
  enum { MAX_BIN_COUNT = 128 };
  int32_t n_query_pts = hg_sd->n_query_pts;
  size_t row_size     = hg_sd->max_n_neigh;
  float radius        = hg_sd->radius;
  uint64_t slab_size  = hg->_slab_size;
  float cs            = hg->cell_size;
  float ics           = hg->_inv_cell_size;
  int64_t w           = hg->width;
  int64_t h           = hg->height;
  int64_t d           = hg->depth;
  float radius_sq     = radius * radius;
  

  int n_threads = 1;
  int n_pts_per_thread = n_query_pts;
#if defined(_OPENMP)
  #pragma omp parallel
  {
    n_threads = omp_get_num_threads();
    n_pts_per_thread = ceilf((float)n_query_pts / n_threads);
    int thread_idx = omp_get_thread_num();
#else
  for( int thread_idx = 0; thread_idx < n_threads; ++thread_idx )
  {
#endif
    int low_lim   = thread_idx * n_pts_per_thread;
    int high_lim  = MSH_HG_MIN((thread_idx + 1) * n_pts_per_thread, n_query_pts);
    int cur_n_pts = high_lim - low_lim;

    msh_hg_v3_t *query_pt = ((msh_hg_v3_t*)hg_sd->query_pts) + low_lim;
    size_t* n_neighbors   = hg_sd->n_neighbors + low_lim;
    float* dists_sq       = hg_sd->distances_sq + (low_lim * row_size);
    int32_t* indices      = hg_sd->indices + (low_lim * row_size);

    int32_t bin_indices[ MAX_BIN_COUNT ];
    float bin_dists_sq[ MAX_BIN_COUNT ];
    msh_hash_grid_dist_storage_t storage;

    for( int32_t pt_idx = 0; pt_idx < cur_n_pts; ++pt_idx )
    {
      // Prep the storage for the next point
      msh_hash_grid_dist_storage_init( &storage, row_size, dists_sq, indices );

      // Normalize query pt with respect to grid
      msh_hg_v3_t q = (msh_hg_v3_t) { query_pt->x - hg->min_pt.x,
                                      query_pt->y - hg->min_pt.y,
                                      query_pt->z - hg->min_pt.z };

      // Get base bin idx for query pt
      int64_t ix = (int64_t)( q.x * ics );
      int64_t iy = (int64_t)( q.y * ics );
      int64_t iz = (int64_t)( q.z * ics );

      // Decide where to look
      int64_t px  = (int64_t)( (q.x + radius) * ics );
      int64_t nx  = (int64_t)( (q.x - radius) * ics );
      int64_t opx = px - ix;
      int64_t onx = nx - ix;

      int64_t py  = (int64_t)( (q.y + radius) * ics );
      int64_t ny  = (int64_t)( (q.y - radius) * ics );
      int64_t opy = py - iy;
      int64_t ony = ny - iy;

      int64_t pz  = (int64_t)( (q.z + radius) * ics );
      int64_t nz  = (int64_t)( (q.z - radius) * ics );
      int64_t opz = pz - iz;
      int64_t onz = nz - iz;

      int n_visited_bins = 0;
      float dx, dy, dz;
      int64_t cx, cy, cz;
      for( int64_t oz = onz; oz <= opz; ++oz )
      {
        cz = (int64_t)iz + oz;
        if( cz < 0 || cz >= d ) { continue; }
        uint64_t idx_z = cz * slab_size;

        if( oz < 0 )      { dz = q.z - (cz + 1) * cs; }
        else if( oz > 0 ) { dz = cz * cs - q.z; }
        else              { dz = 0.0f; }

        for( int64_t oy = ony; oy <= opy; ++oy )
        {
          cy = iy + oy;
          if( cy < 0 || cy >= h ) { continue; }
          uint64_t idx_y = cy * w;

          if( oy < 0 )      { dy = q.y - (cy + 1) * cs; }
          else if( oy > 0 ) { dy = cy * cs - q.y; }
          else              { dy = 0.0f; }

          for( int64_t ox = onx; ox <= opx; ++ox )
          {
            cx = ix + ox;
            if( cx < 0 || cx >= w ) { continue; }

            assert( n_visited_bins < MAX_BIN_COUNT );

            bin_indices[n_visited_bins] = idx_z + idx_y + cx;

            if( ox < 0 )      { dx = q.x - (cx + 1) * cs; }
            else if( ox > 0 ) { dx = cx * cs - q.x; }
            else              { dx = 0.0f; }
            
            bin_dists_sq[n_visited_bins] = dz * dz + dy * dy + dx * dx;
            n_visited_bins++;
          }
        }
      }

      msh_hash_grid__sort( bin_dists_sq, bin_indices, n_visited_bins );

      for( int i = 0; i < n_visited_bins; ++i )
      {
        msh_hash_grid__find_neighbors_in_bin( hg, bin_indices[i], radius_sq, query_pt, &storage );
        if( storage.len >= row_size &&
            dists_sq[ storage.max_dist_idx ] <= bin_dists_sq[i] )
        {
          break;
        }
      }

      if( hg_sd->sort ) { msh_hash_grid__sort( dists_sq, indices, storage.len ); }

      if( n_neighbors ) { (*n_neighbors++) = storage.len; }

      // Advance pointers 
      dists_sq += row_size;
      indices  += row_size;
      query_pt += 1;
    }
  }

  int total_num_neighbors = 0;
  for( int i = 0 ; i < n_query_pts; ++i )
  {
    total_num_neighbors += hg_sd->n_neighbors[i];
  }
  
  return total_num_neighbors;
}


// DESMOS for the derivation of piecewise to continuous approximation : https://www.desmos.com/calculator/adbcuy9ys6
// Approximation snippet (for z dimension): 
    // sgn = 2.0f / ( 1.0f + expf(-k*oz) ) - 1.0f;
    // r = -sgn * ( pt_prime.z - cs * (cz+1) );
    // t =  sgn * ( cs*cz - pt_prime.z);
    // dz = ((r+t) - fabs(r-t)) * 0.5f;
// This snipped ended up being slower than the if statement however
int
msh_hash_grid_radius_search( const msh_hash_grid_t* hg, const float* query_pt, 
                             const float radius, float* dists_sq, int* indices, 
                             size_t max_n_neigh, int sort )
{
  msh_hash_grid_dist_storage_t storage;
  msh_hash_grid_dist_storage_init( &storage, max_n_neigh, dists_sq, indices );

  msh_hg_v3_t* pt      = (msh_hg_v3_t*)query_pt;
  msh_hg_v3_t pt_prime = (msh_hg_v3_t) { pt->x - hg->min_pt.x,
                                         pt->y - hg->min_pt.y,
                                         pt->z - hg->min_pt.z };

  // Get base bin idx for query pt
  int64_t ix = (int64_t)( pt_prime.x * hg->_inv_cell_size );
  int64_t iy = (int64_t)( pt_prime.y * hg->_inv_cell_size );
  int64_t iz = (int64_t)( pt_prime.z * hg->_inv_cell_size );

  // Decide where to look
  int64_t px  = (int64_t)( (pt_prime.x + radius) * hg->_inv_cell_size );
  int64_t nx  = (int64_t)( (pt_prime.x - radius) * hg->_inv_cell_size );
  int64_t opx = px - ix;
  int64_t onx = nx - ix;

  int64_t py  = (int64_t)( (pt_prime.y + radius) * hg->_inv_cell_size );
  int64_t ny  = (int64_t)( (pt_prime.y - radius) * hg->_inv_cell_size );
  int64_t opy = py - iy;
  int64_t ony = ny - iy;

  int64_t pz  = (int64_t)( (pt_prime.z + radius) * hg->_inv_cell_size );
  int64_t nz  = (int64_t)( (pt_prime.z - radius) * hg->_inv_cell_size );
  int64_t opz = pz - iz;
  int64_t onz = nz - iz;

  float radius_sq = radius * radius;
  enum { MAX_BIN_COUNT = 128 };
  int32_t bin_indices[MAX_BIN_COUNT];
  float  bin_dists_sq[MAX_BIN_COUNT];
  int n_visited_bins = 0;

  float dx, dy, dz;
  int64_t cx, cy, cz;
  int64_t w = hg->width, h = hg->height, d = hg->depth;
  uint64_t slab_size = hg->_slab_size;
  float cs = hg->cell_size;

  for( int64_t oz = onz; oz <= opz; ++oz )
  {
    cz = iz + oz;
    if( cz < 0 || cz >= d ) { continue; }
    uint64_t idx_z = cz * slab_size;

    if( oz < 0 )      { dz = pt_prime.z - (cz + 1) * cs; }
    else if( oz > 0 ) { dz = cz * cs - pt_prime.z; }
    else              { dz = 0.0f; }

    for( int64_t oy = ony; oy <= opy; ++oy )
    {
      cy = iy + oy;
      if( cy < 0 || cy >= h ) { continue; }
      uint64_t idx_y = cy * w;

      if( oy < 0 )      { dy = pt_prime.y - (cy + 1) * cs; }
      else if( oy > 0 ) { dy = cy * cs - pt_prime.y; }
      else              { dy = 0.0f; }

      for( int64_t ox = onx; ox <= opx; ++ox )
      {
        cx = ix + ox;
        if( cx < 0 || cx >= w ) { continue; }

        if( ox < 0 )      { dx = pt_prime.x - (cx + 1) * cs; }
        else if( ox > 0 ) { dx = cx * cs - pt_prime.x; }
        else              { dx = 0.0f; }

        assert( n_visited_bins < MAX_BIN_COUNT );

        int32_t idx = idx_z + idx_y + cx;
        float dist_sq = dz * dz + dy * dy + dx * dx;
        bin_indices[n_visited_bins] = idx;
        bin_dists_sq[n_visited_bins] = dist_sq;
        n_visited_bins++;
      }
    }
  }

  // Now we are checking bins in order of closeness to the current point + 
  // we have a minimum closest distance to a point in a bin, so we can early out and not
  // check all the bins.
  msh_hash_grid__sort( bin_dists_sq, bin_indices, n_visited_bins );

  for( int i = 0; i < n_visited_bins; ++i )
  {
    msh_hash_grid__find_neighbors_in_bin( hg, bin_indices[i], radius_sq, pt, &storage );

    if( storage.len >= max_n_neigh &&
        dists_sq[ storage.max_dist_idx ] <= bin_dists_sq[i] )
    {
      break;
    }
  }

  if( sort ) { msh_hash_grid__sort( dists_sq, indices, storage.len ); }
  
  return storage.len;
}


MSH_HG_INLINE void
msh_hash_grid__add_bin_contents( const msh_hash_grid_t* hg, const uint64_t bin_idx,   
                                 const msh_hg_v3_t* pt, msh_hash_grid_dist_storage_t* s )
{
  uint64_t* bin_table_idx = msh_hg_map_get( hg->bin_table, bin_idx );

  if( !bin_table_idx ) { return; }
  
  msh_hg__bin_info_t bi = hg->offsets[*bin_table_idx];
  int n_pts = bi.length;
  const msh_hg_v3i_t* data = &hg->data_buffer[bi.offset];

  for( int32_t i = 0; i < n_pts; ++i )
  {
    msh_hg_v3_t v = { data[i].x - pt->x, data[i].y - pt->y, data[i].z - pt->z };
    float dist_sq = v.x * v.x + v.y * v.y + v.z * v.z;
    msh_hash_grid_dist_storage_push( s, dist_sq, data[i].i );
  }
}

int
msh_hash_grid_knn_search( const msh_hash_grid_t* hg, const float* query_pt, const size_t k,
                          float* dists_sq, int* indices, int sort )
{
  msh_hash_grid_dist_storage_t storage;
  msh_hash_grid_dist_storage_init( &storage, k, dists_sq, indices );

  msh_hg_v3_t* pt = (msh_hg_v3_t*)query_pt;
  msh_hg_v3_t pt_prime = (msh_hg_v3_t) { pt->x - hg->min_pt.x,
                                         pt->y - hg->min_pt.y,
                                         pt->z - hg->min_pt.z };
  // get base bin for query
  uint64_t ix = (uint64_t)( (pt_prime.x) * hg->_inv_cell_size );
  uint64_t iy = (uint64_t)( (pt_prime.y) * hg->_inv_cell_size );
  uint64_t iz = (uint64_t)( (pt_prime.z) * hg->_inv_cell_size );
  
  int32_t layer = 0;
  int8_t should_break = 0;
  
  float dx, dy, dz;
  int64_t cx, cy, cz;
  int64_t w = hg->width;
  int64_t h = hg->height;
  int64_t d = hg->depth;

  uint64_t slab_size = hg->_slab_size;
  float cs = hg->cell_size;

  enum { MAX_BIN_COUNT = 1024 };
  int32_t bin_indices[ MAX_BIN_COUNT ];
  
  while( true )
  {
    int32_t inc_x = 1;
    int n_visited_bins = 0;
    for( int64_t oz = -layer; oz <= layer; oz++ )
    {
      cz = iz + oz;
      if( cz < 0 || cz >= d ) continue;
      uint64_t idx_z = cz * slab_size;

      if( oz < 0 )      { dz = pt_prime.z - (cz + 1) * cs; }
      else if( oz > 0 ) { dz = cz * cs - pt_prime.z; }
      else              { dz = 0.0f; }

      for( int64_t oy = -layer; oy <= layer; oy++ )
      {
        cy = iy + oy;
        if( cy < 0 || cy >= h ) continue;
        uint64_t idx_y = cy * w;

        if( oy < 0 )      { dy = pt_prime.y - (cy + 1) * cs; }
        else if( oy > 0 ) { dy = cy * cs - pt_prime.y; }
        else              { dy = 0.0f; }

        if( abs(oy) != layer && abs(oz) != layer ) { inc_x = 2 * layer; }
        else                                       { inc_x = 1; }

        for( int64_t ox = -layer; ox <= layer; ox += inc_x )
        {
          cx = ix + ox;
          if( cx < 0 || cx >= w ) continue;


          if( ox < 0 )      { dx = pt_prime.x - (cx + 1) * cs; }
          else if( ox > 0 ) { dx = cx * cs - pt_prime.x; }
          else              { dx = 0.0f; }

          float dist_sq = dz * dz + dy * dy + dx * dx;

          if( storage.len >= k &&
              dist_sq > dists_sq[storage.max_dist_idx] ) { continue; }
  
          assert( n_visited_bins < MAX_BIN_COUNT );
  
          bin_indices[ n_visited_bins ] = idx_z + idx_y + cx;
          bin_dists_sq[n_visited_bins] = dist_sq;

          n_visited_bins++;
        }
      }
    }

    for( int i = 0; i < n_visited_bins; ++i )
    {
      msh_hash_grid__add_bin_contents( hg, bin_indices[i], pt, &storage );
    }
    layer++;

    if( should_break ) { break; }
    if( storage.len >= k ) { should_break = true; }
  }


  if( sort ) { msh_hash_grid__sort( dists_sq, indices, storage.len ); }
  return storage.len;
}


int msh_hash_grid_knn_search_mt( const msh_hash_grid_t* hg, 
                                 msh_hash_grid_search_desc_t* hg_sd )
{
  // Unpack the some useful data from structs
  enum { MAX_BIN_COUNT = 128 };
  int32_t n_query_pts = hg_sd->n_query_pts;
  uint32_t k          = hg_sd->k;
  uint64_t slab_size  = hg->_slab_size;
  int8_t sort         = hg_sd->sort;
  float cs            = hg->cell_size;
  int64_t w           = hg->width;
  int64_t h           = hg->height;
  int64_t d           = hg->depth;

  int n_threads = 1;
  int n_pts_per_thread = n_query_pts;

#if defined(_OPENMP)
  #pragma omp parallel
  {
    n_threads = omp_get_num_threads();
    n_pts_per_thread = ceilf((float)n_query_pts / n_threads);
    int thread_idx = omp_get_thread_num();
#else
  for( int thread_idx = 0; thread_idx < n_threads; ++thread_idx )
  {
#endif
    int low_lim   = thread_idx * n_pts_per_thread;
    int high_lim  = MSH_HG_MIN((thread_idx + 1) * n_pts_per_thread, n_query_pts);
    int cur_n_pts = high_lim - low_lim;
    
    msh_hg_v3_t *query_pt = ((msh_hg_v3_t*)hg_sd->query_pts) + low_lim;
    size_t* n_neighbors   = hg_sd->n_neighbors + low_lim;
    float* dists_sq       = hg_sd->distances_sq + (low_lim * k);
    int32_t* indices      = hg_sd->indices + (low_lim * k);

    int32_t bin_indices[ MAX_BIN_COUNT ];
    msh_hash_grid_dist_storage_t storage;

    for( int32_t pt_idx = 0; pt_idx < cur_n_pts; ++pt_idx )
    {
      // Prep the storage for the next point
      msh_hash_grid_dist_storage_init( &storage, k, dists_sq, indices );

      // Normalize query pt with respect to grid
      float dx, dy, dz;
      int64_t cx, cy, cz;
      int32_t layer = 0;
      int8_t should_break = 0;

      msh_hg_v3_t* pt = (msh_hg_v3_t*)query_pt;
      msh_hg_v3_t pt_prime = (msh_hg_v3_t) { pt->x - hg->min_pt.x,
                                             pt->y - hg->min_pt.y,
                                             pt->z - hg->min_pt.z };
      // get base bin for query
      uint64_t ix = (uint64_t)( (pt_prime.x) * hg->_inv_cell_size );
      uint64_t iy = (uint64_t)( (pt_prime.y) * hg->_inv_cell_size );
      uint64_t iz = (uint64_t)( (pt_prime.z) * hg->_inv_cell_size );
    
      while( true )
      {
        int32_t inc_x = 1;
        uint32_t n_visited_bins = 0;
        for( int64_t oz = -layer; oz <= layer; oz++ )
        {
          cz = iz + oz;
          if( cz < 0 || cz >= d ) continue;
          uint64_t idx_z = cz * slab_size;

          if( oz < 0 )      { dz = pt_prime.z - (cz + 1) * cs; }
          else if( oz > 0 ) { dz = cz * cs - pt_prime.z; }
          else              { dz = 0.0f; }

          for( int64_t oy = -layer; oy <= layer; oy++ )
          {
            cy = iy + oy;
            if( cy < 0 || cy >= h ) continue;
            uint64_t idx_y = cy * w;

            if( oy < 0 )      { dy = pt_prime.y - (cy + 1) * cs; }
            else if( oy > 0 ) { dy = cy * cs - pt_prime.y; }
            else              { dy = 0.0f; }

            if( abs(oy) != layer && abs(oz) != layer ) { inc_x = 2 * layer; }
            else                                       { inc_x = 1; }

            for( int64_t ox = -layer; ox <= layer; ox += inc_x )
            {
              cx = ix + ox;
              if( cx < 0 || cx >= w ) continue;


              if( ox < 0 )      { dx = pt_prime.x - (cx + 1) * cs; }
              else if( ox > 0 ) { dx = cx * cs - pt_prime.x; }
              else              { dx = 0.0f; }

              float dist_sq = dz * dz + dy * dy + dx * dx;

              if( storage.len >= k &&
                  dist_sq > dists_sq[storage.max_dist_idx] ) { continue; }
      
              assert( n_visited_bins < MAX_BIN_COUNT );
      
              bin_indices[n_visited_bins]  = idx_z + idx_y + cx;
              bin_dists_sq[n_visited_bins] = dist_sq;

              n_visited_bins++;
            }
          }
        }

        for( int i = 0; i < n_visited_bins; ++i )
        {
          msh_hash_grid__add_bin_contents( hg, bin_indices[i], pt, &storage );
        }

        layer++;

        if( should_break ) { break; }
        if( storage.len >= k ) { should_break = true; }
      }

      if( n_neighbors ) { (*n_neighbors++) = storage.len; }

      if( sort ) { msh_hash_grid__sort( dists_sq, indices, storage.len ); }

      // Advance pointers 
      dists_sq += k;
      indices  += k;
      query_pt += 1;
    }
  }

  int total_num_neighbors = 0;
  for( int i = 0 ; i < n_query_pts; ++i )
  {
    total_num_neighbors += hg_sd->n_neighbors[i];
  }

  return total_num_neighbors;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// msh_array / msh_hg_map implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

void* 
msh_hg__array_grow(const void *array, size_t new_len, size_t elem_size) {
  size_t old_cap = msh_hg_array_cap( array );
  size_t new_cap = (size_t)msh_hg_array__grow_formula( old_cap );
  new_cap = (size_t)MSH_HG_MAX( new_cap, MSH_HG_MAX(new_len, 16) );
  size_t new_size = sizeof(msh_hg_array_hdr_t) + new_cap * elem_size;
  msh_hg_array_hdr_t *new_hdr;

  if( array ) {
    new_hdr = (msh_hg_array_hdr_t*)MSH_HG_REALLOC( msh_hg_array__hdr( array ), new_size );
  } else {
    new_hdr = (msh_hg_array_hdr_t*)MSH_HG_MALLOC( new_size );
    new_hdr->len = 0;
  }
  new_hdr->cap = new_cap;
  return (void*)((char*)new_hdr + sizeof(msh_hg_array_hdr_t));
}

MSH_HG_INLINE uint64_t 
msh_hg_hash_uint64(uint64_t x) 
{
  x *= 0xff51afd7ed558ccd;
  x ^= x >> 32;
  return x;
}


size_t 
msh_hg_map__pow2ceil( uint32_t v )
{
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  ++v;
  v += ( v == 0 );
  return v;
}

void 
msh_hg_map_init( msh_hg_map_t* map, uint32_t cap )
{
  assert( !map->keys && !map->vals );
  cap = msh_hg_map__pow2ceil( cap );
  map->keys = (uint64_t*)MSH_HG_CALLOC( cap, sizeof(uint64_t) );
  map->vals = (uint64_t*)MSH_HG_MALLOC( cap * sizeof(uint64_t) );
  map->_len = 0;
  map->_cap = cap;
}

void 
msh_hg_map__grow( msh_hg_map_t *map, size_t new_cap) {
  new_cap = msh_max( new_cap, 16 );
  msh_hg_map_t new_map;
  new_map.keys = (uint64_t*)MSH_HG_CALLOC( new_cap, sizeof(uint64_t) );
  new_map.vals = (uint64_t*)MSH_HG_MALLOC( new_cap * sizeof(uint64_t) );
  new_map._len = 0;
  new_map._cap = new_cap;

  for( size_t i = 0; i < map->_cap; i++ ) 
  {
    if( map->keys[i] ) 
    {
      msh_hg_map_insert( &new_map, map->keys[i] - 1, map->vals[i] );
    }
  }
  MSH_HG_FREE( (void *)map->keys );
  MSH_HG_FREE( map->vals );
  *map = new_map;
}

size_t 
msh_hg_map_len( msh_hg_map_t* map )
{
  return map->_len;
}

size_t 
msh_hg_map_cap( msh_hg_map_t* map )
{
  return map->_cap;
}

void 
msh_hg_map_insert( msh_hg_map_t* map, uint64_t key, uint64_t val )
{
  key += 1;
  if( 2 * map->_len >= map->_cap) { msh_hg_map__grow( map, 2 * map->_cap ); }
  assert( 2 * map->_len < map->_cap );
  size_t i = (size_t)key;
  for (;;) 
  {
    i &= map->_cap - 1;
    if( !map->keys[i] )
    {
      map->_len++;
      map->keys[i] = key;
      map->vals[i] = val;
      return;
    } 
    else if( map->keys[i] == key )
    {
      map->vals[i] = val;
      return;
    }
    i++;
  }
}

uint64_t* 
msh_hg_map_get( const msh_hg_map_t* map, uint64_t key )
{
  if (map->_len == 0) { return NULL; }
  key += 1;
  size_t i = (size_t)key;
  assert(map->_len < map->_cap);
  for (;;) {
    i &= map->_cap - 1;
    if( map->keys[i] == key )
    {
      return &map->vals[i];
    } 
    else if( !map->keys[i] ) 
    {
      return NULL;
    }
    i++;
  }
}

void 
msh_hg_map_free( msh_hg_map_t* map )
{
  MSH_HG_FREE( map->keys );
  MSH_HG_FREE( map->vals );
  map->_cap = 0;
  map->_len = 0;
}

#endif /* MSH_HASH_GRID_IMPLEMENTATION */

// if( nk_button_label( nk_ctx, "Colorize" ) )
          // {
          //   msh_hash_grid_t* hg = msh_hash_grid_create( (float*)input_scenes[TIME_IDX]->positions[0], 
          //                                               input_scenes[TIME_IDX]->n_pts[0], 0.3 );
          //   int bin_idx = -1;
          //   uint64_t* bin_table_idx = NULL;
          //   while( bin_table_idx == NULL )
          //   {
          //     bin_idx++;
          //     bin_table_idx = msh_hg_map_get( hg->bin_table, (bin_idx + 1));
          //   }
          //   printf("%d %d\n", bin_idx, (int)(*bin_table_idx));
          //   msh_hg__bin_info_t bi = hg->offsets[ *bin_table_idx ];
          //   int n_pts = bi.length;
          //   const msh_hg_v3i_t* data = &hg->data_buffer[bi.offset];
          //   printf(" || %d %d\n", bi.length, bi.offset );
          //   for( int i = 0; i < n_pts; ++i )
          //   {
          //     input_scenes[TIME_IDX]->colors[0][data[i].i] = msh_vec3(1.0f, 0.0f, 0.0f);
          //   }
          //   bin_idx++;
          //   bin_table_idx = msh_hg_map_get( hg->bin_table, (bin_idx + 1));
          //   while( bin_table_idx == NULL )
          //   {
          //     bin_idx++;
          //     bin_table_idx = msh_hg_map_get( hg->bin_table, (bin_idx + 1));
          //   }
          //   printf("%d %d\n", bin_idx, (int)(*bin_table_idx));
          //   bi = hg->offsets[ *bin_table_idx ];
          //   n_pts = bi.length;
          //   data = &hg->data_buffer[bi.offset];
          //   printf(" || %d %d\n", bi.length, bi.offset );
          //   for( int i = 0; i < n_pts; ++i )
          //   {
          //     input_scenes[TIME_IDX]->colors[0][data[i].i] = msh_vec3(0.0f, 1.0f, 0.0f);
          //   }
          //   bin_idx++;
          //   bin_table_idx = msh_hg_map_get( hg->bin_table, (bin_idx + 1));
          //   while( bin_table_idx == NULL )
          //   {
          //     bin_idx++;
          //     bin_table_idx = msh_hg_map_get( hg->bin_table, (bin_idx + 1));
          //   }
          //   printf("%d %d\n", bin_idx, (int)(*bin_table_idx));
          //   bi = hg->offsets[ *bin_table_idx ];
          //   n_pts = bi.length;
          //   data = &hg->data_buffer[bi.offset];
          //   printf(" || %d %d\n", bi.length, bi.offset );
          //   for( int i = 0; i < n_pts; ++i )
          //   {
          //     input_scenes[TIME_IDX]->colors[0][data[i].i] = msh_vec3(0.0f, 0.0f, 1.0f);
          //   }

          //   mshgfx_geometry_free( &pointclouds[0][n_objects+TIME_IDX] );
          //   pointclouds[0][n_objects+TIME_IDX] = convert_rs_pointcloud_to_gpu( input_scenes[TIME_IDX], 0 );

          // }