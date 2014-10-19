1 #alloc inode (directory)
1
6 #write inode
1 0 777
5 #read inode
1 0
1 #alloc inode (regular file)
2
6 #write inode
2 0 777
5 #read inode
2 0
1 #alloc inode (directory)
1
6 #write inode
3 0 777
5 #read inode
3 0
1 #alloc inode (directory)
1
6 #write inode
4 0 777
5 #read inode
4 0
1 #alloc inode (regular file)
2
6 #write inode
5 0 777
5 #read inode
5 0
1 #alloc inode (directory)
1
6 #write inode
6 0 777
5 #read inode
6 0
1 #alloc inode (directory)
1
6 #write inode
7 0 777
5 #read inode
7 0
16 # add dir entry
0 1 dira1
0
16 # add dir entry
0 2 filea2
0
16 # add dir entry
0 3 dira3
0
16 # add dir entry
1 4 dirb1
0
16 # add dir entry
1 5 fileb2
0
16 # add dir entry
3 6 dirb3
0
16 # add dir entry
3 5 fileb4
0
16 # add dir entry
4 7 dirc1
0
5 #read inode
0 0
5 #read inode
1 0
5 #read inode
2 0
5 #read inode
3 0
5 #read inode
4 0
5 #read inode
5 0
5 #read inode
6 0
5 #read inode
7 0
18 # rename dir entry
0 filea2
faa2
18 # rename dir entry
4 dirc1
dc1
18 # rename dir entry (it is going to fail - there is no entry with that name in the directory)
4 dirc1
dc3
6 #write inode
4 0 444
5 #read inode
4 0
18 # rename dir entry (it is going to fail - no executition permission in the directory)
4 dc1
dc3
6 #write inode
4 0 555
5 #read inode
4 0
18 # rename dir entry (it is going to fail - no write permission in the directory)
4 dc1
dc3
17 # remove dir entry (it is going to fail - no write permission in the directory)
4 dc1
0
6 #write inode
4 0 777
5 #read inode
4 0
17 # remove dir entry
4 dc1
0
5 #read inode
4 0
17 # remove dir entry (it is going to fail - it has been already removed)
4 dc1
0
5 #read inode
4 0
17 # remove dir entry (it is going to fail - directory is not empty)
0 dira1
0
17 # remove dir entry
1 dirb1
0
17 # remove dir entry
1 fileb2
0
17 # remove dir entry
0 dira1
0
17 # remove dir entry (it is going to fail - it is not a directory)
2 dira1
0
17 # remove dir entry
0 faa2
0
17 # remove dir entry
3 dirb3
0
17 # remove dir entry
3 fileb4
0
17 # remove dir entry
0 dira3
0
0
