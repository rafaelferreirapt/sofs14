/**
 *  \file soHandleFileCluster.c (implementation file)
 *
 *  \author Rafael Ferreira e Rodrigo Cunha
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#include "sofs_ifuncs_2.h"
#include "sofs_ifuncs_3.h"

/** \brief operation get the logical number of the referenced data cluster for an inode in use */
#define GET         0
/** \brief operation allocate a new data cluster and associate it to the inode which describes the file */
#define ALLOC       1
/** \brief operation free the referenced data cluster */
#define FREE        2
/** \brief operation free the referenced data cluster and dissociate it from the inode which describes the file */
#define FREE_CLEAN  3
/** \brief operation dissociate the referenced data cluster from the inode which describes the file */
#define CLEAN       4

/* allusion to internal functions */

int soHandleDirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
        uint32_t *p_outVal);
int soHandleSIndirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
        uint32_t *p_outVal);
int soHandleDIndirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
        uint32_t *p_outVal);
int soAttachLogicalCluster(SOSuperBlock *p_sb, uint32_t nInode, uint32_t clustInd, uint32_t nLClust);
int soCleanLogicalCluster(SOSuperBlock *p_sb, uint32_t nInode, uint32_t nLClust);

/**
 *  \brief Handle of a file data cluster.
 *
 *  The file (a regular file, a directory or a symlink) is described by the inode it is associated to.
 *
 *  Several operations are available and can be applied to the file data cluster whose logical number is given.
 *
 *  The list of valid operations is
 *
 *    \li GET:        get the logical number of the referenced data cluster for an inode in use
 *    \li ALLOC:      allocate a new data cluster and associate it to the inode which describes the file
 *    \li FREE:       free the referenced data cluster
 *    \li FREE_CLEAN: free the referenced data cluster and dissociate it from the inode which describes the file
 *    \li CLEAN:      dissociate the referenced data cluster from the inode which describes the file.
 *
 *  Depending on the operation, the field <em>clucount</em> and the lists of direct references, single indirect
 *  references and double indirect references to data clusters of the inode associated to the file are updated.
 *
 *  Thus, the inode must be in use and belong to one of the legal file types for the operations GET, ALLOC, FREE and
 *  FREE_CLEAN and must be free in the dirty state for the operation CLEAN.
 *
 *  \param nInode number of the inode associated to the file
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> or the <em>index to the list of direct references</em> are out of
 *                      range or the requested operation is invalid or the <em>pointer to outVal</em> is \c NULL when it
 *                      should not be (GET / ALLOC)
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (ALLOC / FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleFileCluster(uint32_t nInode, uint32_t clustInd, uint32_t op, uint32_t *p_outVal) {
    soColorProbe(413, "07;31", "soHandleFileCluster (%"PRIu32", %"PRIu32", %"PRIu32", %p)\n",
            nInode, clustInd, op, p_outVal);

    int stat; // function return stat control
    SOSuperBlock *p_sb; // pointer to the superblock
    SOInode inode; // inode to fill with soReadInode

    if ((stat = soLoadSuperBlock())){
        return stat;
    }

    p_sb = soGetSuperBlock();

    /* START OF VALIDATION */

    /* if nInode is out of range */
    if (nInode >= p_sb->iTotal){
        return -EINVAL;
    }

    /* index (clustInd) to the list of direct references are out of range */
    if (clustInd >= MAX_FILE_CLUSTERS){
        return -EINVAL;
    }

    /* requested operation is invalid */
    if (op > 4){
        return -EINVAL;
    }

    /* the pointer p_outVal is NULL when it should not be (GET / ALLOC) */
    if ((op == GET || op == ALLOC) && p_outVal == NULL){
        return -EINVAL;
    }

    if (op == CLEAN) {
        // if operation is CLEAN, readInode as Free in Dirty State
        if ((stat = soReadInode(&inode, nInode, FDIN)) != 0){
            return stat;
        }
    } else {
        // for all other operations read inode as inode in use
        if ((stat = soReadInode(&inode, nInode, IUIN)) != 0){
            return stat;
        }
    }

    /* END OF VALIDATION */

    if (clustInd < N_DIRECT) {
        // flow trough direct references
        if ((stat = soHandleDirect(p_sb, nInode, &inode, clustInd, op, p_outVal)) != 0){
            return stat;
        }
    } else if (clustInd < N_DIRECT + RPC) {
        // flow trough single indirect references
        if ((stat = soHandleSIndirect(p_sb, nInode, &inode, clustInd, op, p_outVal)) != 0){
            return stat;
        }
    } else {
        // flow trough double indirect references
        if ((stat = soHandleDIndirect(p_sb, nInode, &inode, clustInd, op, p_outVal)) != 0){
            return stat;
        }
    }

    if (op == CLEAN) {
        if ((stat = soWriteInode(&inode, nInode, FDIN)) != 0){
            return stat;
        }
    } else if(op != GET){
        if ((stat = soWriteInode(&inode, nInode, IUIN)) != 0){
            return stat;
        }
    }

    return 0;
}

