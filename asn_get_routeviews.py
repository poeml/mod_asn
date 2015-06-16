#!/usr/bin/env python

import os, os.path
import sys
import time
import urllib

# the data snapshot that we need is put into monthly directories, like this:
# example url: 'http://archive.routeviews.org/oix-route-views/2008.11/oix-full-snapshot-latest.dat.bz2'

filenames = ['oix-full-snapshot-latest.dat.bz2', 'ipv6-rib-snapshot-latest.txt.bz2']

for filename in filenames:
    #url = 'http://archive.routeviews.org/oix-route-views/%s/%s' \
    #        % (time.strftime("%Y.%m", time.gmtime()), filename)

    # mirrored daily from archive.routeviews.org, to save routeviews.org the traffic
    url = 'http://mirrorbrain.org/routeviews/%s' % filename

    if len(sys.argv) > 1 and sys.argv[1] == '--no-download':
        sys.argv.pop(1)
    else:
        if os.path.exists(filename) \
           and (time.time() - os.path.getmtime(filename)) < (60 * 60 * 8):
            print >>sys.stderr, 'Using existing file "%s", because it is less than 8h old.' % filename
            print >>sys.stderr, 'Remove it to have it downloaded again.'
        else:
            print >>sys.stderr, 'Downloading', url
            urllib.urlretrieve(url, filename=filename)

    if len(sys.argv) > 1 and sys.argv[1] == '--download-only':
        sys.exit(0)


def gen_open(filenames): 
    """Open a sequence of filenames"""
    import gzip, bz2 
    for name in filenames: 
        if name.endswith(".gz"): 
             yield gzip.open(name) 
        elif name.endswith(".bz2"): 
             yield bz2.BZ2File(name) 
        else: 
             yield open(name) 

def gen_cat(sources): 
    """Concatenate items from one or more 
    source into a single sequence of items"""
    for s in sources: 
        for item in s: 
            yield item.rstrip()

def gen_lines(lines): 
    """Some lines in IPv4 data come broken in two lines, like this:

    *  63.105.200.0/21  203.181.248.168          0      0      0 7660 2516 703 9848 9957 i
    *  63.105.202.0/27  203.62.252.186           0      0      0 1221 4637 4766 9318 9957 9957 9286 i
    *  63.105.204.128/25
                         203.62.252.186           0      0      0 1221 4637 4766 9318 9957 i
    *  63.105.205.0/25  203.62.252.186           0      0      0 1221 4637 4766 9318 9957 i
    *  63.105.207.144/28
                         203.62.252.186           0      0      0 1221 4637 4766 9318 9957 9957 9286 i
    *  63.105.248.0/21  196.7.106.245            0      0      0 2905 701 19830 i

    This generator puts them together, and outputs them on one line.
    """
    lastline = ''
    for line in lines: 
        if ':' in line:
            # IPv6
            yield line
        else:
            # IPv4
            if len(line) > 35: 
                if lastline:
                    #print 'last:', lastline
                    #print 'line:', line
                    yield lastline + line
                else:
                    yield line
                lastline = ''
            else:
                lastline = line


def gen_grep(patc, pat6c, lines): 
    """Generate a sequence of lines that contain 
    a given regular expression"""
    for line in lines: 
        if patc.search(line) or pat6c.search(line): 
            yield line


