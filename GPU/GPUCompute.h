
// CUDA Kernel main function

// -----------------------------------------------------------------------------------------

__device__ void ComputeKangaroos(uint64_t *kangaroos,uint32_t maxFound,uint32_t *out,uint64_t *dpMask) {

  uint64_t px[GPU_GRP_SIZE][4];
  uint64_t py[GPU_GRP_SIZE][4];
  uint64_t dist[GPU_GRP_SIZE][4];
#ifdef USE_SYMMETRY
  uint64_t lastJump[GPU_GRP_SIZE];
#endif

  uint64_t dx[GPU_GRP_SIZE][4];
  uint64_t dy[4];
  uint64_t rx[4];
  uint64_t ry[4];
  uint64_t _s[4];
  uint64_t _p[4];
  uint64_t dpmask0, dpmask1, dpmask2, dpmask3;
  uint32_t jmp;

#ifdef USE_SYMMETRY
  LoadKangaroos(kangaroos,px,py,dist,lastJump);
#else
  LoadKangaroos(kangaroos,px,py,dist);
#endif

  dpmask0 = dpMask[0];
  dpmask1 = dpMask[1];
  dpmask2 = dpMask[2];
  dpmask3 = dpMask[3];

  for(int run = 0; run < NB_RUN; run++) {

    // P1 = jumpPoint
    // P2 = kangaroo

    __syncthreads();

    for(int g = 0; g < GPU_GRP_SIZE; g++) {
      jmp = (uint32_t)px[g][0] & (NB_JUMP-1);

#ifdef USE_SYMMETRY
      if(jmp==lastJump[g]) jmp = (lastJump[g] + 1) % NB_JUMP;
      lastJump[g] = jmp;
#endif

      ModSub256(dx[g],px[g],jPx[jmp]);
    }

    _ModInvGrouped(dx);

    __syncthreads();

    for(int g = 0; g < GPU_GRP_SIZE; g++) {

#ifdef USE_SYMMETRY
      jmp = lastJump[g];
#else
      jmp = (uint32_t)px[g][0] & (NB_JUMP-1);
#endif

      ModSub256(dy,py[g],jPy[jmp]);
      _ModMult(_s,dy,dx[g]);
      _ModSqr(_p,_s);

      ModSub256(rx,_p,jPx[jmp]);
      ModSub256(rx,px[g]);

      ModSub256(ry,px[g],rx);
      _ModMult(ry,_s);
      ModSub256(ry,py[g]);

      Load256(px[g],rx);
      Load256(py[g],ry);

      Add256(dist[g],jD[jmp]);

#ifdef USE_SYMMETRY
      if(ModPositive256(py[g]))
        ModNeg256Order(dist[g]);
#endif
      uint64_t *pxg = px[g];
      if((pxg[0] & dpmask0) == 0 && (pxg[1] & dpmask1) == 0 && (pxg[2] & dpmask2) == 0 && (pxg[3] & dpmask3) == 0) {

        // Distinguished point
        uint32_t pos = atomicAdd(out,1);
        if(pos < maxFound) {
          uint64_t kIdx = (uint64_t)IDX + (uint64_t)g*(uint64_t)blockDim.x + (uint64_t)blockIdx.x*((uint64_t)blockDim.x*GPU_GRP_SIZE);
          OutputDP(px[g],dist[g],&kIdx);
        }

      }

    }

  }

#ifdef USE_SYMMETRY
  StoreKangaroos(kangaroos,px,py,dist,lastJump);
#else
  StoreKangaroos(kangaroos,px,py,dist);
#endif

}
