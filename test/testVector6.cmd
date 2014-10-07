1 #alloc inode
2
1 #alloc inode
2
2 #free inode
2
6 #write inode (it is going to fail - inode 1 is in use)
1 1 555
6 #write inode (it is going to fail - inode 2 is free in dirty state)
2 0 222
6 #write inode (it is going to fail - inode 7 is free)
7 0 222
6 #write inode (it is going to fail - inode 7 is free)
7 1 222
6 #write inode (it is going to fail - inode 100 is out of range)
100 1 222
6 #write inode
1 0 555
6 #write inode
2 1 222
5 #read inode (it is going to fail - inode 1 is in use)
1 1
5 #read inode (it is going to fail - inode 2 is free in dirty state)
2 0
5 #read inode (it is going to fail - inode 7 is free)
7 0
5 #read inode (it is going to fail - inode 7 is free)
7 1
5 #read inode (it is going to fail - inode 100 is out of range)
100 1
5 #read inode
1 0
5 #read inode
2 1
8 #access granted (it is going to fail - inode 2 is free in dirty state)
2 1
8 #access granted (it is going to fail - inode 7 is free)
7 1
8 #access granted (it is going to fail - inode 100 is out of range)
100 1
8 #access granted (it is going to fail - illegal operation)
1 0
8 #access granted (it is going to fail - illegal operation)
1 10
8 #access granted (single test)
1 1
8 #access granted (double test)
1 5
8 #access denied (single test)
1 2
8 #access denied (double test)
1 6
8 #access denied (triple test)
1 7
7 #clean inode (it is going to fail - inode 1 is in use)
1
7 #clean inode (it is going to fail - inode 7 is free in clean state)
7
7 #clean inode (it is going to fail - inode 100 is out of range)
100
7 #clean inode (it is going to fail - inode 0 can not be cleaned)
0
7 #clean inode (OK, since inode 2  is free in dirty state, it can be cleaned)
2
0
