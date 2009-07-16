
Installation
======================


Prerequirements
------------------------------------

A recent enough version of the Apache HTTP server is required. 2.2.6 or later
should be used. In addition, the apr-util library needs to be 1.3.0 or newer.
This is because the DBD database pool functionality was developed mainly
between 2006 and 2007, and reached production quality at the time.



Installing the ip4r data type into PostgreSQL
----------------------------------------------

You need to install the contributed `ip4r` data type into PostgreSQL. This
project is found at http://ip4r.projects.postgresql.org/. To install it,
a shared library needs to be built to be loaded into PostgreSQL, and an SQL
script needs to be run to make the data type known to PostgreSQL and install
functions that use it.

It would preferable to use a binary package if one exists for your operating
system:

openSUSE/SLE rpm package: 
    http://download.opensuse.org/repositories/server:/database:/postgresql/

The Debian package is called postgresql-8.3-ip4r.

Gentoo portage overlay:
    http://github.com/ramereth/ramereth-overlay/tree

If a manual install is required, you need the PostgresSQL devel package of your
operating system and compile a shared library, following the procedure
described in the installation instructions provided with the software. 

After installing the shared object by package or manual install, you will need
to run the SQL script provided with the ip4r sources::

    su - postgres
    psql -f /usr/share/postgresql-ip4r/ip4r.sql template1

"template1" means that all databases that are created later will have the
datatype available. To install it onto an existing database, use your database
name instead of "template1".

It is normal to see a a good screenful of output printed out by the above
:command:`psql` command.



Creating the database table
------------------------------------

Assuming that a database exists already, execute the following command to
install the pfx2asn table into it. The :file:`asn.sql` file ships with
mod_asn::

    psql -U <dbuser> -f asn.sql <dbname>

If you see some "NOTICE" printed out by the command, that's normal; it's due to
the default logging setup of PostgreSQL which is verbose.

.. note::
   The command creates a table named `pfx2asn` in the <dbname> database. Since
   the table name is used in some other places, so you should not change it.



Config file for the import script
------------------------------------

.. versionadded:: 1.1

If you happen to have a `MirrorBrain <http://mirrorbrain.org/>`_ setup, you'll
have a configuration file named :file:`/etc/mirrorbrain.conf`, which is
automatically used by the :program:`asn_import` script. No further
configuration is needed then. If you have several MirrorBrain instances, the
instance into which to import the data can be selected with the ``-b``
commandline option.

Alternatively, you need to create config file with the database connection
info, named :file:`/etc/asn_import.conf`, looking like this::

    [general]
    user = database_user
    password = database_password
    host = database_server
    dbname = name_of_database


Load the database with routing data
------------------------------------

The data is downloaded and imported into the database with the following
command::

    asn_get_routeviews.py | asn_import.py

It is recommendable to run the command as unprivileged user, for safety
reasons (as any network client).

It will take at least a few minutes to download and process the data - about
30MB are downloaded, and the data is about 1GB uncompressed (beginning of
2009). (In the postgresql database it will again be small.)

The command shown above can be used to update the database with fresh
routeviews data, by just running it again. This is explained in the next
section.


Keep the data up to date
------------------------

The data changes almost constantly, but most of the changes will be microscopic
and won't directly matter to you. However, you should regularly update from
time to time. A weekly (or even monthly) schedule could be entirely sufficient,
depending on what you use the data for.


.. warning::
   You should be aware of the fact that routeview.org kindly provides this data
   to the public, and you should use their bandwidth with consideration. 
   
Therefore, the MirrorBrain project provides a daily mirror at
http://mirrorbrain.org/routeviews/ containing the latest snapshot. This
location is used by the provided scripts.

The same command as you ran initially can be used to update the database with
fresh routeviews data, by just running it again. This works in production while
the database is in active use; it is done in a way that doesn't block any
ongoing connections.

.. note::
   The tarball with the data snapshot will be downloaded only if it doesn't
   exist already in the current working directory. To redownload it, remove the
   file first.

A cron snippet for running the script daily to download and import the data
could look as shown below::

    35 2 * * *   mirrorbrain  sleep $(($RANDOM/16)); asn_get_routeviews | asn_import

If you have a MirrorBrain setup, and possibly several MirrorBrain instances,
you could update each database like this::

    # update ASN data in all MB instances
    35 2 * * *   mirrorbrain  sleep $(($RANDOM/16)); \
                                for i in $(mb instances); do \
                                  asn_get_routeviews | asn_import -b $i; done


The ``sleep`` command serves to randomize the job time a bit, and allows the
example to be used verbatim. Also note that in the example the scripts are
called without the ``.py`` extension.

The data is downloaded to the user's home directory in this case. Make sure the
script runs in a directory where other users don't have write permissions.



Install the Apache module
------------------------------------

There are binary packages of mod_asn at the following locations:

openSUSE/SLE:
    http://download.opensuse.org/repositories/Apache:/MirrorBrain/ 
Debian/Ubuntu:
    http://download.opensuse.org/repositories/Apache:/MirrorBrain/
Gentoo portage overlay:
    http://github.com/ramereth/ramereth-overlay/tree

To manually build mod_asn, all you need to do normally is to use
:program:`apxs2` with -c to compile and -i to install the module::

    apxs2 -ci mod_asn.c