def gen_asn(lines): 
    """Generate a sequence of lines that end in 'i'
    and return the first, third last and second word for each of them.

    Ignore lines ending in '?' (that's marking incomplete entries), 
    but complain if a line otherwise doesn't end in 'i' or 'e'.

    For prefix 0.0.0.0/0, we don't return AS number 286 - but rather zero,
    because this is more meaningful later. An AS with number 0 doesn't exist.
    0.0.0.0/0 will be the prefix that contains 127.0.0.1.

    In routeviews data, 0.0.0.0/0 seems to be listed with a random (changing)
    AS number, which seems like an artifact.
    """
    for line in lines: 
        s = line.split()

        # IPv6?
        if ':' in line:

            # There have been few AS_SETs used, before they were depracated (see rfc6472)
            # we use the first AS in those cases
            # 2001:0410::/32 6509 {271,7860,8111,26677}
            # ->
            # 2001:0410::/32 6509 26677
            # there are AS_SETs with a single AS, also:
            # 2001:0578:0600::/40 3257 3356 22773 {62957}
            if s[-1].startswith('{'):
                s[-1] = s[-1].lstrip('{').rstrip('}').split(',')[-1]

            if len(s) > 2:
                yield s[0], s[-2], s[-1]
            else:
                # no AS neighbour - that happens
                yield s[0], None, s[-1]

        # IPv4
        else:
            if s[-1] == '?':
                continue
            if s[-1] not in ['i', 'e']:
                print >>sys.stderr, repr(line)
                sys.exit('Error: unusal line seen, ending in %r' % s[-1])
            if s[1].startswith('0.0.0.0/0'):
                # see comment above
                yield s[1], '0', '0'
            # drop the 'i' at the end
            s.pop()
            # drop doublettes of the as number at the end
            while s[-1] == s[-2]:
                s.pop()
            yield s[1], s[-2], s[-1]

# not used here, but useful another time maybe...
def gen_uniq(lines): 
    """Generate a sequence of lines that filters
    lines that are identical to the line before"""
    lastline = ''
    for line in lines: 
        if line != lastline:
            yield line
        lastline = line


def gen_firstuniq(tupls): 
    """Generate a sequence of tuples that filters
    tuples where the first word is the same as on the line above"""
    last = ''
    for tupl in tupls: 

        if tupl[0] != last:
            yield tupl

        last = tupl[0]


def main():
    """
    Create a generator pipeline and process 900 MB's worth of routeviews data.

    You can directly process the bz2 or gz compressed file. If you unpack it
    before, it can be a few times faster, but the uncompressed data is nearly a
    GB in size (2008).

    The output format is, for each line:

    prefix asnpeer asn

    Usage: get_routeviews [oix.dat[.gz|.bz2]]

    Will read an existing file named 'oix-full-snapshot-latest.dat.bz2' if no
    argument is given.

    If the file is older than 1 week, the script will suggest to download it
    again. It'll automatically do so if you remove the file.
    """
    import re 

    # IPv4:
    # ignore lines not matching regular expression for '* 1.2.3.4/11 '
    # this filters seemingly broken lines like these:
    #
    # '*  12.127.255.255/3212.0.1.63                0      0      0 7018 i'
    #
    # '*  61.19.0.0/20     164.128.32.11            0      0      0 3303 1273 4651 2.17 i'
    #
    # '*  12.12.96.0/20    209.123.12.51            0      0      0 8001 3257 7018 32328 {32786} i'
    #
    pat = r'^\*\s+\d+\.\d+\.\d+\.\d+/\d+\s+.* \d+ [ie]'

    # IPv6:
    # f8f0:1100::/24 393406 4258 3356 3561 40443
    # 2001:067c:15b0:0000:0000:0000:0000:0001/128 22652 5580 60922
    # 2408::/22 3257 2914 4697 55817
    # 2001:0428:4c02:01fd::/64 209
    #                         ^ no neighbour AS in this case
    pat6 = r'^[0-9a-fA-F]+:.*/'

    patc = re.compile(pat) 
    pat6c = re.compile(pat6) 

    global filenames
    if len(sys.argv[1:]):
        filenames = sys.argv[1:]

    try:

        oixfile     = gen_open(filenames)
        oixlines    = gen_cat(oixfile)
        fixedlines  = gen_lines(oixlines)
        patlines    = gen_grep(patc, pat6c, fixedlines)
        pfxasn      = gen_asn(patlines)
        pfxasn_uniq = gen_firstuniq(pfxasn)

        for pfx, asnb, asn in pfxasn_uniq:
            print pfx, asnb, asn

    except KeyboardInterrupt:
        sys.exit('interrupted!')
    except IOError, e:
        sys.exit(e)

if __name__ == '__main__':
    main()
