A program to parse a Windows html help table of contents into an html file
that can be used as an index for a live web page.

The program can also create sitemap file(s) based on the table of contents
as well as update the lastmod labels on existing sitemap files.

To build, see the COPYING file.

See swagmac.conf.doc.txt for documentation on the (required) swagmac.conf input file.

See swagmac.conf-save for an example swagmac.conf file.

See passwin.xml-save for a sample sitemap output file.

See helpIndex.html-save for a sample html help table of contents output file (after running tidy on it).

See frames.html-save for an example of how I used the above table of contents html to reference a (sanitized*) copy of my Windows html help files. <a href="http://www.kuras-pen.net/passwin/frames.html">click for link to live example</a>

<i>*sanitized => If your Windows help files use Microsoft's Active X macros, you'll either need to come up with a substitute JavaScript macro or just ignore those features... You may also run into some funny business with default folder references made by the Windows html Help Viewer. Ie, you'll need to check each web help page and make sure that all of the links work as expected... But, you'd do that anyway, right?</i>