To enable the module to be loaded into Apache, you typically will have to run a
command like the following - depending on your platform::

    a2enmod asn


Configure Apache / mod_dbd
------------------------------------

mod_dbd provides the database connection pool that is used by mod_asn. The
module needs to be loaded into Apache::

    a2enmod dbd

The DBD module needs a database adapter which connects to the database. 

Put the following configuration into server-wide context::

    # configure the dbd connection pool.
    # for the prefork MPM, this configuration is inactive. Prefork simply uses 1
    # connection per child.
    <IfModule !prefork.c>
            DBDMin  0
            DBDMax  32
            DBDKeep 4
            DBDExptime 10
    </IfModule>

As you might note, the cited configuration is relevant for threaded MPMs only.
If you plan to use the prefork MPM, you don't need it. You should however
consider using a threaded MPM if you intend to serve high volumes of requests,
because it will scale better, which is partly due to the fact that the threads
within one process can share a common database pool, which results in fewer
connections that are better utilized, and persistance of connections.

The database driver needs to be configured as well, by putting the following
configuration into *server-wide* **or** *vhost* context. Make the file `chmod
0640` and owned by `root:root`, because it will contain the database password::

    DBDriver pgsql
    DBDParams "host=localhost user=mb password=12345 dbname=mb connect_timeout=15"


Troubleshooting
------------------------------------

If Apache doesn't start, or anything else seems wrong, make sure to check
Apache's error_log. It usually points into the right direction.

A general note about Apache configuration which might be in order. With most
config directives, it is important to pay attention where to put them - the
order does not matter, but the context does. There is the concept of directory
contexts and vhost contexts, which must not be overlooked.  Things can be
"global", or inside a <VirtualHost> container, or within a <Directory>
container.

This matters because Apache applies the config recursively onto subdirectories,
and for each request it does a "merge" of possibly overlapping directives.
Settings in vhost context are merged only when the server forks, while settings
in directory context are merged for each request. This is also the reason why
some of mod_asn's config directives are programmed to be used in one or the
other context, for performance reasons.

The install docs you are reading attempt to always point out in which context
the directives belong.



Configure mod_asn
------------------------------------

.. describe:: ASLookup

Simply set ``ASLookup On`` in the directory context where you want it to be
active. The shipped config (:file:`mod_asn.conf`) shows an example.

.. describe:: ASSetHeaders

Set ``ASSetHeaders Off`` if you don't want the data to be added to the HTTP
response headers. In that case, the lookup result is only available through the
env table for perusal of other Apache modules.

.. describe:: ASIPHeader

The client IP address looked up is the one that the requests originates from.
If mod_asn is running behind a frontend server and can't see the original
client IP address, the frontend may pass the IP via a header and mod_asn can
look at the header instead. You can configure this like below::

    ASIPHeader X-Forwarded-For

.. describe:: ASIPEnvvar

Alternatively, if you need to use mod_rewrite, you can also make mod_asn look
at any variable in Apache's subprocess environment for the IP, for instance::

    ASIPEnvvar CLIENT_IP

.. describe:: ASLookupDebug

``ASLookupDebug`` can be set to ``On`` to switch on debug logging. This can be
done per directory.

.. describe:: ASLookupQuery

You may use the ``ASLookupQuery`` directive (server-wide context) to define a
custom SQL query. The compiled in default is::

  SELECT pfx, asn FROM pfx2asn WHERE pfx >>= ip4r(%s) ORDER BY ip4r_size(pfx) LIMIT 1



Testing
------------------------------------

Once mod_asn is configured, you should be able to verify that it works by doing
some arbitrary request and looking at the response::

     % curl -sI 'http://download.opensuse.org/distribution/11.1/iso/openSUSE-11.1-Addon-Lang-i586.iso' 
    HTTP/1.1 302 Found
    Date: Fri, 26 Jun 2009 22:35:50 GMT
    Server: Apache/2.2.11 (Linux/SUSE)
    X-Prefix: 87.78.0.0/15
    X-AS: 8422
    X-MirrorBrain-Mirror: ftp.uni-kl.de
    X-MirrorBrain-Realm: country
    Location: http://ftp.uni-kl.de/pub/linux/opensuse/distribution/11.1/iso/openSUSE-11.1-Addon-Lang-i586.iso
    Content-Type: text/html; charset=iso-8859-1

(The `X-Prefix` and `X-AS` headers are not present in the response if mod_asn
is configured with ``ASSetHeaders Off``.

When testing with local IP addresses like 192.168.x.x, there's not much to look
up. These addresses are reserved for local use (see :rfc:`1918`). You could
however play with sending X-Forwarded-For headers, provided that you configured
"ASIPHeader X-Forwarded-For", and can lookup arbitrary IPs thereby. You can use
:program:`curl` with the following option, causing it to add an X-Forwarded-For
header with arbitrary value to the request headers::

     % curl -sv -H "X-Forwarded-For: 128.176.216.184" <url>

It can be helpful to set ``ASLookupDebug On`` for some directory - you'll see
every step which the module does being logged to the error_log.



Logging
------------------------------------

Since the data being looked up is stored in the subprocess environment, it is
trivial to log it, by adding the following placeholder to the ``LogFormat``::

    ASN:%{ASN}e P:%{PFX}e


That's it!

Questions, bug reports, patches are welcome at mirrorbrain@mirrorbrain.org.
