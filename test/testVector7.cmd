1 #alloc inode
2
6 #write inode
1 0 555
5 #read inode
1 0
11 #handle file cluster    (it is going to fail - illegal operation)
1 6 5
11 #handle file cluster    (it is going to fail - inode is free)
10 0 1
11 #handle file cluster    (it is going to fail - inode number out of range)
100 0 1
11 #handle file cluster
1 0 1
11 #handle file cluster
1 6 1
11 #handle file cluster    (it is going to fail - a data cluster was already allocated at that index)
1 6 1
11 #handle file cluster
1 100 1
11 #handle file cluster
1 101 1
11 #handle file cluster
1 102 1
11 #handle file cluster    (it is going to fail - a data cluster was already allocated at that index)
1 102 1
11 #handle file cluster
1 1000 1
11 #handle file cluster
1 1001 1
11 #handle file cluster
1 2000 1
11 #handle file cluster
1 3000 1
11 #handle file cluster
1 4000 1
11 #handle file cluster    (it is going to fail - a data cluster was already allocated at that index)
1 4000 1
11 #handle file cluster
1 5000 1
11 #handle file cluster
1 5001 1
11 #handle file cluster
1 5002 1
11 #handle file cluster
1 5002 2
11 #handle file cluster
1 5001 2
11 #handle file cluster    (it is going to fail - there was not a data cluster allocated at that index)
1 5010 2
11 #handle file cluster
1 5000 2
11 #handle file cluster
1 4000 2
11 #handle file cluster
1 3000 2
11 #handle file cluster
1 2000 2
11 #handle file cluster
1 1001 2
11 #handle file cluster
1 1000 2
11 #handle file cluster
1 102 2
11 #handle file cluster
1 101 2
11 #handle file cluster    (it is going to fail - there was not a data cluster allocated at that index)
1 200 2
11 #handle file cluster
1 100 2
11 #handle file cluster
1 6 2
11 #handle file cluster    (it is going to fail - there was not a data cluster allocated at that index)
1 5 2
11 #handle file cluster
1 0 2
5 #read inode
1 0
2 #free inode
1
11 #handle file cluster    (it is going to fail - inode is free)
1 0 1
11 #handle file cluster    (it is going to fail - inode is free)
1 0 2
5 #read inode
1 1
0
