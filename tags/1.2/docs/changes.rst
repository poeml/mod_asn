
Release Notes/Change History
============================

Release 1.2 (Jul 27, 2009)
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

