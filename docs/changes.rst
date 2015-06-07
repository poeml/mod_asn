
Release Notes/Change History
============================

Release TBA (TBA)
-------------------------------

mod_asn now supports AS lookups of IPv6 addresses. This requires
a current version of the contrib ip4r PostgreSQL data type (which
despite its name now supports IPv6), and a slightly changed
database scheme - it is probably simplest if you drop the ``pfx2asn``
database table and recreate it.


Release 1.6 (r100, Jan 7, 2014)
-------------------------------

This release adjusts for the API changes in Apache 2.4. Thanks Cristian
Rodriguez for the help. (`issue 128`_, `issue 129`_)

This release also fixes a bug in the :program:`asn_get_routeviews` script: It
could fail when the BGP routing data snapshot contains bogus AS numbers. (`issue 93`_) 
Patch courtesy of *agy*.

:program:`asn_get_routeviews` now allows to only download routing data, but
don't process it, by using the switch ``--download-only``. In addition,
``--no-download`` can be used if the data is distributed by other means, e.g.
with distro updates. Thanks Dagobert Michelsen for the suggestion! (`issue
127`_)


This release also adds documentation.

.. _`issue 93`: http://mirrorbrain.org/issues/issue93
.. _`issue 127`: http://mirrorbrain.org/issues/issue127
.. _`issue 128`: http://mirrorbrain.org/issues/issue128
.. _`issue 129`: http://mirrorbrain.org/issues/issue129


Release 1.5 (r88, Sep 5, 2010)
------------------------------

This release fixes one important bug, and improves documentation.

* mod_asn now avoids lookups of IPv6 addresses. The database of AS (autonomous
  system) numbers is IPv4-only, and in addition, attempted lookups seem to
  cause problems within the PostgreSQL ``ip4r`` contrib data type. The symptom
  was a failure of the database after a while of running, and subsequent error
  messages from Apache. See `issue 58`_.

* The used version of the APR/APR-Util library is now checked when Apache
  starts, and not when the module is compiled. This is useful to choose the
  correct way to access the database, which unfortunately changed between the
  1.2 and 1.3 (APR-Util) release. This change makes the deployment more robust,
  because even if a user mixes packages from different distro versions on a
  system, mod_asn will still work correctly. This improves the existing fix for
  `issue 7`_.
  
* The documentation has been updated with

  - updated examples of Debian package names and filenames
  - an improved example about installing onto an existing database


.. _`issue 7`: http://mirrorbrain.org/issues/issue7
.. _`issue 58`: http://mirrorbrain.org/issues/issue58


Release 1.4 (r79, Mar 27, 2010) 
-------------------------------

This release does not bring about significant user-visible changes, but under
the hood, some optimizations were done.

* For more efficient database connection usage, mod_asn now closes the used
  connection when its handler quits. Before, a connection with lifetime of the
  request was acquired; if a long-running handler runs after mod_asn, this
  could mean that the connection is blocked for other threads until the end of
  the request. This could occur, for instance, when mod_mirrorbrain ran later,
  but exited early because a file was supposed to be delivered directly.
  This was tracked in `issue 44`_.

* Database errors from the lower DBD layer are now resolved to strings, where
  available. In relation to this: if an IP address is not found it isn't
  necessarily an error, because it could be a private IP, for instance, which
  is never present in global routing tables. That case is now logged with
  NOTICE log level.

* When compiling mod_asn with the Apache Portable Runtime 1.2, different
  semantics are used to access database rows, couting from 0 instead of from 1. It
  seemed to work either way (maybe because only a single row is accessed), but
  hopefully now it is done more correctly and therefore safer in the future.
  See `issue 29`_ and `issue 7`_ for the context.


* In the documentation, the support scripts are now mentioned without their
  :file:`.py` suffix in the example for data import, which might be less
  confusing.

.. _`issue 44`: http://mirrorbrain.org/issues/issue44
.. _`issue 29`: http://mirrorbrain.org/issues/issue29
.. _`issue 7`: http://mirrorbrain.org/issues/issue7


Release 1.3 (r70, Jul 30, 2009)
-------------------------------

* Bugs in the :program:`asn_get_routeviews` and :program:`asn_import` scripts were fixed:

  - The logic which decided whether to download the routing data snapshot file
    was fixed.  If :program:`asn_get_routeviews` is called and it finds a file
    which was downloaded less then 8 hours ago, the file is reused. If no file
    exists or the file is older than 8 hours, it is downloaded again.

  - Deletion of existing entries in the database is now prevented, if not at
    least one entry has been imported. This fixes a bug where the routing data
    would be deleted if the script was called with no input.


Release 1.2 (Jul 28, 2009)
--------------------------

* :program:`asn_get_routeviews` script:

  - download data from the `mirror <http://mirrorbrain.org/routeviews/>`_
    provided by the MirrorBrain project, so routeviews.org doesn't get
    additional traffic by additional users downloading from them

* the documentation has been moved into a docs subdirectory, and rewritten in
  reStructured Text format, from which HTML is be generated via Sphinx
  (http://sphinx.pocoo.org/). When the documentation is changed in subversion,
  the changes automatically get online on http://mirrorbrain.org/mod_asn/docs/

* documentation updates

  - section :ref:`keep_the_data_up_to_date` added
  - add :ref:`upgrading` notes about PostgreSQL (8.4)
  - install the new documentaion when building Debian or RPM packages

* "debian" subdirectory added, for Debian package builds

* the Subversion repository was moved to http://svn.mirrorbrain.org/svn/mod_asn/trunk/


Release 1.1 (Jul 4, 2009)
-------------------------

* mod_asn.c: 

  - bump version (1.1)
  - update year in copyright header

* :program:`asn_import` script:

  - be able to read config from :file:`/etc/asn_import.conf` or
    :file:`/etc/mirrorbrain.conf`; thus, the script doesn't need to be edited
    any longer with database configuration data and credentials.

    1. if a MirrorBrain config file is found, it is used (and the MirrorBrain
       instance can be selected with -b on the commandline, if needed) 
    2. alternatively, the script looks for a config file named
       :file:`/etc/asn_import.conf`.

* :program:`asn_get_routeviews` script:

  - handle the slightly changed format of routeviews data
  - more sanity checks for parsing newer routing data

* INSTALL:

  - add links to binaries for Debian and ebuilds for Gentoo
  - add instructions for troubleshooting and testing
  - correct a wrong example of loading mod_asn instead of mod_dbd
  - added example for cron snippet for updating the routing database
  - documentation about the newly supported config file

* add debian subdirectory for building Debian packages


Release 1.0 (Mar 31, 2009)
--------------------------

* mod_asn.c:

  - fix bug that lead to ignorance of variables in the subprocess environment
    set by ``ASIPEnvvar``, which falsely looked for the wrong variable name (one
    that was configured via ``ASIPHeader``).

* document an example how to log the looked up data


Older changes
-----------------

Please refer to the subversion changelog: http://svn.mirrorbrain.org/svn/mod_asn/trunk
respectively http://svn.mirrorbrain.org/viewvc/mod_asn/trunk/