/**
 *  \brief Handle of a file data cluster which belongs to the direct references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB(clustInd - N_DIRECT - RPC) % RPC, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleDirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
        uint32_t *p_outVal) {

    uint32_t NLClt; // logical number of the cluster
    int stat; // function return status control

    NLClt = p_inode->d[clustInd]; // NLClt: cluster logic number

    /* requested operation is invalid */
    if (op > 4){
        return -EINVAL;   
    }
    
    switch (op) {
        case GET:
        {
            *p_outVal = NLClt; // can return NULL_CLUSTER 
            return 0;
        }
        case ALLOC:
        {
            if (NLClt != NULL_CLUSTER){
                return -EDCARDYIL;
            }

            if ((stat = soAllocDataCluster(nInode, &NLClt)) != 0){
                return stat;
            }
                
            if((stat = soAttachLogicalCluster(p_sb,nInode, clustInd, NLClt)) != 0){
                return stat;
            }

            p_inode->d[clustInd] = *p_outVal = NLClt;
            p_inode->cluCount += 1; // number of data clusters attached to the file

            return 0;
        }
        case FREE:

        case FREE_CLEAN:

        case CLEAN:
        {
             p_outVal = NULL;
             
            if (NLClt == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if (op != CLEAN) {
                if ((stat = soFreeDataCluster(NLClt)) != 0){
                    return stat;    
                }
                if (op == FREE){
                    return 0;
                }
            }
            if ((stat = soCleanLogicalCluster(p_sb, nInode, NLClt)) != 0){
                return stat;
            }

            p_inode->cluCount -= 1;
            p_inode->d[clustInd] = NULL_CLUSTER;
            return 0;
        }
        default: return -EINVAL;
    }
    return -EINVAL;
}

