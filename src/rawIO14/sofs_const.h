/**
 *  \file sofs_const.h (interface file)
 *
 *  \brief Definition of basic constants.
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - July 2010 / August 2011
 */

#ifndef SOFS_CONST_H_
#define SOFS_CONST_H_

/** \brief block size (in bytes) */
#define BLOCK_SIZE (512)

/** \brief block size (in bits) */
#define BITS_PER_BLOCK (8 * BLOCK_SIZE)

/** \brief number of contiguous blocks in a cluster */
#define BLOCKS_PER_CLUSTER (4)

/** \brief cluster size (in bytes) */
#define CLUSTER_SIZE (BLOCKS_PER_CLUSTER * BLOCK_SIZE)

#endif /* SOFS_CONST_H_ */
