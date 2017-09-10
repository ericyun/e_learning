#!/usr/bin/env  python2.7
import sys
import time
from optparse import OptionParser
import os
import pprint
def dir_list(path, allfile,alldir):
    filelist =  os.listdir(path)
    tempdir = []
    for filename in filelist:
        filepath = os.path.join(path, filename)
        if os.path.isdir(filepath):
            tempdir.append(filepath)
            dir_list(filepath, allfile,alldir)
        else:
            pass
            #allfile.append(filepath)
    if  len(tempdir) >  0 :
        alldir.append(tempdir)
    return [alldir,allfile]
def dir_list3(path,dirs):
    filelist =  os.listdir(path)
    cur_dir = []
    for filename in filelist:
        filepath = os.path.join(path, filename)
        if os.path.isdir(filepath):
            cur_dir.append(filepath)
            temp_dir = dir_list3(filepath, cur_dir)
            if len(temp_dir) > 0 :
                for dir in temp_dir :
                    cur_dir.append(dir)
    return cur_dir
def get_ref_dir_depth(ref, target):
    if target.find(ref) != 0 :
        print(ref, target)
        raise "no sub dir"
    ref_len = len(ref) +1
    num = 0
    sub = target[ref_len:]
    for ch in sub:
        if ch == '/' :
            num  = num + 1
    if sub[-1] == '/':
        num =  num -1
    return (num, sub)
def  walk_dir(top_dir):
    dir_list = []
    for dirpath, dirnames, files in os.walk(top_dir):        
        for dir in dirnames:
            abs_dir = os.path.abspath(dir)
            print("%s --> %s"%(dirpath,dir))
def get_chapter_index(num): 
    ch =  ''
    if num == 0:
        ch = '*'
    elif num == 1:
        ch = '-'
    elif  num == 2 :
        ch = '.'
    ch =  '*'
    return "%s%s ["%("    "*num,ch)
    return "%s* [No. %d"%("\t"*num, num)
def get_chapter_des(file):
    if os.path.exists(file) == False:
        fd = open(file,"w")
        fd.write("TODO !! %s"%file)
        fd.close()
    fd = open(file)
    des = fd.readline()
    fd.close()
    print("file ->",file, des)
    return des
def write_summary_file(file, lines):
    fd = open(file,"w")
    fd.writelines(lines)
    fd.close()

def gen_dir_content(book_top,dir):
    result = []
    (num, sub) = get_ref_dir_depth(book_top, dir);
    index = get_chapter_index(num)
    print(index, sub)
    title_des =  get_chapter_des("%s/README.md"%(sub))
    title_des = title_des.replace("\n","")
    print("title des ->",title_des)
    line = "%s %s](%s/README.md)\n"%(index, title_des, sub)
    result.append(line)
    files = os.listdir(dir)
    print("get dir contet ",dir)
    for fd in files :
        if fd == "README.md":
            continue
        if fd.endswith("md") == False:
            continue
        absfd = "%s/%s"%(dir,fd)
        print(fd)
        if os.path.isfile(absfd) == True:
            print("check ->",fd)
            des = get_chapter_des(absfd)
            index = get_chapter_index(num+1)
            title_des =  get_chapter_des(absfd)
            title_des = title_des.replace("\n","")
            line = "%s %s](%s/%s)\n"%(index, title_des,sub,fd)
            print(line)
            result.append(line)
            #break
            pass
            
    
    return result

def get_chapter_list(chapter_file,book_top):
    chapter_list = []
    if os.path.exists(chapter_file) == True:
        print("get chapter list from chaper file");
    else:
        if book_top  == "." :
            book_top = os.getcwd();
        print("auto get chapter list from top dir ->",book_top)
    dirs = dir_list3(book_top,[])
    pprint.pprint(dirs)
    return dirs
    #pprint.pprint(dirs)
    
   
    
        
def gen_summary_header(book_top):
    result = []
    result.append("#SUMMARY\n")
   # result.append("\n")
    result.append("*[Brife](README.md)\n")

    return result
def gen_sumary_chapters(chapter_list):
    pass
def gen_sumary_end():
    pass
def gen_chapter_line(num, dir):
    pass
def gen_book_sumary(chapter_file ,book_top,output_file):
    result  = []
    header_lines = gen_summary_header(book_top);
    for line in header_lines:
        result.append(line)
    chapter_list = get_chapter_list(chapter_file,book_top)
    index = 0 
    for dir in chapter_list :
        if dir.find("_book") != -1:
            continue
        if dir.find("node_modules") != -1:
            continue
   #     (depth, target_dir) = get_ref_dir_depth(os.path.abspath(book_top), dir)
        lines  = gen_dir_content(os.path.abspath(book_top),dir)
        #chapter_list.append("\n")
        for line in lines :
            result.append(line)
        index =  index +1 
        if index > 10:
            continue
            break
       # print((depth, target_dir));
        #break
    print("\nsummary->\n");
    pprint.pprint(result)
    
    write_summary_file(output_file, result)
    
#    print (result)
    #print(summary_string)
        
    pass
def parse_sys_args():
    parser = OptionParser(usage="usage:%prog [optinos] filepath")
    parser.add_option("-c", "--chapter_list",
                    action = "store",
                    type = 'str',
                    dest = "chapter_list",
                    default = "ichapter.list",
                    help="chapter list file"
                    )
    parser.add_option("-b", "--book_top",
                    action = "store",
                    type = 'str',
                    dest = "book_top",
                    default = ".",
                    help = "book top dir"
                    )
    parser.add_option("-d", "--output_dir",
                    action = "store",
                    type = 'str',
                    dest = "output_dir",
                    default = "_book",
                    help = "output book dir"
                    )
    parser.add_option("-f", "--format",
                    action = "store",
                    type = 'str',
                    dest = "format",
                    default = "RTL",
                    help = "output_format"
                    )
    parser.add_option("-m", "--work_mode",
                    action = "store",
                    type = 'str',
                    dest = "work_mode",
                    default = "gen_summary",
                    help = "app work mode -> gen_summary ,gen_book"
                    )
    parser.add_option("-o", "--output_file",
                    action = "store",
                    type = 'str',
                    dest = "output_file",
                    default = "SUMMARY.md",
                    help = "app work mode -> gen_summary ,gen_book"
                    )
    (options, args) = parser.parse_args()
    print("chapter list -> ",options.chapter_list)
    print("book_top -> ",options.book_top)
    print("output_dir -> ",options.output_dir)
    print("work_mode-> ",options.work_mode)
    print("output_file-> ",options.output_file)
    return (options, args)
if __name__ == "__main__":
    (options, args) = parse_sys_args();
    if(options.work_mode == "gen_summary"):
        gen_book_sumary(options.chapter_list, options.book_top, options.output_file)
