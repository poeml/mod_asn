.. _glossary:

Glossary
========

.. glossary::


   Apache env table
      Within the Apache HTTP server, a table that is maintained during request processing. It
      is used by handlers that run during request processing to pass along arbitrary data to be 
      available in later phases. Hense the name, environment table.

   Autonomous system (AS)
      A collection of connected Internet Protocol (IP) routing prefixes under
      the control of one or more network operators that presents a common
      source files for one Sphinx project. See
      http://en.wikipedia.org/wiki/Autonomous_system_number for more
      information.

   ip4r data type
      An indexable data type for ranges of IPv4 addresses.
      PostgreSQL has builtin data types for IP addresses and ranges ('inet' and
      'cidr'), but they cannot be indexed. See http://ip4r.projects.postgresql.org/

   MirrorBrain
      A download redirector and Metalink generator, which can use mod_asn to
      refine the selection of content mirror servers. See http://mirrorbrain.org/

   Network prefix
      A topological space of addresses of networked computers grouped together,
      which is significant for routing decisions. See 
      http://en.wikipedia.org/wiki/Subnetwork for a detailed introduction.

   Patricia trie
      A specialized data structure based on a so-called "trie", used to store a
      set of strings. In the case of IP addresses / network prefixes, it is known to perform
      best for efficient lookups. See http://en.wikipedia.org/wiki/Radix_tree for more info.


.. vim: :ai:
