#!/usr/bin/env python2.7

import sys

def help():
    print("app line file")

if __name__ == "__main__":
   if len(sys.argv) < 3 :
       help()
   fd = open(sys.argv[2],"r")
   old_lines =  fd.readlines()
   fd.close()
   new_lines = []
  # print("replace->",sys.argv[1])
   for line in old_lines :
       if line.find(sys.argv[1]) < 0:
           new_lines.append(line)
           #print("skip", line)
           continue
       else:
           print line
           start_index = line.find("(")
           end_index = line.find(")")
           temp_index = line.rfind("/")
           print temp_index-start_index, len(sys.argv[1])
           if temp_index-start_index != len(sys.argv[1]):
               new_lines.append(line)
           #pr int("skip", line)
               continue
           #print start_index, end_index
           new_line = line[:start_index+1]+sys.argv[1]+"_book" +line[end_index:]
           #print(new_line)
           new_lines.append(new_line)

   fd=open(sys.argv[2],"w")
   fd.writelines(new_lines)
   fd.close()
