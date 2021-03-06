Introduction
============

This is the documentation for mod_asn. mod_asn is an Apache module doing
lookups of the :term:`autonomous system (AS)` number, and the :term:`network prefix`, that an
IP address is contained in. 

It is written with scalability in mind. To do high-speed lookups, it uses the
PostgreSQL :term:`ip4r data type` that is indexable with a :term:`Patricia
Trie` algorithm to store network prefixes.

It comes with a script to create such a database, and update it with snapshots
from a router's "view of the world".

The module sets the looked up data as :term:`Apache env table` variables, for
use by other Apache module to do things with it, or for logging -- and it can
add the data as response headers to the client.


Example HTTP response headers::

  HTTP/1.1 200 OK
  Date: Thu, 12 Feb 2009 23:24:33 GMT
  Server: Apache/2.2.11 (Linux/SUSE)
  X-Prefix: 83.133.0.0/16
  X-AS: 13237



Performance
-----------

The database with all ~250.000 prefixes is about 20-30MB in size in the form of
a PostgreSQL database. Without any tuning, it is able to do >3000 lookups per
second on a MacBook Pro (tested with random IPs, a single connection, and
client written in Python running on the same machine).

The Apache module is extremely lightweight. 



Design notes
------------

Performed with a :term:`Patricia Trie` algorithm, the lookup is very efficient.
The Patricia Trie is a special radix tree that works it way from bit to bit,
starting at the most significant bit. At each bit, there are two alternative
"paths". Or put another way, the space of prefixes is roughly divided in two
halfs at each point. The ip4r datatype achieves this by implementing an index
that works this way. Without the index, a full table scan would be required,
plus bitmask prefix match, for each of the ~250.000 candidate rows.

"Conventional" storage in databases is possible with a workaround, e.g. with
two long integers denoting each prefix in a MySQL database. But this would
require an SQL "between" query. An additional column would be needed to store
the prefix length, in order to find the closest match (the most narrow prefix).
The built-in inet/cidr data type in PostgreSQL doens't help either because it
can't be indexed. With conventional methods, only about 30 lookups per second
can be achieved with a database.

Having the data in a real database makes it accessible for other means as well;
for instance, it is easily possible to query it for the list of prefixes that
an AS announces.  In addition, the storage in the database offers the
possibility to change and update the data (or even completely replace it) in a
simple way, by doing this in transaction, which means that it won't block any
running queries. 

The implementation here makes the database accessible through a request
processing handler inside Apache. For usage outside of Apache, a small
libpq-based standalone daemon could be written that queries the database.
Alternatively, a small handler could be written for mod_asn that does nothing
than read an IP address from a request body (or URL) and return the result
(effectively implementing such a specialized server within Apache).

One argument for the ip4r data type in PostgreSQL is that it is IPv6-ready.
Some IPv6 autonomous systems already exist (about 800 as of the beginning of
2009). 

*Update June 2015*: There are ~9747 IPv6 ASs (mapped to ~24000 prefixes). ip4r
fully supports IPv6 since version 2.x, and is currently rolled out. mod_asn
uses this capability from version 1.7 on.


Usage with :term:`MirrorBrain`
--------------------------------

mod_asn can support mod_mirrorbrain (see http://mirrorbrain.org).
mod_mirrorbrain can use the data (set in the subprocess environment) for its
mirror selection algorithm.

The ``mb`` tool that comes with MirrorBrain provide means to query the database::

   # mb iplookup mirror.susestudio.com
  130.57.19.0/24 (AS3680)
   # mb iplookup mirror.susestudio.com --all-prefixes
  130.57.19.0/24 (AS3680)
  130.57.0.0/16, 130.57.0.0/20, 130.57.19.0/24, 130.57.32.0/21, 137.65.0.0/16,
  147.2.0.0/17, 151.155.0.0/16, 164.99.0.0/16, 192.31.114.0/24, 192.94.118.0/24,
  192.108.102.0/24, 192.149.26.0/24, 195.109.215.0/24, 212.153.69.0/24

