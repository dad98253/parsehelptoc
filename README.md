A program to parse a Windows html help table of contents into an html file
that can be used as an index for a live web page.

The program can also create sitemap file(s) based on the table of contents
as well as update the lastmod lables on exsisting sitmap files.

To compile on ubuntu:

sudo apt-get install zlib1g-dev libxml2-dev
git clone ...
cd  parsehelptoc
./configure
make

