#!/usr/bin/python

#from sys import exit
import sys
from os import mkdir,path
from ROOT import TFile,TH1,TCanvas
from optparse import OptionParser
from shutil import copy2

# globals
img_suffix = "png"

# need to initialize a canvas to print out
c1 = TCanvas("c1", "A canvas", 800, 600)

def write_html_header(f, thedir):
    f.write("<html>\n<head>\n<title>")
    f.write(thedir)
    f.write("</title>\n</head>\n<body>\n\n")

    header_str = "<h1>"
    dirs = thedir.split("/")
    
    print "printing out for " + thedir + " with " + str(len(dirs)) + " elements" 

    for x in xrange(len(dirs)):
        if(x == 0):
            header_str = header_str + "/"
        else:
            header_str = header_str + dirs[x] + "/"
            
    header_str = header_str + "</h1>"
    f.write(header_str+"\n\n")

def write_html_footer(f):
    f.write("\n</body>\n</html>\n")


def write_html_dirs(f, dirs):
    f.write("<ul>\n")
    f.write("<li> <a href=\"../\">../</a>\n")

    for dir in dirs:
        f.write("<li> <a href=\"" + dir + "\">" + dir + "/</a>\n")

    f.write("</ul>\n\n")

def write_html_hists(f, hists):
    for hist in hists:
        hist_file = hist + "."  + img_suffix
        #f.write("<h23>" + hist + "</h2>\n")
        f.write("<img src =\"" + hist_file + "\" alt=\"" + hist_file + "\">\n")


def process_files(rootdir, basedir, subdir):
    dirs = []
    hists = []

    html_file = open(basedir + "/" + subdir + "/index.html", 'w')
    write_html_header(html_file, subdir)
    #write_html_header(html_file, basedir)

    for key in rootdir.GetListOfKeys():

        name = key.GetName()
        obj = rootdir.Get( key.GetName() )
        print key.GetName() + " is an object of type " + str(type(obj))

        obj_type = str(type(obj))
        if obj_type == "<class 'ROOT.TDirectoryFile'>":
            dirs.append(name)
            newdir = basedir + "/" + subdir + "/" + name

            if not path.exists(newdir):
                mkdir(newdir,0755)

            #process_files( obj, newdir )
            process_files( obj, basedir, subdir+"/"+name )
#        elif str(type(obj)) == "ROOT.TH1":
        elif obj_type.find(".TH") > 0:
            hists.append(name)
            if obj_type.find(".TH1") > 0:
                obj.Draw()
            elif obj_type.find(".TH2") > 0:
                obj.Draw("COLZ")
            c1.Print(basedir + "/" + subdir + "/" + name + "."  + img_suffix)
            

    write_html_dirs(html_file, dirs)
    write_html_hists(html_file, hists)

    write_html_footer(html_file)

if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-f", "--file", dest="filename", help="read ROOT histograms from FILE", metavar="FILE")
    parser.add_option("-d", "--dir", dest="dir", help="set base directory to DIR", metavar="DIR")
    parser.add_option("-s", "--suffix", dest="suffix", help="image file suffix" )
    (options,args) = parser.parse_args()

    basedir = "."
    
    if options.filename is None:
        print "did not specify input file name!"
        sys.exit()
    if options.suffix is not None:
        img_suffix = options.suffix
    if options.dir is not None:
        basedir = options.dir

    #print options.filename
    f = TFile( str(options.filename) )

    basedir = basedir + "/" + img_suffix
    if not path.exists(basedir):
        mkdir(basedir,0755)

    # copy ROOT file
    copy2( str(options.filename), basedir )

    # make image files
    process_files(f, basedir, ".")

    

    f.Close()
