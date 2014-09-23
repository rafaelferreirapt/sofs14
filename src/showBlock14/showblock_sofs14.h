/**
 *  \file showblock_sofs14.h (interface file)
 *
 *  \brief The SOFS14 block/cluster display tool.
 *
 *  It displays preselected blocks/clusters of the storage device, supposed to contain file system data or metadata.
 *  With it, the storage device is envisaged as an array of blocks, some of them joined in clusters.
 *
 *  The following display formats are available:
 *      \li block/cluster contents as hexadecimal data
 *      \li block/cluster contents as ascii/hexadecimal data
 *      \li block/cluster contents both as hexadecimal and ascii data
 *      \li block contents as superblock data
 *      \li block contents as a sub-array of inode entries
 *      \li cluster contents as a byte stream
 *      \li cluster contents as a sub-array of directory entries
 *      \li cluster contents as a sub-array of data cluster references.
 *
 *  SINOPSIS:
 *  <P><PRE>                   showblock_sofs14 OPTIONS supp-file
 *
 *                OPTIONS:
 *                 -x blockNumber   --- show the block contents as hexadecimal data
 *                 -X clusterNumber --- show the cluster contents as hexadecimal data
 *                 -b blockNumber   --- show the block contents as ascii/hexadecimal data
 *                 -B clusterNumber --- show the cluster contents as ascii/hexadecimal data
 *                 -a blockNumber   --- show the block contents both as hexadecimal and ascii data
 *                 -A clusterNumber --- show the cluster contents both as hexadecimal and ascii data
 *                 -s blockNumber   --- show the block contents as superblock data
 *                 -i blockNumber   --- show the block contents as a sub-array of inode entries
 *                 -T clusterNumber --- show the cluster contents as a byte stream
 *                 -D clusterNumber --- show the cluster contents as a sub-array of directory entries
 *                 -R clusterNumber --- show the cluster contents as a sub-array of data cluster references
 *                 -h               --- print this help.</PRE>
 *
 *  \remarks All cluster and block numbers in OPTIONS are physical numbers (indexes of the array of blocks that
 *           represent the storage device at this abstraction level). Furthermore, a cluster number is the number of
 *           the first block that comprises it.
 *
 *  \author João Silva - January 2006
 *  \author Artur Carneiro Pereira - September 2006
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - August 2010 - August 2011, September 2014
 */
