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
1 #alloc inode (symbolic link)
3
6 #write inode
8 0 777
20 #init symbolic link
8 ../../../dira3
16 # add dir entry
7 8 sl1
0
5 #read inode
8 0
1 #alloc inode (symbolic link)
3
6 #write inode
9 0 777
20 #init symbolic link
9 dira3/fileb4
16 # add dir entry
0 9 sl2
0
5 #read inode
9 0
1 #alloc inode (symbolic link)
3
6 #write inode
10 0 777
20 #init symbolic link
10 /filea2
16 # add dir entry
3 10 sl3
0
5 #read inode
10 0
14 #get dir entry by path
/sl2
14 #get dir entry by path
/dira1/dirb1/dirc1/sl1
14 #get dir entry by path
/dira1/dirb1/dirc1/sl1/fileb4
14 #get dir entry by path
/dira3/sl3
0
