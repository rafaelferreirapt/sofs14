1 #alloc inode for a directory
1
3 #alloc data cluster (it is going to fail - inode number 100 is out of range)
100
3 #free data cluster (it is going to fail - inode number 15 is free)
15
3 #alloc data cluster
1
3 #alloc data cluster
1
3 #alloc data cluster
1
3 #alloc data cluster
1
4 #free data cluster (it is going to fail - data cluster number 100 is out of range)
100
4 #free data cluster
4
4 #free data cluster (it is going to fail - data cluster number 4 is already free)
4
4 #free data cluster
3
4 #free data cluster
2
4 #free data cluster
1
4 #free data cluster (it is going to fail - data cluster number 0 can not be freed)
0
0
