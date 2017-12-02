----
## 恢复文件
git checkout audiobox2/include/base.h /*恢复修改的文件*/
git reset HEAD audiobox2/include/base.h　/*已经add的文件需要这样先从工作区中移除，然后再checkout*/
git checkout audiobox2/include/base.h /*恢复修改的文件*/

----
## t

----
## e

----
## r

----
## d

----
## c

----
## 

----
## 

----
## 

常用的操作：
1. 强制切换版本:
   git reset --hard 2d7cef2c3ed41762986c8632effca15883974810
2. 强制删除所有untracked files.
   git clean -f
3. 把commit id改动移植到当前分支
　git cherry-pick 6c2f9a6dc0f742568bc4a720fb3267f65af728fe
　git cherry-pick <commit id>
4. git rebase -i HEAD~~~~     ~的个数代表向前回溯的commit个数

git init 生成空的.git目录而已
.git/HEAD --- ref: refs/heads/master current active branch.
    git checkout master 之后，HEAD的值就是成了我们checkout的提交的SHA-1值

git clone ssh://eric.yun@gerrit.in.infotm.com:29418/manifest/buildroot/ -b dev_qsdk
git add <path>　　<path>可以是文件也可以是目录
git add -u [<path>]
git rm <file>

git log
git commit -am "some comment"
git push

Git索引是一个在你的工作目录和项目仓库间的暂存区(staging area). 有了它, 你可以把许多内容的修改一起提交(commit). 如果你创建了一个提交(commit), 那么提交的是当前索引(index)里的内容, 而不是工作目录中的内容.
    git add 不但是用来添加不在版本控制中的新文件，也用于添加已在版本控制中但是刚修改过的文件
    你可以在你的顶层工作目录中添加一个叫".gitignore"的文件，来告诉Git系统要忽略 掉哪些文件
表示commit或其他git对象的方式：
    git命令中可以使用不重复的Sha短名(会自动补齐)，也可以使用分支,remote或标签名来代替SHA串名, 它们只是指向某个对象的指针. 假设你的master分支目前在提交(commit):'980e3'上, 现在把它推送(push)到origin上并把它命名为标签'v1.0', 那么下面的串都会被git视为等价的,git log会有相同的输出:
        980e3ccdaac54a0d4de358f3fe5d718027d96aae  origin/master  refs/remotes/origin/master  master  refs/heads/master  v1.0  refs/tags/v1.0
    7b593b5..51bea1".." 区间，表示之间所有的commit，7b593b5..表示之后的commit
求教gitk　tree解释
GIT学习和实验
cd /home/yuan
mkdir testgit
cd testgit
mv /home/yuan/dev_qsdk/system/testing/watchdog/ .
git init
git add watchdog
git status
git commit -m "my first commit"
cd watchdog
touch t1.txt
git add t1.txt
git commit -m "add t1.txt"
touch t1.txt
git commit -am "add t2.txt" 没有git add, fail.
git add t2.txt
git reset HEAD t2.txt
git branch -v
git checkout -b slave　　　//-b　create a new branch.
    git branch slave
    git checkout slave
git add t2.txt    // "git add ."  will add all files
git add t6
git rm -f t6    //need force remove if in staging area.
git commit -am "add t2.txt"
git checkout master //no t2.txt on master branch
git merge slave     //t2.txt merged to master branch

git add t2.txt　//After Modify t2.txt
git reset --hard HEAD        //Cancel last merge, before commit
git commit -am "merge t2.txt"
git branch -d slave

git reset --hard ORIG_HEAD　　//Cancel last merge.
git branch slave
git commit -am "change t1.txt" //After Modify t1.txt
git diff master..slave    //branch diff
git log
git diff 78ca0fdbd3aad..86007ef4926d910ac  //version diff

git diff --cached //common part of staging and working area
git diff HEAD      //compare with current branch HEAD
git diff      //diff part of staging and working area
git checkout master
git diff slave      //compare with slave branch
git diff --stat

git tag v1.0
git tag v0.1 1b2e1d63ff //tag on a old object, refer to specified commit
git tag -a v0.2 1b2e1d63ff -m "new tag"//create and refer to a new tag object
git tag -d v1.0

git stash
git stash list
git stash apply/show/drop/pop stash@{1} 或者　git stash apply/show/drop/pop　对最新的stash
git stash apply --index
git stash branch <branch name>
git stash clear

git config --global core.editor gedit
git config --global core.editor emacs

git clone --bare /home/yuan/work/testgit maingit

新的仓库
cd /home/yuan/work
git clone /home/yuan/work/localgit newgit
git branch
git branch -r             //display: origin/HEAD-->origin/master origin/master origin/slave
git remote             //display: origion
git remote add rem_repo /home/yuan/work/testgit
git remote             //display: origion rem_repo

git fetch rem_repo slave1:local_slave1    //create branch:local_slave1, and .git/FETCH_HEAD　only include slave1 info
git fetch rem_repo
git fetch  /  git fetch origin
git fetch rem_repo slave
git merge rem_repo/slave

