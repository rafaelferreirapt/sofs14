1 #alloc inode for a directory
1
1 #alloc inode for a regular file
2
3 #alloc data cluster
1
3 #alloc data cluster
2
3 #alloc data cluster
1
3 #alloc data cluster (it is going to fail - there are no more free data clusters)
1
2 #free inode
1
4 #free data cluster
3
4 #free data cluster
2
4 #free data cluster
1
3 #alloc data cluster (it fails if clean inode is operational)
2
0
