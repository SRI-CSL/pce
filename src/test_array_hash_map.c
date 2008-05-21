#include "array_hash_map.h"
#include "memalloc.h"
#include <stddef.h>
#include <stdio.h>

int main(){

  int32_t *k3 = (int32_t *) safe_malloc(3*sizeof(int32_t));
  int32_t *k4 = (int32_t *) safe_malloc(4*sizeof(int32_t));
  int32_t *k5 = (int32_t *) safe_malloc(4*sizeof(int32_t));

  array_hmap_t *hmap = (array_hmap_t *) safe_malloc(sizeof(array_hmap_t));

  init_array_hmap(hmap, 0);

  array_hmap_pair_t *hmap_rec; 

  int32_t i, j, m, n;
  n = 0;
  for (i = -5; i < 5; i++){
    for (j = -5; j < 5; j++){
      for (m = -5; m < 5; m++){
	k3[0] = i;
	k3[1] = j;
	k3[2] = m;
	hmap_rec = array_size_hmap_find(hmap, 3, k3);
	if (hmap_rec != NULL){
	  printf("\nfind fails");
	  return 1;
	}
	hmap_rec = array_size_hmap_get(hmap, 3, k3);
	hmap_rec->val = n++;
      }
    }
  }
  for (i = -5; i < 5; i++){
    for (j = -5; j < 5; j++){
      for (m = -5; m < 5; m++){
	k3[0] = i;
	k3[1] = j;
	k3[2] = m;
	hmap_rec = array_size_hmap_find(hmap, 3, k3);
	if (hmap_rec != NULL){
	  printf("\n [i: %d, j: %d, m: %d] |-> %d", i, j, m, hmap_rec->val);
	}
      }
    }
  }

  return 1;
}