git branch --track slave origin/slave //tracking branch, no use for me for now.

touch t5.txt
git add t5.txt
git commit -am "local add t5.txt"
git revert HEAD        //add new commit to cancel last commit contents, content keep same as before last commit
git commit --amend     //modify last commit: both content and comment

cd /home/yuan/work/testgit
git pull /home/yuan/work/localgit slave    //fetch then merge

git remote add local_repo /home/yuan/work/localgit  //"new_repo" is short cut to remote repo
git log -p slave..local_repo/slave        //diff after slave.
git fetch local_repo slave            //FETCH_HEAD
git merge local_repo/slave            //merge  new_repo slave branch
git pull . remotes/local_repo/slave        //fetch then merge
git pull local_repo slave:master            //to master branch
git pull local_repo slave                //to current branch

git checkout -- t4.txt    //restore t4.txt
git checkout watchdog

git checkout -b newbase origin     //
git checkout newbase
git add .            //resolve conflict
git rebase --continue         //exec after resolve conflict
git push origin HEAD:master    //push update on newbase branch to origin master branch
git checkout master
git pull            //update to latest
git checkout 892739473205    //892739473205 is the same as svn version

git pull需要全解析

git pull = git fetch + git merge
git pull --rebase = git fetch + git rebase
git rebase --abort　　终止rebase的行动，并且mywork分支会回到rebase开始前的状态

这样的 network 最漂亮: 同分支总是 git pull --rebase origin xxx, 合并分支总是 git merge --no-ff xxx 禁止 rebase

有些常用命令的基本格式：
Git diff 工作区与stage差异
Git diff HEAD 工作区与HEAD版本差异
git diff --cached  stage和版本库差异ｄｓ
git rm --cached t5.txt    //rm from repo, but keep working version
git grep -n watchdog v1.0/master
git grep -e define --and -e printf
git grep \( -e define --or -e NULL \) --and -e printfc // support ()
git --version
git mv old_name new_name    //
git rm <file>...
git diff  git diff --cached    //show non_cached or cached contents
git add <file>...        //add to staging area
git reset HEAD <file>...    //clear these files from staging area
git checkout -- <file>...     //restore modified files, from HEAD
git revert <commit-id> 这条命令会把指定的提交的所有修改回滚，并同时生成一个新的提交。
git reset [options] <commit>    git reset会修改HEAD到指定的状态
    这条命令会使HEAD提向指定的Commit，一般会用到3个参数，这3个参数会影响到工作区与暂存区中的修改：
        --soft: 只改变HEAD的State，不更改工作区与暂存区的内容 --mixed(默认): 撤销暂存区的修改，暂存区的修改会转移到工作区 --hard: 撤销工作区与暂存区的修改

最好不要在master分支上直接开发，而是创建新的本地issue branch,中间开发过程可以比较随意的commit, 开发完成之后再合并到master，master上只会增加一个commit,然后再push?
git mergetool
git branch -v
git branch --merged
git branch --no-merged
git branch <branch-name>
git checkout -- [file]     //restore repo version
git checkout -b <branch-name>
git branch -d <branch-name>
git branch -D <branch-name>
git fetch <remote>
git fetch <remote>　<remote-branch>
git merge <remote>/<remote-branch>
    git merge --abort //abort if any conflict
    git reset --hard HEAD　　//restore
git push <remote> <local-branch>:<remote-branch>
如果本地分支与远程分支同名, git push <remote> <branch-name> 等价于 git push <remote> HEAD:<branch-name> 等价于 git push <remote> refs/heads/<branch-name>:refs/for/<branch-name>
如果本地分支的名字为空，可以删除远程分支:  git push <remote> :<remote-branch>
如果是本地分支之间的操作，应该用git merge
git remote add <short-name> <url>   添加远程分支
git remote show
git ls-remote

远程跟踪分支是远程分支状态的引用,它们以 (remote)/(branch) 形式命名, 缺省的origin/master，远程update时候本地的不会改变，git fetch origin, 如果本地master和远程都有新的commit,那么origin/master和本地master
    就成了两个分叉开的branch, 需要merge操作。

slave 分支开发软件
    git checkout master
    git merge slave

(my_branch) $ git add .
(my_branch) $ git commit -m "..."
(my_branch) $ git checkout devel
(devel) $ git pull origin devel
(devel) $ git merge my_branch
(devel) $ git push origin devel

本地保持一个ｍａｓｔｅｒ分支，每次提交到远程之前，先ｐｕｌｌ一下，然后再合并自己的修改到ｍａｓｔｅｒ，然后再ｐｕｓｈ
自己的开发应该是在新的ｔｅｓｔ分支上。
！！！！！如果想要在自己的分之上更加随意的修改，怎么办，能否手动修改自己的commit记录再ｐｕｓｈ呢？