/**
 *  \brief Handle of a file data cluster which belongs to the single indirect references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleSIndirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
        uint32_t *p_outVal) {

    uint32_t ref_offset; // reference position
    int stat; // function return status control
    SODataClust *p_dc; // pointer to a data cluster
    uint32_t nclust; // number of the allocated data cluster called in soAllocDataCluster

    if (op > 4){
        return -EINVAL;
    }

    // calculate the offset for the single indirect reference
    //ref_offset = (clustInd - N_DIRECT) % RPC;
    ref_offset = clustInd - N_DIRECT;
 
    switch (op) {
        case GET:
        {
            if (p_inode->i1 == NULL_CLUSTER) {
                *p_outVal = NULL_CLUSTER;
            } else {
                if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER)) != 0){
                    return stat;
                }

                p_dc = soGetDirRefClust();

                *p_outVal = p_dc->info.ref[ref_offset];
            }
            return 0;
        }
        case ALLOC:
        {
            if (p_inode->i1 == NULL_CLUSTER) {
                if ((stat = soAllocDataCluster(nInode, &nclust)) != 0){
                    return stat;
                }

                p_inode->i1 = nclust;
                p_inode->cluCount++;

                if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER)) != 0){
                    return stat;
                }
                
                p_dc = soGetDirRefClust();

                uint32_t i; // reference position 
                for (i = 0; i < RPC; i++){
                    p_dc->info.ref[i] = NULL_CLUSTER;
                }

                if ((stat = soStoreDirRefClust()) != 0){
                    return stat;
                }
            }
            if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dc = soGetDirRefClust();

            if (p_dc->info.ref[ref_offset] != NULL_CLUSTER){
                return -EDCARDYIL;
            }
            
            if ((stat = soAllocDataCluster(nInode, &nclust)) != 0){
                return stat;
            }
            
            if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dc = soGetDirRefClust();

            p_dc->info.ref[ref_offset] = *p_outVal = nclust;
            p_inode->cluCount++;

            if ((stat = soAttachLogicalCluster(p_sb, nInode, clustInd, p_dc->info.ref[ref_offset])) != 0){
                return stat;
            }

            if ((stat = soStoreDirRefClust()) != 0){
                return stat;
            }
            
            return 0;
        }
        case FREE:
        case FREE_CLEAN:
        case CLEAN:
        {
            p_outVal = NULL;

            if (p_inode->i1 == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dc = soGetDirRefClust();

            if (p_dc->info.ref[ref_offset] == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if (p_dc->stat != nInode){
                return -EWGINODENB;
            }

            if (op != CLEAN) {
                if ((stat = soFreeDataCluster(p_dc->info.ref[ref_offset])) != 0){
                    return stat;
                }
                if (op == FREE){
                    return 0;
                }
            }

            if ((stat = soCleanLogicalCluster(p_sb, nInode, p_dc->info.ref[ref_offset])) != 0){
                return stat;
            }

            p_dc->info.ref[ref_offset] = NULL_CLUSTER;
            p_inode->cluCount--;

            
            uint32_t clusterref_pos;
            for (clusterref_pos = 0; clusterref_pos < RPC; clusterref_pos++) {
                if (p_dc->info.ref[clusterref_pos] != NULL_CLUSTER) break;
            }

            if (clusterref_pos == RPC) {
                if ((stat = soStoreDirRefClust())){
                    return stat;
                }
                
                if((stat = soFreeDataCluster(p_inode->i1))){
                    return stat;
                }
                
                if ((stat = soCleanLogicalCluster(p_sb, nInode, p_inode->i1))){
                    return stat;
                }
                
                p_inode->cluCount--;
                p_inode->i1 = NULL_CLUSTER;
                
                return 0;
            }
            if ((stat = soStoreDirRefClust()) != 0){
                return stat;
            }
            
            return 0;
        }
        default:
        {
            p_outVal = NULL;
            return -EINVAL;
        }
    }

    return -EINVAL;
}

/**
 *  \brief Handle of a file data cluster which belongs to the double indirect references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleDIndirect(SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
        uint32_t *p_outVal) {

    uint32_t ref_Soffset, ref_Doffset; // reference positions
    int stat; // function return status control
    SODataClust *p_dcS, *p_dcD; // pointer to single and double reference data cluster
    uint32_t nclust; // pointer no cluster number

    if (op > 4){
        return -EINVAL;
    }

    // calculate single indirect and double indirect reference position
    ref_Soffset = (clustInd - N_DIRECT - RPC) / RPC;
    ref_Doffset = (clustInd - N_DIRECT - RPC) % RPC;

    switch (op) {
        case GET:
        {
            if (p_inode->i2 == NULL_CLUSTER) {
                *p_outVal = NULL_CLUSTER;
            } else {
                if ((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
                    return stat;
                }

                p_dcS = soGetSngIndRefClust();

                if (p_dcS->info.ref[ref_Soffset] == NULL_CLUSTER) {
                    *p_outVal = NULL_CLUSTER;
                } else {
                    if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_dcS->info.ref[ref_Soffset] * BLOCKS_PER_CLUSTER)) != 0){
                        return stat;
                    }

                    p_dcD = soGetDirRefClust();

                    *p_outVal = p_dcD->info.ref[ref_Doffset];
                }
            }
            return 0;
        }
        case ALLOC:
        {
            if (p_inode->i2 == NULL_CLUSTER) {
                if ((stat = soAllocDataCluster(nInode, &nclust)) != 0){
                    return stat;
                }

                p_inode->i2 = nclust;
                p_inode->cluCount++;

                if ((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
                    return stat;
                }

                p_dcS = soGetSngIndRefClust();

                uint32_t i; // reference position counter

                for (i = 0; i < RPC; i++){
                    p_dcS->info.ref[i] = NULL_CLUSTER;
                }

                if ((stat = soStoreSngIndRefClust()) != 0){
                    return stat;
                }
            }
            if ((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dcS = soGetSngIndRefClust();

            if (p_dcS->info.ref[ref_Soffset] == NULL_CLUSTER) {
                if ((stat = soAllocDataCluster(nInode, &nclust)) != 0){
                    return stat;
                }

                p_dcS->info.ref[ref_Soffset] = nclust;
                p_inode->cluCount++;

                if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_dcS->info.ref[ref_Soffset] * BLOCKS_PER_CLUSTER)) != 0){
                    return stat;
                }

                p_dcD = soGetDirRefClust();

                uint32_t i; // reference position counter

                for (i = 0; i < RPC; i++){
                    p_dcD->info.ref[i] = NULL_CLUSTER;
                }
            }
            
            if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_dcS->info.ref[ref_Soffset] * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dcD = soGetDirRefClust();

            if (p_dcD->info.ref[ref_Doffset] != NULL_CLUSTER){
                return -EDCARDYIL;
            }

            if ((stat = soAllocDataCluster(nInode, &nclust)) != 0){
                return stat;
            }

            p_dcD->info.ref[ref_Doffset] = *p_outVal = nclust;
            p_inode->cluCount++;
            
            if ((stat = soAttachLogicalCluster(p_sb, nInode, clustInd, p_dcD->info.ref[ref_Doffset])) != 0){
                return stat;
            }

            if ((stat = soStoreDirRefClust()) != 0){
                return stat;
            }

            if ((stat = soStoreSngIndRefClust()) != 0){
                return stat;
            }

            return 0;
        }
        case FREE:
        case FREE_CLEAN:
        case CLEAN:
        {
            if (p_inode->i2 == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if ((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dcS = soGetSngIndRefClust();

            if (p_dcS->info.ref[ref_Soffset] == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if ((stat = soLoadDirRefClust(p_sb->dZoneStart + p_dcS->info.ref[ref_Soffset] * BLOCKS_PER_CLUSTER)) != 0){
                return stat;
            }

            p_dcD = soGetDirRefClust();

            if (p_dcD->info.ref[ref_Doffset] == NULL_CLUSTER){
                return -EDCNOTIL;
            }

            if (op != CLEAN) {
                if ((stat = soFreeDataCluster(p_dcD->info.ref[ref_Doffset]) != 0)){
                    return stat;
                }
                if (op == FREE){
                    return 0;
                }
            }
            if ((stat = soCleanLogicalCluster(p_sb, nInode, p_dcD->info.ref[ref_Doffset])) != 0){
                return stat;            
            }
            
            p_dcD->info.ref[ref_Doffset] = NULL_CLUSTER;
            p_inode->cluCount--;

            uint32_t clusterref_pos;

            for (clusterref_pos = 0; clusterref_pos < RPC; clusterref_pos++) {
                if (p_dcD->info.ref[clusterref_pos] != NULL_CLUSTER){
                    break;
                }
            }
            
            if ((stat = soStoreDirRefClust()) != 0){
                return stat;
            }

            if (clusterref_pos == RPC) {
                if ((stat = soFreeDataCluster(p_dcS->info.ref[ref_Soffset])) != 0){
                    return stat;
                }
                if ((stat = soCleanLogicalCluster(p_sb, nInode, p_dcS->info.ref[ref_Soffset]) != 0)){
                    return stat;
                }
                p_dcS->info.ref[ref_Soffset] = NULL_CLUSTER;
                p_inode->cluCount--;
                
                if ((stat = soStoreSngIndRefClust()) != 0){
                    return stat;
                }
                

                for (clusterref_pos = 0; clusterref_pos < RPC; clusterref_pos++) {
                    if (p_dcS->info.ref[clusterref_pos] != NULL_CLUSTER){
                        break;
                    }
                }

                if (clusterref_pos == RPC) {
                    if ((stat = soFreeDataCluster(p_inode->i2)) != 0){
                        return stat;
                    }
                    if ((stat = soCleanLogicalCluster(p_sb, nInode, p_inode->i2)) != 0){
                        return stat;
                    }
                    p_inode->i2 = NULL_CLUSTER;
                    p_inode->cluCount--;
                    return 0;
                }
                return 0;
            }

            if ((stat = soStoreSngIndRefClust()) != 0){
                return stat;
            }

            return 0;
        }
        default:
        {
            p_outVal = NULL;
            return -EINVAL;
        }
    }
    return 0;
}

/**
 *  \brief Attach a file data cluster whose index to the list of direct references and logical number are known.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param nLClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAttachLogicalCluster(SOSuperBlock *p_sb, uint32_t nInode, uint32_t clustInd, uint32_t nLClust) {

    int stat; // function return control
    uint32_t idx_prev, idx_next; // logical cluster number of adjacent clusters
    SODataClust dc, dc_prev, dc_next; // data cluster to be updated


    /* temos de verificar se o prev está bem ligado e se o next tambem esta bem ligado */
    if(clustInd == 0){
        idx_prev = NULL_CLUSTER;
    }else{
        if((stat = soHandleFileCluster(nInode, clustInd-1, GET, &idx_prev))){
            return 0;
        }

        if(idx_prev != NULL_CLUSTER){
            if((stat = soReadCacheCluster(p_sb->dZoneStart+idx_prev*BLOCKS_PER_CLUSTER, &dc_prev))){
                return stat;
            }
        }
    }

    if(clustInd == MAX_FILE_CLUSTERS){
        idx_next = NULL_CLUSTER;
    }else{
        if((stat = soHandleFileCluster(nInode, clustInd+1, GET, &idx_next))){
            return 0;
        }

        if(idx_next != NULL_CLUSTER){
            if((stat = soReadCacheCluster(p_sb->dZoneStart+idx_next*BLOCKS_PER_CLUSTER, &dc_next))){
                return stat;
            }
        }
    }

    if((idx_prev != NULL_CLUSTER || idx_next != NULL_CLUSTER)){
        if((stat = soReadCacheCluster(p_sb->dZoneStart + nLClust * BLOCKS_PER_CLUSTER, &dc))){
            return stat;
        }
        if(idx_prev != NULL_CLUSTER){
            dc.prev = idx_prev;
            dc_prev.next = nLClust;
            if((stat = soWriteCacheCluster(p_sb->dZoneStart+idx_prev*BLOCKS_PER_CLUSTER, &dc_prev))){
                return stat;
            }
        }
        if(idx_next != NULL_CLUSTER){
            dc.next = idx_next;
            dc_next.prev = nLClust;
            if((stat = soWriteCacheCluster(p_sb->dZoneStart+idx_next*BLOCKS_PER_CLUSTER, &dc_next))){
                return stat;
            }
        }
        if((stat = soWriteCacheCluster(p_sb->dZoneStart + nLClust*BLOCKS_PER_CLUSTER, &dc))){
            return stat;
        }
    }
    return 0;
}

/**
 *  \brief Clean a file data cluster whose logical number is known.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param nLClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soCleanLogicalCluster(SOSuperBlock *p_sb, uint32_t nInode, uint32_t nLClust) {

    int stat; // function return status control 
    SODataClust dc; // data cluster to be retrieved, modified and saved

    // read the data cluster, converting it's logical number to physical number
    if ((stat = soReadCacheCluster(p_sb->dZoneStart + nLClust * BLOCKS_PER_CLUSTER, &dc)) != 0){
        return stat;
    }

    // test if the given data cluster belongs to the right inode
    if (dc.stat != nInode){
        return -EWGINODENB;
    }

    //mark as clean
    dc.stat = NULL_INODE;
    
    // save the data cluster
    if ((stat = soWriteCacheCluster(p_sb->dZoneStart + nLClust * BLOCKS_PER_CLUSTER, &dc)) != 0){
        return stat;
    }
    
    return 0;
}
