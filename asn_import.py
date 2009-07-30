#!/usr/bin/env python



import os, os.path, sys
import psycopg2
import fileinput

# change these
tablename = 'pfx2asn'
# not needed when a MirrorBrain setup exists
conffile = '/etc/asn_import.conf'

# is there a MirrorBrain config file? If yes, use that.
try:
    import mb.conf
    mirrorbrain_instance = None
    if '-b' in sys.argv:
        mirrorbrain_instance = sys.argv[sys.argv.index('-b') + 1]
        del sys.argv[1:3]
    config = mb.conf.Config(instance = mirrorbrain_instance)
    import mb.conn
    connection = mb.conn.Conn(config.dbconfig)
    cursor = connection.Pfx2asn._connection.getConnection().cursor()
except ImportError:
    # alternatively, is there our own config file?
    if os.path.exists(conffile):
        import ConfigParser
        cp = ConfigParser.SafeConfigParser()
        cp.read(conffile)
        config = dict(cp.items('general'))
        connection = psycopg2.connect("host=%s dbname=%s user=%s password=%s" \
                % (config['host'], config['dbname'], config['user'], config['password']));
        cursor = connection.cursor()
    else:
        print >>sys.stderr, """
Error: No config found. Please create %r with the following contents:

[general]
user = database_user
password = database_password
host = database_server
dbname = name_of_database

""" % conffile
        sys.exit()



# import the AS world
def import_raw():
    try:
        # earlier versions didn't have them separated
        import psycopg2.errorcodes
    except:
        pass

    cursor.execute("begin")
    cursor.execute("delete from %s" % tablename)

    inserted = 0
    for line in fileinput.input():
        pfx, asnb, asn = line.split()
        try:
            cursor.execute("INSERT INTO %s VALUES ( %%s, %%s )" % tablename, [pfx, asn])
            inserted += 1
        except psycopg2.IntegrityError, e:
            print e
            if hasattr(psycopg2, 'errorcodes'):
                unique_violation = psycopg2.errorcodes.UNIQUE_VIOLATION
            else:
                unique_violation = '23505'
            if e.pgcode == unique_violation:
                print 'UNIQUE_VIOLATION for %r; rolling back' % pfx
                cursor.execute("rollback")
        except:
            raise
            sys.exit('insert failed for %s, %s' % (pfx, asn))

    if inserted > 0:
        cursor.execute("commit")
        cursor.execute("vacuum analyze %s" % tablename)
    else:
        sys.exit('nothing imported, no change comitted to the database')

import_raw()

